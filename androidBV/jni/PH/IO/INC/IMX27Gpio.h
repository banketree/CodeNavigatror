/*
 * IMX27Gpio.h
 *
 *  Created on: 2012-9-26
 *      Author: root
 */



#ifndef _IMX27GPIO_H_
#define _IMX27GPIO_H_

typedef enum
{
	IMX27_GPIO_LOW = 0,
	IMX27_GPIO_HIGH
}IMX27_GPIO_VALUE_EN;




int IMX27GpioInit(void);
int IMX27GpioUninit(void);
void IMX27GpioSetLed1(IMX27_GPIO_VALUE_EN value);//net led
void IMX27GpioSetLed2(IMX27_GPIO_VALUE_EN value);//sip led

#endif
