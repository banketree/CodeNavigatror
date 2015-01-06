#ifndef _YK_IMX27_AV_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <YKSystem.h>
#include "YKMsgQue.h"
#include "IDBT.h"
#include "LPIntfLayer.h"                            // add by cs for get sip status
#include "../INC/SMUtil.h"
#include "../INC/SMSerial.h"
#include "../INC/SMEventHndl.h"
#include "../INC/SMRfCard.h"
#include "TMInterface.h"
#include "LOGIntfLayer.h"
//#include "YK8126Gpio.h"
//#include "SMXt8126SndTool.h"


//串口接收数据起始结束标志的状态
#define BEGIN_FLAG_NOT_FOUND    0
#define BEGIN_FLAG_FOUND        1

//串口接收数据起始结束标志
#define PROT_BEGIN_FLAG         '^'
#define PROT_END_FLAG           '~'

#define RECV_LMT_TIMES   5

//IO驱动路径
#define IO_DRIVER	 			"/dev/iDBT_ioctrl"
#define KEY_DEV_NAME    		"/dev/ftsar_adc010_0_key"

//协议编解码数据定义.
#define	CODEC_IDBT_CLIENT_REQ 0x30
#define	COEDC_IDBT_HOST_REQ   0x31

//错误命令处理.
#define CODEC_ERROR            0xff        //编解码错误
#define CODEC_CRC_ERROR        0xfe        //CRC校验错误
#define CODEC_ENCODE_ERROR     0xfd        //编码错误
#define CODEC_DECODE_ERROR     0xfc        //解码错误

//协议数据长度
#define PROT_BEGIN_FLAG_SIZE   1
#define PROT_CMD_TYPE_SIZE     1
#define PROT_MACHINE_TYPE      1
#define PROT_CRC_LEN           4
#define PROT_END_FLAG_SIZE     1

//串口数据解析操作结果定义
#define OPERATOR_CRC_ERROR    'C'
#define OPERATOR_ERROR        'E'
#define OPERATOR_PASS         'P'

#define SM_IF_BROARD_ONLINE		1				//接口板在线
#define SM_IF_BROARD_OFFLINE       0               //接口板离线

#define SM_CALL_CENTRE             0               //呼叫中心状态
#define SM_CALL_NORMAL_USER        1               //呼叫普通用户

static unsigned int g_iCallStatus;                   // add by cs

static volatile unsigned char g_ucCurMachine = DEV_TYPE_TTL;      //当前设备类型
static volatile char g_cCurCmd;                                    //当前发送的命令
static volatile char g_acCurRoom[12] = {0};                        //当前正在通话的的房号

SM_TIMER_ST g_stHeartBeatTimer;                                   //心跳计时器
SM_TIMER_ST g_stClientRspTimeoutTimer;                            //应答超时计时器
SM_TIMER_ST g_stUnlockTimer;                                      // 开锁时间   add by cs
TM_TIMER_ST g_stPwdOpenDoorTimer;										//密码开门
static char g_cUnlockFlag = 0;                                    //开锁标志    add by cs

static COM_DATA_RCV_ST g_stRcvData;								  //串口接收数据
static int g_iComFd = -1;					                      //串口文件文件描述符
int g_iIoFd;						                              //IO设备驱动文件描述符
static int g_iKeyFd=-1;

pthread_t g_rcvComData;				                              //串口接收线程标识符
pthread_t g_HndlMsgTask;					                      //SM消息接收及处理线程标识符
pthread_t g_rcvKeyMsg;					                      //SM Recv key value.

//extern pthread_mutex_t snd_card_lock;                                    // 声卡操作的锁add by cs 2012-09-29

static BOOL g_bRunHndlMsgTaskFlg = TRUE;						  //消息接收及处理线程运行状态标志
static BOOL g_bRunCommRcvTaskFlg = TRUE;                          //串口接收线程运行状态标志

YK_MSG_QUE_ST*  g_pstSMMsgQ = NULL;                                //SM消息队列指针
extern YK_MSG_QUE_ST* g_pstCCMsgQ;                                //CC消息队列指针
extern YK_MSG_QUE_ST* g_pstXTIntfMsgQ;                            //XT消息队列指针

static volatile BOOL g_bVedioMonitorFlg = FALSE;                 //是否处于监控状态（TRUE-监控中  FALSE-非监控中）
static volatile BOOL g_bFillLightOpenFlg = FALSE;                //梯口机补光灯状态（TRUE-打开   FALSE-关闭）

static volatile int g_iIfBoardStatus = SM_IF_BROARD_OFFLINE;	   //接口板是否在线状态

static volatile int g_iCallUserType = SM_CALL_NORMAL_USER;       //呼叫用户类型

int SMIoCtl(unsigned char ucIoNum, int iSta)
{
	if (g_iIoFd >= 0)
	{
		ioctl(g_iIoFd, iSta, ucIoNum);

		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

void SMCtlSipLed(int iSta)
{
	SMIoCtl(10, iSta);
}

void SMCtlNetLed(int iSta)
{
	SMIoCtl(11, iSta);
}

void SM210CloseDoor()
{
	SMIoCtl(5, 0);
}

void SM210OpenDoor()
{
	SMIoCtl(5, 1); // led mie
//	sleep(3);
//	SMIoCtl(5, 0);;
	g_stPwdOpenDoorTimer.pstTimer = YKCreateTimer(SM210CloseDoor,NULL,5000, YK_TT_NONPERIODIC,(unsigned long *)&g_stPwdOpenDoorTimer.ulMagicNum);
}

void SMCtlLightLed(int iSta)
{
	LOG_RUNLOG_DEBUG("SMCtlLightLed : %d\n", iSta);
	SMIoCtl(3, iSta);
	SMIoCtl(6, iSta);
}

int SMGetSenseState()
{
	char iostate;
	if (g_iIoFd >= 0)
	{
//		ioctl(g_iIoFd, iSta, ucIoNum);
		read(g_iIoFd,&iostate,7);
		LOG_RUNLOG_DEBUG("SMGetSenseState : %d\n", iostate);
		if(iostate == 0x0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
}

void SMCtlYkLed(int iSta)
{
	SMIoCtl(1, iSta);
}

int SMSendDataToMsgQue(char *pcData, unsigned int uiLen)
{
	SM_RCV_DATA_ST	*pstMsg = YKNew0(SM_RCV_DATA_ST,1);
	if(NULL != pstMsg)
	{
		pstMsg->uiPrmvType = SM_SERIAL_DATA_RCV;
		pstMsg->uiLen = uiLen;
		pstMsg->pcData = pcData;

		YKWriteQue(g_pstSMMsgQ , pstMsg,  0);
	}

	return FAILURE;
}

void SMHndlComRcvData(COM_DATA_RCV_ST *pstComDataRcv)
{
	char acTmpBuf[COMM_DATA_RCV_SIZE / 2] ={0};
	int iRcvDataLen = 0;
	int iIndex;
	int iBeginIndex;
	char *pcRcvData = NULL;

	//调用的次数和单次休眠时间决定了已收到数据的有效期
	if(pstComDataRcv->ucCounter >= RECV_LMT_TIMES)
	{
		pstComDataRcv->iLen = 0;
		pstComDataRcv->pcHead = pstComDataRcv->acBuf;
		pstComDataRcv->ucCounter = 0;
		pstComDataRcv->ucState = BEGIN_FLAG_NOT_FOUND;
		return;
	}

	iRcvDataLen = SMReadComPort(g_iComFd, acTmpBuf, sizeof(acTmpBuf));
	if(0 == iRcvDataLen)
	{
		return;
	}
	LOG_RUNLOG_DEBUG("SM [SM_MESSAGE] -SMRecv[%d] : %s\n", iRcvDataLen, acTmpBuf);

	if(&pstComDataRcv->acBuf[COMM_DATA_RCV_SIZE] < pstComDataRcv->pcHead + pstComDataRcv->iLen + iRcvDataLen)
	{
		//所有数据都清空，恢复初始状态
		pstComDataRcv->iLen = 0;
		pstComDataRcv->pcHead = pstComDataRcv->acBuf;
		pstComDataRcv->ucState = BEGIN_FLAG_NOT_FOUND;
	}
	memcpy(pstComDataRcv->pcHead + pstComDataRcv->iLen, acTmpBuf, iRcvDataLen);
	pstComDataRcv->iLen += iRcvDataLen;

	iIndex = 0xFFFF;
	while((pstComDataRcv->iLen > 0) && (iIndex != 0))
	{
		iIndex = SMFindChar((unsigned char *)pstComDataRcv->pcHead, pstComDataRcv->iLen, PROT_BEGIN_FLAG);
		if(iIndex != 0)
		{
			if(pstComDataRcv->ucState == BEGIN_FLAG_FOUND)
			{
				iBeginIndex = SMFindChar((unsigned char *)pstComDataRcv->pcHead, iIndex, PROT_END_FLAG);
				if(iBeginIndex != 0)
				{
					pstComDataRcv->ucCounter = 0; //收到结束标志，counter清零
					pcRcvData = YKNew0(char,iBeginIndex + 1);
					if(pcRcvData != NULL)
					{
						*pcRcvData= PROT_BEGIN_FLAG;
						memcpy(pcRcvData+ 1, pstComDataRcv->pcHead, iBeginIndex);
						SMSendDataToMsgQue(pcRcvData, iBeginIndex + 1);
					}
				}
			}

			pstComDataRcv->pcHead += iIndex;
			pstComDataRcv->iLen -= iIndex;
			pstComDataRcv->ucState = BEGIN_FLAG_FOUND;
		}
		else
		{
			if(pstComDataRcv->ucState == BEGIN_FLAG_FOUND)
			{
				iIndex = SMFindChar((unsigned char *)pstComDataRcv->pcHead, pstComDataRcv->iLen, PROT_END_FLAG);
				if(iIndex == 0) //结束标志未收到需要继续接收，counter开始计数
				{
					pstComDataRcv->ucCounter++;
				}
				else
				{
					pstComDataRcv->ucCounter = 0; //收到结束标志，counter清零
					pcRcvData = YKNew0(char, iIndex + 1);
					if(pcRcvData != NULL)
					{
						*pcRcvData = PROT_BEGIN_FLAG;
						memcpy(pcRcvData+ 1, pstComDataRcv->pcHead, iIndex);
						SMSendDataToMsgQue(pcRcvData, iIndex + 1);
					}

					pstComDataRcv->pcHead += iIndex;
					pstComDataRcv->iLen -= iIndex;
					pstComDataRcv->ucState = BEGIN_FLAG_NOT_FOUND;
				}
			}
			else //(pstComDataRcv->ucState == BEGIN_FLAG_NOT_FOUND)
			{
				pstComDataRcv->pcHead = pstComDataRcv->acBuf;
				pstComDataRcv->iLen = 0;
			}
		}
	}

	//将数据调整到buf最前端
	if(pstComDataRcv->pcHead != pstComDataRcv->acBuf)
	{
		memcpy(pstComDataRcv->acBuf, pstComDataRcv->pcHead, pstComDataRcv->iLen);
		pstComDataRcv->pcHead = pstComDataRcv->acBuf;
	}
}

int SMHostEncode(char cCmdType, char* pcBufIn, int iBufLen, char** ppBufOut)
{
	int iSize = -1;
	if (cCmdType < 0 || cCmdType > 0xff)
	{
		return iSize;
	}

	*ppBufOut = NULL;
	do
	{
		int iCodecSize = iBufLen + PROT_BEGIN_FLAG_SIZE + PROT_CMD_TYPE_SIZE + PROT_CRC_LEN + PROT_END_FLAG_SIZE;

		int index = 0; //协议包下标

		*ppBufOut = YKNew0(char, iCodecSize);
		if (NULL == *ppBufOut)
		{
			break;
		}
		memset(*ppBufOut, 0x0, iCodecSize);
		//起始标志
		(*ppBufOut)[index++] = PROT_BEGIN_FLAG;

		//命令类型
		(*ppBufOut)[index++] = cCmdType;
		//命令内容
		memcpy(&(*ppBufOut)[index], pcBufIn, iBufLen);
		index += iBufLen;
		//校验码
		unsigned short crcValue = SMCalcuCRC(&(*ppBufOut)[PROT_BEGIN_FLAG_SIZE], iBufLen + PROT_CMD_TYPE_SIZE, 0);
		sprintf((char *) &(*ppBufOut)[index], "%04X", crcValue);
		index += PROT_CRC_LEN;
		//结束标志
		(*ppBufOut)[index] = PROT_END_FLAG;
		iSize = iCodecSize;
	} while (FALSE);

	return iSize;
}

BOOL SMSendHostReq(char cEventType, char cMachineType)
{
	char *pcBuf = NULL;
	unsigned int uiLen = 0;
	HOST_REQ_PACKET_ST stHostReqContent;
	char cCMD = COEDC_IDBT_HOST_REQ;

	memset(&stHostReqContent, 0x00, sizeof(HOST_REQ_PACKET_ST));
	stHostReqContent.acEventType[0] = cEventType;
	stHostReqContent.acMachineType[0] = cMachineType;

	//记录本次发送信息，发送失败的时重发
	g_cCurCmd = cEventType;

	//改造终端向接口板发送命令
	uiLen = SMHostEncode(cCMD, (char*) &stHostReqContent, sizeof(HOST_REQ_PACKET_ST), &pcBuf);

	if (uiLen > 0)
	{
#ifdef _YK_XT8126_AV_
		SMWriteComPort(g_iComFd, pcBuf, uiLen);
#endif
		return TRUE;
	}

	return FALSE;
}

BOOL SMNMDevReboot(void)
{
	if(SMSendHostReq(STH_DEV_REBOOT, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(device reboot)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg (device reboot)");
	return TRUE;
}

#ifdef _YK_XT8126_DOOR_
/************************************************************************
add by cs 
************************************************************************/
void SMUnlockCallback(void* pvPara)
{
    g_cUnlockFlag = 0;
    YK8126CloseLock();
    LOG_RUNLOG_DEBUG("SM send msg (close door)");
}
#endif

#ifndef _YK_XT8126_DOOR_
BOOL SMOpenDoor(void)
{
	if(SMSendHostReq(STH_OPEN_DOOR, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(open door)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg (open door)");

	return TRUE;
}
#else
BOOL SMOpenDoor(void)
{

    if (g_cUnlockFlag == 0)     
    {
        YKRemoveTimer(g_stUnlockTimer.pstTimer,g_stUnlockTimer.ulMagicNum);
    
    	//添加超时定时器
    	g_stUnlockTimer.pstTimer = YKCreateTimer(SMUnlockCallback, NULL, UNLOCK_TIMROUT,
    			YK_TT_NONPERIODIC, (unsigned long *)&g_stUnlockTimer.ulMagicNum);
    	if(NULL == g_stUnlockTimer.pstTimer)
    	{
    		LOG_RUNLOG_ERROR("SM create unlock timer failed!");
    		return FAILURE;
    	}
        
        YK8126OpenLock();
        g_cUnlockFlag = 1;

    	LOG_RUNLOG_DEBUG("SM send msg (---------open door --------)");
    }

	return TRUE;
}
#endif

BOOL SMCalleePickUp(void)
{
	if (SMSendHostReq(CALLEE_PICK_UP, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(CALLEE_PICK_UP)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg (callee pick up)");
	return TRUE;
}

BOOL SMCalleeHandoff(void)
{
	if (SMSendHostReq(CALLEE_HANG_OFF, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(CALLEE_HANG_OFF)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (callee hand off)");
	return TRUE;
}

BOOL SMCalleeMusicEnd(void)
{
	if (SMSendHostReq(CALL_AUDIO_CONNECT, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(CALL_AUDIO_CONNCET)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (audio connect)");
	return TRUE;
}

BOOL SMVedioMonitor(void)
{
	if(SMSendHostReq(VEDIO_MONITOR_REQUSET, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed! (vedio monitor)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (vedio monitor)");
	return TRUE;
}

BOOL SMVedioMonitorCancel(void)
{
	if(SMSendHostReq(VEDIO_MOMITOR_CANCEL, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed! (vedio monitor)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg (vedio monitor cancel)");
	return TRUE;
}

BOOL SMSendHeartBeat(void)
{
	if(SMSendHostReq(HEART_BEAT, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed! (heart beat)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send heart beat!");
	return TRUE;
}

void SMHeartBeatCallback(void* pvPara)
{
	SM_TIMER_EVENT_ST *pstPrmv = YKNew0(SM_TIMER_EVENT_ST,1);
	if (NULL == pstPrmv)
	{
		LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
		return;
	}

	pstPrmv->uiPrmvType = SM_TIMER_EVENT;
	pstPrmv->uiTimerId = TIMER_ID_HEARTBEAT;

	YKWriteQue(g_pstSMMsgQ , pstPrmv,  0);
}

void SMClientRspTimeOutCallback(void* pvPara)
{
	LOG_RUNLOG_DEBUG("SM client rsp time out!");
	SM_TIMER_EVENT_ST *pstPrmv = YKNew0(SM_TIMER_EVENT_ST,1);
	if (NULL== pstPrmv)
	{
		LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
		return;
	}

	pstPrmv->uiPrmvType = SM_TIMER_EVENT;
	pstPrmv->uiTimerId = TIMER_ID_PACKET_RSP_TIMEOUT;

	YKWriteQue(g_pstSMMsgQ , pstPrmv,  0);
}

int SMClientRspTimeOutReset(void)
{
	//删除超时定时器
	YKRemoveTimer(g_stClientRspTimeoutTimer.pstTimer,g_stClientRspTimeoutTimer.ulMagicNum);

	//添加超时定时器
	g_stClientRspTimeoutTimer.pstTimer = YKCreateTimer(SMClientRspTimeOutCallback, NULL, CLIENT_RSP_TIMROUT,
			YK_TT_NONPERIODIC, (unsigned long *)&g_stClientRspTimeoutTimer.ulMagicNum);
	if(NULL == g_stClientRspTimeoutTimer.pstTimer)
	{
		LOG_RUNLOG_ERROR("SM create client rsp timer failed!");
		return FAILURE;
	}

	return SUCCESS;
}

int SMTimerInit(void)
{
#ifndef _YK_XT8126_DOOR_
	//心跳定时器
	g_stHeartBeatTimer.pstTimer = YKCreateTimer(SMHeartBeatCallback, NULL, HEARTBEAT_INTERVAL,
			YK_TT_PERIODIC, (unsigned long *)&g_stHeartBeatTimer.ulMagicNum);
	if(NULL == g_stHeartBeatTimer.pstTimer)
	{
		LOG_RUNLOG_ERROR("SM: create heartbeat timer failed!");
		return FAILURE;
	}

	//应答包超时计时器
	g_stClientRspTimeoutTimer.pstTimer = YKCreateTimer(SMClientRspTimeOutCallback, NULL, CLIENT_RSP_TIMROUT,
			YK_TT_NONPERIODIC, (unsigned long *)&g_stClientRspTimeoutTimer.ulMagicNum);
	if(NULL == g_stClientRspTimeoutTimer.pstTimer)
	{
		LOG_RUNLOG_ERROR("SM: create client rsp timer failed!");
		return FAILURE;
	}
#else
    g_stUnlockTimer.pstTimer = NULL;
#endif
	return SUCCESS;
}

int SMHndlClientReq(CLIENT_REQ_PACKET_ST *pstContent)
{
	char acClientReqType[1] = { 0 };

	strncpy(acClientReqType, pstContent->acEventType, sizeof(pstContent->acEventType));

	switch (acClientReqType[0])
	{
		case DOOR_MACHINE_CALL:
		{
			LOG_RUNLOG_DEBUG("SM call start!");
			//SMCC_DOOR_CALL_ST *pstMsg = YKNew0(SMCC_DOOR_CALL_ST, 1);
			SMCC_DOOR_CALL_ST *pstMsg = (SMCC_DOOR_CALL_ST*)malloc(sizeof(SMCC_DOOR_CALL_ST));
			if(NULL == pstMsg)
			{
				return FAILURE;
			}

            // add by cs 2012-10-12
            if (g_iCallStatus == 1)
            {
                LOG_RUNLOG_DEBUG("SM call err, calling center!");
                return FAILURE;
            }

            g_iCallStatus = 1;

			memset(pstMsg, 0, sizeof(SMCC_DOOR_CALL_ST));

			pstMsg->uiPrmvType = SMCC_DOOR_CALL;
			pstMsg->ucDevType = '0';
			strncpy(g_acCurRoom, (char *)pstContent->acRoomNum, 4);
			strncpy((char *)pstMsg->aucRoomNum, "0000", 4);
			strncpy((char *)pstMsg->aucRoomNum+4, (char *)pstContent->acRoomNum, 4);
			YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
			g_iCallUserType = SM_CALL_NORMAL_USER;
			break;
		}
		case INDOOR_MACHINE_PICK_UP:
		{
			LOG_RUNLOG_DEBUG("SM indoor pick up,call end!");
			SMCC_INDOOR_PICK_UP_ST *pstMsg = YKNew0(SMCC_INDOOR_PICK_UP_ST, 1);
			if(NULL == pstMsg)
			{
				return FAILURE;
			}

            if ((g_iCallStatus == 1) && (g_iCallUserType == SM_CALL_NORMAL_USER))   // add by cs 2012-10-12
            {
    			pstMsg->uiPrmvType = SMCC_INDOOR_PICK_UP;
    			YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
            }
			break;
		}
		case DOOR_MACHINE_HANG_OFF:
		{
			LOG_RUNLOG_DEBUG("SM door machine hang off,call end!");
			SMCC_DOOR_HANG_OFF_ST *pstMsg = YKNew0(SMCC_DOOR_HANG_OFF_ST, 1);
			if(NULL == pstMsg)
			{
				return FAILURE;
			}
            if ((g_iCallStatus == 1) && (g_iCallUserType == SM_CALL_NORMAL_USER))   // add by cs 2012-10-12
            {
                g_iCallStatus = 0;
                
    			pstMsg->uiPrmvType = SMCC_DOOR_HANG_OFF;
    			YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
            }
			break;
		}
		case INDOOR_MACHINE_HANG_OFF:
		{
			LOG_RUNLOG_DEBUG("SM indoor machine hand off,do nothing!");
            if ((g_iCallStatus == 1) && (g_iCallUserType == SM_CALL_NORMAL_USER))   // add by cs 2012-10-12
            {
                g_iCallStatus = 0;
            }
			break;
		}
		default:
			break;
	}

	return SUCCESS;
}

BOOL SMHndlClientRsp(CLIENT_RSP_PACKET_ST *pstContent)
{
	char acClientRspType[PROT_CMD_TYPE_SIZE + 1] = { 0 };
	BOOL bStatus = FALSE;

	strncpy(acClientRspType, (char *)(pstContent->acCmdContent), sizeof(pstContent->acCmdContent));

#ifdef _SM_EVENT_DEBUG_
	LOG_RUNLOG_DEBUG("SM clientRspContent: %c", acClientRspType[0]);
#endif

	switch(acClientRspType[0])
	{
	case OPERATOR_CRC_ERROR :
	case OPERATOR_ERROR :
		//错误，重发命令
//		if(SMSendHostReq(g_cCurCmd, g_ucCurMachine) == FALSE)
//		{
//			return bStatus;
//		}
		bStatus = TRUE;
		break;
	case OPERATOR_PASS :
#ifdef _SM_EVENT_DEBUG_
		LOG_RUNLOG_DEBUG("SM client is online.");
#endif
		if(g_iIfBoardStatus == SM_IF_BROARD_OFFLINE)
		{
#if _TM_TYPE_ == _YK069_
			TMUpdateAlarmState(TM_ALARM_IF_BOARD, '0');
#else
			TMUpdateAlarmState(TM_ALARM_IF_BOARD, "0");
#endif
			g_iIfBoardStatus = SM_IF_BROARD_ONLINE;
		}
		SMClientRspTimeOutReset();
		bStatus = TRUE;
		break;

	default:
		break;
	}

	return bStatus;
}

BOOL SMVerifyCMDContent(char cCmd, const char* pcContent, int iSize)
{
	BOOL bVerify = FALSE;

	switch (cCmd)
	{
	//校验接口板请求内容
	case CODEC_IDBT_CLIENT_REQ:
	{
		if (iSize != sizeof(CLIENT_REQ_PACKET_ST))
		{
			LOG_RUNLOG_DEBUG("SM verify size error!");
			break;
		}
		CLIENT_REQ_PACKET_ST* pClientReq = (CLIENT_REQ_PACKET_ST*) pcContent;
		char acTmp[sizeof(pClientReq->acRoomNum) + 1] = { 0 };

		//校验事件类型
		strncpy(acTmp, pClientReq->acEventType, sizeof(pClientReq->acEventType));
		if (!SMVerifyHexAscii(acTmp, sizeof(pClientReq->acEventType)))
		{
			LOG_RUNLOG_DEBUG("SM verify event type ascii error!");
			break;
		}
		int iValue = SMHexAsciiToDecimal(acTmp);
		if (iValue > 0x5 || iValue < 0x0)
		{
			LOG_RUNLOG_DEBUG("SM verify event type error!");
			break;
		}

		//校验房号
		strncpy(acTmp, pClientReq->acRoomNum, sizeof(pClientReq->acRoomNum));
		if (!SMVerifyHexAscii(acTmp, sizeof(pClientReq->acRoomNum)))
		{
			LOG_RUNLOG_DEBUG("SM verify room num ascii error!");
			break;
		}
		int iNum = SMHexAsciiToDecimal(acTmp);
		if (iNum > 0xFFFF || iNum < 0x0000)
		{
			LOG_RUNLOG_DEBUG("SM verify room num error!");
			break;
		}
		bVerify = TRUE;
		break;
	}

	//校验改造终端请求应答
	case COEDC_IDBT_HOST_REQ:
	{
		if (iSize != sizeof(CLIENT_RSP_PACKET_ST))
		{
			break;
		}
		CLIENT_RSP_PACKET_ST* pClientRes = (CLIENT_RSP_PACKET_ST*) pcContent;
		char acTmp[sizeof(pClientRes->acCmdContent) + 1] = { 0 };

		//校验事件类型
		strncpy(acTmp, pClientRes->acCmdContent, sizeof(pClientRes->acCmdContent));
		if (acTmp[0] == 'P' || acTmp[0] == 'C' || acTmp[0] == 'E')
		{
			bVerify = TRUE;
		}

		break;
	}
	default:
		break;
	}
	return bVerify;
}

char SMHostDecode(const char* pcDecode, int iBufLen, char**ppcBufOut)
{
	char cCodecCmd = CODEC_ERROR;

	if (NULL == pcDecode)
	{
		return cCodecCmd;
	}
	if (iBufLen < 8)
	{
		return cCodecCmd;
	}
	*ppcBufOut = NULL;
	do
	{
		int index = 0;

		//协议包标志符
		if (pcDecode[index] != PROT_BEGIN_FLAG || pcDecode[iBufLen - 1] != PROT_END_FLAG)
		{
			cCodecCmd = CODEC_ERROR;
			LOG_RUNLOG_DEBUG("SM FLAG ERROR");
			break;
		}
		index += PROT_BEGIN_FLAG_SIZE;

		//CRC校验
		char acTmp[PROT_CRC_LEN + 1] = {0x0};
		unsigned short usCrcVal = SMCalcuCRC(pcDecode + index, iBufLen - PROT_BEGIN_FLAG_SIZE - PROT_END_FLAG_SIZE - PROT_CRC_LEN, 0);
		memcpy(acTmp, pcDecode + iBufLen - PROT_END_FLAG_SIZE - PROT_CRC_LEN, PROT_CRC_LEN);
		unsigned short acTmpDec = (unsigned short) SMHexAsciiToDecimal(acTmp);

		if (usCrcVal != acTmpDec)
		{
			cCodecCmd = CODEC_CRC_ERROR;
			LOG_RUNLOG_DEBUG("SM CRC ERROR");
			break;
		}

		// 获取命令类型
		memset(acTmp, 0x0, sizeof(acTmp));
		memcpy(acTmp, &pcDecode[index], PROT_CMD_TYPE_SIZE);
		cCodecCmd = acTmp[0];
		if (cCodecCmd < 0x30 || cCodecCmd > 0x31)
		{
			cCodecCmd = CODEC_DECODE_ERROR;
			LOG_RUNLOG_DEBUG("SM cCmd TYPE ERROR");
			break;
		}
		index += PROT_CMD_TYPE_SIZE;

		//记录命令内容的起始位置和大小
		int iContentStart = index;
		int iContentSize = iBufLen - PROT_BEGIN_FLAG_SIZE - PROT_CMD_TYPE_SIZE - PROT_CRC_LEN - PROT_END_FLAG_SIZE;
		index += iContentSize;

		//获取命令内容
		if (iContentSize <= 0)
		{
			cCodecCmd = CODEC_DECODE_ERROR;
			return cCodecCmd;
		}
		*ppcBufOut = YKNew0(char, iContentSize);
		if (NULL == *ppcBufOut)
		{
			cCodecCmd = CODEC_DECODE_ERROR;
			break;
		}
		memcpy(*ppcBufOut, &pcDecode[iContentStart], iContentSize);

		if (!SMVerifyCMDContent(cCodecCmd, *ppcBufOut, iContentSize))
		{
			cCodecCmd = CODEC_DECODE_ERROR;
			LOG_RUNLOG_DEBUG("SM CODEC  ERROR");
		}

	} while (FALSE);

	return cCodecCmd;
}

void SMHndlCommData(SM_RCV_DATA_ST *pstPrmv)
{
	char* pcBuf = NULL;
	char cCmd = 0x00;

#ifdef _SM_EVENT_DEBUG_
	LOG_RUNLOG_DEBUG("**************READ MSG CONTENT**************");
	LOG_RUNLOG_DEBUG("pstPrmv->uiPrmvType: %d", pstPrmv->uiPrmvType);
	LOG_RUNLOG_DEBUG("pstPrmv->uiLen: %d", pstPrmv->uiLen);
	LOG_RUNLOG_DEBUG("pstPrmv->pcData: %s", pstPrmv->pcData);
	LOG_RUNLOG_DEBUG("********************************************");
#endif

	cCmd = SMHostDecode(pstPrmv->pcData, pstPrmv->uiLen, &pcBuf);

#ifdef _SM_EVENT_DEBUG_
	LOG_RUNLOG_DEBUG("SM RECV cCmd (0x%02X)", cCmd);
#endif

	if (cCmd == 0x30 || cCmd == 0x31)
	{
		//判断命令类型进行处理
		if (cCmd == CODEC_IDBT_CLIENT_REQ)
		{
			//处理接口板请求
			if (!SMHndlClientReq((CLIENT_REQ_PACKET_ST*) pcBuf))
			{
				cCmd = CODEC_ERROR;
			}
		}
		else
		{
			//处理接口板应答
			if (!SMHndlClientRsp((CLIENT_RSP_PACKET_ST*) pcBuf))
			{
				cCmd = CODEC_ERROR;
			}
		}
	}
	if(pstPrmv->pcData)
	{
		free(pstPrmv->pcData);
		pstPrmv->pcData = NULL;
	}
	if(pcBuf)
	{
		free(pcBuf);
		pcBuf = NULL;
	}
}


void* SMHndlMsgTask(void *pvPara)
{
	void *pvMsg = NULL;

	while(g_bRunHndlMsgTaskFlg)
	{
		//printf("**********************i am SMHndlMsgTask!\n");
		YKReadQue(g_pstSMMsgQ, &pvMsg, 0);			//阻塞读取
		if(NULL == pvMsg)
		{
			continue;
		}
		else
		{
#ifdef _SM_EVENT_DEBUG_
			LOG_RUNLOG_DEBUG("SM receive notification: %04X", *(( unsigned int *)pvMsg));
#endif
			switch(*(( unsigned int *)pvMsg))
			{
			/************************************************************/
#ifdef _YK_XT8126_AV_
			//定时器事件处理
			case SM_TIMER_EVENT:
				switch (((SM_TIMER_EVENT_ST *) pvMsg)->uiTimerId)
				{
				case TIMER_ID_HEARTBEAT: //心跳包处理
					SMSendHeartBeat();
					break;
				case TIMER_ID_PACKET_RSP_TIMEOUT: //应答包响应超时
					//接口板掉线告警
					if(g_iIfBoardStatus == SM_IF_BROARD_ONLINE)
					{
#if _TM_TYPE_ == _YK069_
						TMUpdateAlarmState(TM_ALARM_IF_BOARD, '1');
#else
						TMUpdateAlarmState(TM_ALARM_IF_BOARD, "1");
#endif
						g_iIfBoardStatus = SM_IF_BROARD_OFFLINE;
					}
					SMClientRspTimeOutReset();
					break;
				default:
					break;
				}
				break;
			/************************************************************/
			//串口接收数据处理
			case SM_SERIAL_DATA_RCV:
				SMHndlCommData((SM_RCV_DATA_ST *) pvMsg);
				break;
#endif
			/************************************************************/
			//CC消息处理
			case CCSM_CALLEE_PICK_UP:
			{
				LOG_RUNLOG_DEBUG("SM callee pick up!");
#ifdef _YK_XT8126_BV_
				SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
				if(NULL == pstMsg)
				{
					LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
					break;
				}
				pstMsg->uiPrmvType = SMXT_CALLEE_PICK_UP;
				YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);
#else
				if(g_iCallUserType == SM_CALL_NORMAL_USER)
				{
					SMCalleePickUp();
				}
				else
				{
					LOG_RUNLOG_DEBUG("SM call centre, do nothing!");
				}
#endif
				break;
			}
			case CCSM_CALLEE_HANG_OFF:
			{
				LOG_RUNLOG_DEBUG("SM callee hang off!");
#ifdef _YK_XT8126_BV_
				SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
				if(NULL == pstMsg)
				{
					LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
					break;
				}
				pstMsg->uiPrmvType = SMXT_CALLEE_HANG_OFF;
				YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);
#else

    //#ifdef _YK_XT8126_DOOR_
                if (g_iCallUserType == SM_CALL_CENTRE)
                {
//                    SMStopPlayWavFile();
                }
                LOG_RUNLOG_DEBUG("SM status change 11!");
    //#endif
                g_iCallStatus = 0;                  // add by cs 
                
				if(g_iCallUserType == SM_CALL_NORMAL_USER)
				{
					SMCalleeHandoff();
				}
				else
				{
					LOG_RUNLOG_DEBUG("SM call centre, do nothing!");
				}
#endif
				break;
			}
			case CCSM_CALLEE_ERR:
			{
				LOG_RUNLOG_DEBUG("SM call err!");

#ifdef _YK_XT8126_BV_
				SMXT_CALLEE_ERROR_MSG_ST *pstMsg = YKNew0(SMXT_CALLEE_ERROR_MSG_ST, 1);
				if(NULL == pstMsg)
				{
					LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
					break;
				}
				pstMsg->uiPrmvType = SMXT_CALLEE_ERR;
				pstMsg->uiErrorCause = ((SMXT_CALLEE_ERROR_MSG_ST *)pvMsg)->uiErrorCause;
				YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);
#else
                g_iCallStatus = 0;                  // add by cs
    //#ifdef _YK_XT8126_DOOR_
                LOG_RUNLOG_DEBUG("SM status change 22!");
                if (g_iCallUserType == SM_CALL_CENTRE)
                {
                    usleep(200000);
//                    SMStopPlayWavFile();
                    #ifdef _YK_XT8126_DOOR_
                    usleep(200000);
//                    SMPlayWavFile("/usr/wav/card_fail.wav", 0);
                    #endif
                }
    //#endif
#endif
				break;
			}
			case CCSM_CALLEE_MUSIC_END:
			{
				LOG_RUNLOG_DEBUG("SM callee music end!");
#ifdef _YK_XT8126_BV_
				SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
				if(NULL == pstMsg)
				{
					LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
					break;
				}
				pstMsg->uiPrmvType = SMXT_CALLEE_MUSIC_END;
				YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);
#else
//				SMCalleeMusicEnd();
//    #ifdef _YK_XT8126_DOOR_
                if (g_iCallUserType == SM_CALL_CENTRE)
                {
//                    SMStopPlayWavFile();
                }
//    #endif
#endif
				break;
			}
			case CCSM_CALLEE_SEND_DTMF:		//接收到DTMF信号
			{
				LOG_RUNLOG_DEBUG("SM callee send dtmf(%c)", ((CCSM_CALLEE_SEND_DTMF_ST *)pvMsg)->iDtmfType);
				if(((CCSM_CALLEE_SEND_DTMF_ST *)pvMsg)->iDtmfType == 0x23)	//电话开门
				{
					LOG_RUNLOG_DEBUG("SM open door!");
#ifdef _YK_XT8126_BV_
					SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
					if(NULL == pstMsg)
					{
						LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
						break;
					}
					pstMsg->uiPrmvType = SMXT_OPENDOOR;
					YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);
#else
					if(g_iCallUserType == SM_CALL_CENTRE)
					{
						SMOpenDoor();
					}
					else
					{
						if (SMSendHostReq(DTMF_SIGNAL, g_ucCurMachine) == FALSE)
						{
							LOG_RUNLOG_WARN("SM send host request failed!(DTMF_SIGNAL)");
						}
					}
#endif
				}
				else if(((CCSM_CALLEE_SEND_DTMF_ST *)pvMsg)->iDtmfType == 0x2A)		//LED灯控制
				{
#ifdef _YK_XT8126_BV_
					if(g_bFillLightOpenFlg == FALSE)
					{
						LOG_RUNLOG_INFO("SM: open led!");
						SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
						if(NULL == pstMsg)
						{
							LOG_RUNLOG_ERROR("SM: malloc error! file: %s, line: %d", __FILE__, __LINE__);
							break;
						}
						pstMsg->uiPrmvType = SMXT_OPEN_LED;
						YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);

						g_bFillLightOpenFlg = TRUE;
					}
					else
					{
						LOG_RUNLOG_INFO("SM: close led!");
						SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
						if(NULL == pstMsg)
						{
							LOG_RUNLOG_ERROR("SM: malloc error! file: %s, line: %d", __FILE__, __LINE__);
							break;
						}
						pstMsg->uiPrmvType = SMXT_CLOSE_LED;
						YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);

						g_bFillLightOpenFlg = FALSE;
					}

#endif
				}
			else if(((CCSM_CALLEE_SEND_DTMF_ST *)pvMsg)->iDtmfType == '1')  // 1 键，拒绝开门
			{
#ifdef _YK_XT8126_BV_
                LOG_RUNLOG_DEBUG("SM: callee reject!");
                SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
                if(NULL == pstMsg)
                {
                    LOG_RUNLOG_ERROR("SM: malloc error! file: %s, line: %d", __FILE__, __LINE__);
                    break;
                }
                pstMsg->uiPrmvType = SMXT_CALLEE_REJECT;
                YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);
#endif
            }
			break;
			}
			case CCSM_OPENDOOR:			//一键开门
			{
				LOG_RUNLOG_DEBUG("SM open door!");
#ifdef _YK_XT8126_BV_
				SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
				if(NULL == pstMsg)
				{
					LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
					break;
				}
				pstMsg->uiPrmvType = SMXT_OPENDOOR;
				YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);
#else
				SMOpenDoor();
#endif
				break;
			}
			case CCSM_VEDIO_MONITOR:
			{
				LOG_RUNLOG_DEBUG("SM VEDIO_MONITOR!");
#ifdef _YK_XT8126_BV_
				SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
				if(NULL == pstMsg)
				{
					LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
					break;
				}
				pstMsg->uiPrmvType = SMXT_VIDEO_MONITOR;
				YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);
#else
				LOG_RUNLOG_DEBUG("SM this is not BV version,do nothing!");
#endif
				g_bVedioMonitorFlg = TRUE;
				break;
			}
			case CCSM_VEDIO_MONITOR_CANCEL:
			{
				LOG_RUNLOG_DEBUG("SM vedio monitor cancel!");
#ifdef _YK_XT8126_BV_
				if(g_bVedioMonitorFlg == TRUE)
				{
					g_bVedioMonitorFlg = FALSE;
				}
				if(g_bFillLightOpenFlg == TRUE)
				{
					LOG_RUNLOG_DEBUG("SM close led!");
					SMXT_EVENT_MSG_ST *pstMsg = YKNew0(SMXT_EVENT_MSG_ST, 1);
					if(NULL == pstMsg)
					{
						LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
						break;
					}
					pstMsg->uiPrmvType = SMXT_CLOSE_LED;
					YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);

					g_bFillLightOpenFlg = FALSE;
				}
#endif
				break;
			}
			/************************************************************/
#ifdef _YK_XT8126_BV_
			//兴联设备通知南向事件
			case XTSM_INVOKE_LP_CALL:
			{
				LOG_RUNLOG_DEBUG("SM xt call start!");
				SMCC_DOOR_CALL_ST *pstMsg = YKNew0(SMCC_DOOR_CALL_ST, 1);
				if(NULL == pstMsg)
				{
					LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
					break;
				}
				pstMsg->uiPrmvType = SMCC_DOOR_CALL;
#ifdef YK_XT8126_BV_FKT
				pstMsg->ucDevType = 'F';
				strncpy((char *)g_acCurRoom, (char *)((XTSM_INVOKECALL_DATA_ST *)pvMsg)->aucCallAddr, 11);
				strncpy((char *)pstMsg->aucRoomNum, (char *)((XTSM_INVOKECALL_DATA_ST *)pvMsg)->aucCallAddr, 11);
#else
				pstMsg->ucDevType = '0';
				strncpy((char *)g_acCurRoom, (char *)((XTSM_INVOKECALL_DATA_ST *)pvMsg)->aucCallAddr, 4);
				strncpy((char *)pstMsg->aucRoomNum, "0000", 4);
				strncpy((char *)pstMsg->aucRoomNum+4, (char *)((XTSM_INVOKECALL_DATA_ST *)pvMsg)->aucCallAddr, 4);
#endif
				YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
				if(g_bVedioMonitorFlg == TRUE)
				{
					g_bVedioMonitorFlg = FALSE;
				}
				break;
			}
			case XTSM_TERMINATE_LP_CALL:
			{
				LOG_RUNLOG_DEBUG("SM xt hang off!");
				SMCC_DOOR_HANG_OFF_ST *pstMsg = YKNew0(SMCC_DOOR_HANG_OFF_ST, 1);
				if(NULL == pstMsg)
				{
					LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
					break;
				}
				pstMsg->uiPrmvType = SMCC_DOOR_HANG_OFF;

				YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
				break;
			}
			case XTSM_PICKUP_IDU:
			{
				LOG_RUNLOG_DEBUG("SM xt indoor pick up,call end!");
				SMCC_INDOOR_PICK_UP_ST *pstMsg = YKNew0(SMCC_INDOOR_PICK_UP_ST, 1);
				if(NULL == pstMsg)
				{
					LOG_RUNLOG_ERROR("SM malloc error! file: %s, line: %d", __FILE__, __LINE__);
					break;
				}
				pstMsg->uiPrmvType = SMCC_INDOOR_PICK_UP;

				YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
				break;
			}
#endif
			/************************************************************/
			//退出
			case SM_HNDL_MSG_CANCEL:
				LOG_RUNLOG_DEBUG("SM rcv cancel signal!");
				g_bRunHndlMsgTaskFlg = FALSE;
				break;

			default:
				break;
			}
		}
		if(pvMsg)
		{
			free(pvMsg);
			pvMsg = NULL;
		}
	}

	return NULL;
}

void* SMRcvComDataTask(void *pvPara)
{
	while (g_bRunCommRcvTaskFlg)
	{
		usleep(RCV_COM_DATA_INTERVAL);
		SMHndlComRcvData(&g_stRcvData);
	}
	return NULL;
}

#ifdef _YK_XT8126_DOOR_
/*****************************************************
add by cs  
*****************************************************/
void SMCheckSipStatus(void)
{
    static REG_STATUS_EN flag = REG_STATUS_ONLINE;
    REG_STATUS_EN temp;
    
    temp = LPGetSipRegStatus();
    if (temp != flag)
    {
        flag = temp;
        if (flag == REG_STATUS_ONLINE)
        {
            LOG_RUNLOG_DEBUG("SM REG_STATUS_ONLINE------");
            YK8126CloseFillLight();
        }
        else
        {
            LOG_RUNLOG_DEBUG("SM REG_STATUS_OFFLINE+++++");
            YK8126OpenFillLight();
        }
    }
}
#endif

/*****************************************************
modify by cs 
*****************************************************/
void* SMRcvKeyValueTask(void *pvPara)
{
	unsigned char adc_val=0;
	YK_TIME_VAL_ST start, end;
    unsigned char flag = 0;
	memset(&start, 0x0, sizeof(start));

	while (g_bRunCommRcvTaskFlg)
	{
        //while (read(g_iKeyFd, &adc_val, sizeof(adc_val)) > 0)
        if (read(g_iKeyFd, &adc_val, sizeof(adc_val)) > 0)
        {
        	//YKGetTimeOfDay(&end);
        	//if(end.lSeconds - start.lSeconds > 10)
        	if (flag == 0)                          // 防止同一次按下重复处理
        	{
                flag = 1;
            	//SMPlayWavFile("/usr/wav/card_succ.wav", 0);
//            	SMStopPlayWavFile();
                usleep(100000);
            	LOG_RUNLOG_DEBUG("SM AP get KEY ADC val = %x", adc_val);
                if((adc_val >= 0)&&(adc_val <= 255))    //
                {
                    if (g_iCallStatus == 0)
                    {
                        SMProcessCommand("call 0 00000000");
                    	g_iCallUserType = SM_CALL_CENTRE;
                        g_iCallStatus = 1;
                        LOG_RUNLOG_DEBUG("SM status change 33!");
                    	usleep(300000);            // 防止正在监视时的呼叫抢占音频
                    	if (g_iCallStatus == 1)
                    	{
//                        	SMPlayWavFile("/usr/wav/cartoon.wav", 1);
                        	//SMPlayWavFile("/mnt/mtd/cartoon.wav", 1);
                    	}
                        else
                        {
                            LOG_RUNLOG_DEBUG("SM g_iCallStatus = %d error\n", g_iCallStatus);
                        }
                    }
                    else if (g_iCallUserType == SM_CALL_CENTRE)
                    {
                        SMCC_DOOR_HANG_OFF_ST *pstMsg = YKNew0(SMCC_DOOR_HANG_OFF_ST, 1);
            			if(NULL == pstMsg)
            			{
            				return FAILURE;
            			}
                        g_iCallStatus = 0;
                        
                        LOG_RUNLOG_DEBUG("SM status change 44!");
            			pstMsg->uiPrmvType = SMCC_DOOR_HANG_OFF;

            			YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
                    }
                }
                //YKGetTimeOfDay(&start);
        	}
        }
        else
        {
            flag = 0;
        }
#ifdef _YK_XT8126_DOOR_
        SMCheckSipStatus();
#endif
        usleep(200000);
	}
	return NULL;
}
//#else
#if 0
void* SMRcvKeyValueTask(void *pvPara)
{
	unsigned char adc_val=0;
	YK_TIME_VAL_ST start, end;
    unsigned char flag = 0;
    
	memset(&start, 0x0, sizeof(start));

	while (g_bRunCommRcvTaskFlg)
	{
        if (read(g_iKeyFd, &adc_val, sizeof(adc_val)) > 0)
        {
        	//YKGetTimeOfDay(&end);
        	//if(end.lSeconds - start.lSeconds > 4)
        	if (flag == 0)                          // 防止同一次按下重复处理
        	{
                flag = 1;
                //SMPlayWavFile("/usr/wav/card_succ.wav", 0);
            	LOG_RUNLOG_DEBUG("SM AP get KEY ADC val = %x", adc_val);
                if((adc_val >= 0)&&(adc_val <= 255))    //
                {
//                    SMStopPlayWavFile();
                    usleep(100000);
                    if (g_iCallStatus == 0)                                 // add by cs 2012-10-12
                    {
                    	//usleep(3000000);
                    	SMProcessCommand("call 0 00000000");
                        usleep(100000);
//                        SMPlayWavFile("/usr/wav/cartoon.wav", 1);
                    	g_iCallUserType = SM_CALL_CENTRE;
                        g_iCallStatus = 1;                      // add by cs 2012-10-12
                    }
                    
                    else if (g_iCallUserType == SM_CALL_CENTRE)
                    {
                        SMCC_DOOR_HANG_OFF_ST *pstMsg = YKNew0(SMCC_DOOR_HANG_OFF_ST, 1);
                        
            			if(NULL == pstMsg)
            			{
            				return FAILURE;
            			}
                        g_iCallStatus = 0;
                        
                        LOG_RUNLOG_DEBUG("SM call center handoff!");
            			pstMsg->uiPrmvType = SMCC_DOOR_HANG_OFF;

            			YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
                    }
            	}

                //YKGetTimeOfDay(&start);
            }
        }
        else
        {
            flag = 0;
        }
        usleep(100000);
	}
	return NULL;
}
#endif

int SMKeyValueRcvInit(void)
{
	g_iKeyFd = open(KEY_DEV_NAME, O_RDONLY|O_NONBLOCK);
    if (g_iKeyFd < 0)
    {
    	LOG_RUNLOG_ERROR("SM Can't open %s.", KEY_DEV_NAME);
        return FAILURE;
    }
    LOG_RUNLOG_DEBUG("SM key_fd = %d", g_iKeyFd);

    return SUCCESS;
}

int SMComRcvInit(void)
{
	LOG_RUNLOG_DEBUG("SM communication Initialize");

	g_iComFd = SMOpenComPort(1, 9600, 8, "1", 'N');
	if (-1 == g_iComFd)
	{
		LOG_RUNLOG_ERROR("SM Com Open failed!");
		return FAILURE;
	}

	g_bRunCommRcvTaskFlg = TRUE;

	// 初始化设备文件数据接收缓冲区的数据结构
	memset(g_stRcvData.acBuf, 0x0, COMM_DATA_RCV_SIZE * sizeof(unsigned char));
	g_stRcvData.iLen = 0;
	g_stRcvData.pcHead = g_stRcvData.acBuf;
	g_stRcvData.ucCounter = 0;
	g_stRcvData.ucState = BEGIN_FLAG_NOT_FOUND;

	return SUCCESS;
}

int SMInit(void)
{
	int iSta = SUCCESS;

#if defined(_YK_MN2440_AA_)  || defined(_YK_S5PV210_)
	//初始化IO口
	g_iIoFd = open(IO_DRIVER, 0);
	if(g_iIoFd<0)
	{
		LOG_RUNLOG_ERROR("SM open IO driver device failed!");
		return FAILURE;
	}
	LOG_RUNLOG_DEBUG("SM open IO driver device ok!");
#endif
	SMCtlLightLed(0);

	//创建SM消息队列
	g_pstSMMsgQ  = YKCreateQue(SM_MSG_QUE_LEN);

#if (defined(_YK_MN2440_AA_)||defined(_YK_XT8126_AV_))
	//串口接收缓冲区初始化
	iSta = SMComRcvInit();
	if(iSta != SUCCESS)
	{
		LOG_RUNLOG_ERROR("SM ComRcvInit init failed!");
		return FAILURE;
	}
#endif

#if defined(_YK_XT8126_AV_) || defined(_YK_S5PV210_)
	//一键呼叫初始化
/*	iSta = SMKeyValueRcvInit();
	if (iSta == FAILURE)
	{
		LOG_RUNLOG_ERROR("SM one key init failed!");
		return FAILURE;
	}*/

#ifndef _YK_XT8126_DOOR_    
	//RF刷卡模块初始化
	LOG_RUNLOG_ERROR("SM AV2.0 rf init!");
	iSta = SMRfInit();
	if (iSta == FAILURE)
	{
		LOG_RUNLOG_ERROR("SM AV2.0 rf init failed!");
		//return FAILURE;     只记录日志，不做返回值处理，防止一直重启
	}
#endif

	//SM定时器初始化
	iSta = SMTimerInit();
	if(iSta != SUCCESS)
	{
		LOG_RUNLOG_ERROR("SM timer init failed!");
		return FAILURE;
	}
#endif

#if (defined(_YK_MN2440_AA_)||defined(_YK_XT8126_AV_))
	//创建串口数据接收线程
	iSta = pthread_create(&g_rcvComData, NULL, SMRcvComDataTask, NULL);
	if(iSta != SUCCESS)
	{
		LOG_RUNLOG_ERROR("SM create rcv comm data task failed!");
		return FAILURE;
	}
#endif

#ifdef _YK_XT8126_AV_
	//创建串口数据接收线程 add by amos.
	iSta = pthread_create(&g_rcvKeyMsg, NULL, SMRcvKeyValueTask, NULL);
	if(iSta != SUCCESS)
	{
		LOG_RUNLOG_ERROR("SM create rcv comm data task failed!");
		return FAILURE;
	}
#endif

	//创建SM消息接收及处理线程
	pthread_create(&g_HndlMsgTask, NULL, SMHndlMsgTask, NULL);
	if(iSta != SUCCESS)
	{
		LOG_RUNLOG_ERROR("SM create handle message task failed!");
		return FAILURE;
	}

//    pthread_mutex_init(&snd_card_lock, NULL);       // add by cs 2012-09-29

	return SUCCESS;
}

void SMFini(void)
{
	//退出事件处理线程
	SM_EXIT_MSG_ST *pstMsg = YKNew0(SM_EXIT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return ;
	}
	pstMsg->uiPrmvType = SM_HNDL_MSG_CANCEL;
	YKWriteQue(g_pstSMMsgQ , pstMsg,  0);


#ifdef _YK_XT8126_AV_
	//退出串口数据接收线程
	g_bRunCommRcvTaskFlg = FALSE;

	YKJoinThread((void *)g_rcvComData);
	YKDestroyThread((void *)g_rcvComData);
	//add by amos
	if(g_iKeyFd>=0)
	{
		close(g_iKeyFd);
		YKJoinThread((void *)g_rcvKeyMsg);
	}
#endif

	YKJoinThread((void *)g_HndlMsgTask);
	YKDestroyThread((void *)g_HndlMsgTask);

#ifdef _YK_XT8126_AV_
	//删除定时器
#ifndef _YK_XT8126_DOOR_
	YKRemoveTimer(g_stHeartBeatTimer.pstTimer,g_stHeartBeatTimer.ulMagicNum);
	YKRemoveTimer(g_stClientRspTimeoutTimer.pstTimer,g_stClientRspTimeoutTimer.ulMagicNum);
#else
    YKRemoveTimer(g_stUnlockTimer.pstTimer,g_stUnlockTimer.ulMagicNum);
#endif
#endif

	//删除消息队列
	YKDeleteQue(g_pstSMMsgQ);

#if defined(_YK_XT8126_AV_) || defined(_YK_S5PV210_)
    #ifndef _YK_XT8126_DOOR_ 
	//释放读卡器相关资源
	SMRfFini();
    #endif
#endif

//    pthread_mutex_destroy(&snd_card_lock);

}

int SMProcessCommand(const char *pcCmd)
{
	char acTmp[20] = {0};
	char *pcTmp = NULL;
	char *pcDevType = NULL;
	char *pcCmdText = NULL;

	if(pcCmd == NULL || strlen(pcCmd) == 0)
	{
		LOG_RUNLOG_DEBUG("SMC input cmd error!");
		return FAILURE;
	}

	strcpy(acTmp,pcCmd);

	if(SMStrChrCnt(acTmp, strlen(acTmp), ' ') != 2)
	{
		LOG_RUNLOG_DEBUG("SMC input cmd format error!");
		LOG_RUNLOG_DEBUG("*************************************************");
		LOG_RUNLOG_DEBUG("format: SMC + EVENT TPYE + MACHINE TYPE + ROOMNUM");
		LOG_RUNLOG_DEBUG("eg: SMC call 0 00000101");
		LOG_RUNLOG_DEBUG("*************************************************");
		return FAILURE;
	}

	pcTmp = strchr(acTmp, ' ');
	if(NULL != pcTmp)
	{
		*pcTmp = '\0';
		pcDevType = ++pcTmp;
	}

	pcTmp = strchr(pcTmp, ' ');
	{
		*pcTmp = '\0';
		pcCmdText = ++pcTmp;
	}

	LOG_RUNLOG_DEBUG("pcDevType: %s pcTmp: %s pcCmdText: %s",pcDevType, pcTmp, pcCmdText);

	if(0 == strcmp(acTmp, "call"))
	{
		SMCC_DOOR_CALL_ST *pstMsg = (SMCC_DOOR_CALL_ST *)malloc(sizeof(SMCC_DOOR_CALL_ST));
		if(NULL == pstMsg)
		{
			return FAILURE;
		}
		memset(pstMsg, 0x00, sizeof(SMCC_DOOR_CALL_ST));
		pstMsg->uiPrmvType = SMCC_DOOR_CALL;
		pstMsg->ucDevType = pcDevType[0];
		strcpy((char *)pstMsg->aucRoomNum, pcCmdText);
		//===========add by hb===========
		ATTestOK(AT_TEST_OBJECT_SM);
		//===============================
		YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
	}
	else if(0 == strcmp(acTmp, "pickup"))
	{
		CCSM_CALLEE_PICK_UP_ST *pstMsg = (CCSM_CALLEE_PICK_UP_ST *)malloc(sizeof(CCSM_CALLEE_PICK_UP_ST));
		if(NULL == pstMsg)
		{
			return FAILURE;
		}
		memset(pstMsg, 0x00, sizeof(CCSM_CALLEE_PICK_UP_ST));
		pstMsg->uiPrmvType = SMCC_INDOOR_PICK_UP;

		YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
	}
	else if(0 == strcmp(acTmp, "hangoff"))
	{
		SMCC_DOOR_HANG_OFF_ST *pstMsg = (SMCC_DOOR_HANG_OFF_ST *)malloc(sizeof(SMCC_DOOR_HANG_OFF_ST));
		if(NULL == pstMsg)
		{
			return FAILURE;
		}
		memset(pstMsg, 0x00, sizeof(SMCC_DOOR_HANG_OFF_ST));
		pstMsg->uiPrmvType = SMCC_DOOR_HANG_OFF;

		YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
	}
	else
	{
		LOG_RUNLOG_DEBUG("SMC input cmd event error!");
	}

	return SUCCESS;
}


#endif
