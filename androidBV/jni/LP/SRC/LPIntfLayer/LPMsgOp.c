/**
 *@file        LPMsgOp.c
 *@brief       LP消息组装和处理
 *@author      chensq
 *@version     1.0
 *@date        2012-5-21
 */



#include <stdio.h>
#include <sal.h>
#include <linphonec.h>
#include <linphonecore.h>
#include <assert.h>
#include "LPIntfLayer.h"
#include "YKMsgQue.h"
#include "IDBT.h"
#ifdef _I6_CC_
#include "I6CCTask.h"
#else
#include "CCTask.h"
#endif
#include "LOGIntfLayer.h"
#include "LPMsgOp.h"
#include "TRManage.h"
#include "IDBTJni.h"
#include "IDBTIntfLayer.h"
#include <TMInterface.h>




/**
 * @brief        供TM调用，TM向SIP发送注册信息
 * @param[in]
 * @return       0:成功  其他:失败
 */
int LPMsgSendTmSipUnRegister()
{
    int iRet = -1;


//    static int count = 0;
//    printf("==============================:%d\n", ++count);
    if(NULL == g_pstLPMsgQ || g_iRunLPTaskFlag == FALSE)
    {
        LOG_RUNLOG_WARN("LP g_pstLPMsgQ is null or g_iRunLPTaskFlag == FALSE");
        return -1;
    }


    LOG_RUNLOG_INFO("LP terminated all calls because sip info need to unregister");
	LPProcessCommand("terminate all");
	sleep(3);

    //先发送取消注册信息
    TMSIP_UNREGISTER_ST* pstUnregisterMsg = (TMSIP_UNREGISTER_ST*)malloc(sizeof(TMSIP_UNREGISTER_ST));
    if( NULL == pstUnregisterMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc TMSIP_UNREGISTER_ST failed");
        return -1;
    }
    memset(pstUnregisterMsg, 0x00, sizeof(TMSIP_UNREGISTER_ST));
    pstUnregisterMsg->uiPrmvType = TMSIP_UNREGISTER;
    iRet = YKWriteQue( g_pstLPMsgQ,  pstUnregisterMsg,  LP_QUE_WRITE_TIMEOUT);
    if(iRet != 0)
    {
    	LOG_RUNLOG_WARN("LP write unregister info to g_pstLPMsgQ error");
        return iRet;
    }

    LOG_RUNLOG_DEBUG("LP LPMsg send msg:\n"
    		"uiPrmvType:TMSIP_UNREGISTER | %x\n",
    		TMSIP_UNREGISTER);

    return iRet;
}




/**
 * @brief        分析CC向SIP发送的呼叫消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessCcsipCall(void *pvMsg, char *pcBuff, int iBuffSize)
{
    CCSIP_CALL_ST * pstMsg = NULL;

    //参數驗证
    if( NULL == pvMsg || NULL == pcBuff || iBuffSize <= 0)
    {
        LOG_RUNLOG_WARN("LP function param error");
        return -1;
    }
    pstMsg = (CCSIP_CALL_ST *)pvMsg;
    if(  pstMsg->uiPrmvType != CCSIP_CALL )
    {
    	LOG_RUNLOG_WARN("LP input msg which uiPrmvType(%u) is error(correct is %d)", pstMsg->uiPrmvType, CCSIP_CALL);
        return -1;
    }

    LOG_RUNLOG_DEBUG("LP current reg status:%d", LPGetSipRegStatus());

    //判断最后一次注册是否成功，未成功，则认为呼叫失败
    if( LPGetSipRegStatus() == REG_STATUS_OFFLINE )
    {
    	LOG_RUNLOG_DEBUG("LP call %s(%d) failed, sip offline", pstMsg->aucPhoneNum, pstMsg->aucMediaType);
    	LPMsgSendSipccCallFailure(CALL_FAILURE_REASON_SIPUNREGISTED, 0);
    	return -1;
    }

    //cc记录
    LOG_RUNLOG_INFO("LP call %s start", pstMsg->aucPhoneNum);

    char *strPos = NULL;
    char shanghaiNum[PHONE_NUM_BUFF_LEN] = {0x00};

    if( (strPos = strstr(pstMsg->aucPhoneNum,  "+8621"))  != NULL )
    {
    	strcpy(shanghaiNum, (strPos+5));
    }

	if( (strPos = strstr(shanghaiNum,  "@sh.ctcims.cn"))  != NULL)
	{
		LOG_RUNLOG_DEBUG("LP shanghai num %s", shanghaiNum);
		*strPos = '\0';
		LOG_RUNLOG_DEBUG("LP change shanghai num -->  %s", shanghaiNum);
		strcpy(pstMsg->aucPhoneNum, shanghaiNum);
		LOG_RUNLOG_DEBUG("LP pstMsg->aucPhoneNum -->  %s", pstMsg->aucPhoneNum);
	}

//	//视频呼叫音频编码只选择G729;语音呼叫音频编码G711优先
//    if(pstMsg->aucMediaType == MEDIA_TYPE_AUDIO)
//    {
////    	LPProcessCommand("codec enable all");
//     	LPProcessCommand("codec disable all");
//     	LPProcessCommand("codec enable 2");
//    }
//    else if(pstMsg->aucMediaType == MEDIA_TYPE_AUDIO_VIDEO)
//    {
//    	LPProcessCommand("codec disable all");
//    	LPProcessCommand("codec enable 2");
//    }

#ifdef HINT_MUSIC
    snprintf(pcBuff, iBuffSize - 1,"call %s %s %s",  pstMsg->aucPhoneNum,
         pstMsg->aucMediaType == MEDIA_TYPE_AUDIO ? " --audio-only" : "",
         pstMsg->aucHintMusicEn == HINT_MUSIC_ENABLE ? "--hint_music_start"LP_HINT_MUSIC_NAME"--hint_music_end" : "");
#else
    snprintf(pcBuff, iBuffSize - 1,"call %s %s",  pstMsg->aucPhoneNum,
         pstMsg->aucMediaType == MEDIA_TYPE_AUDIO ? " --audio-only" : "");
#endif

    if(pstMsg->aucCallPreview == 0)
    {
    	LPSetCallViewVersion(0); //非预览版
    }
    else
    {
    	LPSetCallViewVersion(1);	//预览版
    }

    LOG_RUNLOG_DEBUG("LPMsgQ recv msg:\n"
    		"uiPrmvType:CCSIP_CALL\n"
    		"aucPhoneNum:%s\n"
    		"pstMsg->aucMediaType: %u\n"
    		"pstMsg->aucHintMusicEn: %u\n"
    		"pstMsg->aucCallPreview: %u\n"
    		"pcBuff: %s\n",
    		pstMsg->aucPhoneNum,
    		pstMsg->aucMediaType,
    		pstMsg->aucHintMusicEn,
    		pstMsg->aucCallPreview,
    		pcBuff);

    return 0;
}


/**
 * @brief        分析CC向SIP发送的接听消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessCcsipAnswer(void *pvMsg, char *pcBuff, int iBuffSize)
{


    CCSIP_ANSWER_ST * pstMsg = NULL;

    //参數驗证
    if( NULL == pvMsg || NULL == pcBuff || iBuffSize <= 0)
    {
        LOG_RUNLOG_WARN("LP function param  error");
        return -1;
    }
    pstMsg = (CCSIP_ANSWER_ST *)pvMsg;
    if( pstMsg->uiPrmvType != CCSIP_ANSWER )
    {
    	LOG_RUNLOG_WARN("LP input msg which uiPrmvType(%u) is error(correct is %d)", pstMsg->uiPrmvType, CCSIP_ANSWER);
        return -1;
    }

    snprintf(pcBuff, iBuffSize-1, "answer %ld", pstMsg->ulCallId);

    LOG_RUNLOG_DEBUG("LP LPMsgQ recv msg:\nuiPrmvType:CCSIP_ANSWER\npcBuff: %s\n", pcBuff);

    return 0;
}

/**
 * @brief        分析CC向SIP发送的挂机消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessCcsipTerminate(void *pvMsg, char *pcBuff, int iBuffSize)
{
    CCSIP_TERMINATE_ST * pstMsg = NULL;

    //参數驗证
    if( NULL == pvMsg || NULL == pcBuff || iBuffSize <= 0)
    {
        LOG_RUNLOG_WARN("LP function param  error");
        return -1;
    }
    pstMsg = (CCSIP_TERMINATE_ST *)pvMsg;
    if( pstMsg->uiPrmvType != CCSIP_TERMINATE )
    {
    	LOG_RUNLOG_WARN("LP input msg which uiPrmvType(%u) is error(correct is %d)", pstMsg->uiPrmvType, CCSIP_TERMINATE);
        return -1;
    }

    snprintf(pcBuff, iBuffSize-1, "terminate %ld",  pstMsg->ulCallId);

    LOG_RUNLOG_DEBUG("LP LPMsgQ recv msg:\n"
    		"uiPrmvType:CCSIP_TERMINATE\n"
    		"ulCallId:%ld\n"
    		"pcBuff: %s\n",
    		pstMsg->ulCallId,
    		pcBuff);

    return 0;
}

/**
 * @brief        分析CC向SIP发送的挂断所有的消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessCcsipTerminateAll(void *pvMsg, char *pcBuff, int iBuffSize)
{
    CCSIP_TERMINATE_ALL_ST *pstMsg = NULL;

    //参數驗证
    if( NULL == pvMsg || NULL == pcBuff || iBuffSize <= 0)
    {
        LOG_RUNLOG_WARN("LP function param  error");
        return -1;
    }
    pstMsg = (CCSIP_TERMINATE_ALL_ST *)pvMsg;
    if( pstMsg->uiPrmvType != CCSIP_TERMINATE_ALL )
    {
    	LOG_RUNLOG_WARN("LP input msg which uiPrmvType(%d) is error(correct is %d)", pstMsg->uiPrmvType, CCSIP_TERMINATE_ALL);
        return -1;
    }


    snprintf(pcBuff, iBuffSize-1, "%s", "terminate all");

    LOG_RUNLOG_DEBUG("LP LPMsgQ recv msg:\nuiPrmvType:CCSIP_TERMINATE_ALL\npcBuff: %s\n", pcBuff);


    return 0;
}



///**
// * @brief        分析CC向SIP发送的
// * @param[in]    无
// * @return       0:成功  其他:失败
// */
//int LPMsgCntProcessCcsipCallTimeout(void *pvMsg, char *pcBuff, int iBuffSize)
//{
//	CCSIP_CALL_TIMEOUT_ST *pstMsg = NULL;
//
//    //参數驗证
//    if( NULL == pvMsg || NULL == pcBuff || iBuffSize <= 0)
//    {
//        LOG_RUNLOG_WARN("LP function param  error");
//        return -1;
//    }
//    pstMsg = (CCSIP_CALL_TIMEOUT_ST *)pvMsg;
//    if( pstMsg->uiPrmvType != CCSIP_CALL_TIMEOUT )
//    {
//    	LOG_RUNLOG_WARN("LP input msg which uiPrmvType(%d) is error(correct is %d)", pstMsg->uiPrmvType, CCSIP_CALL_TIMEOUT);
//        return -1;
//    }
//
//
//    snprintf(pcBuff, iBuffSize-1, "%s", "terminate all");
//
//    LOG_RUNLOG_DEBUG("LP LPMsgQ recv msg:\nuiPrmvType:CCSIP_CALL_TIMEOUT\npcBuff: %s\n", pcBuff);
//
//
//    return 0;
//}

/**
 * @brief        分析TM向SIP发送的取消注册的消息
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int LPMsgCntProcessTmsipUnRegister(void *pvMsg, char *pcBuff, int iBuffSize)
{
    TMSIP_UNREGISTER_ST *pstMsg = NULL;

    if(NULL == g_pstLPMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstLPMsgQ is null");
        return -1;
    }

    //参數驗证
    if( NULL == pvMsg || NULL == pcBuff || iBuffSize <= 0)
    {
    	LOG_RUNLOG_WARN("LP function param  error");
        return -1;
    }

    pstMsg = (TMSIP_UNREGISTER_ST *)pvMsg;
    if(  TMSIP_UNREGISTER != pstMsg->uiPrmvType )
    {
    	LOG_RUNLOG_WARN("LP input msg which uiPrmvType(%d) is error(correct is %d)", pstMsg->uiPrmvType, TMSIP_UNREGISTER);
        return -1;
    }

    snprintf(pcBuff, iBuffSize-1, "%s", "unregister");

    LOG_RUNLOG_DEBUG("LP LPMsgQ recv msg:\nuiPrmvType:TMSIP_UNREGISTER\npcBuff: %s\n", pcBuff);

    return 0;
}

/**
 * @brief        分析TM向SIP发送的注册的消息
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgCntProcessTmsipRegister(void *pvMsg, char *pcBuff, int iBuffSize)
{

    TMSIP_REGISTER_ST *pstMsg = NULL;

    if(NULL == g_pstLPMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstLPMsgQ is null");
        return -1;
    }

    //参數驗证
    if( NULL == pvMsg || NULL == pcBuff || iBuffSize <= 0)
    {
    	LOG_RUNLOG_WARN("LP: function param  error");
        return -1;
    }

    //LPMsgSendSipccCallFailure(CALL_FAILURE_REASON_REREGISTER);


    pstMsg = (TMSIP_REGISTER_ST *)pvMsg;
    if(  TMSIP_REGISTER != pstMsg->uiPrmvType  && TMSIP_REGISTER_UPDATE != pstMsg->uiPrmvType)
    {
    	LOG_RUNLOG_WARN("LP input msg which uiPrmvType(%d) is error(correct is %d)", pstMsg->uiPrmvType, TMSIP_REGISTER);
        return -1;
    }

    if( NULL == pstMsg->aucUserName || NULL == pstMsg->aucPassword ||
         NULL == pstMsg->aucProxy || NULL == pstMsg->aucRoute ||
         0 == strlen(pstMsg->aucUserName) || 0 == strlen(pstMsg->aucPassword)  ||
         0 == strlen(pstMsg->aucProxy) || 0 == strlen(pstMsg->aucRoute)  )
    {
    	LOG_RUNLOG_WARN("LP input msg which registering info has null param");
    }
    else
    {
        char acUserName[USER_NAME_BUFF_LEN] = {0x00};
        char acProxy[PROXY_BUFF_LEN] = {0x00};
        char acRoute[ROUTE_BUFF_LEN] = {0x00};

//    	sprintf(acUserName, "sip:%s", pstMsg->aucUserName);
//    	sprintf(acProxy, "sip:%s", pstMsg->aucProxy);
//    	sprintf(acRoute, "sip:%s", pstMsg->aucRoute);

        //暫时取消SIP前缀写入
		sprintf(acUserName, "%s", pstMsg->aucUserName);
		sprintf(acProxy, "%s", pstMsg->aucProxy);
		sprintf(acRoute, "%s", pstMsg->aucRoute);



        LOG_RUNLOG_DEBUG("LP LPMsgCntProcessTmsipRegister < username:%s/passwd:%s/proxy:%s/route:%s >",
        		acUserName, pstMsg->aucPassword, acProxy, acRoute);

        linphonec_push_proxy(acUserName, pstMsg->aucPassword, acProxy, acRoute,pstMsg->uiExpires, TRUE);

    }

    LOG_RUNLOG_DEBUG("LP LPMsgQ recv msg:\nuiPrmvType:TMSIP_REGISTER\npcBuff: %s\n", pcBuff);

    return 0;
}


///**
// * @brief        分析TM向SIP发送的更新注册的消息
// * @param[in]    无
// * @return        0:成功  其他:失败
// */
//int LPMsgCntProcessTmsipRegisterUpdate(void *pvMsg, char *pcBuff, int iBuffSize)
//{
//    TMSIP_REGISTER_UPDATE_ST *pstMsg = NULL;
//
//    //参數驗证
//    if( NULL == pvMsg || NULL == pcBuff || iBuffSize <= 0)
//    {
//        LOG_RUNLOG_ERROR("LP:  param  error[LPMsgCntProcessTmsipRegisterUpdate]");
//        return -1;
//    }
//
//    pstMsg = (TMSIP_REGISTER_UPDATE_ST *)pvMsg;
//    if(  pstMsg->uiPrmvType != TMSIP_REGISTER_UPDATE )
//    {
//        LOG_RUNLOG_ERROR("LP: input msg which uiPrmvType(%d) is error(correct is %d) [LPMsgCntProcessTmsipRegisterUpdate]", pstMsg->uiPrmvType, TMSIP_REGISTER_UPDATE);
//        return -1;
//    }
//
//
//    printf("----------------------------------------\n");
//    printf("LPMsgQ recv msg:\n");
//    printf("uiPrmvType:TMSIP_REGISTER_UPDATE\n");
//
//    printf("pcBuff: %s\n", pcBuff);
//    printf("----------------------------------------\n");
//
//
//    return 0;
//}

/**
 * @brief        SIP向CC发送语音来电消息
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipccAudioCall(char *pcDisplayName,  long lId, char *pcSipInfo)
{
    int iRet = -1;

    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }
    //输入参数验证
    if( lId <= 0 || pcDisplayName == NULL || strlen(pcDisplayName) >= PHONE_NUM_BUFF_LEN)
    {
    	LOG_RUNLOG_WARN("LP function param  error");
        return -1;
    }
    SIPCC_AUDIO_INCOMING_ST* pstMsg = (SIPCC_AUDIO_INCOMING_ST*)malloc(sizeof(SIPCC_AUDIO_INCOMING_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPCC_AUDIO_INCOMING_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_AUDIO_INCOMING_ST));
    pstMsg->uiPrmvType = SIPCC_AUDIO_INCOMING;
    pstMsg->ulInComingCallId = lId;
    memcpy(pstMsg->aucPhoneNum, pcDisplayName, strlen(pcDisplayName));
    if(pcSipInfo != NULL)
    {
    	memcpy(pstMsg->aucUserName, pcSipInfo, strlen(pcSipInfo));
    }

    LOG_RUNLOG_DEBUG("LP LPMsgSendSipccAudioCall (callid:%ld/phone:%s/sipinfo:%s)",
    		pstMsg->ulInComingCallId, pstMsg->aucPhoneNum, pstMsg->aucUserName);
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

//    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\n"
//    		"uiPrmvType:SIPCC_AUDIO_INCOMING | %x\n"
//    		"ulInComingCallId: %ld\n"
//    		"aucPhoneNum: %s\n"
//    		"aucUserName: %s\n",
//    		SIPCC_AUDIO_INCOMING,
//    		pstMsg->ulInComingCallId,
//    		pstMsg->aucPhoneNum,
//    		pstMsg->aucUserName);

    return iRet;
}



/**
 * @brief        SIP向CC发送message消息
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipccMessage(char *phone, char *pcSipInfo, int msgSeq)
{
    int iRet = -1;

    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }
    //输入参数验证
    if(pcSipInfo == NULL || phone == NULL)
    {
    	LOG_RUNLOG_WARN("LP function param  error");
        return -1;
    }
    SIPCC_MESSAGE_INCOMING_ST* pstMsg = (SIPCC_MESSAGE_INCOMING_ST*)malloc(sizeof(SIPCC_MESSAGE_INCOMING_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPCC_MESSAGE_INCOMING_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_MESSAGE_INCOMING_ST));
    pstMsg->uiPrmvType = SIPCC_MESSAGE_INCOMING;
    pstMsg->uiUserDataType = 0;
    pstMsg->uiMsgSeq = msgSeq;
    memcpy(pstMsg->aucUserName, pcSipInfo, strlen(pcSipInfo));
    memcpy(pstMsg->aucPhoneNum, phone, strlen(phone));

    LOG_RUNLOG_DEBUG("LP LPMsgSendSipccMessage (sipinfo:%s msgseq:%d)", pstMsg->aucUserName, pstMsg->uiMsgSeq);
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

    return iRet;
}


/**
 * @brief        SIP向CC发送视频来电消息
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipccVideoCall(char *pcDisplayName,  long lId)
{
    int iRet = -1;

    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }

    //输入参数验证
    if( lId <= 0 || pcDisplayName == NULL || strlen(pcDisplayName) >= PHONE_NUM_BUFF_LEN)
    {
    	LOG_RUNLOG_WARN("LP function param  error");
        return -1;
    }
    SIPCC_VIDEO_INCOMING_ST* pstMsg = (SIPCC_VIDEO_INCOMING_ST*)malloc(sizeof(SIPCC_VIDEO_INCOMING_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPCC_VIDEO_INCOMING_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_VIDEO_INCOMING_ST));
    pstMsg->uiPrmvType = SIPCC_VIDEO_INCOMING;
    pstMsg->ulInComingCallId = lId;
    memcpy(pstMsg->aucPhoneNum, pcDisplayName, strlen(pcDisplayName));

	LOG_RUNLOG_DEBUG("LP LPMsgSendSipccVideoCall (callid:%ld/phone:%s)",
			pstMsg->ulInComingCallId, pstMsg->aucPhoneNum);
   iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

//    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\n"
//    		"uiPrmvType:SIPCC_VIDEO_INCOMING | %x\n"
//    		"ulInComingCallId: %ld\n"
//    		"aucPhoneNum: %s\n",
//    		SIPCC_VIDEO_INCOMING,
//    		pstMsg->ulInComingCallId,
//    		pstMsg->aucPhoneNum);

    return iRet;
}

int LPMsgSendSipccRecvEarlyMedia(void)
{
	int iRet = -1;
	if(NULL == g_pstCCMsgQ)
	{
		LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
		return -1;
	}
	SIPCC_RECV_EARLY_MEDIA_ST* pstMsg= (SIPCC_RECV_EARLY_MEDIA_ST*)malloc(sizeof(SIPCC_RECV_EARLY_MEDIA_ST));
	if( NULL == pstMsg )
	{
		LOG_RUNLOG_WARN("LP malloc SIPCC_RECV_EARLY_MEDIA_ST failed");
		return -1;
	}
	memset(pstMsg, 0x00, sizeof(SIPCC_RECV_EARLY_MEDIA_ST));
	pstMsg->uiPrmvType = SIPCC_RECV_EARLY_MEDIA;

	LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\n"
			"uiPrmvType:SIPCC_RECV_EARLY_MEDIA | %x\n",
			SIPCC_RECV_EARLY_MEDIA);
	iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);



	return iRet;
}



int LPMsgSendSipccCalleeProcess(long lId)
{
    int iRet = -1;
    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }
    SIPCC_PROCESS_ST* pstMsg= (SIPCC_PROCESS_ST*)malloc(sizeof(SIPCC_PROCESS_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPCC_PROCESS_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_PROCESS_ST));
    pstMsg->uiPrmvType = SIPCC_PROCESS;
    pstMsg->ulCallId = lId;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPCC_PROCESS | %x\n", SIPCC_PROCESS);
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);


    return iRet;
}


int LPMsgSendSipccCalleeRing(long lId)
{
    int iRet = -1;
    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }
    SIPCC_RING_ST* pstMsg= (SIPCC_RING_ST*)malloc(sizeof(SIPCC_RING_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPCC_RING_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_RING_ST));
    pstMsg->uiPrmvType = SIPCC_RING;
    pstMsg->ulCallId = lId;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPCC_RING | %x\n", SIPCC_RING);
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);


    return iRet;
}

/**
 * @brief        SIP向CC发送被叫方接听消息
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipccCalleePickup(long lId)
{

    int iRet = -1;
    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }
    SIPCC_CALLEE_PICK_UP_ST* pstMsg= (SIPCC_CALLEE_PICK_UP_ST*)malloc(sizeof(SIPCC_CALLEE_PICK_UP_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPCC_CALLEE_PICK_UP_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_CALLEE_PICK_UP_ST));
    pstMsg->uiPrmvType = SIPCC_CALLEE_PICK_UP;
    pstMsg->ulCallId = lId;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPCC_CALLEE_PICK_UP | %x\n", SIPCC_CALLEE_PICK_UP);
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);


    return iRet;
}



int LPMsgSendSipABStopRing()
{
	int iRet = -1;
	if(NULL == g_pstABMsgQ)
	{
		LOG_RUNLOG_WARN("LP g_pstABMsgQ is null");
		return -1;
	}
	SIPAB_STOP_RING_ST* pstMsg= (SIPAB_STOP_RING_ST*)malloc(sizeof(SIPAB_STOP_RING_ST));
	if(pstMsg == NULL)
	{
		LOG_RUNLOG_ERROR("LP malloc LPMsgSendSipABStopRing failed");
		return -1;
	}
	memset(pstMsg, 0x00, sizeof(SIPAB_STOP_RING_ST));
	pstMsg->uiPrmvType = SIPAB_STOP_RING;

	iRet = YKWriteQue( g_pstABMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

	LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPAB_STOP_RING | 0x%04x\n", SIPAB_STOP_RING);

	return iRet;
}


/**
 * @brief        SIP向CC发送被叫方提示音播放结束
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipccCalleeMusicEnd(void)
{
    int iRet = -1;
    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }
    SIPCC_CALLEE_MUSIC_END_ST* pstMsg= (SIPCC_CALLEE_MUSIC_END_ST*)malloc(sizeof(SIPCC_CALLEE_MUSIC_END_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc LPMsgSendSipccCalleeMusicEnd failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_CALLEE_MUSIC_END_ST));
    pstMsg->uiPrmvType = SIPCC_CALLEE_MUSIC_END;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\n"
    		"uiPrmvType:SIPCC_CALLEE_MUSIC_END | %x\n",
    		SIPCC_CALLEE_MUSIC_END);
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);


    return iRet;
}

int LPMsgSendSipABRegStatus(REG_STATUS_EN enRegStatus)
{
	int iRet = -1;
	if(NULL == g_pstABMsgQ)
	{
		LOG_RUNLOG_WARN("LP g_pstABMsgQ is null");
		return -1;
	}
	SIPAB_REG_STATUS_ST* pstMsg= (SIPAB_REG_STATUS_ST*)malloc(sizeof(SIPAB_REG_STATUS_ST));
	if(pstMsg == NULL)
	{
		LOG_RUNLOG_ERROR("LP malloc LPMsgSendSipABRegStation failed");
		return -1;
	}
	memset(pstMsg, 0x00, sizeof(SIPAB_REG_STATUS_ST));

	pstMsg->uiRegStatus = (unsigned int)enRegStatus;

	pstMsg->uiPrmvType = SIPAB_REG_STATUS;

	iRet = YKWriteQue( g_pstABMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

	LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPAB_REG_STATUS | 0x%04x\n", SIPAB_REG_STATUS);

	return iRet;
}

int LPMsgSendSipABNetworkStatus(NETWORK_STATUS_EN enStatus)
{
	int iRet = -1;
	if(NULL == g_pstABMsgQ)
	{
		LOG_RUNLOG_WARN("LP g_pstABMsgQ is null");
		return -1;
	}

	SIPAB_NETWORK_STATUS_ST *pstMsg= (SIPAB_NETWORK_STATUS_ST*)malloc(sizeof(SIPAB_NETWORK_STATUS_ST));
	if(pstMsg == NULL)
	{
		LOG_RUNLOG_ERROR("LP malloc LPMsgSendSipABNetworkStatus failed");
		return -1;
	}
	memset(pstMsg, 0x00, sizeof(SIPAB_NETWORK_STATUS_ST));

	pstMsg->uiNetworkStatus = (unsigned int)enStatus;

	pstMsg->uiPrmvType = SIPAB_NETWORK_STATUS;

	iRet = YKWriteQue( g_pstABMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

	LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPAB_NETWORK_STATUS | 0x%04x\n", SIPAB_NETWORK_STATUS);

	return iRet;
}

/**
 * @brief        SIP向CC发送本次呼叫或通话已挂机
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipccCalleeHandOff(long lId)
{
    int iRet = -1;
    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }
    SIPCC_CALLEE_HANG_OFF_ST* pstMsg= (SIPCC_CALLEE_HANG_OFF_ST*)malloc(sizeof(SIPCC_CALLEE_HANG_OFF_ST));
    if(pstMsg == NULL)
    {
        LOG_RUNLOG_ERROR("LP malloc LPMsgSendSipccCalleeHandOff failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_CALLEE_HANG_OFF_ST));
    //-----add by lcx-----
    pstMsg->ulCallId = lId;
    //--------------------
    pstMsg->uiPrmvType = SIPCC_CALLEE_HANG_OFF;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPCC_CALLEE_HANG_OFF | %x\n", SIPCC_CALLEE_HANG_OFF);
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);


    return iRet;
}

/**
 * @brief        SIP向CC发送本次呼叫或通话已挂机
 * @param[in]    iErrorCode:错误代码
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccCallFailure(int iErrorCode, long id)
{
    int iRet = -1;

#ifndef LP_ONLY
#if _TM_TYPE_ == _YK069_
			TMUpdateAlarmState(TM_ALARM_SIP_CALL, '1'); //呼叫失败告警
			TMUpdateAlarmState(TM_ALARM_CALL_STATE, '0'); //SIP呼叫状态空闲告緊
#else
			TMUpdateAlarmState(TM_ALARM_SIP_CALL, "1");
			TMUpdateAlarmState(TM_ALARM_CALL_STATE, "0"); //SIP呼叫状态空闲告緊
#endif
#endif

    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }

    SIPCC_CALLEE_ERR_ST* pstMsg = (SIPCC_CALLEE_ERR_ST*)malloc(sizeof(SIPCC_CALLEE_ERR_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPCC_CALLEE_ERR_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_CALLEE_ERR_ST));
    pstMsg->uiPrmvType = SIPCC_CALLEE_ERR;
    pstMsg->uiErrorCause = iErrorCode;
    pstMsg->ulCallId = id;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPCC_CALLEE_ERR | %x\niErrorCode:%ld\n callId:%d\n",SIPCC_CALLEE_ERR,iErrorCode, pstMsg->ulCallId);
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);


    return iRet;
}


/**
 * @brief        SIP向CC发送收到DTMF信号消息
 * @param[in]    iDtmf:DTMF信号值
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccDtmf(int iDtmf)
{
    int iRet = -1;

    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }

    SIPCC_CALLEE_SEND_DTMF_ST* pstMsg = (SIPCC_CALLEE_SEND_DTMF_ST*)malloc(sizeof(SIPCC_CALLEE_SEND_DTMF_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPCC_CALLEE_SEND_DTMF_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_CALLEE_SEND_DTMF_ST));
    pstMsg->uiPrmvType = SIPCC_CALLEE_SEND_DTMF;
    pstMsg->iDtmfType = iDtmf;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPCC_CALLEE_SEND_DTMF | %x\niDtmfType:%d\n", SIPCC_CALLEE_SEND_DTMF, iDtmf);
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);



    return iRet;
}

/**
 * @brief        SIP向CC发送停止呼叫指令
 * @param[in]
 * @return       0:成功  其他:失败
 */
int LPMsgSendSipccStopcalling(void)
{
    int iRet = -1;

    if(NULL == g_pstCCMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstCCMsgQ is null");
        return -1;
    }

    SIPCC_STOP_CALLING_ST* pstMsg = (SIPCC_STOP_CALLING_ST*)malloc(sizeof(SIPCC_STOP_CALLING_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPCC_STOP_CALLING_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPCC_STOP_CALLING_ST));
    pstMsg->uiPrmvType = SIPCC_STOP_CALLING;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg: SIPCC_STOP_CALLING");
    iRet = YKWriteQue( g_pstCCMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

    return iRet;
}





/**
 * @brief        供TM调用，TM向SIP发送注册信息
 * @param[in]
 * @return       0:成功  其他:失败
 */
int LPMsgSendTmSipRegister(const char *pcUserName, const char *pcPassword,
        const char *pcProxy, const char *pcRoute)
{
    int iRet = -1;


//    static int count = 0;
//    printf("==============================:%d\n", ++count);
    if(NULL == g_pstLPMsgQ || g_iRunLPTaskFlag == FALSE)
    {
        LOG_RUNLOG_WARN("LP g_pstLPMsgQ is null or g_iRunLPTaskFlag == FALSE");
        return -1;
    }

    if( NULL == pcUserName || NULL == pcPassword || NULL == pcProxy || NULL == pcRoute)
    {
    	LOG_RUNLOG_WARN("LP input param error");
        return -1;
    }
    if( USER_NAME_LEN < strlen(pcUserName) || PASSWORD_LEN < strlen(pcPassword)  ||
         PROXY_LEN < strlen(pcProxy) || ROUTE_LEN < strlen(pcRoute) )
    {
    	LOG_RUNLOG_WARN("LP input param len too long");
        return -1;
    }

    LOG_RUNLOG_INFO("LP terminated all calls because sip info need to update");
	LPProcessCommand("terminate all");
//	sleep(3);

//    //先发送取消注册信息
//    TMSIP_UNREGISTER_ST* pstUnregisterMsg = (TMSIP_UNREGISTER_ST*)malloc(sizeof(TMSIP_UNREGISTER_ST));
//    if( NULL == pstUnregisterMsg )
//    {
//        LOG_RUNLOG_ERROR("LP malloc TMSIP_UNREGISTER_ST failed");
//        return -1;
//    }
//    memset(pstUnregisterMsg, 0x00, sizeof(TMSIP_UNREGISTER_ST));
//    pstUnregisterMsg->uiPrmvType = TMSIP_UNREGISTER;
//    iRet = YKWriteQue( g_pstLPMsgQ,  pstUnregisterMsg,  LP_QUE_WRITE_TIMEOUT);
//    if(iRet != 0)
//    {
//    	LOG_RUNLOG_WARN("LP write unregister info to g_pstLPMsgQ error");
//        return iRet;
//    }

    //再发送注册消息
    TMSIP_REGISTER_ST* pstMsg = (TMSIP_REGISTER_ST*)malloc(sizeof(TMSIP_REGISTER_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc TMSIP_REGISTER_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(TMSIP_REGISTER_ST));
    pstMsg->uiPrmvType = TMSIP_REGISTER;
    memcpy(pstMsg->aucUserName, pcUserName, strlen(pcUserName));
    memcpy(pstMsg->aucPassword, pcPassword, strlen(pcPassword));
    memcpy(pstMsg->aucProxy, pcProxy, strlen(pcProxy));
    memcpy(pstMsg->aucRoute, pcRoute, strlen(pcRoute));
    pstMsg->uiExpires = LP_EXPIRES_TIME;

    LOG_RUNLOG_DEBUG("LP LPMsg send msg:\n"
    		"uiPrmvType:TMSIP_REGISTER | %x\n"
    		"aucUserName: %s\n"
    		"aucPassword: %s\n"
    		"aucProxy: %s\n"
    		"aucRoute: %s\n"
    		"uiExpires: %d\n",
    		TMSIP_REGISTER,
    		pstMsg->aucUserName,
    		pstMsg->aucPassword,
    		pstMsg->aucProxy,
    		pstMsg->aucRoute,
    		pstMsg->uiExpires);
    iRet = YKWriteQue( g_pstLPMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);



    return iRet;
}





#ifdef _YK_GYY_
/**
 * @brief
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipTmSoftUpgrade(void)
{
    int iRet = -1;

    if(NULL == g_pstTMMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstTMMsgQ is null");
        return -1;
    }

    SIPTM_SOFT_UPDATE_ST* pstMsg = (SIPTM_SOFT_UPDATE_ST*)malloc(sizeof(SIPTM_SOFT_UPDATE_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPTM_SOFT_UPDATE_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPTM_SOFT_UPDATE_ST));
    pstMsg->uiPrmvType = SIPTM_SOFT_UPDATE;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPTM_SOFT_UPDATE | %x\n", SIPTM_SOFT_UPDATE);
    iRet = YKWriteQue( g_pstTMMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);


    return iRet;
}

/**
 * @brief
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipTmConfigUpdate(void)
{
    int iRet = -1;

    if(NULL == g_pstTMMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstTMMsgQ is null");
        return -1;
    }

    SIPTM_CONFIG_UPDATE_ST* pstMsg = (SIPTM_CONFIG_UPDATE_ST*)malloc(sizeof(SIPTM_CONFIG_UPDATE_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPTM_CONFIG_UPDATE_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPTM_CONFIG_UPDATE_ST));
    pstMsg->uiPrmvType = SIPTM_CONFIG_UPDATE;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPTM_CONFIG_UPDATE | %x\n", SIPTM_CONFIG_UPDATE);
    iRet = YKWriteQue( g_pstTMMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

    return iRet;
}

/**
 * @brief
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipTmPwdUpdate(void)
{
    int iRet = -1;

    if(NULL == g_pstTMMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstTMMsgQ is null");
        return -1;
    }

    SIPTM_PWD_UPDATE_ST* pstMsg = (SIPTM_PWD_UPDATE_ST*)malloc(sizeof(SIPTM_PWD_UPDATE_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPTM_PWD_UPDATE_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPTM_PWD_UPDATE_ST));
    pstMsg->uiPrmvType = SIPTM_PWD_UPDATE;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPTM_PWD_UPDATE | %x\n", SIPTM_PWD_UPDATE);
    iRet = YKWriteQue( g_pstTMMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);


    return iRet;
}

/**
 * @brief
 * @param[in]    无
 * @return        0:成功  其他:失败
 */
int LPMsgSendSipTmCardUpdate(void)
{
    int iRet = -1;

    if(NULL == g_pstTMMsgQ)
    {
        LOG_RUNLOG_WARN("LP g_pstTMMsgQ is null");
        return -1;
    }

    SIPTM_CARD_UPDATE_ST* pstMsg = (SIPTM_CARD_UPDATE_ST*)malloc(sizeof(SIPTM_CARD_UPDATE_ST));
    if( NULL == pstMsg )
    {
        LOG_RUNLOG_ERROR("LP malloc SIPTM_CARD_UPDATE_ST failed");
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(SIPTM_CARD_UPDATE_ST));
    pstMsg->uiPrmvType = SIPTM_CARD_UPDATE;

    LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:SIPTM_CARD_UPDATE | %x\n", SIPTM_CARD_UPDATE);
    iRet = YKWriteQue( g_pstTMMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

    return iRet;
}
#endif



#define RESP_OK "OK"
#define RESP_ERROR "ERROR"
/**
 * @brief
 * @param[in]    to目标 msgSeq消息序号 res反馈结果(0:ok -1:err)
 * @return
 */
static char g_acMsgCallResp[USER_NAME_BUFF_LEN] = {0x00};
void LPMsgUpdateCallRespTo(char *to)
{
	memset(g_acMsgCallResp, 0x00, USER_NAME_BUFF_LEN);
	memcpy(g_acMsgCallResp, to, USER_NAME_BUFF_LEN);
}
void LPMsgSendCallResp(int msgSeq, int res)
{
    if(NULL == g_acMsgCallResp)
    {
        LOG_RUNLOG_WARN("LP LPMsgSendCallResp param error");
        return;
    }
	char msgBody[LP_MESSAGE_BODY_MAX_LEN+1] = {0x00};
	char acResp[LP_MESSAGE_BODY_MAX_LEN+1+32] = {0x00};

	snprintf(msgBody, LP_MESSAGE_BODY_MAX_LEN, "<?xml version&\"1.0\" encoding&\"UTF-8\"?>"
			"<MESSAGE Version&\"1.0\">"
			"<HEADER ServiceType&\"CALL_MSG\" MsgType&\"MSG_ANSWER_RSP\" MsgSeq&\"%d\"/>"
			"<REUSULT Value&\"%s\" ErrorCode&\"0\"/>"
			"</MESSAGE>", msgSeq, res == 0 ? RESP_OK : RESP_ERROR);
	LOG_RUNLOG_DEBUG("LP LPMsgSendDoorOpenResp msgBody:\n%s", msgBody);


	snprintf(acResp, sizeof(acResp)-1, "chat %s %s", g_acMsgCallResp, msgBody);
	LOG_RUNLOG_DEBUG("LP will exec:%s", acResp);
	LPProcessCommand(acResp);
}

/**
 * @brief
 * @param[in]     to目标 msgSeq消息序号 res反馈结果(0:ok -1:err)
 * @return
 */
void LPMsgSendDoorOpenResp(char *to, int msgSeq, int res)
{
    if(NULL == to)
    {
        LOG_RUNLOG_WARN("LP LPMsgSendDoorOpenResp param error");
        return;
    }

	char msgBody[LP_MESSAGE_BODY_MAX_LEN+1] = {0x00};
	char acResp[LP_MESSAGE_BODY_MAX_LEN+1+32] = {0x00};

	snprintf(msgBody, LP_MESSAGE_BODY_MAX_LEN, "<?xml version&\"1.0\" encoding&\"UTF-8\" ?>"
			"<MESSAGE Version&\"1.0\">"
			"<HEADER ServiceType&\"ACCESS_MSG\" MsgType&\"MSG_UNLOCK_RSP\" MsgSeq&\"%d\"/>"
			"<REUSULT Value&\"%s\" ErrorCode&\"0\"/>"
			"</MESSAGE>", msgSeq, res == 0 ? RESP_OK : RESP_ERROR);
	LOG_RUNLOG_DEBUG("LP LPMsgSendDoorOpenResp msgBody:\n%s", msgBody);


	snprintf(acResp, sizeof(acResp)-1, "chat %s %s", to, msgBody);
	LOG_RUNLOG_DEBUG("LP will exec:%s", acResp);
	LPProcessCommand(acResp);
}
