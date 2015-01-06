/**
 *@file        LPMsgOp.h
 *@brief       LP消息组装和处理
 *@author      chensq
 *@version     1.0
 *@date        2012-5-21
 */


#ifndef LPBUSLAYER_H_
#define LPBUSLAYER_H_

#include "YKMsgQue.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef enum
{
	SIP_MSG_TYPE_MESSAGE = 1,
	SIP_MSG_TYPE_NOTIFY,
	SIP_MSG_TYPE_PUSHINFO,
	SIP_MSG_TYPE_CALL_MSG
}SIP_MSG_TYPE_EN;


//开门消息
typedef struct
{
	char cCntType;            //MESSAGE 1 / notify 2 / pushInfo 3
	char acServiceType[32];   //"ACCESS_MSG"
	char acMsgType[32];       //"MSG_UNLOCK_REQ"
	int  iMsgSeq;             //"1"
}SIP_MESSAGE_OC_ST;

//门禁消息通知
typedef struct
{
	//MESSAGE 1 / notify 2 / pushInfo 3
	char cCntType;
	// "notify":    向终端通知信息
	// "report":    终端上报信息
	// "pushinfo":  业务向终端推送消息
	char acServiceType[32];
	// accesssoftupgrade:    通知软件升级消息
	// accessconfigupdate：  通知配置参数更新
	// accesspwdupdate:      门禁密码更新
	// accesscardupdate:     门禁卡更新
	char acMsgType[32];
	//char acContent[512];
	//char acDescription[128];
	//int  iAutoLink;
	//char acLinkUrl[255];
}SIP_MESSAGE_DOORPLATFORM_ST;







/**
 * @brief        分析CC向SIP发送的呼叫消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessCcsipCall(void *pvMsg, char *pcBuff, int iBuffSize);


/**
 * @brief        分析CC向SIP发送的接听消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessCcsipAnswer(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        分析CC向SIP发送的挂机消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessCcsipTerminate(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        分析CC向SIP发送的挂断所有的消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessCcsipTerminateAll(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        分析TM向SIP发送的取消注册的消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessTmsipUnRegister(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        分析TM向SIP发送的注册的消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessTmsipRegister(void *pvMsg, char *pcBuff, int iBuffSize);

///**
// * @brief        分析TM向SIP发送的更新注册的消息
// * @param[in]    无
// * @return       0:成功  其他:失败
// */
//int LPMsgCntProcessTmsipRegisterUpdate(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        SIP向CC发送语音来电消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccAudioCall(char *pcDisplayName,  long lId, char *pcSipInfo);

/**
 * @brief        SIP向CC发送视频来电消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccVideoCall(char *pcDisplayName,  long lId);

/**
 * @brief        SIP向CC发送被叫方接听消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccCalleePickup(long lId);

/**
 * @brief        SIP向CC发送被叫方提示音播放结束消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccCalleeMusicEnd(void);

/**
 * @brief        SIP向CC发送停止呼叫指令
 * @param[in]
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccStopcalling(void);


/**
 * @brief        SIP向CC发送本次呼叫或通话已挂机消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccCalleeHandOff(long lId);

/**
 * @brief        SIP向CC发送本次呼叫或通话已挂机消息
 * @param[in]    iErrorCode:错误代码
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccCallFailure(int iErrorCode, long lId);

/**
 * @brief        SIP向CC发送收到DTMF信号消息
 * @param[in]    iDtmf:DTMF信号值
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccDtmf(int iDtmf);



/**
 * @brief        供TM调用，TM向SIP发送注册信息
 * @param[in]
 * @return       0:成功  其他:失败
 */
int LPMsgSendTmSipRegister(const char *pcUserName, const char *pcPassword,
       const char *pcProxy, const char *pcRoute);



int LPMsgSendSipABStopRing();






#ifdef _YK_GYY_
int LPMsgSendSipTmSoftUpgrade(void);
int LPMsgSendSipTmConfigUpdate(void);
int LPMsgSendSipTmPwdUpdate(void);
int LPMsgSendSipTmCardUpdate(void);
#endif


#ifdef __cplusplus
};
#endif


#endif /* LPBUSLAYER_H_ */
