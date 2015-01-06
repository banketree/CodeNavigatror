/**
 *@addtogroup YKCRT
 *@{
 */

/**
 *@file ykapi.h
 *@brief Platform-dependent functions defines 
 *
 *All system-special function and non-standard C/C++ routines go here
 *@author Amos
 *@version 1.0
 *@date 2012-04-27
 */

#ifndef __YK_API_H__
#define __YK_API_H__

#include "YKSystem.h"

#ifdef __cplusplus
extern "C"{
#endif

/**
 *@name Thread Thread-relative operator defines
 *@{
 */

/**
 *@brief Thread callback function type.
 */
typedef void* (YKThreadCallBackFunc)(void* pvParam);

/**
 *@brief Create thread.
 *@pvParam[in] func Thread callback function.
 *@pvParam[in] pvParam Thread parameter will used by func.
 *@return NULL indicate create thread failed others indicate success.
 *Call YKDestroyThread to release handle. @see YKDestroyThread
 */
YKHandle YKCreateThread(YKThreadCallBackFunc Func, void* pvParam);

/**
 *@brief Wait for thread exit.
 *@param[in] handle Handle of thread.
 *@return void
 */
void YKJoinThread(YKHandle hHandle);

/**
 *@brief Destroy the handle of thread
 *@param[in] handle Handle of thread.
 *@return void
 */
void YKDestroyThread(YKHandle hHandle);

/**@}*///Thread

/**
 *@name Mutex Mutex-relative operator defines
 *@{
 */

/**
 *@brief Create Mutex
 *@param[in] attr Should be NULL.
 *@return NULL indicated failed else indicates success.
 *Call YKDestroyMutex to release handle. @see YKDestroyMutex
 */
YKHandle YKCreateMutex(void* pvAttr);

/**
 *@brief Lock mutex
 *@param[in] handle Handle of mutex.
 *@return void
 */
void YKLockMutex(YKHandle hHandle);

/**
 *@brief Unlock mutex
 *@param[in] handle Handle of mutex.
 *@return void
 */
void YKUnLockMutex(YKHandle hHandle);

/**
 *@brief Destroy mutex.
 *@param[in] handle Handle of mutex.
 *@return void
 */
void YKDestroyMutex(YKHandle hHandle);

/**@}*///Mutex

/**
 *@name Semaphore Semaphore-relative operator defines
 *@{
 */

/**
 *@brief Create Semaphore
 *@param[in] lMaxCount Semaphore max count.
 *@return NULL indicated failed else indicates success.
 */
YKHandle    YKCreateSemaphore(YKLong lMaxCount);

/**
 *@brief Wait Semaphore
 *@param[in] handle Handle of Semaphore.
 *@return void
 */
int        YKWaitSemaphore(YKHandle hHandle, YKLong lTimeOut);

/**
 *@brief Release Semaphore
 *@param[in] handle Handle of Semaphore.
 *@return void
 */
void        YKPostSemaphore(YKHandle hHandle);

/**
 *@brief Destroy Semaphore.
 *@param[in] handle Handle of Semaphore.
 *@return void
 */
void        YKDestroySemaphore(YKHandle hHandle);

/**@}*///Semaphore

/**
 *@name Time Time-relative operator defines.
 @{
 */
/**@brief Time struct.*/
typedef struct _YKTimeVal{
    YKLong lSeconds;         /**<! lSeconds.*/
    YKLong lMilliSeconds;    /**<! millisecond. */
}YK_TIME_VAL_ST;

typedef struct
{
   int iSec;
   int iMin;
   int iHour;
   int iMday;
   int iMon;
   int iYear;
   int iWday;
   int iYday;
   int iIsdst;
}YK_CURTIME_ST;

/**
 *@brief Get current time.
 *@param[in/out] tv User for save current time.
 *@return void
 */
void		YKGetCurrtime(YK_CURTIME_ST *pstCurtime);

/**
 *@brief Get current time.
 *@param[in/out] tv User for save current time.
 *@return void
 */
void YKGetTimeOfDay(YK_TIME_VAL_ST *pstTv);

/**
 *@brief Through from the operating system to start now
 *@return YKlong
 */
YKLong YKGetTickCount();

/**
 *@brief The suspension of execution for a period of time
 *@param[in] millSeconds Suspend millSeconds
 *@return void
 */
void YKSleep(YKLong lMillSeconds);
/**@}*///Time

/**
 *@name Memory.
 *@{
 */

#define YKNew(type,count)	(type*)malloc(sizeof(type)*(count))
#define YKNew0(type,count)	(type*)calloc((count), sizeof(type))
#define YKFree(p)           free(p);
/**@}*/
#define Text(x)             #x

/**
 *@name Browser folder
 *@{
 */
typedef enum{
    YK_TYPE_DIRECTORY=0,
    YK_TYPE_FILE,
    YK_TYPE_UNKNOW,
}YK_FILE_TYPE_EN;

/**
 *@brief Open directory.
 *@param[in] folder Directory path.
 *@return The handle of Opened directory. Call YKFindClose to release handle.
 */
YKHandle YKFindFirstFile(const char* pcDirectory);

/**
 *@brief Close The handle of Opened directory
 *@param[in] hFolder The handle of Opened directory
 *@return void
 */
void YKFindClose(YKHandle hHandle);

/**
 *@brief Get files form directory.
 *@param[in] hFolder The handle of Opened directory.
 *@return  TRUE indicates find success.
 */
BOOL YKFindNextFile(YKHandle hHandle);

/**
 *@brief Get file type. @see YK_FILE_TYPE_EN
 *@param[in] handle The handle of Opened directory
 *@return File type @see YK_FILE_TYPE_EN
 */
YK_FILE_TYPE_EN YKGetFileType(YKHandle hHandle);

/**
 *@brief Get file name.
 *@param[in] handle The handle of Opened directory
 *@return The name of the file.
 */
const char* YKGetFileName(YKHandle hHandle);

/**@}*///Browse folder

#ifdef __cplusplus
};
#endif

#endif//!__YK_API_H__

/**@}*///YKCRT
