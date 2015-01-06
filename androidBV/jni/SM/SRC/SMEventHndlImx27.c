#ifdef _YK_IMX27_AV_

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
#include "../INC/SMUtil.h"
#include "../INC/SMSerial.h"
#include "../INC/SMEventHndlImx27.h"
#include "../INC/SMRfCard.h"
#include "../INC/SMDtmfSerial.h"
#include "TMInterface.h"
#include "LOGIntfLayer.h"
#include "IMX27Gpio.h"
#ifdef _I6_CC_
#include "I6CCTask.h"
#else
#include "CCTask.h"
#endif
#include "../../AT/INC/AutoTest.h"

//���ڽ���������ʼ������־��״̬
#define BEGIN_FLAG_NOT_FOUND    0
#define BEGIN_FLAG_FOUND        1

//���ڽ���������ʼ������־
#define PROT_BEGIN_FLAG         '^'
#define PROT_END_FLAG           '~'

#define RECV_LMT_TIMES   5

//IO����·��
#define IO_DRIVER	 			"/dev/iDBT2440_ioctrl"

//Э���������ݶ���.
#define	CODEC_IDBT_CLIENT_REQ    0x30
#define	COEDC_IDBT_HOST_REQ      0x31
#define    COEDC_IDBT_CLIENT_RES    0x32
#define    COEDC_IDBT_HOST_RESERVED 0x35    //��ͬ����Ԥ��

//���������.
#define CODEC_ERROR            0xff        //��������
#define CODEC_CRC_ERROR        0xfe        //CRCУ�����
#define CODEC_ENCODE_ERROR     0xfd        //�������
#define CODEC_DECODE_ERROR     0xfc        //�������

//Э�����ݳ���
#define PROT_BEGIN_FLAG_SIZE   1
#define PROT_CMD_TYPE_SIZE     1
#define PROT_MACHINE_TYPE      1
#define PROT_CRC_LEN           4
#define PROT_END_FLAG_SIZE     1

//�������ݽ��������������
#define OPERATOR_CRC_ERROR    'C'
#define OPERATOR_ERROR        'E'
#define OPERATOR_REJECT       'R'
#define OPERATOR_PASS         'P'

#define SM_IF_BROARD_ONLINE		1				//�ӿڰ�����
#define SM_IF_BROARD_OFFLINE       0               //�ӿڰ�����

static volatile unsigned char g_ucCurMachine = DEV_TYPE_TTL;      //��ǰ�豸����
static volatile char g_cCurCmd;                                   //��ǰ���͵�����

SM_TIMER_ST g_stHeartBeatTimer;                                   //������ʱ��
SM_TIMER_ST g_stClientRspTimeoutTimer;                            //Ӧ��ʱ��ʱ��

static COM_DATA_RCV_ST g_stRcvData;								  //���ڽ�������
static int g_iComFd = 0;					                      //�����ļ��ļ�������
static int g_iComDtmfFd = 0;                                      //����DTMF�ļ�������
int g_iIoFd;						                              //IO�豸�����ļ�������

pthread_t g_rcvComData;				                              //���ڽ����̱߳�ʶ��
pthread_t g_HndlMsgTask;					                      //SM��Ϣ���ռ������̱߳�ʶ��

static BOOL g_bRunHndlMsgTaskFlg = TRUE;						  //��Ϣ���ռ������߳�����״̬��־
static BOOL g_bRunCommRcvTaskFlg = TRUE;                         //���ڽ����߳�����״̬��־

YK_MSG_QUE_ST*  g_pstSMMsgQ = NULL;                                //SM��Ϣ����ָ��
extern YK_MSG_QUE_ST* g_pstCCMsgQ;                                //CC��Ϣ����ָ��

static volatile int g_iIfBoardStatus = SM_IF_BROARD_OFFLINE;	   //�ӿڰ��Ƿ�����״̬


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

	//���õĴ����͵�������ʱ����������յ����ݵ���Ч��
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
	LOG_RUNLOG_DEBUG("SM [SM_MESSAGE] -SMRecv[%d] : %s", iRcvDataLen, acTmpBuf);

	if(&pstComDataRcv->acBuf[COMM_DATA_RCV_SIZE] < pstComDataRcv->pcHead + pstComDataRcv->iLen + iRcvDataLen)
	{
		//�������ݶ���գ��ָ���ʼ״̬
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
					pstComDataRcv->ucCounter = 0; //�յ�������־��counter����
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
				if(iIndex == 0) //������־δ�յ���Ҫ�������գ�counter��ʼ����
				{
					pstComDataRcv->ucCounter++;
				}
				else
				{
					pstComDataRcv->ucCounter = 0; //�յ�������־��counter����
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

	//�����ݵ�����buf��ǰ��
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

		int index = 0; //Э����±�

		*ppBufOut = YKNew0(char, iCodecSize);
		if (NULL == *ppBufOut)
		{
			break;
		}
		memset(*ppBufOut, 0x0, iCodecSize);
		//��ʼ��־
		(*ppBufOut)[index++] = PROT_BEGIN_FLAG;

		//��������
		(*ppBufOut)[index++] = cCmdType;
		//��������
		memcpy(&(*ppBufOut)[index], pcBufIn, iBufLen);
		index += iBufLen;
		//У����
		unsigned short crcValue = SMCalcuCRC(&(*ppBufOut)[PROT_BEGIN_FLAG_SIZE], iBufLen + PROT_CMD_TYPE_SIZE, 0);
		sprintf((char *) &(*ppBufOut)[index], "%04X", crcValue);
		index += PROT_CRC_LEN;
		//������־
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

	//��¼���η�����Ϣ������ʧ�ܵ�ʱ�ط�
	g_cCurCmd = cEventType;

	//�����ն���ӿڰ巢������
	uiLen = SMHostEncode(cCMD, (char*) &stHostReqContent, sizeof(HOST_REQ_PACKET_ST), &pcBuf);

	if (uiLen > 0)
	{
		SMWriteComPort(g_iComFd, pcBuf, uiLen);
		return TRUE;
	}

	return FALSE;
}

BOOL SMSendHostRemark(char cEventType, char *acRemark)
{
	char *pcBuf = NULL;
	unsigned int uiLen = 0;
	HOST_REMARK_PACKET_ST stHostRemarkContent;
	char cCMD = COEDC_IDBT_HOST_RESERVED;

	memset(&stHostRemarkContent, 0x00, sizeof(HOST_REMARK_PACKET_ST));
	stHostRemarkContent.acEventType[0] = cEventType;
	strncpy(stHostRemarkContent.acRemark, acRemark, 8);

	//��¼���η�����Ϣ������ʧ�ܵ�ʱ�ط�
	g_cCurCmd = cEventType;

	//�����ն���ӿڰ巢������
	uiLen = SMHostEncode(cCMD, (char*) &stHostRemarkContent, sizeof(HOST_REMARK_PACKET_ST), &pcBuf);

	if (uiLen > 0)
	{
		SMWriteComPort(g_iComFd, pcBuf, uiLen);
		return TRUE;
	}

	return FALSE;
}

BOOL SMSendHostRsq(char cContent)
{
	char *pcBuf = NULL;
	unsigned int uiLen = 0;
	HOST_RSP_PACKET_ST stHostRsqContent;
	char cCMD = COEDC_IDBT_HOST_REQ;

	memset(&stHostRsqContent, 0x00, sizeof(HOST_REQ_PACKET_ST));
	stHostRsqContent.cCmdContent = cContent;

	//�����ն���ӿڰ巢������
	uiLen = SMHostEncode(cCMD, (char*) &stHostRsqContent, sizeof(HOST_RSP_PACKET_ST), &pcBuf);

	if (uiLen > 0)
	{
		SMWriteComPort(g_iComFd, pcBuf, uiLen);
		return TRUE;
	}

	return FALSE;
}

BOOL SMNMDevReboot(void)
{
	if(SMSendHostReq(SMIB_INTERFACE_BOARD_REBOOT, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(device reboot ...)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (device reboot ...)");
	return TRUE;
}

BOOL SMOpenDoor(void)
{
	if(SMSendHostReq(SMIB_DTMF_SIGNAL, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(open door)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (open door)");
	return TRUE;
}

BOOL SMCalleeMusicEnd(void)
{
	if (SMSendHostReq(SMIB_CALL_AUDIO_CONNECT, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(audio connect");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (audio connect)");
	return TRUE;
}

BOOL SMCalleePickUp(void)
{
	if (SMSendHostReq(SMIB_CALLEE_PICK_UP, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(CALLEE_PICK_UP)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (callee pick up)");
	return TRUE;
}

BOOL SMCalleeHandoff(void)
{
	if (SMSendHostReq(SMIB_CALLEE_HANG_OFF, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(CALLEE_HANG_OFF)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (callee hand off)");
	return TRUE;
}

BOOL SMVedioMonitor(void)
{
	if(SMSendHostReq(SMIB_MONITOR_REQUSET, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(vedio monitor)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (vedio monitor)");
	return TRUE;
}

BOOL SMVedioMonitorCancel(void)
{
	if(SMSendHostReq(SMIB_MONITOR_CANCEL, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send msg failed!(vedio monitor cancel)");
		return FALSE;
	}
	LOG_RUNLOG_DEBUG("SM send msg: (vedio monitor cancel)");
	return TRUE;
}

BOOL SMSendHeartBeat(void)
{
	if(SMSendHostReq(SMIB_HEART_BEAT, g_ucCurMachine) == FALSE)
	{
		LOG_RUNLOG_WARN("SM send heart beat failed!");
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
		return;
	}

	pstPrmv->uiPrmvType = SM_TIMER_EVENT;
	pstPrmv->uiTimerId = TIMER_ID_PACKET_RSP_TIMEOUT;

	YKWriteQue(g_pstSMMsgQ , pstPrmv,  0);
}

int SMClientRspTimeOutReset(void)
{
	//ɾ����ʱ��ʱ��
	YKRemoveTimer(g_stClientRspTimeoutTimer.pstTimer,g_stClientRspTimeoutTimer.ulMagicNum);

	//��ӳ�ʱ��ʱ��
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
	//������ʱ��
	g_stHeartBeatTimer.pstTimer = YKCreateTimer(SMHeartBeatCallback, NULL, HEARTBEAT_INTERVAL,
			YK_TT_PERIODIC, (unsigned long *)&g_stHeartBeatTimer.ulMagicNum);
	if(NULL == g_stHeartBeatTimer.pstTimer)
	{
		LOG_RUNLOG_ERROR("SM create heartbeat timer failed!");
		return FAILURE;
	}

	//Ӧ�����ʱ��ʱ��
	g_stClientRspTimeoutTimer.pstTimer = YKCreateTimer(SMClientRspTimeOutCallback, NULL, CLIENT_RSP_TIMROUT,
			YK_TT_NONPERIODIC, (unsigned long *)&g_stClientRspTimeoutTimer.ulMagicNum);
	if(NULL == g_stClientRspTimeoutTimer.pstTimer)
	{
		LOG_RUNLOG_ERROR("SM create client rsp timer failed!");
		return FAILURE;
	}
	return SUCCESS;
}

int SMHndlClientReq(CLIENT_REQ_PACKET_ST *pstContent)
{
	char acClientReqType[1] = { 0 };

	strncpy(acClientReqType, pstContent->acEventType, sizeof(pstContent->acEventType));

	switch (acClientReqType[0])
	{
		case IBSM_DOOR_MACHINE_CALL:
		{
			LOG_RUNLOG_DEBUG("SM IB call start!");
			SMCC_DOOR_CALL_ST *pstMsg = YKNew0(SMCC_DOOR_CALL_ST, 1);
			if(NULL == pstMsg)
			{
				return FAILURE;
			}
			pstMsg->uiPrmvType = SMCC_DOOR_CALL;
			switch(((char *)pstContent->acCallerId)[0])
			{
			case 'M':	//�������Ļ�
				pstMsg->ucDevType = 'M';
				break;
			case 'C':	//ҵ��ƽ̨
				pstMsg->ucDevType = 'C';
				break;
			case 'E':	//���ڻ�
				pstMsg->ucDevType = 'E';
				break;

			default:	//�ݿڻ��򻧻�ͨ
				pstMsg->ucDevType = '0';
				break;
			}

			strncpy((char *)pstMsg->aucRoomNum, (char *)pstContent->acCalleeId, 8);			//ֻ��4λ��źͷ���

			YKWriteQue(g_pstCCMsgQ , pstMsg,  0);

#ifndef _YK_IMX27_SINGLE_
			SMSendHostRsq('P');
#endif
			break;
		}
		case IBSM_INDOOR_MACHINE_PICK_UP:
		{
			LOG_RUNLOG_DEBUG("SM IB indoor pick up,call end!");
			SMCC_INDOOR_PICK_UP_ST *pstMsg = YKNew0(SMCC_INDOOR_PICK_UP_ST, 1);
			if(NULL == pstMsg)
			{
				return FAILURE;
			}
			pstMsg->uiPrmvType = SMCC_INDOOR_PICK_UP;

			YKWriteQue(g_pstCCMsgQ , pstMsg,  0);

			SMSendHostRsq('P');

			break;
		}
		case IBSM_DOOR_MACHINE_HANG_OFF:
		{
			LOG_RUNLOG_DEBUG("SM IB door machine hang off,call end!");
			SMCC_DOOR_HANG_OFF_ST *pstMsg = YKNew0(SMCC_DOOR_HANG_OFF_ST, 1);
			if(NULL == pstMsg)
			{
				return FAILURE;
			}
			pstMsg->uiPrmvType = SMCC_DOOR_HANG_OFF;

			YKWriteQue(g_pstCCMsgQ, pstMsg, 0);

			SMSendHostRsq('P');

			break;
		}
		case IBSM_RESERVED:
		{
			LOG_RUNLOG_DEBUG("SM IB reserved,do nothing!");

			SMSendHostRsq('P');

			break;
		}
		case IBSM_READ_CARD:
		{
			LOG_RUNLOG_DEBUG("SM IB read card!");
			char acCardNo[17] = {0};
			BOOL iRfCardNoExist = TRUE;
			strncpy(acCardNo, (char *)pstContent->acRemark, 16);
			LOG_RUNLOG_DEBUG("SM cardNo is %s", acCardNo+8);
			if(TMSMIsICOpenDoorEnabled() == TRUE)
			{
				iRfCardNoExist = TMSMIsValidICIDCard(acCardNo+8);
				if(iRfCardNoExist == FALSE)
				{
					LOG_RUNLOG_DEBUG("SM rf card id is invalid!");
				}
				else
				{
					SMOpenDoor();
					//==============ADD BY HB==================
					TMSetICIDOpenDoor(acCardNo+8);
					//==============ADD BY HB==================
					LOG_RUNLOG_DEBUG("SM rf card id is on the list,open the door!");
					LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_CARD_SWIPING, 0, acCardNo+8);
				}
			}
			else
			{
				LOG_RUNLOG_DEBUG("SM IC open door is disable!");
			}
			break;
		}
		case IBSM_OPEN_DOOR:
		{
			LOG_RUNLOG_DEBUG("SM indoor machine open door!");

			char acRoomNum[9] = {0};

			strncpy(acRoomNum, (char *)pstContent->acCallerId, 8);

			//д����־
			LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, acRoomNum, LOG_EVT_INDOOR_OPEN_DOOR, 0, "");
			LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);

			SMSendHostRsq('P');

			break;
		}
		case IBSM_MONITOR_CANCEL:
		{
			LOG_RUNLOG_DEBUG("SM IB monitor cancel!");
			SMCC_DOOR_HANG_OFF_ST *pstMsg = YKNew0(SMCC_DOOR_HANG_OFF_ST, 1);
			if(NULL == pstMsg)
			{
				return FAILURE;
			}
			pstMsg->uiPrmvType = SMCC_DOOR_DET_CANCEL;

			YKWriteQue(g_pstCCMsgQ, pstMsg, 0);

			SMSendHostRsq('P');

			break;
		}
		case IBSM_ALARM:
		{
			LOG_RUNLOG_DEBUG("SM IB send alarm!");
			char acTmp[11] = {0};
			char acAlarmInfo[17] = {0};
			strncpy(acAlarmInfo, (char *)pstContent->acRemark, 16);
			strncpy(acTmp, (char *)pstContent->acCalleeId, 8);
			strcat(acTmp, acAlarmInfo+14);
			LOG_RUNLOG_DEBUG("SM alarm is %s", acTmp);
			TMUpdateAlarmState(TM_ALARM_IF_BOARD, acTmp);
			SMSendHostRsq('P');
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
	case OPERATOR_REJECT :
		//�����ط�����
//		if(SMSendHostReq(g_cCurCmd, g_ucCurMachine) == FALSE)
//		{
//			return bStatus;
//		}

		if(ATIsRunning == 0)
		{
			if(g_cCurCmd == SMIB_MONITOR_REQUSET)
			{
				LOG_RUNLOG_DEBUG("SM IB reject, vedio monitor cancel!");
				SMCC_DOOR_HANG_OFF_ST *pstMsg = YKNew0(SMCC_DOOR_HANG_OFF_ST, 1);
				if(NULL == pstMsg)
				{
					return FALSE;
				}
				pstMsg->uiPrmvType = SMCC_DOOR_DET_CANCEL;

				YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
			}
			if(g_cCurCmd == SMIB_CALL_AUDIO_CONNECT)
			{
				LOG_RUNLOG_DEBUG("SM IB reject, terminatated all calls!");
				SMCC_DOOR_HANG_OFF_ST *pstMsg = YKNew0(SMCC_DOOR_HANG_OFF_ST, 1);
				if(NULL == pstMsg)
				{
					return FAILURE;
				}
				pstMsg->uiPrmvType = SMCC_DOOR_HANG_OFF;

				YKWriteQue(g_pstCCMsgQ, pstMsg, 0);
				break;
			}
		}
		else
		{
			LOG_RUNLOG_DEBUG("SM IB reject, do nothing! (auto testing...)");
		}
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
		bStatus = TRUE;
		SMClientRspTimeOutReset();
		break;

	default:
		break;
	}

	return bStatus;
}

BOOL SMVerifyCMDContent(const char* pcContent, int iSize)
{
	BOOL bVerify = FALSE;

	//printf("iSize: %d, %d, %d\n", iSize, sizeof(CLIENT_REQ_PACKET_ST), sizeof(CLIENT_RSP_PACKET_ST));

	//У��ӿڰ���������
	if (iSize == sizeof(CLIENT_REQ_PACKET_ST))
	{
		CLIENT_REQ_PACKET_ST* pClientReq = (CLIENT_REQ_PACKET_ST*) pcContent;
		char acTmp[5] = { 0 };

		//У���¼�����
		strncpy(acTmp, pClientReq->acEventType, sizeof(pClientReq->acEventType));
		if (!SMVerifyHexAscii(acTmp, sizeof(pClientReq->acEventType)))
		{
			LOG_RUNLOG_DEBUG("SM verify event type ascii error!");
			return bVerify;
		}
		int iValue = SMHexAsciiToDecimal(acTmp);
		if (iValue > 0x7 || iValue < 0x0)
		{
			LOG_RUNLOG_DEBUG("SM verify event type error!");
			return bVerify;
		}
		//У�鷿��
		strncpy(acTmp, pClientReq->acCallerId+4, 4);
		if (!SMVerifyHexAscii(acTmp, 4))
		{
			LOG_RUNLOG_DEBUG("SM verify room num ascii error!");
			return bVerify;
		}
		int iNum = SMHexAsciiToDecimal(acTmp);
		if (iNum > 0xFFFF || iNum < 0x0000)
		{
			LOG_RUNLOG_DEBUG("SM verify room num error!");
			return bVerify;
		}
		bVerify = TRUE;
	}
	//У������ն�����Ӧ��
	if (iSize == sizeof(CLIENT_RSP_PACKET_ST))
	{
		CLIENT_RSP_PACKET_ST* pClientRes = (CLIENT_RSP_PACKET_ST*) pcContent;
		char acTmp[sizeof(pClientRes->acCmdContent) + 1] = { 0 };

		//У���¼�����
		strncpy(acTmp, pClientRes->acCmdContent, sizeof(pClientRes->acCmdContent));
		if (acTmp[0] == 'P' || acTmp[0] == 'C' || acTmp[0] == 'E' || acTmp[0] == 'R')
		{
			bVerify = TRUE;
		}
	}

	return bVerify;
}

char SMHostDecode(const char* pcDecode, int iBufLen, char**ppcBufOut)
{
	char cCodecCmd = CODEC_ERROR;
	char cCmd;

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

		//Э�����־��
		if (pcDecode[index] != PROT_BEGIN_FLAG || pcDecode[iBufLen - 1] != PROT_END_FLAG)
		{
			cCodecCmd = CODEC_ERROR;
			LOG_RUNLOG_DEBUG("SM FLAG ERROR");
			SMSendHostRsq('E');
			break;
		}
		index += PROT_BEGIN_FLAG_SIZE;

		//CRCУ��
		char acTmp[PROT_CRC_LEN + 1] = {0x0};
		unsigned short usCrcVal = SMCalcuCRC(pcDecode + index, iBufLen - PROT_BEGIN_FLAG_SIZE - PROT_END_FLAG_SIZE - PROT_CRC_LEN, 0);
		memcpy(acTmp, pcDecode + iBufLen - PROT_END_FLAG_SIZE - PROT_CRC_LEN, PROT_CRC_LEN);
		unsigned short acTmpDec = (unsigned short) SMHexAsciiToDecimal(acTmp);

		if (usCrcVal != acTmpDec)
		{
			cCodecCmd = CODEC_CRC_ERROR;
			LOG_RUNLOG_DEBUG("SM CRC ERROR");
			SMSendHostRsq('C');
			break;
		}

		// ��ȡ��������
		memset(acTmp, 0x0, sizeof(acTmp));
		memcpy(acTmp, &pcDecode[index], PROT_CMD_TYPE_SIZE);
		cCmd = acTmp[0];
		if (cCmd != 0x30)
		{
			cCodecCmd = CODEC_DECODE_ERROR;
			LOG_RUNLOG_DEBUG("SM cCmd TYPE ERROR");
			SMSendHostRsq('E');
			break;
		}
		index += PROT_CMD_TYPE_SIZE;

		//��¼�������ݵ���ʼλ�úʹ�С
		int iContentStart = index;
		int iContentSize = iBufLen - PROT_BEGIN_FLAG_SIZE - PROT_CMD_TYPE_SIZE - PROT_CRC_LEN - PROT_END_FLAG_SIZE;
		index += iContentSize;

		//��ȡ��������
		if (iContentSize <= 0)
		{
			cCodecCmd = CODEC_DECODE_ERROR;
			SMSendHostRsq('E');
			break;
		}
		*ppcBufOut = YKNew0(char, iContentSize);
		if (NULL == *ppcBufOut)
		{
			cCodecCmd = CODEC_DECODE_ERROR;
			break;
		}
		memcpy(*ppcBufOut, &pcDecode[iContentStart], iContentSize);

		if (!SMVerifyCMDContent(*ppcBufOut, iContentSize))
		{
			cCodecCmd = CODEC_DECODE_ERROR;
			LOG_RUNLOG_DEBUG("SM CODEC  ERROR");
			SMSendHostRsq('E');
			break;
		}

		if(iContentSize == sizeof(CLIENT_REQ_PACKET_ST))
		{
			cCodecCmd = CODEC_IDBT_CLIENT_REQ;
		}
		else
		{
			cCodecCmd =COEDC_IDBT_CLIENT_RES;
		}

	} while (FALSE);

	return cCodecCmd;
}

void SMHndlCommData(SM_RCV_DATA_ST *pstPrmv)
{
	char* pcBuf = NULL;
	char cCmd = 0x00;

#ifdef _SM_EVENT_DEBUG_
	LOG_RUNLOG_DEBUG("*************READ MSG CONTENT**************");
	LOG_RUNLOG_DEBUG("pstPrmv->uiPrmvType: %d", pstPrmv->uiPrmvType);
	LOG_RUNLOG_DEBUG("pstPrmv->uiLen: %d", pstPrmv->uiLen);
	LOG_RUNLOG_DEBUG("pstPrmv->pcData: %s", pstPrmv->pcData);
	LOG_RUNLOG_DEBUG("********************************************");
#endif

	cCmd = SMHostDecode(pstPrmv->pcData, pstPrmv->uiLen, &pcBuf);

#ifdef _SM_EVENT_DEBUG_
	LOG_RUNLOG_DEBUG("SM RECV: cCmd (0x%02X)", cCmd);
#endif

	if (cCmd == 0x30 || cCmd == 0x32)
	{
		//�ж��������ͽ��д���
		if (cCmd == CODEC_IDBT_CLIENT_REQ)
		{
			//����ӿڰ�����
			if (!SMHndlClientReq((CLIENT_REQ_PACKET_ST*) pcBuf))
			{
				cCmd = CODEC_ERROR;
			}
		}
		else if(cCmd == COEDC_IDBT_CLIENT_RES)
		{
			//����ӿڰ�Ӧ��
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
		YKReadQue(g_pstSMMsgQ, &pvMsg, 0);			//������ȡ
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
			//��ʱ���¼�����
			case SM_TIMER_EVENT:
				switch (((SM_TIMER_EVENT_ST *) pvMsg)->uiTimerId)
				{
				case TIMER_ID_HEARTBEAT: //����������
					SMSendHeartBeat();
					break;
				case TIMER_ID_PACKET_RSP_TIMEOUT: //Ӧ�����Ӧ��ʱ
					//�ӿڰ���߸澯
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
			//���ڽ������ݴ���
			case SM_SERIAL_DATA_RCV:
				SMHndlCommData((SM_RCV_DATA_ST *) pvMsg);
				break;
			/************************************************************/
			//CC��Ϣ����
			case CCSM_CALLEE_PICK_UP:
				LOG_RUNLOG_DEBUG("callee pick up!");
				SMCalleePickUp();
				break;
			case CCSM_CALLEE_HANG_OFF:
				LOG_RUNLOG_DEBUG("callee hang off!");
				SMCalleeHandoff();
				break;
			case CCSM_CALLEE_ERR:
				LOG_RUNLOG_DEBUG("SM call err!");
				break;
			case CCSM_CALLEE_MUSIC_END:
				LOG_RUNLOG_DEBUG("SM callee music end!");
				SMCalleeMusicEnd();
				break;
			case CCSM_CALLEE_SEND_DTMF:
				if(((CCSM_CALLEE_SEND_DTMF_ST *)pvMsg)->iDtmfType == 0x23)
				{
					LOG_RUNLOG_DEBUG("SM callee send dtmf, open door!");
					SMOpenDoor();
					char *SMCurRoom = NULL;
					SMCurRoom = CCGetCurFsmRoom();
					SMSendHostRemark('0', SMCurRoom);
				}
				break;
			case CCSM_OPENDOOR:
			{
				LOG_RUNLOG_DEBUG("SM open door!");
				SMOpenDoor();
				char *SMCurRoom = NULL;
				SMCurRoom = CCGetCurFsmRoom();
				SMSendHostRemark('0', SMCurRoom);
			}
				break;
			case CCSM_VEDIO_MONITOR:
				LOG_RUNLOG_DEBUG("SM vedio monitor!");
				SMVedioMonitor();
				break;
			case CCSM_VEDIO_MONITOR_CANCEL:
				LOG_RUNLOG_DEBUG("SM vedio monitor cancel!");
				SMVedioMonitorCancel();
				break;
			/************************************************************/
			//�˳�
			case SM_HNDL_MSG_CANCEL:
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

int SMComRcvInit(void)
{
	LOG_RUNLOG_DEBUG("SM communication Initialize");

	g_iComFd = SMOpenComPort(1, 9600, 8, "1", 'N');         //����ͨѶ����
	if (-1 == g_iComFd)
	{
		LOG_RUNLOG_ERROR("SM com Open failed!");
		return FAILURE;
	}
	


	g_bRunCommRcvTaskFlg = TRUE;

	// ��ʼ���豸�ļ����ݽ��ջ����������ݽṹ
	memset(g_stRcvData.acBuf, 0x0, COMM_DATA_RCV_SIZE * sizeof(unsigned char));
	g_stRcvData.iLen = 0;
	g_stRcvData.pcHead = g_stRcvData.acBuf;
	g_stRcvData.ucCounter = 0;
	g_stRcvData.ucState = BEGIN_FLAG_NOT_FOUND;

	return SUCCESS;
}

int SMInit(void)
{

	LOG_RUNLOG_DEBUG("SM this is IMX27 version!\n");

	int iSta = SUCCESS;

	//DTMFͨѶ��ʼ��
	SMDtmfInit();

	//����SM��Ϣ����
	g_pstSMMsgQ  = YKCreateQue(SM_MSG_QUE_LEN);


	//���ڽ��ջ�������ʼ��
	iSta = SMComRcvInit();
	if(iSta != SUCCESS)
	{
		LOG_RUNLOG_ERROR("SM SMComRcvInit init failed!");
		return FAILURE;
	}

	//SM��ʱ����ʼ��
	iSta = SMTimerInit();
	if(iSta != SUCCESS)
	{
		LOG_RUNLOG_ERROR("SM sm timer init failed!");
		return FAILURE;
	}

	//�����������ݽ����߳�
	iSta = pthread_create(&g_rcvComData, NULL, SMRcvComDataTask, NULL);
	if(iSta != SUCCESS)
	{
		LOG_RUNLOG_ERROR("SM create rcv comm data task failed!");
		return FAILURE;
	}

	//����SM��Ϣ���ռ������߳�
	pthread_create(&g_HndlMsgTask, NULL, SMHndlMsgTask, NULL);
	if(iSta != SUCCESS)
	{
		LOG_RUNLOG_ERROR("SM create handle message task failed!");
		return FAILURE;
	}

	return SUCCESS;
}

void SMFini(void)
{
	//�˳��¼������߳�
	SM_EXIT_MSG_ST *pstMsg = YKNew0(SM_EXIT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		return ;
	}
	pstMsg->uiPrmvType = SM_HNDL_MSG_CANCEL;
	YKWriteQue(g_pstSMMsgQ , pstMsg,  0);

	//�˳��������ݽ����߳�
	g_bRunCommRcvTaskFlg = FALSE;

	YKJoinThread((void *)g_rcvComData);
	YKJoinThread((void *)g_HndlMsgTask);
	YKDestroyThread((void *)g_rcvComData);
	YKDestroyThread((void *)g_HndlMsgTask);

	//ɾ����ʱ��
	YKRemoveTimer(g_stHeartBeatTimer.pstTimer,g_stHeartBeatTimer.ulMagicNum);
	YKRemoveTimer(g_stClientRspTimeoutTimer.pstTimer,g_stClientRspTimeoutTimer.ulMagicNum);

	//ɾ����Ϣ����
	YKDeleteQue(g_pstSMMsgQ);

	//DTMF �������ݽ����߳��˳�
	SMDtmfFini();
}

int SMProcessCommand(const char *pcCmd)
{
	char acTmp[20] = {0};
	char *pcTmp = NULL;
	char *pcDevType = NULL;
	char *pcCmdText = NULL;

	if(pcCmd == NULL || strlen(pcCmd) == 0)
	{
		LOG_RUNLOG_DEBUG("SM SMC input cmd error!");
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

	LOG_RUNLOG_DEBUG("SM pcDevType: %s pcTmp: %s pcCmdText: %s",pcDevType, pcTmp, pcCmdText);

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
		LOG_RUNLOG_DEBUG("SM SMC input cmd event error!");
	}

	return SUCCESS;
}


void SMCtlSipLed(int iSta)
{

	if(iSta == 0)//on
	{
		LOG_RUNLOG_DEBUG("SM open sip led");
		IMX27GpioSetLed2(IMX27_GPIO_HIGH);
	}
	else if(iSta == 1)//off
	{
		LOG_RUNLOG_DEBUG("SM close sip led");
		IMX27GpioSetLed2(IMX27_GPIO_LOW);
	}
}

void SMCtlNetLed(int iSta)
{
	if(iSta == 0)//on
	{
		LOG_RUNLOG_DEBUG("SM open net led");
		IMX27GpioSetLed1(IMX27_GPIO_HIGH);
	}
	else if(iSta == 1)//off
	{
		LOG_RUNLOG_DEBUG("SM close net led");
		IMX27GpioSetLed1(IMX27_GPIO_LOW);
	}
}


#endif
