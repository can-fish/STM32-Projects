#ifndef __TRACKING_H
#define __TRACKING_H

#include "stm32f10x.h"		//Device header

/*电平定义：TCRT5000五路循迹模块OUT引脚电平与检测结果的对应关系
  常见模块：检测到黑色（红外被吸收，无反射）时OUT输出高电平，检测到白色（红外反射）时输出低电平
  若实测与上述相反，将下面两个宏的值对调即可；也可调节模块上的电位器改变比较阈值*/
#define TRACKING_DO_BLACK	1		//OUT为高电平：检测到黑线
#define TRACKING_DO_WHITE	0		//OUT为低电平：检测到白色区域

/*位定义：Tracking_Read()返回值的各位与五路OUT的对应关系（以车头方向看，从左到右）
  bit0=OUT1(最左)  bit1=OUT2  bit2=OUT3(中间)  bit3=OUT4  bit4=OUT5(最右)
  每一位为1表示该路检测到黑线，为0表示白色区域
  若模块安装方向相反，把接线顺序整体对调（OUT1接PA12……OUT5接PA8）即可，代码无需修改*/

void Tracking_Init(void);
uint8_t Tracking_Read(void);
int8_t Tracking_GetError(void);

#endif
