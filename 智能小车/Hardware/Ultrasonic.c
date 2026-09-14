#include "stm32f10x.h"                  // Device header
#include "Ultrasonic.h"
#include "Delay.h"

/*HC-SR04超声波测距驱动（非阻塞）
  接线：TRIG=PB10（推挽输出），ECHO=PB6（TIM4_CH1输入捕获，PB6为5V容忍引脚）
  原理：TRIG给≥10µs高脉冲，ECHO输出高电平宽度=声波往返时间，距离cm=脉宽µs/58
  方案：TIM4计数1µs/格，ARR=0xFFFF（65.5ms窗口>最大回波约23ms，无需处理溢出）
        捕获中断里记录上升沿、切换下降沿，差值即脉宽；65.5ms仍无回波判超时
  注意：TRIG原计划PB5，因按键1已占用PB5，改用PB10*/

static volatile uint16_t Distance = DISTANCE_INVALID;	//最近一次测距结果（cm）
static volatile uint8_t EchoPending = 0;				//1=已触发，等待回波
static volatile uint16_t RiseTime = 0;					//上升沿时刻（µs）

/**
  * 函    数：超声波初始化
  * 参    数：无
  * 返 回 值：无
  */
void Ultrasonic_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	/*TRIG=PB10推挽输出，初始低电平*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_10);

	/*ECHO=PB6上拉输入*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*TIM4时基：1µs/格，65.5ms窗口*/
	TIM_InternalClockConfig(TIM4);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

	/*TIM4_CH1输入捕获，初始捕获上升沿*/
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x00;
	TIM_ICInit(TIM4, &TIM_ICInitStructure);

	TIM_ITConfig(TIM4, TIM_IT_CC1 | TIM_IT_Update, ENABLE);

	/*NVIC：捕获中断用最高抢占优先级，保证脉宽测量不受干扰
	  （硬件捕获时间戳不受中断延迟影响，中断只需及时切极性）*/
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM4, ENABLE);
}

/**
  * 函    数：发起一次测距
  * 参    数：无
  * 返 回 值：无
  * 说    明：非阻塞，TRIG高电平15µs由Delay_us保证
  *           两次触发间隔应≥60ms，避免上次回波串扰
  */
void Ultrasonic_Trigger(void)
{
	TIM_SetCounter(TIM4, 0);						//计数清零，使65.5ms窗口从本次触发起算
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update | TIM_IT_CC1);
	EchoPending = 1;

	GPIO_SetBits(GPIOB, GPIO_Pin_10);				//TRIG拉高15µs
	Delay_us(15);
	GPIO_ResetBits(GPIOB, GPIO_Pin_10);
}

/**
  * 函    数：获取最近一次测距结果
  * 返 回 值：距离（cm）；DISTANCE_INVALID(0xFFFF)=超时/无回波/尚未触发
  */
uint16_t Ultrasonic_GetDistance(void)
{
	return Distance;
}

/**
  * 函    数：TIM4中断函数
  * 说    明：CC1捕获中断里记录上升沿、切换下降沿，差值即回波脉宽；
  *           更新中断（65.5ms窗口到）时若仍在等待回波则判超时
  */
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_CC1) == SET)
	{
		if (EchoPending)
		{
			uint16_t CCR = TIM_GetCapture1(TIM4);

			if ((TIM4->CCER & TIM_CCER_CC1P) == 0)		//本次捕获是上升沿
			{
				RiseTime = CCR;
				TIM4->CCER |= TIM_CCER_CC1P;			//切换为下降沿捕获
			}
			else										//本次捕获是下降沿
			{
				uint16_t Width = CCR - RiseTime;		//无符号减法，窗口内自动处理回绕
				Distance = Width / 58;					//µs换算cm（声速343m/s）
				EchoPending = 0;
				TIM4->CCER &= ~TIM_CCER_CC1P;			//切回上升沿捕获
			}
		}
		TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);
	}

	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
	{
		if (EchoPending)								//窗口耗尽仍无回波：超时
		{
			Distance = DISTANCE_INVALID;
			EchoPending = 0;
			TIM4->CCER &= ~TIM_CCER_CC1P;				//恢复上升沿捕获
		}
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	}
}
