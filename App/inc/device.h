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



typedef struct
{
	u32 power;
	u32 total; 
	u32 passed;
	u32 seq;
} Device_Info;

#if TEST

enum 
{
	DEVICE_01 = 0,
	DEVICEn,
};

#define DEVICE_PIN                         GPIO_Pin_5
#define DEVICE_GPIO_PORT                   GPIOA
#define DEVICE_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA

#define DEVICE_BUSY_PIN                         GPIO_Pin_6
#define DEVICE_BUSY_GPIO_PORT                   GPIOA
#define DEVICE_BUSY_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA

#define DEVICE_STATUS_PIN                         GPIO_Pin_7
#define DEVICE_STATUS_GPIO_PORT                   GPIOA
#define DEVICE_STATUS_GPIO_PORT_CLK               RCC_APB2Periph_GPIOA

enum 
{
	GPIO_ENABLE = 0,
	GPIO_BUSY,
	GPIO_STATUS,
	GPIOS
};

extern GPIO_TypeDef* GPIO_PORT[DEVICEn][GPIOS];
extern const u16 GPIO_PIN[DEVICEn][GPIOS]; 
extern Device_Info g_device_status[DEVICEn];

#else

enum 
{
	DEVICE_01 = 0,
	DEVICE_02 = 1,
	DEVICE_03 = 2,
	DEVICE_04 = 3,
	DEVICEn,
};

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

extern GPIO_TypeDef* GPIO_PORT[DEVICEn];
extern const u16 GPIO_PIN[DEVICEn]; 
extern Device_Info g_device_status[DEVICEn];

#endif



void Device_Init(void);
void Device_ON(u8 Device);
void Device_OFF(u8 Device);																	 
Device_Power Device_Power_Status(u8 Device);
bool Device_Check_Status(void);
void Device_Timer_Status(char *buf);
bool Device_SendCmd(u8 *cmd, u8 len, u8 *recv, u16 waittime);
void Device_GPIO_Status(char *buf);
bool isAnyDevBusy(void);
bool isDevBusy(u8 Device);
bool isDevWorking(u8 Device);

#endif

