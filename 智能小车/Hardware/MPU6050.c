#include "stm32f10x.h"                  // Device header
#include "MPU6050.h"
#include "Delay.h"

/*MPU6050陀螺仪驱动（软件I2C）
  引脚：与OLED共用PB8(SCL)/PB9(SDA)开漏输出总线，器件地址0x68与OLED不冲突
  方案：读取GYRO_ZOUT做积分得到相对转角（Z轴积分方案，短时任务精度足够）
  注意：OLED与MPU6050的I2C操作都在主循环中顺序执行，不存在总线竞争*/

/*引脚配置（与OLED.c相同：开漏输出，靠模块上拉电阻）*/
#define MPU_W_SCL(x)	GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
#define MPU_W_SDA(x)	GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))

#define MPU6050_ADDRESS		0xD0		//AD0接地的器件写地址（读地址0xD1）
#define GYRO_SENS_LSB		131.0f		//±250dps量程的灵敏度：131 LSB/(°/s)

#define REG_PWR_MGMT_1		0x6B
#define REG_SMPLRT_DIV		0x19
#define REG_CONFIG			0x1A
#define REG_GYRO_CONFIG		0x1B
#define REG_GYRO_ZOUT_H		0x47
#define REG_GYRO_ZOUT_L		0x48

static float Angle = 0.0f;				//积分得到的相对角度（度）
static float GyroZero = 0.0f;			//零偏（°/s），上电静止校准获得

/*软件I2C时序（与OLED.c同一套写法）*/
static void MPU_I2C_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	MPU_W_SCL(1);
	MPU_W_SDA(1);
}

static void MPU_I2C_Start(void)
{
	MPU_W_SDA(1);
	MPU_W_SCL(1);
	MPU_W_SDA(0);
	MPU_W_SCL(0);
}

static void MPU_I2C_Stop(void)
{
	MPU_W_SDA(0);
	MPU_W_SCL(1);
	MPU_W_SDA(1);
}

static uint8_t MPU_I2C_SendByte(uint8_t Byte)
{
	uint8_t i, Ack;
	for (i = 0; i < 8; i++)
	{
		MPU_W_SDA((Byte & (0x80 >> i)) != 0);
		MPU_W_SCL(1);
		MPU_W_SCL(0);
	}
	MPU_W_SDA(1);						//释放SDA，读第9位应答
	MPU_W_SCL(1);
	Ack = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9);
	MPU_W_SCL(0);
	return Ack;
}

static uint8_t MPU_I2C_ReceiveByte(uint8_t AckBit)
{
	uint8_t i, Byte = 0;
	MPU_W_SDA(1);						//释放SDA
	for (i = 0; i < 8; i++)
	{
		MPU_W_SCL(1);
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9))	Byte |= (0x80 >> i);
		MPU_W_SCL(0);
	}
	MPU_W_SDA(AckBit);					//发送应答位
	MPU_W_SCL(1);
	MPU_W_SCL(0);
	return Byte;
}

/*寄存器读写*/
static void MPU_WriteReg(uint8_t Reg, uint8_t Data)
{
	MPU_I2C_Start();
	MPU_I2C_SendByte(MPU6050_ADDRESS);
	MPU_I2C_SendByte(Reg);
	MPU_I2C_SendByte(Data);
	MPU_I2C_Stop();
}

static uint8_t MPU_ReadReg(uint8_t Reg)
{
	uint8_t Data;
	MPU_I2C_Start();
	MPU_I2C_SendByte(MPU6050_ADDRESS);
	MPU_I2C_SendByte(Reg);
	MPU_I2C_Start();
	MPU_I2C_SendByte(MPU6050_ADDRESS | 0x01);
	Data = MPU_I2C_ReceiveByte(1);		//单字节读，回NACK结束
	MPU_I2C_Stop();
	return Data;
}

/**
  * 函    数：MPU6050初始化
  * 参    数：无
  * 返 回 值：无
  */
void MPU6050_Init(void)
{
	MPU_I2C_Init();
	Delay_ms(50);						//上电稳定

	MPU_WriteReg(REG_PWR_MGMT_1, 0x00);	//退出睡眠，选择内部时钟
	MPU_WriteReg(REG_SMPLRT_DIV, 0x07);	//采样率分频
	MPU_WriteReg(REG_CONFIG, 0x03);		//DLPF 44Hz，滤除电机振动干扰
	MPU_WriteReg(REG_GYRO_CONFIG, 0x00);	//陀螺仪量程±250dps
}

/**
  * 函    数：零偏校准
  * 参    数：无
  * 返 回 值：无
  * 说    明：上电后小车必须静止放置约1秒，期间采样Gyro_Z平均值作为零偏
  *           零偏不校准会导致角度持续漂移、转弯角度失准
  */
void MPU6050_Calibrate(void)
{
	uint8_t i;
	int32_t Sum = 0;

	for (i = 0; i < 200; i++)			//200次×5ms ≈ 1秒
	{
		int16_t Raw = (int16_t)((MPU_ReadReg(REG_GYRO_ZOUT_H) << 8) | MPU_ReadReg(REG_GYRO_ZOUT_L));
		Sum += Raw;
		Delay_ms(5);
	}
	GyroZero = (float)Sum / 200.0f / GYRO_SENS_LSB;		//零偏换算为°/s
	Angle = 0.0f;
}

/**
  * 函    数：积分角度（主循环周期调用）
  * 参    数：dt_ms 距上次调用的时间间隔（毫秒，由系统节拍Tick_Get差值提供）
  * 返 回 值：无
  */
void MPU6050_IntegrateAngle(uint16_t dt_ms)
{
	int16_t Raw = (int16_t)((MPU_ReadReg(REG_GYRO_ZOUT_H) << 8) | MPU_ReadReg(REG_GYRO_ZOUT_L));
	float dps = (float)Raw / GYRO_SENS_LSB - GyroZero;

	Angle += dps * (float)dt_ms / 1000.0f;				//角度 += 角速度×时间
}

/**
  * 函    数：获取相对角度
  * 返 回 值：相对上一次清零时刻的角度（度），左转（逆时针）为正
  */
float MPU6050_GetAngle(void)
{
	return Angle;
}

/**
  * 函    数：角度清零
  * 说    明：每次定角转弯或航向保持开始前调用，以当前朝向为0基准
  */
void MPU6050_ResetAngle(void)
{
	Angle = 0.0f;
}
