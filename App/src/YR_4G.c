#include "yr_4g.h"	 	 	
#include "string.h"  
#include "stdio.h"	
#include "usart.h" 
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
	{NULL, NULL},
};
	
#define COUNT_AT 3
#define RETRY_AT 3

u8 mode = 0;				//0,TCP连接;1,UDP连接
const char *modetbl[2] = {"TCP","UDP"};//连接模式

//const char  *ipaddr = "tuzihog.oicp.net";
//const char  *port = "28106";

//const char  *ipaddr = "42.159.117.91";
const char  *ipaddr = "116.62.187.167";
//const char  *ipaddr = "42.159.107.250";
const char  *port = "8090";
const char delim=',';
const char ending='#';
const char *Error="Err";

char version[LENGTH_VERSION_BUF] = {0};
char wkmod[LENGTH_WKMOD_BUF] = {0};
char password[LENGTH_PASSWORD_BUF] = "usr.cn";
char sysinfo[LENGTH_SYSINFO_BUF] = {0};
char iccid[LENGTH_ICCID_BUF] = {0};
u16 resetTime=0;
char csq[LENGTH_CSQ_BUF] = {0};

t_DEV dev={0};
extern void Reset_Device_Status(u8 status);

const char *msg_id[MSG_STR_ID_MAX]={"00", "01", "03", "05"};
const char *msg_id_s[MSG_STR_ID_MAX]={"00", "01", "03", "05"};
const char *msg_device="000";

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
#define RESET_GPIO_PORT                   GPIOF
#define RESET_GPIO_PORT_CLK               RCC_APB2Periph_GPIOF

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

bool isWork(void)
{
	return (GPIO_ReadOutputDataBit(WORK_GPIO_PORT, WORK_PIN)==1?TRUE:FALSE);
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
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
			break;
		delay_ms(2000);
		retry--;
	}
	
	return ret;
	
}

bool GetVersion(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_VER;
	char recv[50];	
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			memset(version, 0, sizeof(version));
			strcpy(version, strstr(recv, atpair[id].ack)+strlen(atpair[id].ack));
			BSP_Printf("Version: %s\n", version);
			break;
		}
		delay_ms(2000);
		retry--;
	}
	
	return ret;
	
}

bool GetSysinfo(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_SYSINFO;
	char recv[50];	
	bool ret = FALSE;
	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			memset(sysinfo, 0, sizeof(sysinfo));
			strcpy(sysinfo, strstr(recv, atpair[id].ack)+strlen(atpair[id].ack));
			BSP_Printf("SysInfo: %s\n", sysinfo);
			break;
		}
		delay_ms(2000);
		retry--;
	}
	
	return ret;
	
}

bool GetPassword(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_CMDPW;
	char recv[50];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			memset(password, 0, sizeof(password));
			strcpy(password, strstr(recv, atpair[id].ack)+strlen(atpair[id].ack));
			password[strlen(password)-4]=0;
			BSP_Printf("Password: %s\n", password);
			break;
		}
		delay_ms(2000);
		retry--;
	}

	return ret;
}

bool GetResetTime(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_RSTIM;
	char recv[50];	
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
		delay_ms(2000);
		retry--;
	}

	return ret;
}


//查看天线质量
bool GetCSQ(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_CSQ;
	char recv[50];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			memset(csq, 0, sizeof(csq));
			strcpy(csq, strstr(recv, atpair[id].ack)+strlen(atpair[id].ack));
			BSP_Printf("CSQ: %s\n", csq);
			break;
		}
		delay_ms(2000);
		retry--;
	}

	return ret;
}

bool GetICCID(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_ICCID;
	char recv[50];	
	bool ret = FALSE;

	while(retry != 0)
	{
		ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100);
		if(ret) 
		{
			memset(iccid, 0, sizeof(iccid));
			strcpy(iccid, strstr(recv, atpair[id].ack)+strlen(atpair[id].ack));
			BSP_Printf("ICCID: %s\n", iccid);
			break;
		}
		delay_ms(2000);
		retry--;
	}

	return ret;
}

u8 YR4G_GPRS_ON(void)
{
	u8 ret = CMD_ACK_NONE;	
	if((ret = Link_Server_AT(0, ipaddr, port)) == CMD_ACK_OK)
		dev.need_reset = ERR_NONE;
	
	//Clear_Usart3();	
	return ret;

}

//关闭GPRS的链接
u8 YR4G_GPRS_OFF(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	char recv[50];		
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",recv,500);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}

	Reset_Device_Status(CMD_NONE);	
	//Clear_Usart3();	
	return ret;
}

u8 Link_Server_AT(u8 mode,const char* ipaddr,const char *port)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	char p[100]={0};
	char recv[50];			
	if(mode)
		;
	else 
		;
		
  	sprintf((char*)p,"AT+CIPSTART=\"%s\",\"%s\",\"%s\"",modetbl[mode],ipaddr,port);	

	//发起连接
	//AT+CIPSTART指令可能的回文是：CONNECT OK 和ALREADY CONNECT和CONNECT FAIL
	//这里先取三种可能回文的公共部分来作为判断该指令有正确回文的依据
	while(count != 0)
	{
		ret = YR4G_Send_Cmd(p,"CONNECT",recv,15000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
		
		ret = YR4G_Send_Cmd("AT+CIPSTATUS","OK",recv,500);
		if(ret == CMD_ACK_OK)
		{
			if(strstr((const char*)(dev.usart_data),"CONNECT OK") != NULL)
				return ret;
			if(strstr((const char*)(dev.usart_data),"CLOSED") != NULL)
			{
				ret = YR4G_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",recv,500);
				ret = YR4G_Send_Cmd("AT+CIPSHUT","SHUT OK",recv,500);
			}
		}
	}
		
	return ret;
}

char *YR4G_SMS_Create(char *sms_data, char *raw)
{
	sprintf((char*)sms_data,"Reset Type: %d, Dev Status: %d, Msg expect: %d, Msg recv: %d, HB: %d, HB TIMER: %d, Msg TIMEOUT: %d, Msg: \"%s\", AT-ACK: %s\r\n", dev.need_reset, 
		dev.status, dev.msg_expect, dev.msg_recv, dev.hb_count, dev.hb_timer, dev.msg_timeout, raw, dev.atcmd_ack); 
	return sms_data;
}

void YR4G_POWER_ON(void)
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
		delay_ms(1000);	
	}
}

//关闭2G模块的电源芯片，当做急停按钮来使用
void YR4G_POWER_OFF(void)
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
		delay_ms(1000);	
	}

}


//通过2G模块的PWRKEY来实现开关机
void YR4G_PWRKEY_ON(void)
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
		delay_ms(1000);	
	}
	//开机控制引脚释放
	GPIO_ResetBits(GPIOB,GPIO_Pin_9);
	for(i = 0; i < 2; i++)
	{
		delay_ms(1000);	
	}
	dev.msg_recv = 0;	
	dev.need_reset = ERR_NONE;
	Reset_Device_Status(CMD_NONE);
	Clear_Usart3();
}

//通过2G模块的PWRKEY来实现开关机
void YR4G_PWRKEY_OFF(void)
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
		delay_ms(1000);	
	}
	//开机控制引脚释放
	GPIO_ResetBits(GPIOB,GPIO_Pin_9);

	for(i = 0; i < 2; i++)
	{
		delay_ms(1000);	
	}

}

void YR4G_GPRS_Restart(void)
{
	u8 temp = 0;
	YR4G_GPRS_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	YR4G_GPRS_ON();

}

void YR4G_Powerkey_Restart(void)
{
	u8 temp = 0;
	BSP_Printf("Powerkey Restart\r\n");
	YR4G_PWRKEY_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	YR4G_PWRKEY_ON();
}

void YR4G_Power_Restart(void)
{
	u8 temp = 0;
	YR4G_PWRKEY_OFF();
	YR4G_POWER_OFF();
	
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	YR4G_POWER_ON();
	YR4G_PWRKEY_ON();

}

//返回1   某条AT指令执行错误
//返回0   成功连接上服务器
bool YR4G_Link_Server_AT(void)
{
	bool ret = FALSE;
	if(ret = CheckModule())
		if(ret = GetVersion())
			if(ret = GetPassword())
				if(ret = GetCSQ())
					if(ret = GetICCID())
						if(ret = GetResetTime())
							if(ret = GetSysinfo())
								Reset_Device_Status(CMD_LOGIN);

	return ret;
}

bool YR4G_Link_Server_Powerkey(void)
{
	u8 retry = RETRY_AT;
	bool ret = FALSE;	
	while(retry != 0)
	{
		if(ret = YR4G_Link_Server_AT())
			break;
		YR4G_Powerkey_Restart();
		retry--;
	}

	return ret;

}
bool YR4G_Link_Server(void)
{
	u8 retry = RETRY_AT;
	bool ret = FALSE;
	while(retry != 0)
	{
		if(ret = YR4G_Link_Server_Powerkey())
			break;
		YR4G_Power_Restart();
		retry--;
	}
	
	return ret;

}

u8 Get_Device_Upload_Str(u8 msg_str_id, char *msg_str)
{
	MsgDev *msg=(MsgDev*)msg_str;
	char *p_left=msg_str+sizeof(MsgDev);
	u8 Result_Validation = 0;
	u8 i;

	if(msg_str == NULL)
		return 0;

	if(msg_str_id>=MSG_STR_ID_MAX)
		return 0;

	strncpy(msg->id, MSG_STR_DEVICE_HEADER, MSG_STR_LEN_OF_HEADER);
	msg->header[MSG_STR_LEN_OF_HEADER] = delim;
	
	strncpy(msg->id, msg_id[msg_str_id], MSG_STR_LEN_OF_ID);
	msg->id[MSG_STR_LEN_OF_ID] = delim;

  	strncpy(msg->length, "000", MSG_STR_LEN_OF_LENGTH);
	msg->length[MSG_STR_LEN_OF_LENGTH] = delim;

	if(dev.status == CMD_OPEN_DEVICE)
	{
		sprintf(msg->seq,"%03d",dev.msg_seq_s);
	}
	else
	{
	  	sprintf(msg->seq,"%03d",++dev.msg_seq);	
	}
	msg->seq[MSG_STR_LEN_OF_SEQ] = delim;

  	sprintf(msg->dup, "%02d", dev.msg_timeout);
	msg->dup[MSG_STR_LEN_OF_DUP] = delim;
	
	strncpy(msg->device, msg_device, MSG_STR_LEN_OF_DEVICE);
	msg->device[MSG_STR_LEN_OF_DEVICE] = delim;

	//由于读取的是GPIO 高低，因此是设备实时状态
	for(i = 0; i < MSG_STR_LEN_OF_PORTS; i++)
	{
		msg->ports[i] = (ON==Device_Power_Status(i))?'1':'0';		
	}	
	msg->ports[MSG_STR_LEN_OF_PORTS] = delim;

	Device_Timer_Status(msg->period);
	msg->period[MSG_STR_LEN_OF_PORTS_PERIOD] = delim;
	
	switch(msg_str_id)
	{
		case MSG_STR_ID_LOGIN:
			strcpy(p_left, "YR4G_");
			p_left += strlen("YR4G_");
			strncpy(p_left, iccid, LENGTH_ICCID_BUF);
			p_left += LENGTH_ICCID_BUF;
			*p_left++ = delim;
		break;
		
		case MSG_STR_ID_HB:
		case MSG_STR_ID_OPEN:
		case MSG_STR_ID_CLOSE:
			
		break;
		
		default:
		break;
	}

  	sprintf(msg->length,"%03d",strlen(msg_str)-sizeof(msg->header)-sizeof(msg->id)-sizeof(msg->length)+5);
	msg->length[MSG_STR_LEN_OF_LENGTH] = delim;	
	
	//添加校验和
	Result_Validation = Check_Xor_Sum(msg_str, strlen(msg_str));
	
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
u8 Send_Login_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	char Login_buf[100]={0};
	if(Get_Device_Upload_Str(MSG_STR_ID_LOGIN, Login_buf) != 0)
	{
		BSP_Printf("Login_Buffer:%s\r\n",Login_buf);	
		u3_printf(Login_buf);
	}
	return ret;
}

u8 Send_Login_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//执行count次，还不成功的话，就重启GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Login_Data();
		//Clear_Usart3();
		if(ret == CMD_ACK_NONE)
		{
			//发送数据失败
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count -= 1;
	}
	
	return ret;
	
}

u8 Send_Login_Data_To_Server(void)
{
	u8 count = 5;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = Send_Login_Data_Normal();
		if(ret != CMD_ACK_OK)
		{
			YR4G_GPRS_Restart();
		}
		else
			break;
		count--;	
	}

	return ret;

}

//发送心跳包给服务器
u8 Send_Heart_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	char HB_buf[100]={0};
	if(Get_Device_Upload_Str(MSG_STR_ID_HB, HB_buf)!=0)
	{
		BSP_Printf("New HB:%s\r\n",HB_buf);		
		u3_printf(HB_buf);
	}
	return ret;
}

u8 Send_Heart_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//执行count次，还不成功的话，就重启GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Heart_Data();
		//Clear_Usart3();
		if(ret == CMD_ACK_NONE)
		{
			//发送数据失败
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count -= 1;
	}
	
	return ret;

}

u8 Send_Heart_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	ret = Send_Heart_Data_Normal();
	return ret;
}

//发送接收业务指令完成回文给服务器
bool Send_Open_Device_Data(void)
{
	bool ret = FALSE;
	char Open_Device_buf[100]={0};
	if(Get_Device_Upload_Str(MSG_STR_ID_OPEN, Open_Device_buf)!=0)
	{
		BSP_Printf("Open:%s\r\n",Open_Device_buf);		
		u3_printf(Open_Device_buf);
		ret = TRUE;
	}
	return ret;
}

//发送业务执行完成指令给服务器
u8 Send_Close_Device_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	char Close_Device_buf[100]={0};
	if(Get_Device_Upload_Str(MSG_STR_ID_CLOSE, Close_Device_buf)!=0)
	{
		BSP_Printf("New Close:%s\r\n",Close_Device_buf);		
		u3_printf(Close_Device_buf);
	}
	return ret;
}

u8 Send_Close_Device_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//执行count次，还不成功的话，就重启GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Close_Device_Data();
		//Clear_Usart3();
		if(ret == CMD_ACK_NONE)
		{
			//发送数据失败
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count --;		
	}

	return ret;

}

u8 Send_Close_Device_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	ret = Send_Close_Device_Data_Normal();
	return ret;
}

void Clear_buffer(char* buffer,u16 length)
{
	u16 i = 0;
	for(i = 0; i < length;i++)
	{
		buffer[i] = 0;
	}
}

//////////////异或校验和函数///////
u8 Check_Xor_Sum(char* pBuf, u16 len)
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

