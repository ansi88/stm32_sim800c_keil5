#include "device.h"
#include "timer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "usart.h"
#include "usart2.h"
#include "delay.h"

#if TEST

GPIO_TypeDef* GPIO_PORT[DEVICEn][GPIOS] = { 
	{DEVICE_GPIO_PORT, DEVICE_BUSY_GPIO_PORT, DEVICE_STATUS_GPIO_PORT},
};

const u16 GPIO_PIN[DEVICEn][GPIOS] = {
	{DEVICE_PIN, DEVICE_BUSY_PIN, DEVICE_STATUS_PIN},
};

#else

GPIO_TypeDef* GPIO_PORT[DEVICEn] = { 
	DEVICE1_GPIO_PORT, 
	DEVICE2_GPIO_PORT, 
	DEVICE3_GPIO_PORT,
	DEVICE4_GPIO_PORT
};

const u16 GPIO_PIN[DEVICEn] = {
	DEVICE1_PIN, 
	DEVICE2_PIN, 
	DEVICE3_PIN,
	DEVICE4_PIN
};
#endif

Device_Info g_device_status[DEVICEn];

typedef struct
{
	u16 header;
	u8 len;
	u8 data[1];
} t_Msg;

typedef struct
{
	u16 header;
	u8 status;
	u8 crc;
} t_Rpy;

#define MAGIC_B1          0xAA
#define MAGIC_B2          0x55

void Device_Init(void)
{
 
	GPIO_InitTypeDef  GPIO_InitStructure;
#if TEST 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);	 

	GPIO_InitStructure.GPIO_Pin = DEVICE_PIN;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(DEVICE_GPIO_PORT, &GPIO_InitStructure);	
	Device_OFF(DEVICE_01);

	GPIO_InitStructure.GPIO_Pin = DEVICE_BUSY_PIN | DEVICE_STATUS_PIN;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_Init(DEVICE_BUSY_GPIO_PORT, &GPIO_InitStructure);
#else
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	

	GPIO_InitStructure.GPIO_Pin = DEVICE1_PIN;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(DEVICE1_GPIO_PORT, &GPIO_InitStructure);					
	//GPIO_ResetBits(DEVICE1_GPIO_PORT,DEVICE1_PIN);		
	Device_OFF(DEVICE_01);

	GPIO_InitStructure.GPIO_Pin = DEVICE2_PIN;		
	GPIO_Init(DEVICE2_GPIO_PORT, &GPIO_InitStructure);					
	//GPIO_ResetBits(DEVICE2_GPIO_PORT,DEVICE2_PIN);	
	Device_OFF(DEVICE_02);

	GPIO_InitStructure.GPIO_Pin = DEVICE3_PIN;		
	GPIO_Init(DEVICE3_GPIO_PORT, &GPIO_InitStructure);					
	//GPIO_ResetBits(DEVICE3_GPIO_PORT,DEVICE3_PIN);
	Device_OFF(DEVICE_03);
 
	GPIO_InitStructure.GPIO_Pin = DEVICE4_PIN;		
	GPIO_Init(DEVICE4_GPIO_PORT, &GPIO_InitStructure);					
	//GPIO_ResetBits(DEVICE4_GPIO_PORT,DEVICE4_PIN);
	Device_OFF(DEVICE_04);
#endif

	memset(g_device_status, 0, sizeof(Device_Info)*DEVICEn);
	for(u8 i=0; i<DEVICEn; i++)
		g_device_status[i].seq=0xFF;

}

void Device_OFF(u8 Device)
{
#if TEST
	GPIO_ResetBits(GPIO_PORT[Device][GPIO_ENABLE], GPIO_PIN[Device][GPIO_ENABLE]);	
#else
	GPIO_ResetBits(GPIO_PORT[Device], GPIO_PIN[Device]);						
#endif
}

void Device_ON(u8 Device)
{
#if TEST
	GPIO_SetBits(GPIO_PORT[Device][GPIO_ENABLE], GPIO_PIN[Device][GPIO_ENABLE]);	
#else
	GPIO_SetBits(GPIO_PORT[Device], GPIO_PIN[Device]);						
#endif
}



/*
*********************************************************************************************************
*	函 数 名: Is_Device_On
*	功能说明: 判断Device是否已经开启
*	返 回 值: 1表示已经开启，0表示未开启,2表示调用参数不对
*********************************************************************************************************
*/

Device_Power Device_Power_Status(u8 Device)
{
	Device_Power ret = UNKNOWN;
	if(Device >= DEVICEn)
		return ret;
	
#if TEST
	if ((GPIO_PORT[Device][GPIO_ENABLE]->ODR & GPIO_PIN[Device][GPIO_ENABLE]) != 0)
		ret = ON;
	else
		ret = OFF;
#else
	if ((GPIO_PORT[Device]->ODR & GPIO_PIN[Device]) != 0)
		ret = ON;
	else
		ret = OFF;
#endif

	return ret;

}

void Device_Timer_Status(char *buf)
{
	u8 i;
	//char *p=buf;
	for(i=DEVICE_01; i<DEVICEn; i++)
	{
		//BSP_Printf("Device_Timer_Status Dev[%d].total: %d, passed: %d\n", i, g_device_status[i].total, g_device_status[i].passed);
		sprintf(buf, "%02d%02d",
			(g_device_status[i].total+NUMBER_TIMER_1_MINUTE-1)/NUMBER_TIMER_1_MINUTE,
			(g_device_status[i].passed+NUMBER_TIMER_1_MINUTE-1)/NUMBER_TIMER_1_MINUTE);
		buf+=4;
	}
	//BSP_Printf("Device Status: %s\n", p);
}

//GPIO High: IDLE; Low: BUSY
bool isDevBusy(u8 Device)
{
	return (GPIO_ReadOutputDataBit(GPIO_PORT[Device][GPIO_BUSY], GPIO_PIN[Device][GPIO_BUSY])==1?FALSE:TRUE);	
	//return FALSE;
}

//GPIO High: IDLE; Low: BUSY
bool isAnyDevBusy(void)
{
	bool busy = FALSE;
	for(u8 i=DEVICE_01; i<DEVICEn; i++)
		if(isDevBusy(i))
		{
			busy = TRUE;
			break;
		}
	return busy;
}

//GPIO High: ERROR; Low: WORKING
bool isDevWorking(u8 Device)
{
	return (GPIO_ReadOutputDataBit(GPIO_PORT[Device][GPIO_STATUS], GPIO_PIN[Device][GPIO_STATUS])==1?FALSE:TRUE);	
}

void Device_GPIO_Status(char *buf)
{
	u8 i;
	//char *p=buf;
	for(i=DEVICE_01; i<DEVICEn; i++)
	{
		//BSP_Printf("Device_Timer_Status Dev[%d].total: %d, passed: %d\n", i, g_device_status[i].total, g_device_status[i].passed);
		sprintf(buf, "%d%d%d", Device_Power_Status(i), isDevBusy(i)?1:0, isDevWorking(i)?1:0);
		buf+=GPIOS;
	}
	//BSP_Printf("Device Status: %s\n", p);
}

u8 CheckSum(char* pBuf, u16 len);
//waittime:等待时间(单位:10ms)
bool Device_SendCmd(u8 *cmd, u8 len, u8 *recv, u16 waittime)
{
	bool ret = FALSE; 
	t_Msg *msg=(t_Msg *)malloc(sizeof(t_Msg)+len);  //last byte for crc
	char *p = (char *)USART2_RX_BUF;
	char *end = NULL;		
	msg->header = 0x55AA;
	msg->len = len+1;
	memcpy(msg->data, cmd, len);
	msg->data[len] = CheckSum((char *)msg, sizeof(t_Msg)+len-1);

	Clear_Usart2();
	u2_msg((u8 *)msg, sizeof(t_Msg)+len);
	
	while(waittime!=0)	//等待倒计时
	{ 
		delay_ms(20);
		if((USART2_RX_STA&(1<<15)) != 0)
		{
			end = (USART2_RX_STA & 0x7FFF)+p;
			if(((p = strchr(p, MAGIC_B1)) != NULL) && (end-p>=sizeof(t_Rpy)))
			{
				ret = TRUE;
				break;
			}
			Clear_Usart2();
		}
		waittime--;	
	}

	return ret;
} 
