#include "FMQ.h"
#include "stm32f10x.h"   


void fmq_Init(void)	
{
	GPIO_InitTypeDef  GPIO_InitStructure; 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;// pp代表普通推挽 输出
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
}

void fmq(int temp,int humi)
{
		if(temp>20|| humi>95)
		{
			GPIO_ResetBits(GPIOB, GPIO_Pin_1);// 置位 0
			
    }
		else
		{
			GPIO_SetBits(GPIOB, GPIO_Pin_1);// 1 置位
		}
}
