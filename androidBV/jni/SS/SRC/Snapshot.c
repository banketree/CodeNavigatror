/*
 * SMCapturePicture.c
 *
 *  Created on: 2013-1-17
 *      Author: zhengwx
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


#include "YKApi.h"
#include "YKTimer.h"
#include "YKMsgQue.h"
#ifdef _I6_CC_
#include "I6CCTask.h"
#else
#include "CCTask.h"
#endif
#include "IDBT.h"
#include "LPIntfLayer.h"
#include "LOGIntfLayer.h"
#include "TMInterface.h"
#include "FPInterface.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../INC/Snapshot.h"
#ifdef _YK_XT8126_
extern V8126StartCapturePic(void);
#endif
#define 	PICTURE_SIZE   4500  //b
#define PIC_TAKE_TIMEOUT 20000 	//��λms
#define CAPTURE_PIC_FILE   "/data/data/com.fsti.ladder/capture.jpeg"

#define PICTURE_DELAY   1500   //��λMS,�����豸200ms,�����豸1500ms

#ifndef _YK_PC_
	#ifdef _YK_IMX27_AV_
		#define PICTURE_SIZE          3500 //��λB
	#elif defined(_YK_XT8126_)
		#define PICTURE_SIZE          1000 //��λB
	#endif
#endif





#define SS_QUE_LEN 1024

YK_MSG_QUE_ST*   g_pstSSMsgQ = NULL;
YKHandle g_SSThreadId = NULL;


extern int g_iRunSSTaskFlag;


/*
 *	��ȡͼƬ��С
 */
int GetPictureSize(char *file)
{
	struct stat info;
	if (stat(file, &info) != -1)
	{
		return (info.st_size);
	}
	else
	{
		  LOG_RUNLOG_DEBUG("Picture size is 0");
		  return 0;
	}

}


int CapturePicture(void)                              // ץ����Ƭ
{
	LOG_RUNLOG_DEBUG("SS CapturePicture");
#ifdef _CAPTURE_PIC_
#ifdef _YK_IMX27_AV_
    char cmd[128] = {0};
    system("rm /home/*.jpeg");
    LOG_RUNLOG_DEBUG("SS CapturePicture : remove old pic file");
    LOG_RUNLOG_DEBUG("SS CapturePicture : capture begin");
    LOG_RUNLOG_DEBUG("SS CapturePicture sleep %d ms", PICTURE_DELAY);
    usleep(PICTURE_DELAY * 1000);	
    return CPCapturePic(CAPTURE_PIC_FILE);
#elif defined(_YK_XT8126_)
    system("rm -f /home/*.jpeg");
    LOG_RUNLOG_DEBUG("SS CapturePicture : remove old pic file");
	LOG_RUNLOG_DEBUG("SS CapturePicture : capture begin");
    V8126StartCapturePic();
    return TRUE;
#endif


//    system("busybox rm -f /data/data/com.fsti.ladder/*.jpeg");
//    LOG_RUNLOG_DEBUG("SS CapturePicture : remove old pic file");

	LOG_RUNLOG_DEBUG("SS CapturePicture : capture begin");
	int r = capture_pic();

	LOG_RUNLOG_DEBUG("SS CapturePicture : capture finish:%d", r);
#endif
    return TRUE;
}

int UploadPicture(const char *phoneNum)               // �ϴ���Ƭ
{
#ifdef _CAPTURE_PIC_
    time_t stTimep;
	struct tm *pstTm;
    char name[128] = {0};
    char local[128] = {0};
    char remote[128] = {0};
    char cmd[128] = {0};
    char num[64] = {0};
    char *tmp;
    int ret = 0;
    int  iSize = 0;

    iSize = GetPictureSize(CAPTURE_PIC_FILE);
    LOG_RUNLOG_DEBUG("Get Picture size is %d Byte", iSize);
    LOG_RUNLOG_DEBUG("Current set picture will upload min value is %d Byte", PICTURE_SIZE);
    if (iSize < PICTURE_SIZE )
    {
    	LOG_RUNLOG_INFO("Picture size is too small ,so will not upload!");
    	return FALSE;
    }

    LOG_RUNLOG_DEBUG("SS UploadPicture -> begin");

    strcpy(num, phoneNum);
    tmp = strstr(num, "@");
    if (tmp != NULL)
    {
        *tmp = 0;
    }

    LOG_RUNLOG_DEBUG("SS UploadPicture : phoneNum:%s, num:%s", phoneNum, num);

    	time(&stTimep);
	pstTm = localtime(&stTimep);

    snprintf(name, sizeof(name), "%s_%04d%02d%02d%02d%02d%02d%s",
    								num,
    								1900 + pstTm->tm_year,
    								1 + pstTm->tm_mon,
    								pstTm->tm_mday,
    								pstTm->tm_hour,
    								pstTm->tm_min,
    								pstTm->tm_sec,
    								".jpeg");


    sprintf(remote, "%s%s%s", "./", "temp_",name);	//�ϴ�û����ɳɹ�ǰ�����ļ���ǰ��temp_
    LOG_RUNLOG_DEBUG("SS UploadPicture -> upload file");

    ret = UploadFile(CAPTURE_PIC_FILE, remote);

//    LOG_RUNLOG_DEBUG("SS UploadPicture : remove file");
//    sprintf(cmd, "rm %s", local);
//    system(cmd);

    return ret;
#endif
    return TRUE;
}

int UploadFile(char *local, char *remote)
{
    char acUserName[256], acPassWord[256], pacHost[256]; int iPort=21;
    char acUser[256];

    if ((local == NULL) || (remote == NULL))
    {
    	return FALSE;
    }
    LOG_RUNLOG_DEBUG("SS CPUploadPic : local:%s, remote:%s", local, remote);

    TMQueryFtpInfo(pacHost, &iPort, acUserName, acPassWord);

    sprintf(acUser,"%sjpg",acUserName);

    FPPutPicture(pacHost, iPort, acUser, acPassWord, local, remote, NULL, NULL, FP_TRANSFER_NOWAIT);

    return TRUE;
}

/*
 * Snapshot ������
 */
int  SSTask(void)
{
    int iErrCode;
    void *pvPrmvType = NULL;

    while(g_iRunSSTaskFlag)		                                    //SSTask���б�־Ϊ��
    {
        iErrCode = YKReadQue( g_pstSSMsgQ, &pvPrmvType,  0);			//������ȡ��Ϣ
        if ( 0 != iErrCode || NULL == pvPrmvType )
        {
        	LOG_RUNLOG_ERROR("SS iErrCode = [%d],  &pvPrmvType= [%p] ", iErrCode, pvPrmvType);
        	continue;
        }

        if (SS_SNAPSHOT == *((unsigned int *)pvPrmvType))			//ץ��
        {
            SSProcessMessage((SS_MSG_DATA_ST*) pvPrmvType);
        }
        else if (SS_CANCEL_EVENT == *((unsigned int *)pvPrmvType))		//�˳��߳�
        {
        	g_iRunSSTaskFlag = FALSE;
        	continue;
        }

    }
    return TRUE;
}

void SSProcessMessage(SS_MSG_DATA_ST* msg)
{
	const char * SipNum = msg->SipNum;

    gettimeofday(&g_CapturePictureStart, NULL);		//����ǰʱ���¼
	if ( FALSE == CapturePicture())			//����
	{
		LOG_RUNLOG_ERROR("SS capture picture failed!");
		return ;
	}

	gettimeofday(&g_CapturePictureEnd, NULL);	//������ʱ���¼
	UploadPicture(SipNum);					//�ϴ�ͼƬ

}

/*
 * ͼ��ץ�ĳ�ʼ��
 */
int SnapshotInit(void)
{
	LOG_RUNLOG_DEBUG("SS SnapshotInit begin");
	g_iRunSSTaskFlag = TRUE;		                 //Snapshot ���б�־��λ
	g_pstSSMsgQ = YKCreateQue(SS_QUE_LEN);			//������Ϣ����

	if (NULL == g_pstSSMsgQ)
	{
		LOG_RUNLOG_ERROR("SS can't create message queue ");
		return FALSE;
	}

	g_SSThreadId = YKCreateThread(SSTask,NULL);
    if(NULL == g_SSThreadId)
    {
    	LOG_RUNLOG_ERROR("SS can't create SSTask Thread  ");
        return FALSE;
    }

    LOG_RUNLOG_DEBUG("SS INIT OK");
    return SUCCESS;


}


/*
 *	ͼƬץ������ж��
 */
void SSFini(void)
{
	int iErrCode;
	SS_MSG_DATA_ST *pstSendMsg = (SS_MSG_DATA_ST *)malloc(sizeof(SS_MSG_DATA_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = SS_CANCEL_EVENT;	//�˳��߳�

	iErrCode = YKWriteQue( g_pstSSMsgQ , pstSendMsg,  0);
    if(NULL != g_SSThreadId)
    {
        YKJoinThread(g_SSThreadId);
        YKDestroyThread(g_SSThreadId);
    }

    YKDeleteQue(g_pstSSMsgQ);
}
