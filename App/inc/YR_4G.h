#ifndef __YR4G_H__
#define __YR4G_H__	 
#include "sys.h"
#include "device.h"

#define LENGTH_VERSION_BUF 20
extern char version[LENGTH_VERSION_BUF];

#define LENGTH_WKMOD_BUF 20
extern char wkmod[LENGTH_WKMOD_BUF];

#define LENGTH_PASSWORD_BUF 20
extern char password[LENGTH_PASSWORD_BUF];

#define LENGTH_SYSINFO_BUF 20 
extern char sysinfo[LENGTH_SYSINFO_BUF];

#define LENGTH_ICCID_BUF 20
extern char iccid[LENGTH_ICCID_BUF];

#define LENGTH_CSQ_BUF 20
extern char csq[LENGTH_CSQ_BUF];

//存储设备重发命令的数组
#define LENGTH_RESEND 35
extern char Resend_Buffer[LENGTH_RESEND];

//存储设备登陆命令的数组
#define LENGTH_LOGIN 80
extern char Login_Buffer[LENGTH_LOGIN];

//存储心跳包的数组
#define LENGTH_HEART 50
extern char Heart_Buffer[LENGTH_HEART];

//存储业务指令回文的数组
#define LENGTH_ENABLE 50
extern char Enbale_Buffer[LENGTH_ENABLE];

//存储业务执行完成指令的数组
#define LENGTH_DEVICE_OK 50
extern char Device_OK_Buffer[LENGTH_DEVICE_OK];

#define LENGTH_ATCMD_ACK 50
#define LENGTH_DEVICE_OPEN_CMD        50
#define LENGTH_USART_DATA        100
#define LENGTH_SMS_BACKUP        100

/*********WJ*********/
bool 	YR4G_Send_Cmd(char *cmd,char *ack,char *recv,u16 waittime);
u8 Check_Xor_Sum(char* pBuf, u16 len);

char *DumpQueue(char * recv);
bool 	CheckModule(void);
bool 	GetVersion(void);
bool 	GetSysinfo(void);
bool 	GetPassword(void);
bool 	GetResetTime(void);
bool 	GetCSQ(void);
bool 	GetICCID(void);
bool SaveSetting(void);
bool SocketParam(u8 rw, u8 sock, u8 *protocol, u8 *addr, u8 *port);
bool SocketEnable(u8 rw, u8 sock, bool enable);
bool SocketSL(u8 rw, u8 sock, u8 sl);
bool isConnected(u8 sock);
bool SocketTO(u8 sock);
	
void YR4G_POWER_ON(void);
void YR4G_POWER_OFF(void);
void YR4G_PWRKEY_ON(void);
void YR4G_PWRKEY_OFF(void);

void YR4G_GPRS_Restart(void);
void YR4G_Powerkey_Restart(void);
void YR4G_Power_Restart(void);

bool YR4G_Link_Server(void);
bool YR4G_Link_Server_AT(void);
bool YR4G_Link_Server_Powerkey(void);

void Send_Login_Data(void);

u8 Send_Heart_Data(void);
u8 Send_Heart_Data_Normal(void);
u8 Send_Heart_Data_To_Server(void);

u8 Send_Open_Device_Data(void);
u8 Send_Open_Device_Data_Normal(void);
u8 Send_Open_Device_Data_To_Server(void);

u8 Send_Close_Device_Data(void);
u8 Send_Close_Device_Data_Normal(void);
u8 Send_Close_Device_Data_To_Server(void);
char *YR4G_SMS_Create(char *sms_data, char *raw);

#define MSG_STR_DEVICE_HEADER       "TRVAP"
#define MSG_STR_SERVER_HEADER       "TRVBP"

#define MSG_STR_LEN_OF_HEADER                        5         //strlen("TRVXP")
#define MSG_STR_LEN_OF_ID                                  2 
#define MSG_STR_LEN_OF_LENGTH                         3
#define MSG_STR_LEN_OF_SEQ                               3
#define MSG_STR_LEN_OF_DUP                               2
#define MSG_STR_LEN_OF_DEVICE                          3
#define MSG_STR_LEN_OF_PORTS                           4
#define MSG_STR_LEN_OF_PORTS_PERIOD            (MSG_STR_LEN_OF_PORTS*4)

typedef struct
{
	u8 status;
	u8 hb_timer;   //hb always running
	u8 reply_timeout;
	u8 need_reset;
	u16 hb_count;
	u8 msg_seq;
	u8 msg_seq_s;
	u32 msg_timeout;
	u32 msg_recv;
	u32 msg_expect;
	char atcmd_ack[LENGTH_ATCMD_ACK];
	char device_on_cmd_string[LENGTH_DEVICE_OPEN_CMD];
	char usart_data[LENGTH_USART_DATA];
	char sms_backup[LENGTH_SMS_BACKUP];	
}t_DEV;

typedef struct
{
	char header[MSG_STR_LEN_OF_HEADER+1];
	char id[MSG_STR_LEN_OF_ID+1];
	char length[MSG_STR_LEN_OF_LENGTH+1];
	char seq[MSG_STR_LEN_OF_SEQ+1];
	char dup[MSG_STR_LEN_OF_DUP+1];
	char device[MSG_STR_LEN_OF_DEVICE+1];
	char ports[MSG_STR_LEN_OF_PORTS+1];
	char period[MSG_STR_LEN_OF_PORTS_PERIOD+1];
}MsgDev;

typedef struct
{
	char header[MSG_STR_LEN_OF_HEADER+1];
	char id[MSG_STR_LEN_OF_ID+1];
	char length[MSG_STR_LEN_OF_LENGTH+1];
	char seq[MSG_STR_LEN_OF_SEQ+1];
	char dup[MSG_STR_LEN_OF_DUP+1];
	char device[MSG_STR_LEN_OF_DEVICE+1];	
}MsgSrv;

/*********WJ*********/
extern t_DEV dev;
enum
{
	CMD_NONE,   //before connect

	CMD_IDLE,    //connected but no op
		
	CMD_LOGIN,
	CMD_HB,
	CMD_CLOSE_DEVICE,

	CMD_OPEN_DEVICE,

	CMD_TO_IDLE,
	CMD_ERROR,
};

enum
{ 
	CMD_ACK_OK = 0,                 //USART3_RX_STA置位，返回的数据正确
	CMD_ACK_DISCONN = 1,       //USART3_RX_STA置位，返回的数据表明掉线
	CMD_ACK_NONE,                  //USART3_RX_STA没有置位
};

enum
{
	MSG_BIT_ACK = 0,                  //Dev->Server
	MSG_BIT_LOGIN,
	MSG_BIT_HB,                        
	MSG_BIT_CLOSE,	

	MSG_BIT_OPEN,               //Server->Dev

	//MSG_BIT_RESEND,          //Both Dir
	
	//MSG_BIT_RESET,             //Reset	
};

#define MSG_DEV_ACK          ((u32)(1<<MSG_BIT_ACK))
#define MSG_DEV_LOGIN     ((u32)(1<<MSG_BIT_LOGIN))
#define MSG_DEV_HB            ((u32)(1<<MSG_BIT_HB))
#define MSG_DEV_CLOSE      ((u32)(1<<MSG_BIT_CLOSE))

#define MSG_DEV_OPEN        ((u32)(1<<MSG_BIT_OPEN))

//#define MSG_DEV_RESEND   ((u32)(1<<MSG_BIT_RESEND))

//#define MSG_DEV_RESET      ((u32)(1<<MSG_BIT_RESET))

enum
{
	MSG_STR_ID_LOGIN=0,
	MSG_STR_ID_HB,
	MSG_STR_ID_OPEN,
	MSG_STR_ID_CLOSE,

	MSG_STR_ID_MAX
};

enum
{
	ERR_NONE = 0,

	ERR_INIT_MODULE,
	ERR_INIT_LINK_SERVER,
	ERR_INIT_SEND_LOGIN,
	ERR_RESET_LINK_SERVER,     
	
	ERR_SEND_LOGIN,	
	ERR_SEND_HB,
	ERR_SEND_CLOSE_DEVICE,
	
	ERR_SEND_OPEN_DEVICE,	
	ERR_RETRY_TOO_MANY_TIMES,	

	ERR_DISCONNECT,	

	ERR_SEND_CMD,	
};

enum
{
	READ,
	WRITE,
};

typedef struct
{
	bool isOn;
	bool isConnected;	
	u8 timeout;
	char *sl;
	char *protocol;
	char addr[50];
	char port[10];
}SockSetting;

enum
{
	SOCK_A,
	SOCK_B,
	SOCK_C,
	SOCK_D,
	SOCK_MAX,
};

extern SockSetting socketSetting[SOCK_MAX];

extern const char *msg_id[MSG_STR_ID_MAX];
extern const char *msg_id_s[MSG_STR_ID_MAX];
extern char  *cell;
#endif
