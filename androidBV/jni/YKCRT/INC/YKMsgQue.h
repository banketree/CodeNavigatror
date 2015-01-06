/**
 *@addtogroup YKCRT
 *@{
 */

/**
 *@file YKMsgQue.h
 *@brief Basic types defines. 
 *@author liyu
 *@version 1.0
 *@date 2012-05-22
 */

#ifndef YK_MSG_QUE_H_
#define YK_MSG_QUE_H_

#include "YKList.h"

//#ifdef __cplusplus
//extern "C"{
//#endif

#define RW_QUEUQ_TIMEOUT -1
#define RW_QUEUE_SUCCESS 0

typedef struct
{
    YK_LIST_ST *pstMsgList;         /**<!List.*/
    YKHandle    hLocker;            /**<!Handle of mutex.*/
    YKHandle    hWriteQueSem;       /**<!Handle of writing queue semaphore */
    YKHandle    hReadQueSem;        /**<!Handle of reading queue semaphore */
    YKInt32     iCapacity;          /**<!The max element of message queue */
}YK_MSG_QUE_ST;

YK_MSG_QUE_ST*  YKCreateQue(int iCapacity);
void            YKDeleteQue(YK_MSG_QUE_ST *pstQue);
int             YKReadQue(YK_MSG_QUE_ST *pstQue,  void **pvMsg, int iTimeout);
int             YKWriteQue(YK_MSG_QUE_ST *pstQue, void *pvMsg, int iTimeout);
BOOL            YKIsQueEmpty(YK_MSG_QUE_ST *pstQue);
BOOL            YKIsQueFull(YK_MSG_QUE_ST *pstQue);
int             YKGetMsgCount(YK_MSG_QUE_ST *pstQue);
int             YKGetQueCapacity(YK_MSG_QUE_ST *pstQue);


//#ifdef __cplusplus
//}
//#endif

#endif //!YK_MSG_QUE_H_
