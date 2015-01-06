/*
 * AutoTest.h
 *
 *  Created on: 2012-9-11
 *      Author: root
 */

#ifndef AUTOTEST_H_
#define AUTOTEST_H_

#ifndef AT_ADD_IP_LEN
#define AT_ADD_IP_LEN				16
#endif

#ifndef AT_PORT_LEN
#define AT_PORT_LEN					5
#endif

#ifndef SIZE_1024
#define SIZE_1024 1024
#endif

#ifndef RECV_LMT_TIMES
#define RECV_LMT_TIMES   			5
#endif

#ifndef BEGIN_FLAG_NOT_FOUND
#define BEGIN_FLAG_NOT_FOUND   		0
#endif

#ifndef BEGIN_FLAG_FOUND
#define BEGIN_FLAG_FOUND        	1
#endif

#define AT_CODEC_START_FLAG 		'^'
#define AT_CODEC_END_FLAG   		'~'
#define AT_CODEC_START_FLAG_SIZE	1
#define AT_CODEC_END_FLAG_SIZE		1

#define AT_CODEC_TYPE_FLAG			'0'
#define AT_CODEC_TYPE_FLAG_SIZE		1

#define AT_CODEC_CMD_SIZE			3

#define AT_CODEC_APART_FLAG			'/'
#define AT_CODEC_APART_FLAG_SIZE	1

#define AT_QUE_READ_TIMEOUT    		250

#define AT_TRY_RECONNECT_TIMES		99
#define AT_RECV_TIMEOUT				30

#define AT_TEST_STATE_FAIL			'0'
#define AT_TEST_STATE_SUCCESS		'1'

#define AT_TO_PC_TEST_SUCCESS		"OK"
#define AT_TO_PC_TEST_FAIL			"FAILED"
#define AT_TO_PC_ID					"0000000000"

#define AT_TO_PC_TEST_SUCCESS_SIZE	2
#define AT_TO_PC_TEST_FAIL_SIZE		6
#define AT_TO_PC_ID_SIZE			10

#define AT_QUE_LEN 256
typedef struct __RecvDataCtx
{
    char buf[SIZE_1024];
    int iLen;
    char *head;
    unsigned char counter;
    unsigned char cState;
    int lens;
}RecvDataCtx;

typedef enum
{
	AT_QUERY				=	000,

	AT_REGISTER_REQUEST		=	100,

	AT_CALL_TEST			=	200,
	AT_CALL_RESULT			=	201,

	AT_REQUEST_CONVERSATION =	300,
	AT_INSTANT_ANSWER		=	301,
	AT_INSTANT_REJECT		=	302,
	AT_DELAY_ANSWER			=	303,
	AT_DELAY_REJECT			=	304,
	AT_SEND_TDMF			=	305,
	AT_TALK_TIME			=	306,

	AT_ONEKEY_TEST			=	400,
	AT_ONEKEY_REQUEST		=	401,
	AT_ONEKEY_OK			=	402,

	AT_MONITOR_TEST			=	500,
	AT_MONITOR_REQUEST		=	501,
	AT_MONITOR_OK			=	502,

	AT_LOG_UPDATE			=	601,
	AT_LOG_RESULT			=   602,

	AT_CODEC_ERROR			=	999
}AT_CMD_EN;

typedef enum{
    MESSAGE_FOR_DECODE=0,
    MESSAGE_FOR_ENCODE
}AT_MESSAGE_FOR_EN;

typedef enum{
	AT_REGISTER_STATE_REQUEST		=	1,
	AT_REGISTER_STATE_SUCCESS		=	2,
	AT_REGISTER_STATE_REJECT		=	3,
	AT_REGISTER_STATE_FAIL			=   4,
	AT_REGISTER_STATE_WAIT			= 	5
}AT_REGISTER_STATE_EN;

typedef enum{
	AT_CMD_LINE_HELP		=	1,
	AT_CMD_LINE_TEST		=	2,
	AT_CMD_LINE_STOP		=	3,
	AT_CMD_LINE_STATE		=	4,
	AT_CMD_LINE_DISPLAY		=   5,
	AT_CMD_LINE_UNDISPLAY	=	6
}AT_CMD_LINE_EN;

typedef struct __MsgData{
    void*               pvBuffer;
    int                 iSize;
//    AT_MESSAGE_FOR_EN   enMessageType;
//    AT_CMD_EN			enCommandType;
//    void*               pvResolve;
}AT_MSG_DATA_ST;

typedef enum
{
	AT_STATE_IDLE = 0,
	AT_STATE_RUNNING,
	AT_STATE_QUIT
}AT_STATE_EN;

typedef enum
{
	AT_TEST_OBJECT_SM,
	AT_TEST_OBJECT_CC,
	AT_TEST_OBJECT_LP
}AT_TEST_OBJECT_EN;

typedef struct
{
	char buf[SIZE_1024];
	int len;
	char *head;
	unsigned char counter;
	unsigned char state;
}RECV_DATA_CTX_ST;

typedef struct
{
	char acTestID[AT_TO_PC_ID_SIZE+1];
	char cTestStart ;
	char cLock;
	char cSM;
	char cCC;
	char cLP;
}AT_TEST_ST;

typedef struct
{
	char acIP[64];
	char acUserName[64];
	char acPasswd[64];
	int iTestStart ;
	int iLogFtpDefault;
	int iLogSuccess;
	int iPort;
}AT_LOG_ST;

/**
 *@brief	网络连接信息结构体
 *			该结构体主要用于保存宽带账号,密码,IMS信息
 */
typedef struct
{
	char acHost[AT_ADD_IP_LEN];					/**<! IP . */
	char acPort[AT_PORT_LEN];					/**<! 端口. */
	char acSIP[128];
	char acIP[AT_ADD_IP_LEN];
}AT_INFO_ST;

typedef struct
{
	int iState;
	int iReconnect;
	char acHost[20];
	char acPort[10];
}AT_GLOBAL_ST;

void ATTestOK(AT_TEST_OBJECT_EN penObject);

int ATProcessCommand(const char *cmd);

void ATSendTestResult();

void ATSendOneKeyOpenResult(int iState,char *pcRoomNum,char *pcPhoneNum);

void ATSendMonitorResult(int iState,char *pcRoomNum,char *pcPhoneNum);

void ATSendLogResult();

void ATQueryFtpInfo(char* pcHost, int*piPort, char* pcUserName, char* pcPassWord);

int ATCheckLogDefault();

int ATCheckLogTest();

void  ATLogUpdateSuccess();

// return  0 idle  1 running
int ATIsRunning();

#endif /* AUTOTEST_H_ */
