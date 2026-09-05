#ifndef __OLED_H
#define __OLED_H 
#include "stm32f10x.h"  

void OLED_W_command(u8 command);
void OLED_W_data(u8 data);
void OLED_setcursor(u8 x,u8 y);
void OLED_init(void);
void OLED_clear(void);
void OLED_showchar(u8 x, u8 y,char CHAR);
void OLED_showstring(u8 x, u8 y,char * STRING);
void OLED_showimage(u8 x, u8 y,u8 width,u8 hegiht,const u8 *image);
void OLED_showchinese(u8 x, u8 y,char * chinese);
void OLED_shownumber(u8 x, u8 y,u8 number,u8 length);
u8 OLED_pow(u8 x,u8 y);

#endif
