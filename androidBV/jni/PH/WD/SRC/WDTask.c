/***************************************************************
  Copyright (C), 1988-2012, YouKe Tech. Co., Ltd.
  @file:
  @author:			zlin
  @date:			2012.4.27
  @description:		//描述
  @version:			//版本
  @history:         // 历史修改记录
  		<author>	<time>   	<version >   <desc>
		hb	   		2012/5/7     1.0      	创建模块
***************************************************************/

#include <YKSystem.h>
#include <YKApi.h>
#include <YKTimer.h>
#include <YKMsgQue.h>
#include <WDTask.h>
#include <IDBT.h>
#include <LOGIntfLayer.h>
#include <TMInterface.h>

#ifdef _YK_XT8126_
#include <YK8126Gpio.h>
#endif

#define WATCHDOG_INTERVAL              500
#define LP_DOG_INTERVAL                15000
#define XT_DOG_INTERVAL                15000
#define TM_DOG_INTERVAL                60000
#define TIMER_ID_WATCHDOG              0x01
#define TIMER_ID_LP_DOG                0x02
#define TIMER_ID_XT_DOG                0x03
#define TIMER_ID_TM_DOG                0x04

typedef struct
{
	unsigned int uiPrmvType;
	unsigned int uiTimerId;
}WD_TIMER_EVENT_ST;

typedef struct	
{
	unsigned int 	uiPrmvType;						//原语类型
}WD_WATCHDOG_FEED_ST;

typedef struct
{
    YK_TIMER_ST     *pstTimer;
    unsigned long	ulMagicNum;
}WD_TIMER_ST;

WD_TIMER_ST g_WDWatchdogTimer;
WD_TIMER_ST g_WDMoniXTTimer;
WD_TIMER_ST g_WDMoniLPTimer;
WD_TIMER_ST g_WDMoniTMTimer;

YK_MSG_QUE_ST  *g_pstWDMsgQ;
YKHandle        g_WDThreadId;
int WatchDogClearFlag = 0;
int FeedDogFlag = 0;

void OpenWatchDog_Func(void)
{
#ifdef _YK_XT8126_
	YK8126IOSetLow(WATCHDOG_IO_EN);
#endif
	printf("Enable WatchDog\n");
}

void CloseWatchDog_Func(void)
{
#ifdef _YK_XT8126_
	YK8126IOSetHigh(WATCHDOG_IO_EN);
	YK8126IOSetHigh(WATCHDOG_IO_CLEAR);
#endif
    printf("Disable WatchDog\n");
}

void ClearWatchFlag(void)
{
#ifdef _YK_XT8126_
    if(WatchDogClearFlag == 0)
        WatchDogClearFlag = 1;
    else
        WatchDogClearFlag = 0;

    if(WatchDogClearFlag == 0)
      	YK8126IOSetLow(WATCHDOG_IO_CLEAR);
    else
      	YK8126IOSetHigh(WATCHDOG_IO_CLEAR);

#endif
	//printf("Clear WatchDog!!!\n");
}

void Init_WatchDog(void)
{
	printf("Watchdog Init!!!\n");
	WatchDogClearFlag = 0;
#ifdef _YK_XT8126_
  	YK8126IOSetOutputLow(WATCHDOG_IO_CLEAR);
  	usleep(1000);
  	YK8126IOSetHigh(WATCHDOG_IO_CLEAR);
  	usleep(1000);
  	YK8126IOSetLow(WATCHDOG_IO_CLEAR);

  	YK8126IOSetOutputLow(WATCHDOG_IO_EN);
#endif
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:接收消息
*Others		:
*********************************************************************/
int WDFeedDogFormLP(void)
{
    WD_WATCHDOG_FEED_ST *pstMsg = YKNew0(WD_WATCHDOG_FEED_ST, 1);
    if(NULL == pstMsg)
    {
        printf("WD malloc error!\n");
        return FAILURE;
    }
    pstMsg->uiPrmvType = SIPWD_WATCHDOG_FEED;

    YKWriteQue(g_pstWDMsgQ , (void *)pstMsg,  0);
    return SUCCESS;
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:接收消息
*Others		:
*********************************************************************/
int WDFeedDogFormXT(void)
{
    WD_WATCHDOG_FEED_ST *pstMsg = YKNew0(WD_WATCHDOG_FEED_ST, 1);
    if(NULL == pstMsg)
    {
        printf("WD malloc error!\n");
        return FAILURE;
    }
    pstMsg->uiPrmvType = XTWD_WATCHDOG_FEED;

    YKWriteQue(g_pstWDMsgQ , (void *)pstMsg,  0);
    return SUCCESS;
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:接收消息
*Others		:
*********************************************************************/
int WDCloseTask(void)
{
    WD_WATCHDOG_FEED_ST *pstMsg = YKNew0(WD_WATCHDOG_FEED_ST, 1);
    if(NULL == pstMsg)
    {
        printf("WD malloc error!\n");
        return FAILURE;
    }
    pstMsg->uiPrmvType = WD_CLOSE_TASK;

    YKWriteQue(g_pstWDMsgQ , (void *)pstMsg,  0);
    return SUCCESS;
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:接收消息
*Others		:
*********************************************************************/
void WDTimerCallback(void *pCtx)
{
	int iErrCode;

	WD_TIMER_EVENT_ST *pstPrmv = YKNew0(WD_TIMER_EVENT_ST,1);
	if (NULL == pstPrmv)
	{
		printf("WD malloc error!\n");
		return;
	}
    
	pstPrmv->uiPrmvType = WD_TIMEOUT_EVENT;
	pstPrmv->uiTimerId  =(int)(pCtx);

	iErrCode = YKWriteQue(g_pstWDMsgQ ,  (void *)pstPrmv,  0);
}

void StopWatchdogTimer()
{
    if(g_WDWatchdogTimer.pstTimer != NULL) //stop first
    {
        YKRemoveTimer(g_WDWatchdogTimer.pstTimer, g_WDWatchdogTimer.ulMagicNum);
    }
}

void StartWatchdogTimer()
{
	StopWatchdogTimer();

    //create new one
    g_WDWatchdogTimer.pstTimer = YKCreateTimer(WDTimerCallback,
                                               (void *)TIMER_ID_WATCHDOG,
                                               WATCHDOG_INTERVAL, YK_TT_NONPERIODIC,
                                               (unsigned long *)&g_WDWatchdogTimer.ulMagicNum);
}

void TMStopMoniTimer()
{
    if(g_WDMoniTMTimer.pstTimer != NULL) //stop first
    {
        YKRemoveTimer(g_WDMoniTMTimer.pstTimer, g_WDMoniTMTimer.ulMagicNum);
    }
}


void TMStartMoniTimer()
{
	TMStopMoniTimer();
    //create new one
    g_WDMoniTMTimer.pstTimer = YKCreateTimer(WDTimerCallback,
                                               (void *)TIMER_ID_TM_DOG,
                                               TM_DOG_INTERVAL, YK_TT_NONPERIODIC,
                                               (unsigned long *)&g_WDMoniTMTimer.ulMagicNum);
}

void XTStopMoniTimer()
{
    if(g_WDMoniXTTimer.pstTimer != NULL) //stop first
    {
        YKRemoveTimer(g_WDMoniXTTimer.pstTimer, g_WDMoniXTTimer.ulMagicNum);
    }
}


void XTStartMoniTimer()
{
	XTStopMoniTimer();
    //create new one
    g_WDMoniXTTimer.pstTimer = YKCreateTimer(WDTimerCallback,
                                               (void *)TIMER_ID_XT_DOG,
                                               XT_DOG_INTERVAL, YK_TT_NONPERIODIC,
                                               (unsigned long *)&g_WDMoniXTTimer.ulMagicNum);
}

void LPStopMoniTimer()
{
    if(g_WDMoniLPTimer.pstTimer != NULL) //stop first
    {
        YKRemoveTimer(g_WDMoniLPTimer.pstTimer, g_WDMoniLPTimer.ulMagicNum);
    }
}

void LPStartMoniTimer()
{
	LPStopMoniTimer();

    //create new one
    g_WDMoniLPTimer.pstTimer = YKCreateTimer(WDTimerCallback,
                                               (void *)TIMER_ID_LP_DOG,
                                               LP_DOG_INTERVAL, YK_TT_NONPERIODIC,
                                               (unsigned long *)&g_WDMoniLPTimer.ulMagicNum);
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void *WDTask(void* arg)
{
    int iErrCode;
    void *pvPrmvType = NULL;

    StartWatchdogTimer();
#if _TM_TYPE_ == _TR069_
    //TMStartMoniTimer();
#endif
#ifdef _YK_XT8126_BV_
    XTStartMoniTimer();
#endif
    //LPStartMoniTimer();

    while(g_iRunWDTaskFlag)		                                    //CCTask运行标志为真
    {

        iErrCode = YKReadQue(g_pstWDMsgQ, &pvPrmvType, 0);						//阻塞读取消息
        if ( 0 != iErrCode || NULL == pvPrmvType )
        {
            continue;
        }
        switch(*((unsigned int *)pvPrmvType))
        {
        case WD_CLOSE_TASK:
            g_iRunWDTaskFlag = FALSE;
            StopWatchdogTimer();
            LPStopMoniTimer();
            XTStopMoniTimer();
            TMStopMoniTimer();
        	break;
        case TMWD_WATCHDOG_FEED:
            TMStartMoniTimer();
            break;
        case XTWD_WATCHDOG_FEED:
            XTStartMoniTimer();
        	break;
        case SIPWD_WATCHDOG_FEED:
            LPStartMoniTimer();
            break;
        case WD_TIMEOUT_EVENT:
            switch(((WD_TIMER_EVENT_ST *)pvPrmvType)->uiTimerId)
            {
            case TIMER_ID_WATCHDOG:
            	if(FeedDogFlag == 0)
            	{
            		ClearWatchFlag();
            	}
                StartWatchdogTimer();
                break;
            case TIMER_ID_LP_DOG:
            case TIMER_ID_XT_DOG:
            	FeedDogFlag = 1;
            	break;
            case TIMER_ID_TM_DOG:
            	//evcpe_shutdown(0);
            	/*printf("TIMER_ID_TM_DOG \n");
            	TMStopTerminateMange();
            	if(TMStartTerminateManage(YK_APP_VERSION, NULL));
            	{
            		g_iRunTMTaskFlag = TRUE;
            	}*/
            	break;
            default:
                break;
            }
            break;
        default:
            break;
        }

        free((void *)(pvPrmvType));										//释放消息内容指针
    }
}

/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
int WDTaskInit(void)
{
    //初始化
    Init_WatchDog();
    g_iRunWDTaskFlag = TRUE;		                                 //CCTask 运行标志置位

    g_pstWDMsgQ  = YKCreateQue(1024);                                //创建消息队列；

    g_WDThreadId = YKCreateThread(WDTask, NULL);                     //创建CCTask

    if(NULL == g_WDThreadId)
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
void WDTaskFini(void)
{
    //卸载
    g_iRunWDTaskFlag = FALSE;		                                //CCTask 运行标志置位
    CloseWatchDog_Func();
    WDCloseTask();

    if(NULL != g_WDThreadId)
    {
        YKJoinThread(g_WDThreadId);

        YKDestroyThread(g_WDThreadId);
    }

    YKDeleteQue(g_pstWDMsgQ);
}

