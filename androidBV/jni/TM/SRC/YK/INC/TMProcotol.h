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
#include <IDBT.h>

#if _TM_TYPE_ == _YK069_

#ifndef __TM_PROCOTOL_H__
#define __TM_PROCOTOL_H__

typedef enum {
    TM_COMMAND_OPTION  = 0x0,
    TM_COMMAND_REGISTER= 0x1,
    TM_COMMAND_QUIT    = 0x2,
    TM_COMMAND_HEART   = 0x3,

    TM_COMMAND_SET_DEVICE_INFO      = 0x10,
    TM_COMMAND_SET_COMMUNICATE_INFO = 0x11,
    TM_COMMAND_SET_WORK_INFO        = 0x12,
    TM_COMMAND_SET_ALARM_ENABLE_INFO = 0x13,
    TM_COMMAND_SET_SYSTEM_REBOOT    = 0x14,
    TM_COMMAND_SET_USER_SERVICES_INFO= 0x15,
    TM_COMMAND_SET_DOWNLOAD    = 0x16,
    TM_COMMAND_SET_UPLOAD      = 0x17,
    TM_COMMAND_SET_TRANSFER    = 0x18,
    TM_COMMAND_SET_WISDOM_KEY   = 0x19,

    TM_COMMAND_REQ_DEVICE_INFO      = 0x20,
    TM_COMMAND_REQ_COMMUNICATE_INFO = 0x21,
    TM_COMMAND_REQ_WORK_INFO        = 0x22,
    TM_COMMAND_REQ_ALARM_ENABLE_INFO = 0x23,
    TM_COMMAND_REQ_ALARM_ENABLE_STATE= 0x24,
    TM_COMMAND_REQ_USER_SERVICES_INFO= 0x25,


    TM_COMMAND_SET_CALL_OUTGOING=0x31,


    TM_COMMAND_RES_ALARM = 0x42,
    TM_COMMAND_UNKNOW = 0xFE,
}TM_COMMAND_TYPE_EN;

#define TM_PROCOTOL_START_FLAG '^'
#define TM_PROCOTOL_END_FLAG   '~'

#define TM_SIZE_PROCOTOL_START 1
#define TM_SIZE_PROCOTOL_LENGTH 4
#define TM_SIZE_PROCOTOL_DEVICE_ID 16
#define TM_SIZE_PROCOTOL_COMMAND_TYPE 2
#define TM_SIZE_PROCOTOL_CRC 4
#define TM_SIZE_PROCOTOL_END 1

#define TM_SIZE_IP_ADDR 15
#define TM_SIZE_IP_PORT 5
#define TM_SIZE_FIRMWARE_VERSION 11
#define TM_SIZE_PHONE_NUMBER 20

#define TM_PHONE_INFO_MAX_COUNT     2
#define TM_CARD_INFO_MAX_COUNT      5
#define TM_NOTDISTURBTIME_MAX_COUNT 2

typedef char Hex;
typedef enum{TM_CONTEXT_CRC=0, TM_CONTEXT_PASS, TM_CONTEXT_ERROR}TM_CONTEXT_STATE_EN;

#define SIZE_16 16
#define SIZE_32 32
#define SIZE_64 64
#define SIZE_128 128
#define SIZE_256 256
#define SIZE_512 512
#define SIZE_1024 1024

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

typedef struct __ProcotolPacket{
    char acBegin[TM_SIZE_PROCOTOL_START];
    char acLength[TM_SIZE_PROCOTOL_LENGTH];
    char acDeviceID[TM_SIZE_PROCOTOL_DEVICE_ID];
    char acCommandType[TM_SIZE_PROCOTOL_COMMAND_TYPE];
    //char Context[4];
    char acCrc[TM_SIZE_PROCOTOL_CRC];
    char acEnd[TM_SIZE_PROCOTOL_END];
}__attribute__((packed)) TM_PROCOTOL_PACKET_ST;

typedef struct __DeviceInfo{
    char acManufacturer[SIZE_32];
    char acOui[6];
    char acProductClass[SIZE_64];
    char acSerialNumber[SIZE_64];
    char acDeviceID[SIZE_16];
}__attribute__((packed)) TM_SET_DEVICE_INFO_ST;

typedef struct __CommunicateInfo{
    char acWanUserName[SIZE_32];
    char acWanPassword[SIZE_32];
    char acSipUsername[SIZE_32];
    char acSipPassword[SIZE_32];
    char acTr069Username[SIZE_32];
    char acTr069Password[SIZE_32];
    char acImsDomain[SIZE_64];
    char acImsProxyIP[TM_SIZE_IP_ADDR];
    char acImsPorxyPort[TM_SIZE_IP_PORT];
    char acPlatformIP[TM_SIZE_IP_ADDR];
    char acPlatformPort[TM_SIZE_IP_PORT];
    char acFtpIp[TM_SIZE_IP_ADDR];
    char acFtpPort[TM_SIZE_IP_PORT];
    char acFtpUsername[SIZE_32];
    char acFtpPassword[SIZE_32];
}__attribute__((packed)) TM_SET_COMMUNICATE_INFO_ST, TM_REQ_COMMUNICATE_INFO_ST;

typedef struct __WorkInfo{
    Hex  acSipCallTimeout[2];
    char cImsCallEnable[1];
    char cIcOpenDoorEnable[1];
    char cWisdomOpenDoorEnable[1];
    char cMonitorEnable[1];
}__attribute__((packed)) TM_SET_WORK_INFO_ST, TM_REQ_WORK_INFO_ST;

typedef struct __AlarmInfo{
    char cImsRegisterFail[1];
    char cSipCallFail[1];
    char cIfBoardComFail[1];
    char cFileTransferFail[1];
}__attribute__((packed)) TM_SET_ALARM_ENABLE_ST, TM_REQ_ALARM_ENABLE_ST, TM_REQ_ALARM_STATUS_ST, TM_SET_ALARM_STATUS_ST;

typedef struct __SysReboot{
    char cReboot[1];
}__attribute__((packed)) TM_SET_SYS_REBOOT_ST;

typedef struct __Download{
    Hex cType[1];
    char acRemoteUrl[SIZE_64];
}__attribute__((packed)) TM_SET_DOWNLOAD_ST;

typedef struct __Upload{
    Hex cType[1];
    char acRemoteUrl[SIZE_64];
}__attribute__((packed)) TM_SET_UPLOAD_ST;

typedef struct __Transfer{
    char cType[1];
    char acRemoteUrl[SIZE_64];
    char acLocalUrl[SIZE_64];
}__attribute__((packed)) TM_SET_TRANSFER_ST;

typedef struct __ReqDeviceInfo{
    char acManufacturer[SIZE_32];
    char acOui[6];
    char acProductClass[SIZE_64];
    char acSerialNumber[SIZE_64];
    char acFirmwareVersion[TM_SIZE_FIRMWARE_VERSION];    
}__attribute__((packed)) TM_REQ_DEVICE_INFO_ST;

typedef struct __CardInfo{
    char acSerinalNumber[10];
}__attribute__((packed)) TM_CARD_INFO_ST;

typedef struct __Disturb{
    char acTime[8];
}__attribute__((packed)) TM_DISTURB_ST;

typedef struct __PhoneInfo{
    char cVoipEnable[1];
    char acCsPhoneNumber[TM_SIZE_PHONE_NUMBER];
    char acSipPhoneNumber[TM_SIZE_PHONE_NUMBER];
    Hex  cNotDisturbCount[1];
    TM_DISTURB_ST stNotDisturbTime[TM_NOTDISTURBTIME_MAX_COUNT];
}__attribute__((packed)) TM_PHONE_INFO_ST;

typedef struct __HouseInfo{
    char acHouseCode[5];
    Hex  acRingTime[2];
    char cInCommingcCallNotice[1];
    Hex  acSIPCallDelay[2];
    Hex  cPhoneCount[1];
    TM_PHONE_INFO_ST stPhoneInfo[TM_PHONE_INFO_MAX_COUNT];
    Hex  cCardCount[1];
    TM_CARD_INFO_ST stCard[TM_CARD_INFO_MAX_COUNT];
}__attribute__((packed)) TM_HOUSE_INFO_ST;

typedef struct __UserServiceInfo{
    Hex           acUserCount[4];
    TM_HOUSE_INFO_ST*  stHouseInfo;
}__attribute__((packed)) TM_USER_SERVICE_INFO_ST;

typedef struct __OutgoingCallInfo{
	char acPhoneNumber[TM_SIZE_PHONE_NUMBER];
	char cVoipEnable[1];
}__attribute__((packed)) TM_OUT_GOING_CALL_INFO;

#endif//!__TM_PROCOTOL_H__

#endif

/**@}*///TManage
