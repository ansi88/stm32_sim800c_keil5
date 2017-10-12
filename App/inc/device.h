#ifndef __DEVICE_H
#define __DEVICE_H 			   
#include "stm32F10x.h"

#define TEST   1

typedef enum {FALSE = 0, TRUE = !FALSE} bool;

typedef enum 
{
	OFF = 0,
	ON = 1,
	UNKNOWN,	
} Device_Power;

enum 
{
	DEVICE_01 = 0,
	DEVICE_02 = 1,
	DEVICE_03 = 2,
	DEVICE_04 = 3,
	DEVICEn,
};

typedef struct
{
	u32 power;
	u32 total; 
	u32 passed;
	u32 seq;
} Device_Info;

#if TEST
#define DEVICE1_PIN                         GPIO_Pin_4
#define DEVICE1_GPIO_PORT                   GPIOA
#define DEVICE1_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA

  
#define DEVICE2_PIN                         GPIO_Pin_5
#define DEVICE2_GPIO_PORT                   GPIOA
#define DEVICE2_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA

#define DEVICE3_PIN                         GPIO_Pin_6
#define DEVICE3_GPIO_PORT                   GPIOA
#define DEVICE3_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA

#define DEVICE4_PIN                         GPIO_Pin_7
#define DEVICE4_GPIO_PORT                   GPIOA
#define DEVICE4_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA
#else
#define DEVICE1_PIN                         GPIO_Pin_1
#define DEVICE1_GPIO_PORT                   GPIOA
#define DEVICE1_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA

  
#define DEVICE2_PIN                         GPIO_Pin_2
#define DEVICE2_GPIO_PORT                   GPIOA
#define DEVICE2_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA

#define DEVICE3_PIN                         GPIO_Pin_3
#define DEVICE3_GPIO_PORT                   GPIOA
#define DEVICE3_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA

#define DEVICE4_PIN                         GPIO_Pin_4
#define DEVICE4_GPIO_PORT                   GPIOA
#define DEVICE4_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA
#endif

extern GPIO_TypeDef* GPIO_PORT[DEVICEn];
extern const u16 GPIO_PIN[DEVICEn]; 
extern Device_Info g_device_status[DEVICEn];

void Device_Init(void);
void Device_ON(u8 Device);
void Device_OFF(u8 Device);																	 
Device_Power Device_Power_Status(u8 Device);
bool Device_Check_Status(void);
void Device_Timer_Status(char *buf);
bool Device_SendCmd(u8 *cmd, u8 len, u8 *recv, u16 waittime);

#endif

