#include "SIM800.h"	 	 	
#include "string.h"  
#include "stdio.h"	
#include "usart.h" 
#include "usart3.h" 
#include "timer.h"
#include "delay.h"
#include <stdlib.h>
#include "device.h"

#define COUNT_AT 3

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

char  *cell = "13910138465";

//存储PCB_ID的数组（也就是SIM卡的ICCID）
char ICCID_BUF[LENGTH_ICCID_BUF+1] = {0};

t_DEV dev={0};
extern void Reset_Device_Status(u8 status);

const char *msg_id[MSG_STR_ID_MAX]={"TRVAP00", "TRVAP01", "TRVAP03", "TRVAP05"};
const char *msg_id_s[MSG_STR_ID_MAX]={"TRVBP00", "TRVBP01", "TRVBP03", "TRVBP05"};
const char *msg_device="000";

//SIM800发送命令
//cmd:发送的命令字符串(不需要添加回车了),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
u8 SIM800_Send_Cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 ret = CMD_ACK_NONE; 

	//放在下面还是这里合适???
	//dev.msg_recv &= ~MSG_DEV_ACK;	
	
	if(ack!=NULL)
	{
		//新的一次发送开始，需要把之前recv 的ack 状态清除掉
		//dev.msg_recv = 0;
		
		dev.msg_expect |= MSG_DEV_ACK;
		memset(dev.atcmd_ack, 0, sizeof(dev.atcmd_ack));
		strcpy(dev.atcmd_ack, (char *)ack);
	}	

	//Clear_Usart3();	  //放下面还是放在这里合适
	if((u32)cmd <= 0XFF)
	{
		while((USART3->SR&0X40)==0);//等待上一次数据发送完成  
		USART3->DR = (u32)cmd;
	}
	else 
	{
		u3_printf("%s\r\n",cmd);//发送命令
	}

	//Clear_Usart3();	//放上面还是放在这里合适
	if(ack&&waittime)		//需要等待应答
	{
		while(waittime!=0)	//等待倒计时
		{ 
			delay_ms(10);	
			//if(dev.msg_recv & MSG_DEV_RESET)
			if(dev.need_reset != ERR_NONE)
			{
				ret = CMD_ACK_DISCONN;
				break;
			}
			//IDLE 是指串口同时收到"SEND OK" + "正确的服务器回文"，在
			//定时器处理中已经将设备状态转换为IDLE 状态
			//else if((dev.msg_recv & MSG_DEV_ACK) && ((dev.status == CMD_IDLE) || (dev.status == CMD_OPEN_DEVICE)))
			else if(dev.msg_recv & MSG_DEV_ACK)
			{
				ret = CMD_ACK_OK;
				dev.msg_recv &= ~MSG_DEV_ACK;
				break;
			}				
			waittime--;	
		}
	}
	else   //不需要等待应答,这里暂时不添加相关的处理代码
	{
		;
	
	}
	return ret;
} 



u8 Check_Module(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT","OK",100);
		if(ret == CMD_ACK_NONE) 
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))  //在正常AT 命令里基本上不可能返回"CLOSED" 吧 ，仅放在这里
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	return ret;
	
}

//关闭回显
u8 Disable_Echo(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("ATE0","OK",200);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);	
	return ret;
	
}

u8 Check_Network(void)
{
	u8 count = 20;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CREG?","+CREG: 0,1",500);
		if(ret == CMD_ACK_NONE) 
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))  //在正常AT 命令里基本上不可能返回"CLOSED" 吧 ，仅放在这里
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	return ret;
	
}

//查看SIM是否正确检测到
u8 Check_SIM_Card(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;

	delay_ms(10000);	
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CPIN?","OK",1000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}

	//Clear_Usart3();
	delay_ms(2000);	
	return ret;
}

u8 Check_OPS(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+COPS?","CHINA",500);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	return ret;
}


//查看天线质量
u8 Check_CSQ(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	u8 *p1 = NULL; 
	u8 *p2 = NULL;
	u8 p[50] = {0}; 
  	u8 signal=0;

	while(signal < 5)
	{
		delay_ms(2000);
		while(count != 0)
		{
			ret = SIM800_Send_Cmd("AT+CSQ","+CSQ:",200);
			if(ret == CMD_ACK_NONE)
			{
				delay_ms(2000);
			}
			else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
				break;
			
			count--;
		}		
		
		if(ret == CMD_ACK_OK)
		{
			//AT指令已经指令完成，下面对返回值进行处理
			p1=(u8*)strstr((const char*)(dev.usart_data),":");
			p2=(u8*)strstr((const char*)(p1),",");
			p2[0]=0;//加入结束符
			signal = atoi((const char *)(p1+2));
			//sprintf((char*)p,"信号质量:%s",p1+2);
			sprintf((char*)p,"信号质量:%d",signal);
			BSP_Printf("%s\r\n",p);
		}
		//AT指令的回文已经处理完成，清零
	}	
	return ret;
}

//获取SIM卡的ICCID
//SIM卡的ICCID,全球唯一性，可以用作PCB的身份ID
//打印USART3_RX_BUF的內容 調試用途
		/*****  注意+号前面有两个空格
  +CCID: 1,"898602B8191650216485"

		OK
		****/
//这个函数还没有最终确认....
u8 Get_ICCID(void)
{
	u8 index = 0;
	char *p_temp = NULL;
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CCID","OK",200);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if(ret == CMD_ACK_OK)
		{
			if(strstr(dev.usart_data, "AT+CCID")==NULL)
				break;
			else
				Disable_Echo();
		}
		else if(ret == CMD_ACK_DISCONN)
			break;
		
		count--;
	}

	if(ret == CMD_ACK_OK)	
	{
		//AT指令已经指令完成，下面对返回值进行处理
		p_temp = dev.usart_data;
		memset(ICCID_BUF, 0, sizeof(ICCID_BUF));
		//提取ICCID信息到全局变量ICCID_BUF
		for(index = 0;index < LENGTH_ICCID_BUF;index++)
		{
			ICCID_BUF[index] = *(p_temp+OFFSET_ICCID+index);
		}
		BSP_Printf("ICCID_BUF:%s\r\n",ICCID_BUF);
	}
	//AT指令的回文已经处理完成，清零
	//Clear_Usart3();
	return ret;
}

u8 SIM800_GPRS_ON(void)
{
	u8 ret = CMD_ACK_NONE;	
	if((ret = Link_Server_AT(0, ipaddr, port)) == CMD_ACK_OK)
		dev.need_reset = ERR_NONE;
	
	//Clear_Usart3();	
	return ret;

}

//关闭GPRS的链接
u8 SIM800_GPRS_OFF(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",500);
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

//附着GPRS
u8 SIM800_GPRS_Adhere(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CGATT=1","OK",1000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);
	return ret;
}

//设置为GPRS链接模式
u8 SIM800_GPRS_Set(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CIPCSGP=1,\"CMNET\"","OK",600);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);	
	return ret;
}

//设置接收数据显示IP头(方便判断数据来源)	
u8 SIM800_GPRS_Dispaly_IP(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CIPHEAD=1","OK",300);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	return ret;
}

//关闭移动场景 
u8 SIM800_GPRS_CIPSHUT(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CIPSHUT","SHUT OK",1000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);	
	return ret;
}

//设置GPRS移动台类别为B,支持包交换和数据交换 
u8 SIM800_GPRS_CGCLASS(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CGCLASS=\"B\"","OK",300);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);	
	return ret;
}


//设置PDP上下文,互联网接协议,接入点等信息
u8 SIM800_GPRS_CGDCONT(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",600);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);	
	return ret;
}

u8 Link_Server_AT(u8 mode,const char* ipaddr,const char *port)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	u8 p[100]={0};
	
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
		ret = SIM800_Send_Cmd(p,"CONNECT",15000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
		
		ret = SIM800_Send_Cmd("AT+CIPSTATUS","OK",500);
		if(ret == CMD_ACK_OK)
		{
			if(strstr((const char*)(dev.usart_data),"CONNECT OK") != NULL)
				return ret;
			if(strstr((const char*)(dev.usart_data),"CLOSED") != NULL)
			{
				ret = SIM800_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",500);
				ret = SIM800_Send_Cmd("AT+CIPSHUT","SHUT OK",500);
			}
		}
	}
		
	return ret;
}

u8 Send_Data_To_Server(char* data)
{
	u8 ret = CMD_ACK_NONE;

	if(dev.status == CMD_TO_IDLE)
	{
		BSP_Printf("Send_Data_To_Server: already IDLE status\r\n");
		return CMD_ACK_OK;
	}
	
	if(dev.need_reset != ERR_NONE)
	{
		BSP_Printf("Send_Data_To_Server: Need Reset\r\n");	
		return CMD_ACK_DISCONN;
	}
	else
	{
		BSP_Printf("准备开始发送数据\r\n");
		//由于前一次发送可能收到了ack, 但没有收到服务器回文
		//因此需要开始重发的时候重置某些变量
		//PS. 但在中断外部操作设备状态可能有风险!!!
		//Reset_Device_Status(dev.status);
		dev.msg_recv = 0;		
		//dev.hb_timer = 0;
		//dev.reply_timeout = 0;
		//dev.msg_timeout = 0;
		//dev.msg_recv = 0;
		dev.msg_expect = 0;
		memset(dev.atcmd_ack, 0, sizeof(dev.atcmd_ack));
		memset(dev.device_on_cmd_string, 0, sizeof(dev.device_on_cmd_string));
		
		ret = SIM800_Send_Cmd("AT+CIPSEND",">",500);
	}
	
	if(ret == CMD_ACK_OK)		//发送数据
	{ 
		//Clear_Usart3();   //成功发送"AT+CIPSEND" 之后，才使能串口接收
		u3_printf("%s",data);
		delay_ms(100);
		ret = SIM800_Send_Cmd((u8*)0x1A,"SEND OK",3000);
	}
	else
	{
		BSP_Printf("Cancel Sending: %d\r\n", ret);
		SIM800_Send_Cmd((u8*)0x1B,0,0);
	}
	
	BSP_Printf("已完成一次发送: %d\r\n", ret);
	return ret;
}

#if 0
u8 Check_Link_Status(void)
{
	u8 count = 0;

	while(SIM800_Send_Cmd("AT+CMSTATE","CONNECTED",500))//检测是否应答AT指令 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
//AT指令已经尝试了COUNT_AT次，仍然失败，断电SIM800，开启TIME_AT分钟的定时，定时时间到，再次链接服务器
//目前代码中没有调用本函数，也没有对Flag_TIM6_2_S的判定代码，所以暂时屏蔽掉Flag_TIM6_2_S的赋值
			//Flag_TIM6_2_S = 0xAA;
			return 1;		
		}
	}	

		//AT指令的回文不需要处理，清零
	Clear_Usart3();
	return 0;

}
#endif

//设置文本模式 
u8 SIM800_CMGF_Set(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CMGF=1","OK",1000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}

	delay_ms(2000);	
	return ret;
}

//设置短消息文本模式参数 
u8 SIM800_CSMP_Set(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CSMP=17,167,0,0","OK",200);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	delay_ms(2000);	
	return ret;
}

u8 SIM800_CSCS_Set(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CSCS=\"GSM\"","OK",200);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	delay_ms(2000);	
	return ret;
}

char *SIM800_SMS_Create(char *sms_data, char *raw)
{
	sprintf((char*)sms_data,"Reset Type: %d, Dev Status: %d, Msg expect: %d, Msg recv: %d, HB: %d, HB TIMER: %d, Msg TIMEOUT: %d, Msg: \"%s\", AT-ACK: %s\r\n", dev.need_reset, 
		dev.status, dev.msg_expect, dev.msg_recv, dev.hb_count, dev.hb_timer, dev.msg_timeout, raw, dev.atcmd_ack); 
	return sms_data;
}

u8 SIM800_SMS_Notif(char *phone, char *sms)
{
	u8 ret = CMD_ACK_NONE;
	u8 sms_cmd[100]={0};
	//u8 sms_data[100]={0};

	if((ret = Check_Module()) == CMD_ACK_OK)
		if((ret = Disable_Echo()) == CMD_ACK_OK)
			if((ret = Check_SIM_Card()) == CMD_ACK_OK)	
				if((SIM800_CSCS_Set()) == CMD_ACK_OK)
					if((ret = SIM800_CMGF_Set()) == CMD_ACK_OK)
						if((ret = SIM800_CSMP_Set()) == CMD_ACK_OK)
						{		
							sprintf((char*)sms_cmd,"AT+CMGS=\"%s\"\r\n",phone); 
							if(SIM800_Send_Cmd(sms_cmd,">",200)==CMD_ACK_OK)					//发送短信命令+电话号码
							{
								//sprintf((char*)sms_data,"Dev Status: %d, Msg expect: %d, Msg recv: %d, HB: %d, HB TIMER: %d, Msg TIMEOUT: %d Msg: \"%s\"\r\n", dev.status, dev.msg_expect, dev.msg_recv, dev.hb_count, dev.hb_timer, dev.msg_timeout, current); 
								BSP_Printf("SMS: %s\r\n", sms);
								u3_printf("%s",sms);		 						//发送短信内容到GSM模块 
								delay_ms(500);                                   //必须延时，否则不能发送短信
								ret = SIM800_Send_Cmd((u8*)0X1A,"+CMGS:",2000); //发送结束符,等待发送完成(最长等待10秒钟,因为短信长了的话,等待时间会长一些)
							}  			
						}

	return ret;
}

//开启2G模块的电源芯片，当做急停按钮来使用
void SIM800_POWER_ON(void)
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
void SIM800_POWER_OFF(void)
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
void SIM800_PWRKEY_ON(void)
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
void SIM800_PWRKEY_OFF(void)
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

void SIM800_GPRS_Restart(void)
{
	u8 temp = 0;
	SIM800_GPRS_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	SIM800_GPRS_ON();

}

void SIM800_Powerkey_Restart(void)
{
	u8 temp = 0;
	BSP_Printf("Powerkey Restart\r\n");
	SIM800_PWRKEY_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	SIM800_PWRKEY_ON();
}

void SIM800_Power_Restart(void)
{
	u8 temp = 0;
	SIM800_PWRKEY_OFF();
	SIM800_POWER_OFF();
	
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	SIM800_POWER_ON();
	SIM800_PWRKEY_ON();

}

//返回1   某条AT指令执行错误
//返回0   成功连接上服务器
u8 SIM800_Link_Server_AT(void)
{
	u8 ret = CMD_ACK_NONE;
	//操作AT指令进行联网操作
	if((ret = Check_Module()) == CMD_ACK_OK)
		if((ret = Disable_Echo()) == CMD_ACK_OK)
			if((ret = Check_Network()) == CMD_ACK_OK)		
				if((ret = Check_SIM_Card()) == CMD_ACK_OK)
					if((ret = Check_CSQ()) == CMD_ACK_OK)
						if((ret = Get_ICCID()) == CMD_ACK_OK)
							//if((ret = Check_OPS()) == CMD_ACK_OK)
								//if((ret = SIM800_GPRS_OFF()) == CMD_ACK_OK)
									if((ret = SIM800_GPRS_CIPSHUT()) == CMD_ACK_OK)
										if((ret = SIM800_GPRS_CGCLASS()) == CMD_ACK_OK)
											if((ret = SIM800_GPRS_CGDCONT()) == CMD_ACK_OK)
												//if((ret = SIM800_GPRS_Adhere()) == CMD_ACK_OK)
													if((ret = SIM800_GPRS_Set()) == CMD_ACK_OK)
														//if((ret = SIM800_GPRS_Dispaly_IP()) == CMD_ACK_OK)
															if((ret = Link_Server_AT(0, ipaddr, port)) == CMD_ACK_OK)
																Reset_Device_Status(CMD_LOGIN);


	return ret;
}

u8 SIM800_Link_Server_Powerkey(void)
{
	u8 count = 5;
	u8 ret = CMD_ACK_NONE;	
	while(count != 0)
	{
		ret = SIM800_Link_Server_AT();
		if(ret != CMD_ACK_OK)
		{
			SIM800_Powerkey_Restart();
		}
		else
			break;
		count--;
	}

	return ret;

}
u8 SIM800_Link_Server(void)
{
	u8 count = 5;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Link_Server_Powerkey();
		if(ret != CMD_ACK_OK)
		{
			SIM800_Power_Restart();
		}
		else
			break;
		count--;
	}
	
	return ret;

}

u8 Get_Device_Upload_Str(u8 msg_str_id, char *msg_str)
{
	msg_data *msg=(msg_data *)msg_str;
	char *p_left=msg_str+sizeof(msg_data);
	u8 Result_Validation = 0;
	u8 i;

	if(msg_str == NULL)
		return 0;

	if(msg_str_id>=MSG_STR_ID_MAX)
		return 0;

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
			strcpy(p_left, "SIM800_");
			p_left += strlen("SIM800_");
			strncpy(p_left, ICCID_BUF, LENGTH_ICCID_BUF);
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

  	sprintf(msg->length,"%03d",strlen(msg_str)-sizeof(msg->id)-sizeof(msg->length)+5);
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
		ret = Send_Data_To_Server(Login_buf);
	}
	return ret;
}

u8 Send_Login_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = COUNT_AT;	//执行count次，还不成功的话，就重启GPRS
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
			SIM800_GPRS_Restart();
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
		ret = Send_Data_To_Server(HB_buf);
	}
	return ret;
}

u8 Send_Heart_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = COUNT_AT;	//执行count次，还不成功的话，就重启GPRS
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
u8 Send_Open_Device_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	char Open_Device_buf[100]={0};
	if(Get_Device_Upload_Str(MSG_STR_ID_OPEN, Open_Device_buf)!=0)
	{
		BSP_Printf("New Open:%s\r\n",Open_Device_buf);		
		//Get_Open_Device_Data();
		//ret = Send_Data_To_Server(Enbale_Buffer);
		ret = Send_Data_To_Server(Open_Device_buf);
	}
	return ret;
}

u8 Send_Open_Device_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = COUNT_AT;	//执行count次，还不成功的话，就重启GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret= Send_Open_Device_Data();
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

//这个流程其实是一条设备回文，设备在CMD_NONE 状态的时候(当前没有处理任何消息)才会进入此流程
//当然硬件的开关在收到服务器指令时就需要完成，不依赖于这条消息什么时候发送    
u8 Send_Open_Device_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	ret = Send_Open_Device_Data_Normal();	
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
		//Get_Close_Device_Data();
		//ret = Send_Data_To_Server(Device_OK_Buffer);
		ret = Send_Data_To_Server(Close_Device_buf);
	}
	return ret;
}

u8 Send_Close_Device_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = COUNT_AT;	//执行count次，还不成功的话，就重启GPRS
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
