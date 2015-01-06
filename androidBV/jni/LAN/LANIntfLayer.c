#include <IDBT.h>
#include <YKSystem.h>
#include <YKMsgQue.h>
#include <YKTimer.h>
#include <LANIntfLayer.h>
#include <LOGIntfLayer.h>
#ifndef _I6_CC_
#include <CCTask.h>
#else
#include <I6CCTask.h>
#endif
#include <common.h>
 
#include "CFimcMjpeg.h"

YK_MSG_QUE_ST*  g_pstLANIntfMsgQ = NULL;          //XT接口消息队列指针
YKHandle g_LANTaskThreadId;
YKHandle g_LANIntfThreadId;
int SipRecAudioOpen 	= 0;
int SipPlayAudioOpen 	= 0;
int SndRecAudioOpen 	= 0;
int SndPlayAudioOpen 	= 0;

int MsgCC2LANCall(const char *houseCode)
{
	int iErrCode;

	CCLAN_DOOR_CALL_ST *pstMsg = (CCLAN_DOOR_CALL_ST*)malloc(sizeof(CCLAN_DOOR_CALL_ST));
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	memset(pstMsg, 0, sizeof(CCLAN_DOOR_CALL_ST));

	pstMsg->uiPrmvType = CCLAN_CALL;
	pstMsg->ucDevType = '0';
	strncpy((char *)pstMsg->aucRoomNum, houseCode, strlen(houseCode));
	iErrCode = YKWriteQue(g_pstLANIntfMsgQ , pstMsg,  0);

	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgCC2LANTerminate(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgCC2LANTerminate!\n");
	pstMsg->uiPrmvType = CCLAN_TERMINATE;
	iErrCode = YKWriteQue(g_pstLANIntfMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgCC2LANWatchLineuse(char *cFromIP)
{
	int iErrCode;
	LAN_WATCH_MSG_ST *pstMsg = YKNew0(LAN_WATCH_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgCC2LANWatchLineuse!\n");
	pstMsg->uiPrmvType = CCLAN_WATCH_LINEUSE;
	memcpy(pstMsg->cFromIP, cFromIP, strlen(cFromIP));
	iErrCode = YKWriteQue(g_pstLANIntfMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgCC2LANWatchRespond(char *cFromIP)
{
	int iErrCode;
	LAN_WATCH_MSG_ST *pstMsg = YKNew0(LAN_WATCH_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgCC2LANWatchRespond!\n");
	pstMsg->uiPrmvType = CCLAN_WATCH_RESPOND;
	memcpy(pstMsg->cFromIP, cFromIP, strlen(cFromIP));
	iErrCode = YKWriteQue(g_pstLANIntfMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}


int MsgLAN2CCCallFail(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCCallFail!\n");
	pstMsg->uiPrmvType = LANCC_CALLEE_FAIL;
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgLAN2CCCallRespond(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCCallRespond!\n");
	pstMsg->uiPrmvType = LANCC_CALLEE_RESPOND;
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgLAN2CCCallPickUp(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCCallPickUp!\n");
	pstMsg->uiPrmvType = LANCC_CALLEE_PICK_UP;
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgLAN2CCCallHangOff(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCCallHangOff!\n");
	pstMsg->uiPrmvType = LANCC_CALLEE_HANG_OFF;
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgLAN2CCCallLineuse(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCCallLineuse!\n");
	pstMsg->uiPrmvType = LANCC_CALLEE_LINEUSE;
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgLAN2CCCallOpendoor(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCCallOpendoor!\n");
	pstMsg->uiPrmvType = LANCC_CALLEE_OPENDOOR;
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgLAN2CCTalkEnd(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCTalkEnd !\n");
	pstMsg->uiPrmvType = LANCC_CALLEE_TALK_END;
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgLAN2CCCallTimeOut(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCCallTimeOut!\n");
	pstMsg->uiPrmvType = LANCC_CALLEE_RING_TIMEOUT;
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgLAN2CCTalkTimeOut(void)
{
	int iErrCode;
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCTalkTimeOut!\n");
	pstMsg->uiPrmvType = LANCC_CALLEE_TALK_TIMEOUT;
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int MsgLAN2CCWatch(char *cFromIP)
{
	int iErrCode;
	LAN_WATCH_MSG_ST *pstMsg = YKNew0(LAN_WATCH_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("LAN MsgLAN2CCWatch!\n");
	pstMsg->uiPrmvType = LANCC_WATCH_INCOMING;
	memcpy(pstMsg->cFromIP, cFromIP, strlen(cFromIP));
	iErrCode = YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

void* LANIntfThread(void *arg)
{
	void *pvMsg = NULL;
	int iErrCode;

	//创建XT接口层消息队列
	g_pstLANIntfMsgQ  = YKCreateQue(LANINTF_MSG_QUE_LEN);
	if(NULL == g_pstLANIntfMsgQ)
	{
		return NULL;
	}

	while(g_iRunLANTaskFlag == TRUE)
	{

		iErrCode = YKReadQue(g_pstLANIntfMsgQ, &pvMsg, 0);			//阻塞读取
		if(0 != iErrCode || NULL == pvMsg)
		{
			//printf("**********************i am XTIntfThread!\n");
			continue;
		}
		else
		{
			switch(*(( unsigned int *)pvMsg))
			{
			case CCLAN_CALL:
				{
					CCLAN_DOOR_CALL_ST *pstRcvMsg = (CCLAN_DOOR_CALL_ST *)pvMsg;
					//LOG_RUNLOG_DEBUG("LANIntfThread CCLAN_CALL %s",pstRcvMsg->aucRoomNum);
					if ((strcmp(pstRcvMsg->aucRoomNum, "0000") == 0) || (strcmp(pstRcvMsg->aucRoomNum, "0") == 0))
					{
						Call_Func(1, pstRcvMsg->aucRoomNum, "Z00010000000"); //呼叫   1  中心  2 住户
						LOG_RUNLOG_DEBUG("LANIntfThread CCLAN_CALL %s",pstRcvMsg->aucRoomNum);
					}
					else
					{
						//Call_Func(2, "0101", ""); //呼叫   1  中心  2 住户
						Call_Func(2, pstRcvMsg->aucRoomNum, ""); //呼叫   1  中心  2 住户
					}
				}
				break;
			case CCLAN_TERMINATE:
				if ((Local.Status == 3) || (Local.Status == 4))
				{
					WatchEnd_Func();
				}
				else
				{
					TalkEnd_Func();
				}
				break;
			case CCLAN_WATCH_LINEUSE:
				{
					LAN_WATCH_MSG_ST *pstRcvMsg = (LAN_WATCH_MSG_ST *)pvMsg;
					//LOG_RUNLOG_DEBUG("LANIntfThread CCLAN_WATCH_LINEUSE %s",pstRcvMsg->cFromIP);
					WatchLineUse_Func(pstRcvMsg->cFromIP);
				}
				break;
			case CCLAN_WATCH_RESPOND:
				{
					LAN_WATCH_MSG_ST *pstRcvMsg = (LAN_WATCH_MSG_ST *)pvMsg;
					//LOG_RUNLOG_DEBUG("LANIntfThread CCLAN_WATCH_RESPOND %s",pstRcvMsg->cFromIP);
					WatchRespond_Func(pstRcvMsg->cFromIP);
				}
				break;
			default:
				break;
			}

			free(pvMsg);
		}
	}

	return NULL;
}

void* LANTaskThread(void *arg)
{
	LANTask(0, NULL);
}

int LANInit(void)
{
	g_iRunLANTaskFlag = TRUE;
	g_LANTaskThreadId = YKCreateThread(LANTaskThread, NULL);
	if(g_LANTaskThreadId == NULL)
	{
		return FAILURE;
	}

	return SUCCESS;
}

void LANUninit(void)
{
	// 将线程运行标志清除
//	g_iRunXTTaskFlag = FALSE;

	// 向线程发出退出的消息
	LAN_EVENT_MSG_ST *pstMsg = YKNew0(LAN_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		//LOG_RUNLOG_DEBUG("LAN malloc error!\n");
	}
	pstMsg->uiPrmvType = XT_EXIT;
	YKWriteQue(g_pstLANIntfMsgQ, pstMsg, 0);

	// 等待线程资源回收
	YKJoinThread(g_LANIntfThreadId);
	YKJoinThread(g_LANTaskThreadId);

	sleep(4);

	// 删除消息队列
	YKDeleteQue(g_pstLANIntfMsgQ);
}
