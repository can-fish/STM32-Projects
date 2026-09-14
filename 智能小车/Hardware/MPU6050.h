#ifndef __MPU6050_H
#define __MPU6050_H

#include "stm32f10x.h"

void MPU6050_Init(void);
void MPU6050_Calibrate(void);					//上电静止零偏校准（阻塞约1秒，期间小车不能动）
void MPU6050_IntegrateAngle(uint16_t dt_ms);	//主循环每5ms调用一次，积分Z轴角速度
float MPU6050_GetAngle(void);					//获取相对角度（度），左转/逆时针为正
void MPU6050_ResetAngle(void);					//角度清零（每次转弯/航向保持前调用）

#endif
