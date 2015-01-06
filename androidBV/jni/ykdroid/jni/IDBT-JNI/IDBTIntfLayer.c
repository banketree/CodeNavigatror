/*
 * IdbtIntfLayer.c
 *
 *  Created on: 2013-1-4
 *      Author: root
 */
#include "YKApi.h"
#include "YKTimer.h"
#include "YKMsgQue.h"
#include "IDBT.h"
#include "IDBTCfg.h"
#include "LPMsgOp.h"
#include "IDBTIntfLayer.h"
#include "LOGIntfLayer.h"
#include "TMInterface.h"
#include "YKSystem.h"
#ifdef _I6_CC_
#include "I6CCTask.h"
#else
#include "CCTask.h"
#endif


YK_MSG_QUE_ST  *g_pstABMsgQ;
YKHandle        g_IDBTIntfLayerThreadId;

#define PHONE_NUM_MAX_LEN			20

static IDBTJNI_REG_STATUS_CHANGED_CB   pfIDBTJniRegStatusChanged    = NULL;


int IDBTMsgSendABReboot(JNI_REBOOT_EN enReboot)
{
	int iRet = -1;
	if(NULL == g_pstABMsgQ)
	{
		LOG_RUNLOG_WARN("LP g_pstABMsgQ is null");
		return -1;
	}
	OTAB_REBOOT_ST* pstMsg= (OTAB_REBOOT_ST*)malloc(sizeof(OTAB_REBOOT_ST));
	if(pstMsg == NULL)
	{
		LOG_RUNLOG_ERROR("LP malloc IDBTMsgSendABReboot failed");
		return -1;
	}
	memset(pstMsg, 0x00, sizeof(OTAB_REBOOT_ST));

	if(enReboot == REMOTE_REBOOT)
	{
		pstMsg->uiPrmvType = OTAB_REBOOT;
	}
	else if(enReboot == CONFIG_REBOOT)
	{
		pstMsg->uiPrmvType = OTAB_CONFIG_REBOOT;
	}

	iRet = YKWriteQue( g_pstJNIMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

	LOG_RUNLOG_DEBUG("LP LPMsgQ send msg:\nuiPrmvType:OTAB_REBOOT | 0x%04x\n", OTAB_REBOOT);

	return iRet;
}

int ABMsgCntProcessReboot(void *pvMsg)
{
	int iErrCode;

	LOG_RUNLOG_DEBUG("AB ABMsgCntProcessReboot");
	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}

int SendHouseCode(const char *houseCode)
{
	int iErrCode;
	int iRet = 0;
	unsigned char roomNum[ROOM_NUM_BUFF_LEN] = {0};

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "IDBTInt SendHouseCode sendHouseCode:%s\n", houseCode);
	LOG_RUNLOG_DEBUG("IDBTInt SendHouseCode sendHouseCode:%s\n", houseCode);

	ABCC_DOOR_CALL_ST *pstMsg = (ABCC_DOOR_CALL_ST*)malloc(sizeof(ABCC_DOOR_CALL_ST));
	if(NULL == pstMsg)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "IDBTInt SendHouseCode malloc ABCC_DOOR_CALL_ST failed\n");
		return -1;
	}
	memset(pstMsg, 0, sizeof(ABCC_DOOR_CALL_ST));

	pstMsg->uiPrmvType = ABCC_DOOR_CALL;
	pstMsg->ucDevType = '0';
//	strncpy((char *)pstMsg->aucRoomNum, "0000", 4);
//	strncpy((char *)pstMsg->aucRoomNum+4, houseCode, 4);
	strncpy((char *)pstMsg->aucRoomNum, houseCode, strlen(houseCode));
	iRet = YKWriteQue(g_pstCCMsgQ , pstMsg,  0);

	return iRet;
}

int ValidatePassword(const char *password)
{
	LOG_RUNLOG_DEBUG("ValidatePassword:%s OpendoorPassword:%s", password, g_stIdbtCfg.acOpendoorPassword);
	int ret = 0;

//	return TMXTIsValidDoorPassword(password);
	if(strcmp(g_stIdbtCfg.acOpendoorPassword, password) == 0)
	{
		SM210OpenDoor();
		return TRUE;
	}

	if(TMCCIsPasswordOpendoorEnabled() == FALSE)
	{
		return FALSE;
	}
	ret = TMXTIsValidDoorPassword(password);
	if(ret == TRUE)
	{
		SM210OpenDoor();

		return TRUE;
	}
	else
	{
		return FALSE;
	}

}

void HangupCall(void)
{
	int iErrCode;

//	ABSIP_TERMINATE_ALL_ST *pstSendMsg = (ABSIP_TERMINATE_ALL_ST *)malloc(sizeof(ABSIP_TERMINATE_ALL_ST));
//
//	if(pstSendMsg == NULL)
//	{
//		return;
//	}
//
//	pstSendMsg->uiPrmvType = ABSIP_TERMINATE_ALL;
//
//	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);
	LOG_RUNLOG_DEBUG("***** ABCC hangupCall *****");

	ABCC_DOOR_HANG_OFF_ST *pstMsg = YKNew0(ABCC_DOOR_HANG_OFF_ST, 1);
	if(NULL == pstMsg)
	{
		return;
	}

	pstMsg->uiPrmvType = ABCC_DOOR_HANG_OFF;
	YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
}





int CheckImsCallEnable()
{
	int iRet;

	LOG_RUNLOG_DEBUG("IDBTInt CheckImsCallEnable");

	iRet = TMCCIsIMSCallEnabled();
	if(iRet == 1)
	{
		LOG_RUNLOG_DEBUG("IDBTInt TMCCIsIMSCallEnabled==1");
		return 0;
	}
	else																			//不存在房号对应的电话号码
	{
		LOG_RUNLOG_DEBUG("IDBTInt TMCCIsIMSCallEnabled!=1");
		return -1;
	}
}



int GetImsCallTimeout()
{
	int iRet;

	LOG_RUNLOG_DEBUG("IDBTInt GetImsCallTimeout");

	iRet = TMCCGetIMSCallTimeOut();

	LOG_RUNLOG_DEBUG("IDBTInt GetImsCallTimeout=%d", iRet);

	return iRet;
}


//int ABMsgCntProcessSipABAudioIncoming(void *pvMsg)
//{
//	int iErrCode;
//
////	SIPAB_AUDIO_INCOMING_ST *pstMsg = (SIPAB_AUDIO_INCOMING_ST *)pvMsg;
////
////	pstMsg->uiPrmvType = JNI_DEV_CALL_AUDIO_INCOMING_RECEIVED;
//
//	LOG_RUNLOG_DEBUG("YKWriteQue:g_pstJNIMsgQ-ABMsgCntProcessSipABAudioIncoming");
//	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);
//
//	return iErrCode;
//
//}

int TMMsgCntProcessJniAdvInfo(void *pvMsg)
{
	int iErrCode = -1;

	if(!g_pstJNIMsgQ)
	{
		LOG_RUNLOG_DEBUG("TM TMMsgCntProcessJniAdvInfo g_pstJNIMsgQ = NULL");
		return iErrCode;
	}

	LOG_RUNLOG_DEBUG("TM TMMsgCntProcessJniAdvInfo");

	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;

}

int ABMsgCntProcessCCABVideoIncoming(void *pvMsg)
{
	int iErrCode;

//	SIPAB_VIDEO_INCOMING_ST *pstMsg = (SIPAB_VIDEO_INCOMING_ST *)pvMsg;
//
//	pstMsg->uiPrmvType = JNI_DEV_CALL_VIDEO_INCOMING_RECEIVED;

	LOG_RUNLOG_DEBUG("AB ABMsgCntProcessCCABVideoIncoming");
	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;

}

//int ABMsgCntProcessCCABCalleeOutgoingEarlyMedia(void *pvMsg)
//{
//	int iErrCode;
//
////	SIPAB_CALLEE_OUTGOING_EARLYMEDIA_ST *pstMsg = (SIPAB_CALLEE_OUTGOING_EARLYMEDIA_ST *)pvMsg;
////
////	pstMsg->uiPrmvType = JNI_DEV_CALL_OUTGOING_EARLYMEDIA;
//
//	LOG_RUNLOG_DEBUG("AB ABMsgCntProcessCCABCalleeOutgoingEarlyMedia");
//	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);
//
//	return iErrCode;
//
//}
//
//int ABMsgCntProcessSipABOutgoingInit(void *pvMsg)
//{
//	int iErrCode;
//
////	SIPAB_CALLEE_OUTGOING_INIT_ST *pstMsg = (SIPAB_CALLEE_OUTGOING_INIT_ST *)pvMsg;
////
////	pstMsg->uiPrmvType = JNI_DEV_OUTGOING_INIT;
//
//	LOG_RUNLOG_DEBUG("YKWriteQue:g_pstJNIMsgQ-ABMsgCntProcessSipABOutgoingInit");
//	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);
//
//	return iErrCode;
//}
//
//int ABMsgCntProcessSipABOutgoingProgress(void *pvMsg)
//{
//	int iErrCode;
//
////	SIPAB_CALLEE_OUTGOING_PROGRESS_ST *pstMsg = (SIPAB_CALLEE_OUTGOING_PROGRESS_ST *)pvMsg;
////
////	pstMsg->uiPrmvType = JNI_DEV_OUTGOING_PROGRESS;
//
//	LOG_RUNLOG_DEBUG("YKWriteQue:g_pstJNIMsgQ-ABMsgCntProcessSipABOutgoingProgress");
//	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);
//
//	return iErrCode;
//}
//
//int ABMsgCntProcessSipABOutgoingRinging(void *pvMsg)
//{
//	int iErrCode;
//
////	SIPAB_CALLEE_OUTGOING_RINGING_ST *pstMsg = (SIPAB_CALLEE_OUTGOING_RINGING_ST *)pvMsg;
////
////	pstMsg->uiPrmvType = JNI_DEV_OUTGOING_RINGING;
//
//	LOG_RUNLOG_DEBUG("YKWriteQue:g_pstJNIMsgQ-ABMsgCntProcessSipABOutgoingRinging");
//	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);
//
//	return iErrCode;
//}

int ABMsgCntProcessCCABCalleeHangoff(void *pvMsg)
{
	int iErrCode;

//	SIPAB_CALLEE_HANG_OFF_ST *pstMsg = (SIPAB_CALLEE_HANG_OFF_ST *)pvMsg;
//
//	pstMsg->uiPrmvType = JNI_DEV_CALL_END;

	LOG_RUNLOG_DEBUG("AB ABMsgCntProcessCCABCalleeHangoff");
	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}

int ABMsgCntProcessCCABCalleePickup(void *pvMsg)
{
	int iErrCode;

//	SIPAB_CALLEE_PICK_UP_ST *pstMsg = (SIPAB_CALLEE_PICK_UP_ST *)pvMsg;
//
//	pstMsg->uiPrmvType = JNI_DEV_CALL_CONNECTED;

	LOG_RUNLOG_DEBUG("AB ABMsgCntProcessCCABCalleePickup");
	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}


int ABMsgCntProcessSipabStopRing(void *pvMsg)
{
	int iErrCode;


	LOG_RUNLOG_DEBUG("AB ABMsgCntProcessSipabStopRing");
	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}

int ABMsgCntProcessSipABRegStatusChanged(void *pvMsg)
{
	int iErrCode;

	SIPAB_REG_STATUS_ST *pstMsg = (SIPAB_REG_STATUS_ST *)pvMsg;

//	if(pfIDBTJniRegStatusChanged == NULL)
//	{
//		LOG_RUNLOG_DEBUG("pfIDBTJniRegStatusChanged == NULL");
//		return FAILURE;
//	}

//	if(pstMsg->uiRegStatus == REG_STATUS_OFFLINE)
//	{
//		pstMsg->uiRegStatus = JNI_DEV_SIP_REG_STATUS_OFFLINE;
//	}
//	else if(pstMsg->uiRegStatus == REG_STATUS_ONLINE)
//	{
//		pstMsg->uiRegStatus = JNI_DEV_SIP_REG_STATUS_ONLINE;
//	}
//	else
//	{
//		LOG_RUNLOG_DEBUG("pstMsg->uiRegStatus = %d", pstMsg->uiRegStatus);
//		return FAILURE;
//	}

//	LOG_RUNLOG_DEBUG("pfIDBTJniRegStatusChanged");
//	pfIDBTJniRegStatusChanged(pstMsg->uiRegStatus);

	LOG_RUNLOG_DEBUG("YKWriteQue:g_pstJNIMsgQ-ABMsgCntProcessSipABRegStatusChanged");
	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}

int ABMsgCntProcessTMLinkStatusChanged(void *pvMsg)
{
	int iErrCode;

	TMAB_TM_LINK_STATUS_ST *pstMsg = (TMAB_TM_LINK_STATUS_ST *)pvMsg;

	LOG_RUNLOG_DEBUG("YKWriteQue:g_pstJNIMsgQ-ABMsgCntProcessTMLinkStatusChanged");
	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}
int ABMsgCntProcessSipABNetworkStatusChanged(void *pvMsg)
{
	int iErrCode;

	SIPAB_NETWORK_STATUS_ST *pstMsg = (SIPAB_NETWORK_STATUS_ST *)pvMsg;

	LOG_RUNLOG_DEBUG("YKWriteQue:g_pstJNIMsgQ-ABMsgCntProcessSipABNetworkStatusChanged");
	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}



int ABMsgCntProcessCCABCalleeErr(void *pvMsg)
{
	int iErrCode;

//	SIPAB_CALLEE_ERR_ST *pstMsg = (SIPAB_CALLEE_ERR_ST *)pvMsg;
//
//	pstMsg->uiPrmvType = JNI_DEV_CALL_ERROR;

	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}

int ABMsgCntProcessCCABCalleeErrRoomInvalid(void *pvMsg)
{
	int iErrCode;


	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}

int ABMsgCntProcessCCABCalleeErrRoomValid(void *pvMsg)
{
	int iErrCode;


	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}

int ABMsgCntProcessRfidCardValidate(void *pvMsg)
{
	int iErrCode;

	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}

int ABMsgCntProcessDtmf(void *pvMsg)
{
//	int iErrCode = -1;
//
//	CCAB_DTMF_OPENDOOR_ST *pstMsg = NULL;
//
//	if(((CCAB_CALLEE_SEND_DTMF_ST *)pvMsg)->iDtmfType == '#')
//	{
//		pstMsg->uiPrmvType = CCAB_DTMF_OPENDOOR;
//		iErrCode = YKWriteQue( g_pstJNIMsgQ , pstMsg,  0);
//	}
//
//	return iErrCode;
}

int ABMsgCntProcessOpenDoor(void *pvMsg)
{
	int iErrCode;

	LOG_RUNLOG_DEBUG("AB ABMsgCntProcessCCABOpendoor");

	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	SM210OpenDoor();

	return iErrCode;
}

int ABMsgCntProcessAdvertUpdate(void *pvMsg)
{
	int iErrCode;

	iErrCode = YKWriteQue( g_pstJNIMsgQ , pvMsg,  0);

	return iErrCode;
}

void *IDBTIntfLayerTask(void *pv)
{
    char acCmdBuff[AB_CMDLINE_BUFF_SIZE] =  { 0x00 };
    int iErrCode = 0;
    void* pvMsg = NULL;

    while(g_iRunABTaskFlag)
    {
    	iErrCode = YKReadQue(g_pstABMsgQ, &pvMsg, 0);
    	if ( 0 != iErrCode || NULL == pvMsg )
		{
//    		LOG_RUNLOG_DEBUG("iErrCode = %d, pvMsg = %d", iErrCode, (int)pvMsg);
			continue;
		}
    	memset(acCmdBuff, 0x00, sizeof(acCmdBuff));
    	LOG_RUNLOG_DEBUG("recv moudle type(0x%04x) [LPRecvMsg]", *(( unsigned int *)pvMsg));
    	switch ( *(( unsigned int *)pvMsg) )
    	{

//    		case ABSIP_CALL:
//				if(LPMsgCntProcessABSipCall(pvMsg, acCmdBuff, AB_CMDLINE_BUFF_SIZE) == 0)
//				{
//					LOG_RUNLOG_DEBUG("ABSIP_CALL");
//					LPProcessCommand(acCmdBuff);
//				}
//				break;
//			case ABSIP_ANSWER:
//				if(LPMsgCntProcessABSipAnswer(pvMsg, acCmdBuff, AB_CMDLINE_BUFF_SIZE) == 0)
//				{
//					LOG_RUNLOG_DEBUG("ABSIP_ANSWER");
//					LPProcessCommand(acCmdBuff);
//				}
//				break;
//			case ABSIP_TERMINATE:
//				if(LPMsgCntProcessABSipTerminate(pvMsg, acCmdBuff, AB_CMDLINE_BUFF_SIZE) == 0)
//				{
//					LOG_RUNLOG_DEBUG("ABSIP_TERMINATE");
//					LPProcessCommand(acCmdBuff);
//				}
//				break;
//			case ABSIP_TERMINATE_ALL:
//				if(LPMsgCntProcessABSipTerminateAll(pvMsg, acCmdBuff, AB_CMDLINE_BUFF_SIZE) == 0)
//				{
//					LOG_RUNLOG_DEBUG("ABSIP_TERMINATE_ALL");
//					LPProcessCommand(acCmdBuff);
//				}
//				break;
//			case AB_AUDIO_INCOMING:
//				LOG_RUNLOG_DEBUG("AB CCAB_AUDIO_INCOMING");
//				ABMsgCntProcessSipABAudioIncoming(pvMsg);
//				break;
//			case CCAB_VEDIO_MONITOR:
//				LOG_RUNLOG_DEBUG("AB CCAB_VEDIO_MONITOR");
//				ABMsgCntProcessCCABVideoIncoming(pvMsg);
//				break;
//			case CCAB_CALLEE_MUSIC_END:
//				LOG_RUNLOG_DEBUG("AB CCAB_CALLEE_MUSIC_END");
//				ABMsgCntProcessCCABCalleeOutgoingEarlyMedia(pvMsg);
//				break;
//			case CCAB_CALLEE_OUTGOING_INIT:
//				LOG_RUNLOG_DEBUG("ABMsgCntProcessSipABOutgoingInit");
//				ABMsgCntProcessSipABOutgoingInit(pvMsg);
//				break;
//			case CCAB_CALLEE_OUTGOING_PROGRESS:
//				LOG_RUNLOG_DEBUG("ABMsgCntProcessSipABOutgoingProgress");
//				ABMsgCntProcessSipABOutgoingProgress(pvMsg);
//				break;
//			case CCAB_CALLEE_OUTGOING_RINGING:
//				LOG_RUNLOG_DEBUG("ABMsgCntProcessSipABOutgoingRinging");
//				ABMsgCntProcessSipABOutgoingRinging(pvMsg);
//				break;
//			case CCAB_CALLEE_RECV_EARLYMEDIA:
//				ABMsgCntProcessSipABAudioIncoming(pvMsg);
//				break;
			case OTAB_REBOOT:
				LOG_RUNLOG_DEBUG("AB OTAB_REBOOT");
				ABMsgCntProcessReboot(pvMsg);
				break;
			case CCAB_CALLEE_MUSIC_END:		//保留
				break;
			case SIPAB_STOP_RING:
				LOG_RUNLOG_DEBUG("AB SIPAB_STOP_RING");
				ABMsgCntProcessSipabStopRing(pvMsg);
				break;
			case CCAB_CALLEE_PICK_UP:
				LOG_RUNLOG_DEBUG("AB CCAB_CALLEE_PICK_UP");
				ABMsgCntProcessCCABCalleePickup(pvMsg);
				break;
			case CCAB_CALLEE_HANG_OFF:
				LOG_RUNLOG_DEBUG("AB CCAB_CALLEE_HANG_OFF");
				ABMsgCntProcessCCABCalleeHangoff(pvMsg);
				break;
			case CCAB_CALLEE_ERR:
				LOG_RUNLOG_DEBUG("AB CCAB_CALLEE_ERR");
				ABMsgCntProcessCCABCalleeErr(pvMsg);
				break;
			case CCAB_CALLEE_ERR_ROOM_INVALID:
				LOG_RUNLOG_DEBUG("AB CCAB_CALLEE_ERR_ROOM_INVALID");
				ABMsgCntProcessCCABCalleeErrRoomInvalid(pvMsg);
				break;
			case CCAB_CALLEE_ERR_ROOM_VALID:
				LOG_RUNLOG_DEBUG("AB CCAB_CALLEE_ERR_ROOM_VALID");
				ABMsgCntProcessCCABCalleeErrRoomValid(pvMsg);
				break;
			case CCAB_OPENDOOR:
				LOG_RUNLOG_DEBUG("AB CCAB_OPENDOOR");
				ABMsgCntProcessOpenDoor(pvMsg);
				break;
			case CCAB_VEDIO_MONITOR:
				LOG_RUNLOG_DEBUG("AB CCAB_VEDIO_MONITOR");
				ABMsgCntProcessCCABVideoIncoming(pvMsg);
				break;
			case CCAB_CALLEE_SEND_DTMF:
				LOG_RUNLOG_DEBUG("AB CCAB_CALLEE_SEND_DTMF");
				CCAB_CALLEE_SEND_DTMF_ST *pstRcvMsg = (CCAB_CALLEE_SEND_DTMF_ST *)pvMsg;
				if(pstRcvMsg->iDtmfType == '#')
				{
					LOG_RUNLOG_DEBUG("AB RCV DTMF #, OPEN DOOR.");
					ABMsgCntProcessOpenDoor(pvMsg);
				}
				break;
			case SIPAB_REG_STATUS:
				LOG_RUNLOG_DEBUG("ABMsgCntProcessSipABRegStatusChanged");
				ABMsgCntProcessSipABRegStatusChanged(pvMsg);
				break;
			case SIPAB_NETWORK_STATUS:
				LOG_RUNLOG_DEBUG("ABMsgCntProcessNetworkStatusChanged");
				ABMsgCntProcessSipABNetworkStatusChanged(pvMsg);
				break;
			case TMAB_TM_LINK_STATUS:
				LOG_RUNLOG_DEBUG("ABMsgCntProcessTMLinkStatusChanged");
				ABMsgCntProcessTMLinkStatusChanged(pvMsg);
				break;
			case SMAB_RFIDCARD_VALIDATE_STATUS:
				LOG_RUNLOG_DEBUG("SMAB_RFIDCARD_VALIDATE_STATUS");
				ABMsgCntProcessRfidCardValidate(pvMsg);
				break;
			case TMAB_ADVERT_UPDATE:
				LOG_RUNLOG_DEBUG("TMAB_ADVERT_UPDATE");
				ABMsgCntProcessAdvertUpdate(pvMsg);
				break;
			default:
			LOG_RUNLOG_DEBUG("warn:recv unrecognizable moudle type(%d) [LPRecvMsg]", *(( unsigned int *)pvMsg));
				break;
    	}

    	//free(pvMsg);
    	LOG_RUNLOG_DEBUG("acCmdBuff value(%s)\n", acCmdBuff);
    }
}

int IDBTABInit(void)
{
	//创建消息队列；
	g_pstABMsgQ  = YKCreateQue(1024);
	if(g_pstABMsgQ == NULL)
	{
		return FAILURE;
	}

	g_pstJNIMsgQ = YKCreateQue(1024);
	if(g_pstJNIMsgQ == NULL)
	{
		return FAILURE;
	}

	//启动处理线程
	g_IDBTIntfLayerThreadId = YKCreateThread(IDBTIntfLayerTask, NULL);                     //创建CCTask

	if(NULL == g_IDBTIntfLayerThreadId)
	{
		return FAILURE;
	}
}

void IDBTJniSetRegStatusChangedCB(IDBTJNI_REG_STATUS_CHANGED_CB pstCB)
{
	pfIDBTJniRegStatusChanged = pstCB;
}
