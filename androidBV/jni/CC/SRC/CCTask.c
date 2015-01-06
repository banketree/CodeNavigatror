/*********************************************************************
*  Copyright (C), 2001 - 2012, 福建邮科通信技术有限公司.
*  File			:
*  Author		:李程达
*  Version 		:
*  Date			:
*  Description	:
*  History		: 更新记录 .
*            	  <修改作者>  <修改时间>  <修改描述>
**********************************************************************/
#include "IDBT.h"

#ifndef _I6_CC_

//与工程相关
#include "YKApi.h"
#include "YKTimer.h"
#include "YKMsgQue.h"
#include "CCTask.h"
#include "LPIntfLayer.h"
#include "LOGIntfLayer.h"
#include "TMInterface.h"
#include "FPInterface.h"
#include "Snapshot.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

extern YK_MSG_QUE_ST  *g_pstABMsgQ;

#ifdef _YK_IMX27_AV_
#include "SMEventHndlImx27.h"
#include "CaptureJpeg.h"
#endif

#define PIC_TAKE_TIMEOUT 10 * 1000 	//个推超时，单位ms

#define CC_NO_ROOM "000000000000"

#define CAPTURE_PIC_FILE   		"/data/data/com.fsti.ladder/capture.jpeg"
#define _PIC_FILE_NAME_TPYE_    0           // 1:LOCAL SIP NUM; 0:REMOTE SIP NUM

#define SIP_CALL_ERROR 0   //sip call ,IMS return error
#define SIP_CALL_OK 1    // sip call ,IMS return ok
#define SOCKET_CONNECT_ERROR  	0  //socket connect error
#define SOCKET_CONNECT_OK      		1  //socket connect ok

#define PRIMARY_CALL_RINGING_TIME  30*1000  //主叫号码振铃时长
#define TEL1                    0   //电话号码1
#define TEL2                    1   //电话号码2
#define DEMOL_CALL_TEL			2  //访客体验电路域
//全局变量定义
int 			g_CCFsmState;
int 			g_iRunCCTaskFlag;
unsigned char	g_CCFsmRoom[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;		//当前房号（第一位表示设备类型）
struct timeval g_stGetActEventTime;
extern struct timeval g_stGetActTime;

int g_iSipCallFlag = SIP_CALL_OK;
int g_iSocketConnectFlag = SOCKET_CONNECT_OK;
unsigned char 	g_aucDemolCallPhoneNum[PHONE_NUM_BUFF_LEN];		//访客体验电路域电话号码

CC_TIMER_ST     g_CCDelayCallTimer;							//Timer1
CC_TIMER_ST     g_CCCallWaitTimer;								//Timer2
CC_TIMER_ST     g_CCCallPicResultWaitTimer;					//Timer4

YK_MSG_QUE_ST  *g_pstCCMsgQ;
YK_MSG_QUE_ST  *g_pstSMMsgQ; //temp
//extern YK_MSG_QUE_ST  *g_pstSSMsgQ;
YKHandle        g_CCThreadId;

static unsigned char  aucPhoneNum[PHONE_NUM_BUFF_LEN];
static unsigned long ulCallId=0;
static unsigned long ulCurErrId = 0;

//extern void PICTakePicture(char *pcSipAccount);
//extern void PICTimeOut();
extern void PICCallBegin();  // "#"号键按完
extern void TRHndlDemo(const char *acPhoneNumber, const char *cVoipEnable);

#ifdef _YK_XT8126_
extern V8126StartCapturePic(void);
#endif



/*
*判断sip呼叫是否拒接
*拒接返回1,其它错误返回0
*add by cs 2013-04-23
*/
unsigned int CCSipCallReject(unsigned int CodeId)
{
    if ((CodeId == 1021) || (CodeId == 603) || (CodeId == 486))
    {
        LOG_RUNLOG_DEBUG("CC SIP CALL STATE reject");
        return 1;
    }
    else
    {
        return 0;
    }
}



char *CCGetCurFsmRoom(void)
{
	return g_CCFsmRoom+1;
}

char *CCGetCcStatus(void)
{
	return g_CCFsmState;
}

void CCCreateTimer(int iTimeType, unsigned int uiInterval)
{
    LOG_RUNLOG_DEBUG("CC STATE:CALLEE_MUSIC_END CCSIPCall Interval=%dMs",uiInterval);
    g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                        (void*) iTimeType,
                                                                        uiInterval, YK_TT_NONPERIODIC,
                                                                        (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
    LOG_RUNLOG_DEBUG("CC STATE:IDLE Create TalkWait Timer");
}

void CCRemoveTimer(void)
{
    if(g_CCCallWaitTimer.pstTimer != NULL)
    {
        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);
    }
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
int CCTaskInit(void)
{
    //初始化
    g_iRunCCTaskFlag = TRUE;		                                 //CCTask 运行标志置位
    g_pstCCMsgQ  = YKCreateQue(1024);                                //创建消息队列；

    g_CCFsmState = CCFSM_IDLE;	                                     //有限状态机状态初始化
    g_CCThreadId = YKCreateThread(CCTask, NULL);                     //创建CCTask

    if(NULL == g_CCThreadId)
    {
        return FAILURE;
    }

    return SUCCESS;
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCTaskUninit(void)
{
    //卸载
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
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void *CCTask(void)
{
    while(g_iRunCCTaskFlag)		                                    //CCTask运行标志为真
    {
        CCFsm();
    }
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
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

    	if(ulCurErrId >= pstRcvMsg->ulCallId)
    	{
    		LOG_RUNLOG_DEBUG("CC this error call is already processed!");
    		free((void *)(pvPrmvType));
    		return ;
    	}
    	else
    	{
    		ulCurErrId = pstRcvMsg->ulCallId;
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
    case CCFSM_SIP1_CALL_WAIT:									//第一个SIP账号呼叫等待状态
        CCFsmSip1Process(pvPrmvType);
        break;
    case CCFSM_TEL1_CALL_WAIT:									//第一个开门号码呼叫等待状态
        CCFsmTel1Process(pvPrmvType);
        break;
    case CCFSM_SIP2_CALL_WAIT:									//第二个SIP账号呼叫等待状态
        CCFsmSip2Process(pvPrmvType);
        break;
    case CCFSM_TEL2_CALL_WAIT:									//第二个开门号码呼叫等待状态
        CCFsmTel2Process(pvPrmvType);
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
    case CCFSM_DEMO_CALL:
        CCFsmDemoCallProcess(pvPrmvType);
        break;
    case CCFSM_PRIMARY_CALL_WAIT:
    	CCFsmPrimaryCallProcess(pvPrmvType);
    	break;

    default:
        break;
    }
    free((void *)(pvPrmvType));										//释放消息内容指针
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmIdleProcess(void *pvPrmvType)
{
    int  iRet;
    unsigned int uiInterval;

    LOG_RUNLOG_DEBUG("CC STATE:IDLE");
    strcpy(g_CCFsmRoom, CC_NO_ROOM);

    //---------------------------------------------------------------------------------->>To IDLE
    if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
    {
        SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
        //无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
                pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
        LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);	//判断号码是否合法


        if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
        {
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");

            LOG_RUNLOG_DEBUG("CC STATE:IDLE InComing Phone Num Valid");
            //CCSMOpenDoor();
            CCABOpenDoor();
            LOG_RUNLOG_DEBUG("CC STATE:IDLE CCABOpenDoor");
            //===========add by hb===========
            //ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
            //===============================
            LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);

        }
        //===========add by hb===========
        else
        {
        	//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
        	LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
        }
        //===============================



        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
    }
    //---------------------------------------------------------------------------------->>To Delay or SIP1_CALL_WAIT or TEL1_CALL_WAIT
    if(*(( unsigned int *)pvPrmvType) == ABCC_DOOR_CALL)
    {
        ABCC_DOOR_CALL_ST *pstRcvMsg = (ABCC_DOOR_CALL_ST *)pvPrmvType;
        g_CCFsmRoom[0] = pstRcvMsg->ucDevType;
        strcpy(g_CCFsmRoom + 1,pstRcvMsg->aucRoomNum);

        //记录合法的房号
        //写入日志
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    	//写入日志
    	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
#ifdef TM_TAKE_PIC
    	PICCallBegin();  // "#"号键按完
        InitSipCallFlagAndSocketConnectFlag();
#endif
    	if(TMCCIsIMSCallEnabled() == 1)//呼叫使能
        {
    		LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:ABCC_DOOR_CALL DEVTYPE =%c",pstRcvMsg->ucDevType);
            LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:ABCC_DOOR_CALL ROOMNUM =%s",pstRcvMsg->aucRoomNum);

            if(TMCCGetIMSPhoneNumber(g_CCFsmRoom) == NULL)
            {
				LOG_RUNLOG_DEBUG("CC STATE:IDLE PRIMARY NUM not exist");
            }
            else
            {
#ifdef _YK_IMX27_AV_
            	LOG_RUNLOG_DEBUG("CC send host rsp (P)");
            	SMSendHostRsq('P');
#endif
            	LOG_RUNLOG_DEBUG("CC STATE:IDLE PRIMARY NUM exist");
				CCSIPTerminateAll();
				LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminateAll");

				CCSIPCall(TMCCGetIMSPhoneNumber(g_CCFsmRoom), MEDIA_TYPE_AUDIO_VIDEO, HINT_MUSIC_DISABLE);
//				sleep(1);	//防止短时间内发出呼叫和挂断指令造成服务器无法及时处理

				LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPCall Interval=%dMs",PRIMARY_CALL_RINGING_TIME);
				g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
														  (void*) TIMERID_CALL_WAIT,
														  PRIMARY_CALL_RINGING_TIME, YK_TT_NONPERIODIC,
														  (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
				LOG_RUNLOG_DEBUG("CC STATE:IDLE Create CallWait Timer");
				g_CCFsmState = CCFSM_PRIMARY_CALL_WAIT;
				LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:PRIMARY_CALL_WAIT");

				CCABCalleeErrRoomValid();
            	return ;
            }

            iRet = IsRoomPhoneNumExist(g_CCFsmRoom,ROOM_NUM_LEN);
            if(iRet == ROOM_PHONE_NUM_Y)					//存在房号对应的电话号码
            {
                LOG_RUNLOG_DEBUG("CC STATE:IDLE Room-Phone Num exist");

                CCABCalleeErrRoomValid();

#ifdef _YK_IMX27_AV_
                LOG_RUNLOG_DEBUG("CC send host rsp (P)");
                SMSendHostRsq('P');
#endif
                LOG_RUNLOG_DEBUG("CC ABCC_DOOR_CALL ");

                uiInterval = CCGetTime4DelayCall(g_CCFsmRoom,ROOM_NUM_LEN);					//根据房号提取延时呼叫参数,毫秒单位
                LOG_RUNLOG_DEBUG("CC STATE:IDLE CCGetTime4DelayCall Interval=%dMs",uiInterval);

                if(uiInterval == 0)                                                         //延时呼叫参数为0
                {
                    LOG_RUNLOG_DEBUG("CC STATE:IDLE DelayCall Interval = 0,Omit STATE DELAY");
                    iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_IDLE);
                    switch(iRet)
                    {
                    case SIP1_Y:													        //SIP1号码存在
                        LOG_RUNLOG_DEBUG("CC STATE:IDLE SIP1 exist");
                        CCSIPTerminateAll();
                        LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminateAll");

						CCSSCapturePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP1));	//抓拍
						LOG_RUNLOG_DEBUG("CC SIP1 CapturePicture ");

                        CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP1),               //获取电话号码等参数
                                        CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP1),
                                        CCGetHintMusicEn(g_CCFsmRoom));
//                        sleep(1);	//防止短时间内发出呼叫和挂断指令造成服务器无法及时处理

                        uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                        LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPCall Interval=%dMs",uiInterval);
                        g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                  (void*) TIMERID_CALL_WAIT,
                                                                  uiInterval, YK_TT_NONPERIODIC,
                                                                  (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                        LOG_RUNLOG_DEBUG("CC STATE:IDLE Create CallWait Timer");
                        g_CCFsmState = CCFSM_SIP1_CALL_WAIT;
                        LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:SIP1_CALL_WAIT");
                        break;
                    case SIP1_N_TEL1_Y:                                                     //SIP1号码不存在,TEL1号码存在
                        LOG_RUNLOG_DEBUG("CC STATE:IDLE TEL1 exist");
                        CCSIPTerminateAll();

                        LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminateAll");
                        CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL1),               //获取电话号码等参数
                                        CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL1),
                                        CCGetHintMusicEn(g_CCFsmRoom));

                        uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                        LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPCall Interval=%dMs",uiInterval);
                        g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                                            (void*) TIMERID_CALL_WAIT,
                                                                                            uiInterval, YK_TT_NONPERIODIC,
                                                                                            (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                        LOG_RUNLOG_DEBUG("CC STATE:DELAY Create CallWait Timer");
                        g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
                        LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:TEL1_CALL_WAIT");
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    g_CCDelayCallTimer.pstTimer = YKCreateTimer(CCTimerCallBack,
                                                               (void*) TIMERID_DELAY_CALL,
                                                               uiInterval, YK_TT_NONPERIODIC,
                                                               (unsigned long *)&g_CCDelayCallTimer.ulMagicNum);

                    LOG_RUNLOG_DEBUG("CC STATE:IDLE Create DelayCall Timer Interval=%dMs",uiInterval);
                    g_CCFsmState = CCFSM_DELAY;
                    LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:DELAY");
                }
            }
            else																			//不存在房号对应的电话号码
            {
                strcpy(g_CCFsmRoom, CC_NO_ROOM);
                LOG_RUNLOG_DEBUG("CC STATE:IDLE Room-Phone Num not exist");
                //CCSMCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);
                //CCABCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);
                CCABCalleeErrRoomInvalid();

                g_CCFsmState = CCFSM_IDLE;
                LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
            }
        }
        //---------------------------------------------------------------------------------->>add by ytz
        //禁用SIP呼叫时，呼叫时通知SM呼叫错误
        else
        {
        	strcpy(g_CCFsmRoom, CC_NO_ROOM);
			LOG_RUNLOG_DEBUG("CC STATE:Ims Call Disable!");
			//CCSMCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);
			CCABCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);

			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
        }
    }
    //---------------------------------------------------------------------------------->>To VIDEO_MONIT
    if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
    {
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;
        LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
                    pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        iRet = VideoIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);

        if((iRet == NUM_VALID) && (TMCCIsVoipMonitorEnabled() == 1))						//视频监控使能 && 号码合法
        {
            LOG_RUNLOG_DEBUG("CC STATE:IDLE InComing Phone Num Valid");

#ifdef _YK_XT8126_
			CCSIPAnswer(pstRcvMsg->ulInComingCallId);																//CCSIP_ANSWER
			LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPAnswer()");

            //写入日志
            LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_CLIENT_MONITOR_START, 0, "");

			CCSMVideoMonitor();
			ulCallId = pstRcvMsg->ulInComingCallId;//add by zlin
			LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSMVideoMonitor()");
			//===========add by hb===========
			ATSendMonitorResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
			g_CCFsmState = CCFSM_VIDEO_MONIT;
			LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:VIDEO_MONIT");

			uiInterval = 30 * 1000;//30秒
			CCCreateTimer(TIMERID_TALK_WAIT, uiInterval);
#else



		    	CCSIPAnswer(pstRcvMsg->ulInComingCallId);																//CCSIP_ANSWER
		    	LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPAnswer()");

		    	//写入日志
				LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_CLIENT_MONITOR_START, 0, "");
				g_CCFsmState = CCFSM_VIDEO_MONIT;
				LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:VIDEO_MONIT");
		        uiInterval = 30 * 1000;//30秒
		        CCCreateTimer(TIMERID_TALK_WAIT, uiInterval);


            //CCSMVideoMonitor();
			CCABVideoMonitor();

            ulCallId = pstRcvMsg->ulInComingCallId;//add by zlin
            LOG_RUNLOG_DEBUG("CC STATE:IDLE CCABVideoMonitor()");
            //===========add by hb===========
            //ATSendMonitorResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
#endif
        }
        else																			//视频来电 && 号码不合法
        {
            LOG_RUNLOG_DEBUG("CC STATE:IDLE InComing Phone Num InValid");
            CCSIPTerminate(pstRcvMsg->ulInComingCallId);								//CCSIP_TERMINATE
            LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);
            //===========add by hb===========
            //ATSendMonitorResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
            LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_CLIENT_MONITOR_START, 1, "");
        }
    }

    //-----------------------------add by zlin------------------------------------------>>To CCFSM_DEMO_CALL
    if(*(( unsigned int *)pvPrmvType) == TMCC_DEMO_CALL)
    {
        TMCC_DEMO_CALL_ST *pstRcvMsg = (TMCC_DEMO_CALL_ST *)pvPrmvType;
        //无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:TMCC_DEMO_CALL PhoneNum = %s,MediaType = %d",
                pstRcvMsg->aucPhoneNum,	pstRcvMsg->aucMediaType);

#ifdef TM_TAKE_PIC
        LOG_RUNLOG_DEBUG("CC CapturePicture!");

	  switch (pstRcvMsg->aucMediaType)
	  {
	  	  case MEDIA_TYPE_AUDIO:				//保留
	  		    break;

	  	  case MEDIA_TYPE_AUDIO_VIDEO:
			    CCSIPTerminateAll();
			    LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminateAll");
			    CCSIPCall(pstRcvMsg->aucPhoneNum, pstRcvMsg->aucMediaType, 0);
			    uiInterval = 30* 1000;		//
			    CCCreateTimer(TIMERID_TALK_WAIT, uiInterval);
			    LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPCall Interval=%dMs",uiInterval);
			    g_CCFsmState = CCFSM_DEMO_CALL;
			    LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:DEMO_CALL");

			    LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_DEMO_CALL, 0, "");
			    break;

	  	  case MEDIA_TYPE_TEL:
				strcpy(g_aucDemolCallPhoneNum,pstRcvMsg->aucPhoneNum);
				PICCallBegin();
				CCSMVideoMonitor();
				CCDealTakePicture(DEMOL_CALL_TEL);
				sleep(4);				//不然会抓不到图片
				CCSMVideoMonCancel();
				g_CCFsmState = CCFSM_DEMO_CALL;
	  		break;

	  	  default:
	  		  break;
	  }


#else
	  CCSIPTerminateAll();
	  LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminateAll");
        CCSIPCall(pstRcvMsg->aucPhoneNum, pstRcvMsg->aucMediaType, 0);

        g_CCFsmState = CCFSM_DEMO_CALL;
        LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:DEMO_CALL");

        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_DEMO_CALL, 0, "");
        uiInterval = 30 * 1000;//30秒
        CCCreateTimer(TIMERID_TALK_WAIT, uiInterval);
#endif
    }

    //---------------------------------------------------------------------------------->>add by xuqd
    if(*(( unsigned int *)pvPrmvType) == ABCC_DOOR_HANG_OFF)
    {
    	sleep(1);
        CCSIPTerminateAll();
//        PICTimeOut();
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:IDLE RECEIVE HANG OFF AGAIN -> STATE:IDLE");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    }
//    if(*(( unsigned int *)pvPrmvType) == SMCC_INDOOR_PICK_UP )
//    {
//        //写入日志
//        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");
//        CCSIPTerminateAll();
//
//        g_CCFsmState = CCFSM_IDLE;
//        LOG_RUNLOG_DEBUG("CC STATE:IDLE RECEIVE HANG OFF AGAIN -> STATE:IDLE");
//    }
//    if(*(( unsigned int *)pvPrmvType) == SMCC_VEDIO_MONITOR_START )
//    {
//    	CCSIPAnswer(ulCallId);																//CCSIP_ANSWER
//    	LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPAnswer()");
//
//    	//写入日志
//		LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
//		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_CLIENT_MONITOR_START, 0, "");
//		g_CCFsmState = CCFSM_VIDEO_MONIT;
//		LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:VIDEO_MONIT");
//        uiInterval = 30 * 1000;//30秒
//        CCCreateTimer(TIMERID_TALK_WAIT, uiInterval);
//    }
//    if(*(( unsigned int *)pvPrmvType) == SMCC_DOOR_DET_CANCEL )
//    {
//        //写入日志
//        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_CLIENT_MONITOR_START, 1, "");
//
//        LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:SMCC_DOOR_DET_CANCEL");
//        CCSIPTerminateAll();
//        LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminateAll");
//
//        g_CCFsmState = CCFSM_IDLE;
//        LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
//    }
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmDelayProcess(void *pvPrmvType)
{
    int iRet;
    unsigned int uiInterval;
    LOG_RUNLOG_DEBUG("CC STATE:DELAY");
    //---------------------------------------------------------------------------------->>To DELAY
    if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
    {
        SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
																						//无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
                    pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
        LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
        memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

        iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum, g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法
        if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
        {
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
            LOG_RUNLOG_DEBUG("CC STATE:DELAY InComing Phone Num Valid");
            //CCSMOpenDoor(); 															//CCSM_OPENDOOR;
            CCABOpenDoor();
            LOG_RUNLOG_DEBUG("CC STATE:DELAY CCABOpenDoor");
            //===========add by hb===========
            //ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
            LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================

         memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);

        g_CCFsmState = CCFSM_DELAY;
        LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:DELAY");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
    {
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;
        LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
                    pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
        LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

//        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
//        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_CLIENT_MONITOR_START, 1, "");

        g_CCFsmState = CCFSM_DELAY;
        LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:DELAY");
    }

    //---------------------------------------------------------------------------------->>To IDLE
    if( *(( unsigned int *)pvPrmvType) == ABCC_DOOR_HANG_OFF)
    {
    	sleep(1);
        LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:ABCC_DOOR_HANG_OFF");
        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//        PICTimeOut();
        LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminateAll");
        YKRemoveTimer(g_CCDelayCallTimer.pstTimer, g_CCDelayCallTimer.ulMagicNum);		//Destroy the timer1 for delay call
#ifdef TM_TAKE_PIC
        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
#endif
        LOG_RUNLOG_DEBUG("CC STATE:DELAY Destroy DelayCall Timer");
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:DELAY ->STATE:IDLE ");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    }
//    if( *(( unsigned int *)pvPrmvType) == SMCC_INDOOR_PICK_UP )
//    {
//        LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:SMCC_INDOOR_PICK_UP OR SMCC_INDOOR_PICK_UP");
//        //写入日志
//        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");
//        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//        LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminateAll");
//        YKRemoveTimer(g_CCDelayCallTimer.pstTimer, g_CCDelayCallTimer.ulMagicNum);		//Destroy the timer1 for delay call
//#ifdef TM_TAKE_PIC
//        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
//#endif
//        LOG_RUNLOG_DEBUG("CC STATE:DELAY Destroy DelayCall Timer");
//        g_CCFsmState = CCFSM_IDLE;
//        LOG_RUNLOG_DEBUG("CC STATE:DELAY ->STATE:IDLE ");
//    }

    if(*(( unsigned int *)pvPrmvType) == CCCC_TIMEROUT_EVENT)							//超时事件
    {
        CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

        LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:CCCC_TIMEROUT_EVENT ");
        if(pstRcvMsg->uiTimerId == TIMERID_DELAY_CALL)									//延时呼叫超时
        {
            LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:CCCC_TIMEROUT_EVENT ID=DelayCall");
            iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_DELAY);
            switch(iRet)
            {
            case SIP1_Y:													        //SIP1号码存在
                LOG_RUNLOG_DEBUG("CC STATE:DELAY SIP1 exist");
                CCSIPTerminateAll();
                LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminateAll");

 				CCSSCapturePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP1));
				LOG_RUNLOG_DEBUG("CC SIP1 CapturePicture ");


                CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP1),               //获取电话号码等参数
                                CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP1),
                                CCGetHintMusicEn(g_CCFsmRoom));

//                sleep(1);
                uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPCall Interval=%dMs",uiInterval);

                g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                                    (void*) TIMERID_CALL_WAIT,
                                                                                    uiInterval, YK_TT_NONPERIODIC,
                                                                                    (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                LOG_RUNLOG_DEBUG("CC STATE:DELAY Create CallWait Timer");
                g_CCFsmState = CCFSM_SIP1_CALL_WAIT;
                LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:SIP1_CALL_WAIT");

                break;
            case SIP1_N_TEL1_Y:                                                     //SIP1号码不存在,TEL1号码存在
                LOG_RUNLOG_DEBUG("CC STATE:DELAY TEL1 exist");
                CCSIPTerminateAll();
                LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminateAll");
                CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL1),               //获取电话号码等参数
                                CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL1),
                                CCGetHintMusicEn(g_CCFsmRoom));

                uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPCall Interval=%dMs",uiInterval);
                g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                                    (void*) TIMERID_CALL_WAIT,
                                                                                    uiInterval, YK_TT_NONPERIODIC,
                                                                                    (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                LOG_RUNLOG_DEBUG("CC STATE:DELAY Create CallWait Timer");

                g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
                LOG_RUNLOG_DEBUG("CC STATE:DELAY -> STATE:TEL1_CALL_WAIT");
                break;
            default:
                break;
            }
        }
    }
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmSip1Process(void *pvPrmvType)
{
    int 			iRet ;
    unsigned int 	uiInterval;
    LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT");
    //---------------------------------------------------------------------------------->>To SIP1_CALL_WAIT
    if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
    {
        SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
																						//无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
            pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
        memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

        iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum, g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法

        if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
        {
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT InComing Phone Num Valid");
            //CCSMOpenDoor(); 															//CCSM_OPENDOOR;
            CCABOpenDoor();
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCABOpenDoor");
            //===========add by hb===========
            //ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
            LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================

         memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);

        g_CCFsmState = CCFSM_SIP1_CALL_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:SIP1_CALL_WAIT");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_MESSAGE_INCOMING)							//SIP_MESSAGE
	{
		SIPCC_MESSAGE_INCOMING_ST *pstRcvMsg = (SIPCC_MESSAGE_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_MESSAGE_INCOMING aucPhoneNum = %s, aucUserName = %s,uiMsgSeq = %ld",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->aucUserName,	pstRcvMsg->uiMsgSeq);

	    unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
		memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

		iRet = VideoIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);	//判断号码是否合法

		if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
		{
			LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT InComing Phone Num Valid");
			CCABOpenDoor();															//CCSM_OPENDOOR;
			LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSMOpenDoor");
			LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, 0);
			//写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");

		}
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, -1);
		}
		memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);
        g_CCFsmState = CCFSM_SIP1_CALL_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:SIP1_CALL_WAIT");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
    {
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;

        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
        	pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        g_CCFsmState = CCFSM_SIP1_CALL_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:SIP1_CALL_WAIT");
    }
    //---------------------------------------------------------------------------------->>To IDLE
    if( *(( unsigned int *)pvPrmvType) == ABCC_DOOR_HANG_OFF )
    {
    	sleep(1);
	#ifdef TM_TAKE_PIC
    	InitSipCallFlagAndSocketConnectFlag();
        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
	#endif
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:ABCC_DOOR_HANG_OFF");
        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//        PICTimeOut();
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");

        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Remove CallWait Timer");

        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:IDLE");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    }
//    if( *(( unsigned int *)pvPrmvType) == SMCC_INDOOR_PICK_UP )
//    {
//	#ifdef TM_TAKE_PIC
//		InitSipCallFlagAndSocketConnectFlag();
//	    YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
//
//	#endif
//        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SMCC_C_PICK_UP");
//        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");
//
//        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
//        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Remove CallWait Timer");
//
//        g_CCFsmState = CCFSM_IDLE;
//        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:IDLE");
//        //写入日志
//        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");
//    }
//    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_HANG_OFF)
//     {
//         SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;
//         LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_CALLEE_HANG_OFF:%d", pstRcvMsg->ulCallId);
//         //-------------------edit by zlin
//         if(ulCallId == pstRcvMsg->ulCallId)
//         {
//             CCSIPTerminate(pstRcvMsg->ulCallId);									//CCSIP_TERMINATE
//             LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");
//             //写入日志
//             LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_HAND_UP, 0, "");
//
//             //CCSMCalleeHangOff();															//CCSM_CALLEE_HANG_OFF
//             CCABCalleeHangOff();
//             LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSMCalleeHangOff");
//
//             CCRemoveTimer();
//
//             g_CCFsmState = CCFSM_IDLE;
//             LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:IDLE");
//         }
//         //-------------------
//
// 		//===========add by hb===========
// 		//ATSendTestResult();
// 		//===============================
//     }
	if(*(( unsigned int *)pvPrmvType) == SIPCC_PROCESS)
	{
		SIPCC_PROCESS_ST *pstRcvMsg = (SIPCC_PROCESS_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_PROCESS id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
	if(*(( unsigned int *)pvPrmvType) == SIPCC_RING)
	{
		SIPCC_RING_ST *pstRcvMsg = (SIPCC_RING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_RING id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
    if(*(( unsigned int *)pvPrmvType) == CCCC_TIMEROUT_EVENT)							//超时事件
    {
	#ifdef TM_TAKE_PIC
    	InitSipCallFlagAndSocketConnectFlag();
        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
	#endif
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ");
        CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

        if(pstRcvMsg->uiTimerId == TIMERID_CALL_WAIT)									//呼叫等待超时
        {
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
            //写入日志
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, ERR_TIMEOUT_CALL_WAIT);

            iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_SIP1_CALL_WAIT);

            switch(iRet)
            {
            case SIP1_Y_OTHR_N:                                                     //SIP1存在 其他号码不存在
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT SIP1 exist but others not exist");
                CCSIPTerminateAll();												//CCSIP_TERMINATE_ALL
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");

                //CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT);								//CCSM_CALLEE_ERR(超时)
                CCABCalleeErr(ERR_TIMEOUT_CALL_WAIT);
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT)");

                g_CCFsmState = CCFSM_IDLE;
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:IDLE");

                //写入日志
                LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);

                break;
            case SIP1_Y_TEL1_Y:                                                     //TEL1存在
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT TEL1 exist");
                CCSIPTerminateAll();
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");
                CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL1),               //获取TEL1号码及相关参数
                                CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL1),
                                CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL

                uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);

                g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                                    (void*) TIMERID_CALL_WAIT,
                                                                                    uiInterval, YK_TT_NONPERIODIC,
                                                                                    (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Create CallWait Timer");
                g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:TEL1_CALL_WAIT");
                break;
               
            case SIP1_Y_TEL1_N_SIP2_Y:                                              //TEL1不存在 SIP2存在
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT TEL1 not exist but SIP2 exist");
                CCSIPTerminateAll();												//CCSIP_TERMINATE_ALL
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");

                CCSSCapturePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2));	//抓拍
				LOG_RUNLOG_DEBUG("CC SIP2 CapturePicture ");

                CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2),               //获取SIP2号码及相关参数
                                CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP2),
                                CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL

//                sleep(1);
                uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);

                g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                                    (void*) TIMERID_CALL_WAIT,
                                                                                    uiInterval, YK_TT_NONPERIODIC,
                                                                                    (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Create CallWait Timer");

                g_CCFsmState = CCFSM_SIP2_CALL_WAIT;
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:SIP2_CALL_WAIT");

                break;
            case SIP1_Y_TEL1_N_SIP2_N_TEL2_Y:                                       //TEL1不存在 SIP2不存在 TEL2 存在
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT only Tel2 exist");
                CCSIPTerminateAll();												//CCSIP_TERMINATE_ALL
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");

                CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL2),               //获取TEL2号码及相关参数
                                CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL2),
                                CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL

                uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);

                g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                                    (void*) TIMERID_CALL_WAIT,
                                                                                    uiInterval, YK_TT_NONPERIODIC,
                                                                                    (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Create CallWait Timer");
                g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
                LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:TEL2_CALL_WAIT");
                break;
            default:
                break;
            }
        }
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_ERR)								//被叫方异常
    {
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_CALLEE_ERR ");
        SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;
        //写入日志
        char errorCause[20] = {0x0};
        sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);

        iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_SIP1_CALL_WAIT);

        if (CCSipCallReject(pstRcvMsg->uiErrorCause) == 1)      // 拒接add by cs 2013-04-23
		{
			 if (iRet == SIP1_Y_TEL1_Y)
			 {
				iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_TEL1_CALL_WAIT);
				if (iRet == TEL1_Y_SIP2_Y)
				{
					iRet = SIP1_Y_TEL1_N_SIP2_Y;
				}
				else if (iRet == TEL1_Y_SIP2_N_TEL2_Y)
				{
					iRet = SIP1_Y_TEL1_N_SIP2_N_TEL2_Y;
				}
				else
				{
					iRet = SIP1_Y_OTHR_N;
				}
			 }
		}

        switch(iRet)
        {
        case SIP1_Y_OTHR_N:                                                         //SIP1存在 其他号码不存在
		#ifdef TM_TAKE_PIC
			InitSipCallFlagAndSocketConnectFlag();
		    YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
		#endif
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT SIP1 exist but others not exist");
            //CCSIPTerminateAll();													//CCSIP_TERMINATE_ALL
            CCSIPTerminate(ulCurErrId);
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");

            //CCSMCalleeErr(pstRcvMsg->uiErrorCause);									//CCSM_CALLEE_ERR(原因)
            CCABCalleeErr(pstRcvMsg->uiErrorCause);
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSMCalleeErr(%d)",pstRcvMsg->uiErrorCause);

            YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Remove CallWait Timer");

            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:IDLE");
            break;

        case SIP1_Y_TEL1_Y:                                                         //TEL1存在
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT TEL1 exist");

            #ifdef TM_TAKE_PIC
            CCDealTakePicture(TEL1);
            #else
            CCSIPTerminate(ulCurErrId);
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");
            CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL1),                   //获取TEL1号码及相关参数
                            CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL1),
                            CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL

            YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Remove CallWait Timer");

            uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数

            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
            g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
                                                                                (void*) TIMERID_CALL_WAIT,
                                                                                uiInterval, YK_TT_NONPERIODIC,
                                                                                (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Create CallWait Timer");

            g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:TEL1_CALL_WAIT");
            #endif
            break;

        case SIP1_Y_TEL1_N_SIP2_Y:                                                  //TEL1不存在 SIP2存在
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT TEL1 not exist but SIP2 exist");
            CCSIPTerminate(ulCurErrId);													//CCSIP_TERMINATE_ALL
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");
#ifdef TM_TAKE_PIC
		  	InitSipCallFlagAndSocketConnectFlag();
	        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4

            PICTakePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2));		//上报平台，图片即将要上传
		    CCSSCapturePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2));
		    LOG_RUNLOG_DEBUG("CC SIP2 CapturePicture ");
#endif
            CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2),                   //获取SIP2号码及相关参数
                            CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP2),
                            CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL

//            sleep(1);
            YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Remove CallWait Timer");

            uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数

            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
            g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
                                                                                (void*) TIMERID_CALL_WAIT,
                                                                                uiInterval, YK_TT_NONPERIODIC,
                                                                                (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Create CallWait Timer");

            g_CCFsmState = CCFSM_SIP2_CALL_WAIT;
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:SIP2_CALL_WAIT");

            break;

        case SIP1_Y_TEL1_N_SIP2_N_TEL2_Y:                                           //TEL1不存在 SIP2不存在 TEL2 存在
	#ifdef TM_TAKE_PIC
		  	InitSipCallFlagAndSocketConnectFlag();
	        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
	#endif
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT only Tel2 exist");
            CCSIPTerminate(ulCurErrId);												//CCSIP_TERMINATE_ALL
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");

            CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL2),                   //获取TEL2号码及相关参数
                            CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL2),
                            CCGetHintMusicEn(g_CCFsmRoom));  				            //CCSIP_CALL

            YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Remove CallWait Timer");

            uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数

            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
            g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
                                                                                (void*) TIMERID_CALL_WAIT,
                                                                                uiInterval, YK_TT_NONPERIODIC,
                                                                                (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Create CallWait Timer");

            g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:TEL2_CALL_WAIT");

            break;

        default:
            break;
        }
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_PICK_UP)
    {
	#ifdef TM_TAKE_PIC
		InitSipCallFlagAndSocketConnectFlag();
	    YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
	#endif
    	SIPCC_CALLEE_PICK_UP_ST *pstRcvMsg = (SIPCC_CALLEE_PICK_UP_ST *)pvPrmvType;
        ulCallId = pstRcvMsg->ulCallId;
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_CALLEE_PICK_UP ");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_PICK_UP, 0, "");
    	//===========add by hb===========
    	//ATTestOK(AT_TEST_OBJECT_LP);
    	//===============================
        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Remove CallWait Timer");

        //CCSMCalleePickUp();																//CCSM_CALLEE_PICK_UP
        CCABCalleePickUp();
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCABCalleePickUp");

        g_CCFsmState = CCFSM_PLAY_MUSIC;
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:PLAY_MUSIC");

    }
    
	if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_SEND_DTMF)
	{
		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_CALLEE_SEND_DTMF");
		SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSMSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);
		if(pstRcvMsg->iDtmfType == '#')
		{
			CCABOpenDoor();
			//写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");
		}

		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:SIP1_CALL_WAIT");
	}

	if(*(( unsigned int *)pvPrmvType) == SIPCC_STOP_CALLING)
	{
		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_STOP_CALLING");
		CCABCalleeHangOff();
		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSMCalleeHangOff");
		CCRemoveTimer();
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:IDLE");
	}

#ifdef TM_TAKE_PIC
	  DealTakePicState(*(( unsigned int *)pvPrmvType),TEL1);
#endif
    
//	if(*(( unsigned int *)pvPrmvType) == SIPCC_RECV_EARLY_MEDIA)
//	{
//		SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;
//
//		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_RECV_EARLY_MEDIA ");
//		iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_SIP1_CALL_WAIT);
//
//		LOG_RUNLOG_DEBUG("CC ----------------------------------------");
//		LOG_RUNLOG_DEBUG("CC iRet=%d*****************:", iRet);
//		LOG_RUNLOG_DEBUG("CC ----------------------------------------");
//
//		switch(iRet)
//		{
//		case SIP1_Y_OTHR_N:
//			break;
//		case SIP1_Y_TEL1_Y:
//			CCSIPTerminateAll();
//
//			LOG_RUNLOG_DEBUG("CC ----------------------------------------");
//			//LOG_RUNLOG_DEBUG("CC i am here11111*****************:");
//			LOG_RUNLOG_DEBUG("CC ----------------------------------------");
//
//			YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
//			g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
//																		(void*) TIMERID_CALL_WAIT,
//																		uiInterval, YK_TT_NONPERIODIC,
//			                        									(unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
//			LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Create CallWait Timer");
//
//			CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL1),                   //获取TEL1号码及相关参数
//								          CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL1),
//								          CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL
//
//			g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
//
//			LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:TEL1_CALL_WAIT");
//
//			break;
//		case SIP1_Y_TEL1_N_SIP2_N_TEL2_Y:
//
//			CCSIPTerminateAll();
//			YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
//			CCSMCalleeErr(pstRcvMsg->uiErrorCause);
//			g_CCFsmState = CCFSM_IDLE;
//			LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:CCFSM_IDLE");
//			break;
//		case SIP1_Y_TEL1_N_SIP2_Y:
//			CCSIPTerminateAll();
//			CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2),                   //获取SIP2号码及相关参数
//								          CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP2),
//								          CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL
//
//			YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
//			uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数
//
//			LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
//			g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
//														(void*) TIMERID_CALL_WAIT,
//														uiInterval, YK_TT_NONPERIODIC,
//														(unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
//			LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Create CallWait Timer");
//
//			g_CCFsmState = CCFSM_SIP2_CALL_WAIT;
//			LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:SIP2_CALL_WAIT");
//			break;
//		default:
//			break;
//		}
//	}
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmTel1Process(void *pvPrmvType)
{
    int iRet;
    unsigned int uiInterval;
    LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT");
    //---------------------------------------------------------------------------------->>TEL1_CALL_WAIT
    if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
    {
        SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
        //无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
				pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);


        unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
        memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

        iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum, g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法

        if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
        {
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT InComing Phone Num Valid");
            //CCSMOpenDoor(); 															//CCSM_OPENDOOR;
            CCABOpenDoor();
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCABOpenDoor");
            //===========add by hb===========
            //ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================

            LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================

        memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);


        g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:TEL1_CALL_WAIT");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
    {
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;

        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
                    pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:TEL1_CALL_WAIT");
    }

    //---------------------------------------------------------------------------------->>To IDLE
    if( *(( unsigned int *)pvPrmvType) == ABCC_DOOR_HANG_OFF )
    {
    	sleep(1);
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT RCV:ABCC_DOOR_HANG_OFF");
        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//        PICTimeOut();
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminateAll");

        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Remove CallWait Timer");

        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:IDLE");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    }
//    if( *(( unsigned int *)pvPrmvType) == SMCC_INDOOR_PICK_UP )
//    {
//        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT RCV:SMCC_INDOOR_PICK_UP");
//        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminateAll");
//
//        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
//        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Remove CallWait Timer");
//
//        g_CCFsmState = CCFSM_IDLE;
//        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:IDLE");
//        //写入日志
//        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");
//    }

    if(*(( unsigned int *)pvPrmvType) == CCCC_TIMEROUT_EVENT)							//超时事件
    {
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ");
        CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

        if(pstRcvMsg->uiTimerId == TIMERID_CALL_WAIT)									//呼叫等待超时
        {
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ID=CallWait");

            //写入日志
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, ERR_TIMEOUT_CALL_WAIT);
            iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_TEL1_CALL_WAIT);
            switch(iRet)
            {
            case TEL1_Y_OTHR_N:                                                     //TEL1存在 其他号码不存在
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT TEL1 exist but others not exist");
                CCSIPTerminateAll();												//CCSIP_TERMINATE_ALL
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminateAll");
                //CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT);								//CCSM_CALLEE_ERR(超时)
                CCABCalleeErr(ERR_TIMEOUT_CALL_WAIT);
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT)");
                g_CCFsmState = CCFSM_IDLE;
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:IDLE");
                //===========add by hb===========
                ATSendTestResult();
                //===============================

                //写入日志
                LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
                break;
            case TEL1_Y_SIP2_Y:                                                     //SIP2存在
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT SIP2 exist");
                CCSIPTerminateAll();
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminateAll");
#ifdef TM_TAKE_PIC
                PICTakePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2));		//上报平台，图片即将要上传
    		    CCSSCapturePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2));	//抓拍
    		    LOG_RUNLOG_DEBUG("CC SIP2 CapturePicture");
#endif
                CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2),               //获取SIP2号码及相关参数
                                CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP2),
                                CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL
//                sleep(1);
                uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);

                g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                                    (void*) TIMERID_CALL_WAIT,
                                                                                    uiInterval, YK_TT_NONPERIODIC,
                                                                                    (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Create CallWait Timer");
                g_CCFsmState = CCFSM_SIP2_CALL_WAIT;
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:SIP2_CALL_WAIT");
                break;
            case TEL1_Y_SIP2_N_TEL2_Y:                                              //SIP2不存在 TEL2存在
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT SIP2 not exist TEL2 exist");
                CCSIPTerminateAll();												//CCSIP_TERMINATE_ALL
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminateAll");

                CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL2),               //获取TEL2号码及相关参数
                                CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL2),
                                CCGetHintMusicEn(g_CCFsmRoom)); 					        //CCSIP_CALL
                uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);

                g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                                    (void*) TIMERID_CALL_WAIT,
                                                                                    uiInterval, YK_TT_NONPERIODIC,
                                                                                    (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Create CallWait Timer");

                g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
                LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:TEL2_CALL_WAIT");
                break;
            default:
                break;
            }
        }
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_ERR)								//被叫方异常
    {
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT RCV:SIPCC_CALLEE_ERR ");
        SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;

        //写入日志
        char errorCause[20] = {0x0};
        sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);

        iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_TEL1_CALL_WAIT);
        switch(iRet)
        {
        case TEL1_Y_OTHR_N:                                                         //TEL1存在 其他号码不存在
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT TEL1 exist but others not exist");
            CCSIPTerminate(ulCurErrId);													//CCSIP_TERMINATE_ALL
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminateAll");

            //CCSMCalleeErr(pstRcvMsg->uiErrorCause);									//CCSM_CALLEE_ERR(原因)
            CCABCalleeErr(pstRcvMsg->uiErrorCause);
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCABCalleeErr(%d)",pstRcvMsg->uiErrorCause);

            YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Remove CallWait Timer");

            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:IDLE");

            //===========add by hb===========
			//ATSendTestResult();
			//===============================
            break;
        case TEL1_Y_SIP2_Y:                                                         //SIP2存在
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT SIP2 exist");
            CCSIPTerminate(ulCurErrId);
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminateAll");

		    CCSSCapturePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2));
		    LOG_RUNLOG_DEBUG("CC SIP2 CapturePicture ");

            CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2),                   //获取SIP2号码及相关参数
                            CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP2),
                            CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL

//            sleep(1);
            YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Remove CallWait Timer");

            uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
            g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
                                                                                (void*) TIMERID_CALL_WAIT,
                                                                                uiInterval, YK_TT_NONPERIODIC,
                                                                                (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Create CallWait Timer");
            g_CCFsmState = CCFSM_SIP2_CALL_WAIT;
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:SIP2_CALL_WAIT");
            break;
        case TEL1_Y_SIP2_N_TEL2_Y:                                                  //SIP2不存在 TEL2存在
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT SIP2 not exist TEL2 exist");
            CCSIPTerminate(ulCurErrId);												//CCSIP_TERMINATE_ALL
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminateAll");

            CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL2),                   //获取TEL2号码及相关参数
                            CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL2),
                            CCGetHintMusicEn(g_CCFsmRoom));   				        //CCSIP_CALL

            YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Remove CallWait Timer");

            uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
            g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
                                                                                (void*) TIMERID_CALL_WAIT,
                                                                                uiInterval, YK_TT_NONPERIODIC,
                                                                                (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Create CallWait Timer");
            g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
            LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:TEL2_CALL_WAIT");
            break;
        default:
            break;
        }
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_PICK_UP)
    {
    	SIPCC_CALLEE_PICK_UP_ST *pstRcvMsg = (SIPCC_CALLEE_PICK_UP_ST *)pvPrmvType;
        ulCallId = pstRcvMsg->ulCallId;
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT RCV:SIPCC_CALLEE_PICK_UP ");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_PICK_UP, 0, "");
    	//===========add by hb===========
    	//ATTestOK(AT_TEST_OBJECT_LP);
    	//===============================
        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Remove CallWait Timer");
        //CCSMCalleePickUp();																//CCSM_CALLEE_PICK_UP
        CCABCalleePickUp();
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCABCalleePickUp ");

        g_CCFsmState = CCFSM_PLAY_MUSIC;
        LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:PLAY_MUSIC");
    }

//	if(*(( unsigned int *)pvPrmvType) == SIPCC_RECV_EARLY_MEDIA)
//	{
//		LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:SIPCC_CALLEE_ERR ");
//		SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;
//
//		iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_TEL1_CALL_WAIT);
//		switch(iRet)
//		{
//		case TEL1_Y_OTHR_N:
//			CCSIPTerminateAll();
//			YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
//			CCSMCalleeErr(pstRcvMsg->uiErrorCause);
//			g_CCFsmState = CCFSM_IDLE;
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:CCFSM_IDLE");
//
//			break;
//		case TEL1_Y_SIP2_Y:
//			CCSIPTerminateAll();
//			CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2),                   //获取SIP2号码及相关参数
//										  CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP2),
//										  CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL
//
//			YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
//			uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数
//
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
//			g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
//														(void*) TIMERID_CALL_WAIT,
//														uiInterval, YK_TT_NONPERIODIC,
//														(unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Create CallWait Timer");
//
//			g_CCFsmState = CCFSM_SIP2_CALL_WAIT;
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:SIP2_CALL_WAIT");
//			break;
//		case TEL1_Y_SIP2_N_TEL2_Y:
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT SIP2 not exist TEL2 exist");
//			CCSIPTerminateAll();													//CCSIP_TERMINATE_ALL
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPTerminateAll");
//
//			CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL2),                   //获取TEL2号码及相关参数
//						  CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL2),
//						  CCGetHintMusicEn(g_CCFsmRoom));   				        //CCSIP_CALL
//
//			YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Remove CallWait Timer");
//
//			uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数
//
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
//			g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
//														(void*) TIMERID_CALL_WAIT,
//														uiInterval, YK_TT_NONPERIODIC,
//														(unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT Create CallWait Timer");
//
//			g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
//			LOG_RUNLOG_DEBUG("CC STATE:TEL1_CALL_WAIT -> STATE:TEL2_CALL_WAIT");
//			break;
//		default:
//			break;
//		}
//
//	}
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmSip2Process(void *pvPrmvType)
{
    int iRet;
    unsigned int uiInterval;
    LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT");
    //---------------------------------------------------------------------------------->>SIP2_CALL_WAIT
    if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
    {
        SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
        //无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
                    pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
        memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);


        iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法
        if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
        {
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT InComing Phone Num Valid");
            //CCSMOpenDoor(); 															//CCSM_OPENDOOR;
            CCABOpenDoor();
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCABOpenDoor");
            //===========add by hb===========
            //ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================

            LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================

         memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);

        g_CCFsmState = CCFSM_SIP2_CALL_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:SIP2_CALL_WAIT");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_MESSAGE_INCOMING)							//SIP_MESSAGE
	{
		SIPCC_MESSAGE_INCOMING_ST *pstRcvMsg = (SIPCC_MESSAGE_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:SIPCC_MESSAGE_INCOMING aucPhoneNum = %s, aucUserName = %s,uiMsgSeq = %ld",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->aucUserName,	pstRcvMsg->uiMsgSeq);

		unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
		memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

		iRet = VideoIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);	//判断号码是否合法

		if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
		{
			LOG_RUNLOG_DEBUG("CC STATE:IDLE InComing Phone Num Valid");
			CCABOpenDoor(); 															//CCSM_OPENDOOR;
			LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSMOpenDoor");
			//===========add by hb===========
		//	ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
			LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, 0);
			//写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");

		}
		else
		{
		//	ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, -1);
		}
		memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);
        g_CCFsmState = CCFSM_SIP2_CALL_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:SIP2_CALL_WAIT");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
    {
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;

        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
                pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);
        g_CCFsmState = CCFSM_SIP2_CALL_WAIT;
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:SIP2_CALL_WAIT");
    }

    //---------------------------------------------------------------------------------->>To IDLE
    if( *(( unsigned int *)pvPrmvType) == ABCC_DOOR_HANG_OFF )
    {
    	sleep(1);
		#ifdef TM_TAKE_PIC
    	InitSipCallFlagAndSocketConnectFlag();
    	YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
		#endif
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SMCC_DOOR_HANG_OFF OR SMCC_INDOOR_PICK_UP");
        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//        PICTimeOut();
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminateAll");

        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT Remove CallWait Timer");
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:IDLE");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    }
//    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_HANG_OFF)
//     {
//         SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;
//         LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SIPCC_CALLEE_HANG_OFF");
//         //-------------------edit by zlin
//         if(ulCallId == pstRcvMsg->ulCallId)
//         {
//             CCSIPTerminate(pstRcvMsg->ulCallId);									//CCSIP_TERMINATE
//             LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminateAll");
//             //写入日志
//             LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_HAND_UP, 0, "");
//
//             //CCSMCalleeHangOff();															//CCSM_CALLEE_HANG_OFF
//             CCABCalleeHangOff();
//             LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSMCalleeHangOff");
//
//             CCRemoveTimer();
//
//             g_CCFsmState = CCFSM_IDLE;
//             LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:IDLE");
//         }
//         //-------------------
//
// 		//===========add by hb===========
// 		//ATSendTestResult();
// 		//===============================
//     }
	if(*(( unsigned int *)pvPrmvType) == SIPCC_PROCESS)
	{
		SIPCC_PROCESS_ST *pstRcvMsg = (SIPCC_PROCESS_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SIPCC_PROCESS id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
	if(*(( unsigned int *)pvPrmvType) == SIPCC_RING)
	{
		SIPCC_RING_ST *pstRcvMsg = (SIPCC_RING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SIPCC_RING id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
//    if( *(( unsigned int *)pvPrmvType) == SMCC_INDOOR_PICK_UP )
//    {
//		#ifdef TM_TAKE_PIC
//		InitSipCallFlagAndSocketConnectFlag();
//	    YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
//		#endif
//        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SMCC_INDOOR_PICK_UP");
//        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminateAll");
//
//        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
//        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT Remove CallWait Timer");
//
//        g_CCFsmState = CCFSM_IDLE;
//        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:IDLE");
//        //写入日志
//        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");
//    }

    if(*(( unsigned int *)pvPrmvType) == CCCC_TIMEROUT_EVENT)							//超时事件
    {
		#ifdef TM_TAKE_PIC
	  	InitSipCallFlagAndSocketConnectFlag();
	  	YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
		#endif
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ");
        CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

        if(pstRcvMsg->uiTimerId == TIMERID_CALL_WAIT)									//呼叫等待超时
        {
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
            //写入日志
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, ERR_TIMEOUT_CALL_WAIT);
            iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_SIP2_CALL_WAIT);
            switch(iRet)
            {
            case SIP2_Y_TEL2_N:                                                     //TEL2不存在
                LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT SIP2 exist but others not exist");
                CCSIPTerminateAll();												//CCSIP_TERMINATE_ALL
                LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminateAll");

                //CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT);								//CCSM_CALLEE_ERR(超时)
                CCABCalleeErr(ERR_TIMEOUT_CALL_WAIT);
                LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT)");
                g_CCFsmState = CCFSM_IDLE;
                LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:IDLE");
                //写入日志
                LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, ERR_TIMEOUT_CALL_WAIT);
                break;
            case SIP2_Y_TEL2_Y:                                                     //TEL2存在
                LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT TEL2 exist");
                CCSIPTerminateAll();
                LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminateAll");
                CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL2),               //获取TEL2号码及相关参数
                                CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL2),
                                CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL

                uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);

                g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                                    (void*) TIMERID_CALL_WAIT,
                                                                                    uiInterval, YK_TT_NONPERIODIC,
                                                                                    (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT Create CallWait Timer");

                g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
                LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:TEL2_CALL_WAIT");
                break;
            default:
                break;
            }
        }
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_ERR)								//被叫方异常
    {
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SIPCC_CALLEE_ERR ");
        SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;

        //写入日志
        char errorCause[20] = {0x0};
        sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);
        if (CCSipCallReject(pstRcvMsg->uiErrorCause) == 1)          // 拒接modify by cs 2013-04-23
        {
            iRet = SIP2_Y_TEL2_N;
        }
        else
        {
            iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_SIP2_CALL_WAIT);
        }
        switch(iRet)
        {
        case SIP2_Y_TEL2_N:                                                         //TEL2不存在
	#ifdef TM_TAKE_PIC
		InitSipCallFlagAndSocketConnectFlag();
	#endif
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT SIP2 exist but others not exist");
            CCSIPTerminate(ulCurErrId);												//CCSIP_TERMINATE_ALL
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminateAll");
            //CCSMCalleeErr(pstRcvMsg->uiErrorCause);									//CCSM_CALLEE_ERR(原因)
            //CCSMCalleeErr(pstRcvMsg->uiErrorCause);
            CCABCalleeErr(pstRcvMsg->uiErrorCause);
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSMCalleeErr(%d)",pstRcvMsg->uiErrorCause);
            YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT Remove CallWait Timer");

            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:IDLE");
            break;
        case SIP2_Y_TEL2_Y:                                                         //TEL2存在
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT TEL2 exist");

    		#ifdef TM_TAKE_PIC
            CCDealTakePicture(TEL2);
    		#else
			CCSIPTerminate(ulCurErrId);
			LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminateAll");
            CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL2),                   //获取TEL2号码及相关参数
                            CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL2),
                            CCGetHintMusicEn(g_CCFsmRoom));                               //CCSIP_CALL

            YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT Remove CallWait Timer");

            uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
            g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
                                                                                (void*) TIMERID_CALL_WAIT,
                                                                                uiInterval, YK_TT_NONPERIODIC,
                                                                                (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT Create CallWait Timer");
            g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
            LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:TEL2_CALL_WAIT");
    #endif
            break;
        default:
            break;
        }
    }
    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_PICK_UP)
    {
#ifdef TM_TAKE_PIC
	  InitSipCallFlagAndSocketConnectFlag();
	  YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
#endif
        SIPCC_CALLEE_PICK_UP_ST *pstRcvMsg = (SIPCC_CALLEE_PICK_UP_ST *)pvPrmvType;
        ulCallId = pstRcvMsg->ulCallId;
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SIPCC_CALLEE_PICK_UP ");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_PICK_UP, 0, "");
    	//===========add by hb===========
    	//ATTestOK(AT_TEST_OBJECT_LP);
    	//===============================
        YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT Remove CallWait Timer");
        //CCSMCalleePickUp();																//CCSM_CALLEE_PICK_UP
        CCABCalleePickUp();
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSMCalleePickUp");

        g_CCFsmState = CCFSM_PLAY_MUSIC;
        LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:PLAY_MUSIC");
    }

	if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_SEND_DTMF)
	{
		LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SIPCC_CALLEE_SEND_DTMF");
		SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

		LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSMSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);
		if(pstRcvMsg->iDtmfType == '#')
		{
			CCABOpenDoor();
			//写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");
		}

		LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:SIP2_CALL_WAIT");
	}

	if(*(( unsigned int *)pvPrmvType) == SIPCC_STOP_CALLING)
	{
		LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT RCV:SIPCC_STOP_CALLING");
		CCABCalleeHangOff();
		LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSMCalleeHangOff");
		CCRemoveTimer();
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:IDLE");
	}

#ifdef TM_TAKE_PIC
	  DealTakePicState(*(( unsigned int *)pvPrmvType),TEL2);
#endif
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmTel2Process(void *pvPrmvType)
{
	int iRet;

	LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT");
	//---------------------------------------------------------------------------------->>TEL2_CALL_WAIT
	if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
	{
		SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
																						//无论号码是否合法均拒接
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
				pstRcvMsg->aucPhoneNum,
				pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

	   unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
		memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

		iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法

		if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
		{
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
			LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT InComing Phone Num Valid");
			//CCSMOpenDoor(); 															//CCSM_OPENDOOR;
			CCABOpenDoor();
			LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCABOpenDoor");
            //===========add by hb===========
			//ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================

		memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);


       	g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
       	LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT -> STATE:TEL2_CALL_WAIT");
    }
	if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
	{
		SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;

		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
				pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

		g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT -> STATE:TEL2_CALL_WAIT");
	}

	//---------------------------------------------------------------------------------->>To IDLE
	if( *(( unsigned int *)pvPrmvType) ==  ABCC_DOOR_HANG_OFF )
	{
		sleep(1);
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT RCV:SMCC_DOOR_HANG_OFF");
	   	CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//	   	PICTimeOut();
	   	LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCSIPTerminateAll");

		YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT Remove CallWait Timer");

	    g_CCFsmState = CCFSM_IDLE;
	    LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT -> STATE:IDLE");
	    //写入日志
	    LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
		LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
	}
//	if( *(( unsigned int *)pvPrmvType) == SMCC_INDOOR_PICK_UP )
//	{
//		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT RCV:SMCC_INDOOR_PICK_UP");
//	   	CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//	   	LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCSIPTerminateAll");
//
//		YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
//		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT Remove CallWait Timer");
//
//	    g_CCFsmState = CCFSM_IDLE;
//	    LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT -> STATE:IDLE");
//	    //写入日志
//	    LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");
//	}

	if(*(( unsigned int *)pvPrmvType) == CCCC_TIMEROUT_EVENT)							//超时事件
	{
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ");
		CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

    	if(pstRcvMsg->uiTimerId == TIMERID_CALL_WAIT)									//呼叫等待超时
    	{
    		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
			CCSIPTerminateAll();														//CCSIP_TERMINATE_ALL
			LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCSIPTerminateAll");

			//CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT);										//CCSM_CALLEE_ERR(超时)
			CCABCalleeErr(ERR_TIMEOUT_CALL_WAIT);
			LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT)");

			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT -> STATE:IDLE");

		    //写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, ERR_TIMEOUT_CALL_WAIT);
			LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
		}

		//===========add by hb===========
		//ATSendTestResult();
		//===============================
	}

	if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_ERR)								//被叫方异常
	{
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT RCV:SIPCC_CALLEE_ERR ");
		SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;

		//写入日志
		char errorCause[20] = {0x0};
		sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);

		CCSIPTerminate(ulCurErrId);															//CCSIP_TERMINATE_ALL
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCSIPTerminateAll");

		//CCSMCalleeErr(pstRcvMsg->uiErrorCause);											//CCSM_CALLEE_ERR
		CCABCalleeErr(pstRcvMsg->uiErrorCause);
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCABCalleeErr(%d)",pstRcvMsg->uiErrorCause);

		YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT Remove CallWait Timer");

		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT -> STATE:IDLE");

		//===========add by hb===========
		//ATSendTestResult();
		//===============================
	}

	if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_PICK_UP)
	{
    	SIPCC_CALLEE_PICK_UP_ST *pstRcvMsg = (SIPCC_CALLEE_PICK_UP_ST *)pvPrmvType;
        ulCallId = pstRcvMsg->ulCallId;
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT RCV:SIPCC_CALLEE_PICK_UP ");
	       //写入日志
	       LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_PICK_UP, 0, "");
       	//===========add by hb===========
       	//ATTestOK(AT_TEST_OBJECT_LP);
       	//===============================
		YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT Remove CallWait Timer");

		//CCSMCalleePickUp();																//CCSM_CALLEE_PICK_UP
		CCABCalleePickUp();
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT CCABCalleePickUp");

		g_CCFsmState = CCFSM_PLAY_MUSIC;
		LOG_RUNLOG_DEBUG("CC STATE:TEL2_CALL_WAIT -> STATE:PLAY_MUSIC");
	}
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmPlayMusicProcess(void *pvPrmvType)
{
    int iRet;
    int uiInterval;

    LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC");
    //---------------------------------------------------------------------------------->>To PLAY_MUSIC
    if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
    {
        SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
																						//无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
                pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);


        unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
        memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

        iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法

        if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
        {
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
            LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC InComing Phone Num Valid");
            //CCSMOpenDoor(); 															//CCSM_OPENDOOR;
            CCABOpenDoor();
            LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCABOpenDoor");
            //===========add by hb===========
            //ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================

            LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================

        memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);

        g_CCFsmState = CCFSM_PLAY_MUSIC;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:PLAY_MUSIC");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
    {
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;

        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
        pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        g_CCFsmState = CCFSM_PLAY_MUSIC;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:PLAY_MUSIC");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_SEND_DTMF)
    {
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_CALLEE_SEND_DTMF");
        SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

        //CCSMSendDTMF(pstRcvMsg->iDtmfType);
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSMSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);

        if(pstRcvMsg->iDtmfType == '#')
        {
        	//写入日志
        	CCABOpenDoor();
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");
        }

        g_CCFsmState = CCFSM_PLAY_MUSIC;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:PLAY_MUSIC");
    }

    //--------------------------------------------------------------------------------->>To IDLE
    if(*(( unsigned int *)pvPrmvType) == ABCC_DOOR_HANG_OFF)
    {
    	sleep(1);
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:ABCC_DOOR_HANG_OFF");
        CCSIPTerminateAll();
//        PICTimeOut();
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSIPTerminateAll");

        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:IDLE");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_HANG_OFF)
    {
        SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_CALLEE_HANG_OFF");
        //----------------edit by zlin
        if(ulCallId == pstRcvMsg->ulCallId)
        {
            CCSIPTerminate(pstRcvMsg->ulCallId);									//CCSIP_TERMINATE
            LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSIPTerminateAll");
            //写入日志
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_HAND_UP, 0, "");

            //CCSMCalleeHangOff();															//CCSM_CALLEE_HANG_OFF
            CCABCalleeHangOff();
            LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSMCalleeHangOff");

            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:IDLE");
        }
        //-----------------
    }
	if(*(( unsigned int *)pvPrmvType) == SIPCC_PROCESS)
	{
		SIPCC_PROCESS_ST *pstRcvMsg = (SIPCC_PROCESS_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_PROCESS id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
	if(*(( unsigned int *)pvPrmvType) == SIPCC_RING)
	{
		SIPCC_RING_ST *pstRcvMsg = (SIPCC_RING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_RING id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_ERR)
    {
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_CALLEE_ERR ");
        SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;

        //写入日志
        char errorCause[20] = {0x0};
        sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);

        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSIPTerminateAll");

        //CCSMCalleeErr(pstRcvMsg->uiErrorCause);											//CCSM_CALLEE_ERR
        //CCSMCalleeErr(pstRcvMsg->uiErrorCause);
        CCABCalleeErr(pstRcvMsg->uiErrorCause);
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSMCalleeErr(%d)",pstRcvMsg->uiErrorCause);

        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:IDLE");
    }

    //---------------------------------------------------------------------------------->>To TALKING
    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_MUSIC_END)
    {
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC RCV:SIPCC_CALLEE_MUSIC_END ");
        //CCSMCalleeMusicEnd();															//CCSM_CALLEE_MUSIC_END
        CCABCalleeMusicEnd();
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC CCSMCalleeMusicEnd");

        g_CCFsmState = CCFSM_TALKING;
        LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:TALKING");

        uiInterval = TMCCGetIMSCallTimeOut() * 1000;			//获取通话等待时间参数
        LOG_RUNLOG_DEBUG("CC STATE:CALLEE_MUSIC_END CCSIPCall Interval=%dMs",uiInterval);
        g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                            (void*) TIMERID_TALK_WAIT,
                                                                            uiInterval, YK_TT_NONPERIODIC,
                                                                            (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
        LOG_RUNLOG_DEBUG("CC STATE:IDLE Create TalkWait Timer");
    }
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmTalkingProcess(void *pvPrmvType)
{
    int iRet;

    LOG_RUNLOG_DEBUG("CC STATE:TALKING");
    //---------------------------------------------------------------------------------->>To TALKING
    if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
    {
        SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
																						//无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
                pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);										//CCSIP_TERMINATE;
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);


        unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
        memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);


        iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法

        if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
        {
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
            LOG_RUNLOG_DEBUG("CC STATE:TALKING InComing Phone Num Valid");
            //CCSMOpenDoor(); 															//CCSM_OPENDOOR;
            CCABOpenDoor();
            LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABOpenDoor");
            //===========add by hb===========
            //ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================

            LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================

        memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);

        g_CCFsmState = CCFSM_TALKING;
        LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_MESSAGE_INCOMING)							//SIP_MESSAGE
	{
		SIPCC_MESSAGE_INCOMING_ST *pstRcvMsg = (SIPCC_MESSAGE_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:SIPCC_MESSAGE_INCOMING aucPhoneNum = %s, aucUserName = %s,uiMsgSeq = %ld",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->aucUserName,	pstRcvMsg->uiMsgSeq);

		unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
		memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

		iRet = VideoIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);	//判断号码是否合法

		if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
		{
			LOG_RUNLOG_DEBUG("CC STATE:IDLE InComing Phone Num Valid");
			CCABOpenDoor();														//CCSM_OPENDOOR;
			LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSMOpenDoor");
			//===========add by hb===========
			ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
			LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, 0);
			//写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
		}
		else
		{
//    			ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, -1);
		}
		memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);
		g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");
	}

    if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
    {
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;

        LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
                pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        g_CCFsmState = CCFSM_TALKING;
        LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_SEND_DTMF)
    {
    	LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_CALLEE_SEND_DTMF");
    	SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

    	//????????????????????????????????
        //CCSMSendDTMF(pstRcvMsg->iDtmfType);
        //CCABSendDTMF(pstRcvMsg->iDtmfType);
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCABSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);
        if(pstRcvMsg->iDtmfType == '#')
        {
        	//写入日志
        	CCABOpenDoor();
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");
        }
	    g_CCFsmState = CCFSM_TALKING;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:TALKING");
    }

    //---------------------------------------------------------------------------------->>To IDLE
    if(*(( unsigned int *)pvPrmvType) == ABCC_DOOR_HANG_OFF)
    {
    	sleep(1);
        LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:ABCC_DOOR_HANG_OFF");
        CCSIPTerminateAll();
//        PICTimeOut();
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminateAll");

        CCRemoveTimer();

        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:IDLE");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_HANG_OFF)
    {
        SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;
        LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_CALLEE_HANG_OFF");
        //-------------------edit by zlin
        if(ulCallId == pstRcvMsg->ulCallId)
        {
            CCSIPTerminate(pstRcvMsg->ulCallId);									//CCSIP_TERMINATE
            LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminateAll");
            //写入日志
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_HAND_UP, 0, "");

            //CCSMCalleeHangOff();															//CCSM_CALLEE_HANG_OFF
            CCABCalleeHangOff();
            LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSMCalleeHangOff");

            CCRemoveTimer();

            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:IDLE");
        }
        //-------------------

		//===========add by hb===========
		//ATSendTestResult();
		//===============================
    }

    if(*(( unsigned int *)pvPrmvType) == CCCC_TIMEROUT_EVENT)							//超时事件
    {
        LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ");
        CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

        if(pstRcvMsg->uiTimerId == TIMERID_TALK_WAIT)									//通话超时
        {
            //写入日志
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_COMM_TIMEOUT, 0, "");
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
            CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
            LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminateAll");
            //CCSMCalleeHangOff();															//CCSM_CALLEE_HANG_OFF
            CCABCalleeHangOff();
            LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSMCalleeHangOff");

            CCRemoveTimer();

            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:IDLE");
        }

		//===========add by hb===========
		//ATSendTestResult();
		//===============================
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_ERR)
    {
        LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_CALLEE_ERR");
        SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;

        //写入日志
        char errorCause[20] = {0x0};
        sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);

        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSIPTerminateAll");

        CCRemoveTimer();

        //CCSMCalleeErr(pstRcvMsg->uiErrorCause);											//CCSM_CALLEE_ERR
        CCABCalleeErr(pstRcvMsg->uiErrorCause);
        LOG_RUNLOG_DEBUG("CC STATE:TALKING CCSMCalleeErr(%d)",pstRcvMsg->uiErrorCause);

        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:TALKING -> STATE:IDLE");

		//===========add by hb===========
		//ATSendTestResult();
		//===============================
    }
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmVideoMonProcess(void *pvPrmvType)
{
    int 			iRet;
    unsigned int 	uiInterval;

    LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT");
    //---------------------------------------------------------------------------------->>To VIDEO_MONIT
    if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
    {
        SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
																						//无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);


        unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
        memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

        iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法

        if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
        {
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT InComing Phone Num Valid");
            //CCSMOpenDoor(); 															//CCSM_OPENDOOR;
            CCABOpenDoor();
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCABOpenDoor");
            //===========add by hb===========
            //ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
            LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================

    	memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);


        g_CCFsmState = CCFSM_VIDEO_MONIT;
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_MESSAGE_INCOMING)							//SIP_MESSAGE
	{
		SIPCC_MESSAGE_INCOMING_ST *pstRcvMsg = (SIPCC_MESSAGE_INCOMING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:IDLE RCV:SIPCC_MESSAGE_INCOMING aucPhoneNum = %s, aucUserName = %s,uiMsgSeq = %ld",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->aucUserName,	pstRcvMsg->uiMsgSeq);

		unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
		memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

		iRet = VideoIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);	//判断号码是否合法

		if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
		{
			LOG_RUNLOG_DEBUG("CC STATE:IDLE InComing Phone Num Valid");
			CCABOpenDoor();														//CCSM_OPENDOOR;
			LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSMOpenDoor");
			//===========add by hb===========
			ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
			LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, 0);
			//写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
		}
		else
		{
//    			ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPMsgSendDoorOpenResp(pstRcvMsg->aucUserName, pstRcvMsg->uiMsgSeq, -1);
		}
		memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);
		g_CCFsmState = CCFSM_VIDEO_MONIT;
		LOG_RUNLOG_DEBUG("CC STATE:CCFSM_VIDEO_MONIT -> STATE:CCFSM_VIDEO_MONIT");
	}

    if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
    {
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;

        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
                pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        g_CCFsmState = CCFSM_VIDEO_MONIT;
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_SEND_DTMF)
    {
    	LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_CALLEE_SEND_DTMF");
    	SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

        // ------------------modify by cs 20120926
        if (pstRcvMsg->iDtmfType == '#')
        {
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");

            //---------------edit by zlin
//#ifdef _YK_XT8126_AV_
//            CCSMOpenDoor();
//#else
//        	CCSMSendDTMF(pstRcvMsg->iDtmfType);
//#endif
        	CCABOpenDoor();
            //---------------
        }
        else
        {
            //CCSMSendDTMF(pstRcvMsg->iDtmfType);
        }
        // ------------------end modify
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSMSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);
        g_CCFsmState = CCFSM_VIDEO_MONIT;
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
    }

    //---------------------------------------------------------------------------------->>To IDLE
    if( *(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_HANG_OFF )
    {
        SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SIPCC_CALLEE_HANG_OFF,CallId = %ld", pstRcvMsg->ulCallId);
        //-------------------edit by zlin
        if(ulCallId == pstRcvMsg->ulCallId)
        {
            CCSIPTerminate(pstRcvMsg->ulCallId);									//CCSIP_TERMINATE
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminateAll");
            //写入日志
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_CLIENT_MONITOR_END, 0, "");
            //CCSMVideoMonCancel();
            CCABVideoMonCancel();
            ulCallId = 0;
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSMVideoMonCancel");
            CCRemoveTimer();
            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:IDLE");
        }
        //--------------------
    }

    if(*(( unsigned int *)pvPrmvType) == CCCC_TIMEROUT_EVENT)							//超时事件
    {
        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:CCCC_TIMEROUT_EVENT ");
        CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

        if(pstRcvMsg->uiTimerId == TIMERID_TALK_WAIT)									//呼叫等待超时
        {
        	//写入日志
        	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_CLIENT_MONITOR_TIMEOUT, 0, "");

            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
            CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminateAll");
            //CCSMVideoMonCancel();
            CCABVideoMonCancel();
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSMVideoMonCancel");
            CCRemoveTimer();
            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:IDLE");
        }
    }
//    //---------------------------------------------------------------------------------->>To IDLE
//    if(*(( unsigned int *)pvPrmvType) == SMCC_DOOR_DET_CANCEL)
//    {
//        //写入日志
//        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_CLIENT_MONITOR_END, 0, "");
//
//        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SMCC_DOOR_DET_CANCEL");
//        CCSIPTerminateAll();
//        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminateAll");
//
//        g_CCFsmState = CCFSM_IDLE;
//        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:IDLE");
//        CCRemoveTimer();
//    }

    if(*(( unsigned int *)pvPrmvType) == ABCC_DOOR_CALL)
    {
        //写入日志
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);

        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT RCV:SMCC_DOOR_CALL");
        SMCC_DOOR_CALL_ST *pstRcvMsg = (SMCC_DOOR_CALL_ST *)pvPrmvType;

        g_CCFsmRoom[0] = pstRcvMsg->ucDevType;
        strcpy(g_CCFsmRoom + 1,pstRcvMsg->aucRoomNum);									//记录合法的房号

        //写入日志
    	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
        if(TMCCIsIMSCallEnabled() == 1)                                                                            //呼叫使能
        {
        	if(TMCCGetIMSPhoneNumber(g_CCFsmRoom) == NULL)
			{
				LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT PRIMARY NUM not exist");
			}
			else
			{
				LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT PRIMARY NUM exist");
				CCSIPTerminateAll();
				LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminateAll");

				CCSIPCall(TMCCGetIMSPhoneNumber(g_CCFsmRoom), MEDIA_TYPE_AUDIO_VIDEO, HINT_MUSIC_DISABLE);
//				sleep(1);	//防止短时间内发出呼叫和挂断指令造成服务器无法及时处理

				LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPCall Interval=%dMs",PRIMARY_CALL_RINGING_TIME);
				g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
														  (void*) TIMERID_CALL_WAIT,
														  PRIMARY_CALL_RINGING_TIME, YK_TT_NONPERIODIC,
														  (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
				LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Create CallWait Timer");
				g_CCFsmState = CCFSM_PRIMARY_CALL_WAIT;
				LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:PRIMARY_CALL_WAIT");
				CCABCalleeErrRoomValid();
				return ;
			}

        	iRet = IsRoomPhoneNumExist(g_CCFsmRoom,ROOM_NUM_LEN);

            if(iRet == ROOM_PHONE_NUM_Y)													//存在房号对应的电话号码
            {
                LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Room Phone Num exist");
                CCABCalleeErrRoomValid();
                LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPTerminateAll");
                CCSIPTerminateAll();
                CCRemoveTimer();

                uiInterval = CCGetTime4DelayCall(g_CCFsmRoom,ROOM_NUM_LEN);					//根据房号提取延时呼叫参数,毫秒单位
                LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCGetTime4DelayCall Interval=%dMs",uiInterval);
                if(uiInterval == 0)
                {
                    LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT DelayCall Interval = 0,Omit STATE IDLE , DELAY");
                    iRet  = VerifyRoomNum(g_CCFsmRoom,CCFSM_VIDEO_MONIT);
                    switch(iRet)
                    {
                    case SIP1_Y:													    //SIP1号码存在
                        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT SIP1 exist");
                        //CCSIPTerminateAll();
                        CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP1),           //获取电话号码等参数
                                        CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP1),
                                        CCGetHintMusicEn(g_CCFsmRoom));

                        uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);	    //获取呼叫等待时间参数
                        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPCall Interval=%dMs",uiInterval);

                        g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,		//start timer2 for calling
                                                                   (void*) TIMERID_CALL_WAIT,
                                                                   uiInterval, YK_TT_NONPERIODIC,
                                                                   (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Create CallWait Timer");

                        g_CCFsmState = CCFSM_SIP1_CALL_WAIT;
                        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:SIP1_CALL_WAIT");

                        break;
                    case SIP1_N_TEL1_Y:                                                     //SIP1号码不存在,TEL1号码存在
                        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT TEL1 exist");
                        //CCSIPTerminateAll();
                        CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL1),               //获取电话号码等参数
                                        CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL1),
                                        CCGetHintMusicEn(g_CCFsmRoom));

                        uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT CCSIPCall Interval=%dMs",uiInterval);

                        g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                   (void*) TIMERID_CALL_WAIT,
                                                                   uiInterval, YK_TT_NONPERIODIC,
                                                                   (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Create CallWait Timer");

                        g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
                        LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:TEL1_CALL_WAIT");
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    g_CCDelayCallTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer1
                                                                                        (void*) TIMERID_DELAY_CALL,
                                                                                        uiInterval, YK_TT_NONPERIODIC,
                                                                                        (unsigned long *)&g_CCDelayCallTimer.ulMagicNum);
                    LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT Create DelayCall Timer Interval=%dMs",uiInterval);
                    g_CCFsmState = CCFSM_DELAY;
                    LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:DELAY");
                }
            }
            else																			//不存在房号对应的电话号码
            {
                strcpy(g_CCFsmRoom, CC_NO_ROOM);
				LOG_RUNLOG_DEBUG("CC STATE:IDLE Room-Phone Num not exist");
				CCABCalleeErrRoomInvalid();

				g_CCFsmState = CCFSM_VIDEO_MONIT;
				LOG_RUNLOG_DEBUG("CC STATE:VIDEO_MONIT -> STATE:VIDEO_MONIT");
            }
        }
        //add----->by ytz
        //禁用SIP呼叫时，呼叫时通知SM呼叫错误
        else
        {
            strcpy(g_CCFsmRoom,LOG_EVENTLOG_NO_ROOM);
            LOG_RUNLOG_DEBUG("CC STATE:Ims Call Disable!");
            //CCSMCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);
            CCABCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);
            g_CCFsmState = CCFSM_VIDEO_MONIT;
            LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:IDLE");
        }
    }
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmDemoCallProcess(void *pvPrmvType)
{
    int 			iRet;
    unsigned int 	uiInterval;

    LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL");
    //---------------------------------------------------------------------------------->>To VIDEO_MONIT
    if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
    {
        SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
																						//无论号码是否合法均拒接
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
				pstRcvMsg->aucPhoneNum, pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
         memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);


        iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法

        if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
        {
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL InComing Phone Num Valid");
            //CCSMOpenDoor(); 															//CCSM_OPENDOOR;
            CCABOpenDoor();
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCABOpenDoor");
            //===========add by hb===========
            //ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================

            LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================


     	memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);



        g_CCFsmState = CCFSM_DEMO_CALL;
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:DEMO_CALL");
    }
    if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
    {
        SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;

        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
                pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
        CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

        g_CCFsmState = CCFSM_DEMO_CALL;
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:DEMO_CALL");
    }

    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_PICK_UP)
    {
    	SIPCC_CALLEE_PICK_UP_ST *pstRcvMsg = (SIPCC_CALLEE_PICK_UP_ST *)pvPrmvType;
		#ifdef TM_TAKE_PIC
	  	InitSipCallFlagAndSocketConnectFlag();
        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
		#endif
        ulCallId = pstRcvMsg->ulCallId;
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SIPCC_CALLEE_PICK_UP ");

        //CCSMCalleePickUp();																//CCSM_CALLEE_PICK_UP
        CCABCalleePickUp();
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSMCalleePickUp");

        g_CCFsmState = CCFSM_DEMO_CALL;
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:DEMO_CALL");

    }
    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_MUSIC_END)
    {
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SIPCC_CALLEE_MUSIC_END ");
        //CCSMCalleeMusicEnd();															//CCSM_CALLEE_MUSIC_END
        //CCSMVideoMonitor();
        CCABVideoMonitor();
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSMCalleeMusicEnd");

        g_CCFsmState = CCFSM_DEMO_CALL;
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:DEMO_CALL");
    }
    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_SEND_DTMF)
    {
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SIPCC_CALLEE_SEND_DTMF");
        SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

        if (pstRcvMsg->iDtmfType == '#')
        {
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");
        	CCABOpenDoor();
        }

        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSMSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);
        g_CCFsmState = CCFSM_DEMO_CALL;
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:DEMO_CALL");
    }

    //---------------------------------------------------------------------------------->>To IDLE
    if( *(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_HANG_OFF )
    {
        SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SIPCC_CALLEE_HANG_OFF,CallId = %ld,ulCallId = %ld ", pstRcvMsg->ulCallId, ulCallId);
		#ifdef TM_TAKE_PIC
	  	InitSipCallFlagAndSocketConnectFlag();
        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
		#endif

        //-------------------edit by zlin
        if(ulCallId == pstRcvMsg->ulCallId)
        {
            CCSIPTerminate(pstRcvMsg->ulCallId);									//CCSIP_TERMINATE
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPTerminateAll");
            //写入日志
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_HAND_UP, 0, "");
            //CCSMVideoMonCancel();
            CCABCalleeHangOff();
            ulCallId = 0;
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSMVideoMonCancel");
            CCRemoveTimer();
            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:IDLE");
        }
        //--------------------
    }
	if(*(( unsigned int *)pvPrmvType) == SIPCC_PROCESS)
	{
		SIPCC_PROCESS_ST *pstRcvMsg = (SIPCC_PROCESS_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SIPCC_PROCESS id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
	if(*(( unsigned int *)pvPrmvType) == SIPCC_RING)
	{
		SIPCC_RING_ST *pstRcvMsg = (SIPCC_RING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SIPCC_RING id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
    if(*(( unsigned int *)pvPrmvType) == CCCC_TIMEROUT_EVENT)							//超时事件
    {
        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:CCCC_TIMEROUT_EVENT ");
        CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;
		#ifdef TM_TAKE_PIC
	  	InitSipCallFlagAndSocketConnectFlag();
        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
		#endif

        if(pstRcvMsg->uiTimerId == TIMERID_TALK_WAIT)									//呼叫等待超时
        {
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
            CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPTerminateAll");
            //CCSMVideoMonCancel();
            CCABCalleeHangOff();
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSMVideoMonCancel");
            CCRemoveTimer();
            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:IDLE");
        }
    }
//    //---------------------------------------------------------------------------------->>To IDLE
//    if(*(( unsigned int *)pvPrmvType) == SMCC_DOOR_DET_CANCEL)
//    {
//		#ifdef TM_TAKE_PIC
//	  	InitSipCallFlagAndSocketConnectFlag();
//        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
//		#endif
//        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SMCC_DOOR_DET_CANCEL");
//        CCSIPTerminateAll();
//        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPTerminateAll");
//
//        g_CCFsmState = CCFSM_IDLE;
//        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:IDLE");
//        CCRemoveTimer();
//    }

    if(*(( unsigned int *)pvPrmvType) == SMCC_DOOR_CALL)
    {
        //写入日志
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);

        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL RCV:SMCC_DOOR_CALL");
        SMCC_DOOR_CALL_ST *pstRcvMsg = (SMCC_DOOR_CALL_ST *)pvPrmvType;
		#ifdef TM_TAKE_PIC
	  	InitSipCallFlagAndSocketConnectFlag();
        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
		#endif

        g_CCFsmRoom[0] = pstRcvMsg->ucDevType;
        strcpy(g_CCFsmRoom + 1,pstRcvMsg->aucRoomNum);									//记录合法的房号
    	//写入日志
    	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_START, 0, "");
        if(TMCCIsIMSCallEnabled() == 1)                                                                            //呼叫使能
        {
            iRet = IsRoomPhoneNumExist(g_CCFsmRoom,ROOM_NUM_LEN);

            if(iRet == ROOM_PHONE_NUM_Y)													//存在房号对应的电话号码
            {
                LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL Room Phone Num exist");

                CCSIPTerminateAll();
                CCRemoveTimer();

                LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPTerminateAll");
                uiInterval = CCGetTime4DelayCall(g_CCFsmRoom,ROOM_NUM_LEN);					//根据房号提取延时呼叫参数,毫秒单位
                LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCGetTime4DelayCall Interval=%dMs",uiInterval);
                if(uiInterval == 0)
                {
                    LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL DelayCall Interval = 0,Omit STATE IDLE , DELAY");
                    iRet  = VerifyRoomNum(g_CCFsmRoom,CCFSM_DEMO_CALL);
                    switch(iRet)
                    {
                    case SIP1_Y:													    //SIP1号码存在
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL SIP1 exist");
                        CCSIPTerminateAll();
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPTerminateAll");

                        CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP1),           //获取电话号码等参数
                                        CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_SIP1),
                                        CCGetHintMusicEn(g_CCFsmRoom));

                        uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);	    //获取呼叫等待时间参数
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPCall Interval=%dMs",uiInterval);

                        g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,		//start timer2 for calling
                                                                   (void*) TIMERID_CALL_WAIT,
                                                                   uiInterval, YK_TT_NONPERIODIC,
                                                                   (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL Create CallWait Timer");

                        g_CCFsmState = CCFSM_SIP1_CALL_WAIT;
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:SIP1_CALL_WAIT");

                        break;
                    case SIP1_N_TEL1_Y:                                                     //SIP1号码不存在,TEL1号码存在
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL TEL1 exist");
                        CCSIPTerminateAll();
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPTerminateAll");
                        CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL1),               //获取电话号码等参数
                                        CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL1),
                                        CCGetHintMusicEn(g_CCFsmRoom));

                        uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);			//获取呼叫等待时间参数
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSIPCall Interval=%dMs",uiInterval);

                        g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,			//start timer2 for calling
                                                                   (void*) TIMERID_CALL_WAIT,
                                                                   uiInterval, YK_TT_NONPERIODIC,
                                                                   (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL Create CallWait Timer");

                        g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
                        LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:TEL1_CALL_WAIT");
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    g_CCDelayCallTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer1
                                                                (void*) TIMERID_DELAY_CALL,
                                                                uiInterval, YK_TT_NONPERIODIC,
                                                                (unsigned long *)&g_CCDelayCallTimer.ulMagicNum);
                    LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL Create DelayCall Timer Interval=%dMs",uiInterval);
                    g_CCFsmState = CCFSM_DELAY;
                    LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:DELAY");
                }
            }
            else																			//不存在房号对应的电话号码
            {
                LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL Room Phone Num not exist");
                strcpy(g_CCFsmRoom,LOG_EVENTLOG_NO_ROOM);

                //CCSMCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);
                CCABCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);
                LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL CCSM CALL ERROR");

                g_CCFsmState = CCFSM_DEMO_CALL;
                LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:DEMO_CALL");
            }
        }
        //add----->by ytz
        //禁用SIP呼叫时，呼叫时通知SM呼叫错误
        else
        {
            strcpy(g_CCFsmRoom,LOG_EVENTLOG_NO_ROOM);
            LOG_RUNLOG_DEBUG("CC STATE:Ims Call Disable!");
            //CCSMCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);
            CCABCalleeErr(CALLEE_ERROR_PHONENUM_NOT_EXIST);
            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:CCFSM_IDLE");
        }
    }
    //未通知AB???????????
    if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_ERR)								//被叫方异常
    {
		LOG_RUNLOG_DEBUG("CC STATE:DEMOL CALL  RCV:SIPCC_CALLEE_ERR ");
		SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;
		//写入日志
		char errorCause[20] = {0x0};
		sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);
        	CCRemoveTimer();
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:DEMO_CALL -> STATE:CCFSM_IDLE");
    }
#ifdef TM_TAKE_PIC
    DealTakePicState(*(( unsigned int *)pvPrmvType) , DEMOL_CALL_TEL);//访客体验电路域
#endif
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCFsmPrimaryCallProcess(void *pvPrmvType)
{
	int iRet;

	LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT");
	//---------------------------------------------------------------------------------->>TEL2_CALL_WAIT
	if(*(( unsigned int *)pvPrmvType) == SIPCC_AUDIO_INCOMING)							//语音来电
	{
		SIPCC_AUDIO_INCOMING_ST *pstRcvMsg = (SIPCC_AUDIO_INCOMING_ST *)pvPrmvType;
																						//无论号码是否合法均拒接
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:SIPCC_AUDIO_INCOMING PhoneNum = %s,CallId = %ld",
				pstRcvMsg->aucPhoneNum,
				pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE;
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

	   unsigned char ccFsmRoomBak[ROOM_NUM_BUFF_LEN]= CC_NO_ROOM;
		memcpy(ccFsmRoomBak, g_CCFsmRoom, ROOM_NUM_BUFF_LEN);

		iRet = AudioIncomingVerifyPhoneNum(pstRcvMsg->aucPhoneNum,g_CCFsmRoom,PHONE_NUM_LEN);		//判断号码是否合法

		if((iRet == NUM_VALID) && (TMCCIsKeyOpenDoorEnabled() == 1))					//一键开门使能&&号码合法
		{
			//写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_ONEKEY_OPEN_DOOR, 0, "");
			LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT InComing Phone Num Valid");
			//CCSMOpenDoor(); 															//CCSM_OPENDOOR;
			CCABOpenDoor();
			LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCABOpenDoor");
			//===========add by hb===========
			//ATSendOneKeyOpenResult(1,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			//===============================
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, 0);
		}
		//===========add by hb===========
		else
		{
			//ATSendOneKeyOpenResult(0,pstRcvMsg->aucPhoneNum,g_CCFsmRoom);
			LPSendOpenDoorResp(pstRcvMsg->aucUserName, -1);
		}
		//===============================

		memcpy(g_CCFsmRoom, ccFsmRoomBak, ROOM_NUM_BUFF_LEN);


		g_CCFsmState = CCFSM_PRIMARY_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT -> STATE:PRIMARY_CALL_WAIT");
	}
	if(*(( unsigned int *)pvPrmvType) == SIPCC_VIDEO_INCOMING)							//视频来电
	{
		SIPCC_VIDEO_INCOMING_ST *pstRcvMsg = (SIPCC_VIDEO_INCOMING_ST *)pvPrmvType;

		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:SIPCC_VIDEO_INCOMING PhoneNum = %s,CallId = %ld",
				pstRcvMsg->aucPhoneNum,	pstRcvMsg->ulInComingCallId);
		CCSIPTerminate(pstRcvMsg->ulInComingCallId);									//CCSIP_TERMINATE
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSIPTerminate(Id=%ld)",pstRcvMsg->ulInComingCallId);

		g_CCFsmState = CCFSM_PRIMARY_CALL_WAIT;
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT -> STATE:PRIMARY_CALL_WAIT");
	}

//	//---------------------------------------------------------------------------------->>To IDLE
//	if( *(( unsigned int *)pvPrmvType) == SMCC_DOOR_HANG_OFF )
//	{
//		sleep(1);
//		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:SMCC_DOOR_HANG_OFF");
//		CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
////		PICTimeOut();
//		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSIPTerminateAll");
//
//		YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
//		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT Remove CallWait Timer");
//
//		g_CCFsmState = CCFSM_IDLE;
//		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT -> STATE:IDLE");
//		//写入日志
//		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
//		LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
//	}
//	if( *(( unsigned int *)pvPrmvType) == SMCC_INDOOR_PICK_UP )
//	{
//		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:SMCC_INDOOR_PICK_UP");
//		CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSIPTerminateAll");
//
//		YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
//		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT Remove CallWait Timer");
//
//		g_CCFsmState = CCFSM_IDLE;
//		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT -> STATE:IDLE");
//		//写入日志
//		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_INDOOR_PICK_UP, 0, "");
//	}
    //---------------------------------------------------------------------------------->>To IDLE
    if( *(( unsigned int *)pvPrmvType) == ABCC_DOOR_HANG_OFF)
    {
    	sleep(1);
        LOG_RUNLOG_DEBUG("CC STATE:DELAY RCV:ABCC_DOOR_HANG_OFF");
        CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
//        PICTimeOut();
        LOG_RUNLOG_DEBUG("CC STATE:DELAY CCSIPTerminateAll");
        YKRemoveTimer(g_CCDelayCallTimer.pstTimer, g_CCDelayCallTimer.ulMagicNum);		//Destroy the timer1 for delay call
#ifdef TM_TAKE_PIC
        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
#endif
        LOG_RUNLOG_DEBUG("CC STATE:DELAY Destroy DelayCall Timer");
        g_CCFsmState = CCFSM_IDLE;
        LOG_RUNLOG_DEBUG("CC STATE:DELAY ->STATE:IDLE ");
        //写入日志
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_OUT_DOOR_CALL_END, 0, "");
        LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
    }
	if(*(( unsigned int *)pvPrmvType) == CCCC_TIMEROUT_EVENT)							//超时事件
	{
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ");
		CCCC_TIMEROUT_EVENT_ST *pstRcvMsg = (CCCC_TIMEROUT_EVENT_ST *)pvPrmvType;

		if(pstRcvMsg->uiTimerId == TIMERID_CALL_WAIT)									//呼叫等待超时
		{
			LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:CCCC_TIMEROUT_EVENT ID=CallWait");
			CCSIPTerminateAll();														//CCSIP_TERMINATE_ALL
			LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSIPTerminateAll");

			//CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT);										//CCSM_CALLEE_ERR(超时)
			CCABCalleeErr(ERR_TIMEOUT_CALL_WAIT);
			LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSMCalleeErr(ERR_TIMEOUT_CALL_WAIT)");

			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT -> STATE:IDLE");

			//写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, ERR_TIMEOUT_CALL_WAIT);
			LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
		}

		//===========add by hb===========
		//ATSendTestResult();
		//===============================
	}

	if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_ERR)								//被叫方异常
	{
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:SIPCC_CALLEE_ERR ");
		SIPCC_CALLEE_ERR_ST *pstRcvMsg = (SIPCC_CALLEE_ERR_ST *)pvPrmvType;

		//写入日志
		char errorCause[20] = {0x0};
		sprintf(errorCause, "%d", pstRcvMsg->uiErrorCause);
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_FAIL, 0, errorCause);

		CCSIPTerminate(ulCurErrId);															//CCSIP_TERMINATE_ALL
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSIPTerminateAll");

		//CCSMCalleeErr(pstRcvMsg->uiErrorCause);											//CCSM_CALLEE_ERR
		CCABCalleeErr(pstRcvMsg->uiErrorCause);
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSMCalleeErr(%d)",pstRcvMsg->uiErrorCause);

		YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT Remove CallWait Timer");

		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT -> STATE:IDLE");

		//===========add by hb===========
		//ATSendTestResult();
		//===============================
	}

	if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_PICK_UP)
	{
		SIPCC_CALLEE_PICK_UP_ST *pstRcvMsg = (SIPCC_CALLEE_PICK_UP_ST *)pvPrmvType;
		ulCallId = pstRcvMsg->ulCallId;
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:SIPCC_CALLEE_PICK_UP ");
		   //写入日志
		   LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_PICK_UP, 0, "");
		//===========add by hb===========
		//ATTestOK(AT_TEST_OBJECT_LP);
		//===============================
		YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);		//destroy timer2
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT Remove CallWait Timer");

		//CCSMCalleePickUp();																//CCSM_CALLEE_PICK_UP
		CCABCalleePickUp();
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSMCalleePickUp");

		g_CCFsmState = CCFSM_PLAY_MUSIC;
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT -> STATE:PLAY_MUSIC");
	}
	if(*(( unsigned int *)pvPrmvType) == SIPCC_PROCESS)
	{
		SIPCC_PROCESS_ST *pstRcvMsg = (SIPCC_PROCESS_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:SIPCC_PROCESS id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
	if(*(( unsigned int *)pvPrmvType) == SIPCC_RING)
	{
		SIPCC_RING_ST *pstRcvMsg = (SIPCC_RING_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:SIPCC_RING id=%d", pstRcvMsg->ulCallId);
		ulCallId = pstRcvMsg->ulCallId;
	}
	if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_HANG_OFF)
	{
		SIPCC_CALLEE_HANG_OFF_ST *pstRcvMsg = (SIPCC_CALLEE_HANG_OFF_ST *)pvPrmvType;
		LOG_RUNLOG_DEBUG("CC STATE:TALKING RCV:SIPCC_CALLEE_HANG_OFF");
		//-------------------edit by zlin
		if(ulCallId == pstRcvMsg->ulCallId)
		{
			CCSIPTerminate(pstRcvMsg->ulCallId);									//CCSIP_TERMINATE
			LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSIPTerminateAll");
			//写入日志
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_HAND_UP, 0, "");

			CCSMCalleeHangOff();															//CCSM_CALLEE_HANG_OFF
			LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSMCalleeHangOff");

			CCRemoveTimer();

			g_CCFsmState = CCFSM_IDLE;
			LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT -> STATE:IDLE");
		}
	}
	if(*(( unsigned int *)pvPrmvType) == SIPCC_CALLEE_SEND_DTMF)
	{
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT RCV:SIPCC_CALLEE_SEND_DTMF");
		SIPCC_CALLEE_SEND_DTMF_ST *pstRcvMsg = (SIPCC_CALLEE_SEND_DTMF_ST *)pvPrmvType;

		//CCSMSendDTMF(pstRcvMsg->iDtmfType);
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSMSendDTMF,DTMF=%c",pstRcvMsg->iDtmfType);
		if(pstRcvMsg->iDtmfType == '#')
		{
			//写入日志
			CCABOpenDoor();
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_USERS_PHONE_OPEN_DOOR, 0, "");
		}
		CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSIPTerminateAll");
		//CCSMCalleeHangOff();
		//CCSMCalleeHangOff();
		CCABCalleeHangOff();
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT CCSMCalleeHangOff");
		CCRemoveTimer();
		g_CCFsmState = CCFSM_IDLE;
		LOG_RUNLOG_DEBUG("CC STATE:PRIMARY_CALL_WAIT -> STATE:IDLE");
	}
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
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
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
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
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCSIPCall(unsigned char *pucPhoneNum,unsigned char ucMediaType,unsigned char ucHintMusicEn)
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
	pstSendMsg->aucHintMusicEn = ucHintMusicEn;

	iErrCode = YKWriteQue( g_pstLPMsgQ , pstSendMsg,  0);

	//写入日志
	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_SIP_CALL_START, 0, pucPhoneNum);
	//ATTestOK(AT_TEST_OBJECT_CC);
}
/********************************************************************
*Function	:
*Input		:
*Output		:a
*Description:
*Others		:
*********************************************************************/
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

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCSMVideoMonitor(void)
{
	int iErrCode;

	CCSM_VEDIO_MONITOR_ST *pstSendMsg = (CCSM_VEDIO_MONITOR_ST *)malloc(sizeof(CCSM_VEDIO_MONITOR_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSM_VEDIO_MONITOR;

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);
}
void CCABVideoMonitor(void)
{
//	int iErrCode;
//
//	CCAB_VEDIO_MONITOR_ST *pstSendMsg = (CCAB_VEDIO_MONITOR_ST *)malloc(sizeof(CCAB_VEDIO_MONITOR_ST));
//
//	if(pstSendMsg == NULL)
//	{
//		return;
//	}
//
//	pstSendMsg->uiPrmvType = CCAB_VEDIO_MONITOR;
//
//	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
}



/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void	CCSMOpenDoor(void)
{
	int iErrCode;

	CCSM_OPENDOOR_ST *pstSendMsg = (CCSM_OPENDOOR_ST *)malloc(sizeof(CCSM_OPENDOOR_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSM_OPENDOOR;

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);

}
void	CCABOpenDoor(void)
{
	int iErrCode;

	CCAB_OPENDOOR_ST *pstSendMsg = (CCAB_OPENDOOR_ST *)malloc(sizeof(CCAB_OPENDOOR_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCAB_OPENDOOR;

	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);

}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCSMSendDTMF(int iDtmfType)
{
	int iErrCode;

	CCSM_CALLEE_SEND_DTMF_ST *pstSendMsg = (CCSM_CALLEE_SEND_DTMF_ST *)malloc(sizeof(CCSM_CALLEE_SEND_DTMF_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSM_CALLEE_SEND_DTMF;
	pstSendMsg->iDtmfType =  iDtmfType;

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void 	CCSMCalleeErr(unsigned int uiErrorCause)
{
	int iErrCode;

	CCSM_CALLEE_ERR_ST *pstSendMsg = (CCSM_CALLEE_ERR_ST *)malloc(sizeof(CCSM_CALLEE_ERR_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType   = CCSM_CALLEE_ERR;
	pstSendMsg->uiErrorCause = uiErrorCause;

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);
}
void 	CCABCalleeErr(unsigned int uiErrorCause)
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

void 	CCABCalleeErrRoomInvalid()
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

void 	CCABCalleeErrRoomValid()
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

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCSMCalleePickUp(void)
{
	int iErrCode;

	CCSM_CALLEE_PICK_UP_ST *pstSendMsg = (CCSM_CALLEE_PICK_UP_ST *)malloc(sizeof(CCSM_CALLEE_PICK_UP_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSM_CALLEE_PICK_UP;


	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);
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
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCSMCalleeHangOff(void)
{
	int iErrCode;

	CCSM_CALLEE_HANG_OFF_ST *pstSendMsg = (CCSM_CALLEE_HANG_OFF_ST *)malloc(sizeof(CCSM_CALLEE_HANG_OFF_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSM_CALLEE_HANG_OFF;

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);
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
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void CCSMCalleeMusicEnd(void)
{
	int iErrCode;

	CCSM_CALLEE_MUSIC_END_ST *pstSendMsg = (CCSM_CALLEE_MUSIC_END_ST *)malloc(sizeof(CCSM_CALLEE_MUSIC_END_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSM_CALLEE_MUSIC_END;

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);
}

void CCABCalleeMusicEnd(void)
{
//	int iErrCode;
//
//	CCSM_CALLEE_MUSIC_END_ST *pstSendMsg = (CCSM_CALLEE_MUSIC_END_ST *)malloc(sizeof(CCSM_CALLEE_MUSIC_END_ST));
//
//	if(pstSendMsg == NULL)
//	{
//		return;
//	}
//
//	pstSendMsg->uiPrmvType = CCSM_CALLEE_MUSIC_END;
//
//	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);
}


/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
CCSMVideoMonCancel()
{
	int iErrCode;

	CCSM_VEDIO_MONITOR_CANCEL_ST *pstSendMsg = (CCSM_VEDIO_MONITOR_CANCEL_ST *)malloc(sizeof(CCSM_VEDIO_MONITOR_CANCEL_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = CCSM_VEDIO_MONITOR_CANCEL;

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);
}

CCABVideoMonCancel()
{
//	int iErrCode;
//
//	CCSM_VEDIO_MONITOR_CANCEL_ST *pstSendMsg = (CCSM_VEDIO_MONITOR_CANCEL_ST *)malloc(sizeof(CCSM_VEDIO_MONITOR_CANCEL_ST));
//
//	if(pstSendMsg == NULL)
//	{
//		return;
//	}
//
//	pstSendMsg->uiPrmvType = CCSM_VEDIO_MONITOR_CANCEL;
//
//	iErrCode = YKWriteQue( g_pstSMMsgQ , pstSendMsg,  0);
}


/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
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

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
unsigned int CCGetTime4DelayCall(unsigned char *pucRoomNum,unsigned int uiRoomNumLen)
{
	YKHandle 		House;
	unsigned int	uiInterval;

	House 		= TMCCGetHouseInfoByHouseCode(pucRoomNum);
	uiInterval  = TMCCGetSipCallDelay(House);

	TMCCDestroyHouseInfo(House);

	return uiInterval*1000;
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
unsigned int CCGetTime4CallWait(unsigned char *pucRoomNum,unsigned int uiRoomNumLen)
{
    YKHandle 		House;
	unsigned int	uiInterval;

    House 		= TMCCGetHouseInfoByHouseCode(pucRoomNum);
	uiInterval  = TMCCGetRingTime(House);

	TMCCDestroyHouseInfo(House);

	return uiInterval*1000;
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
unsigned char *CCGetPhoneNum(unsigned char *pucRoomNum,unsigned char ucPhoneType)
{
	YKHandle 		House;
	YKHandle    	User1;
	YKHandle		User2;


	House = TMCCGetHouseInfoByHouseCode(pucRoomNum);
    User1 = TMCCGetPhoneInfoByIndex(House,0);
	User2 = TMCCGetPhoneInfoByIndex(House,1);
	switch(ucPhoneType)
	{
		case PHONE_TYPE_SIP1:
			strcpy(aucPhoneNum,TMCCGetSipPhoneNumber(User1));
			//pucPhoneNum = TMCCGetSipPhoneNumber(User1);
			break;
		case PHONE_TYPE_TEL1:
			strcpy(aucPhoneNum,TMCCGetCsPhoneNumber(User1));
			//pucPhoneNum = TMCCGetCsPhoneNumber(User1);
			break;
		case PHONE_TYPE_SIP2:
			strcpy(aucPhoneNum,TMCCGetSipPhoneNumber(User2));
			//pucPhoneNum = TMCCGetSipPhoneNumber(User2);
			break;
		case PHONE_TYPE_TEL2:
			strcpy(aucPhoneNum,TMCCGetCsPhoneNumber(User2));
			//pucPhoneNum = TMCCGetCsPhoneNumber(User2);
			break;
		default:
			break;
	}

	TMCCDestroyHouseInfo(House);

	return aucPhoneNum;
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
unsigned char CCGetMediaType(unsigned char *pucRoomNum,unsigned char ucPhoneType)
{
	YKHandle 		House;
	YKHandle    	User1;
	YKHandle		User2;

	unsigned char   ucMediaType;

	House = TMCCGetHouseInfoByHouseCode(pucRoomNum);
    User1 = TMCCGetPhoneInfoByIndex(House,0);
	User2 = TMCCGetPhoneInfoByIndex(House,1);

	switch(ucPhoneType)
	{
		case PHONE_TYPE_SIP1:
			ucMediaType = (TMCCIsVoipEnable(User1)== TRUE)? MEDIA_TYPE_AUDIO_VIDEO : MEDIA_TYPE_AUDIO;
			break;
		case PHONE_TYPE_TEL1:
			 ucMediaType = MEDIA_TYPE_AUDIO;
			break;
		case PHONE_TYPE_SIP2:
			 ucMediaType = (TMCCIsVoipEnable(User2)== TRUE)? MEDIA_TYPE_AUDIO_VIDEO : MEDIA_TYPE_AUDIO;
			break;
		case PHONE_TYPE_TEL2:
			 ucMediaType = MEDIA_TYPE_AUDIO;
			break;
		default:
			break;
	}

	TMCCDestroyHouseInfo(House);

	return ucMediaType;
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
unsigned char CCGetHintMusicEn(unsigned char *pucRoomNum)
{
    YKHandle 		House;
	unsigned char	ucHintMusicEn;

    House 		   = TMCCGetHouseInfoByHouseCode(pucRoomNum);
	ucHintMusicEn  = (TMCCIsInCommingCallNotice(House) == TRUE)?HINT_MUSIC_ENABLE : HINT_MUSIC_DISABLE;

	TMCCDestroyHouseInfo(House);

	return ucHintMusicEn;
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
int VerifyRoomNum(unsigned char *pucRoomNum,int iCCFsmState)
{
	YKHandle 		House;
	YKHandle    	User1;
	YKHandle		User2;

	unsigned int 	uiSIP1;
	unsigned int 	uiTEL1;
	unsigned int 	uiSIP2;
	unsigned int 	uiTEL2;

	int				iPhoneCount;
	int				iRet = 0;

	House 		= TMCCGetHouseInfoByHouseCode(pucRoomNum);
	iPhoneCount = TMCCGetPhoneCount(House);

	if(iPhoneCount >0)
	{
		switch(iPhoneCount)
		{
			case 1:
		        User1  = TMCCGetPhoneInfoByIndex(House,0);
		        if (TMCCIsVoipEnable(User1) == TRUE)
		        {
		        	uiSIP1 = (TMCCGetSipPhoneNumber(User1) == NULL)? FALSE:TRUE;
		        }
		        else
		        {
		        	uiSIP1 = FALSE;
		        }
		        uiTEL1 = (TMCCGetCsPhoneNumber (User1) == NULL)? FALSE:TRUE;
                uiSIP2 = FALSE;
                uiTEL2 = FALSE;

                break;
			case 2:
				User1  = TMCCGetPhoneInfoByIndex(House,0);
				User2  = TMCCGetPhoneInfoByIndex(House,1);

		        if (TMCCIsVoipEnable(User1) == TRUE)
		        {
		        	uiSIP1 = (TMCCGetSipPhoneNumber(User1) == NULL)? FALSE:TRUE;
		        }
		        else
		        {
		        	uiSIP1 = FALSE;
		        }
                uiTEL1 = (TMCCGetCsPhoneNumber (User1) == NULL)? FALSE:TRUE;

		        if (TMCCIsVoipEnable(User2) == TRUE)
		        {
		        	uiSIP2 = (TMCCGetSipPhoneNumber(User2) == NULL)? FALSE:TRUE;
		        }
		        else
		        {
		        	uiSIP2 = FALSE;
		        }
                uiTEL2 = (TMCCGetCsPhoneNumber (User2) == NULL)? FALSE:TRUE;

                break;
			default:
				break;
		}

		switch(iCCFsmState)
		{
		    case CCFSM_IDLE:
		    	if( uiSIP1 == TRUE )
		    	{
		    		iRet = SIP1_Y;                         //SIP1号码存在
		    	}
		    	if( (uiSIP1 == FALSE) && (uiTEL1 == TRUE))
		    	{
		    		iRet = SIP1_N_TEL1_Y;                  //SIP1号码不存在  TEL1号码存在
		    	}
		    	break;
			case CCFSM_DELAY:
				if( uiSIP1 == TRUE )
				{
					iRet = SIP1_Y;                         //SIP1号码存在
				}
				if( (uiSIP1 == FALSE) && (uiTEL1 == TRUE))
				{
					iRet = SIP1_N_TEL1_Y;                  //SIP1号码不存在  TEL1号码存在
				}
				break;
			case CCFSM_SIP1_CALL_WAIT:
				if( (uiSIP1 == TRUE) && (uiTEL1 == FALSE) && (uiSIP2 == FALSE) && (uiTEL2 == FALSE) )
				{
					iRet = SIP1_Y_OTHR_N ;                 //SIP1号码存在    其他号码不存在
				}
				if( (uiSIP1 == TRUE) && (uiTEL1 == TRUE ) )
				{
					iRet = SIP1_Y_TEL1_Y;                  //SIP1号码存在    TEL1号码存在
				}
				if( (uiSIP1 == TRUE) && (uiTEL1 == FALSE) && (uiSIP2 == TRUE ) )
				{
					iRet = SIP1_Y_TEL1_N_SIP2_Y;           //SIP1号码存在	TEL1号码不存在 SIP2存在
				}
				if( (uiSIP1 == TRUE) && (uiTEL1 == FALSE) && (uiSIP2 == FALSE) && (uiTEL2 == TRUE ) )
				{
					iRet = SIP1_Y_TEL1_N_SIP2_N_TEL2_Y;    //SIP1号码存在 TEL1号码不存在 SIP2不存在 TEL2存在
				}
				break;
			case CCFSM_TEL1_CALL_WAIT:
				if( (uiTEL1 == TRUE ) && (uiSIP2 == FALSE) && (uiTEL2 == FALSE) )
				{
					iRet = TEL1_Y_OTHR_N;                  //                TEL1号码存在      其他号码不存在
				}
				if( (uiTEL1 == TRUE ) && (uiSIP2 == TRUE ) )
				{
					iRet = TEL1_Y_SIP2_Y;                  //                TEL1号码存在      SIP2存在
				}
				if( (uiTEL1 == TRUE ) && (uiSIP2 == FALSE) && (uiTEL2 == TRUE ) )
				{
					iRet = TEL1_Y_SIP2_N_TEL2_Y;           //                TEL1号码存在      SIP2不存在   TEL2存在
				}
				break;
			case CCFSM_SIP2_CALL_WAIT:
				if((uiSIP2 == TRUE ) && (uiTEL2 == FALSE) )
				{
					iRet = SIP2_Y_TEL2_N;                  //								   SIP2存在     TEL2不存在
				}
				if(                                          (uiSIP2 == TRUE ) && (uiTEL2 == TRUE ) )
				{
					iRet = SIP2_Y_TEL2_Y;                  //								   SIP2存在	    TEL2存在
				}
				break;
			case CCFSM_VIDEO_MONIT:
			case CCFSM_DEMO_CALL:
			    if( uiSIP1 == TRUE )
				{
					iRet = SIP1_Y;                         //SIP1号码存在
				}
				if( (uiSIP1 == FALSE) && (uiTEL1 == TRUE))
				{
					iRet = SIP1_N_TEL1_Y;                  //SIP1号码不存在  TEL1号码存在
				}
				break;
			default:
				break;
		}
	}
	TMCCDestroyHouseInfo(House);

	return iRet;
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
int IsRoomPhoneNumExist(unsigned char *pucRoomNum,unsigned int uiRoomNumLen)
{
	YKHandle 	House;
	int			iPhoneCount;

	//LOG_RUNLOG_DEBUG("IsRoomPhoneNumExist %s", pucRoomNum);
	House 		= TMCCGetHouseInfoByHouseCode(pucRoomNum);
	iPhoneCount = TMCCGetPhoneCount(House);
	TMCCDestroyHouseInfo(House);

	if(iPhoneCount>0)
	{
		return ROOM_PHONE_NUM_Y;	//存在房号对应的电话号码
	}
	else
	{
		return ROOM_PHONE_NUM_N; 	//不存在房号对应的电话号码
	}
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:   先房间号码搜索后房客体验号码搜索
*Others		:
*********************************************************************/
int VideoIncomingVerifyPhoneNum(unsigned char *pucPhoneNum, char* pcHouseCode,unsigned int PhoneNumLen)
{

	if(TMCCIsValidPhoneNum(pucPhoneNum, pcHouseCode) == TRUE )		//房间号码搜索
    {
        return NUM_VALID;	//号码合法
    }
	if(TMCCIsValidDemoPhoneNum(pucPhoneNum, pcHouseCode) == TRUE )	//房客体验号码搜索
    {
        return NUM_VALID;	//号码合法
    }
    else
    {
        return NUM_INVALID;	//号码非法
    }
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
int AudioIncomingVerifyPhoneNum(unsigned char *pucPhoneNum,char* pcHouseCode, unsigned int PhoneNumLen)
{
	if(TMCCIsValidDemoPhoneNum(pucPhoneNum, pcHouseCode) == TRUE )
    {
        return NUM_VALID;	//号码合法
    }
	if(TMCCIsValidPhoneNum(pucPhoneNum,pcHouseCode) == TRUE )
    {
        return NUM_VALID;	//号码合法
    }
    else
    {
        return NUM_INVALID;	//号码非法
    }
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:接收消息
*Others		:
*********************************************************************/
void CCTimerCallBack(void *pCtx)
{
	int iErrCode;

	CCCC_TIMEROUT_EVENT_ST *pSendMsg = (CCCC_TIMEROUT_EVENT_ST *)malloc(sizeof(CCCC_TIMEROUT_EVENT_ST));
	if(pSendMsg == NULL)
	{
		return;
	}

	pSendMsg->uiPrmvType = CCCC_TIMEROUT_EVENT;
	pSendMsg->uiTimerId  =(int)(pCtx);

	iErrCode = YKWriteQue( g_pstCCMsgQ , pSendMsg,  0);
}
#ifdef TM_TAKE_PIC
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:图片上传接收消息
*Others		:
*********************************************************************/
void CCTimerPicResultCallBack(void *pCtx)
{
	int iErrCode;

	CCCC_TIMEROUT_EVENT_ST *pSendMsg = (CCCC_TIMEROUT_EVENT_ST *)malloc(sizeof(CCCC_TIMEROUT_EVENT_ST));
	if(pSendMsg == NULL)
	{
		return;
	}

	pSendMsg->uiPrmvType = CCCC_PIC_RESULT_TIMEOUT_EVENT;
	pSendMsg->uiTimerId  =(int)(pCtx);

	iErrCode = YKWriteQue( g_pstCCMsgQ , pSendMsg,  0);
}
/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:个推处理
*Others		:
*********************************************************************/
void CCDealTakePicture(unsigned char ucTelType)
{
    YKRemoveTimer(g_CCCallWaitTimer.pstTimer, g_CCCallWaitTimer.ulMagicNum);//destroy timer2
//    YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer2
    if (g_iSocketConnectFlag == SOCKET_CONNECT_OK)
    {
    	g_iSipCallFlag = SIP_CALL_ERROR;
    	g_CCCallPicResultWaitTimer.pstTimer = YKCreateTimer(CCTimerPicResultCallBack,   //start timer4 for Picture result
    													    (void*) TIMERID_PIC_RESULT_WAIT,
    													    PIC_TAKE_TIMEOUT, YK_TT_NONPERIODIC,
    														(unsigned long *)&g_CCCallPicResultWaitTimer.ulMagicNum);
    	LOG_RUNLOG_DEBUG("CC SET PIC_TAKE_TIMEOUT is %d",  PIC_TAKE_TIMEOUT);

        if (TEL1 == ucTelType)
        {
            PICTakePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP1));
        }
        else if(TEL2 == ucTelType)
        {
            PICTakePicture(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_SIP2));
        }
        else if (DEMOL_CALL_TEL == ucTelType)
        {
        	PICTakePicture(g_aucDemolCallPhoneNum);							//上报平台
        	CCSSCapturePicture(g_aucDemolCallPhoneNum);						//抓拍


        }
        LOG_RUNLOG_DEBUG("CC =======SEND _PICTURE_SEND_SERVER_ " );
    }
    else
    {
    	g_iSipCallFlag = SIP_CALL_OK;
    	CCTelCall(ucTelType);
    }
}


void CCTelCall(unsigned char ucTelType)
{
    unsigned int uiInterval;
    if (TEL1 == ucTelType)
    {
        CCTe1Call();
    }
    else if(TEL2 == ucTelType)
    {
        CCTe2Call();
    }
    else if (DEMOL_CALL_TEL == ucTelType )
   {
        YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
        CCSIPTerminateAll();
        LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPTerminateAll");
		CCSIPCall(g_aucDemolCallPhoneNum, 0, 0);
		uiInterval = 30* 1000;		//
		CCCreateTimer(TIMERID_TALK_WAIT, uiInterval);
		LOG_RUNLOG_DEBUG("CC STATE:IDLE CCSIPCall Interval=%dMs",uiInterval);
		g_CCFsmState = CCFSM_DEMO_CALL;
		LOG_RUNLOG_DEBUG("CC STATE:IDLE -> STATE:DEMO_CALL");

		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, g_CCFsmRoom, LOG_EVT_DEMO_CALL, 0, "");
   }
    else
    {
         LOG_RUNLOG_DEBUG("CC STATE: TelType  error");
    }
}
void CCTe1Call(void)
{
    unsigned int 	uiInterval;
    YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
    CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL

    LOG_RUNLOG_DEBUG("CC STATE CCGetHintMusicEn %d ",CCGetHintMusicEn(g_CCFsmRoom));     
    CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL1),                   //获取TEL1号码及相关参数
                        CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL1),
                        CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL

    LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Remove CallWait Timer");

    uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数

    LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
    g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
                                                                        (void*) TIMERID_CALL_WAIT,
                                                                        uiInterval, YK_TT_NONPERIODIC,
                                                                        (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
    LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT Create CallWait Timer");

    g_CCFsmState = CCFSM_TEL1_CALL_WAIT;
    LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT -> STATE:TEL1_CALL_WAIT");    
}

void CCTe2Call(void)
{
    unsigned int 	uiInterval;
    YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
    CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
    LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPTerminateAll");
    CCSIPCall(CCGetPhoneNum(g_CCFsmRoom,PHONE_TYPE_TEL2),                   //获取TEL1号码及相关参数
                        CCGetMediaType(g_CCFsmRoom,PHONE_TYPE_TEL2),
                        CCGetHintMusicEn(g_CCFsmRoom));                           //CCSIP_CALL

    LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT Remove CallWait Timer");

    uiInterval = CCGetTime4CallWait(g_CCFsmRoom, ROOM_NUM_LEN);				//获取呼叫等待时间参数

    LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT CCSIPCall Interval=%dMs",uiInterval);
    g_CCCallWaitTimer.pstTimer = YKCreateTimer(CCTimerCallBack,				//start timer2 for calling
                                                                        (void*) TIMERID_CALL_WAIT,
                                                                        uiInterval, YK_TT_NONPERIODIC,
                                                                        (unsigned long *)&g_CCCallWaitTimer.ulMagicNum);
    LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT Create CallWait Timer");

    g_CCFsmState = CCFSM_TEL2_CALL_WAIT;
    LOG_RUNLOG_DEBUG("CC STATE:SIP2_CALL_WAIT -> STATE:TEL1_CALL_WAIT");    
}

//初始化SipCall和SocketConnect标识
void InitSipCallFlagAndSocketConnectFlag(void)
{
	g_iSipCallFlag = SIP_CALL_OK;
	g_iSocketConnectFlag = SOCKET_CONNECT_OK;
}

//个推状态处理
void DealTakePicState(unsigned int uiPrmvType,unsigned char ucTelType)
{
    switch(uiPrmvType)  
    {
        case TMCC_PIC_RESULT_PICKUP:    //接听
            InitSipCallFlagAndSocketConnectFlag();
            LOG_RUNLOG_DEBUG("CC TMCC_PIC_RESULT_PICKUP");
            CCTelCall(ucTelType);
            break;

        case TMCC_PIC_RESULT_OPEN_DOOR:         //开门
            InitSipCallFlagAndSocketConnectFlag();
            LOG_RUNLOG_DEBUG("CC TMCC_PIC_RESULT_OPEN_DOOR");
            CCSMOpenDoor();
            LOG_RUNLOG_DEBUG("CC TM_TAKE_PIC  CCSMOpenDoor");
            CCTelCall(ucTelType);
            break;

		case TMCC_PIC_CLINET_RECV_PIC_OK: //客户端收到图片
            InitSipCallFlagAndSocketConnectFlag();
            LOG_RUNLOG_DEBUG("CC TMCC_PIC_CLINET_RECV_PIC_OK");
            YKRemoveTimer(g_CCCallPicResultWaitTimer.pstTimer, g_CCCallPicResultWaitTimer.ulMagicNum);//destroy timer4
            LOG_RUNLOG_DEBUG("CC remove CCCallPicResultWaitTime ");
        	g_CCCallPicResultWaitTimer.pstTimer = YKCreateTimer(CCTimerPicResultCallBack,   //start timer4 for Picture result
        													    (void*) TIMERID_PIC_RESULT_WAIT,
        													    PIC_TAKE_TIMEOUT, YK_TT_NONPERIODIC,
        														(unsigned long *)&g_CCCallPicResultWaitTimer.ulMagicNum);
        	LOG_RUNLOG_DEBUG("CC SET PIC_TAKE_TIMEOUT is %d",  PIC_TAKE_TIMEOUT);
            break;

		case CCCC_PIC_RESULT_TIMEOUT_EVENT: //超时
            InitSipCallFlagAndSocketConnectFlag();
            LOG_RUNLOG_DEBUG("CC CCCC_PIC_RESULT_TIMEOUT_EVENT");
            PICTimeOut();
            CCTelCall(ucTelType);
            break;
		case TMCC_PIC_RESULT_HANG_OFF:              //挂断
            InitSipCallFlagAndSocketConnectFlag();
            LOG_RUNLOG_DEBUG("CC TMCC_PIC_RESULT_HANG_OFF");
            CCSIPTerminateAll();															//CCSIP_TERMINATE_ALL
            LOG_RUNLOG_DEBUG("CC STATE:SIP1_CALL_WAIT CCSIPTerminateAll");
#ifdef _YK_XT8126_
        int iRet = -1;
        iRet = VerifyRoomNum(g_CCFsmRoom,CCFSM_TEL1_CALL_WAIT);
        switch(iRet)
        {
        case TEL1_Y_OTHR_N:
        	LOG_RUNLOG_DEBUG("CC NO OTHER CALL NUM, HAND UP THIS CALL");
            CCSMCalleeErr(ERR_CALLEE_REJECT);
        	break;

        default:
        	break;
        }
#endif

            g_CCFsmState = CCFSM_IDLE;
            LOG_RUNLOG_DEBUG("CC STATE:PLAY_MUSIC -> STATE:IDLE");
            break;

        case TMCC_PIC_RESULT_ERROR:                 //个推错误
            LOG_RUNLOG_DEBUG("CC TMCC_PIC_RESULT_ERROR");
            if (g_iSipCallFlag == SIP_CALL_ERROR)
            {
                g_iSocketConnectFlag = SOCKET_CONNECT_OK;
                CCTelCall(ucTelType);
            }
            else
            {
                g_iSocketConnectFlag = SOCKET_CONNECT_ERROR;
            }
            break;

        case TMCC_PIC_SOCKET_ERROR:                 //SOCKET连接 错误
            LOG_RUNLOG_DEBUG("CC TMCC_PIC_SOCKET_ERROR");
            if (g_iSipCallFlag == SIP_CALL_ERROR)
            {
                g_iSocketConnectFlag = SOCKET_CONNECT_OK;
                CCTelCall(ucTelType);
            }
            else
            {
                g_iSocketConnectFlag = SOCKET_CONNECT_ERROR;
            }
            break;
        
        default:
            break;
        
    }


}
#endif




#endif





