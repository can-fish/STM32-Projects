#include "stm32f10x.h"                  // Device header
#include "Motor2.h"
#include "PWM.h"

/**
  * 函    数：右侧电机驱动模块初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：对应第2片TB6612，A通道接右前电机，B通道接右后电机
  *           接线：AIN1与BIN1并联接PA6，AIN2与BIN2并联接PA7（方向控制）
  *           PWMA与PWMB并联接PA1，即TIM2_CH2（速度控制）；STBY接VCC
  */
void Motor2_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA6、PA7引脚初始化为推挽输出，控制右侧两电机的方向

	PWM_Init();					//初始化底层PWM，开通TIM2的CH1~CH4四个输出通道
}								//PWM_Init重复调用无害，两个电机模块可以独立初始化

/**
  * 函    数：右侧电机设置速度（右前、右后两个电机同步动作）
  * 参    数：Speed 要设置的速度，范围：-100~100
  * 返 回 值：无
  */
void Motor2_SetSpeed(int8_t Speed)
{
	if (Speed >= 0)		//如果设置正转的速度值
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_6);	//PA6置高电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_7);	//PA7置低电平，右侧两电机正转
		PWM_SetCompare2(Speed);				//CH2输出PWM，占空比为速度值
	}
	else		//否则，即设置反转的速度值
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_6);	//PA6置低电平
		GPIO_SetBits(GPIOA, GPIO_Pin_7);	//PA7置高电平，右侧两电机反转
		PWM_SetCompare2(-Speed);			//PWM设置为速度的绝对值，因为此时速度值为负数，而PWM只能给正数
	}
}
