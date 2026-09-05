#include "stm32f10x.h"                  
#include "Delay.h"
#include "dht11.h"
#include "FMQ.h"
#include "OLED.h"




int main(void)
{
	//开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	
	
	/*GPIO初始化*/
	//定义结构体变量
	GPIO_InitTypeDef GPIO_InitStructure;					
	//GPIO模式，赋值为推挽输出模式
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
	//GPIO引脚，赋值为第1，12，15号引脚
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_12|GPIO_Pin_15;	
	
	//GPIO速度，赋值为50MHz
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//GPIO初始默认高电平
	GPIO_SetBits(GPIOB, GPIO_Pin_1);	
	
	//外设初始化
	OLED_init();
	DHT11_Init();
	
	fmq_Init();
	u8 temp,humi;
	
	//OLED屏幕显示注释
	OLED_showchinese(0,0,"温度：");
	OLED_showchinese(80,0,"℃");
	OLED_showchinese(0,2,"湿度：");
	OLED_showstring	(80,2,"RH");
	
	
	while(1)
	{
		//存储数据
		DHT11_Read_Data(&temp,&humi);
		//OLED显示湿温度数据
		OLED_shownumber(50,0,temp,2);	
		OLED_shownumber(50,2,humi,2);	
			
			
			if(temp<20||temp>30||humi<40||humi>85)
			{
					//将PB1引脚设置为低电平，蜂鸣器鸣叫
					GPIO_ResetBits(GPIOB, GPIO_Pin_1);	
					GPIO_ResetBits(GPIOB, GPIO_Pin_12);	
					GPIO_SetBits(GPIOB, GPIO_Pin_15);	
									
			}
			else 
			{
					//将PB1引脚设置为高电平，蜂鸣器停止	
					GPIO_SetBits(GPIOB, GPIO_Pin_1);
					GPIO_ResetBits(GPIOB, GPIO_Pin_15);	
					GPIO_SetBits(GPIOB, GPIO_Pin_12);	
			}
		Delay_ms(500);
	}
}
