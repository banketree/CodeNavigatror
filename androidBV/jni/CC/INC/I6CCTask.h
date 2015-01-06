/*********************************************************************
*  Copyright (C), 2001 - 2012, 福建邮科通信技术有限公司.
*  File			:
*  Author		:lcx
*  Version 		:
*  Date			:
*  Description	:
*  History		: 更新记录 .
*            	  <修改作者>  <修改时间>  <修改描述>
**********************************************************************/
#include "IDBT.h"

#ifdef _I6_CC_

#ifndef I6CCTASK_H_
#define I6CCTASK_H_

#include "YKMsgQue.h"


//CC 状态
#define		CCFSM_IDLE					0x01
#define		CCFSM_DELAY					0x02
#define		CCFSM_CALL_WAIT				0x03
#define 	CCFSM_PIC_WAIT				0x04
#define		CCFSM_PLAY_MUSIC			0x05
#define		CCFSM_TALKING				0x06
#define		CCFSM_VIDEO_MONIT			0x07


/*CC数据结构*/
//CC相关定时器超时时长
typedef struct
{
	int iCallDelay;					//呼叫延迟时间
	int iRingTime;					//振铃时长
	int iTalkingTimeout;			//通话超时
	int iMonitorTImeout;			//见
	int iPicConnectTimeout;			//个推连接超时时间
	int iPicRingTime;				//个推振铃时长
	int iOpenDoorTimeout;			//个推开门后挂断超时时间
}CC_TIMEOUT_VALUE_ST;

//全局变量声明
extern YK_MSG_QUE_ST*  g_pstCCMsgQ;

unsigned char *CCGetCurFsmRoom(void);

int CCGetCurSipCallIsValid(void);

int CCGetCurRoomNumCanCallSip(void);

int CCGetCcStatus(void);

void CCTimerCallBack(void *pvCtx);

void CCFsm(void);

void *CCTask(void *pvCtx);

void CCFsmIdleProcess(void *pvPrmvType);

void CCFsmDelayProcess(void *pvPrmvType);

void CCFsmCallProcess(void *pvPrmvType);

void CCFsmPicProcess(void *pvPrmvType);

void CCFsmPlayMusicProcess(void *pvPrmvType);

void CCFsmTalkingProcess(void *pvPrmvType);

void CCFsmVideoMonProcess(void *pvPrmvType);

#endif

#endif		//#ifdef _I6_CC_
