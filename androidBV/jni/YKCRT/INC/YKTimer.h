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
    YK_TS_IDLE = 0, /**<! ��ʼ״̬ */
    YK_TS_RUNNING,  /**<! �������� */
    YK_TS_STOPPED   /**<! ֹͣ */
}YK_TIMER_STATUS_EN;

/**@brief timer type.*/
typedef enum 
{
    YK_TT_NONPERIODIC = 0, /**<! ��ʱ�����Խ���  */
    YK_TT_PERIODIC 		/**<! ���¶�ʱ�������������Զ�ʱ��  */
}YK_TIMER_TYPE_EN;

typedef void (*YK_TIMER_CALLBACK_FUNC)(void*); /**<! ��ʱ���ص�����ָ������ */

/**@brief timer info struct.*/
typedef struct TIMER_ST_TAG
{
    struct TIMER_ST_TAG *pstPrev;	  /**<! ǰһ����ʱ�� */
    struct TIMER_ST_TAG *pstNext;	  /**<! ��һ����ʱ�� */

    unsigned long ulInterval;         /**<! ��ʱ��ʱ�� */
    unsigned long ulRelativeInterval; /**<! �����ǰһ����ʱ����ʱ����������ʱ�ݼ� */
    YK_TIMER_STATUS_EN enStatus;      /**<! ��ʱ��״̬ */
    YK_TIMER_TYPE_EN enType;          /**<! ��ʱ�������� */

    void* pvContext;                  /**<! �����ģ���Żص�������Ҫ�Ĳ�����ע�⣬���ָ����Ƕ�̬������ڴ���Ҫ�ɵ��÷�ȥ�ͷ�(��ֹͣ��ʱ���ͳ�ʱʱ) */
    YK_TIMER_CALLBACK_FUNC pfCallback;   /**<! ��ʱ����ʱִ�еĻص�����ָ�� */

    unsigned long ulMagicNum;         /**<! �ڴ�����ʱ��ʱ���䣬����ɾ����ʱ��ʱ���ڷֱ��Ƿ��Ƕ�Ӧ�Ķ�ʱ�� */
}YK_TIMER_ST;

/**
 * @brief ��ʱ����ʼ��.
 *@return SUCCESS �ɹ�  FAILURE ʧ��
 */
int 	YKInitTimerMgmt();

/**@brief ��ʱ������.*/
void 	YKClearTimerMgmt();

/**
 *@brief		�Ƴ���ʱ��.
 *@param[in] 	pRemoved 	׼���Ƴ��Ķ�ʱ��.
 *@param[in] 	ulMagicNum 	�ڴ�����ʱ��ʱ���䣬����ɾ����ʱ��ʱ���ڷֱ��Ƿ��Ƕ�Ӧ�Ķ�ʱ��
 */
void	YKRemoveTimer(YK_TIMER_ST *pRemoved, unsigned long ulMagicNum);

/**@brief �Ƴ����ж�ʱ��.*/
void	YKRemoveAllTimers();

/**@brief ��ʱ�����Զ�ʱ����ʱ����Ϊ��������Ե��ñ�����������Ƿ��ж�ʱ����ʱ.*/
void	YKTimerTask();

/**
 * @brief 		��ʱ�����Զ�ʱ����ʱ����Ϊ��������Ե��ñ�����������Ƿ��ж�ʱ����ʱ.
 * @brief		��ʱ������Ϊ100ms
 * @param[in]	pfCallback	��ʱʱ�䵽Ҫִ�еĻص�����
 * @param[in]	pvContext	��Żص�������Ҫ�Ĳ���
 * @param[in]	ulInterval	��ʱ���
 * @param[in]	enTimerType	�Ƿ�ѭ������
 * @param[in]	pulMagicNum �ڴ�����ʱ��ʱ���䣬����ɾ����ʱ��ʱ���ڷֱ��Ƿ��Ƕ�Ӧ�Ķ�ʱ��
 * @return 		��ʱ���ṹ��ָ��
 */
YK_TIMER_ST*	YKCreateTimer(YK_TIMER_CALLBACK_FUNC pfCallback,void* pvContext,
						unsigned long ulInterval, YK_TIMER_TYPE_EN enTimerType,
                        unsigned long *pulMagicNum); //������ʱ��

//#ifdef __cplusplus
//};
//#endif

#endif //ifndef TIMER_H_
