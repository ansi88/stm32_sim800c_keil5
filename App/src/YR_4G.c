#include "yr_4g.h"	 	 	
#include "string.h"  
#include "stdio.h"	
#include "usart.h" 
#include "usart2.h"
#include "usart3.h" 
#include "timer.h"
#include "delay.h"
#include <stdlib.h>
#include "queue.h"

enum{
	AT=0,
	AT_Z,
	AT_REBOOT,
	AT_E,
	AT_ENTM,
	AT_WKMOD,
	AT_CMDPW,
	AT_RSTIM,
	AT_CSQ,
	AT_SYSINFO,
	AT_RELD,
	AT_CLEAR,
	AT_CFGTF,
	AT_VER,
	AT_SN,
	AT_ICCID,
	AT_IMEI,

	AT_SOCK_PARAM,
	AT_SOCK_ENABLE,
	AT_SOCK_SL,
	AT_SOCK_LK,
	AT_SOCK_TO,

	AT_SOCK_IND,
	AT_SDPEN,

	AT_REGEN,
	AT_REGTP,
	AT_REGDT,
	AT_REGSND,

	AT_HEARTEN,
	AT_HEARTDT,
	AT_HEARTSND,
	AT_HEARTTM,		
	
	AT_MAX,
};

typedef struct
{
	char *cmd;
	char *ack;
}AtPair;

AtPair atpair[]=
{
	{"", "OK"},
	{"+Z", "OK"},	
	{"+REBOOT", "OK"},	
	{"+E", "+E:"},	
	{"+ENTM", "OK"},
	{"+WKMOD", "+WKMOD:"},
	{"+CMDPW", "+CMDPW:"},	
	{"+RSTIM", "+RSTIM:"},	
	{"+CSQ", "+CSQ:"},	
	{"+SYSINFO", "+SYSINFO:"},		
	{"+RELD", "+RELD:"},
	{"+CLEAR", "OK"},		
	{"+CFGIF", "OK"},	
	{"+VER", "+VER:"},		
	{"+SN", "+SN:"},
	{"+ICCID", "+ICCID:"},	
	{"+IMEI", "+IMEI:"},

	{"", ""},
	{"EN", "EN"},	
	{"SL", "SL"},	
	{"LK", "LK"},	
	{"TO", "TO"},		

	{"+SOCKIND", "+SOCKIND:"},		
	{"+SDPEN", "+SDPEN:"},

	{"+REGEN", "+REGEN:"},
	{"+REGTP", "+REGTP:"},
	{"+REGDT", "+REGDT:"},
	{"+REGSND", "+REGSND:"},

	{"+HEARTEN", "+HEARTEN:"},
	{"+HEARTDT", "+HEARTDT:"},
	{"+HEARTSND", "+HEARTSND:"},
	{"+HEARTTM", "+HEARTTM:"},
	
	{NULL, NULL},
};

enum
{
	ECHO_ON=0,
	ECHO_OFF,
};

char *EchoStatus[]={"ON", "OFF"};

enum
{
	NET_NONETWORK=0,
	NET_GSM,
	NET_WCDMA,
	NET_TDSCDMA,
	NET_LTE,
};

char *Network[]={"No Network", "GSM/GPRS", "WCDMA", "TD-SCDMA", "LTE"};

enum
{
	WKMOD_NET=0,
	WKMOD_HTTPD,
};

char *Wkmod[]={"NET", "HTTPD"};

enum
{
	SOCK_EN=0,
	SOCK_DIS,
};

char *SocketEN[]={"ON", "OFF"};

enum
{
	SOCK_TCP=0,
	SOCK_UDP,
};

char *SocketProtocol[] = {"TCP","UDP"};

enum
{
	SOCK_S=0,
	SOCK_L,
};

char *Socketsl[]={"SHORT", "LONG"};

enum
{
	SOCK_LK=0,
	SOCK_NLK,
};

char *SocketLK[]={"ON", "OFF"};

enum
{
	SOCKIND_ON=0,
	SOCKIND_OFF,
};

char *SocketInd[]={"ON", "OFF"};

enum
{
	REG_EN=0,
	REG_DIS,
};

char *RegEN[]={"ON", "OFF"};

enum
{
	REG_ICCID=0,
	REG_IMEI,
	REG_D2DID,
	REG_CLOUD,	
	REG_USER,	
};

char *RegTP[]={"ICCID", "IMEI", "D2DID", "CLOUD", "USER"};

enum
{
	REG_LINK=0,
	REG_DATA,
};

char *RegSND[]={"LINK", "DATA"};

enum
{
	HEART_EN=0,
	HEART_DIS,
};

char *HeartEN[]={"ON", "OFF"};

const char SocketLabel[SOCK_MAX]={'A', 'B', 'C', 'D'};

const char  *DefaultServer = "116.62.187.167";
const char  *DefaultPort = "8089";
const char delim=',';
const char ending='#';
const char *Error="Err";

#define RETRY_AT 3

char version[LENGTH_VERSION_BUF] = {0};
char wkmod[LENGTH_WKMOD_BUF] = {0};
char password[LENGTH_PASSWORD_BUF] = "usr.cn";
char sysinfo[LENGTH_SYSINFO_BUF] = {0};
u8 network = 0;
char sn[LENGTH_SN_BUF] = {0};
char iccid[LENGTH_ICCID_BUF] = {0};
char imei[LENGTH_IMEI_BUF] = {0};
u16 resetTime=0;
char csq[LENGTH_CSQ_BUF] = {0};
u16 signal=0;
SockSetting socketSetting[SOCK_MAX]={
	{TRUE, FALSE, 0, "LONG", "TCP", "116.62.187.167", "8089", FALSE, NULL, {0}, NULL, FALSE, {0}, NULL, 0},
	{FALSE, FALSE, 0, "LONG", "TCP", "test.usr.cn", "2317", FALSE, NULL, {0}, NULL, FALSE, {0}, NULL, 0},
	{FALSE, FALSE, 0, NULL, NULL, {0}, {0}, FALSE, NULL, {0}, NULL, FALSE, {0}, NULL, 0},
	{FALSE, FALSE, 0, NULL, NULL, {0}, {0}, FALSE, NULL, {0}, NULL, FALSE, {0}, NULL, 0},
};

uint32_t  lastOutActivity=0;
uint32_t  lastInActivity=0;
   
t_DEV dev;
extern void Reset_Device_Status(u8 status);

const char *msg_id[MSG_STR_ID_MAX]={"00", "01", "02", "03"};
const char *msg_device="100";

#define LINKA_PIN                         GPIO_Pin_11
#define LINKA_GPIO_PORT                   GPIOF
#define LINKA_GPIO_PORT_CLK               RCC_APB2Periph_GPIOF

#define LINKB_PIN                         GPIO_Pin_7
#define LINKB_GPIO_PORT                   GPIOF
#define LINKB_GPIO_PORT_CLK               RCC_APB2Periph_GPIOF

#define WORK_PIN                         GPIO_Pin_8
#define WORK_GPIO_PORT                   GPIOF
#define WORK_GPIO_PORT_CLK               RCC_APB2Periph_GPIOF

#define RESET_PIN                         GPIO_Pin_9
#define RESET_GPIO_PORT                   GPIOB
#define RESET_GPIO_PORT_CLK               RCC_APB2Periph_GPIOB

void YR4G_Port_Init(void)
{
 
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);	 

	GPIO_InitStructure.GPIO_Pin = RESET_PIN;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(RESET_GPIO_PORT, &GPIO_InitStructure);					

	GPIO_InitStructure.GPIO_Pin = LINKA_PIN | LINKB_PIN | WORK_PIN;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_Init(LINKA_GPIO_PORT, &GPIO_InitStructure);
}

void trimStr(char *dst, char *src, u8 ackId)
{
	char *tmp=strstr(src, atpair[ackId].ack)+strlen(atpair[ackId].ack);
	while(*tmp == ' ') tmp++;   //delete blank
	if(*tmp == 0)  // All spaces?
	    return;

	strcpy(dst, tmp);	
	tmp = strchr(dst, '\r');
	if(tmp!=NULL)
		*tmp=0;
	tmp = strchr(dst, '\n');
	if(tmp!=NULL)
		*tmp=0;
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

//waittime:等待时间(单位:10ms)
bool YR4G_Send_Cmd(char *cmd,char *ack,char *recv,u16 waittime)
{
	bool ret = FALSE; 
	char cmd_str[100]={0};

	if(!isWorking())
		return ret;
		
	sprintf(cmd_str, "%s%s%s", password, "AT", cmd);
	u3_printf("%s\r\n",cmd_str);//发送命令

	if(ack&&waittime)		//需要等待应答
	{
		while(waittime!=0)	//等待倒计时
		{ 
			delay_ms(10);
			if(DumpQueue(recv) != NULL)
			{
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
	u8 retry = RETRY_AT;
	u8 id=AT;
	char recv[MAXSIZE+1];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
			break;
		delay_ms(1500);
		retry--;
	}
	
	return ret;
	
}

bool GetVersion(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_VER;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			memset(version, 0, sizeof(version));
			trimStr(version, recv, id);
			BSP_Printf("Version: %s\n", version);
			break;
		}
		delay_ms(1500);
		retry--;
	}
	
	return ret;
	
}

bool GetSysinfo(void)
{
	u8 retry = RETRY_AT*60;
	u8 id=AT_SYSINFO;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;
	u8 i=0;
	for(i=0;i<30;i++)
		delay_s(2);
	
	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			for(i=NET_NONETWORK; i<=NET_LTE; i++)
				if(strstr(recv, Network[i])!=NULL)
					break;
				
			if((i==NET_NONETWORK) || (i>NET_LTE))
				ret = FALSE;
			else
			{
				memset(sysinfo, 0, sizeof(sysinfo));
				trimStr(sysinfo, recv, id);
				BSP_Printf("SysInfo[%d]: %s\n", i, sysinfo);
				network =  i;
				break;
			}
		}
		delay_ms(1500);
		retry--;
	}
	
	return ret;
	
}

bool GetPassword(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_CMDPW;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			memset(password, 0, sizeof(password));
			trimStr(password, recv, id);
			BSP_Printf("Password: %s\n", password);
			break;
		}
		delay_ms(1500);
		retry--;
	}

	return ret;
}

bool GetResetTime(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_RSTIM;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			resetTime=atoi(strstr(recv, atpair[id].ack)+strlen(atpair[id].ack));
			BSP_Printf("Reset Time: %d\n", resetTime);
			break;
		}
		delay_ms(1500);
		retry--;
	}

	return ret;
}

bool GetCSQ(void)
{
	u8 retry = RETRY_AT*10;
	u8 id=AT_CSQ;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,1000);
		if(ret) 
		{
			memset(csq, 0, sizeof(csq));
			trimStr(csq, recv, id);
			BSP_Printf("CSQ: %s\n", csq);
			
			signal = atoi(csq)+network*1000;
			BSP_Printf("signal: %d\n", signal);
			break;
		}
		delay_ms(1500);
		retry--;
	}

	return ret;
}

bool GetSN(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_SN;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			memset(sn, 0, sizeof(sn));
			trimStr(sn, recv, id);
			BSP_Printf("SN: %s\n", sn);
			break;
		}
		delay_ms(1500);
		retry--;
	}

	return ret;
}

bool GetICCID(void)
{
	u8 retry = RETRY_AT*3;
	u8 id=AT_ICCID;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			memset(iccid, 0, sizeof(iccid));
			trimStr(iccid, recv, id);
			if(strlen(iccid)>10)
			{
				BSP_Printf("ICCID: %s\n", iccid);
				break;
			}
			else
				ret = FALSE;
		}
		delay_ms(1500);
		retry--;
	}

	return ret;
}

bool GetIMEI(void)
{
	u8 retry = RETRY_AT*3;
	u8 id=AT_IMEI;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,200);
		if(ret) 
		{
			memset(imei, 0, sizeof(imei));
			trimStr(imei, recv, id);
			if(strlen(imei)>10)
			{
				BSP_Printf("IMEI: %s\n", imei);
				break;
			}
			else
				ret = FALSE;			
		}
		
		delay_ms(1500);
		retry--;
	}

	return ret;
}

bool SaveSetting(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_CFGTF;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
			break;
		delay_ms(1500);
		retry--;
	}

	return ret;
}

bool SocketParam(u8 rw, u8 sock, u8 *protocol, u8 *addr, u8 *port, SockSetting *pSocketSetting)
{
	u8 retry = RETRY_AT;
	u8 id=AT_SOCK_PARAM;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;
	char cmd[10], ack[10];
	char *p;

	if(rw==READ)
	{
		sprintf(cmd, "+SOCK%c%s", SocketLabel[sock], atpair[id].cmd);
		sprintf(ack, "+SOCK%c%s:", SocketLabel[sock], atpair[id].ack);
		while(retry != 0)
		{
			ret = YR4G_Send_Cmd(cmd,ack,recv,100);
			if(ret)
			{
				recv[strlen(strstr(recv, ack))-2]=0;
				p=strtok(strstr(recv, ack)+strlen(ack),",");
				if(strstr(p, SocketProtocol[SOCK_TCP]))
					pSocketSetting->protocol=SocketProtocol[SOCK_TCP];
				else
					pSocketSetting->protocol=SocketProtocol[SOCK_UDP];
				
				p=strtok(NULL,",");
				if(p)
					strcpy(pSocketSetting->addr, p);
				p=strtok(NULL,",");
				if(p)
					strcpy(pSocketSetting->port, p);

				BSP_Printf("Sock[%c]: Protocol[%s] Addr[%s] Port[%s]\n", SocketLabel[sock], pSocketSetting->protocol, pSocketSetting->addr, pSocketSetting->port);
				break;
			}
			delay_ms(1500);
			retry--;
		}
	}
	else
	{
		//not implemented yet	
	}
	
	return ret;
}

bool SocketEnable(u8 rw, u8 sock, bool enable, SockSetting *pSocketSetting)
{
	u8 retry = RETRY_AT;
	u8 id=AT_SOCK_ENABLE;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;
	char cmd[10], ack[10];

	if(rw==READ)
	{
		sprintf(cmd, "+SOCK%c%s", SocketLabel[sock], atpair[id].cmd);
		sprintf(ack, "+SOCK%c%s:", SocketLabel[sock],atpair[id].ack);
		while(retry != 0)
		{
			ret = YR4G_Send_Cmd(cmd,ack,recv,100);
			if(ret)
			{
				pSocketSetting->isOn = (strstr(recv, SocketEN[SOCK_EN])!=NULL)?TRUE:FALSE;
				BSP_Printf("Enable[%c]: %d\n", SocketLabel[sock], pSocketSetting->isOn);
				break;
			}
			delay_ms(1500);
			retry--;
		}
	}
	else
	{
		sprintf(cmd, "+SOCK%c%s=%s", SocketLabel[sock], atpair[id].cmd,SocketEN[(enable?SOCK_EN:SOCK_DIS)]);	
		while(retry != 0)
		{
			ret = YR4G_Send_Cmd(cmd,"OK",recv,100);
			if(ret)
			{
				socketSetting[sock].isOn = enable;
				BSP_Printf("Enable[%c]: %d\n", SocketLabel[sock], socketSetting[sock].isOn);
				break;
			}
			delay_ms(1500);
			retry--;
		}		
	}
	
	return ret;
}

bool SocketSL(u8 rw, u8 sock, u8 sl, SockSetting *pSocketSetting)
{
	u8 retry = RETRY_AT;
	u8 id=AT_SOCK_SL;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;
	char cmd[10], ack[10];
	
	if(rw==READ)
	{
		sprintf(cmd, "+SOCK%c%s", SocketLabel[sock], atpair[id].cmd);
		sprintf(ack, "+SOCK%c%s:", SocketLabel[sock],atpair[id].ack);
		while(retry != 0)
		{
			ret = YR4G_Send_Cmd(cmd,ack,recv,100);
			if(ret)
			{
				if(strstr(recv, Socketsl[SOCK_S]))
					pSocketSetting->sl=Socketsl[SOCK_S];
				else
					pSocketSetting->sl=Socketsl[SOCK_L];				
				BSP_Printf("SL[%c]: %s\n", SocketLabel[sock], pSocketSetting->sl);
				break;
			}
			delay_ms(1500);
			retry--;
		}
	}
	else
	{
		//not implemented yet	
	}
	
	return ret;
}

bool isConnected(u8 sock, SockSetting *pSocketSetting)
{
	u8 retry = RETRY_AT;
	u8 id=AT_SOCK_LK;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;
	char cmd[10], ack[10];

	sprintf(cmd, "+SOCK%c%s", SocketLabel[sock], atpair[id].cmd);
	sprintf(ack, "+SOCK%c%s:", SocketLabel[sock],atpair[id].ack);
	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(cmd,ack,recv,100);
		if(ret)
		{
			if(strstr(recv, SocketLK[SOCK_LK]))
				pSocketSetting->isConnected=TRUE;
			else
				pSocketSetting->isConnected=FALSE;				
			BSP_Printf("Connected[%c]: %d\n", SocketLabel[sock], pSocketSetting->isConnected);
			break;
		}
		delay_ms(1500);
		retry--;
	}
	
	return ret;
}

bool SocketTO(u8 sock, SockSetting *pSocketSetting)
{
	u8 retry = RETRY_AT;
	u8 id=AT_SOCK_TO;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;
	char cmd[10], ack[10];

	sprintf(cmd, "+SOCK%c%s", SocketLabel[sock], atpair[id].cmd);
	sprintf(ack, "+SOCK%c%s:", SocketLabel[sock],atpair[id].ack);
	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(cmd,ack,recv,100);
		if(ret)
		{
			pSocketSetting->timeout=atoi(strstr(recv, ack));			
			BSP_Printf("Timerout[%c]: %d\n", SocketLabel[sock], pSocketSetting->timeout);
			break;
		}
		delay_ms(1500);
		retry--;
	}

	
	return ret;
}

bool RegEnable(u8 rw, u8 sock, bool enable, SockSetting *pSocketSetting)
{
	u8 retry = RETRY_AT;
	u8 id=AT_REGEN;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;

	if(rw==READ)
	{
		while(retry != 0)
		{
			ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
			
			if(ret)
			{
				pSocketSetting->isRegEn = (strstr(recv, RegEN[REG_EN])!=NULL)?TRUE:FALSE;
				BSP_Printf("RegEn: %d\n", pSocketSetting->isRegEn);
				break;
			}
			delay_ms(1500);
			retry--;
		}
	}
	else
	{
		while(retry != 0)
		{
			ret = YR4G_Send_Cmd(atpair[id].cmd,"OK",recv,100);
			if(ret)
			{
				socketSetting[sock].isRegEn = enable;
				BSP_Printf("RegEn: %d\n", socketSetting[sock].isRegEn);
				break;
			}
			delay_ms(1500);
			retry--;
		}		
	}
	
	return ret;
}

bool HeartEnable(u8 rw, u8 sock, bool enable, SockSetting *pSocketSetting)
{
	u8 retry = RETRY_AT;
	u8 id=AT_HEARTEN;
	char recv[MAXSIZE+1];	
	bool ret = FALSE;

	if(rw==READ)
	{
		while(retry != 0)
		{
			ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
			if(ret)
			{
				pSocketSetting->isHeartEn = (strstr(recv, HeartEN[REG_EN])!=NULL)?TRUE:FALSE;
				BSP_Printf("HeartEn: %d\n", pSocketSetting->isHeartEn);
				break;
			}
			delay_ms(1500);
			retry--;
		}
	}
	else
	{
		while(retry != 0)
		{
			ret = YR4G_Send_Cmd(atpair[id].cmd,"OK",recv,100);
			if(ret)
			{
				socketSetting[sock].isHeartEn = enable;
				BSP_Printf("HeartEn: %d\n", socketSetting[sock].isHeartEn);
				break;
			}
			delay_ms(1500);
			retry--;
		}		
	}
	
	return ret;
}

char *YR4G_SMS_Create(char *sms_data, char *raw)
{
	//sprintf((char*)sms_data,"Reset Type: %d, Dev Status: %d, Msg expect: %d, Msg recv: %d, HB: %d, HB TIMER: %d, Msg TIMEOUT: %d, Msg: \"%s\", AT-ACK: %s\r\n", dev.need_reset, 
	//	dev.status, dev.msg_expect, dev.msg_recv, dev.hb_count, dev.hb_timer, dev.msg_timeout, raw, dev.atcmd_ack); 
	return sms_data;
}

void YR4G_Reset(void)
{
	u8 i= 0;
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RESET_GPIO_PORT_CLK, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = RESET_PIN ;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(RESET_GPIO_PORT, &GPIO_InitStructure);	

	GPIO_ResetBits(RESET_GPIO_PORT,RESET_PIN);	

	for(i = 0; i < 2; i++)
	{
		delay_s(1);	
	}

	GPIO_SetBits(RESET_GPIO_PORT,RESET_PIN);
	for(i = 0; i < 2; i++)
	{
		delay_s(1);	
	}
}

void YR4G_ResetRestart(void)
{
	char recv[100];	
	BSP_Printf("Reset Restart\r\n");
	YR4G_Reset();
	while(1)
	{
		delay_s(2);
		if(DumpQueue(recv) != NULL)
			if(strstr(recv,YR4G_STARTUP_MSG)!=NULL)
				break;
	}
}

bool YR4G_RecoverSocket(void)
{
	bool ret = FALSE;
	u8 i;

	for(i=SOCK_A; i<SOCK_MAX; i++)
	{
		SockSetting tSockSetting;
		ret = SocketEnable(READ, i, FALSE, &tSockSetting);
		if(tSockSetting.isOn != socketSetting[i].isOn)
			ret = SocketEnable(WRITE, i, socketSetting[i].isOn, NULL);

		ret = RegEnable(READ, i, FALSE, &tSockSetting);
		if(tSockSetting.isRegEn != socketSetting[i].isRegEn)
			ret = RegEnable(WRITE, i, socketSetting[i].isRegEn, NULL);

		ret = HeartEnable(READ, i, FALSE, &tSockSetting);
		if(tSockSetting.isHeartEn != socketSetting[i].isHeartEn)
			ret = HeartEnable(WRITE, i, socketSetting[i].isHeartEn, NULL);
	
		//ret = SocketParam(READ, i, NULL, NULL, NULL, &tSockSetting);
		//ret = isConnected(i, &tSockSetting);
		//ret = SocketSL(READ, i, 0, &tSockSetting);

		if(!ret)
			break;
	}
	return ret;
}

bool YR4G_Link_Server_AT(void)
{
	bool ret = FALSE;
	if(ret = CheckModule())
		if(ret = GetVersion())
			if(ret = GetPassword())
				if(ret = (GetSN() && GetICCID() && GetIMEI()))
					if(ret = GetResetTime())					
						if(ret = GetSysinfo())
							if(ret = GetCSQ())					
								if(ret = YR4G_RecoverSocket())
								{
									ret = FALSE;
									BSP_Printf("Wait Connected\n");	
									for(u8 i=SOCK_A; i<SOCK_MAX; i++)
									{
										if(socketSetting[i].isOn)
										{
#define MAX_TRY  10			
											SockSetting tSockSetting;
											//for(u8 j=0; j<MAX_TRY; j++)
											for(;;)
											{
												delay_s(2);
												isConnected(i, &tSockSetting);
												if(tSockSetting.isConnected)
												{
													ret = TRUE;
													break;
												}
											}
										}
									}
								}

	return ret;
}

bool YR4G_Link_Server(void)
{
	u8 retry = RETRY_AT;
	bool ret = FALSE;
	while(retry != 0)
	{
		if(ret = YR4G_Link_Server_AT())
			break;
		YR4G_ResetRestart();
		retry--;
	}
	
	return ret;

}

u8 GetUploadStr(u8 msg_str_id, char *msg_str)
{
	MsgDev *msg=(MsgDev*)msg_str;
	char *p_left=msg_str+sizeof(MsgDev);
	u8 Result_Validation = 0;

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

#if TEST
	Device_GPIO_Status(msg->ports);
#else
	for(u8 i = 0; i < MSG_STR_LEN_OF_PORTS; i++)
	{
		msg->ports[i] = (ON==Device_Power_Status(i))?'1':'0';		
	}
#endif
	msg->ports[MSG_STR_LEN_OF_PORTS] = delim;
	
	Device_Timer_Status(msg->period);
	msg->period[MSG_STR_LEN_OF_PORTS_PERIOD] = delim;

	for(char *p=msg->preSeq, i = 0; i < DEVICEn; i++)
 	{	
		sprintf(p, "%03d", g_device_status[i].seq);
		p+=MSG_STR_LEN_OF_SEQ;	
	}

	msg->preSeq[MSG_STR_LEN_OF_PORTS_SEQ] = delim;
	
	strncpy(p_left, iccid, LENGTH_ICCID_BUF);
	p_left += strlen(iccid);
	*p_left++ = delim;

	if(msg_str_id == MSG_STR_ID_LOGIN)
	{
		sprintf(p_left, "%02d", dev.need_login);
		p_left += 2;
		*p_left++ = delim;
		sprintf(p_left, "%04d", signal);
		p_left += 4;
		*p_left++ = delim;		
	}

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

	memset(dev.sms_backup, 0, sizeof(dev.sms_backup));
	strncpy(dev.sms_backup, msg_str, strlen(msg_str));
	
	return strlen(msg_str);
}

//发送登陆信息给服务器
void SendLogin(void)
{
	char Loginbuf[150]={0};
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
	char HBbuf[150]={0};
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
	char StartAck[150]={0};
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
	char FinishBuf[150]={0};
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
