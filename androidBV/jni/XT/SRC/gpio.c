#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <poll.h>


#define CommonH
#include "common.h"

int user_fd = -1;

int Init_Gpio(void);     //初始化Gpio
int Uninit_Gpio(void);   //释放Gpio
void gpio_set_high(unsigned char io);
void gpio_set_low(unsigned char io);

//开锁函数
void OpenLock_Func(void);
//关锁函数
void CloseLock_Func(void);

//开启补光函数
void OpenFillLight_Func(void);
//关闭补光函数
void CloseFillLight_Func(void);

//开启LCD 背光函数
void OpenLcdLight_Func(void);
//关闭LCD 背光函数
void CloseLcdLight_Func(void);
//---------------------------------------------------------------------------
int Init_Gpio(void)
{
  pthread_attr_t attr;

  user_fd = open(USER_DEV_NAME, O_RDONLY);
  printf("user_fd = %d\n", user_fd);
  if (user_fd < 0) {
		printf("Can't open %s.\n", USER_DEV_NAME);
		return -1;
	}

  //ioctl(user_fd, _INIT_GPIO, 0);

  ioctl(user_fd, _INIT_GPIO, 2);
  //开锁
  ioctl(user_fd, _SET_GPIO_OUTPUT_LOW, OPENLOCK_IO);


  //补光
  ioctl(user_fd, _SET_GPIO_OUTPUT_LOW, PWM_IO);

  //LCD背光
  ioctl(user_fd, _SET_GPIO_OUTPUT_HIGH, LCD_LIGHT_IO);
}
//---------------------------------------------------------------------------
int Uninit_Gpio(void)
{
  
  if(user_fd > 0)
    close(user_fd);

  user_fd = -1;
}
//---------------------------------------------------------------------------
void gpio_set_high(unsigned char io)
{
  if(user_fd > 0)
//    ioctl(user_fd, _SET_GPIO_OUTPUT_HIGH, io);
    ioctl(user_fd, _SET_GPIO_HIGH, io);
}
//---------------------------------------------------------------------------
void gpio_set_low(unsigned char io)
{
  if(user_fd > 0)
//    ioctl(user_fd, _SET_GPIO_OUTPUT_LOW, io);
    ioctl(user_fd, _SET_GPIO_LOW, io);
}
//---------------------------------------------------------------------------
//开锁函数
void OpenLock_Func(void)
{
  Local.OpenDoorFlag = 2;   //开锁标志 0 未开锁  1 开锁延时中  2 开锁中
  Local.OpenDoorTime = 0;   //时间
  printf("open door...\n");
  gpio_set_high(OPENLOCK_IO);
}
//--------------------------------------------------------------------------
//关锁函数
void CloseLock_Func(void)
{
  gpio_set_low(OPENLOCK_IO);
}
//--------------------------------------------------------------------------
//开启补光函数
void OpenFillLight_Func(void)
{
  gpio_set_high(PWM_IO);
}
//--------------------------------------------------------------------------
//关闭补光函数
void CloseFillLight_Func(void)
{
  gpio_set_low(PWM_IO);
}
//--------------------------------------------------------------------------
//开启LCD 背光函数
void OpenLcdLight_Func(void)
{
  gpio_set_high(LCD_LIGHT_IO);
}
//--------------------------------------------------------------------------
//关闭LCD 背光函数
void CloseLcdLight_Func(void)
{
  gpio_set_low(LCD_LIGHT_IO);
}
//--------------------------------------------------------------------------
