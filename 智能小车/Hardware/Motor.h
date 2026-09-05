#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"		//包含设备头文件，确保int8_t等类型已定义，
							//避免在未先包含stm32f10x.h的文件里包含本文件时报“int8_t未定义”

void Motor_Init(void);
void Motor1_SetSpeed(int8_t Speed);
void Motor2_SetSpeed(int8_t Speed);

#endif
