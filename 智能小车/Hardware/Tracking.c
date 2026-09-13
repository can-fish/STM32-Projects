#include "stm32f10x.h"                  // Device header
#include "Tracking.h"

/**
  * 函    数：五路红外循迹模块初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：五路TCRT5000循迹模块安装于车头，黑线从中间探头OUT3下方穿过为居中
  *           接线（镜像对调）：OUT1(物理最左)→PA12、OUT2→PA11、OUT3→PA10、OUT4→PA9、
  *           OUT5(物理最右)→PA8，用于补偿电机驱动模块左右接反（详见Tracking.h位定义）
  *           模块VCC接5V（或3.3V），GND与单片机共地，AO悬空不用（数字量方案）
  *           PA8~PA12避开了已占用的PA0/PA1（PWM）、PA4~PA7（电机方向）、PB8/PB9（OLED）、PB1/PB11（按键）
  */
void Tracking_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟

	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;				    //上拉输入，模块OUT为比较器输出，上拉保证空闲电平稳定
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						      //将PA8~PA12引脚初始化为上拉输入
}

/**
  * 函    数：读取五路循迹状态
  * 参    数：无
  * 返 回 值：5位状态值，bit0~bit4对应物理从右到左的五个探头（接线已镜像，见Tracking.h）
  *           每一位为1表示该路检测到黑线，为0表示白色区域
  */
uint8_t Tracking_Read(void)
{
	uint8_t value;
	value = (uint8_t)((GPIO_ReadInputData(GPIOA) >> 8) & 0x1F);		//一次性读出PA8~PA12五个引脚

#if TRACKING_DO_BLACK == 0
	value = (~value) & 0x1F;		//若模块黑线输出低电平，整体取反，统一为1表示黑线
#endif

	return value;
}

/**
  * 函    数：获取黑线相对车体的位置偏差
  * 参    数：无
  * 返 回 值：偏差值，范围：-2~+2
  *           0为黑线居中，负数为黑线偏左（车偏右，需向左修正），正数为黑线偏右（车偏左，需向右修正）
  *           五路全部丢线时返回0；五路全黑（十字或锐角弯）时按最外侧-2处理，
  *           如需特殊处理可在上层用Tracking_Read()的返回值自行判断
  * 注    意：接线已镜像补偿电机左右接反，本函数的左右为代码坐标，
  *           物理左右与其相反（OLED显示同理），见Tracking.h位定义
  */
int8_t Tracking_GetError(void)
{
	uint8_t value = Tracking_Read();

	if (value & 0x01)	return -2;		//bit0压线（物理最右探头），代码坐标判为黑线在最左侧
	if (value & 0x02)	return -1;		//bit1压线（物理右二探头），代码坐标判为黑线偏左
	if (value & 0x04)	return 0;		  //bit2压线（中间探头），黑线居中
	if (value & 0x08)	return 1;		  //bit3压线（物理左二探头），代码坐标判为黑线偏右
	if (value & 0x10)	return 2;	  	//bit4压线（物理最左探头），代码坐标判为黑线在最右侧
	return 0;						          	//五路均未检测到黑线，丢线
}
