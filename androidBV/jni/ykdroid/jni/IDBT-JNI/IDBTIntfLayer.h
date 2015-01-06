/*
 * IDBTIntfLayer.h
 *
 *  Created on: 2013-1-4
 *      Author: root
 */
#include "YKApi.h"
#include "YKTimer.h"
#include "YKMsgQue.h"
#include "LPIntfLayer.h"

#ifndef IDBTINTFLAYER_H_
#define IDBTINTFLAYER_H_

#define AB_CMDLINE_BUFF_SIZE 	(256)
#define AB_QUE_LEN             	(256)      	//AB消息队列最长长度
#define AB_QUE_WRITE_TIMEOUT   	(250)      	//写AB消息队列满时最长等待时间(毫秒)
#define AB_QUE_READ_TIMEOUT    	(250)      	//读AB消息队列空时最长等待时间(毫秒)

typedef void   (*IDBTJNI_REG_STATUS_CHANGED_CB)(REG_STATUS_EN enRegStatus);

typedef enum
{
	JNI_DEV_IDLE = 0,
	JNI_DEV_CALL_AUDIO_INCOMING_RECEIVED = 1,
	JNI_DEV_CALL_VIDEO_INCOMING_RECEIVED = 2,
	JNI_DEV_OUTGOING_INIT = 3,
	JNI_DEV_OUTGOING_PROGRESS = 4,
	JNI_DEV_OUTGOING_RINGING = 5,
//	JNI_DEV_CALL_HANGOFF = 6,
	JNI_DEV_CALL_ERROR = 7,
	JNI_DEV_CALL_OUTGOING_EARLYMEDIA = 8,
	JNI_DEV_CALL_CONNECTED = 9,
	JNI_DEV_CALL_END = 10,
	JNI_DEV_SIP_REG_STATUS_OFFLINE = 11,
	JNI_DEV_SIP_REG_STATUS_ONLINE = 12,
	JNI_DEV_TM_LINK_STATUS_OFFLINE = 13,
	JNI_DEV_TM_LINK_STATUS_ONLINE = 14,
	JNI_DEV_NETWORK_STATUS_OFFLINE = 15,
	JNI_DEV_NETWORK_STATUS_ONLINE = 16,
	JNI_DEV_RFIDCARD_VALIDATE_STATUS_INVALID = 17,
	JNI_DEV_RFIDCARD_VALIDATA_STATUS_VALID = 18,
	JNI_DEV_OPENDOOR = 19,
	JNI_DEV_ADVERT_UPDATE = 20,
	CALL_ERROR_ROOM_INVALID = 21,
	CALL_ERROR_ROOM_VALID = 22,

	JNI_DEV_REBOOT= 23,
	CONFIG_UPDATE_DEV_REBOOT= 24,
	JNI_ADV_DOWNLOAD = 25,
	JNI_ANN_DOWNLOAD = 26,
	JNI_DEV_SIPAB_STOP_RING = 27
}JNI_DEV_STATUS_EN;


extern YK_MSG_QUE_ST  *g_pstABMsgQ;
extern YKHandle        g_IDBTIntfLayerThreadId;

extern YK_MSG_QUE_ST  *g_pstJNIMsgQ;

extern int IDBTABInit(void);
extern int SendHouseCode(const char *houseCode);
extern int CheckImsCallEnable(void);
extern int GetImsCallTimeout(void);
extern void HangupCall(void);
extern void IDBTJniSetRegStatusChangedCB(IDBTJNI_REG_STATUS_CHANGED_CB pstCB);
extern int ValidatePassword(const char *password);

#endif /* IDBTINTFLAYER_H_ */
