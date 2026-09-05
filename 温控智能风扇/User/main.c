#include "stm32f10x.h"               			 // Device header

#include "IIC.h"                       			 // IIC接口函数库
#include "OLED.h"                    			 // OLED驱动函数库
#include "LED.h"                      	 		 // LED驱动函数库
#include "Encoder.h"                  	 		 // 旋转编码器驱动函数库
#include "Key.h"                      			 // 按键驱动函数库
#include "Delay.h"                    	 		 // 延时函数库
#include "PWM.h"                      		 	 // PWM驱动函数库
#include "AD.h"                        			 // ADC驱动函数库
#include "Motor.h"                    			 // 电机驱动函数库
#include "math.h"                       		 // 主函数头文件


#define R_UP 10000.0f					//上拉电阻值
#define B 3950.0f						//NTC热敏电阻B值

uint16_t adc_val = 0;					//ADC值变量
float voltage;							//电压变量
float Rntc;							    //NTC电阻变量 
float temp_now;							//当前温度变量
float temp_set = 10.0f;					//设定温度变量
int8_t pwm_duty = 0;					//PWM占空比变量
uint8_t control_mode = 0;				//控制模式：0-温控模式，1-手动模式

int8_t manual_speed = 0;				//手动模式下的电机转速（0到100）
uint8_t key_pressed = 0;				//按键按下标志
int16_t encoder_delta;					//编码器增量变量
#define LED1 1							//LED1标识符，用于温控模式指示
#define LED2 2							//LED2标识符，用于手动模式指示


int main(void)
{
	OLED_Init();						//OLED初始化
	LED_Init();							//LED初始化
	Encoder_Init();						//旋转编码器初始化
	Key_Init();							//按键初始化
	PWM_Init();							//PWM初始化
	AD_Init();							//ADC初始化
	Motor_Init();						//电机初始化

	while(1)
	{
		// 检测按键
		key_pressed = Key_GetNum();
		if(key_pressed == 1)				// 按键1切换控制模式
		{
			control_mode = !control_mode;				// 切换模式
			if(control_mode == 0)
			{
				// 温控模式：LED1亮，LED2灭
				LED1_ON();
				LED2_OFF();
			}
			else
			{
				// 手动模式：LED1灭，LED2亮
				LED1_OFF();
				LED2_ON();
			}
			Delay_ms(200);	// 消抖
		}
		
		// 编码器处理
		if(control_mode == 1)	// 手动模式
		{
			encoder_delta = Encoder_Get();	// 获取编码器增量
			if(encoder_delta != 0)
			{
				// 顺时针旋转加速（上限100），逆时针旋转减速（下限0）
				if(encoder_delta > 0)	// 顺时针旋转，加速
				{
					// 防止溢出，先检查边界
					if(manual_speed <= 100 - encoder_delta)
					{
						manual_speed += encoder_delta;
					}
					else
					{
						manual_speed = 100;  // 直接设置为上限
					}
				}
				else	// 逆时针旋转，减速
				{
					// 防止下溢，先检查边界
					if(manual_speed >= -encoder_delta)
					{
						manual_speed += encoder_delta;
					}
					else
					{
						manual_speed = 0;  // 直接设置为下限
					}
				}
				Motor_SetSpeed(manual_speed);	// 设置电机转速
			}
		}
		else	// 温控模式
		{
			adc_val = AD_GetValue();					//获取ADC值
			voltage = (float)adc_val * 3.3f / 4095.0f;	//计算电压值
			// 防止除零错误，确保电压在有效范围内
			if(voltage > 0.0f && voltage < 3.3f)
			{
				Rntc = R_UP * voltage / (3.3f - voltage);	//计算NTC电阻值
				temp_now = 1.0f / (logf(Rntc / 10000.0f) / B + 1.0f / 298.15f) - 273.15f;	//计算当前温度
			}
			else
			{
				temp_now = 0.0f;  // 电压异常时设置为0度
			}
			// 根据temp_now做逻辑判断，控制风扇
			if (temp_now > temp_set)
			{
				pwm_duty = 50;					//温度高于设定值，风扇全速运转
			}
			else
			{
				pwm_duty = 0;					//温度低于设定值，风扇停止运转
			}
			
			Motor_SetSpeed(pwm_duty);		//设置电机转速
		}
		
		// 确保LED状态正确
		if(control_mode == 0)
		{
			LED1_ON();	// 温控模式：LED1亮
			LED2_OFF();	// 温控模式：LED2灭
		}
		else
		{
			LED1_OFF();	// 手动模式：LED1灭
			LED2_ON();	// 手动模式：LED2亮
		}
		
		// OLED显示
		OLED_ShowString(1, 1, "Temp:");		//在OLED上显示温度
		// 显示温度时使用有符号数显示，支持负温度
		OLED_ShowSignedNum(1, 6, (int32_t)temp_now, 2);	//显示当前温度
		OLED_ShowString(2, 1, "Set :");
		OLED_ShowNum(2, 6, (uint32_t)temp_set, 2);
		
		if(control_mode == 0)
		{
			OLED_ShowString(3, 1, "Mode:Auto");	// 显示温控模式
			OLED_ShowString(4, 1, "Fan :");
			OLED_ShowString(4, 6, pwm_duty ? "ON " : "OFF");
		}
		else
		{
			OLED_ShowString(3, 1, "Mode:Manual");// 显示手动模式
			OLED_ShowString(4, 1, "Speed:");
			OLED_ShowNum(4, 7, (uint32_t)manual_speed, 3);	// 显示手动转速
		}
		
		Delay_ms(200);	// 延时200ms
	}
}
