#include "../INC/YKTimer.h"

#define MAX_TIMER_COUNT 100
#define TIMER_PRECISION_IN_MS 100 //��ʱ������Ϊ���ٺ���

//���������� PTHREAD_MUTEX_RECURSIVE_NP��linux��ʱ����Ҫ���� win����NULL
#if  _YK_OS_ == _YK_OS_WIN32_
#define PTHREAD_MUTEX_RECURSIVE_NP NULL
#endif
#define CREATE_MUTEX() 		g_stTimerMgmt.Locker = YKCreateMutex(PTHREAD_MUTEX_RECURSIVE_NP)
#define DELETE_MUTEX() 		YKDestroyMutex(g_stTimerMgmt.Locker);		//ɾ��������
#define OS_ENTER_CRITICAL() YKLockMutex(g_stTimerMgmt.Locker);			//����
#define OS_EXIT_CRITICAL()  YKUnLockMutex(g_stTimerMgmt.Locker);		//����
#define OS_SLEEP_FOR_A_TIMER_PRECISION_SPAN() YKSleep(TIMER_PRECISION_IN_MS) //����һ�ٺ���

typedef struct
{
	//�б�Ļ������ݺͲ�����������ҵ�������޹�
    YK_TIMER_ST		astTimerList[MAX_TIMER_COUNT];
    YK_TIMER_ST* 	pstFirstIdleTimer;
    YK_TIMER_ST*	pstFirstRunningTimer;
    unsigned long 	ulMagicNum; //���ڸ���ʱ������magic number
    
    //�Ͳ���ϵͳ��صĻ���������
    OS_MUTEX 		Locker;
}YK_TIMER_MGMT_ST;

YK_TIMER_MGMT_ST g_stTimerMgmt;

static void YKInsertRunningTimer(YK_TIMER_ST *pstTimer)  //��YKCreateTimer()�ڵ���
{
    YK_TIMER_ST *pstTmp = NULL;
    YK_TIMER_ST *pstPrev= NULL;
    unsigned long ulTotalInter = 0;

    pstTimer->enStatus = YK_TS_RUNNING;
    
    //�б�Ϊ�գ���ֱ�Ӳ�����ǰ��
    if(g_stTimerMgmt.pstFirstRunningTimer == NULL)
    {
        pstTimer->pstPrev = NULL;
        pstTimer->pstNext = NULL;

        OS_ENTER_CRITICAL();
        g_stTimerMgmt.pstFirstRunningTimer = pstTimer;        
        OS_EXIT_CRITICAL();
        return;
    }
    
    //��ʱʱ����С����ֱ�Ӳ����б���ǰ��
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
        if(pstTimer->ulInterval <= ulTotalInter + pstTmp->ulRelativeInterval) //�����б�
        {
        	//���±�����Ķ�ʱ������
            pstTimer->ulRelativeInterval = pstTimer->ulInterval - ulTotalInter;
            pstTimer->pstNext = pstTmp;
            pstTimer->pstPrev = pstPrev;

            OS_ENTER_CRITICAL();
            
            //������̽ڵ�����ʱ��
            pstTmp->ulRelativeInterval -= pstTimer->ulRelativeInterval;
            
            //�������һ���ڵ����ϵ
            pstTmp->pstPrev = pstTimer;
            
            //������ǰһ���ڵ����ϵ
            pstPrev->pstNext = pstTimer;
            
            OS_EXIT_CRITICAL();
            return;
        }

        ulTotalInter += pstTmp->ulRelativeInterval;
        pstPrev= pstTmp;
        pstTmp = pstTmp->pstNext;        
    }
    
    //����ʱ�������б����ĩ��
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

    //����ʱ����������б��У������б���һ����������
    for(i = 0; i < MAX_TIMER_COUNT - 1; i++)
    {
        g_stTimerMgmt.astTimerList[i].pstNext = &(g_stTimerMgmt.astTimerList[i + 1]);
        g_stTimerMgmt.astTimerList[i].pstPrev = NULL;
    }
    
    //���һ���ڵ�
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
                        unsigned long *pulMagicNum)//��λΪ���룬��һ��Ҫ�ȶ�ʱ�����ȴ��������������������Ҳ�ᱻ�����������
{
	YK_TIMER_ST *pstTimer = NULL;
    
	if(ulInterval < TIMER_PRECISION_IN_MS)
	{
		ulInterval = TIMER_PRECISION_IN_MS;
	}
    ulInterval /= TIMER_PRECISION_IN_MS; //������Զ�ʱ������Ϊ��λ����ֵ
    
    //�ӿ����б��л�ö�ʱ��
    if(g_stTimerMgmt.pstFirstIdleTimer == NULL)
    {
        return NULL; //û�ж���Ķ�ʱ��
    }
    
    OS_ENTER_CRITICAL();
    pstTimer = g_stTimerMgmt.pstFirstIdleTimer;
    g_stTimerMgmt.pstFirstIdleTimer = g_stTimerMgmt.pstFirstIdleTimer->pstNext; //�������ж�ʱ���б�
    OS_EXIT_CRITICAL();
    
    pstTimer->pfCallback = pfCallback;
    pstTimer->pvContext  = pvContext;
    pstTimer->ulInterval = ulInterval;
    pstTimer->ulRelativeInterval = ulInterval;
    pstTimer->enStatus   = YK_TS_IDLE;
    pstTimer->enType     = enTimerType;

    pstTimer->ulMagicNum = g_stTimerMgmt.ulMagicNum++;
    *pulMagicNum = pstTimer->ulMagicNum;
    
	YKInsertRunningTimer(pstTimer); //����ʱ���������ж���

    return pstTimer;
}

void YKRemoveTimer(YK_TIMER_ST *pstRemoved, unsigned long ulMagicNum)
{
	//��ֹ�����ָ��
    if(pstRemoved == NULL)
    {
        return;
    }

    if(pstRemoved->ulMagicNum != ulMagicNum)
    {
        return;
    }
    
    ///��ʱ���Ƿ��Ѿ���ɾ�����߳�ʱ
    if((pstRemoved->enStatus == YK_TS_IDLE) || (pstRemoved->enStatus == YK_TS_STOPPED))
    {
        return;
    }

    //���б���ɾ������ǰ��ڵ���������
    OS_ENTER_CRITICAL();
    if((pstRemoved->pstPrev == NULL) && (pstRemoved->pstNext == NULL)) //˵���б���ֻ��һ����ʱ��
    {
        if(pstRemoved->enStatus == YK_TS_RUNNING)
        {
            g_stTimerMgmt.pstFirstRunningTimer = NULL;
        }
    }    
    else if(pstRemoved->pstPrev == NULL) //˵�����б����ǰ��
    {
        if(pstRemoved->enStatus == YK_TS_RUNNING)
        {
            g_stTimerMgmt.pstFirstRunningTimer = pstRemoved->pstNext;
        }

        //���±�ɾ����ʱ������Ķ�ʱ������Գ�ʱʱ��
        pstRemoved->pstNext->ulRelativeInterval += pstRemoved->ulRelativeInterval;
            
        pstRemoved->pstNext->pstPrev = NULL;
    }
    else if(pstRemoved->pstNext == NULL) //���б������
    {
        pstRemoved->pstPrev->pstNext = NULL;
    }
    else //���б���м�
    {
        pstRemoved->pstPrev->pstNext = pstRemoved->pstNext;
        pstRemoved->pstNext->pstPrev = pstRemoved->pstPrev;

        //���±�ɾ����ʱ������Ķ�ʱ������Գ�ʱʱ�������Լ��ϱ�ɾ����ʱ�����ʱ����
        //Ҳ����ʹ�ñ�ɾ��ǰ��ʱ������ʱ���Ĳ��ټ�ȥǰһ����ʱ�������ʱ��
        pstRemoved->pstNext->ulRelativeInterval += pstRemoved->ulRelativeInterval;
    }
    OS_EXIT_CRITICAL();
  
    //��ʼ�����ж�ʱ��
    pstRemoved->pfCallback = NULL;
	pstRemoved->pvContext  = NULL;
    pstRemoved->ulInterval = 0;
    pstRemoved->ulRelativeInterval = 0;
    pstRemoved->enStatus   = YK_TS_IDLE;
    pstRemoved->pstPrev = NULL;
    pstRemoved->pstNext = NULL;
  
    //��������б�
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
    
    if(g_stTimerMgmt.pstFirstRunningTimer->ulRelativeInterval != 0) //��ֹ��ʼ��ʱ��Ϊ0�����
    {
        g_stTimerMgmt.pstFirstRunningTimer->ulRelativeInterval--;
    }

    while((g_stTimerMgmt.pstFirstRunningTimer != NULL)
       && (g_stTimerMgmt.pstFirstRunningTimer->ulRelativeInterval == 0)) //����Ӧ�ò�����0��1����0xFFFFFFFF�����
    {
        pstTmp = g_stTimerMgmt.pstFirstRunningTimer;

        if(pstTmp->enStatus == YK_TS_RUNNING) //�����������
        {
            pstTmp->enStatus = YK_TS_STOPPED; //����ͣ���������º�������???ͬʱ�ڻص�������ɾ����ʱ��Ҳ����������

            pstTmp->pfCallback(pstTmp->pvContext); //ִ�г�ʱ����
            
            pstTmp->enStatus = YK_TS_RUNNING;
        }        
        
        //����ʱ���������б���ɾ��
        if(pstTmp->enType == YK_TT_PERIODIC)
        { 
            g_stTimerMgmt.pstFirstRunningTimer = pstTmp->pstNext;
            if(g_stTimerMgmt.pstFirstRunningTimer != NULL) //���ֻ��һ����ʱ����Ϊ�գ���Ҫ�ڴ��ж�
            {
                g_stTimerMgmt.pstFirstRunningTimer->pstPrev = NULL;
            }
          
            pstTmp->ulRelativeInterval = pstTmp->ulInterval; //������
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
