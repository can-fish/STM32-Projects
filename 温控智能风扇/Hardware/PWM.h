#ifndef __PWM_H
#define __PWM_H

void PWM_Init(void);
void PWM_SetCompare3(uint16_t Compare);
void PWM_SetDuty(uint16_t Duty);		//设置占空比，范围：0~100

#endif
