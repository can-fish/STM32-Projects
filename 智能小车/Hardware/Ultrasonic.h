#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "stm32f10x.h"

#define DISTANCE_INVALID	0xFFFF		//无效距离：超时/未触发/无回波

void Ultrasonic_Init(void);
void Ultrasonic_Trigger(void);					//发起一次测距（非阻塞）
uint16_t Ultrasonic_GetDistance(void);			//获取距离（cm），0xFFFF表示无效

#endif
