#ifndef __OLED_DATA_H
#define __OLED_DATA_H

#include <stdint.h>

typedef struct 
{
	char Index[4];	
	uint8_t Data[32];					
} chinese;


extern const uint8_t OLED_F8x16[][16];


extern const chinese OLED_CF16x16[];



#endif



