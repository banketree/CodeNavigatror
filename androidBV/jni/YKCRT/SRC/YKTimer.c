#include "../INC/YKTimer.h"

#define MAX_TIMER_COUNT 100
#define TIMER_PRECISION_IN_MS 100 //定时器精度为多少毫秒

//创建互斥量 PTHREAD_MUTEX_RECURSIVE_NP在linux下时候需要加上 win下用NULL
#if  _YK_OS_ == _YK_OS_WIN32_
#define PTHREAD_MUTEX_RECURSIVE_NP NULL
#endif
#define CREATE_MUTEX() 		g_stTimerMgmt.Locker = YKCreateMutex(PTHREAD_MUTEX_RECURSIVE_NP)
#define DELETE_MUTEX() 		YKDestroyMutex(g_stTimerMgmt.Locker);		//删除互斥量
#define OS_ENTER_CRITICAL() YKLockMutex(g_stTimerMgmt.Locker);			//上锁
#define OS_EXIT_CRITICAL()  YKUnLockMutex(g_stTimerMgmt.Locker);		//解锁
#define OS_SLEEP_FOR_A_TIMER_PRECISION_SPAN() YKSleep(TIMER_PRECISION_IN_MS) //休眠一百毫秒

typedef struct
{
	//列表的基本内容和操作，与其中业务内容无关
    YK_TIMER_ST		astTimerList[MAX_TIMER_COUNT];
    YK_TIMER_ST* 	pstFirstIdleTimer;
    YK_TIMER_ST*	pstFirstRunningTimer;
    unsigned long 	ulMagicNum; //用于给定时器分配magic number
    
    //和操作系统相关的互斥量定义
    OS_MUTEX 		Locker;
}YK_TIMER_MGMT_ST;

YK_TIMER_MGMT_ST g_stTimerMgmt;

static void YKInsertRunningTimer(YK_TIMER_ST *pstTimer)  //在YKCreateTimer()内调用
{
    YK_TIMER_ST *pstTmp = NULL;
    YK_TIMER_ST *pstPrev= NULL;
    unsigned long ulTotalInter = 0;

    pstTimer->enStatus = YK_TS_RUNNING;
    
    //列表为空，则直接插入最前端
    if(g_stTimerMgmt.pstFirstRunningTimer == NULL)
    {
        pstTimer->pstPrev = NULL;
        pstTimer->pstNext = NULL;

        OS_ENTER_CRITICAL();
        g_stTimerMgmt.pstFirstRunningTimer = pstTimer;        
        OS_EXIT_CRITICAL();
        return;
    }
    
    //超时时长最小，则直接插入列表最前端
    if(pstTimer->ulInterval <= g_stTimerMgmt.pstFirstRunningTimer->ulRelativeInterval)
    {        
        pstTimer->pstNext = g_stTimerMgmt.pstFirstRunningTimer;
        pstTimer->pstPrev = NULL;

        OS_ENTER_CRITICAL();
        g_stTimerMgmt.pstFirstRunningTimer->ulRelativeInterval -= pstTimer->ulInterval;
        g_stTimerMgmt.pstFirstRunningTimer->pstPrev = pstTimer;
        g_stTimerMgmt.pstFirstRunningTimer = pstTimer;
        OS_EXIT_CRITICAL();
        return;
    }
    
    pstPrev= g_stTimerMgmt.pstFirstRunningTimer;
    pstTmp = g_stTimerMgmt.pstFirstRunningTimer->pstNext;
    ulTotalInter = g_stTimerMgmt.pstFirstRunningTimer->ulRelativeInterval;
    
    while(pstTmp != NULL)
    {
        if(pstTimer->ulInterval <= ulTotalInter + pstTmp->ulRelativeInterval) //插入列表
        {
        	//更新被插入的定时器参数
            pstTimer->ulRelativeInterval = pstTimer->ulInterval - ulTotalInter;
            pstTimer->pstNext = pstTmp;
            pstTimer->pstPrev = pstPrev;

            OS_ENTER_CRITICAL();
            
            //调整后继节点的相对时长
            pstTmp->ulRelativeInterval -= pstTimer->ulRelativeInterval;
            
            //建立与后一个节点的联系
            pstTmp->pstPrev = pstTimer;
            
            //建立与前一个节点的联系
            pstPrev->pstNext = pstTimer;
            
            OS_EXIT_CRITICAL();
            return;
        }

        ulTotalInter += pstTmp->ulRelativeInterval;
        pstPrev= pstTmp;
        pstTmp = pstTmp->pstNext;        
    }
    
    //将定时器插入列表的最末端
    pstTimer->ulRelativeInterval = pstTimer->ulInterval - ulTotalInter;
    pstTimer->pstNext = NULL;
    pstTimer->pstPrev = pstPrev;

    OS_ENTER_CRITICAL();    
    pstPrev->pstNext  = pstTimer;    
    OS_EXIT_CRITICAL();
}

int YKInitTimerMgmt()
{   
    int i = 0;
    g_stTimerMgmt.pstFirstRunningTimer = NULL;

    //将定时器加入空闲列表中，空闲列表是一个单向链表
    for(i = 0; i < MAX_TIMER_COUNT - 1; i++)
    {
        g_stTimerMgmt.astTimerList[i].pstNext = &(g_stTimerMgmt.astTimerList[i + 1]);
        g_stTimerMgmt.astTimerList[i].pstPrev = NULL;
    }
    
    //最后一个节点
    g_stTimerMgmt.astTimerList[MAX_TIMER_COUNT - 1].pstPrev = NULL;
    g_stTimerMgmt.astTimerList[MAX_TIMER_COUNT - 1].pstNext = NULL;

    g_stTimerMgmt.pstFirstIdleTimer = g_stTimerMgmt.astTimerList;

    g_stTimerMgmt.ulMagicNum = 0;
    
    CREATE_MUTEX();
    if(g_stTimerMgmt.Locker == NULL)
    {
        return FAILURE;
    }
    else
    {
        return SUCCESS;
    }
}

void YKClearTimerMgmt()
{
    DELETE_MUTEX();
}

YK_TIMER_ST* YKCreateTimer(YK_TIMER_CALLBACK_FUNC pfCallback,void* pvContext,
						unsigned long ulInterval, YK_TIMER_TYPE_EN enTimerType,
                        unsigned long *pulMagicNum)//单位为毫秒，但一定要比定时器精度大，且最好是整数倍，否则也会被计算成整数倍
{
	YK_TIMER_ST *pstTimer = NULL;
    
	if(ulInterval < TIMER_PRECISION_IN_MS)
	{
		ulInterval = TIMER_PRECISION_IN_MS;
	}
    ulInterval /= TIMER_PRECISION_IN_MS; //换算成以定时器精度为单位的数值
    
    //从空闲列表中获得定时器
    if(g_stTimerMgmt.pstFirstIdleTimer == NULL)
    {
        return NULL; //没有多余的定时器
    }
    
    OS_ENTER_CRITICAL();
    pstTimer = g_stTimerMgmt.pstFirstIdleTimer;
    g_stTimerMgmt.pstFirstIdleTimer = g_stTimerMgmt.pstFirstIdleTimer->pstNext; //调整空闲定时器列表
    OS_EXIT_CRITICAL();
    
    pstTimer->pfCallback = pfCallback;
    pstTimer->pvContext  = pvContext;
    pstTimer->ulInterval = ulInterval;
    pstTimer->ulRelativeInterval = ulInterval;
    pstTimer->enStatus   = YK_TS_IDLE;
    pstTimer->enType     = enTimerType;

    pstTimer->ulMagicNum = g_stTimerMgmt.ulMagicNum++;
    *pulMagicNum = pstTimer->ulMagicNum;
    
	YKInsertRunningTimer(pstTimer); //将定时器插入运行队列

    return pstTimer;
}

void YKRemoveTimer(YK_TIMER_ST *pstRemoved, unsigned long ulMagicNum)
{
	//防止传入空指针
    if(pstRemoved == NULL)
    {
        return;
    }

    if(pstRemoved->ulMagicNum != ulMagicNum)
    {
        return;
    }
    
    ///定时器是否已经被删除或者超时
    if((pstRemoved->enStatus == YK_TS_IDLE) || (pstRemoved->enStatus == YK_TS_STOPPED))
    {
        return;
    }

    //从列表中删除，将前后节点连接起来
    OS_ENTER_CRITICAL();
    if((pstRemoved->pstPrev == NULL) && (pstRemoved->pstNext == NULL)) //说明列表中只有一个定时器
    {
        if(pstRemoved->enStatus == YK_TS_RUNNING)
        {
            g_stTimerMgmt.pstFirstRunningTimer = NULL;
        }
    }    
    else if(pstRemoved->pstPrev == NULL) //说明在列表的最前端
    {
        if(pstRemoved->enStatus == YK_TS_RUNNING)
        {
            g_stTimerMgmt.pstFirstRunningTimer = pstRemoved->pstNext;
        }

        //更新被删除定时器后面的定时器的相对超时时长
        pstRemoved->pstNext->ulRelativeInterval += pstRemoved->ulRelativeInterval;
            
        pstRemoved->pstNext->pstPrev = NULL;
    }
    else if(pstRemoved->pstNext == NULL) //在列表的最后端
    {
        pstRemoved->pstPrev->pstNext = NULL;
    }
    else //在列表的中间
    {
        pstRemoved->pstPrev->pstNext = pstRemoved->pstNext;
        pstRemoved->pstNext->pstPrev = pstRemoved->pstPrev;

        //更新被删除定时器后面的定时器的相对超时时长，可以加上被删除定时器相对时长，
        //也可以使用被删除前后定时器绝对时长的差再减去前一个定时器的相对时长
        pstRemoved->pstNext->ulRelativeInterval += pstRemoved->ulRelativeInterval;
    }
    OS_EXIT_CRITICAL();
  
    //初始化空闲定时器
    pstRemoved->pfCallback = NULL;
	pstRemoved->pvContext  = NULL;
    pstRemoved->ulInterval = 0;
    pstRemoved->ulRelativeInterval = 0;
    pstRemoved->enStatus   = YK_TS_IDLE;
    pstRemoved->pstPrev = NULL;
    pstRemoved->pstNext = NULL;
  
    //放入空闲列表
    pstRemoved->pstNext = g_stTimerMgmt.pstFirstIdleTimer;
    OS_ENTER_CRITICAL();
    g_stTimerMgmt.pstFirstIdleTimer = pstRemoved;
    OS_EXIT_CRITICAL();
}

void YKRemoveAllTimers()
{
	YK_TIMER_ST *pstTimer = g_stTimerMgmt.pstFirstRunningTimer;
	YK_TIMER_ST *pstNext = NULL;

    while(pstTimer != NULL)
    {
        pstNext = pstTimer->pstNext;
        YKRemoveTimer(pstTimer, pstTimer->ulMagicNum);
        pstTimer = pstNext;
    }
}

static void YKCheckTimer()
{
	YK_TIMER_ST *pstTmp = NULL;

    OS_ENTER_CRITICAL();  
    if(g_stTimerMgmt.pstFirstRunningTimer == NULL)
    {
        OS_EXIT_CRITICAL();
        return;
    }
    
    if(g_stTimerMgmt.pstFirstRunningTimer->ulRelativeInterval != 0) //防止初始化时就为0的情况
    {
        g_stTimerMgmt.pstFirstRunningTimer->ulRelativeInterval--;
    }

    while((g_stTimerMgmt.pstFirstRunningTimer != NULL)
       && (g_stTimerMgmt.pstFirstRunningTimer->ulRelativeInterval == 0)) //这里应该不存在0减1后变成0xFFFFFFFF的情况
    {
        pstTmp = g_stTimerMgmt.pstFirstRunningTimer;

        if(pstTmp->enStatus == YK_TS_RUNNING) //如果正在运行
        {
            pstTmp->enStatus = YK_TS_STOPPED; //先暂停，可能是怕函数重入???同时在回调函数中删除定时器也不会有问题

            pstTmp->pfCallback(pstTmp->pvContext); //执行超时操作
            
            pstTmp->enStatus = YK_TS_RUNNING;
        }        
        
        //将定时器从运行列表中删除
        if(pstTmp->enType == YK_TT_PERIODIC)
        { 
            g_stTimerMgmt.pstFirstRunningTimer = pstTmp->pstNext;
            if(g_stTimerMgmt.pstFirstRunningTimer != NULL) //如果只有一个定时器则为空，需要在此判断
            {
                g_stTimerMgmt.pstFirstRunningTimer->pstPrev = NULL;
            }
          
            pstTmp->ulRelativeInterval = pstTmp->ulInterval; //周期性
            YKInsertRunningTimer(pstTmp);
        }
        else
        {   
            YKRemoveTimer(pstTmp, pstTmp->ulMagicNum);
        }
    }

    OS_EXIT_CRITICAL();
}

void YKTimerTask()
{
    OS_SLEEP_FOR_A_TIMER_PRECISION_SPAN();
    
    YKCheckTimer();
}
