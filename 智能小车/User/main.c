#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor1.h"
#include "Motor2.h"
#include "Tracking.h"

/*循迹行驶参数，可根据实际赛道情况调整*/
#define TRACK_SMALL_ADJ		15		//小幅偏航修正量：一侧加速此值，另一侧减速此值（2号/4号压线）
#define TRACK_LARGE_ADJ		30		//大幅偏航修正量：在直行速度基础上加减此值（1号/5号压线）

int8_t Speed = 50;		//直行基准速度，范围：0~100，循迹修正在此基础上增减
uint8_t Car_State = 0;	//小车状态变量：0-停止，1-运行
uint8_t TrackValue;		//五路循迹状态，bit0~bit4对应1~5号探头（从左到右）
int8_t LeftSpeed;		//循迹输出的左侧电机速度
int8_t RightSpeed;		//循迹输出的右侧电机速度

/**
  * 函    数：循迹行驶
  * 参    数：无
  * 返 回 值：无
  * 说    明：按五路循迹状态控制左右两侧电机差速（以左转修正为例）：
  *           仅3号压线：黑线居中，两侧同速直行
  *           2号压线：判定小车小幅向右偏航，左侧小幅加速、右侧小幅减速，小幅左转修正
  *           1号压线：判定小车大幅右偏，在2号基础上再次加大差速，大幅左转修正
  *           4号/5号压线：右转修正，与左转对称
  *           五路全黑（十字路口）：按直行通过
  *           五路全白（丢线）：保持上一次的电机输出不变，沿原修正方向找线
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

int main(void)
{
	uint8_t i;				//循环变量

	/*模块初始化*/
	OLED_Init();		//OLED初始化
	Motor1_Init();		//左侧电机驱动模块初始化（第1片TB6612：左前+左后，方向PA4/PA5，PWM=CH1/PA0）
	Motor2_Init();		//右侧电机驱动模块初始化（第2片TB6612：右前+右后，方向PA6/PA7，PWM=CH2/PA1）
	Tracking_Init();	//五路红外循迹模块初始化（OUT1~OUT5依次接PA8~PA12）
						//注意：Timer定时中断模块也占用TIM2，与PWM冲突，二者只能用其一

	Car_State = 1;						//小车进入运行状态，后续可扩展按键启停

	/*显示静态字符串*/
	OLED_ShowString(3, 1, "State:");	//3行1列显示字符串State:
	OLED_ShowString(3, 8, "Run ");		//3行8列显示当前状态为运行
	OLED_ShowString(1, 1, "L:");		//1行1列显示字符串L:
	OLED_ShowString(2, 1, "R:");		//2行1列显示字符串R:
	OLED_ShowString(4, 1, "IR:");		//4行1列显示五路循迹状态，从左到右对应1~5号探头
	OLED_ShowString(4, 10, "E:");		//4行10列显示黑线偏差，负数为黑线偏左

	while (1)
	{
		Track_Follow();										//循迹行驶，根据五路状态差速修正

		OLED_ShowSignedNum(1, 3, LeftSpeed, 3);				//1行显示左侧两电机实际速度
		OLED_ShowSignedNum(2, 3, RightSpeed, 3);			//2行显示右侧两电机实际速度

		for (i = 0; i < 5; i++)
		{
			OLED_ShowNum(4, 4 + i, (TrackValue >> i) & 0x01, 1);	//4行显示1~5号探头，1为压黑线
		}
		OLED_ShowSignedNum(4, 12, Tracking_GetError(), 1);	//4行显示偏差值，范围-2~+2

		Delay_ms(50);										//控制周期约50ms，循迹抖动时可适当减小
	}
}
