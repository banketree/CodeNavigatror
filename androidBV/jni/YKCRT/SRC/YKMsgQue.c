#include "../INC/YKMsgQue.h"
#include "../INC/YKApi.h"

YK_MSG_QUE_ST*  YKCreateQue(int iCapacity)
{
    YK_MSG_QUE_ST* pstQueue = YKNew0(YK_MSG_QUE_ST, 1);
    if(NULL!=pstQueue)
    {
        pstQueue->hLocker   = YKCreateMutex(NULL);
        pstQueue->iCapacity = iCapacity;
        pstQueue->hReadQueSem  = YKCreateSemaphore(0);
        pstQueue->hWriteQueSem = YKCreateSemaphore(iCapacity);
		pstQueue->pstMsgList = YKListNew(NULL);
    }
    return  pstQueue;
}

void YKDeleteQue(YK_MSG_QUE_ST *pstQue)
{
    if(NULL != pstQue)
    {
        YKDestroyMutex(pstQue->hLocker);
        YKDestroySemaphore(pstQue->hReadQueSem);
        YKDestroySemaphore(pstQue->hWriteQueSem);
        YKListFree(pstQue->pstMsgList);
		YKFree(pstQue);
    }
}

int YKReadQue(YK_MSG_QUE_ST *pstQue, void **pvMsg, int iTimeout)
{
    if(pstQue == NULL || -1==YKWaitSemaphore(pstQue->hReadQueSem, iTimeout))
    {
        return -1;
    }
    YKLockMutex(pstQue->hLocker);
    *pvMsg=YKListPop(&pstQue->pstMsgList);
    YKUnLockMutex(pstQue->hLocker);
    YKPostSemaphore(pstQue->hWriteQueSem);
    return NULL==*pvMsg?-1:0;
}

int YKWriteQue(YK_MSG_QUE_ST *pstQue, void *pvMsg, int iTimeout)
{
    if(pstQue == NULL || -1==YKWaitSemaphore(pstQue->hWriteQueSem, iTimeout))
    {
        return -1;
    }
    YKLockMutex(pstQue->hLocker);
    pstQue->pstMsgList=YKListAppend(pstQue->pstMsgList, pvMsg);
    YKUnLockMutex(pstQue->hLocker);
    YKPostSemaphore(pstQue->hReadQueSem);
    return 0;
}

BOOL YKIsQueEmpty(YK_MSG_QUE_ST *pstQue)
{
    return 0==YKGetMsgCount(pstQue);
}

BOOL YKIsQueFull(YK_MSG_QUE_ST *pstQue)
{
    if(pstQue)
    {
        return YKGetMsgCount(pstQue)>=pstQue->iCapacity;
    }
    return FALSE;
}

int YKGetMsgCount(YK_MSG_QUE_ST *pstQue)
{
    return YKListSize(pstQue->pstMsgList);
}

int YKGetQueCapacity(YK_MSG_QUE_ST *pstQue)
{
    if(NULL != pstQue)
    {
        return pstQue->iCapacity;
    }
}
