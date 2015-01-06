
/**
 *@file    LPIntLayer.h
 *@brief   Linpone接口封装
 *
 *@author  chensq
 *@version 1.0
 *@date    2012-5-20
 */

#ifndef LPINTLAYER_H_
#define LPINTLAYER_H_

#include "YKMsgQue.h"
#include "IDBT.h"


#define PRACK_SEND								//prack send support
#define DTMF_RESP                               	//DTMF反馈支持
#define LP_EXPIRES_TIME                     (600) 	//600
#define LP_MESSAGE_BODY_MAX_LEN             (256)


typedef enum
{
	REG_STATUS_OFFLINE  = 0,                //注册状态离线
	REG_STATUS_ONLINE,                      //注册状态在线
}REG_STATUS_EN;

typedef enum
{
	NETWORK_STATUS_OFFLINE = 0,
	NETWORK_STATUS_ONLINE,
}NETWORK_STATUS_EN;

typedef enum
{
	CONN_TYPE_AUDIO = 0,
	CONN_TYPE_VIDEO
}CONN_TYPE_EN;

typedef enum
{
	CALL_DIR_OUTGOING  = 0,                 //去电
	CALL_DIR_INCOMMING,                     //来电
}CALL_DIR_EN;

//呼叫通话过程的状态定义
typedef enum
{
	CALL_STATUS_INCOMING_RECEIVED          = 1,
	CALL_STATUS_INIT                       = 2,
	CALL_STATUS_IN_PROGRESS                = 3,
	CALL_STATUS_RINIGING                   = 4,
	CALL_STATUS_OUTGOING_EARLYMEDIA        = 5,
	CALL_STATUS_CONNECTED                  = 7,
	CALL_STATUS_ERROR                      = 12,
	CALL_STATUS_END                        = 13,
}CALL_STATUS_EN;


typedef enum
{
	CALL_ERR_NORESPONSES,
	CALL_ERR_PROTOCOL,
	CALL_ERR_FAILURE,
	CALL_ERR_OTHER,
}CALL_ERR_EN;


typedef enum
{
	CALL_FAILURE_REASON_DECLINED,
	CALL_FAILURE_REASON_BUSY,
	CALL_FAILURE_REASON_REDIRECT,
	CALL_FAILURE_REASON_TEMPORARILY_UNAVAILABLE,
	CALL_FAILURE_REASON_NOTFOUND,
	CALL_FAILURE_REASON_DONOT_DISTURB,
	CALL_FAILURE_REASON_MEDIA,
	CALL_FAILURE_REASON_OTHER,
}CALL_FAILURE_REASON;



//LP定时器
typedef struct
{
	YK_TIMER_ST     *pstTimer;
	unsigned long	 ulMagicNum;
}LP_TIMER_ST;

#define STOP_CALLING_TIMEOUT		   (5*1000)    //5s
#define DTMF_RECV_TIMROUT              (5*1000)    //5s
#define SIP_REG_TIMROUT                (5*1000*60) //5min
#define SIP_INCOMMING_TIMROUT                (5*1000) //5s

#define CALL_FAILURE_REASON_SIPUNREGISTED    0x1001
#define CALL_FAILURE_REASON_REREGISTER       0x1002
#define CALL_FAILURE_REASON_TOO_MANY_DTMF    0x1003


#define LP_HINT_MUSIC_NAME    "LP/LPConfig/dooropen.wav"
//#define LP_CONFIG_NAME      "LP/config/lpconfig"                 //linphone配置文件绝对路径
//#define LP_IMS_SERVER_REALM      "fj.ctcims.cn"
//#define LP_IMS_SERVER_REALM_GZ   "zte.com"
//#define LP_IMS_SERVER_REALM_GZ2   "gd.ctcims.cn"
#define LP_INVALID_CALL_ID     (0)        	//无效呼叫ID
#define LP_QUE_LEN             (256)      	//linphone消息队列最长长度
#define LP_QUE_WRITE_TIMEOUT   (250)      	//写linphone消息队列满时最长等待时间(毫秒)
#define LP_QUE_READ_TIMEOUT    (250)      	//读linphone消息队列空时最长等待时间(毫秒)




extern YK_MSG_QUE_ST*  g_pstLPMsgQ;


/**
 * @brief        当当前处于通话状态时,通过该接口得知是被叫还是主叫
 * @param[in]
 * @return       CALL_DIR_OUTGOING/CALL_DIR_INCOMMING
 */
CALL_DIR_EN LPGetCallDir(void);

void LPSetCallViewVersion(char cFlg);

/**
 * @brief        通过该接口得知SIP注册状态
 * @param[in]
 * @return       REG_STATUS_OFFLINE/REG_STATUS_ONLINE
 */
REG_STATUS_EN LPGetSipRegStatus(void);

/**
 * @brief        通过该接口变更SIP注册状态
 * @param[in]
 * @return       REG_STATUS_OFFLINE
 */
void LPSetSipRegStatus(REG_STATUS_EN enStatus);

void LPSetNetworkStatus(NETWORK_STATUS_EN enStatus);

/**
 * @brief        通过该接口变更SIP注册状态
 * @param[in]
 * @return       REG_STATUS_OFFLINE
 */
void LPSetSipRegStatusForBV(REG_STATUS_EN enStatus);

/**
 * @brief        Linphone模块资源初始化
 * @param[in]    无
 * @return       0:成功  <0:失败
 */
int LPInit(void);

/**
 * @brief       Linphone资源销毁
 * @param[in]   无
 * @return      无
 */
void LPFini(void);


/**
 * @brief        Linphone命令发送到管道
 * @param[in]    pcCommand:命令
 * @return       无
 */
void LPProcessCommand(const char* pcCommand);


void LPSendDtmf(char value);


void LPSendOpenDoorResp(char *pcTo, char cResult);

#endif /* LPINTLAYER_H_ */
