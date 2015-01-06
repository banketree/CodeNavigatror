/*
 * I6Interface.h
 *
 *  Created on: 2012-12-29
 *      Author: root
 */

#ifndef I6INTERFACE_H_
#define I6INTERFACE_H_

#if _TM_TYPE_ == _GYY_I6_
typedef enum
{
	TM_ALARM_IMS_REGISTER = 1,
	TM_ALARM_SIP_CALL,
	TM_ALARM_IF_BOARD,
	TM_ALARM_TRANSFER,
	TM_ALARM_UPDATERESULT,
	TM_ALARM_CALL_STATE,
	TM_ALARM_SOUTH_INTERFACE
}TM_ALARM_ID_EN;
#endif

typedef enum
{
	I6_AUTHCODE_REQ,
	I6_AUTHCODE_RES,
	I6_AUTHENTICATION_REQ,
	I6_AUTHENTICATION_RES,
	I6_CONFIG_REQ,
	I6_CONFIG_RES,
	I6_CONFIG_DOWNLOAD_REQ,
	I6_CONFIG_UPDATE_NOTIFY_REQ,
	I6_CONFIG_UPDATE_NOTIFY_RES,
	I6_PASSWORD_INFORM_REQ,
	I6_PASSWORD_INFORM_RES,
	I6_CARD_INFROM_REQ,
	I6_CARD_INFROM_RES,
	I6_DOWNLOAD_CARD_REQ,
	I6_DOWNLOAD_CARD_RES,
	I6_CARD_SWIP_REQ,
	I6_CARD_SWIP_RES,
	I6_SOFT_INFORM_REQ,
	I6_SOFT_INFORM_RES,
	I6_SOFT_UPDATE_NOTIFY_REQ,
	I6_SOFT_UPDATE_NOTIFY_RES,
	I6_INFORMATION_UPDATE_REQ,
	I6_INFORMATION_UPDATE_RES,
	I6_INFORMATION_UPDATE_NOTIFY_REQ,
	I6_INFORMATION_UPDATE_NOTIFY_RES,
	I6_ADVERTISE_UPDATE_REQ,
	I6_ADVERTISE_UPDATE_RES,
	I6_ADVERTISE_UPDATE_NOTIFY_REQ,
	I6_ADVERTISE_UPDATE_NOTIFY_RES,
	I6_FILE_UPLOAD_REQ,
	I6_FILE_UPLOAD_RES,
	I6_FILE_UPLOAD_NOTIFY_REQ,
	I6_FILE_UPLOAD_NOTIFY_RES,
	I6_HEART_REQ,
	I6_HEART_RES,
	I6_ALARM_UPDATE_REQ,
	I6_ALARM_UPDATE_RES,
	I6_REBOOT,
	I6_ERROR,
}I6_CMD_EN;

typedef struct
{
	char acID[16];
	int iState;
}I6_ADV_ST,I6_INF_ST;

typedef struct
{
	int iState;	// 0
	int iSort;
	int iThread;
}I6_STATE_ST;

typedef struct
{
	char acUserName[33];
	char acPassword[33];
}I6_AUTHENTICATION_ST;

typedef struct
{
	int iDownloadStatus;
	int iConfigVersion;
	char acDedicatedNumber[32];
}I6_CONFIG_ST;

typedef struct
{
	int iDownloadStatus;
	char acDedicatedNumber[32];
}I6_PASSWORD_ST;

typedef struct
{
	int iDownloadStatus;
	int iConfigVersion;
	char acDedicatedNumber[32];
}I6_CARD_ST;

typedef struct
{
	int iDownloadStatus;
	int iConfigVersion;
	char acSoftwareNumber[32];
//	char acDedicatedNumber[32];
}I6_SOFT_ST;

typedef struct
{
	char acCardId[64];
	char acCardNo[32];
	char acStateCode[4];
	unsigned long lCardTime;
}I6_CARD_LOG_ST;

typedef struct
{
	char acName[64];
	char acCode[32];
	char acCornet[16];
	char acTpye[4];
}I6_CONFIG_PHONE_BOOK_ST;

typedef struct
{
	char acDeviceID[32];
	char acSerialNum[64];
	char acDevicePassword[32];
	char acSipAccont[128];
	char acDeviceType[8];
	int iSoftVersion;
	int iConfigVersion;
	int iKeyVersion;
	int iPasswordVersion;
}I6_DEVICE_ST;

typedef struct
{
	char acSipAccont[128];
	char acSipPwd[256];
	char acDomain[64];
	char acProxy[64];
	int iPort;
}I6_SIP_INFO_ST;

typedef enum
{
	I6_TERMINAL_HOME = 1,
	I6_TERMINAL_UNIT_DOOR,
	I6_TERMINAL_VILLA_DOOR,
	I6_TERMINAL_AREA_GATE,
	I6_TERMINAL_AREA_WALL,
	I6_TERMINAL_MANAGE,
}I6_TERMINAL_EN;

#define MSG_TYPE "msgType"
#define SAFE_CODE "safeCode"
#define FAULT_CODE "faultCode"
#define UPDRADE_STATUS "upgradeStatus"
#define DOWNLOAD_URL "downloadUrl"

#define MSG_TPYE_AUTHCODE_RES "AuthcodeResponse"
#define MSG_TPYE_AUTHENTICATION_RES "AuthenticationResponse"
#define MSG_TPYE_CONFIG_RES "ConfigInformResponse"
#define MSG_TPYE_CONFIG_UPDATE_NOTIFY_RES "ConfigUpdateNotifyResponse"
#define MSG_TPYE_PASSWORD_INFORM_RES "PasswordInformResponse"
#define MSG_TPYE_CARD_INFROM_RES "CardInformResponse"
#define MSG_TPYE_DOWNLOAD_CARD_RES "DownloadCardResponse"
#define MSG_TPYE_CARD_SWIP_RES "CardSwipResponse"
#define MSG_TPYE_SOFT_INFORM_RES "SoftwareInformResponse"
#define MSG_TPYE_SOFT_UPDATE_NOTIFY_RES "SoftwareUpdateNotifyResponse"
#define MSG_TPYE_ALARM_UPDATE_RES "AlarmUpdateResponse"
#define MSG_TPYE_INFORMATION_UPDATE_RES "InformationUpdateReponse"
#define MSG_TPYE_ADVERTISE_UPDATE_RES "AdvertiseUpdateReponse"
#define MSG_TPYE_FILE_UPLOAD_RES "FileUploadResponse"

#define CONFIG_VERSION "VERSION"
#define CONFIG_PHONE_BOOK "PHONE_BOOK"
#define CONFIF_ENTRY_START	"<"
#define CONFIF_ENTRY_END	"/"
#define CONFIG_ENTRY_NAME	"Name"
#define CONFIG_ENTRY_CODE	"Code"
#define CONFIG_ENTRY_CORNET	"Cornet"
#define CONFIG_ENTRY_TYPE	"Type"

#define TERMINAL_TYPE		50001

void* I6Boot();

#endif /* I6INTERFACE_H_ */
