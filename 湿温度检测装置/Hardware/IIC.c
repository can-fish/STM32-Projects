#include "IIC.h"
#include "stm32f10x.h"  

  


void IIC_init(void)
{
		uint32_t i, j;
	for (i = 0; i < 1000; i ++)
	{
		for (j = 0; j < 1000; j ++);
	}
	
	GPIO_InitTypeDef  GPIO_InitStructure; 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;   // OD 开漏输出  进行输入之前 先输出1 就能读取数据输入
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	IIC_W_SCL(1);
	IIC_W_SDA(1);
}


void IIC_W_SCL(u8 x)
{
  GPIO_WriteBit(GPIOB,GPIO_Pin_8,(BitAction)x); // BitAction 只要数据不为 0x00都为1 
	// Delay_us(30);
	
}
void IIC_W_SDA(u8 x)
{
  GPIO_WriteBit(GPIOB,GPIO_Pin_9,(BitAction)x); // BitAction 只要数据不为 0x00都为1 
	// Delay_us(30);
	
}

u8 IIC_R_SDA(void)
{
	 u8 bit =GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13);
 	// Delay_us(30);
	return bit;
}

void IIC_start(void)
{
	IIC_W_SDA(1);
	IIC_W_SCL(1);	 // 先将总线放手

	IIC_W_SDA(0);// 在scl为高电平时拉低SDA产生起始条件
	IIC_W_SCL(0);
	
}

void IIC_stop(void)
{
	IIC_W_SDA(0);
	IIC_W_SCL(1);
	IIC_W_SDA(1);
}


void IIC_setbyte(u8 byte)
{

	for(u8 i=0;i<8;i++){
		
	IIC_W_SDA(!!(byte & (0x80>>i)));
	IIC_W_SCL(1);
	IIC_W_SCL(0);
	
	}
	
	IIC_W_SCL(1);
	IIC_W_SCL(0);
}

u8 IIC_getbyte(void)
{
	IIC_W_SDA(1);
	u8 byte=0x00;
	for(u8 i=0;i<8;i++){
	
		 IIC_W_SCL(1);
		
	if(IIC_R_SDA()==1)
	{
  	byte|=(0x80>>i); 
	}
		IIC_W_SCL(0);
}
	
	return byte;
}


void IIC_setbit(u8 bit)
{
	IIC_W_SDA(bit);
	IIC_W_SCL(1);
	IIC_W_SCL(0);
	
}

u8 IIC_getbit(void)
{
	IIC_W_SDA(1);
	IIC_W_SCL(1);	
	u8 bit= IIC_R_SDA();

	IIC_W_SCL(0);
	return bit;
}


