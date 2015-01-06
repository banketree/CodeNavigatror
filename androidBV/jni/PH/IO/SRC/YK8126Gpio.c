/**
 *@file   YK8126Gpio.c
 *@brief
 *
 *
 *@author chensq
 *@version 1.0
 *@date 2012-5-21
 */


#ifdef _YK_XT8126_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <YK8126Gpio.h>



int g_i8126IoFd = -1;


/**
 * @brief        8126GPIO资源初始化
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int YK8126GpioInit(void)
{
  	g_i8126IoFd = open(USER_DEV_NAME, O_RDONLY);
  	if (g_i8126IoFd < 0)
	{
		printf("error:open io fd error[YK8126GpioInit:%s]\n", USER_DEV_NAME);
		return -1;
	}
  	else
  	{
  		printf("debug:open io fd ok[YK8126GpioInit:%s(%d)]\n", USER_DEV_NAME, g_i8126IoFd);
  	}

    ioctl(g_i8126IoFd, _INIT_GPIO, 2);
    //开锁
    ioctl(g_i8126IoFd, _SET_GPIO_OUTPUT_LOW, OPENLOCK_IO);
    //补光
    ioctl(g_i8126IoFd, _SET_GPIO_OUTPUT_LOW, PWM_IO);
    //LCD背光
    ioctl(g_i8126IoFd, _SET_GPIO_OUTPUT_HIGH, LCD_LIGHT_IO);

  	return g_i8126IoFd;
}


/**
 * @brief        8126GPIO资源清理
 * @param[in]    无
 * @return       无
 */
void YK8126GpioUninit(void)
{
	if(g_i8126IoFd > 0)
	{
		if(close(g_i8126IoFd)== 0)
			printf("debug:close gpio fd ok\n");
		else
			printf("debug:close gpio fd error\n");
	}
	g_i8126IoFd = -1;
}


/**
 * @brief        8126GPIO资源初始化
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int YK8126GpioInit2(void)
{
	printf("debug:YK8126GpioInit2 g_i8126IoFd=%d\n", g_i8126IoFd);
	return g_i8126IoFd;
}


/**
 * @brief        8126GPIO资源清理
 * @param[in]    无
 * @return       0:成功  其他:失败
 */
int YK8126GpioUninit2(void)
{
	printf("debug:YK8126GpioUninit2 g_i8126IoFd=%d\n", g_i8126IoFd);
	return 0;
}


/**
 * @brief        8126GPIO设置(高)
 * @param[in]    无
 * @return       无
 */
void YK8126IOSetHigh(unsigned char ucIo)
{
  if(g_i8126IoFd > 0)
    ioctl(g_i8126IoFd, _SET_GPIO_HIGH, ucIo);
  else
	  printf("error:g_i8126IoFd error[%d]\n", g_i8126IoFd);
}

/**
 * @brief        8126GPIO设置(低)
 * @param[in]    无
 * @return       无
 */
void YK8126IOSetLow(unsigned char ucIo)
{
    if(g_i8126IoFd > 0)
        ioctl(g_i8126IoFd, _SET_GPIO_LOW, ucIo);
    else
        printf("error:g_i8126IoFd error[%d]\n", g_i8126IoFd);
}



/**
 * @brief        8126GPIO设置(高)
 * @param[in]    无
 * @return       无
 */
void YK8126IOSetOutputHigh(unsigned char ucIo)
{
  if(g_i8126IoFd > 0)
    ioctl(g_i8126IoFd, _SET_GPIO_OUTPUT_HIGH, ucIo);
  else
	  printf("error:g_i8126IoFd error[%d]\n", g_i8126IoFd);
}

/**
 * @brief        8126GPIO设置(低)
 * @param[in]    无
 * @return       无
 */
void YK8126IOSetOutputLow(unsigned char ucIo)
{
    if(g_i8126IoFd > 0)
        ioctl(g_i8126IoFd, _SET_GPIO_OUTPUT_LOW, ucIo);
    else
        printf("error:g_i8126IoFd error[%d]\n", g_i8126IoFd);
}

/**
 * @brief        8126 开启补光
 * @param[in]    无
 * @return       无
 */
void YK8126OpenFillLight(void)
{
	if(g_i8126IoFd < 0)
	{
		printf("error:YK8126OpenFillLight g_i8126IoFd error[%d]\n", g_i8126IoFd);
	}
	else
	{
		printf("debug:YK8126OpenFillLight[%d]\n", g_i8126IoFd);
	}
	YK8126IOSetHigh(PWM_IO);
}

/**
 * @brief        8126 关闭补光
 * @param[in]    无
 * @return       无
 */
void YK8126CloseFillLight(void)
{
	if(g_i8126IoFd < 0)
	{
		printf("error:YK8126CloseFillLight g_i8126IoFd error[%d]\n", g_i8126IoFd);
	}
	else
	{
		printf("debug:YK8126CloseFillLight[%d]\n", g_i8126IoFd);
	}
	YK8126IOSetLow(PWM_IO);
}

/**************************************************************************
add by cs 
**************************************************************************/
//开锁函数
void YK8126OpenLock(void)
{
    YK8126IOSetHigh(OPENLOCK_IO);
}
//--------------------------------------------------------------------------
//关锁函数
void YK8126CloseLock(void)
{
    YK8126IOSetLow(OPENLOCK_IO);
}



#endif
