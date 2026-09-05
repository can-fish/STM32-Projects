#ifndef __MOTOR2_H
#define __MOTOR2_H

#include "stm32f10x.h"		//Device header
							//头文件自身包含设备头文件，确保int8_t等类型已定义
							//避免先包含本文件时出现“int8_t未定义”的报错

void Motor2_Init(void);
void Motor2_SetSpeed(int8_t Speed);

#endif
