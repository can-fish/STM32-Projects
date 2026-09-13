#ifndef __TRACKING_H
#define __TRACKING_H

#include "stm32f10x.h"		//Device header

/*电平定义：TCRT5000五路循迹模块OUT引脚电平与检测结果的对应关系（按本模块实测标定）
  本模块：检测到白色（接收到红外反射）时OUT输出高电平，检测到黑色（无反射）时OUT输出低电平
  若更换模块后极性相反，将下面两个宏的值对调即可；也可调节模块上的电位器改变比较阈值*/
#define TRACKING_DO_BLACK	0		//OUT为低电平：检测到黑线（实测本模块黑线输出低电平）
#define TRACKING_DO_WHITE	1		//OUT为高电平：检测到白色（接收到红外反射）

/*位定义：Tracking_Read()返回值的各位与五路探头的对应关系（以车头方向看）
  注意：OUT接线为镜像对调——OUT1(物理最左)接PA12、OUT2接PA11、OUT3接PA10、
  OUT4接PA9、OUT5(物理最右)接PA8，用于补偿电机驱动模块左右接反：
  bit0=物理最右探头  bit1=右二  bit2=中间  bit3=左二  bit4=物理最左探头
  每一位为1表示该路检测到黑线，为0表示白色区域
  因此代码里的"左/右"与物理左右相反，OLED显示同理（挡物理最左探头显示bit0=1）
  若日后把电机模块接线改正（Motor1=物理左侧），五根OUT线需换回正序
  （OUT1→PA8……OUT5→PA12），否则方向又会镜像回去*/

void Tracking_Init(void);
uint8_t Tracking_Read(void);
int8_t Tracking_GetError(void);

#endif
