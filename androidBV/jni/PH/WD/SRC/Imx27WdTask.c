/**
 *@file        Imx27WdTask.c
 *@brief
 *@author      chensq
 *@version     1.0
 *@date        2012-7-11
 */


#ifdef _YK_IMX27_AV_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <LOGIntfLayer.h>

#define WD_TIMEOUT             60 //60s
#define WD_KEEPALIVE_TIME      2  //2s


int g_iImx27Wdfd = -1;
YKHandle g_WDThreadId;

void *Imx27WdTask(void* arg)
{
    int iErrCode;

    while(g_iRunWDTaskFlag)
    {
    	iErrCode = ioctl(g_iImx27Wdfd,WDIOC_KEEPALIVE,0);
    	//LOG_RUNLOG_DEBUG("PH Imx27WdTask WDIOC_KEEPALIVE");
		sleep(WD_KEEPALIVE_TIME);
    }
}


int Imx27WdTaskInit(void)
{
	int iRet = -1;
	int iTimeout = WD_TIMEOUT;

	g_iImx27Wdfd = open("/dev/misc/watchdog",O_WRONLY);//default timer:60s
	if(g_iImx27Wdfd < 0)
	{
		LOG_RUNLOG_ERROR("PH Imx27WdTaskInit dev/misc/watchdog can't open");
		return FAILURE;
	}
	else
	{
		LOG_RUNLOG_DEBUG("PH Imx27WdTaskInit /dev/misc/watchdog/: fd=%d", g_iImx27Wdfd);
	}


	LOG_RUNLOG_DEBUG("PH Imx27WdTaskInit will set imx27 wd timeout:%d\n", iTimeout);
	iRet = ioctl(g_iImx27Wdfd,WDIOC_SETTIMEOUT,&iTimeout);
	if(iRet != 0)
	{
		LOG_RUNLOG_WARN("PH Imx27WdTaskInit set imx27 wd timeout fail,default=60 sec");
	}

	LOG_RUNLOG_DEBUG("PH Imx27WdTaskInit 1:%d", ioctl(g_iImx27Wdfd,WDIOC_GETTIMEOUT,&iTimeout));//0
	LOG_RUNLOG_DEBUG("PH Imx27WdTaskInit 2:%d", iTimeout);//=set value


	//load hd driver
//	system("/usr/sbin/telnetd &");
	system("./exec_second.sh");

    g_WDThreadId = YKCreateThread(Imx27WdTask, NULL);

    if(NULL == g_WDThreadId)
    {
    	LOG_RUNLOG_ERROR("PH Imx27WdTaskInit FAILURE");
        return FAILURE;
    }
    LOG_RUNLOG_DEBUG("PH Imx27WdTaskInit SUCCESS");
    return SUCCESS;
}


void Imx27WdTaskFini(void)
{
    g_iRunWDTaskFlag = FALSE;

    if(NULL != g_WDThreadId)
    {
        YKJoinThread(g_WDThreadId);
        YKDestroyThread(g_WDThreadId);
    }
    LOG_RUNLOG_DEBUG("PH Imx27WdTaskFini");
}



#endif
