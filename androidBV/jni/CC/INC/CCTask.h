/*********************************************************************
*  Copyright (C), 2001 - 2012, 福建邮科通信技术有限公司.
*  File			:
*  Author		:李程达
*  Version 		:
*  Date			:
*  Description	:
*  History		: 更新记录 .
*            	  <修改作者>  <修改时间>  <修改描述>
**********************************************************************/
#include "IDBT.h"

#ifndef _I6_CC_

#ifndef CCTASK_H_
#define CCTASK_H_
//头文件包含

//与系统相关

//与工程相关

//宏定义
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

#define     NUM_VALID					0x01		                //号码合法
#define		NUM_INVALID					0x02		                //号码非法

//CC 内部定时器消息ID
#define		TIMERID_DELAY_CALL			0x01						//私有 	延时呼叫定时器
#define		TIMERID_CALL_WAIT			0x02						//私有	呼叫等待定时器
#define		TIMERID_TALK_WAIT			0x03						//私有	通话超时定时器
#define		TIMERID_PIC_RESULT_WAIT		0x04						//私有	通话超时定时器

#define		ERR_TIMEOUT_CALL_WAIT		"0901"
#define        ERR_CALLEE_REJECT           "0902"

//ROOM NUM Judge
#define	 	ROOM_PHONE_NUM_N			0x01						//没有可用号码
#define		ROOM_PHONE_NUM_Y			0x02						//存在可用号码

//Verif RoomNum
#define     SIP1_Y                      0x01  	                    //SIP1号码存在
#define     SIP1_N_TEL1_Y               0x02                        //SIP1号码不存在  TEL1号码存在

#define     SIP1_Y_OTHR_N               0x03                        //SIP1号码存在    其他号码不存在
#define     SIP1_Y_TEL1_Y               0x04                        //SIP1号码存在    TEL1号码存在
#define     SIP1_Y_TEL1_N_SIP2_Y        0x05                        //SIP1号码存在	  TEL1号码不存在    SIP2存在
#define     SIP1_Y_TEL1_N_SIP2_N_TEL2_Y 0x06                        //SIP1号码存在    TEL1号码不存在    SIP2不存在   TEL2存在

#define     TEL1_Y_OTHR_N               0x07                        //                TEL1号码存在      其他号码不存在
#define     TEL1_Y_SIP2_Y               0x08                        //                TEL1号码存在      SIP2存在
#define     TEL1_Y_SIP2_N_TEL2_Y        0x09                        //                TEL1号码存在      SIP2不存在   TEL2存在


#define     SIP2_Y_TEL2_N               0x0a                        //								    SIP2存在     TEL2不存在
#define     SIP2_Y_TEL2_Y               0x0b                        //								    SIP2存在	 TEL2存在
//Phone Type
#define		PHONE_TYPE_SIP1             0X01
#define		PHONE_TYPE_TEL1             0X02
#define		PHONE_TYPE_SIP2             0X03
#define		PHONE_TYPE_TEL2             0X04


//全局变量声明
//extern YK_MSG_QUE_ST*  g_pstSMMsgQ;
extern YK_MSG_QUE_ST*  g_pstCCMsgQ;



//函数声明
//本地函数声明
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
//外部函数声明
#endif

#endif
