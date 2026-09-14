#include "stm32f10x.h"                  // Device header
#include "Tick.h"

/*系统节拍：TIM3每1ms产生一次更新中断，TickCount自增
  用途：陀螺仪积分时基、状态机计时、显示降频等，替代主循环里的阻塞延时
  注意：TIM2已被电机PWM占用，TIM3在本工程中专用于系统节拍*/

static volatile uint32_t TickCount = 0;		//毫秒计数，49.7天回绕，无符号减法可正确比较

/**
  * 函    数：系统节拍初始化
  * 参    数：无
  * 返 回 值：无
  */
void Tick_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);		//开启TIM3的时钟

	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;			//ARR，1MHz/1000 = 1kHz
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;			//PSC，72MHz/72 = 1MHz
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);

	TIM_ClearFlag(TIM3, TIM_FLAG_Update);						//清除初始化产生的更新标志
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);					//使能更新中断

	/*NVIC配置（分组2在此统一设定，抢占优先级高于普通控制，低于捕获）*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM3, ENABLE);										//启动节拍
}

/**
  * 函    数：获取当前系统毫秒计数
  * 参    数：无
  * 返 回 值：上电以来的毫秒数（回绕安全：始终用无符号差值比较）
  */
uint32_t Tick_Get(void)
{
	return TickCount;
}

/**
  * 函    数：TIM3中断函数
  * 说    明：仅做计数，保持极短，不影响其他中断响应
  */
void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
	{
		TickCount++;
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}
