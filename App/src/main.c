/**
  ******************************************************************************
  * @file    Demo/src/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    09/13/2010
  * @brief   Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32F10x.h"
#include "usart.h"
#include "usart3.h"
#include "delay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "SIM800.h"
#include "device.h"


////////////////////////ST官方程序框架需要的变量和函数///////////////////////////////////////////

/**********************程序中断优先级设置******************************************  
                PreemptionPriority        SubPriority
USART3                 0											0

DMA1                   1											0

TIM7									 2											0	

TIM6									 3											0

TIM4                   3                      1

************************************************************************************/

////////////////////////用户程序自定义的变量和函数///////////////////////////////////////////

#define TOTAL_WAIT_ECO  3        
u8 Total_Wait_Echo  =  0;

void Reset_Device_Status(u8 status)
{
	dev.hb_timer = 0;
	dev.reply_timeout = 0;
	dev.msg_timeout = 0;
	//dev.msg_recv = 0;
	dev.msg_expect = 0;
	memset(dev.atcmd_ack, 0, sizeof(dev.atcmd_ack));
	memset(dev.device_on_cmd_string, 0, sizeof(dev.device_on_cmd_string));	
	dev.status = status;	
}

int main(void)
{
	u8 i;
	char sms_data[100];
	//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
	/* Enable GPIOx Clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	delay_init();

#ifdef LOG_ENABLE	
	//注意串口1仅仅用来发送，接收的宏没有使能
	//#define EN_USART1_RX 			   0		//使能（1）/禁止（0）串口1接收
	usart1_init(115200);                            //串口1,Log
#endif
	
	usart3_init(115200);                            //串口3,对接SIM800
	dev.msg_recv = 0;	
	Reset_Device_Status(CMD_NONE);
	//清零USART3_RX_BUF和USART3_RX_STA
	//在使用串口3之前先清零，排除一些意外情况
	Clear_Usart3();
	
	//开机
	//包含2G模块电源芯片的软件使能和2G模块的PWRKEY的使能
	//SIM800_POWER_ON函数是作为急停使用
	SIM800_POWER_ON();  
	SIM800_PWRKEY_ON();
	BSP_Printf("SIM800C开机完成\r\n");
	
	Device_Init();
	//Device_ON(DEVICE_01);
	//Device_ON(DEVICE_04);
	for(i=DEVICE_01; i<DEVICEn; i++)
	{
		BSP_Printf("Power[%d]: %d\n", i, Device_Power_Status(i));
	}
	
	if(SIM800_Link_Server() != CMD_ACK_OK)
	{
		BSP_Printf("INIT: Failed to connect to Server\r\n");
		dev.need_reset = ERR_INIT_LINK_SERVER;
		//while(1){闪烁LED}
	}
	
	BSP_Printf("SIM800C连接服务器完成\r\n");

	if(Send_Login_Data_To_Server() != CMD_ACK_OK)	
	{
		BSP_Printf("INIT: Failed to Send Login Data to Server\r\n");
		dev.need_reset = ERR_INIT_SEND_LOGIN;
		//while(1){闪烁LED}
	}
	BSP_Printf("SIM800C发送登录信息给服务器完成\r\n");

	//程序运行至此，已经成功发送登录信息给服务器，下面开启定时器，等待服务器的回文信息
	//清零串口3的数据，等待接收服务器的数据
	Clear_Usart3();
	
	//开启定时器，每50ms查询一次服务器是否有下发命令
	//主循环TIMER
	TIM6_Int_Init(9999,2399);						     // 1s中断
	TIM_SetCounter(TIM6,0); 
	TIM_Cmd(TIM6,ENABLE);
	
	while(1)
	{
		//BSP_Printf("Main_S Dev Status: %d, Msg expect: %d, Msg recv: %d\r\n", dev.status, dev.msg_expect, dev.msg_recv);
		//BSP_Printf("Main_S HB: %d, HB TIMER: %d, Msg TIMEOUT: %d\r\n", dev.hb_count, dev.hb_timer, dev.msg_timeout);
	
		if(dev.need_reset != ERR_NONE)
		{
			memset(sms_data, 0, sizeof(sms_data));
			SIM800_SMS_Create(sms_data, dev.sms_backup);	
			
			BSP_Printf("开始重启\r\n");	
			TIM_Cmd(TIM7, DISABLE);
			dev.msg_recv = 0;			
			Reset_Device_Status(CMD_NONE);
			//dev.need_reset = FALSE;
			SIM800_Powerkey_Restart(); 
			Clear_Usart3();
			TIM_Cmd(TIM7, ENABLE);	

			//SIM800_SMS_Notif(cell, sms_data);
			if(SIM800_Link_Server() != CMD_ACK_OK)
			{
				BSP_Printf("重启连接服务器失败\r\n");
				dev.need_reset = ERR_RESET_LINK_SERVER;
			}
		}
		else
		{
			switch(dev.status)
			{
				case CMD_LOGIN:
					if((dev.msg_expect == 0) && (dev.status == CMD_LOGIN))
					{
						if(Send_Login_Data_To_Server() != CMD_ACK_OK)
						{
							BSP_Printf("SIM800C发送登录信息给服务器失败\r\n");
							dev.need_reset = ERR_SEND_LOGIN;
						}
						else
						{
							//Clear_Usart3();
							BSP_Printf("SIM800C发送登录信息给服务器完成\r\n");
						}	
					}
				break;
				
				case CMD_HB:					
					if((dev.msg_expect == 0) && (dev.status == CMD_HB))
					{
						BSP_Printf("第  %d  次发送心跳\r\n", dev.hb_count++);
						if(Send_Heart_Data_To_Server() != CMD_ACK_OK)
						{
							BSP_Printf("SIM800C发送心跳信息给服务器失败\r\n");
							dev.need_reset = ERR_SEND_HB;
						}
						else
						{
							//Clear_Usart3();
							BSP_Printf("SIM800C发送心跳信息给服务器完成\r\n");
						}						
					}					
				break;
				
				case CMD_CLOSE_DEVICE:
					if((dev.msg_expect == 0) && (dev.status == CMD_CLOSE_DEVICE))
					{
						if(Send_Close_Device_Data_To_Server() != CMD_ACK_OK)
						{
							BSP_Printf("SIM800C发送设备关闭给服务器失败\r\n");
							dev.need_reset = ERR_SEND_CLOSE_DEVICE;
						}
						else
						{
							//Clear_Usart3();
							BSP_Printf("SIM800C发送设备关闭给服务器完成\r\n");
						}						
					}					
				break;
				
				case CMD_OPEN_DEVICE:
				{
					char *msg_id, *seq, *device, *interfaces, *periods;
					bool interface_on[DEVICEn]={FALSE};
					int period_on[DEVICEn]={0};
					BSP_Printf("cmd string: %s\n", dev.device_on_cmd_string);	
					//根据当前设备状态进行开启(GPIO)，已经开了的就不处理了
					//开启设备并本地计时
					msg_id = strtok(dev.device_on_cmd_string, ",");
					if(msg_id)
					{
						//BSP_Printf("msg_id: %s\n", msg_id);
					}
					device = strtok(NULL, ",");
					if(device)
					{						
						//BSP_Printf("device: %s\n", device);	
					}
					seq = strtok(NULL, ",");
					if(seq)
					{						
						//BSP_Printf("seq: %s\n", seq);
					}					
					interfaces = strtok(NULL, ",");
					if(interfaces)
					{						
						//BSP_Printf("ports: %s\n", interfaces);
					}
					for(i=DEVICE_01; i<DEVICEn; i++)
						interface_on[i]=(interfaces[i]=='1')?TRUE:FALSE;
						
					periods = strtok(NULL, ",");
					if(periods)
					{						
						//BSP_Printf("periods: %s\n", periods);	
					}
					sscanf(periods, "%02d%02d%02d%02d,", &period_on[DEVICE_01], 
						&period_on[DEVICE_02], &period_on[DEVICE_03], &period_on[DEVICE_04]);

					for(i=DEVICE_01; i<DEVICEn; i++)
					{
						if(interface_on[i] && (g_device_status[i].power == OFF))
						{
							g_device_status[i].total = period_on[i] * NUMBER_TIMER_1_MINUTE;
							g_device_status[i].passed = 0;
							g_device_status[i].power = ON;		
							Device_ON(i);
							//BSP_Printf("Port[%d]: %d\n", i, g_device_status[i].total);	
						}			
					}
					memset(dev.device_on_cmd_string, 0, sizeof(dev.device_on_cmd_string));
					
					//发送回文给服务器（这里要有四路设备的状态，并且有设备的已运行时间：Count_Local_Time个50毫秒）
					if(Send_Open_Device_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("发送打开设备回文失败\r\n");
						dev.need_reset = ERR_SEND_OPEN_DEVICE;			
					}
					else
					{
						//Clear_Usart3();
						BSP_Printf("SIM800C发送Enable 回文给服务器完成\r\n");
					}				
				}
								
				break;

				case CMD_TO_IDLE:
					dev.msg_recv = 0;
					Reset_Device_Status(CMD_IDLE);
				break;
				
				default:
				break;
			}
		}
		
		//BSP_Printf("Main_E Dev Status: %d, Msg expect: %d, Msg recv: %d\r\n", dev.status, dev.msg_expect, dev.msg_recv);
		//BSP_Printf("Main_E HB: %d, HB TIMER: %d, Msg TIMEOUT: %d\r\n", dev.hb_count, dev.hb_timer, dev.msg_timeout);
	}
	
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
