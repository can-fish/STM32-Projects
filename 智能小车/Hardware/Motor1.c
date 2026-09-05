#include "stm32f10x.h"                  // Device header
#include "Motor1.h"
#include "PWM.h"

/**
  * 函    数：左侧电机驱动模块初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：对应第1片TB6612，A通道接左前电机，B通道接左后电机
  *           接线：AIN1与BIN1并联接PA4，AIN2与BIN2并联接PA5（方向控制）
  *           PWMA与PWMB并联接PA0，即TIM2_CH1（速度控制）；STBY接VCC
  */
void Motor1_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA4、PA5引脚初始化为推挽输出，控制左侧两电机的方向

	PWM_Init();					//初始化底层PWM，开通TIM2的CH1~CH4四个输出通道
}

/**
  * 函    数：左侧电机设置速度（左前、左后两个电机同步动作）
  * 参    数：Speed 要设置的速度，范围：-100~100
  * 返 回 值：无
  */
void Motor1_SetSpeed(int8_t Speed)
{
	if (Speed >= 0)		//如果设置正转的速度值
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_4);	//PA4置高电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_5);	//PA5置低电平，左侧两电机正转
		PWM_SetCompare1(Speed);				//CH1输出PWM，占空比为速度值
	}
	else		//否则，即设置反转的速度值
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_4);	//PA4置低电平
		GPIO_SetBits(GPIOA, GPIO_Pin_5);	//PA5置高电平，左侧两电机反转
		PWM_SetCompare1(-Speed);			//PWM设置为速度的绝对值，因为此时速度值为负数，而PWM只能给正数
	}
}
