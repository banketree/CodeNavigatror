
#ifndef _TEST_SOFT_H_
#define _TEST_SOFT_H_

#include <stdio.h>

#define TS_QUE_LEN                  1024
#define TS_QUE_READ_TIMEOUT         250
#define TS_CODEC_START_FLAG 		'^'
#define TS_CODEC_END_FLAG   		'~'
#ifdef _YK_IMX27_AV_
	#define TS_TEMP_FILE_PATH			 "/home/"
	#define TS_APP_FILE_PATH				 "/home/YK-IDBT/"
	#define TS_CONFIG_FILE_PATH			 "/home/YK-IDBT/config-idbt/"
#else
	#define TS_TEMP_FILE_PATH			 "/"
	#define TS_APP_FILE_PATH            "/mnt/mtd/"
	#define TS_CONFIG_FILE_PATH         "/mnt/mtd/config-idbt/"
#endif
#define TS_ACK_MSG_OK               "OK"
#define TS_ACK_MSG_ERR              "FAILED"

#define TS_FILE_DATA					 0
#define TS_READ_DATAREQUEST         1
#define TS_WRITE_DATAREQUEST        2
#define TS_READ_WRITE_OK            3
#define TS_GET_MAC					 4
#define TS_DEVICE_REBOOT				 5

#define FILE_TYPE_RESERVED 			 0				   // 纯命令时，文件类型保留
#define TS_FILE_TYPE_APP            1               // 应用程序
#define TS_FILE_TYPE_CONFIG         2               // 配置文件



typedef enum
{
	TS_STATE_IDLE,
	TS_STATE_RUNNING,
	TS_STATE_QUIT
}TS_STATE_EN;

typedef struct _MSGDATA{
    void*       pvBuffer;
    int         iSize;
}TS_MSG_DATA_ST;

typedef struct
{
	char buf[1024];
	int len;
	char *head;
	unsigned char counter;
	unsigned char state;
}TS_RECV_DATA_ST;



typedef struct
{
	TS_STATE_EN enState;
	char        acHost[20];
	char        acPort[10];
    char        acIP[20];
}TS_INFO_ST;

typedef struct
{
	int         iCmd;
	int         iType;
	char        acData[1024];
}TS_MSG_INFO_ST;

typedef struct
{
	FILE *      fd;
	char        acFileName[64];
	char        acFilePath[128];
    int         iType;
    int         iFlag;
    unsigned int uiSize;
}TS_FILE_INFO_ST;

extern int g_iTSRunFlag;
extern int TSInit(void);

#endif

