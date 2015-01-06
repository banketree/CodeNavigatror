/*********************************************************************
*  Copyright (C), 2001 - 2012, �����ʿ�ͨ�ż������޹�˾.
*  File			:
*  Author		:lcx
*  Version 		:
*  Date			:
*  Description	:
*  History		: ���¼�¼ .
*            	  <�޸�����>  <�޸�ʱ��>  <�޸�����>
**********************************************************************/
#include "IDBT.h"

#ifdef _I6_CC_

#ifndef I6CCTASK_H_
#define I6CCTASK_H_

#include "YKMsgQue.h"


//CC ״̬
#define		CCFSM_IDLE					0x01
#define		CCFSM_DELAY					0x02
#define		CCFSM_CALL_WAIT				0x03
#define 	CCFSM_PIC_WAIT				0x04
#define		CCFSM_PLAY_MUSIC			0x05
#define		CCFSM_TALKING				0x06
#define		CCFSM_VIDEO_MONIT			0x07


/*CC���ݽṹ*/
//CC��ض�ʱ����ʱʱ��
typedef struct
{
	int iCallDelay;					//�����ӳ�ʱ��
	int iRingTime;					//����ʱ��
	int iTalkingTimeout;			//ͨ����ʱ
	int iMonitorTImeout;			//��
	int iPicConnectTimeout;			//�������ӳ�ʱʱ��
	int iPicRingTime;				//��������ʱ��
	int iOpenDoorTimeout;			//���ƿ��ź�Ҷϳ�ʱʱ��
}CC_TIMEOUT_VALUE_ST;

//ȫ�ֱ�������
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
