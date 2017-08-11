#ifndef __SIM800_TP_H__
#define __SIM800_TP_H__	 
#include "sys.h"
#include "device.h"

#define LENGTH_ICCID_BUF 20
extern char iccid[LENGTH_ICCID_BUF+1];

extern uint32_t  lastOutActivity;
extern uint32_t  lastInActivity;

/*********WJ*********/
#define MSG_STR_DEVICE_HEADER       "TRVAP"
#define MSG_STR_SERVER_HEADER       "TRVBP"

#define MSG_STR_LEN_OF_HEADER                        5         //strlen("TRVXP")
#define MSG_STR_LEN_OF_ID                                  2 
#define MSG_STR_LEN_OF_LENGTH                         3
#define MSG_STR_LEN_OF_SEQ                               3
#define MSG_STR_LEN_OF_DUP                               2
#define MSG_STR_LEN_OF_DEVICE                          3
#define MSG_STR_LEN_OF_PORTS                           DEVICEn
#define MSG_STR_LEN_OF_PORTS_PERIOD            (MSG_STR_LEN_OF_PORTS*4)

typedef struct
{
	bool is_login;
	bool hb_ready;
	u8 hb_timer;
	bool wait_reply;	
	u8 reply_timer;	
	u8 need_reset;
	u8 portClosed;
	u8 msg_seq;
	u8 msg_seq_s;
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
	//u8 preSeq[MSG_STR_LEN_OF_PORTS+1];
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
	ERR_RETRY_TOO_MANY_TIMES,	

	ERR_DISCONNECT,	

	ERR_SEND_CMD,	
};

extern const char *msg_id[MSG_STR_ID_MAX];

bool 	SIM800_TP_Send_Cmd(char *cmd,char *ack,char *recv,u16 waittime);
u8 CheckSum(char* pBuf, u16 len);

char *DumpQueue(char * recv);
bool DisableEcho(void);
bool CheckNetwork(void);
bool CheckSIM(void);
bool CheckOPS(void);
bool GetCSQ(void);
bool 	GetICCID(void);
bool AdhereGPRS(void);
bool SetGPRS(void);
bool DisplayIP(void);
bool ShutGPRS(void);
bool SetCGClass(void);
bool SetCGDCONT(void);
bool Link_Server_AT(u8 mode,const char* ipaddr,const char *port);
bool SetCMGF(void);
bool SetCSMP(void);
bool SetCSCS(void);
bool SetCIPMode(void);
bool SetCIPCCFG(void);

bool SIM800_TP_GPRS_ON(void);
bool SIM800_TP_GPRS_OFF(void);
void SIM800_TP_POWER_ON(void);
void SIM800_TP_POWER_OFF(void);
void SIM800_TP_PWRKEY_ON(void);
void SIM800_TP_PWRKEY_OFF(void);

void SIM800_TP_GPRS_Restart(void);
void SIM800_TP_Powerkey_Restart(void);
void SIM800_TP_Power_Restart(void);
bool SIM800_TP_Link_Server_AT(void);
bool SIM800_TP_Link_Server(void);

void SendLogin(void);
void SendHeart(void);
void SendStartAck(void);
void SendFinish(void);
bool isWorking(void);

#endif

