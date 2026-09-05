#ifndef __IIC_H
#define __IIC_H 
#include "stm32f10x.h"  

void IIC_init(void);
void IIC_W_SCL(u8 x);
void IIC_W_SDA(u8 x);
u8 IIC_R_SDA(void);
void IIC_start(void);
void IIC_stop(void);
void IIC_setbyte(u8 byte);
u8 IIC_getbyte(void);
void IIC_setbit(u8 bit);
u8 IIC_getbit(void);

#endif
