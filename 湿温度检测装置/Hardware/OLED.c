#include "IIC.h"
#include "stm32f10x.h"  
#include "OLED.h"
#include "OELD_Data.h"
#include "string.h"

extern const uint8_t OLED_F8x16[][16];
extern const chinese OLED_CF16x16[];


void OLED_shownumber(u8 x, u8 y,u8 number,u8 length)
{
	for(u8 i=0;i<length;i++){
		OLED_showchar(x+i*8,y,number/OLED_pow(10,length - i -1)%10 + '0');
	}
}

u8 OLED_pow(u8 x,u8 y)
{
		u8 i=1;
	while(y--)
	{
	 i*=x;
	}
	return i;
}


void OLED_showimage(u8 x, u8 y,u8 width,u8 hegiht,const u8 *image)
{
	for(u8 j=0;j<hegiht;j++)
	{
			OLED_setcursor(x,y+j);
		
	for(u8 i=0;i<width;i++){
		
	OLED_W_data(image[j*width+i]);
	}
 }
}

void OLED_showchinese(u8 x, u8 y,char * chinese)
{
	u8 i=0;
	char singlechinese[4]={0};
	u8 schinese =0;
	u8 sindex=0;
	
	while(chinese[i]!='\0')
	{
		singlechinese[schinese]=chinese[i];
		schinese++;
		i++;

		
		if(schinese>=3)
		{
		schinese=0;
		for(sindex=0;strcmp(OLED_CF16x16[sindex].Index,"")!=0;sindex++)
		{
			if(strcmp(OLED_CF16x16[sindex].Index,singlechinese)==0)
			{
			break;
			}
		}
		OLED_showimage(x + ((i + 1) / 3-1) * 16,y,16,2,OLED_CF16x16[sindex].Data);
		}
	}
}


void OLED_showchar(u8 x, u8 y,char CHAR)
{
		OLED_setcursor(x,y);

	for(u8 i=0;i<8;i++){
	OLED_W_data(OLED_F8x16[CHAR-' '][i]);
	}
	
	OLED_setcursor(x,y+1);
	
		for(u8 i=0;i<8;i++){
	OLED_W_data(OLED_F8x16[CHAR-' '][i+8]);
	}
		
}

void OLED_showstring(u8 x, u8 y,char * STRING)
{
	for(u8 i=0;STRING[i]!='\0';i++)
{
		OLED_showchar(x+i*8,y,STRING[i]);
}

}


void OLED_W_command(u8 command)
{
	IIC_start();
	IIC_setbyte(0x78); // oled的从机地址
	
	IIC_setbyte(0x00); //控制字节 0x00表示 即将写入命令

	IIC_setbyte(command);
	
	IIC_stop();
	
}

void OLED_W_data(u8 data)
{
	IIC_start();
	IIC_setbyte(0x78); // oled的从机地址
	
	IIC_setbyte(0x40); //控制字节 0x40表示 即将写入数据

	IIC_setbyte(data);
	
	IIC_stop();
	
}

void OLED_setcursor(u8 x,u8 y) // y 0~7  x 0~127
{
	OLED_W_command(0xB0 | y); 							//设置页地址
	OLED_W_command(0x10| ((0xF0 & x)>>4)); 	//设置x位置的高四位
	OLED_W_command(0x00 |(0x0F & x));				//设置x位置的低四位
}

void OLED_clear(void)
{
	for(u8 j=0;j<8;j++)
	{
	OLED_setcursor(0,j);
	
	for(u8 i=0;i<128;i++)
	{
	OLED_W_data(0x00);
	}
}
}


void OLED_init(void)
{
	IIC_init();
	
	/*写入一系列的命令，对OLED进行初始化配置*/
	OLED_W_command(0xAE);	//设置显示开启/关闭，0xAE关闭，0xAF开启
	
	OLED_W_command(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_W_command(0x80);	//0x00~0xFF
	
	OLED_W_command(0xA8);	//设置多路复用率
	OLED_W_command(0x3F);	//0x0E~0x3F
	
	OLED_W_command(0xD3);	//设置显示偏移
	OLED_W_command(0x00);	//0x00~0x7F
	
	OLED_W_command(0x40);	//设置显示开始行，0x40~0x7F
	
	OLED_W_command(0xA1);	//设置左右方向，0xA1正常，0xA0左右反置
	
	OLED_W_command(0xC8);	//设置上下方向，0xC8正常，0xC0上下反置

	OLED_W_command(0xDA);	//设置COM引脚硬件配置
	OLED_W_command(0x12);
	
	OLED_W_command(0x81);	//设置对比度
	OLED_W_command(0xCF);	//0x00~0xFF

	OLED_W_command(0xD9);	//设置预充电周期
	OLED_W_command(0xF1);

	OLED_W_command(0xDB);	//设置VCOMH取消选择级别
	OLED_W_command(0x30);

	OLED_W_command(0xA4);	//设置整个显示打开/关闭

	OLED_W_command(0xA6);	//设置正常/反色显示，0xA6正常，0xA7反色

	OLED_W_command(0x8D);	//设置充电泵
	OLED_W_command(0x14);

	OLED_W_command(0xAF);	//开启显示

  OLED_clear();
}

