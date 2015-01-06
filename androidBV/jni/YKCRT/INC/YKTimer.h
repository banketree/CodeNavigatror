#ifndef YKTIMER_H_
#define YKTIMER_H_

#include <YKApi.h>

#if _YK_OS_ == _YK_OS_WIN32_
#pragma comment(lib, "../bin/libykcrt.lib")
#endif

//#ifdef __cplusplus
//extern "C"{
//#endif

#define OS_MUTEX  YKHandle

/**@brief timer status.*/
typedef enum
{
    YK_TS_IDLE = 0, /**<! 初始状态 */
    YK_TS_RUNNING,  /**<! 正在运行 */
    YK_TS_STOPPED   /**<! 停止 */
}YK_TIMER_STATUS_EN;

/**@brief timer type.*/
typedef enum 
{
    YK_TT_NONPERIODIC = 0, /**<! 定时器可以结束  */
    YK_TT_PERIODIC 		/**<! 重新定时，适用于周期性定时器  */
}YK_TIMER_TYPE_EN;

typedef void (*YK_TIMER_CALLBACK_FUNC)(void*); /**<! 定时器回调函数指针类型 */

/**@brief timer info struct.*/
typedef struct TIMER_ST_TAG
{
    struct TIMER_ST_TAG *pstPrev;	  /**<! 前一个定时器 */
    struct TIMER_ST_TAG *pstNext;	  /**<! 后一个定时器 */

    unsigned long ulInterval;         /**<! 定时器时长 */
    unsigned long ulRelativeInterval; /**<! 相对于前一个定时器的时长，将被计时递减 */
    YK_TIMER_STATUS_EN enStatus;      /**<! 定时器状态 */
    YK_TIMER_TYPE_EN enType;          /**<! 定时器的类型 */

    void* pvContext;                  /**<! 上下文，存放回调函数需要的参数，注意，如果指向的是动态申请的内存则要由调用方去释放(在停止定时器和超时时) */
    YK_TIMER_CALLBACK_FUNC pfCallback;   /**<! 定时器超时执行的回调函数指针 */

    unsigned long ulMagicNum;         /**<! 在创建定时器时分配，并在删除定时器时用于分辨是否是对应的定时器 */
}YK_TIMER_ST;

/**
 * @brief 定时器初始化.
 *@return SUCCESS 成功  FAILURE 失败
 */
int 	YKInitTimerMgmt();

/**@brief 定时器销毁.*/
void 	YKClearTimerMgmt();

/**
 *@brief		移除定时器.
 *@param[in] 	pRemoved 	准备移除的定时器.
 *@param[in] 	ulMagicNum 	在创建定时器时分配，并在删除定时器时用于分辨是否是对应的定时器
 */
void	YKRemoveTimer(YK_TIMER_ST *pRemoved, unsigned long ulMagicNum);

/**@brief 移除所有定时器.*/
void	YKRemoveAllTimers();

/**@brief 计时任务以定时器计时精度为间隔周期性调用本函数，检查是否有定时器超时.*/
void	YKTimerTask();

/**
 * @brief 		计时任务以定时器计时精度为间隔周期性调用本函数，检查是否有定时器超时.
 * @brief		定时器精度为100ms
 * @param[in]	pfCallback	定时时间到要执行的回调函数
 * @param[in]	pvContext	存放回调函数需要的参数
 * @param[in]	ulInterval	定时间隔
 * @param[in]	enTimerType	是否循环调用
 * @param[in]	pulMagicNum 在创建定时器时分配，并在删除定时器时用于分辨是否是对应的定时器
 * @return 		定时器结构体指针
 */
YK_TIMER_ST*	YKCreateTimer(YK_TIMER_CALLBACK_FUNC pfCallback,void* pvContext,
						unsigned long ulInterval, YK_TIMER_TYPE_EN enTimerType,
                        unsigned long *pulMagicNum); //创建定时器

//#ifdef __cplusplus
//};
//#endif

#endif //ifndef TIMER_H_
