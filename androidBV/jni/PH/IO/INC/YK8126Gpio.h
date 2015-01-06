/**
 *@file   YK8126Gpio.h
 *@brief
 *
 *
 *@author chensq
 *@version 1.0
 *@date 2012-5-21
 */


#ifdef _YK_XT8126_

#ifndef YK8126_GPIO_H_
#define YK8126_GPIO_H_


#define USER_DEV_NAME                                        "/dev/user_apply"
#define _INIT_GPIO                                           0
#define _SET_GPIO_OUTPUT_HIGH                                1
#define _SET_GPIO_OUTPUT_LOW                                 2
#define _SET_GPIO_HIGH                                       3
#define _SET_GPIO_LOW                                        4
#define _SET_GPIO_INPUT                                      5
#define _GET_GPIO_VALUE                                      6
#define OPENLOCK_IO                                          92   //GPIO2     28
#define PWM_IO                                               3    //PWMGPIO0  3
#define LCD_LIGHT_IO                                         2
#define WATCHDOG_IO_CLEAR                                    9    //WatchDog   GPIO0  9
#define WATCHDOG_IO_EN                                       93   //Watchdog   GPIO2 29




extern int g_i8126IoFd;

/**
 * @brief        8126GPIO资源初始化
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int YK8126GpioInit(void);


/**
 * @brief        8126GPIO资源清理
 * @param[in]    无
 * @return       无
 */
void YK8126GpioUninit(void);


/**
 * @brief        8126GPIO资源初始化
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int YK8126GpioInit2(void);

/**
 * @brief        8126GPIO资源清理
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int YK8126GpioUninit2(void);

/**
 * @brief        8126GPIO设置(高)
 * @param[in]    无
 * @return       无
 */
void YK8126IOSetHigh(unsigned char ucIo);
/**
 * @brief        8126GPIO设置(低)
 * @param[in]    无
 * @return       无
 */
void YK8126IOSetOutputLow(unsigned char ucIo);

/**
 * @brief        8126GPIO设置(高)
 * @param[in]    无
 * @return       无
 */
void YK8126IOSetOutputHigh(unsigned char ucIo);
/**
 * @brief        8126GPIO设置(低)
 * @param[in]    无
 * @return       无
 */
void YK8126IOSetLow(unsigned char ucIo);

/**
 * @brief        8126 开启补光
 * @param[in]    无
 * @return       无
 */
void YK8126OpenFillLight(void);

/**
 * @brief        8126 关闭补光
 * @param[in]    无
 * @return       无
 */
void YK8126CloseFillLight(void);

/**
 * @brief        8126 打开锁
 * @param[in]    无
 * @return       无
 */
void YK8126OpenLock(void);          // add by cs 

/**
 * @brief        8126 关闭锁
 * @param[in]    无
 * @return       无
 */
void YK8126CloseLock(void);         // add by cs 


#endif /* YK8126_GPIO_H_ */

#endif
