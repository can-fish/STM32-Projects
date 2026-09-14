#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor1.h"
#include "Motor2.h"
#include "Tracking.h"
#include "Key.h"
#include "Tick.h"
#include "MPU6050.h"
#include "Ultrasonic.h"

/*循迹差速修正参数（正常转弯方案，保持不变）*/
#define TRACK_SMALL_ADJ		15		//小幅偏航修正量：一侧加速此值，另一侧减速此值（2号/4号压线）
#define TRACK_LARGE_ADJ		30		//大幅偏航修正量：在直行速度基础上加减此值（1号/5号压线）

/*避障与转向参数（实车标定）*/
#define AVOID_TRIG_CM		20		//障碍触发距离（cm），连续3次小于此值进入避障
#define AVOID_SPEED			50		//避障各直行段速度
#define FINDLINE_SPEED		40		//找线段速度（低速便于压线捕捉）
#define TURN_KP				1.2f	//定角转弯P系数：轮速 = 角度误差 × KP
#define HOLD_KP				1.5f	//航向保持P系数：左右轮差速 = 角度偏差 × KP
#define TURN_SPEED_MAX		55		//转弯轮速上限
#define TURN_SPEED_MIN		25		//转弯轮速下限（克服静摩擦）
#define ANGLE_TOLERANCE		2.0f	//到位判定：角度误差小于此值（度）
#define SIDE_STEP_MS		700		//横移段时间（≈矩形短边，需实测标定）
#define PASS_MIN_MS			700		//越障段最短时间（保证离开障碍近场）
#define PASS_EXTRA_MS		400		//超声波判定越过障碍后，再直行的余量时间
#define TURN_TIMEOUT_MS		2000	//单次转弯超时保护
#define FINDLINE_TIMEOUT_MS	2500	//找线超时保护
#define TRIG_PERIOD_MS		60		//超声波测距周期（≥60ms防回波串扰）

/*避障状态机状态定义*/
#define CAR_TRACK			0		//循迹行驶
#define CAR_STOP_AVOID		1		//停车确认，停稳
#define CAR_TURN_L90		2		//原地左转90°
#define CAR_SIDE_STEP		3		//横移直行（矩形短边）
#define CAR_TURN_R90A		4		//原地右转90°（第一次）
#define CAR_PASS			5		//直行越过障碍（矩形长边）
#define CAR_TURN_R90B		6		//原地右转90°（第二次）
#define CAR_FIND_LINE		7		//朝黑线直行找线
#define CAR_TURN_L90B		8		//原地左转90°，回到原航向
#define CAR_ERROR			9		//找线失败，停车等待

static const char PhaseChar[] = "T!LARPRFLE";	//各状态在OLED第3行第13列显示的字符

int8_t Speed = 50;		//直行基准速度，范围：0~100
uint8_t Car_State = 0;	//小车状态：0-停止，1-运行（按键1切换）
uint8_t TrackValue;		//五路循迹状态，bit0~bit4对应1~5号探头（从左到右）
int8_t LeftSpeed;		//当前输出到左侧电机的速度（OLED显示用）
int8_t RightSpeed;		//当前输出到右侧电机的速度
uint8_t AvoidCount = 0;	//障碍连续确认计数

uint8_t AvoidState = CAR_TRACK;		//避障状态机当前状态
uint32_t StateTick = 0;				//进入当前状态的时刻（ms）
uint32_t PassClearTick = 0;			//PASS段首次判定越过障碍的时刻
uint32_t LastTrigTick = 0;			//上次超声波触发的时刻
int16_t TurnTarget = 0;				//转弯目标角度（度，左转为正）
uint8_t TurnStable = 0;				//到位保持计数（连续2次小于容差才算到位）

/**
  * 函    数：循迹行驶（正常转弯方案：五路差速修正，保持不变）
  * 说    明：仅3号压线直行；2/1号压线小幅/大幅左转修正；4/5号对称右转修正；
  *           五路全黑（十字）按直行通过；五路全白（丢线）保持上一次输出
  */
void Track_Follow(void)
{
	TrackValue = Tracking_Read();		//读取五路循迹状态，bit0~bit4对应1~5号探头

	if (TrackValue == 0x1F)				//五路全黑：十字路口，按直行通过
	{
		LeftSpeed = Speed;
		RightSpeed = Speed;
	}
	else if (TrackValue & 0x01)			//1号压线：黑线在最左侧，车大幅右偏，大幅左转修正
	{
		LeftSpeed = Speed + TRACK_LARGE_ADJ;
		RightSpeed = Speed - TRACK_LARGE_ADJ;
	}
	else if (TrackValue & 0x02)			//2号压线：黑线偏左，车小幅右偏，小幅左转修正
	{
		LeftSpeed = Speed + TRACK_SMALL_ADJ;
		RightSpeed = Speed - TRACK_SMALL_ADJ;
	}
	else if (TrackValue & 0x04)			//仅3号压线：黑线居中，直行
	{
		LeftSpeed = Speed;
		RightSpeed = Speed;
	}
	else if (TrackValue & 0x08)			//4号压线：黑线偏右，车小幅左偏，小幅右转修正
	{
		LeftSpeed = Speed - TRACK_SMALL_ADJ;
		RightSpeed = Speed + TRACK_SMALL_ADJ;
	}
	else if (TrackValue & 0x10)			//5号压线：黑线在最右侧，车大幅左偏，大幅右转修正
	{
		LeftSpeed = Speed - TRACK_LARGE_ADJ;
		RightSpeed = Speed + TRACK_LARGE_ADJ;
	}
	else								//五路全白，丢线：保持上一次输出，电机维持当前状态
	{
		return;
	}

	Motor1_SetSpeed(LeftSpeed);			//左侧两电机（左前+左后）同步输出
	Motor2_SetSpeed(RightSpeed);		//右侧两电机（右前+右后）同步输出
}

/**
  * 函    数：刹车停止
  */
void Car_Brake(void)
{
	LeftSpeed = 0;
	RightSpeed = 0;
	Motor1_SetSpeed(0);
	Motor2_SetSpeed(0);
}

/**
  * 函    数：航向保持直行（避障状态机专用，离开黑线时用陀螺仪走直线）
  * 参    数：BaseSpeed 直行基准速度
  * 说    明：以进入本段时的朝向为0基准，角度偏正（左偏）则右轮加速修正，反之亦然
  */
void Car_HeadingHold(int8_t BaseSpeed)
{
	float err = -MPU6050_GetAngle();					//角度偏差，正=需向左（逆时针）修正
	int8_t Adj = (int8_t)(err * HOLD_KP);

	LeftSpeed = BaseSpeed + Adj;
	RightSpeed = BaseSpeed - Adj;

	if (LeftSpeed > 100)	LeftSpeed = 100;
	if (LeftSpeed < 0)		LeftSpeed = 0;
	if (RightSpeed > 100)	RightSpeed = 100;
	if (RightSpeed < 0)		RightSpeed = 0;

	Motor1_SetSpeed(LeftSpeed);
	Motor2_SetSpeed(RightSpeed);
}

/**
  * 函    数：定角原地转弯（左右轮反转的第二套转弯方案，陀螺仪闭环）
  * 返 回 值：0=转弯进行中，1=到位（或超时保护触发）
  * 说    明：以TurnTarget为目标角，P控制轮速；角度误差连续2次小于容差判定到位
  *           误差超TURN_TIMEOUT_MS强制刹停并返回1，防止陀螺仪异常时原地打转
  */
uint8_t Car_TurnStep(void)
{
	float err = (float)TurnTarget - MPU6050_GetAngle();
	float abserr = (err > 0) ? err : -err;
	uint32_t Now = Tick_Get();

	if (abserr < ANGLE_TOLERANCE)
	{
		if (++TurnStable >= 2)							//连续2个周期都在容差内
		{
			Car_Brake();
			return 1;
		}
	}
	else
	{
		TurnStable = 0;
	}

	if (Now - StateTick > TURN_TIMEOUT_MS)				//超时保护
	{
		Car_Brake();
		return 1;
	}

	{
		int8_t v = (int8_t)(abserr * TURN_KP);
		if (v < TURN_SPEED_MIN)	v = TURN_SPEED_MIN;
		if (v > TURN_SPEED_MAX)	v = TURN_SPEED_MAX;

		if (err > 0)									//需左转（逆时针）：左侧反转、右侧正转
		{
			LeftSpeed = -v;
			RightSpeed = v;
		}
		else											//需右转（顺时针）：左侧正转、右侧反转
		{
			LeftSpeed = v;
			RightSpeed = -v;
		}
		Motor1_SetSpeed(LeftSpeed);
		Motor2_SetSpeed(RightSpeed);
	}
	return 0;
}

/**
  * 函    数：进入新的避障状态
  */
void Car_GotoState(uint8_t NewState)
{
	AvoidState = NewState;
	StateTick = Tick_Get();
	TurnStable = 0;
}

/**
  * 函    数：避障状态机（每50ms执行一步）
  * 流    程：循迹→停稳→左转90°→横移→右转90°→越障→右转90°→找线→左转90°→循迹
  */
void Car_ControlStep(void)
{
	uint32_t Now = Tick_Get();

	switch (AvoidState)
	{
		case CAR_TRACK:									//循迹行驶（正常转弯方案，不变）
			Track_Follow();
			if (Now - LastTrigTick >= TRIG_PERIOD_MS)	//周期触发超声波测距
			{
				Ultrasonic_Trigger();
				LastTrigTick = Now;
			}
			{
				uint16_t d = Ultrasonic_GetDistance();
				if (d != DISTANCE_INVALID && d < AVOID_TRIG_CM)
				{
					if (AvoidCount < 3)	AvoidCount++;
				}
				else
				{
					AvoidCount = 0;
				}
				if (AvoidCount >= 3)					//连续3次确认有障碍
				{
					Car_Brake();
					Car_GotoState(CAR_STOP_AVOID);
				}
			}
			break;

		case CAR_STOP_AVOID:							//停稳0.3s再动作
			Car_Brake();
			if (Now - StateTick >= 300)
			{
				MPU6050_ResetAngle();					//以当前朝向为0基准
				TurnTarget = 90;						//左转90°
				Car_GotoState(CAR_TURN_L90);
			}
			break;

		case CAR_TURN_L90:								//原地左转90°
			if (Car_TurnStep())
			{
				MPU6050_ResetAngle();
				Car_GotoState(CAR_SIDE_STEP);
			}
			break;

		case CAR_SIDE_STEP:								//横移直行，航向保持
			Car_HeadingHold(AVOID_SPEED);
			if (Now - StateTick >= SIDE_STEP_MS)
			{
				Car_Brake();
				MPU6050_ResetAngle();
				TurnTarget = -90;						//右转90°
				Car_GotoState(CAR_TURN_R90A);
			}
			break;

		case CAR_TURN_R90A:								//原地右转90°
			if (Car_TurnStep())
			{
				MPU6050_ResetAngle();
				PassClearTick = 0;
				Car_GotoState(CAR_PASS);
			}
			break;

		case CAR_PASS:									//沿与黑线平行方向越过障碍
			Car_HeadingHold(AVOID_SPEED);
			if (Now - LastTrigTick >= TRIG_PERIOD_MS)
			{
				Ultrasonic_Trigger();
				LastTrigTick = Now;
			}
			if (Now - StateTick >= PASS_MIN_MS)			//先保证最短行驶距离
			{
				uint16_t d = Ultrasonic_GetDistance();
				if (d == DISTANCE_INVALID || d > AVOID_TRIG_CM + 10)	//障碍已不在正前方
				{
					if (PassClearTick == 0)	PassClearTick = Now;
				}
				else
				{
					PassClearTick = 0;
				}
				if ((PassClearTick != 0 && Now - PassClearTick >= PASS_EXTRA_MS)
					|| (Now - StateTick >= 6000))		//超时兜底，防止毛刺导致永远直行
				{
					Car_Brake();
					MPU6050_ResetAngle();
					TurnTarget = -90;
					Car_GotoState(CAR_TURN_R90B);
				}
			}
			break;

		case CAR_TURN_R90B:								//原地右转90°，朝向黑线
			if (Car_TurnStep())
			{
				MPU6050_ResetAngle();
				Car_GotoState(CAR_FIND_LINE);
			}
			break;

		case CAR_FIND_LINE:								//垂直驶向黑线，3号探头压线即停
			Car_HeadingHold(FINDLINE_SPEED);
			TrackValue = Tracking_Read();
			if (TrackValue & 0x04)						//黑线到了
			{
				Car_Brake();
				MPU6050_ResetAngle();
				TurnTarget = 90;						//左转90°回到原航向
				Car_GotoState(CAR_TURN_L90B);
			}
			else if (Now - StateTick >= FINDLINE_TIMEOUT_MS)
			{
				Car_Brake();
				Car_GotoState(CAR_ERROR);				//找不到线，停车等待人工处理
			}
			break;

		case CAR_TURN_L90B:								//原地左转90°，恢复原航向
			if (Car_TurnStep())
			{
				AvoidCount = 0;
				Car_GotoState(CAR_TRACK);				//回到循迹
			}
			break;

		case CAR_ERROR:									//错误状态：停车等待，按键可重新启动
			Car_Brake();
			break;
	}
}

int main(void)
{
	uint8_t KeyNum, i;
	uint32_t Now, LastGyroTick = 0, LastCtrlTick = 0, LastOLEDTick = 0;

	/*模块初始化*/
	OLED_Init();		//OLED初始化（PB8/PB9软件I2C）
	OLED_ShowString(1, 1, "MPU6050 CAL...");
	OLED_ShowString(2, 1, "Keep Still!");	//提示静止校准
	Motor1_Init();		//左侧电机驱动模块初始化
	Motor2_Init();		//右侧电机驱动模块初始化
	Tracking_Init();	//五路红外循迹模块初始化（接线镜像）
	Key_Init();			//按键初始化（Key1=PB5启停，Key2=PB11备用）
	Ultrasonic_Init();	//超声波初始化（TRIG=PB10，ECHO=PB6）
	Tick_Init();		//系统节拍初始化（TIM3，1ms）
	MPU6050_Init();		//MPU6050初始化（与OLED共用I2C总线）
	MPU6050_Calibrate();//零偏校准：阻塞约1秒，期间小车必须静止
	Car_Brake();

	Car_State = 0;		//上电默认停止，按Key1启动
	AvoidState = CAR_TRACK;

	OLED_Clear();
	OLED_ShowString(1, 1, "L:");
	OLED_ShowString(1, 9, "R:");
	OLED_ShowString(2, 1, "D:");
	OLED_ShowString(2, 6, "cm");
	OLED_ShowString(2, 9, "A:");
	OLED_ShowString(3, 1, "State:");
	OLED_ShowString(3, 8, "Stop");
	OLED_ShowString(4, 1, "IR:");
	OLED_ShowString(4, 10, "E:");

	while (1)
	{
		/*按键1：启动/停止切换（按住时Key_GetNum会阻塞到松手，操作时小车应静止）*/
		KeyNum = Key_GetNum();
		if (KeyNum == 1)
		{
			if (Car_State == 0)
			{
				Car_State = 1;
				AvoidCount = 0;
				AvoidState = CAR_TRACK;
				MPU6050_ResetAngle();
			}
			else
			{
				Car_State = 0;
				Car_Brake();
			}
		}

		Now = Tick_Get();

		/*每5ms采样一次陀螺仪并积分角度，dt取实际节拍差值*/
		if (Now - LastGyroTick >= 5)
		{
			MPU6050_IntegrateAngle((uint16_t)(Now - LastGyroTick));
			LastGyroTick = Now;
		}

		if (Car_State == 0)								//停止状态：电机关断，等待按键
		{
			Car_Brake();
			if (Now - LastOLEDTick >= 200)
			{
				LastOLEDTick = Now;
				OLED_ShowString(3, 8, "Stop ");
				OLED_ShowChar(3, 13, '-');
			}
			continue;
		}

		/*每50ms执行一步控制：循迹/避障状态机*/
		if (Now - LastCtrlTick >= 50)
		{
			LastCtrlTick = Now;
			Car_ControlStep();
		}

		/*每200ms刷新显示*/
		if (Now - LastOLEDTick >= 200)
		{
			LastOLEDTick = Now;
			OLED_ShowSignedNum(1, 3, LeftSpeed, 3);		//1行显示左右电机当前速度
			OLED_ShowSignedNum(1, 11, RightSpeed, 3);

			{
				uint16_t d = Ultrasonic_GetDistance();	//2行显示超声波距离与陀螺仪角度
				if (d == DISTANCE_INVALID)	OLED_ShowString(2, 3, "-- ");
				else						OLED_ShowNum(2, 3, d, 3);
			}
			OLED_ShowSignedNum(2, 11, (int32_t)MPU6050_GetAngle(), 3);

			OLED_ShowString(3, 8, "Run ");				//3行显示运行状态与避障阶段
			OLED_ShowChar(3, 13, PhaseChar[AvoidState]);

			for (i = 0; i < 5; i++)						//4行显示五路循迹与偏差
			{
				OLED_ShowNum(4, 4 + i, (TrackValue >> i) & 0x01, 1);
			}
			OLED_ShowSignedNum(4, 12, Tracking_GetError(), 1);
		}
	}
}
