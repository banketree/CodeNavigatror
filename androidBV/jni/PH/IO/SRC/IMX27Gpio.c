/*
 * IMX27Gpio.c
 *
 *  Created on: 2012-9-26
 *      Author: root
 */


#ifdef _YK_IMX27_AV_


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <IMX27Gpio.h>


//#define GPIO_HIGH	1
//#define GPIO_LOW	0

#define IMX27_GPIO_DEV	"/dev/idbt_gpio"


typedef enum idbt_gpio_table
{
	GPIO_AK8856_nRESET = 0,
	GPIO_WM9712_nRESET,
	GPIO_QL9700_nRESET,
	GPIO_IDBT_LED1,
	GPIO_IDBT_LED2,
};



static int g_IMX27GpioFd = -1;


int IMX27GpioInit(void)
{
	g_IMX27GpioFd = open(IMX27_GPIO_DEV, O_RDONLY);
	if (g_IMX27GpioFd < 0)
	{
		printf("error1:open io fd error[IMX27GpioInit:%s/%d]\n", IMX27_GPIO_DEV, g_IMX27GpioFd);
		return -1;
	}
	else
	{
		printf("debug1:open io fd ok[IMX27GpioInit:%s(%d)]\n", IMX27_GPIO_DEV, g_IMX27GpioFd);
	}

	ioctl(g_IMX27GpioFd, IMX27_GPIO_LOW, GPIO_IDBT_LED1);

	ioctl(g_IMX27GpioFd, IMX27_GPIO_LOW, GPIO_IDBT_LED2);

	return g_IMX27GpioFd;
}

int IMX27GpioUninit(void)
{
	if(g_IMX27GpioFd > 0)
	{
		if(close(g_IMX27GpioFd) == 0)
		{
			printf("debug:close gpio fd ok\n");
		}
		else
		{
			printf("debug:close gpio fd error\n");
		}
	}

	g_IMX27GpioFd = -1;
}


void IMX27GpioSetLed1(IMX27_GPIO_VALUE_EN value)
{
	if(g_IMX27GpioFd > 0)
	{
		ioctl(g_IMX27GpioFd, value, GPIO_IDBT_LED1);
	}

}

void IMX27GpioSetLed2(IMX27_GPIO_VALUE_EN value)
{
	if(g_IMX27GpioFd > 0)
	{
		ioctl(g_IMX27GpioFd, value, GPIO_IDBT_LED2);
	}
}

/*
int main(void)
{
	IMX27GpioInit();

	sleep(3);

	while(1)
	{

		sleep(1);

		ioctl(g_IMX27GpioFd, GPIO_HIGH, GPIO_IDBT_LED1);

		ioctl(g_IMX27GpioFd, GPIO_HIGH, GPIO_IDBT_LED2);

		sleep(1);

		ioctl(g_IMX27GpioFd, GPIO_LOW, GPIO_IDBT_LED1);

		ioctl(g_IMX27GpioFd, GPIO_LOW, GPIO_IDBT_LED2);
	}


}
*/





#endif

