/*********************************************************************
*  Copyright (C), 2001 - 2012, �����ʿ�ͨ�ż������޹�˾.
*  File			:
*  Author		:��̴�
*  Version 		:
*  Date			:
*  Description	:
*  History		: ���¼�¼ .
*            	  <�޸�����>  <�޸�ʱ��>  <�޸�����>
**********************************************************************/
#include "IDBT.h"

#ifndef _I6_CC_

#ifndef CCTASK_H_
#define CCTASK_H_
//ͷ�ļ�����

//��ϵͳ���

//�빤�����

//�궨��
//FSM Type Definition
#define		CCFSM_IDLE					0x01
#define		CCFSM_DELAY					0x02
#define		CCFSM_SIP1_CALL_WAIT		0x03
#define		CCFSM_TEL1_CALL_WAIT		0x04
#define		CCFSM_SIP2_CALL_WAIT		0x05
#define		CCFSM_TEL2_CALL_WAIT		0x06
#define		CCFSM_PLAY_MUSIC			0x07
#define		CCFSM_TALKING				0x08
#define		CCFSM_VIDEO_MONIT			0x09
#define		CCFSM_DEMO_CALL				0x0A
#define     CCFSM_PRIMARY_CALL_WAIT     0x0B

#define     NUM_VALID					0x01		                //����Ϸ�
#define		NUM_INVALID					0x02		                //����Ƿ�

//CC �ڲ���ʱ����ϢID
#define		TIMERID_DELAY_CALL			0x01						//˽�� 	��ʱ���ж�ʱ��
#define		TIMERID_CALL_WAIT			0x02						//˽��	���еȴ���ʱ��
#define		TIMERID_TALK_WAIT			0x03						//˽��	ͨ����ʱ��ʱ��
#define		TIMERID_PIC_RESULT_WAIT		0x04						//˽��	ͨ����ʱ��ʱ��

#define		ERR_TIMEOUT_CALL_WAIT		"0901"
#define        ERR_CALLEE_REJECT           "0902"

//ROOM NUM Judge
#define	 	ROOM_PHONE_NUM_N			0x01						//û�п��ú���
#define		ROOM_PHONE_NUM_Y			0x02						//���ڿ��ú���

//Verif RoomNum
#define     SIP1_Y                      0x01  	                    //SIP1�������
#define     SIP1_N_TEL1_Y               0x02                        //SIP1���벻����  TEL1�������

#define     SIP1_Y_OTHR_N               0x03                        //SIP1�������    �������벻����
#define     SIP1_Y_TEL1_Y               0x04                        //SIP1�������    TEL1�������
#define     SIP1_Y_TEL1_N_SIP2_Y        0x05                        //SIP1�������	  TEL1���벻����    SIP2����
#define     SIP1_Y_TEL1_N_SIP2_N_TEL2_Y 0x06                        //SIP1�������    TEL1���벻����    SIP2������   TEL2����

#define     TEL1_Y_OTHR_N               0x07                        //                TEL1�������      �������벻����
#define     TEL1_Y_SIP2_Y               0x08                        //                TEL1�������      SIP2����
#define     TEL1_Y_SIP2_N_TEL2_Y        0x09                        //                TEL1�������      SIP2������   TEL2����


#define     SIP2_Y_TEL2_N               0x0a                        //								    SIP2����     TEL2������
#define     SIP2_Y_TEL2_Y               0x0b                        //								    SIP2����	 TEL2����
//Phone Type
#define		PHONE_TYPE_SIP1             0X01
#define		PHONE_TYPE_TEL1             0X02
#define		PHONE_TYPE_SIP2             0X03
#define		PHONE_TYPE_TEL2             0X04


//ȫ�ֱ�������
//extern YK_MSG_QUE_ST*  g_pstSMMsgQ;
extern YK_MSG_QUE_ST*  g_pstCCMsgQ;



//��������
//���غ�������
char *CCGetCcStatus(void);
char *CCGetCurFsmRoom(void);
int CCTaskInit(void);
void CCTaskUninit(void);
void *CCTask(void);
void CCFsm(void);
void CCFsmIdleProcess(void *pvPrmvType);
void CCFsmDelayProcess(void *pvPrmvType);
void CCFsmSip1Process(void *pvPrmvType);
void CCFsmTel1Process(void *pvPrmvType);
void CCFsmSip2Process(void *pvPrmvType);
void CCFsmTel2Process(void *pvPrmvType);
void CCFsmPlayMusicProcess(void *pvPrmvType);
void CCFsmTalkingProcess(void *pvPrmvType);
void CCFsmVideoMonProcess(void *pvPrmvType);
void CCSIPTerminate(unsigned long ulIncomingCallId);
void CCSIPTerminateAll(void);
void CCSIPCall(unsigned char *pucPhoneNum,unsigned char ucMediaType,unsigned char ucHintMusicEn);
void CCSIPAnswer(unsigned long ulIncomingCallId);
void CCSMOpenDoor(void);
void CCSMSendDTMF(int iDtmfType);
void CCSMCalleeErr(unsigned int uiErrorCause);
void CCSMCalleePickUp(void);
void CCSMCalleeHangOff(void);
void CCSMCalleeMusicEnd(void);
unsigned int CCGetTime4DelayCall(unsigned char *pucRoomNum,unsigned int uiRoomNumLen);
unsigned int CCGetTime4CallWait(unsigned char *pucRoomNum,unsigned int uiRoomNumLen);
unsigned char *CCGetPhoneNum(unsigned char *pucRoomNum,unsigned char ucPhoneType);
unsigned char CCGetMediaType(unsigned char *pucRoomNum,unsigned char ucPhoneType);
unsigned char CCGetHintMusicEn(unsigned char *pucRoomNum);
int VerifyRoomNum(unsigned char *pucRoomNum,int iCCFsmState);
int IsRoomPhoneNumExist(unsigned char *pucRoomNum,unsigned int uiRoomNumLen);
int VideoIncomingVerifyPhoneNum(unsigned char *pucPhoneNum,char* pcHouseCode,unsigned int PhoneNumLen);
int AudioIncomingVerifyPhoneNum(unsigned char *pucPhoneNum,char* pcHouseCode,unsigned int PhoneNumLen);
void CCTimerCallBack(void *pCtx);
void CCFsmDemoCallProcess(void *pvPrmvType);
void CCFsmPrimaryCallProcess(void *pvPrmvType);
void CCTe1Call(void);
void CCTe2Call(void);
void InitSipCallFlagAndSocketConnectFlag(void);
void CCTimerPicResultCallBack(void *pCtx);
int GetPictureSize(char *file);
void CCTelCall(unsigned char ucTelType);
void DealTakePicState(unsigned int uiPrmvType,unsigned char ucTelType);
void CCDealTakePic(unsigned char ucTelType);
void CCSSCapturePicture(const char *SipNum);
//�ⲿ��������
#endif

#endif
