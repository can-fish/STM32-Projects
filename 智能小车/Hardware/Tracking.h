#ifndef __TRACKING_H
#define __TRACKING_H

#include "stm32f10x.h"		//Device header

/*电平定义：TCRT5000循迹模块DO引脚电平与检测结果的对应关系
  常见模块：检测到黑色（红外被吸收，无反射）时DO输出高电平，检测到白色（红外反射）时输出低电平
  若实测与上述相反，将下面两个宏的值对调即可；也可调节模块上的电位器改变比较阈值*/
#define TRACKING_DO_BLACK	1		//DO为高电平：检测到黑线
#define TRACKING_DO_WHITE	0		//DO为低电平：检测到白色区域

/*循迹状态定义：Tracking_GetState()的返回值
  设计逻辑：两个循迹模块安装于车头左右两侧，黑线从两模块中间穿过
  两侧均检测到白色区域：小车正常沿黑线直行
  左侧检测到黑线：小车偏右行驶，需向左修正（左侧电机减速、右侧电机全速）
  右侧检测到黑线：小车偏左行驶，需向右修正（右侧电机减速、左侧电机全速）*/
#define TRACKING_STRAIGHT			0x00	//两侧均为白色：小车正常沿黑线直行
#define TRACKING_LEFT_ON_BLACK		0x01	//左侧压到黑线：小车偏右行驶，需向左修正
#define TRACKING_RIGHT_ON_BLACK		0x02	//右侧压到黑线：小车偏左行驶，需向右修正
#define TRACKING_ALL_BLACK			0x03	//两侧均压到黑线：十字路口或脱线，留待后续扩展

void Tracking_Init(void);
uint8_t Tracking_GetLeft(void);
uint8_t Tracking_GetRight(void);
uint8_t Tracking_GetState(void);

#endif
