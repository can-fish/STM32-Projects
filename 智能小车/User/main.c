#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor1.h"
#include "Motor2.h"
#include "Key.h"

int8_t Speed = 50;		//电机速度变量，范围：-100~100
uint8_t Car_State = 0;	//小车状态变量：0-停止，1-运行

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	Motor1_Init();		//左侧电机驱动模块初始化（第1片TB6612：左前+左后，方向PA4/PA5，PWM=CH1/PA0）
	Key_Init();		//按键初始化
	Motor2_Init();		//右侧电机驱动模块初始化（第2片TB6612：右前+右后，方向PA6/PA7，PWM=CH2/PA1）
						//注意：Timer定时中断模块也占用TIM2，与PWM冲突，二者只能用其一
	
	/*显示静态字符串*/
	OLED_ShowString(3, 1, "State:");	//3行1列显示字符串State:
	OLED_ShowString(3, 8, "Stop");	//3行8列显示初始状态为Stop
	OLED_ShowString(1, 1, "L:");		//1行1列显示字符串L:
	OLED_ShowString(2, 1, "R:");		//2行1列显示字符串R:
	
	while (1)
	{
	
			/*直行：两侧四个电机同速正转*/
			Motor1_SetSpeed(Speed);
			Motor2_SetSpeed(Speed);
			OLED_ShowSignedNum(1, 3, Speed, 3);
	
		
	}

}
