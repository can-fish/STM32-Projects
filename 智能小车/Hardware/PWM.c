#include "stm32f10x.h"                  // Device header

/**
  * 函    数：PWM初始化
  * 参    数：无
  * 返 回 值：无
  */
void PWM_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 配置PA0 (TIM2_CH1), PA1 (TIM2_CH2), PA2 (TIM2_CH3), PA3 (TIM2_CH4)为复用推挽输出
	//通道用途：CH1(PA0)接左侧TB6612的PWMA/PWMB，CH2(PA1)接右侧TB6612的PWMA/PWMB
	//CH3(PA2)、CH4(PA3)暂未使用，作备用（可接舵机等）
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA0-PA3引脚初始化为复用推挽输出
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;                 //计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;               //预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元
	
	/*输出比较初始化*/ 
	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
	TIM_OCStructInit(&TIM_OCInitStructure);                         //结构体初始化，若结构体没有完整赋值
																    //则最好执行此函数，给结构体所有成员都赋一个默认值
																	//避免结构体初值不确定的问题
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               //输出比较模式，选择PWM模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       //输出极性，选择为高，若选择极性为低，则输出高低电平取反
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   //输出使能
	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
	
	// 配置TIM2的4个输出比较通道
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);                        //配置TIM2的输出比较通道1
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);                        //配置TIM2的输出比较通道2
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);                        //配置TIM2的输出比较通道3
	TIM_OC4Init(TIM2, &TIM_OCInitStructure);                        //配置TIM2的输出比较通道4
	
	/*自动重载预装载使能*/
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);               //CH1预装载使能
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);               //CH2预装载使能
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);               //CH3预装载使能
	TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);               //CH4预装载使能
	
	TIM_ARRPreloadConfig(TIM2, ENABLE);                             //自动重载预装载使能
	
	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);		//使能TIM2，定时器开始运行
}

/**
  * 函    数：PWM设置CCR通道1
  * 参    数：Compare 要写入的CCR的值，范围：0~100
  * 返 回 值：无
  */
void PWM_SetCompare1(uint16_t Compare)
{
	TIM_SetCompare1(TIM2, Compare);		//设置CCR1的值
}

/**
  * 函    数：PWM设置CCR通道2
  * 参    数：Compare 要写入的CCR的值，范围：0~100
  * 返 回 值：无
  */
void PWM_SetCompare2(uint16_t Compare)
{
	TIM_SetCompare2(TIM2, Compare);		//设置CCR2的值
}

/**
  * 函    数：PWM设置CCR通道3
  * 参    数：Compare 要写入的CCR的值，范围：0~100
  * 返 回 值：无
  */
void PWM_SetCompare3(uint16_t Compare)
{
	TIM_SetCompare3(TIM2, Compare);		//设置CCR3的值
}

/**
  * 函    数：PWM设置CCR通道4
  * 参    数：Compare 要写入的CCR的值，范围：0~100
  * 返 回 值：无
  */
void PWM_SetCompare4(uint16_t Compare)
{
	TIM_SetCompare4(TIM2, Compare);		//设置CCR4的值
}
