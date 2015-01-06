/*
 * SMDtmfSerial.c
 *
 *  Created on: 2012-10-11
 *      Author: zhengWenxiang
 */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include <YKSystem.h>
#include "../INC/SMDtmfSerial.h"
#include "../INC/SMUtil.h"
#include "../INC/SMSerial.h"
#include "TMInterface.h"
#include "LOGIntfLayer.h"
#include "SMXt8126SndTool.h"
#include "../INC/SMEventHndlImx27.h"
#include "../INC/YKApi.h"

static int g_iComDtmfFd = 0;
static BOOL DtmfTask_flag = TRUE;
static pthread_t g_dtmfIdHndl;


void *SMDtmfSerialCore(void *pvPara);

int SMDtmfInit(void)
{
	g_iComDtmfFd = SMOpenComPort(2, 9600, 8, "1", 'N');     //DTMF信号通讯串口
	if (-1 == g_iComDtmfFd)
	{
		LOG_RUNLOG_ERROR("SM: com DTMF Open failed!");
		return FAILURE;
	}

	if(pthread_create(&g_dtmfIdHndl, NULL, SMDtmfSerialCore, NULL) < 0)
	{
		LOG_RUNLOG_ERROR("SM: create DTMF serial event hndl thread failed!");
		return FAILURE;
	}

	return SUCCESS;

}

void *SMDtmfSerialCore(void *pvPara)
{

	unsigned char cDtmfData= '0';
	DtmfTask_flag = TRUE;
	printf("enter SMDtmfSerialCore \n");
	while(DtmfTask_flag)
	{
		//receive DTMF flag
		if(read(g_iComDtmfFd, &cDtmfData, 1)>0)
		{
			LOG_RUNLOG_DEBUG("SM: DTMF Type: %c ", cDtmfData);
			switch(cDtmfData)
			{
				case '0':
					break;
				case '1':
					break;
				case '2':
					break;
				case '3':
					break;
				case '4':
					break;
				case '5':
					break;
				case '6':
					break;
				case '7':
					break;
				case '8':
					break;
				case '9':
					break;
				case '*':
					break;
				case '#':
					SMOpenDoor();
					LOG_RUNLOG_INFO("SM open door!");
					break;
				default:
					break;

			}
		}
		usleep(100 * 1000);
	}


}


void SMDtmfFini(void)
{
	DtmfTask_flag = FALSE;
	SMCloseComPort(g_iComDtmfFd);
	YKJoinThread((void *)g_dtmfIdHndl);
	YKDestroyThread((void *)g_dtmfIdHndl);
}











