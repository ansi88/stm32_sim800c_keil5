#include "string.h"  
#include "stdio.h"	
#include "usart.h" 
#include "usart3.h" 
#include "timer.h"
#include "delay.h"
#include <stdlib.h>
#include "device.h"
#include "sim800_tp.h"
#include "queue.h"

#define RETRY_COUNT 3

u8 mode = 0;				//0,TCP连接;1,UDP连接
const char *modetbl[2] = {"TCP","UDP"};//连接模式

//const char  *ipaddr = "42.159.117.91";
const char  *ipaddr = "116.62.187.167";
//const char  *ipaddr = "42.159.107.250";
const char  *port = "8089";
const char delim=',';
const char ending='#';

char  *cell = "13910138465";

char iccid[LENGTH_ICCID_BUF+1] = {0};
uint32_t  lastOutActivity=0;
uint32_t  lastInActivity=0;
t_DEV dev;
extern void Reset_Device_Status(void);

const char *msg_id[MSG_STR_ID_MAX]={"00", "01", "02", "03"};
const char *msg_device="000";

bool SIM800_TP_Send_Cmd(char *cmd,char *ack,char *recv,u16 waittime)
{
	bool ret = FALSE; 

	u3_printf("%s\r\n",cmd);//发送命令

	if(ack&&waittime)		//需要等待应答
	{
		while(waittime!=0)	//等待倒计时
		{ 
			delay_ms(10);
			if(DumpQueue(recv) != NULL)
			{
				// remove crlfl from head
				if (strncmp (recv,"\r\n",2) == 0)
				{
					// shift to left
					for (unsigned int i = 0;( i <= strlen(recv) - 2); ++i)
					{
						recv[i] = recv[i + 2];
					}
				}
				if(strstr(recv, ack))
				{
					ret = TRUE;
					break;
				}
				//No need bad cmd check?
				//if(strstr(recv, Error))
				{
					//ret = FALSE;
					//break;
				}					
			}
			waittime--;	
		}
	}

	return ret;
} 

bool CheckModule(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT","OK",recv,100);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool DisableEcho(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("ATE0","OK",recv,200);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool CheckNetwork(void)
{
	u8 retry = RETRY_COUNT*60;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CREG?","+CREG: 0,1",recv,500);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool CheckSIM(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CPIN?","READY",recv,1000);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool CheckOPS(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+COPS?","CHINA",recv,500);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool GetCSQ(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CSQ","+CSQ:",recv,200);
		if(ret) 
		{
			BSP_Printf("Signal: %d\r\n",atoi(strstr(recv,"+CSQ:")+strlen("+CSQ:")+1));
			break;
		}
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool isValidCCID(char ch)
{
	if (((*str >= '0') && (*str <= '9')) || ((*str >= 'a') && (*str <= 'z')) || ((*str >= 'A') && (*str <= 'Z')))
		return TRUE;

	return FALSE;
}

bool GetICCID(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	u8 i;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CCID","OK",recv,200);
		if(ret) 
		{
			memset(iccid, 0, sizeof(iccid));
			for(i=0; i<LENGTH_ICCID_BUF; i++)
			{
				if(!isValidCCID(recv[i]))
				{
					ret = FALSE;
					break;
				}
				iccid[i]=recv[i];
			}
			if(i==LENGTH_ICCID_BUF)
			{
				BSP_Printf("ICCID: %s\r\n",iccid);
				break;
			}
		}
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool SIM800_TP_GPRS_ON(void)
{
	bool ret = FALSE;
	if(ret = Link_Server_AT(0, ipaddr, port))
		dev.need_reset = ERR_NONE;
	
	return ret;
}

bool SIM800_TP_GPRS_OFF(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",recv,500);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool AdhereGPRS(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CGATT=1","OK",recv,1000);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool SetGPRS(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CIPCSGP=1,\"CMNET\"","OK",recv,600);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool DisplayIP(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CIPHEAD=1","OK",recv,300);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool ShutGPRS(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CIPSHUT","SHUT OK",recv,1000);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool SetCGClass(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CGCLASS=\"B\"","OK",recv,300);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool SetCGDCONT(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",recv,600);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool Link_Server_AT(u8 mode,const char* ipaddr,const char *port)
{
	u8 retry = RETRY_COUNT;
	char cmd[100]={0};
	char recv[50];	
	bool ret = FALSE;
	
  	sprintf((char*)cmd,"AT+CIPSTART=\"%s\",\"%s\",\"%s\"",modetbl[mode],ipaddr,port);	

	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd(cmd,"CONNECT",recv,15000);
		if(ret)
		{
			if((strstr(recv,"CLOSED") != NULL)||(strstr(recv,"PDP DEACT") != NULL))
			{
				ret = SIM800_TP_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",recv,500);
				ret = SIM800_TP_Send_Cmd("AT+CIPSHUT","SHUT OK",recv,500);
				ret = FALSE;
			}
			else
			{
				delay_s(2);
				break;
			}
		}
		
		retry--;
	}
		
	return ret;
}

bool SetCMGF(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CMGF=1","OK",recv,1000);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool SetCSMP(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CSMP=17,167,0,0","OK",recv,200);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool SetCSCS(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CSCS=\"GSM\"","OK",recv,200);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool SetCIPMode(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CIPMODE=1","OK",recv,500);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

bool SetCIPCCFG(void)
{
	u8 retry = RETRY_COUNT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Send_Cmd("AT+CIPCCFG=8,2,1024,1,0,1460,50","OK",recv,500);
		if(ret) 
			break;
		delay_s(2);
		retry--;
	}
	
	return ret;
}

void SIM800_TP_POWER_ON(void)
{
	u8 i= 0;
	
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	

	GPIO_SetBits(GPIOB,GPIO_Pin_8);	

	for(i = 0; i < 5; i++)
	{
		delay_s(1);	
	}
}

//关闭2G模块的电源芯片，当做急停按钮来使用
void SIM800_TP_POWER_OFF(void)
{
	u8 i= 0;

	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
	//电源芯片的失能
	GPIO_ResetBits(GPIOB,GPIO_Pin_8);	

	for(i = 0; i < 5; i++)
	{
		delay_s(1);	
	}

}


//通过2G模块的PWRKEY来实现开关机
void SIM800_TP_PWRKEY_ON(void)
{
	u8 i= 0;
	
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	

	//PWRKEY的使能
	GPIO_SetBits(GPIOB,GPIO_Pin_9);	

	for(i = 0; i < 2; i++)
	{
		delay_s(1);	
	}
	//开机控制引脚释放
	GPIO_ResetBits(GPIOB,GPIO_Pin_9);
	for(i = 0; i < 2; i++)
	{
		delay_s(1);		
	}
}

//通过2G模块的PWRKEY来实现开关机
void SIM800_TP_PWRKEY_OFF(void)
{
	u8 i= 0;
	
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	

	//PWRKEY的使能
	GPIO_SetBits(GPIOB,GPIO_Pin_9);	

	for(i = 0; i < 2; i++)
	{
		delay_s(1);	
	}
	//开机控制引脚释放
	GPIO_ResetBits(GPIOB,GPIO_Pin_9);

	for(i = 0; i < 2; i++)
	{
		delay_s(1);	
	}

}

void SIM800_TP_GPRS_Restart(void)
{
	u8 temp = 0;
	SIM800_TP_GPRS_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_s(1);
	}
	SIM800_TP_GPRS_ON();

}

void SIM800_TP_Powerkey_Restart(void)
{
	u8 temp = 0;
	BSP_Printf("Powerkey Restart\r\n");
	SIM800_TP_PWRKEY_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_s(1);
	}
	SIM800_TP_PWRKEY_ON();
}

void SIM800_TP_Power_Restart(void)
{
	u8 temp = 0;
	SIM800_TP_PWRKEY_OFF();
	SIM800_TP_POWER_OFF();
	
	for(temp = 0; temp < 30; temp++)
	{
		delay_s(1);
	}
	SIM800_TP_POWER_ON();
	SIM800_TP_PWRKEY_ON();

}

bool SIM800_TP_Link_Server_AT(void)
{
	bool ret = FALSE;
	if(ret = CheckModule())
		if(ret = DisableEcho())
			if(ret = CheckNetwork())
				if(ret = CheckSIM())
					if(ret = GetCSQ())
						if(ret = GetICCID())
							//if(ret = CheckOPS())
								//if(ret = SIM800_GPRS_OFF())
									if(ret = ShutGPRS())
										if(ret = SetCGClass())
											if(ret = SetCGDCONT())
												if(ret = SetCIPMode())
													if(ret = SetCIPCCFG())
														//if(ret = AdhereGPRS())
															//if(ret = SetGPRS())
																//if(ret = DisplayIP())
																	if(ret = Link_Server_AT(0, ipaddr, port))
																		Reset_Device_Status();

	return ret;
}

bool SIM800_TP_Link_Server_Powerkey(void)
{
	u8 retry = RETRY_COUNT;
	bool ret = FALSE;	
	while(retry != 0)
	{
		ret = SIM800_TP_Link_Server_AT();
		if(ret)
			break;
		SIM800_TP_Powerkey_Restart();
		retry--;
	}

	return ret;

}
bool SIM800_TP_Link_Server(void)
{
	u8 retry = RETRY_COUNT;
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = SIM800_TP_Link_Server_Powerkey();
		if(ret)
			break;
		SIM800_TP_Power_Restart();
		retry--;
	}
	
	return ret;

}

u8 GetUploadStr(u8 msg_str_id, char *msg_str)
{
	MsgDev *msg=(MsgDev*)msg_str;
	char *p_left=msg_str+sizeof(MsgDev);
	u8 Result_Validation = 0;
	u8 i;

	if(msg_str == NULL)
		return 0;

	if(msg_str_id>=MSG_STR_ID_MAX)
		return 0;

	strncpy(msg->header, MSG_STR_DEVICE_HEADER, MSG_STR_LEN_OF_HEADER);
	msg->header[MSG_STR_LEN_OF_HEADER] = delim;
	
	strncpy(msg->id, msg_id[msg_str_id], MSG_STR_LEN_OF_ID);
	msg->id[MSG_STR_LEN_OF_ID] = delim;

  	strncpy(msg->length, "000", MSG_STR_LEN_OF_LENGTH);
	msg->length[MSG_STR_LEN_OF_LENGTH] = delim;
	switch(msg_str_id)
	{
		case MSG_STR_ID_OPEN:
			sprintf(msg->seq,"%03d",dev.msg_seq_s);
		break;
		case MSG_STR_ID_CLOSE:
		case MSG_STR_ID_HB:
		case MSG_STR_ID_LOGIN:
			sprintf(msg->seq,"%03d",++dev.msg_seq);	
		break;
		default:
		break;
	}

	msg->seq[MSG_STR_LEN_OF_SEQ] = delim;

	sprintf(msg->dup, "%02d", 0);
	msg->dup[MSG_STR_LEN_OF_DUP] = delim;

	strncpy(msg->device, msg_device, MSG_STR_LEN_OF_DEVICE);
	msg->device[MSG_STR_LEN_OF_DEVICE] = delim;

	for(i = 0; i < MSG_STR_LEN_OF_PORTS; i++)
	{
		msg->ports[i] = (ON==Device_Power_Status(i))?'1':'0';		
	}	
	msg->ports[MSG_STR_LEN_OF_PORTS] = delim;

	Device_Timer_Status(msg->period);
	msg->period[MSG_STR_LEN_OF_PORTS_PERIOD] = delim;

	for(char *p=msg->preSeq, i = 0; i < MSG_STR_LEN_OF_PORTS; i++)
	{
		sprintf(p, "%03d", g_device_status[i].seq);
		p+=MSG_STR_LEN_OF_SEQ;	
	}	
	msg->preSeq[MSG_STR_LEN_OF_PORTS_SEQ] = delim;

	strncpy(p_left, iccid, LENGTH_ICCID_BUF);
	p_left += LENGTH_ICCID_BUF;
	*p_left++ = delim;

  	sprintf(msg->length,"%03d",strlen(msg_str)-sizeof(msg->header)-sizeof(msg->id)-sizeof(msg->length)+5);
	msg->length[MSG_STR_LEN_OF_LENGTH] = delim;	
	
	//添加校验和
	Result_Validation = CheckSum(msg_str, strlen(msg_str));
	
	//校验值转化为字符串
  	sprintf(p_left,"%03d",Result_Validation);
	p_left += 3;
	*p_left++ = delim;
	*p_left++ = ending;
	*p_left = 0;
	
	return strlen(msg_str);
}

//发送登陆信息给服务器
void SendLogin(void)
{
	char Loginbuf[100]={0};
	if(GetUploadStr(MSG_STR_ID_LOGIN, Loginbuf) != 0)
	{
		lastOutActivity = RTC_GetCounter();
		BSP_Printf("[%0.2d:%0.2d:%0.2d][%d]Login-", lastOutActivity / 3600, (lastOutActivity % 3600) / 60, (lastOutActivity % 3600) % 60, lastOutActivity);		
		u3_printf(Loginbuf);
	}
	dev.reply_timer = 0;
}

//发送心跳包给服务器
void SendHeart(void)
{
	char HBbuf[100]={0};
	if(GetUploadStr(MSG_STR_ID_HB, HBbuf)!=0)
	{
		lastOutActivity = RTC_GetCounter();  /* Compute  hours */		
		BSP_Printf("[%0.2d:%0.2d:%0.2d][%d]HB-", lastOutActivity / 3600, (lastOutActivity % 3600) / 60, (lastOutActivity % 3600) % 60, lastOutActivity);		
		u3_printf(HBbuf);
	}
}

//发送接收业务指令完成回文给服务器
void SendStartAck(void)
{
	char StartAck[100]={0};
	if(GetUploadStr(MSG_STR_ID_OPEN, StartAck)!=0)
	{
		lastOutActivity = RTC_GetCounter();	
		BSP_Printf("[%0.2d:%0.2d:%0.2d][%d]StartAck-", lastOutActivity / 3600, (lastOutActivity % 3600) / 60, (lastOutActivity % 3600) % 60, lastOutActivity);		
		u3_printf(StartAck);
	}
}

//发送业务执行完成指令给服务器
void SendFinish(void)
{
	char FinishBuf[100]={0};
	if(GetUploadStr(MSG_STR_ID_CLOSE, FinishBuf)!=0)
	{
		lastOutActivity = RTC_GetCounter();	
		BSP_Printf("[%0.2d:%0.2d:%0.2d][%d]Finish-", lastOutActivity / 3600, (lastOutActivity % 3600) / 60, (lastOutActivity % 3600) % 60, lastOutActivity);		
		u3_printf(FinishBuf);
	}
	dev.wait_reply = TRUE;
	dev.reply_timer = 0;
}

//////////////异或校验和函数///////
u8 CheckSum(char* pBuf, u16 len)
{
	u8 Sum = 0;
	u8 i = 0;
	Sum = pBuf[0];
	
	for (i = 1; i < len; i++ )
	{
		Sum = (Sum ^ pBuf[i]);
	}
	
	return Sum;
}

bool isWorking(void)
{
	return TRUE;
	//return (GPIO_ReadOutputDataBit(WORK_GPIO_PORT, WORK_PIN)==1?TRUE:FALSE);
}

char *DumpQueue(char * recv)
{
	u16 i=0;
	char *p=recv;
	if(CycQueueIsEmpty(q) == 0 )
	{
		while(CycQueueIsEmpty(q) == 0 ) // CycQueueIsEmpty() return true when queue is empty.
		{
			uint8_t ch = CycQueueOut(q);
			recv[i++] = ch;	
		}
		recv[i]=0;
	}
	else
		p=NULL;

	return p;
}
