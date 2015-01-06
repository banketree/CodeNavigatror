/*********************************************************************
*  Copyright (C), 2001 - 2012, 福建邮科通信技术有限公司.
*  File			:
*  Author		:lcx
*  Version 		:
*  Date			:
*  Description	:
*  History		: 更新记录 .
*            	  <修改作者>  <修改时间>  <修改描述>
**********************************************************************/
#include "IDBT.h"

#ifdef _I6_CC_

#include "YKApi.h"
#include "YKTimer.h"
#include "YKMsgQue.h"
#include "I6CCTask.h"
#include "LPIntfLayer.h"
#include "LOGIntfLayer.h"
#include "TMInterface.h"
#include "FPInterface.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "Snapshot.h"
#include "SMEventHndl.h"
#include "LANIntfLayer.h"

#ifdef _YK_IMX27_AV_
#include "SMEventHndlImx27.h"
#include "CaptureJpeg.h"
#endif

//是否有室内机
#define AB_HAVE_INDOOR_MACHINE


//开门类型
#define	OD_AUDIO_INCOMING		(1)
#define	OD_SIP_MASSAGE			(2)
#define	OD_SOCKET				(3)
#define	OD_DTMF					(4)

//CC 内部定时器消息ID
#define	TIMERID_DELAY_CALL			(0x01)			//延时呼叫定时器
#define	TIMERID_CALL_WAIT			(0x02)			//呼叫等待定时器
#define	TIMERID_TALK_WAIT			(0x03)			//通话超时定时器
#define TIMERID_MONITOR_WAIT		(0x04)			//监控超时定时器
#define	TIMERID_PIC_CONNECT			(0x05)			//个推连接超时定时器
#define	TIMERID_PIC_RING			(0x06)			//个推振铃超时定时器
#define	TIMERID_OPENDOOR			(0x07)			//开门后挂断超时定时器
#define TIMERID_LAN_CALL_WAIT		(0X08)			//室内机呼叫等待定时器
#define TIMERID_LAN_TALK_WAIT		(0x09)			//室内机通话超时定时器

//定时器超时时间
#define	TM_MONITOR_TIMEOUT			(30*1000)		//监控超时时间
#define	TM_PIC_CONNECT_TIMEOUT		(7*1000)		//个推连接超时时间
#define	TM_PIC_RING_TIME			(15*1000)		//个推振铃时间
#define	TM_OPENDOOR_TIMEOUT			(5*1000)	    //开门后挂断超时时间

#define	CC_QUE_LEN             		(256)      		//CC消息队列最长长度

#define	ERR_TIMEOUT_CALL_WAIT		(901)			//SIP呼叫错误（呼叫超时）编号

static int	g_CCFsmState = 0;			//CC当前状态

#define CC_NO_ROOM "000000000000"		//无对应电话号码时日志格式

CC_TIMER_ST g_CcTimer = { 0 };			//CC内部超时定时器

YK_MSG_QUE_ST  *g_pstCCMsgQ;			//CC消息队列

YKHandle g_CCThreadId;					//CC运行线程ID

int g_iRunCCTaskFlg;					//CC线程运行使能位

static unsigned long g_ulCurCallId = 0;			//当前视频监控CALLID

static unsigned long g_ulCurErrId = 0;						//上一次呼叫错误通话CALLID

static unsigned int g_iIsOpenDoorFlg = 0;         			//是否开门标志位

static unsigned int g_iHaveIndoorMachine = 1;			//是否有室内机

static unsigned int g_iIndoorHangoff = 0;				//是否室内机已挂断

static unsigned int g_iSipHangoff = 0;				//是否客户端已挂断

static unsigned int g_iABHaveCall = 0;				//是否已经开始呼叫

static unsigned int g_iCurSipIsValid = 0;		   //当前sip呼叫是否有效

static unsigned int g_iCurRoomNumCanCallSip = 0;		   //是否可以呼叫SIP号码

CC_TIMEOUT_VALUE_ST g_stCCTimeoutValue = { 0 };					//CC定时器相关操作超时时间

TM_I6NUM_INFO_ST g_stCurPhoneNumInfo;

unsigned char g_iCurRoom[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;  		//当前呼叫的房号（第一位表示设备类型）

unsigned char g_iLanRoom[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;		//传给室内机的房号

char g_acLanMonitorIP[32] = {0x0};

extern void PICTakePicture(unsigned char *pcSipAccount);
extern void PICTimeOut();
extern void PICCallBegin();  // "#"号键按完

extern YK_MSG_QUE_ST  *g_pstABMsgQ;

int CCTaskInit(void)
{
	g_CCFsmState = CCFSM_IDLE;	                                     //有限状态机状态初始化

	//初始化定时器
	g_stCCTimeoutValue.iMonitorTImeout = TM_MONITOR_TIMEOUT;		 //30S
	g_stCCTimeoutValue.iPicConnectTimeout = TM_PIC_CONNECT_TIMEOUT;	 //7S
	g_stCCTimeoutValue.iPicRingTime = TM_PIC_RING_TIME;				 //15S
	g_stCCTimeoutValue.iOpenDoorTimeout = TM_OPENDOOR_TIMEOUT;		 //5S

    g_pstCCMsgQ  = YKCreateQue(CC_QUE_LEN);                          //创建CC消息队列；
	if( NULL == g_pstCCMsgQ )
	{
	    LOG_RUNLOG_ERROR("CC YKCreateQue failed (CC Init)");
	    return FAILURE;
	}

    g_iRunCCTaskFlag = TRUE;		                                 //CCTask 运行标志置位
    g_CCThreadId = YKCreateThread(CCTask, NULL);                     //创建CCTask
    if(NULL == g_CCThreadId)
    {
    	LOG_RUNLOG_ERROR("CC YKCreateThread failed (CC Init)");
        return FAILURE;
    }

    return SUCCESS;
}

void CCTaskUninit(void)
{
    CCCC_CANCAL_EVENT_ST *pstMsg = YKNew0(CCCC_CANCAL_EVENT_ST, 1);
    if(NULL == pstMsg)
    {
        return ;
    }
    pstMsg->uiPrmvType = CCCC_CANCAL_EVENT;
    YKWriteQue(g_pstCCMsgQ , pstMsg,  0);

    if(NULL != g_CCThreadId)
    {
        YKJoinThread(g_CCThreadId);

        YKDestroyThread(g_CCThreadId);
    }

    YKDeleteQue(g_pstCCMsgQ);
}

void *CCTask(void *pvCtx)
{
    while(g_iRunCCTaskFlag)		                                    //CCTask运行标志为真
    {
        CCFsm();
    }

    return NULL;
}

void CCFsm(void)
{
    int iErrCode;
    void *pvPrmvType = NULL;

    iErrCode = YKReadQue( g_pstCCMsgQ, &pvPrmvType,  0);			//阻塞读取消息
    if ( 0 != iErrCode || NULL == pvPrmvType )
    {
        return;
    }

    if(*(( unsigned int *)pvPrmvType) == CCCC_CANCAL_EVENT)
    {
        g_iRunCCTaskFlag = FALSE;
        return ;
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_ERR)
    {
    	SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;

    	if(g_ulCurErrId >= pstRcvMsg->ulCallId)
    	{
    		LOG_RUNLOG_DEBUG("CC this error call is already processed!");
    		free((void *)(pvPrmvType));
    		return ;
    	}
    	else
    	{
    		g_ulCurErrId = pstRcvMsg->ulCallId;
    	}
    }

    LOG_RUNLOG_DEBUG("CC CC Recv Msg %d ",g_CCFsmState);
    switch(g_CCFsmState)											//根据有限状态机的当前状态进行迁移
    {
    case CCFSM_IDLE:											//空闲状态
        CCFsmIdleProcess(pvPrmvType);
        break;
    case CCFSM_DELAY:											//呼叫延时状态
        CCFsmDelayProcess(pvPrmvType);
        break;
    case CCFSM_CALL_WAIT:										//呼叫状态
        CCFsmCallProcess(pvPrmvType);
        break;
    case CCFSM_PIC_WAIT:										//个推状态
    	CCFsmPicProcess(pvPrmvType);
    	break;
    case CCFSM_PLAY_MUSIC:										//播放回铃音状态
        CCFsmPlayMusicProcess(pvPrmvType);
        break;
    case CCFSM_TALKING:											//通话状态
        CCFsmTalkingProcess(pvPrmvType);
        break;
    case CCFSM_VIDEO_MONIT:										//被叫视频监控状态
        CCFsmVideoMonProcess(pvPrmvType);
        break;

    default:
        break;

    }
    free((void *)(pvPrmvType));								//释放消息内容指针
}

//#define ROOM_CODE_LEN 4
//
//void CC2LANRoomNumFilter(char *pcRoomNum)
//{
//	char acHouseCode[64];
//	int len = 0;
//
//	if(!pcRoomNum)
//	{
//		return;
//	}
//
//	len = strlen(pcRoomNum) - 1;
//	memcpy(acHouseCode, pcRoomNum + 1, len);
//	acHouseCode[len] = 0x0;
//
//	if(pcRoomNum[0] == '0')
//	{
//		if(len == ROOM_CODE_LEN)
//		{
//			strcpy(g_iLanRoom,acHouseCode);
//		}
//		else if(len == ROOM_CODE_LEN - 1)
//		{
//			sprintf(g_iLanRoom,"0%s",acHouseCode);
//		}
//		else if(len > ROOM_CODE_LEN)
//		{
//			memcpy(g_iLanRoom, acHouseCode + len - ROOM_CODE_LEN, ROOM_CODE_LEN);
//			g_iLanRoom[ROOM_CODE_LEN] = 0x0;
//		}
//	}
//	else
//	{
//		strcpy(g_iLanRoom,acHouseCode);
//	}
//}

unsigned char *CCGetCurFsmRoom(void)
{
	return g_iCurRoom+1;
}

int CCGetCcStatus(void)
{
	return g_CCFsmState;
}


int CCGetCurSipCallIsValid(void)
{
	return g_iCurSipIsValid;
}

int CCGetCurRoomNumCanCallSip(void)
{
	return g_iCurRoomNumCanCallSip;
}

YK_TIMER_ST *CCCreateTimer(int iTimeType, unsigned int uiInterval)
{
	LOG_RUNLOG_DEBUG("CC Create Timer(iTimeType: %d, uiInterval: %d)", iTimeType, uiInterval);
    g_CcTimer.pstTimer = YKCreateTimer(CCTimerCallBack, (void*) iTimeType,
                                   	   uiInterval, YK_TT_NONPERIODIC, (unsigned long *)&g_CcTimer.ulMagicNum);
    if( g_CcTimer.pstTimer == NULL)
    {
    	LOG_RUNLOG_ERROR("CC Create timer failed!");
    }

    return g_CcTimer.pstTimer;
}

void CCRemoveTimer(void)
{
    if(g_CcTimer.pstTimer != NULL)
    {
        YKRemoveTimer(g_CcTimer.pstTimer, g_CcTimer.ulMagicNum);
        g_CcTimer.pstTimer = NULL;
    }
}

void CCSIPTerminate(unsigned long ulIncomingCallId)
{
	int iErrCode;

	CCSIP_TERMINATE_ST *pstSendMsg = (CCSIP_TERMINATE_ST *)malloc(sizeof(CCSIP_TERMINATE_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSIP_TERMINATE;
	pstSendMsg->ulCallId   = ulIncomingCallId;

	iErrCode = YKWriteQue( g_pstLPMsgQ , pstSendMsg,  0);
}

void CCSIPTerminateAll(void)
{
	int iErrCode;

	CCSIP_TERMINATE_ALL_ST *pstSendMsg = (CCSIP_TERMINATE_ALL_ST *)malloc(sizeof(CCSIP_TERMINATE_ALL_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSIP_TERMINATE_ALL;

	iErrCode = YKWriteQue( g_pstLPMsgQ , pstSendMsg,  0);
}

void CCSIPCallTimeout(void)
{
	int iErrCode;

	CCSIP_CALL_TIMEOUT_ST *pstSendMsg = (CCSIP_CALL_TIMEOUT_ST *)malloc(sizeof(CCSIP_CALL_TIMEOUT_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSIP_CALL_TIMEOUT;

	iErrCode = YKWriteQue( g_pstLPMsgQ , pstSendMsg,  0);
}

void CCSIPCall(unsigned char *pucPhoneNum,unsigned char ucMediaType,unsigned char ucPreView)
{
   	int iErrCode;

	CCSIP_CALL_ST *pstSendMsg = (CCSIP_CALL_ST *)malloc(sizeof(CCSIP_CALL_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType     = CCSIP_CALL;
	strcpy(pstSendMsg->aucPhoneNum,pucPhoneNum);
	pstSendMsg->aucMediaType   = ucMediaType;
	pstSendMsg->aucCallPreview   = ucPreView;
	pstSendMsg->aucHintMusicEn = HINT_MUSIC_DISABLE;

	iErrCode = YKWriteQue( g_pstLPMsgQ , pstSendMsg,  0);

	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_SIP_CALL_START, 0, pucPhoneNum);
	ATTestOK(AT_TEST_OBJECT_CC);
}

void CCSIPAnswer(unsigned long ulIncomingCallId)
{
	int iErrCode;

	CCSIP_ANSWER_ST *pstSendMsg = (CCSIP_ANSWER_ST *)malloc(sizeof(CCSIP_ANSWER_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSIP_ANSWER;
	pstSendMsg->ulCallId   = ulIncomingCallId;

	iErrCode = YKWriteQue( g_pstLPMsgQ , pstSendMsg,  0);
}

void CCABVideoMonitor(void)
{
	int iErrCode;

	CCAB_VEDIO_MONITOR_ST *pstSendMsg = (CCAB_VEDIO_MONITOR_ST *)malloc(sizeof(CCAB_VEDIO_MONITOR_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCAB_VEDIO_MONITOR;

	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}

void CCABOpenDoor(int iType, char *pcUserName, int iMsgSeq)
{
	int iErrCode;

	CCAB_OPENDOOR_ST *pstSendMsg = (CCAB_OPENDOOR_ST *)malloc(sizeof(CCAB_OPENDOOR_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCAB_OPENDOOR;
	pstSendMsg->iType = iType;			//预留

	if(pcUserName != NULL)
	{
		strcpy(pstSendMsg->aucUserName, pcUserName);
	}
	pstSendMsg->uiMsgSeq = iMsgSeq;

	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);

}

void CCABSendDTMF(int iDtmfType)
{
	int iErrCode;

	CCAB_CALLEE_SEND_DTMF_ST *pstSendMsg = (CCAB_CALLEE_SEND_DTMF_ST *)malloc(sizeof(CCAB_CALLEE_SEND_DTMF_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCAB_CALLEE_SEND_DTMF;
	pstSendMsg->iDtmfType =  iDtmfType;

	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}

void CCABCalleeErr(unsigned int uiErrorCause)
{
	int iErrCode;

	CCAB_CALLEE_ERR_ST *pstSendMsg = (CCAB_CALLEE_ERR_ST *)malloc(sizeof(CCAB_CALLEE_ERR_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType   = CCAB_CALLEE_ERR;
	pstSendMsg->uiErrorCause = uiErrorCause;

	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}

void CCABCalleePickUp(void)
{
	int iErrCode;

	CCAB_CALLEE_PICK_UP_ST *pstSendMsg = (CCAB_CALLEE_PICK_UP_ST *)malloc(sizeof(CCAB_CALLEE_PICK_UP_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCAB_CALLEE_PICK_UP;


	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}

void CCABCalleeHangOff(void)
{
	int iErrCode;

	CCAB_CALLEE_HANG_OFF_ST *pstSendMsg = (CCAB_CALLEE_HANG_OFF_ST *)malloc(sizeof(CCAB_CALLEE_HANG_OFF_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCAB_CALLEE_HANG_OFF;

	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}

void CCABCalleeMusicEnd(void)
{
	int iErrCode;

	CCAB_CALLEE_MUSIC_END_ST *pstSendMsg = (CCAB_CALLEE_MUSIC_END_ST *)malloc(sizeof(CCAB_CALLEE_MUSIC_END_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCAB_CALLEE_MUSIC_END;

	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}

void CCABVideoMonCancel(void)
{
//	int iErrCode;
//
//	CCAB_VEDIO_MONITOR_CANCEL_ST *pstSendMsg = (CCAB_VEDIO_MONITOR_CANCEL_ST *)malloc(sizeof(CCAB_VEDIO_MONITOR_CANCEL_ST));
//
//	if(pstSendMsg == NULL)
//	{
//		return;
//	}
//
//	pstSendMsg->uiPrmvType = CCAB_VEDIO_MONITOR_CANCEL;
//
//	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}

void CCABCalleeErrRoomInvalid()
{
	int iErrCode;

	CCAB_CALLEE_ERR_ROOM_INVALID_ST *pstSendMsg = (CCAB_CALLEE_ERR_ROOM_INVALID_ST *)malloc(sizeof(CCAB_CALLEE_ERR_ROOM_INVALID_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType   = CCAB_CALLEE_ERR_ROOM_INVALID;

	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}

void CCABCalleeErrRoomValid()
{
	int iErrCode;

	CCAB_CALLEE_ERR_ROOM_VALID_ST *pstSendMsg = (CCAB_CALLEE_ERR_ROOM_VALID_ST *)malloc(sizeof(CCAB_CALLEE_ERR_ROOM_VALID_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType   = CCAB_CALLEE_ERR_ROOM_VALID;

	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}

void CCSSCapturePicture(const char *SipNum)
{
	int iErrCode;
	SS_MSG_DATA_ST *pstSendMsg = (SS_MSG_DATA_ST *)malloc(sizeof(SS_MSG_DATA_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = SS_SNAPSHOT;
	pstSendMsg->SipNum = SipNum;

	iErrCode = YKWriteQue( g_pstSSMsgQ , pstSendMsg,  0);
}

void CCTimerCallBack(void *pvCtx)
{
	int iErrCode;

	CCCC_TIMEROUT_EVENT_ST *pSendMsg = (CCCC_TIMEROUT_EVENT_ST *)malloc(sizeof(CCCC_TIMEROUT_EVENT_ST));
	if(pSendMsg == NULL)
	{
		return;
	}

	pSendMsg->uiPrmvType = CCCC_TIMEROUT_EVENT;
	pSendMsg->uiTimerId  =(int)(pvCtx);

	iErrCode = YKWriteQue( g_pstCCMsgQ , pSendMsg,  0);
}

void CCFsmIdleProcess(void *pvPrmvType)
{
    LOG_RUNLOG_DEBUG("CC STATE:IDLE");
    LOG_RUNLOG_DEBUG("CC STATE:IDLE default recv %x",*(( unsigned int *)pvPrmvType));
    switch(*(( unsigned int *)pvPrmvType))
	{
    case SIPCC_AUDIO_INCOMING:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:SIPCC_AUDIO_INCOMING");
		SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
		strcpy(g_iCurRoom, CC_NO_ROOM);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);																	//不管号码是否合法均拒接
		LOG_RUNLOG_DEBUG("CC STATE:IDLE Terminated audio incoming(PhoneNum = %s, CallId=%ld)",
						pstRcvMsg->aucPhoneNum,pstRcvMsg->ulInComingCallId);

		if((TMCCIsValidPhoneNum(pstRcvMsg->aucPhoneNum, g_iCurRoom) == TRUE) && (TMCCIsKeyOpenDoorEnabled() == TRUE))	//一键开门使能&&号码合法
		{
			LOG_RUNLOG_DEBUG("CC STATE:IDLE InComing Phone Num Valid");
			CCABOpenDoor(OD_AUDIO_INCOMING, pstRcvMsg->aucUserName, 0);
			LOG_RUNLOG_DEBUG("CC STATE:IDLE CCABOpenDoor");
//#ifdef _YK_XT8126_
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
//#endif
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
		}
		else
		{
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
      	break;
    }
    case SIPCC_VIDEO_INCOMING:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:SIPCC_VIDEO_INCOMING");
    	strcpy(g_iCurRoom, CC_NO_ROOM);
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;
        LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
                    pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        if((TMCCIsValidPhoneNum(pstRcvMsg->aucPhoneNum, g_iCurRoom) == TRUE) && (TMCCIsVoipMonitorEnabled() == TRUE))		//视频监控使能 && 号码合法
        {
            LOG_RUNLOG_DEBUG("CC STATE:IDLE InComing Phone Num Valid");
#ifdef _YK_XT8126_
	#ifdef _YK_XT8126_BV_
            if(XTGetStatus() == 3 || XTGetStatus() == 4)
            {
            	CCSIPTerminate(pstRcvMsg->ulInComingCallId);
            	LOG_RUNLOG_DEBUG("CC STATE:IDLE xt status is in monitor, CCSIPTerminate(Id=%ld)", pstRcvMsg->ulInComingCallId);
            	LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
            	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_CLIENT_MONITOR_START, 1, "");
            }
            else
            {
    			CCSIPAnswer(pstRcvMsg->ulInComingCallId);
				LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPAnswer()");

				LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_CLIENT_MONITOR_START, 0, "");

				CCABVideoMonitor();
				g_ulCurCallId = pstRcvMsg->ulInComingCallId;
				LOG_RUNLOG_DEBUG("CC STATE:IDLE CCABVideoMonitor()");
				g_CCFsmState = CCFSM_VIDEO_MONIT;
				LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:VIDEO_MONIT");

				CCCreateTimer(TIMERID_MONITOR_WAIT, g_stCCTimeoutValue.iMonitorTImeout);
            }
	#else
			CCSIPAnswer(pstRcvMsg->ulInComingCallId);
			LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPAnswer()");

			LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_CLIENT_MONITOR_START, 0, "");

			CCABVideoMonitor();
			g_ulCurCallId = pstRcvMsg->ulInComingCallId;
			LOG_RUNLOG_DEBUG("CC STATE:IDLE CCABVideoMonitor()");
			g_CCFsmState = CCFSM_VIDEO_MONIT;
			LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:VIDEO_MONIT");
			CCCreateTimer(TIMERID_MONITOR_WAIT, g_stCCTimeoutValue.iMonitorTImeout);
	#endif
#else
			CCSIPAnswer(pstRcvMsg->ulInComingCallId);
			LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPAnswer()");
            CCABVideoMonitor();
            g_ulCurCallId = pstRcvMsg->ulInComingCallId;
            LOG_RUNLOG_DEBUG("CC STATE:IDLE CCABVideoMonitor()");
			g_CCFsmState = CCFSM_VIDEO_MONIT;
			LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:VIDEO_MONIT");
			CCCreateTimer(TIMERID_MONITOR_WAIT, g_stCCTimeoutValue.iMonitorTImeout);
#endif
        }
        else																			//号码不合法
        {
            LOG_RUNLOG_DEBUG("CC STATE:IDLE InComing Phone Num InValid");
            CCSIPTerminate(pstRcvMsg->ulInComingCallId);
            LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);
            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:CCFSM_IDLE -> STATE:CCFSM_IDLE");
        }
    	break;
    }
    //HB待改 添加室内机开门,监控
#ifdef AB_HAVE_INDOOR_MACHINE
    case LANCC_WATCH_INCOMING:
    {
    	LAN_WATCH_MSG_ST *pstMsg = (LAN_WATCH_MSG_ST *)pvPrmvType;
    	LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:LANCC_WATCH_INCOMING");
    	strcpy(g_acLanMonitorIP,pstMsg->cFromIP);
    	MsgCC2LANWatchRespond(g_acLanMonitorIP);
		LOG_RUNLOG_DEBUG("CC STATE:IDLE MsgCC2LANWatchRespond()");
		g_CCFsmState = CCFSM_VIDEO_MONIT;
		LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:VIDEO_MONIT");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_MONITOR_REQUEST, 0, "");
    	break;
    }
#endif
    //HB待改 添加呼叫室内机
    case ABCC_DOOR_CALL:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:ABCC_DOOR_CALL");
		ABCC_DOOR_CALL_ST *pstRcvMsg = (ABCC_DOOR_CALL_ST *)pvPrmvType;
		strcpy(g_iCurRoom, CC_NO_ROOM);
		g_iCurRoom[0] = pstRcvMsg->ucDevType;
		if(pstRcvMsg->ucDevType == 'E')
		{
			strcpy(g_iCurRoom + 1,pstRcvMsg->aucRoomNum+1);		//围墙机送7位房号
		}
		else
		{
			strcpy(g_iCurRoom + 1,pstRcvMsg->aucRoomNum);
		}
		LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
		g_iIsOpenDoorFlg = 0;

		g_iABHaveCall = 0;
		g_iCurSipIsValid = 0;
		g_iCurRoomNumCanCallSip = 0;
		//HB待改 室内机呼叫 默认当做有室内机
#ifdef AB_HAVE_INDOOR_MACHINE

		if(TMCCGetHouseInfoByHouseCode(g_iCurRoom) == TRUE)
		{
			g_iCurRoomNumCanCallSip = 1;
		}
		g_iHaveIndoorMachine = 1;
		g_iIndoorHangoff = 0;
		g_iSipHangoff = 0;
		TMCCFilterHouseCode(g_iCurRoom,g_iLanRoom);
		LOG_RUNLOG_DEBUG("CC STATE:IDLE MsgCC2LANCall %s",g_iLanRoom);
		if(MsgCC2LANCall(g_iLanRoom) != SUCCESS)
		{
			g_iHaveIndoorMachine = 0;
			g_iIndoorHangoff = 1;
		}
		LOG_RUNLOG_DEBUG("CC STATE:IDLE MsgCC2LANCall");
#endif

#ifdef TM_TAKE_PIC
		PICCallBegin();							//记录呼叫开始时间
#endif

		if(LPGetSipRegStatus() == REG_STATUS_ONLINE)
		{
			LOG_RUNLOG_DEBUG("CC STATE:LPGetSipRegStatus REG_STATUS_ONLINE");
			if(TMCCIsIMSCallEnabled() == TRUE)		//梯口机呼叫使能
			{
				LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:ABCC_DOOR_CALL DEVTYPE =%c, ROOMNUM =%s g_iCurRoom = %s",pstRcvMsg->ucDevType, pstRcvMsg->aucRoomNum, g_iCurRoom);
				if(TMCCGetHouseInfoByHouseCode(g_iCurRoom) == TRUE)
				{
	#ifdef _YK_IMX27_AV_
					LOG_RUNLOG_DEBUG("CC send host rsp (P)");
					SMSendHostRsq('P');
	#endif
					if(!g_iABHaveCall)
					{
						CCABCalleeErrRoomValid();
						LOG_RUNLOG_DEBUG("CC STATE:IDLE TMCCGetHouseInfoByHouseCode CCABCalleeErrRoomValid");
						g_iABHaveCall = 1;
					}
					LOG_RUNLOG_DEBUG("CC STATE:IDLE Room-Phome Num exist");

					TM_I6NUM_INFO_ST *pstPhoneNumInfoTmp = NULL;
					if(TMCCGetNumInfo(&pstPhoneNumInfoTmp) == 0)
					{
						memcpy(&g_stCurPhoneNumInfo, pstPhoneNumInfoTmp, sizeof(TM_I6NUM_INFO_ST));
						CCSSCapturePicture(g_stCurPhoneNumInfo.acPhoneNum);
						LOG_RUNLOG_DEBUG("CC STATE:IDLE CapturePicture ");

						g_stCCTimeoutValue.iCallDelay = TMCCGetSipCallDelay(NULL) * 1000;
						if(g_stCCTimeoutValue.iCallDelay == 0)
						{
							CCSIPTerminateAll();
							LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminateAll");
							CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
							LOG_RUNLOG_DEBUG("CC STATE:IDLE Start call.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
							LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
							g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
							CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
							LOG_RUNLOG_DEBUG("CC STATE:IDLE Create CallWait Timer");
							g_CCFsmState = CCFSM_CALL_WAIT;
							LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:CALL_WAIT");
						}
						else
						{
							CCCreateTimer(TIMERID_DELAY_CALL, g_stCCTimeoutValue.iCallDelay);
							LOG_RUNLOG_DEBUG("CC STATE:IDLE Create DelayCall Timer Interval=%dMs",g_stCCTimeoutValue.iMonitorTImeout);
							g_CCFsmState = CCFSM_DELAY;
							LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:DELAY");
						}
					}
					else
					{
						LOG_RUNLOG_DEBUG("CC STATE:IDLE Room-Phone Num not exist");
						g_iSipHangoff = 1;
	#ifdef AB_HAVE_INDOOR_MACHINE
						if(g_iIndoorHangoff == 1)
						{
	#endif
							strcpy(g_iCurRoom, CC_NO_ROOM);
							CCABCalleeErr(0);
							LOG_RUNLOG_DEBUG("CC STATE:IDLE CCABCalleeErr");
							g_CCFsmState = CCFSM_IDLE;
							LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
	#ifdef AB_HAVE_INDOOR_MACHINE
						}
						else
						{
							g_CCFsmState = CCFSM_CALL_WAIT;
							LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:CALL_WAIT");
						}
	#endif
					}
				}
				else
				{
					LOG_RUNLOG_DEBUG("CC STATE:IDLE Room Num not exist");
					g_iSipHangoff = 1;
	#ifdef AB_HAVE_INDOOR_MACHINE
					if(g_iIndoorHangoff == 1)
					{
	#endif
						strcpy(g_iCurRoom, CC_NO_ROOM);
						CCABCalleeErrRoomInvalid();
						g_CCFsmState = CCFSM_IDLE;
						LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
	#ifdef AB_HAVE_INDOOR_MACHINE
					}
					else
					{
						g_CCFsmState = CCFSM_CALL_WAIT;
						LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:CALL_WAIT");
					}
	#endif
				}
			}
			else
			{
				LOG_RUNLOG_DEBUG("CC STATE:Ims Call Disable!");
				g_iSipHangoff = 1;
#ifdef AB_HAVE_INDOOR_MACHINE
				if(g_iIndoorHangoff == 1)
				{
#endif
					strcpy(g_iCurRoom, CC_NO_ROOM);
					CCABCalleeErr(CALLEE_ERROR_CALL_DISABLE);
					LOG_RUNLOG_DEBUG("CC STATE:IDLE CCABCalleeErr");
					g_CCFsmState = CCFSM_IDLE;
					LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
#ifdef AB_HAVE_INDOOR_MACHINE
				}
				else
				{
					g_CCFsmState = CCFSM_CALL_WAIT;
					LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:CALL_WAIT");
				}
#endif
			}
		}
		else
		{
			g_iSipHangoff = 1;
			if(g_iIndoorHangoff == 1)
			{
				g_CCFsmState = CCFSM_IDLE;
				LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
			}
			else
			{
				g_CCFsmState = CCFSM_CALL_WAIT;
				LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:CALL_WAIT");
			}
			LOG_RUNLOG_DEBUG("CC STATE:LPGetSipRegStatus REG_STATUS_OFFLINE");
		}
    	break;
    }
    case ABCC_DOOR_HANG_OFF:					//非正常流程，暂留
    {
    	LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:ABCC_DOOR_HANG_OFF");
    	sleep(1);								//防止极短时间内触发呼叫和挂机造成SIP呼叫异常
        CCSIPTerminateAll();
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:IDLE RECEIVE HANG OFF AGAIN -> STATE:IDLE");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    	break;
    }


    default:
    	LOG_RUNLOG_DEBUG("CC STATE:IDLE default recv %x",*(( unsigned int *)pvPrmvType));
    	break;
	}
}

void CCFsmDelayProcess(void *pvPrmvType)
{
    LOG_RUNLOG_DEBUG("CC STATE:DELAY");
    LOG_RUNLOG_DEBUG("CC STATE:DELAY default recv %x",*(( unsigned int *)pvPrmvType));
    switch(*(( unsigned int *)pvPrmvType))
    {
    case SIPCC_AUDIO_INCOMING:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:SIPCC_AUDIO_INCOMING");
    	SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);																	//不管号码是否合法均拒接
		LOG_RUNLOG_DEBUG("CC STATE:DELAY Terminated audio incoming(PhoneNum = %s, CallId=%ld)",
						pstRcvMsg->aucPhoneNum,pstRcvMsg->ulInComingCallId);

		LOG_RUNLOG_DEBUG("CC STATE:DELAY Reject Opendoor.");
		LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);

		g_CCFsmState = CCFSM_DELAY;
		LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:DELAY");
    	break;
    }
    case SIPCC_VIDEO_INCOMING:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:SIPCC_VIDEO_INCOMING");
    	SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
					pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

		g_CCFsmState = CCFSM_DELAY;
		LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:DELAY");
    	break;
    }
#ifdef AB_HAVE_INDOOR_MACHINE
    //HB待改  室内机监控
    case LANCC_WATCH_INCOMING:
    {
    	LAN_WATCH_MSG_ST *pstMsg = (LAN_WATCH_MSG_ST *)pvPrmvType;
    	LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:LANCC_WATCH_INCOMING");
    	MsgCC2LANWatchLineuse(pstMsg->cFromIP);
		LOG_RUNLOG_DEBUG("CC STATE:DELAY MsgCC2LANWatchLineuse()");
		g_CCFsmState = CCFSM_DELAY;
		LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_DELAY");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_MONITOR_REQUEST, 1, "");
    	break;
    }
    //HB待改
    case LANCC_CALLEE_FAIL:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:LANCC_CALLEE_FAIL");
    	g_iIndoorHangoff = 1;
    	g_iHaveIndoorMachine = 0;
    	if(g_iSipHangoff == 1)
    	{
        	CCRemoveTimer();
    		LOG_RUNLOG_DEBUG("CC STATE:DELAY Destroy DelayCall Timer");
    		CCABCalleeErr(0);
    		LOG_RUNLOG_DEBUG("CC STATE:DELAY CCABCalleeErr");
    		g_CCFsmState = CCFSM_IDLE;
    		LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_IDLE");
    	}
    	else
    	{
    		g_CCFsmState = CCFSM_DELAY;
    		LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_DELAY");
    	}
    	break;
    }
    //HB待改
    case LANCC_CALLEE_RESPOND:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:LANCC_CALLEE_RESPOND");
		g_iIndoorHangoff = 0;
		g_iHaveIndoorMachine = 1;
//    	CCABCalleeErrRoomValid();
//    	LOG_RUNLOG_DEBUG("CC STATE:DELAY CCABCalleeErrRoomValid()");
		g_CCFsmState = CCFSM_DELAY;
		LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_DELAY");
    	break;
    }
    //HB待改
	case LANCC_CALLEE_PICK_UP:
	{
		LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:SIPCC_CALLEE_PICK_UP ");
		CCSIPTerminateAll();
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCSIPTerminateAll");
		CCRemoveTimer();
		LOG_RUNLOG_DEBUG("CC STATE:DELAY Destroy DelayCall Timer");
		CCABCalleePickUp();
		LOG_RUNLOG_DEBUG("CC STATE:DELAY CCABCalleePickUp");
		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_TALKING");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");

		break;
	}
	//HB待改
	case LANCC_CALLEE_LINEUSE:
	{
		LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:LANCC_CALLEE_LINEUSE");
		g_iIndoorHangoff = 1;
		g_iHaveIndoorMachine = 1;
		if(g_iSipHangoff == 1)
		{
			CCRemoveTimer();
			LOG_RUNLOG_DEBUG("CC STATE:DELAY Destroy DelayCall Timer");
			CCABCalleeErr(0);
			LOG_RUNLOG_DEBUG("CC STATE:DELAY CCABCalleeErr");
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_IDLE");
		}
		else
		{
			g_CCFsmState = CCFSM_DELAY;
			LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_DELAY");
		}
		break;
	}
	//HB待改
	case LANCC_CALLEE_OPENDOOR:
	{
		LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:LANCC_CALLEE_OPENDOOR");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_OPEN_DOOR, 0, "");
		CCABSendDTMF('#');
		LOG_RUNLOG_DEBUG("CC STATE:DELAY CCABSendDTMF,DTMF=%c",'#');
		g_CCFsmState = CCFSM_DELAY;
		LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_DELAY");
		break;
	}
	//HB待改
	case LANCC_CALLEE_HANG_OFF:
	case LANCC_CALLEE_RING_TIMEOUT:
	{
		LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:LANCC_CALLEE_RING_TIMEOUT");
		g_iIndoorHangoff = 1;
		g_iHaveIndoorMachine = 1;
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_HAND_UP, 0, "");
		if(g_iSipHangoff == 1)
		{
			CCRemoveTimer();
			LOG_RUNLOG_DEBUG("CC STATE:DELAY Destroy DelayCall Timer");
			CCABCalleeErr(0);
			LOG_RUNLOG_DEBUG("CC STATE:DELAY CCABCalleeErr");
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_IDLE");
		}
		else
		{
			g_CCFsmState = CCFSM_DELAY;
			LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CCFSM_DELAY");
		}
		break;
	}
#endif
    //HB待改  挂掉室内机
    case ABCC_DOOR_HANG_OFF:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:ABCC_DOOR_HANG_OFF");
#ifdef AB_HAVE_INDOOR_MACHINE
        g_iIndoorHangoff = 1;
        MsgCC2LANTerminate();
		LOG_RUNLOG_DEBUG("CC STATE:DELAY MsgCC2LANTerminate");
#endif
    	sleep(1);
        CCSIPTerminateAll();
        LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminateAll");
        CCRemoveTimer();
        LOG_RUNLOG_DEBUG("CC STATE:DELAY Destroy DelayCall Timer");
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:DELAY ->STATE:IDLE ");

        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    	break;
    }
    //HB待改
    case CCCC_TIMEROUT_EVENT:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:CCCC_TIMEROUT_EVENT");
    	CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;
    	if(pstRcvMsg->uiTimerId == TIMERID_DELAY_CALL)
    	{
    		CCSIPTerminateAll();
    		LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminateAll");
			CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
			LOG_RUNLOG_DEBUG("CC STATE:DELAY Start call.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
			g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
			CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
			LOG_RUNLOG_DEBUG("CC STATE:DELAY Create CallWait Timer");
			g_CCFsmState = CCFSM_CALL_WAIT;
			LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:CALL_WAIT");
    	}
    	else
    	{
    		LOG_RUNLOG_DEBUG("CC STATE: DELAY CC TIMER ID is not TIMERID_DELAY_CALL");
    	}
    	break;
    }

    default:
    	LOG_RUNLOG_DEBUG("CC STATE:DELAY default recv %x",*(( unsigned int *)pvPrmvType));
    	break;
    }
}


void CCFsmCallProcess(void *pvPrmvType)
{
	LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT");
	LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT default recv %x",*(( unsigned int *)pvPrmvType));
	switch(*(( unsigned int *)pvPrmvType))
	{
    case SIPCC_AUDIO_INCOMING:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_AUDIO_INCOMING");
    	SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);											//不管号码是否合法均拒接
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Terminated audio incoming(PhoneNum = %s, CallId=%ld)",
						pstRcvMsg->aucPhoneNum,pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Reject Opendoor.");
		LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		g_CCFsmState = CCFSM_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		break;
	}
	case SIPCC_MESSAGE_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_MESSAGE_INCOMING");
		SIPCC_MESSAGE_INCOMING_ST *pstRcvMsg = (SIPCC_MESSAGE_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_MESSAGE_INCOMING aucPhoneNum = %s, aucUserName = %s,uiMsgSeq = %d",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->aucUserName,	pstRcvMsg->uiMsgSeq);

		if (g_iIsOpenDoorFlg == 0)
		{
			g_iIsOpenDoorFlg = 1;
//			CCABCalleePickUp();
//			LOG_RUNLOG_DEBUG("CC SIPCC_MESSAGE_INCOMING PICK UP");
//			usleep(400*1000);
//			CCABCalleeMusicEnd();
//			usleep(400*1000);
		}

		CCABOpenDoor(OD_SIP_MASSAGE, pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq); 															//CCAB_OPENDOOR;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABOpenDoor");
//#ifdef _YK_XT8126_
		LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, 0);
//#endif
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");

        g_CCFsmState = CCFSM_CALL_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		break;
	}
	case SIPCC_VIDEO_INCOMING:
	{
		SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
						pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

		g_CCFsmState = CCFSM_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		break;
	}
#ifdef AB_HAVE_INDOOR_MACHINE
	//HB待改  室内机监控
	case LANCC_WATCH_INCOMING:
	{
		LAN_WATCH_MSG_ST *pstMsg = (LAN_WATCH_MSG_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:LANCC_WATCH_INCOMING");
		MsgCC2LANWatchLineuse(pstMsg->cFromIP);
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MsgCC2LANWatchLineuse()");
		g_CCFsmState = CCFSM_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_MONITOR_REQUEST, 1, "");
		break;
	}
	//HB待改
	case LANCC_CALLEE_FAIL:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:LANCC_CALLEE_FAIL");
		g_iIndoorHangoff = 1;
		g_iHaveIndoorMachine = 0;
		if(g_iSipHangoff == 1)
		{
			CCRemoveTimer();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Destroy CallWait Timer");
			if(g_iABHaveCall == 0)
			{
				if(g_iCurRoomNumCanCallSip == 0)
				{
					CCABCalleeErrRoomInvalid();
				}
				else
				{
					CCABCalleeErr(0);
				}
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeErrRoomInvalid");
			}
			else
			{
				CCABCalleeErr(0);
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeErr");
			}
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_IDLE");
		}
		else
		{
			g_CCFsmState = CCFSM_CALL_WAIT;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		}
		break;
	}
	//HB待改
	case LANCC_CALLEE_RESPOND:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:LANCC_CALLEE_RESPOND");
		g_iIndoorHangoff = 0;
		g_iHaveIndoorMachine = 1;
//		CCABCalleeErrRoomValid();
//		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeErrRoomValid()");
		if(g_iABHaveCall == 0)
		{
			CCABCalleeErrRoomValid();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT LANCC_CALLEE_RESPOND CCABCalleeErrRoomValid");
			g_iABHaveCall = 1;
		}
		g_CCFsmState = CCFSM_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		break;
	}
	//HB待改
	case LANCC_CALLEE_PICK_UP:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_CALLEE_PICK_UP ");
		CCSIPTerminateAll();
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCSIPTerminateAll");
		CCRemoveTimer();
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Destroy CallWait Timer");
		CCABCalleePickUp();
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleePickUp");
		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_TALKING");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");

		break;
	}
	//HB待改
	case LANCC_CALLEE_LINEUSE:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:LANCC_CALLEE_LINEUSE");
		g_iIndoorHangoff = 1;
		g_iHaveIndoorMachine = 1;
		if(g_iSipHangoff == 1)
		{
			CCRemoveTimer();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Destroy CallWait Timer");
			CCABCalleeErr(0);
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeErr");
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_IDLE");
		}
		else
		{
			g_CCFsmState = CCFSM_CALL_WAIT;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		}
		break;
	}
	//HB待改
	case LANCC_CALLEE_OPENDOOR:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:LANCC_CALLEE_OPENDOOR");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_OPEN_DOOR, 0, "");
		CCABSendDTMF('#');
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABSendDTMF,DTMF=%c",'#');
		g_CCFsmState = CCFSM_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_CALL_WAIT");
		break;
	}
	case LANCC_CALLEE_HANG_OFF:
	case LANCC_CALLEE_RING_TIMEOUT:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:LANCC_CALLEE_RING_TIMEOUT");
		g_iIndoorHangoff = 1;
		g_iHaveIndoorMachine = 1;
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_HAND_UP, 0, "");
		if(g_iSipHangoff == 1)
		{
			CCRemoveTimer();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Destroy DelayCall Timer");
			CCABCalleeErr(0);
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeErr");
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_IDLE");
		}
		else
		{
			g_CCFsmState = CCFSM_CALL_WAIT;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		}
		break;
	}
#endif
	//HB待改 SIP接听 挂断室内机
	case SIPCC_CALLEE_PICK_UP:
	{
#ifdef AB_HAVE_INDOOR_MACHINE
		g_iIndoorHangoff = 1;
		MsgCC2LANTerminate();
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MsgCC2LANTerminate");
#endif

		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_CALLEE_PICK_UP ");
		SIPCC_CALLEE_PICK_UP_ST *pstRcvMsg = (SIPCC_CALLEE_PICK_UP_ST *)pvPrmvType;
		g_ulCurCallId = pstRcvMsg->ulCallId;
		CCRemoveTimer();
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Remove CallWait Timer");

//		if (g_iIsOpenDoorFlg == 0)
//		{
			CCABCalleePickUp();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleePickUp");
//		}
		g_CCFsmState = CCFSM_PLAY_MUSIC;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:PLAY_MUSIC");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_USERS_PHONE_PICK_UP, 0, "");
		break;
	}
	//HB待改 室内机,SIP都挂断才切换状态
	case SIPCC_STOP_CALLING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_STOP_CALLING");
		g_iIsOpenDoorFlg = 0;
		g_iSipHangoff = 1;
#ifdef AB_HAVE_INDOOR_MACHINE
		if(g_iIndoorHangoff == 1)
		{
#endif
			CCABCalleeHangOff();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeHangOff");
			CCRemoveTimer();
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:IDLE");
#ifdef AB_HAVE_INDOOR_MACHINE
		}
		else
		{
			CCRemoveTimer();
			g_CCFsmState = CCFSM_CALL_WAIT;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		}
#endif
		break;
	}
	case SIPCC_CALLEE_ERR:
	{
		if (g_iIsOpenDoorFlg == 1)
		{
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_CALLEE_ERR");
			CCABCalleeHangOff();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeHangOff");
			CCRemoveTimer();
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:IDLE");
			break;
		}

		SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_CALLEE_ERR %x", pstRcvMsg->uiErrorCause);
		char acErrCause[20] = { 0 };
		sprintf(acErrCause, "%d", pstRcvMsg->uiErrorCause);
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_SIP_CALL_FAIL, 0, acErrCause);
		TM_I6NUM_INFO_ST *pstPhoneNumInfoTmp = NULL;
		if(TMCCGetNumInfo(&pstPhoneNumInfoTmp) == 0)									//查询房间号是否有其他有效号码
		{
			if(pstPhoneNumInfoTmp->ucCallType == MEDIA_TYPE_AUDIO)						//是否语音电话
			{
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MEDIA_TYPE_AUDIO.");
				if(pstPhoneNumInfoTmp->iSerNum == g_stCurPhoneNumInfo.iSerNum)			//是否绑定
				{
					if((pstRcvMsg->uiErrorCause == 603) || (pstRcvMsg->uiErrorCause == 1021) || (pstRcvMsg->uiErrorCause == 486))		//是否拒接
					{
						if(TMCCGetNumInfo(&pstPhoneNumInfoTmp) == 0)	 		 		  //查询房间号是否有其他有效号码
						{
							memcpy(&g_stCurPhoneNumInfo, pstPhoneNumInfoTmp, sizeof(TM_I6NUM_INFO_ST));
							if(g_stCurPhoneNumInfo.ucCallType == MEDIA_TYPE_AUDIO_VIDEO)  //是否视频呼叫
							{
								CCSSCapturePicture(g_stCurPhoneNumInfo.acPhoneNum);
								LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CapturePicture ");
							}
							CCSIPTerminateAll();
							LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminateAll");
							CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
							LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
							LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
							CCRemoveTimer();
							g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
							CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
							LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Create CallWait Timer");
							g_CCFsmState = CCFSM_CALL_WAIT;
							LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
						}
						else
						{
							g_iSipHangoff = 1;
							LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT no others exist");
							CCSIPTerminateAll();
							LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminateAll");
							CCRemoveTimer();
							LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Remove CallWait Timer");
#ifdef AB_HAVE_INDOOR_MACHINE
							if(g_iIndoorHangoff == 1)
							{
#endif
								CCABCalleeErr(pstRcvMsg->uiErrorCause);
								LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeErr(%d)",pstRcvMsg->uiErrorCause);
								g_CCFsmState = CCFSM_IDLE;
								LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:IDLE");
#ifdef AB_HAVE_INDOOR_MACHINE
							}
							else
							{
								g_CCFsmState = CCFSM_CALL_WAIT;
								LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_CALL_WAIT");
							}
#endif
						}
					}
					else//如果无个推则直接呼叫电话号码
					{
#ifdef TM_TAKE_PIC
						PICTakePicture(g_stCurPhoneNumInfo.acPhoneNum);					 //开始个推
						LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:PIC_WAIT");
						CCRemoveTimer();
						CCCreateTimer(TIMERID_PIC_CONNECT, g_stCCTimeoutValue.iPicConnectTimeout);
						LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Create Pic Connect Timer");
						g_CCFsmState = CCFSM_PIC_WAIT;
						LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_PIC_WAIT");
						memcpy(&g_stCurPhoneNumInfo, pstPhoneNumInfoTmp, sizeof(TM_I6NUM_INFO_ST));
#else
						memcpy(&g_stCurPhoneNumInfo, pstPhoneNumInfoTmp, sizeof(TM_I6NUM_INFO_ST));
						CCSIPTerminateAll();
						LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminateAll");
						CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
						LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
						LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
						CCRemoveTimer();
						g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
						CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
						LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Create CallWait Timer");
						g_CCFsmState = CCFSM_CALL_WAIT;
						LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
#endif
					}
				}
				else
				{
					memcpy(&g_stCurPhoneNumInfo, pstPhoneNumInfoTmp, sizeof(TM_I6NUM_INFO_ST));
					CCSIPTerminateAll();
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminateAll");
					CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
					LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
					CCRemoveTimer();
					g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
					CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Create CallWait Timer");
					g_CCFsmState = CCFSM_CALL_WAIT;
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
				}
			}
			else
			{
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MEDIA_TYPE_AUDIO_VEDIO.");
				memcpy(&g_stCurPhoneNumInfo, pstPhoneNumInfoTmp, sizeof(TM_I6NUM_INFO_ST));
				CCSIPTerminateAll();
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminateAll");
				CCSSCapturePicture(g_stCurPhoneNumInfo.acPhoneNum);				//拍照
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CapturePicture ");
				CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
				CCRemoveTimer();
				g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
				CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Create CallWait Timer");
				g_CCFsmState = CCFSM_CALL_WAIT;
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
			}
		}
		else
		{
			g_iSipHangoff = 1;
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT no other nums exist");
			CCSIPTerminateAll();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminateAll");
			CCRemoveTimer();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Remove CallWait Timer");
#ifdef AB_HAVE_INDOOR_MACHINE
			if(g_iIndoorHangoff == 1)
			{
#endif
				CCABCalleeErr(pstRcvMsg->uiErrorCause);
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeErr(%d)",pstRcvMsg->uiErrorCause);
				g_CCFsmState = CCFSM_IDLE;
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:IDLE");
#ifdef AB_HAVE_INDOOR_MACHINE
			}
			else
			{
				g_CCFsmState = CCFSM_CALL_WAIT;
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_CALL_WAIT");
			}
#endif
		}

		break;
	}
	//HB待改
	case CCCC_TIMEROUT_EVENT:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ");
		CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;
		if(pstRcvMsg->uiTimerId == TIMERID_CALL_WAIT)						//定时器是否为呼叫超时
		{
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_SIP_CALL_FAIL, 0, "");
			TM_I6NUM_INFO_ST *pstPhoneNumInfoTmp = NULL;
			if(TMCCGetNumInfo(&pstPhoneNumInfoTmp) == 0)					//查询房间号是否有其他有效号码
			{
				memcpy(&g_stCurPhoneNumInfo, pstPhoneNumInfoTmp, sizeof(TM_I6NUM_INFO_ST));
				CCSIPCallTimeout();
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminateAll");
				if(g_stCurPhoneNumInfo.ucCallType == MEDIA_TYPE_AUDIO_VIDEO)  //是否视频呼叫
				{
					CCSSCapturePicture(g_stCurPhoneNumInfo.acPhoneNum);
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CapturePicture ");
				}
				CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
				CCRemoveTimer();
				g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
				CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Create CallWait Timer");
				g_CCFsmState = CCFSM_CALL_WAIT;
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
			}
			else
			{
				g_iSipHangoff = 1;
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT no other nums exist");
				CCSIPTerminateAll();
				LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminateAll");
#ifdef AB_HAVE_INDOOR_MACHINE
				if(g_iIndoorHangoff == 1)
				{
#endif
					CCABCalleeErr(ERR_TIMEOUT_CALL_WAIT);
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABCalleeErr(ERR_TIMEOUT_CALL_WAIT)");
					g_CCFsmState = CCFSM_IDLE;
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:IDLE");
#ifdef AB_HAVE_INDOOR_MACHINE
				}
				else
				{
					g_CCFsmState = CCFSM_CALL_WAIT;
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_CALL_WAIT");
				}
#endif
			}
		}

		break;
	}
	//HB待改 添加挂断室内机
	case ABCC_DOOR_HANG_OFF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:ABCC_DOOR_HANG_OFF");
#ifdef AB_HAVE_INDOOR_MACHINE
        g_iIndoorHangoff = 1;
        MsgCC2LANTerminate();
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MsgCC2LANTerminate");
#endif
    	sleep(1);
    	LOG_RUNLOG_DEBUG("CC ***********************");
        CCSIPTerminateAll();
        LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCSIPTerminateAll");
        CCRemoveTimer();
        LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Remove CallWait Timer");
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:IDLE");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
		break;
	}
	case SIPCC_CALLEE_SEND_DTMF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:SIPCC_CALLEE_SEND_DTMF");
		SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;
#ifdef AB_HAVE_INDOOR_MACHINE
        g_iIndoorHangoff = 1;
        MsgCC2LANTerminate();
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MsgCC2LANTerminate");
#endif
		if (g_iIsOpenDoorFlg == 0)
		{
			g_iIsOpenDoorFlg = 1;
//			CCABCalleePickUp();
//			LOG_RUNLOG_DEBUG("CC SIPCC_MESSAGE_INCOMING PICK UP");
//			usleep(400*1000);
//			CCABCalleeMusicEnd();
//			usleep(400*1000);
		}

	    if(pstRcvMsg->iDtmfType == '#')
	    {
	    	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");
	    }
		CCABSendDTMF(pstRcvMsg->iDtmfType);
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT CCABSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);
		g_CCFsmState = CCFSM_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CALL_WAIT");
		break;
	}

	default:
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT default recv %x",*(( unsigned int *)pvPrmvType));
		break;
	}
}

void CCFsmPicProcess(void *pvPrmvType)
{
	LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT");
	LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT default recv %x",*(( unsigned int *)pvPrmvType));
	switch(*(( unsigned int *)pvPrmvType))
	{
    case SIPCC_AUDIO_INCOMING:
    {
    	LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:SIPCC_AUDIO_INCOMING");
    	SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);											//不管号码是否合法均拒接
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT Terminated audio incoming(PhoneNum = %s, CallId=%ld)",
						pstRcvMsg->aucPhoneNum,pstRcvMsg->ulInComingCallId);

		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT Reject Opendoor.");
		LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:PIC_WAIT");
		break;
	}
	case SIPCC_VIDEO_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:SIPCC_VIDEO_INCOMING");
		SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
						pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:CCFSM_CALL_WAIT");
		break;
	}
	//HB待改 挂断室内机
	case TMCC_PIC_RESULT_PICKUP:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:TMCC_PIC_RESULT_PICKUP");
		CCSIPTerminateAll();
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCSIPTerminateAll");
		CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
		CCRemoveTimer();
		g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
		CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT Create CallWait Timer");
		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:CALL_WAIT");
		break;
	}
#ifdef AB_HAVE_INDOOR_MACHINE
	//HB待改  室内机监控
	case LANCC_WATCH_INCOMING:
	{
		LAN_WATCH_MSG_ST *pstMsg = (LAN_WATCH_MSG_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:LANCC_WATCH_INCOMING");
		MsgCC2LANWatchLineuse(pstMsg->cFromIP);
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT MsgCC2LANWatchLineuse()");
		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:PIC_WAIT");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_MONITOR_REQUEST, 1, "");

		break;
	}
	//HB待改
	case LANCC_CALLEE_FAIL:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:LANCC_CALLEE_FAIL");
		g_iIndoorHangoff = 1;
		g_iHaveIndoorMachine = 0;
		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:PIC_WAIT");
		break;
	}
	//HB待改
	case LANCC_CALLEE_RESPOND:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:LANCC_CALLEE_RESPOND");
		g_iIndoorHangoff = 0;
		g_iHaveIndoorMachine = 1;
//		CCABCalleeErrRoomValid();
//		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCABCalleeErrRoomValid()");
		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:PIC_WAIT");
		break;
	}
	//HB待改
	case LANCC_CALLEE_PICK_UP:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:SIPCC_CALLEE_PICK_UP ");
		CCSIPTerminateAll();
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCSIPTerminateAll");
		CCRemoveTimer();
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT Destroy CallWait Timer");
		CCABCalleePickUp();
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCABCalleePickUp");
		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:CCFSM_TALKING");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");

		break;
	}
	//HB待改
	case LANCC_CALLEE_LINEUSE:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:LANCC_CALLEE_LINEUSE");
		g_iIndoorHangoff = 1;
		g_iHaveIndoorMachine = 1;
		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:PIC_WAIT");
		break;
	}
	//HB待改
	case LANCC_CALLEE_OPENDOOR:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:LANCC_CALLEE_OPENDOOR");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_OPEN_DOOR, 0, "");
		CCABSendDTMF('#');
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCABSendDTMF,DTMF=%c",'#');
		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:CCFSM_PIC_WAIT");
		break;
	}
	case LANCC_CALLEE_HANG_OFF:
	case LANCC_CALLEE_RING_TIMEOUT:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:LANCC_CALLEE_RING_TIMEOUT");
		g_iIndoorHangoff = 1;
		g_iHaveIndoorMachine = 1;
		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:PIC_WAIT");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_HAND_UP, 1, "");

		break;
	}
#endif
	case TMCC_PIC_RESULT_OPEN_DOOR:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:TMCC_PIC_RESULT_OPEN_DOOR");
#ifdef AB_HAVE_INDOOR_MACHINE
		g_iIndoorHangoff = 1;
		MsgCC2LANTerminate();
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MsgCC2LANTerminate");
#endif
        CCABCalleePickUp();
        LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCAB_PICK_UP");
        usleep(400*1000);
        CCABCalleeMusicEnd();
        usleep(400*1000);
        CCABOpenDoor(OD_SOCKET, NULL, 0);
        LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCAB_OPENDOOR");
        CCRemoveTimer();
        CCCreateTimer(TIMERID_OPENDOOR, g_stCCTimeoutValue.iOpenDoorTimeout);
        LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT Create OpenDoor Timer");
        g_iIsOpenDoorFlg = 1;
        g_CCFsmState = CCFSM_PIC_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:PIC_WAIT");
		break;
	}
	case TMCC_PIC_CLINET_RECV_PIC_OK:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:TMCC_PIC_CLINET_RECV_PIC_OK");
		CCRemoveTimer();
		CCCreateTimer(TIMERID_PIC_RING, g_stCCTimeoutValue.iPicRingTime);
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT Create PicRing Timer");
		g_CCFsmState = CCFSM_PIC_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:PIC_WAIT");
		break;
	}
	case TMCC_PIC_RESULT_HANG_OFF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:TMCC_PIC_RESULT_HANG_OFF");
		if(g_iIsOpenDoorFlg == 1)
		{
			LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT already opendoor, end call.");
#ifdef AB_HAVE_INDOOR_MACHINE
			g_iIndoorHangoff = 1;
			MsgCC2LANTerminate();
			LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MsgCC2LANTerminate");
#endif
			g_iIsOpenDoorFlg = 0;
			CCRemoveTimer();
			LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT Remove Timer.");
			CCSIPTerminateAll();
			CCABCalleeHangOff();
			LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT CCABCalleeHangOff.");
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE: CCFSM_IDLE");
		}
		else
		{
			TM_I6NUM_INFO_ST *pstPhoneNumInfoTmp = NULL;
			if(TMCCGetNumInfo(&pstPhoneNumInfoTmp) == 0)					//查询房间号是否有其他有效号码
			{
				memcpy(&g_stCurPhoneNumInfo, pstPhoneNumInfoTmp, sizeof(TM_I6NUM_INFO_ST));
				if(g_stCurPhoneNumInfo.ucCallType == MEDIA_TYPE_AUDIO_VIDEO)
				{
					CCSSCapturePicture(g_stCurPhoneNumInfo.acPhoneNum);
					LOG_RUNLOG_DEBUG("CC STATE:IDLE CapturePicture ");
				}
				CCSIPTerminateAll();
				LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCSIPTerminateAll");
				CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
				LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
				CCRemoveTimer();
				g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
				CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
				LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT Create CallWait Timer");
				g_CCFsmState = CCFSM_CALL_WAIT;
				LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:CALL_WAIT");
			}
			else
			{
				g_iSipHangoff = 1;
				LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT No Next Num.");
				CCRemoveTimer();
				LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT Remove Timer.");
				CCSIPTerminateAll();
#ifdef AB_HAVE_INDOOR_MACHINE
				if(g_iIndoorHangoff == 1)
				{
#endif
					CCABCalleeHangOff();
					LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT CCABCalleeHangOff.");
					g_CCFsmState = CCFSM_IDLE;
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:IDLE");
#ifdef AB_HAVE_INDOOR_MACHINE
				}
				else
				{
					g_CCFsmState = CCFSM_CALL_WAIT;
					LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT -> STATE:CCFSM_CALL_WAIT");
				}
#endif
			}
		}
		break;
	}
	case TMCC_PIC_RESULT_ERROR:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:TMCC_PIC_RESULT_ERROR");
		CCSIPTerminateAll();
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCSIPTerminateAll");
		CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
		CCRemoveTimer();
		g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
		CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT Create CallWait Timer");
		g_CCFsmState = CCFSM_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:CALL_WAIT");
		break;
	}
	case TMCC_PIC_SOCKET_ERROR:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:TMCC_PIC_SOCKET_ERROR");
		CCSIPTerminateAll();
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCSIPTerminateAll");
		CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
		CCRemoveTimer();
		g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
		CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT Create CallWait Timer");
		g_CCFsmState = CCFSM_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:CALL_WAIT");
		break;
	}
	//HB待改 添加挂断室内机
	case CCCC_TIMEROUT_EVENT:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:CCCC_TIMEROUT_EVENT");
		if(g_iIsOpenDoorFlg == 1)
		{
			LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT already opendoor, end call.");
#ifdef AB_HAVE_INDOOR_MACHINE
        g_iIndoorHangoff = 1;
        MsgCC2LANTerminate();
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MsgCC2LANTerminate");
#endif
			g_iIsOpenDoorFlg = 0;
			LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT Remove Timer.");
			CCSIPCallTimeout();
			CCABCalleeHangOff();
			LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT CCABCalleeHangOff.");
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE: CCFSM_IDLE");
		}
		else
		{
			CCSIPCallTimeout();
			LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT CCSIPTerminateAll");
			CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
			LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT start call next num.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
			CCRemoveTimer();
			g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
			CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
			LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT Create CallWait Timer");
			g_CCFsmState = CCFSM_CALL_WAIT;
			LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT -> STATE:CALL_WAIT");
		}
		break;
	}
	//HB待改 添加挂断室内机
	case ABCC_DOOR_HANG_OFF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT RCV:ABCC_DOOR_HANG_OFF");
#ifdef AB_HAVE_INDOOR_MACHINE
        g_iIndoorHangoff = 1;
        MsgCC2LANTerminate();
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MsgCC2LANTerminate");
#endif
		PICTimeOut();
		LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT Cancel Pic Call.");
		CCRemoveTimer();
		LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT Remove Timer.");
		CCSIPTerminateAll();
		CCABCalleeHangOff();
		LOG_RUNLOG_DEBUG("CC STATE: PIC_WAIT CCABCalleeHangOff.");
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE: CCFSM_IDLE");
		break;
	}

	default:
		LOG_RUNLOG_DEBUG("CC STATE:PIC_WAIT default recv %x",*(( unsigned int *)pvPrmvType));
		break;
	}
}

void CCFsmPlayMusicProcess(void *pvPrmvType)
{
	LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC");
	LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC default recv %x",*(( unsigned int *)pvPrmvType));
	switch(*(( unsigned int *)pvPrmvType))
	{
	case SIPCC_AUDIO_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_AUDIO_INCOMING");
		SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);											//不管号码是否合法均拒接
		LOG_RUNLOG_DEBUG("CC PLAY_MUSIC Terminated audio incoming(PhoneNum = %s, CallId=%ld)",
						pstRcvMsg->aucPhoneNum,pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC Reject Opendoor.");
		LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		g_CCFsmState = CCFSM_PLAY_MUSIC;
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:PLAY_MUSIC");
		break;
	}
	case SIPCC_VIDEO_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_VIDEO_INCOMING");
		SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
						pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

		g_CCFsmState = CCFSM_PLAY_MUSIC;
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:PLAY_MUSIC");
		break;
	}
#ifdef AB_HAVE_INDOOR_MACHINE
	//HB待改  室内机监控
	case LANCC_WATCH_INCOMING:
	{
		LAN_WATCH_MSG_ST *pstMsg = (LAN_WATCH_MSG_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT RCV:LANCC_WATCH_INCOMING");
		MsgCC2LANWatchLineuse(pstMsg->cFromIP);
		LOG_RUNLOG_DEBUG("CC STATE:CALL_WAIT MsgCC2LANWatchLineuse()");
		g_CCFsmState = CCFSM_PLAY_MUSIC;
		LOG_RUNLOG_DEBUG("CC STATE:CCFSM_PLAY_MUSIC -> STATE:CCFSM_PLAY_MUSIC");
		break;
	}
#endif
	case SIPCC_CALLEE_SEND_DTMF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_CALLEE_SEND_DTMF");
		SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

		CCABSendDTMF(pstRcvMsg->iDtmfType);
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCABSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);

		g_CCFsmState = CCFSM_PLAY_MUSIC;
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:PLAY_MUSIC");
		break;
	}
	//HB待改
	case ABCC_DOOR_HANG_OFF:
	{
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:ABCC_DOOR_HANG_OFF");
    	sleep(1);
        CCSIPTerminateAll();
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSIPTerminateAll");
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:IDLE");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
		break;
	}
	//HB待改
	case SIPCC_CALLEE_HANG_OFF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_CALLEE_HANG_OFF");
		SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;
		if(g_ulCurErrId == pstRcvMsg->ulCallId)
		{
			CCSIPTerminate(pstRcvMsg->ulCallId);
			LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSIPTerminateAll");
			CCABCalleeHangOff();
			LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCABCalleeHangOff");
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:IDLE");
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_USERS_PHONE_HAND_UP, 0, "");
		}
		break;
	}
	//HB待改
	case SIPCC_CALLEE_ERR:
	{
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_CALLEE_ERR ");
        SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;

        char errorCause[20] = {0x0};
        sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);
        CCSIPTerminateAll();
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSIPTerminateAll");
        CCABCalleeErr(pstRcvMsg->uiErrorCause);
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCABCalleeErr(%d)",pstRcvMsg->uiErrorCause);
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:IDLE");
		break;
	}
	//HB待改
	case SIPCC_CALLEE_MUSIC_END:
	{
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_CALLEE_MUSIC_END ");

        if (g_iIsOpenDoorFlg == 0)
        {
			CCABCalleeMusicEnd();
			LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCABCalleeMusicEnd");
        }
		CCRemoveTimer();
		g_stCCTimeoutValue.iTalkingTimeout = TMCCGetIMSCallTimeOut() * 1000;			//获取通话超时时间
		CCCreateTimer(TIMERID_TALK_WAIT, g_stCCTimeoutValue.iTalkingTimeout);
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC Create TalkWait Timer");
        g_CCFsmState = CCFSM_TALKING;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:TALKING");
		break;
	}

	default:
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC default recv %x",*(( unsigned int *)pvPrmvType));
		break;
	}
}

void CCFsmTalkingProcess(void *pvPrmvType)
{
	LOG_RUNLOG_DEBUG("CC STATE:TALKING");
	LOG_RUNLOG_DEBUG("CC STATE:TALKING default recv %x",*(( unsigned int *)pvPrmvType));
	switch(*(( unsigned int *)pvPrmvType))
	{
	case SIPCC_AUDIO_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_AUDIO_INCOMING");
		SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);											//不管号码是否合法均拒接
		LOG_RUNLOG_DEBUG("CC TALKING Terminated audio incoming(PhoneNum = %s, CallId=%ld)",
						pstRcvMsg->aucPhoneNum,pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:TALKING Reject Opendoor.");
		LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");
		break;
	}
	case SIPCC_VIDEO_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_VIDEO_INCOMING");
		SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
						pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");
		break;
	}
	case SIPCC_MESSAGE_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_MESSAGE_INCOMING");
		SIPCC_MESSAGE_INCOMING_ST *pstRcvMsg = (SIPCC_MESSAGE_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_MESSAGE_INCOMING aucPhoneNum = %s, aucUserName = %s,uiMsgSeq = %ld",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->aucUserName,	pstRcvMsg->uiMsgSeq);

		CCABOpenDoor(OD_SIP_MASSAGE, pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq); 															//CCAB_OPENDOOR;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABOpenDoor");
#ifdef _YK_XT8126_
		LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, 0);
#endif
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");

		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");
		break;
	}
	case SIPCC_CALLEE_SEND_DTMF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_CALLEE_SEND_DTMF");
		SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

	    if(pstRcvMsg->iDtmfType == '#')
	    {
	    	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");
	    }
		CCABSendDTMF(pstRcvMsg->iDtmfType);
		LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);
		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");
		break;
	}
#ifdef AB_HAVE_INDOOR_MACHINE
    //HB待改  室内机监控
    case LANCC_WATCH_INCOMING:
    {
    	LAN_WATCH_MSG_ST *pstMsg = (LAN_WATCH_MSG_ST *)pvPrmvType;
    	LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:LANCC_WATCH_INCOMING");
    	MsgCC2LANWatchLineuse(pstMsg->cFromIP);
		LOG_RUNLOG_DEBUG("CC STATE:TALKING MsgCC2LANWatchLineuse()");
		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");

        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_MONITOR_REQUEST, 1, "");
    	break;
    }
    //HB待改
	case LANCC_CALLEE_HANG_OFF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:LANCC_CALLEE_HANG_OFF ");
		CCRemoveTimer();
		LOG_RUNLOG_DEBUG("CC STATE:TALKING Destroy DelayCall Timer");
		CCABCalleeHangOff();
		LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABCalleeHangOff");
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:CCFSM_IDLE");

        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_HAND_UP, 0, "");
		break;
	}
	//HB待改
	case LANCC_CALLEE_OPENDOOR:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:LANCC_CALLEE_OPENDOOR");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_OPEN_DOOR, 0, "");
		CCABSendDTMF('#');
		LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABSendDTMF,DTMF=%c",'#');
		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");
		break;
	}
	case LANCC_CALLEE_TALK_TIMEOUT:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:LANCC_CALLEE_RING_TIMEOUT");
        CCABCalleeHangOff();
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABCalleeHangOff");
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:CCFSM_IDLE");

        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_HAND_UP, 0, "");
		break;
	}
#endif
	//HB待改
	case ABCC_DOOR_HANG_OFF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:ABCC_DOOR_HANG_OFF");
#ifdef AB_HAVE_INDOOR_MACHINE
        g_iIndoorHangoff = 1;
        MsgCC2LANTerminate();
		LOG_RUNLOG_DEBUG("CC STATE:TALKING MsgCC2LANTerminate");
#endif
    	sleep(1);
        CCSIPTerminateAll();
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminateAll");
        CCRemoveTimer();
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:IDLE");

        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
        break;
	}
	case SIPCC_CALLEE_HANG_OFF:
	{
        LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_CALLEE_HANG_OFF");
        SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;

        if(g_ulCurCallId == pstRcvMsg->ulCallId)
        {
            CCSIPTerminate(pstRcvMsg->ulCallId);
            LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminateAll");
            CCABCalleeHangOff();
            LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABCalleeHangOff");
            CCRemoveTimer();
            LOG_RUNLOG_DEBUG("CC STATE:TALKING remove TalkWait Timer.");
            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:IDLE");
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_USERS_PHONE_HAND_UP, 0, "");
        }

        break;
	}
	//HB待改
	case CCCC_TIMEROUT_EVENT:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:CCCC_TIMEROUT_EVENT ");
		CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

		if(pstRcvMsg->uiTimerId == TIMERID_TALK_WAIT)
		{
			LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
			CCSIPTerminateAll();
			LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminateAll");
			CCABCalleeHangOff();
			LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABCalleeHangOff");
			CCRemoveTimer();
			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:IDLE");
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_USERS_PHONE_COMM_TIMEOUT, 0, "");
		}
		break;
	}
	case SIPCC_CALLEE_ERR:
    {
        LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_CALLEE_ERR");
        SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;

        char errorCause[20] = {0x0};
        sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);
        CCSIPTerminateAll();
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminateAll");
        CCRemoveTimer();
        LOG_RUNLOG_DEBUG("CC STATE:TALKING remove TalkWait Timer.");
        CCABCalleeErr(pstRcvMsg->uiErrorCause);
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABCalleeErr(%d)",pstRcvMsg->uiErrorCause);
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:IDLE");
        break;
    }
	default:
		LOG_RUNLOG_DEBUG("CC STATE:TALKING default recv %x",*(( unsigned int *)pvPrmvType));
		break;
	}
}


void CCFsmVideoMonProcess(void *pvPrmvType)
{
	 LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT");

	LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT default recv %x",*(( unsigned int *)pvPrmvType));
	switch (*((unsigned int *) pvPrmvType))
	{
	case SIPCC_AUDIO_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_AUDIO_INCOMING");
		SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *) pvPrmvType;
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC VIDEO_MONIT Terminated audio incoming(PhoneNum = %s, CallId=%ld)",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Reject Opendoor.");
		LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		g_CCFsmState = CCFSM_VIDEO_MONIT;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
		break;
	}
	case SIPCC_MESSAGE_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_MESSAGE_INCOMING");
		SIPCC_MESSAGE_INCOMING_ST *pstRcvMsg = (SIPCC_MESSAGE_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_MESSAGE_INCOMING aucPhoneNum = %s, aucUserName = %s,uiMsgSeq = %d",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->aucUserName,	pstRcvMsg->uiMsgSeq);

		CCABOpenDoor(OD_SIP_MASSAGE, pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq); 															//CCAB_OPENDOOR;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCABOpenDoor");
//#ifdef _YK_XT8126_
		LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, 0);
//#endif
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");

		g_CCFsmState = CCFSM_VIDEO_MONIT;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
		break;
	}
	case SIPCC_VIDEO_INCOMING:
	{
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_VIDEO_INCOMING");
		SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
						pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

		g_CCFsmState = CCFSM_VIDEO_MONIT;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
		break;
	}
	case SIPCC_CALLEE_SEND_DTMF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_CALLEE_SEND_DTMF");
		SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

	    if(pstRcvMsg->iDtmfType == '#')
	    {
	    	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");
	    }
		CCABSendDTMF(pstRcvMsg->iDtmfType);
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCABSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);
		g_CCFsmState = CCFSM_VIDEO_MONIT;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
		break;
	}
#ifdef AB_HAVE_INDOOR_MACHINE
    //HB待改  室内机监控
    case LANCC_WATCH_INCOMING:
    {
    	LAN_WATCH_MSG_ST *pstMsg = (LAN_WATCH_MSG_ST *)pvPrmvType;
    	LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:LANCC_WATCH_INCOMING");
    	if(strcmp(g_acLanMonitorIP,pstMsg->cFromIP) == 0)
    	{
    		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT the same IP Monitor");
    		break;
    	}
    	MsgCC2LANWatchLineuse(pstMsg->cFromIP);
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT MsgCC2LANWatchLineuse()");
		g_CCFsmState = CCFSM_VIDEO_MONIT;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
    	break;
    }
    //HB待改
	case LANCC_CALLEE_HANG_OFF:
	{
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:LANCC_CALLEE_HANG_OFF ");
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:CCFSM_IDLE");

        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_MONITOR_END, 0, "");
		break;
	}
	//HB待改
	case LANCC_CALLEE_OPENDOOR:
	{
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:LANCC_CALLEE_OPENDOOR");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_INDOOR_OPEN_DOOR, 0, "");
		CCABSendDTMF('#');
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCABSendDTMF,DTMF=%c",'#');
		g_CCFsmState = CCFSM_VIDEO_MONIT;
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
		break;
	}
	case LANCC_CALLEE_TALK_TIMEOUT:
	{
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:LANCC_CALLEE_RING_TIMEOUT");
        CCABCalleeHangOff();
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABCalleeHangOff");
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:CCFSM_IDLE");
		break;
	}
#endif
	case SIPCC_CALLEE_HANG_OFF:
	{
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_CALLEE_HANG_OFF");
        SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;

        if(g_ulCurCallId == pstRcvMsg->ulCallId)
        {
            CCSIPTerminate(pstRcvMsg->ulCallId);
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminateAll");
            CCABCalleeHangOff();
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCABCalleeHangOff");
            CCRemoveTimer();
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT remove TalkWait Timer.");
            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:IDLE");

            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_USERS_PHONE_HAND_UP, 0, "");
            LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
        }
		break;
	}
	//HB待改
	case CCCC_TIMEROUT_EVENT:
	{
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:CCCC_TIMEROUT_EVENT ");
        CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

        if(pstRcvMsg->uiTimerId == TIMERID_MONITOR_WAIT)
        {
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
            CCSIPTerminateAll();
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminateAll");
            CCABVideoMonCancel();
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCABVideoMonCancel");
            CCRemoveTimer();
            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:IDLE");
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_CLIENT_MONITOR_TIMEOUT, 0, "");
            LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
        }
		break;
	}
	//HB待改 添加呼叫室内机
	case ABCC_DOOR_CALL:
	{
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:ABCC_DOOR_CALL");
		CCSIPTerminateAll();
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminateAll");
#ifdef AB_HAVE_INDOOR_MACHINE
        g_iIndoorHangoff = 1;
        MsgCC2LANTerminate();
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT MsgCC2LANTerminate");
#endif
		ABCC_DOOR_CALL_ST *pstRcvMsg = (ABCC_DOOR_CALL_ST *)pvPrmvType;
		strcpy(g_iCurRoom, CC_NO_ROOM);
		g_iCurRoom[0] = pstRcvMsg->ucDevType;
		if(pstRcvMsg->ucDevType == 'E')
		{
			strcpy(g_iCurRoom + 1,pstRcvMsg->aucRoomNum+1);		//围墙机送7位房号
		}
		else
		{
			strcpy(g_iCurRoom + 1,pstRcvMsg->aucRoomNum);
		}
		LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
		g_iIsOpenDoorFlg = 0;

		g_iABHaveCall = 0;
		//HB待改 室内机呼叫 默认当做有室内机
#ifdef AB_HAVE_INDOOR_MACHINE
		g_iHaveIndoorMachine = 1;
		g_iIndoorHangoff = 0;
		g_iSipHangoff = 0;
		TMCCFilterHouseCode(g_iCurRoom,g_iLanRoom);
		LOG_RUNLOG_DEBUG("CC STATE:IDLE MsgCC2LANCall %s",g_iLanRoom);
		if(MsgCC2LANCall(g_iLanRoom) != SUCCESS)
		{
			g_iHaveIndoorMachine = 0;
			g_iIndoorHangoff = 1;
		}
		else
		{
			CCABCalleeErrRoomValid();
			g_iABHaveCall = 1;
		}
		LOG_RUNLOG_DEBUG("CC STATE:IDLE MsgCC2LANCall");
#endif

#ifdef TM_TAKE_PIC
		PICCallBegin();							//记录呼叫开始时间
#endif

		if(LPGetSipRegStatus() == REG_STATUS_ONLINE)
		{
			LOG_RUNLOG_DEBUG("CC STATE:LPGetSipRegStatus REG_STATUS_ONLINE");
			if(TMCCIsIMSCallEnabled() == TRUE)		//梯口机呼叫使能
			{
				LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:ABCC_DOOR_CALL DEVTYPE =%c, ROOMNUM =%s g_iCurRoom = %s",pstRcvMsg->ucDevType, pstRcvMsg->aucRoomNum, g_iCurRoom);
				if(TMCCGetHouseInfoByHouseCode(g_iCurRoom) == TRUE)
				{
	#ifdef _YK_IMX27_AV_
					LOG_RUNLOG_DEBUG("CC send host rsp (P)");
					SMSendHostRsq('P');
	#endif
					if(!g_iABHaveCall)
					{
						CCABCalleeErrRoomValid();
						g_iABHaveCall =1;
					}
					LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Room-Phome Num exist");

					TM_I6NUM_INFO_ST *pstPhoneNumInfoTmp = NULL;
					if(TMCCGetNumInfo(&pstPhoneNumInfoTmp) == 0)
					{
						memcpy(&g_stCurPhoneNumInfo, pstPhoneNumInfoTmp, sizeof(TM_I6NUM_INFO_ST));
						CCSSCapturePicture(g_stCurPhoneNumInfo.acPhoneNum);
						LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CapturePicture ");

						g_stCCTimeoutValue.iCallDelay = TMCCGetSipCallDelay(NULL) * 1000;
						if(g_stCCTimeoutValue.iCallDelay == 0)
						{
							CCSIPTerminateAll();
							LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminateAll");
							CCSIPCall(g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType,g_stCurPhoneNumInfo.ucPreView);
							LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Start call.(PhoneNum: %s, CallType: %d)", g_stCurPhoneNumInfo.acPhoneNum, g_stCurPhoneNumInfo.ucCallType);
							LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_iCurRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
							g_stCCTimeoutValue.iRingTime = TMCCGetRingTime(NULL) * 1000;
							CCCreateTimer(TIMERID_CALL_WAIT, g_stCCTimeoutValue.iRingTime);
							LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Create CallWait Timer");
							g_CCFsmState = CCFSM_CALL_WAIT;
							LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:CALL_WAIT");
						}
						else
						{
							CCCreateTimer(TIMERID_DELAY_CALL, g_stCCTimeoutValue.iCallDelay);
							LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Create DelayCall Timer Interval=%dMs",g_stCCTimeoutValue.iMonitorTImeout);
							g_CCFsmState = CCFSM_DELAY;
							LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:DELAY");
						}
					}
					else
					{
						strcpy(g_iCurRoom, CC_NO_ROOM);
						LOG_RUNLOG_DEBUG("CC VIDEO_MONIT:VIDEO_MONIT Room-Phone Num not exist");
						g_iSipHangoff = 1;
	#ifdef AB_HAVE_INDOOR_MACHINE
						if(g_iIndoorHangoff == 1)
						{
	#endif
							g_CCFsmState = CCFSM_IDLE;
							LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:IDLE");
	#ifdef AB_HAVE_INDOOR_MACHINE
						}
						else
						{
							g_CCFsmState = CCFSM_CALL_WAIT;
							LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:CALL_WAIT");
						}
	#endif
					}
				}
				else
				{
					strcpy(g_iCurRoom, CC_NO_ROOM);
					LOG_RUNLOG_DEBUG("CC VIDEO_MONIT:VIDEO_MONIT Room Num not exist");
					g_iSipHangoff  = 1;
	#ifdef AB_HAVE_INDOOR_MACHINE
					if(g_iIndoorHangoff == 1)
					{
	#endif
						CCABCalleeErrRoomInvalid();
						g_CCFsmState = CCFSM_IDLE;
						LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:IDLE");
	#ifdef AB_HAVE_INDOOR_MACHINE
					}
					else
					{
						g_CCFsmState = CCFSM_CALL_WAIT;
						LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:CALL_WAIT");
					}
	#endif
				}
			}
			else
			{
			LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Ims Call Disable!");
			g_iSipHangoff = 1;
#ifdef AB_HAVE_INDOOR_MACHINE
					if(g_iIndoorHangoff == 1)
					{
#endif
						strcpy(g_iCurRoom, CC_NO_ROOM);
						CCABCalleeErr(CALLEE_ERROR_CALL_DISABLE);
						LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCABCalleeErr");
						g_CCFsmState = CCFSM_IDLE;
						LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:IDLE");
#ifdef AB_HAVE_INDOOR_MACHINE
					}
					else
					{
						g_CCFsmState = CCFSM_CALL_WAIT;
						LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:CALL_WAIT");
					}
#endif
		}
		}
		else
		{
			g_iSipHangoff = 1;
			if(g_iABHaveCall == 1)
			{
				g_CCFsmState = CCFSM_CALL_WAIT;
			}
			LOG_RUNLOG_DEBUG("CC STATE:LPGetSipRegStatus REG_STATUS_OFFLINE");
		}

		break;
	}
	//HB待改 室内机监控挂断

	default:
		LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT default recv %x",*(( unsigned int *)pvPrmvType));
		break;
	}
}




#endif		//#ifdef _I6_CC_






