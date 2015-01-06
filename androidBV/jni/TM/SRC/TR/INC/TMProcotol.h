 /**
 *@addtogroup TManage
 *@{
 */

/**
 *@file TMProcotol.h
 *@brief TM Procotol defines.
 *@author Amos
 *@version 1.0
 *@date 2012-04-27
 */
#ifndef XNMP_ENV
#include <IDBT.h>
#endif

#if _TM_TYPE_ == _TR069_

#ifndef __TM_PROCOTOL_H__
#define __TM_PROCOTOL_H__

#define SIZE_16 16
#define SIZE_32 32
#define SIZE_64 64
#define SIZE_128 128
#define SIZE_256 256
#define SIZE_512 512
#define SIZE_1024 1024

#define TM_SIZE_IP_ADDR 15
#define TM_SIZE_IP_PORT 5
#define TM_SIZE_FIRMWARE_VERSION 11
#define TM_SIZE_PHONE_NUMBER 64

#define TM_PHONE_INFO_MAX_COUNT     3
#define TM_CARD_INFO_MAX_COUNT      5

typedef enum
{
	TM_ALARM_IMS_REGISTER,
	TM_ALARM_SIP_CALL,
	TM_ALARM_IF_BOARD,
	TM_ALARM_TRANSFER,
	TM_ALARM_UPDATERESULT,
	TM_ALARM_SOUTH_INTERFACE,
	TM_ALARM_CALL_STATE
}TM_ALARM_ID_EN;

typedef struct __WorkInfo{
    char acSipCallTimeout[2];
    char cImsCallEnable[1];
    char cIcOpenDoorEnable[1];
    char cWisdomOpenDoorEnable[1];
    char cMonitorEnable[1];
}TM_SET_WORK_INFO_ST, TM_REQ_WORK_INFO_ST;

typedef struct __AlarmInfo{
    char cImsRegisterFail[1];
    char cSipCallFail[1];
    char cIfBoardComFail[1];
    char cFileTransferFail[1];
    char cUpdateResult[1];
}TM_SET_ALARM_ENABLE_ST, TM_REQ_ALARM_ENABLE_ST, TM_REQ_ALARM_STATUS_ST, TM_SET_ALARM_STATUS_ST;

typedef struct __DeviceInfo{
    char acManufacturer[SIZE_64];
    char acOui[6];
    char acProductClass[SIZE_64];
    char acSerialNumber[SIZE_64];
    char acDeviceID[SIZE_64];
}TM_SET_DEVICE_INFO_ST;

typedef struct __CommunicateInfo{
    char acWanUserName[SIZE_64];
    char acWanPassword[SIZE_64];
    char acSipUsername[SIZE_64];
    char acSipPassword[SIZE_64];
    char acTr069Username[SIZE_64];
    char acTr069Password[SIZE_64];
    char acImsDomain[SIZE_64];
    char acImsProxyIP[TM_SIZE_IP_ADDR];
    char acImsPorxyPort[TM_SIZE_IP_PORT];
    char acPlatformIP[TM_SIZE_IP_ADDR];
    char acPlatformPort[TM_SIZE_IP_PORT];
    char acPlatformDomain[SIZE_64];
    char acFtpIp[TM_SIZE_IP_ADDR];
    char acFtpPort[TM_SIZE_IP_PORT];
    char acFtpUsername[SIZE_64];
    char acFtpPassword[SIZE_64];
}TM_SET_COMMUNICATE_INFO_ST, TM_REQ_COMMUNICATE_INFO_ST;

typedef struct __CardInfo
{
    char acSerinalNumber[100];
}TM_CARD_INFO_ST;

typedef struct __PhoneInfo{
    char cVoipEnable[1];
    char acCsPhoneNumber[TM_SIZE_PHONE_NUMBER];
    char acSipPhoneNumber[TM_SIZE_PHONE_NUMBER];
    char acNotDisturbTime[60];
}TM_PHONE_INFO_ST;

typedef struct __HouseInfo{
	char acBuildingNum[6];
	char acUnitNum[6];
    char acHouseCode[20];
    char acRingTime[2];
    char cInCommingcCallNotice[1];
    char acSIPCallDelay[2];
    int  iPhoneCount;
    TM_PHONE_INFO_ST stPhoneInfo[TM_PHONE_INFO_MAX_COUNT];
    int  iCardCount;
    TM_CARD_INFO_ST  stCard[TM_CARD_INFO_MAX_COUNT];
}TM_HOUSE_INFO_ST;

typedef struct __UserServiceInfo
{
    int 	           	iUserCount;
    TM_HOUSE_INFO_ST*  	stHouseInfo;
}TM_USER_SERVICE_INFO_ST;

#endif//!__TM_PROCOTOL_H__

#endif

/**@}*///TManage
