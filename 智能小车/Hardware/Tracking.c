#include "stm32f10x.h"                  // Device header
#include "Tracking.h"

/**
  * 函    数：红外循迹模块初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：两个TCRT5000循迹模块安装于车头左右两侧，黑线从两模块中间穿过
  *           接线：左模块DO接PB14，右模块DO接PB15，VCC接5V，GND共地，AO悬空不用（数字量方案）
  *           PB14/PB15避开了已占用的PA0/PA1（PWM）、PA4~PA7（电机方向）、PB8/PB9（OLED）、PB1/PB11（按键）
  */
void Tracking_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟

	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;				//上拉输入，模块DO为LM393比较器输出，上拉保证空闲电平稳定
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB14、PB15引脚初始化为上拉输入
}

/**
  * 函    数：获取左侧循迹模块的检测结果
  * 参    数：无
  * 返 回 值：检测到黑线返回1，检测到白色区域返回0
  */
uint8_t Tracking_GetLeft(void)
{
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == TRACKING_DO_BLACK)		//读PB14电平，与黑线电平宏比较
	{
		return 1;					//左侧检测到黑线，说明小车偏右行驶
	}
	return 0;						//左侧为白色区域，小车位置正常
}

/**
  * 函    数：获取右侧循迹模块的检测结果
  * 参    数：无
  * 返 回 值：检测到黑线返回1，检测到白色区域返回0
  */
uint8_t Tracking_GetRight(void)
{
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == TRACKING_DO_BLACK)		//读PB15电平，与黑线电平宏比较
	{
		return 1;					//右侧检测到黑线，说明小车偏左行驶
	}
	return 0;						//右侧为白色区域，小车位置正常
}

/**
  * 函    数：获取循迹状态
  * 参    数：无
  * 返 回 值：循迹状态，取值见Tracking.h中的宏定义：
  *           TRACKING_STRAIGHT：两侧均为白色，小车正常沿黑线直行
  *           TRACKING_LEFT_ON_BLACK：左侧压黑线，小车偏右，需向左修正（左侧减速、右侧全速）
  *           TRACKING_RIGHT_ON_BLACK：右侧压黑线，小车偏左，需向右修正（右侧减速、左侧全速）
  *           TRACKING_ALL_BLACK：两侧均压黑线，十字路口或脱线，当前框架仅上报状态，不做处理
  */
uint8_t Tracking_GetState(void)
{
	uint8_t Left = Tracking_GetLeft();		//读取左侧模块的检测结果
	uint8_t Right = Tracking_GetRight();	//读取右侧模块的检测结果

	if (Left == 0 && Right == 0)			//两侧均为白色区域
	{
		return TRACKING_STRAIGHT;			//小车正常沿黑线直行
	}
	else if (Left == 1 && Right == 0)		//仅左侧压到黑线
	{
		return TRACKING_LEFT_ON_BLACK;		//小车偏右行驶，需向左修正
	}
	else if (Left == 0 && Right == 1)		//仅右侧压到黑线
	{
		return TRACKING_RIGHT_ON_BLACK;		//小车偏左行驶，需向右修正
	}
	else									//两侧均压到黑线
	{
		return TRACKING_ALL_BLACK;			//十字路口或脱线，留待扩展
	}
}
