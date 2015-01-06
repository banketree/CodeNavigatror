/**
 *@addtogroup TManage
 *@{
 */

/**
 *@file TMInterface.h
 *@brief Out going other module's API 
 *
 *All APIs which needed by other module's go here
 *@author Amos
 *@version 1.0
 *@date 2012-04-27
 */

#ifndef __TM_INTERFACE_H__
#define __TM_INTERFACE_H__

#include <IDBT.h>

#if _TM_TYPE_ == _YK069_
#include "../SRC/YK/INC/TMProcotol.h"
#elif _TM_TYPE_ == _TR069_
#include "../SRC/TR/INC/TMProcotol.h"
#elif _TM_TYPE_ == _GYY_I6_
#include "GI6Interface.h"
#endif

#ifdef __cplusplus
extern "C"{
#endif



#define TM_QUE_WRITE_TIMEOUT   (250)      	//写消息队列满时最长等待时间(毫秒)

typedef enum
{
	LINK_STATUS_OFFLINE  = 0,                //tr状态离线
	LINK_STATUS_ONLINE,                      //tr状态在线
}LINK_STATUS_EN;
/**
 *@name For Main Module's
 *@{
 */

void TMSetLinkStatus(LINK_STATUS_EN enStatus);
/**@brief Start to init terminate manage module. */
int TMStartTerminateManage(const char* pcVersion, const char* pcConfig);
/**@brief Stop terminate manage module. */
void TMStopTerminateMange(void);

/**@}*///For Main Module's

/**
 *@name For CC Module
 *@{
 */
/**
 *@brief Get handle of house info by special house code.
 *@param[in] pcHouseCode The code of house.
 *@return The handle of house info which used by other APIs[eg. @see TMCCGetRingTime].
 */
#if _TM_TYPE_ == _YK069_
YKHandle TMCCGetHouseInfoByHouseCode(const char* pcHouseCode);
#elif _TM_TYPE_ == _TR069_
YKHandle TMCCGetHouseInfoByHouseCode(const char* pcHouseCode);
//YKHandle TMCCGetHouseInfoByHouseCode(const char* pcHouseCode ,const char *pcUnitCode , const char* pcBuildingCode);
#elif _TM_TYPE_ == _GYY_I6_
//开始呼叫时候根据房号查找数据并创建数据 返回 FALSE 0 为未找到  TRUE 1 已找到
YKHandle TMCCGetHouseInfoByHouseCode(const char* pcHouseCode);
#endif
/**
 *@brief Free memory which malloc by TMCCGetHouseInfoByHouseCode
 *@param[in] house The handle of house.
 *@return void.
 */
void TMCCDestroyHouseInfo(YKHandle hHouse);
/**
 *@brief Get ring time by house handle.
 *@param[in] house The handle of house. @see TMCCGetHouseInfoByHouseCode
 *@return Ring time.
 */
int TMCCGetRingTime(YKHandle hHouse);

/**
 *@brief Judge is need to play early media.
 *@param[in] house The handle of house.
 *@return TRUE indicates need play early media else not.
 */
BOOL TMCCIsInCommingCallNotice(YKHandle hHouse);

/**
 *@brief Get Sip call delay time.
 *@param[in] house The handle of house.
 *@return Delay time.
 */
int TMCCGetSipCallDelay(YKHandle hHouse);


/**
 *@brief Get the number of phone.
 *@param[in] house The handle of house.
 *@return The count of phone.
 */
int TMCCGetPhoneCount(YKHandle hHouse);

/**
 *@brief Get special phone info.
 *@param[in] house The handle of house.
 *@param[in] iIndex From 0.
 *@return The handle of phone info. NULL indicate failed else indicate success.
 */
YKHandle TMCCGetPhoneInfoByIndex(YKHandle hHouse, int iIndex);

/**
 *@brief Judge is support VOIP.
 *@param[in] phone The handle of phone.
 *@return TRUE if is support VOIP.
 */
BOOL TMCCIsVoipEnable(YKHandle hPhone);

/**
 *@brief Get IMS phone number.
 *@param[in] phone The handle of phone.
 *@return IMS phone number.
 */
typedef struct
{
	unsigned char acPhoneNum[64];
	unsigned char ucCallType;
	unsigned char ucPreView;
	int iSerNum;
}TM_I6NUM_INFO_ST;

#if _TM_TYPE_ == _GYY_I6_
//获取号码: pcPhoneNum 传入参数进行赋值 ，pcPhoneNum［0］号码为0x0时 为无号码
//返回值 sernum值 为0 时候为无CALL信息 返回 Code
int TMCCGetNumInfo(TM_I6NUM_INFO_ST **now_info);
#endif

const char* TMCCGetIMSPhoneNumber(const char* pcHouseCode);

/**
 *@brief Get CS phone number.
 *@param[in] phone The handle of phone.
 *@return CS phone number.
 */
const char* TMCCGetCsPhoneNumber(YKHandle hPhone);

/**
 *@brief Get SIP phone number.
 *@param[in] phone The handle of phone.
 *@return SIP phone number.
 */
const char* TMCCGetSipPhoneNumber(YKHandle hPhone);

/**
 *@brief Judge if is in not disturb time.
 *@param[in] phone The handle of phone.
 *@param[in] currentTime Current time.
 *@return TRUE indicate is in not disturb time else indicate not.
 */
BOOL TMCCIsNotDisturbTime(YKHandle hPhone);

/**
 *@brief Judge is valid phone number.
 *@param[in] pcPhoneNum The phone number.
 *@param[out] houseCode The house code return by the special phone number
 *@return TRUE indicate is valid else is not.
 */
BOOL TMCCIsValidPhoneNum(const char* pcPhoneNum, char* pcHouseCode);

/**
 *@brief Get to call timeout value.
 *@param[in] void.
 *@return timeout value.
 */
int  TMCCGetIMSCallTimeOut(void);

/**
 *@brief Get to call timeout value.
 *@param[in] void.
 *@return timeout value.
 */
#if _TM_TYPE_ != _GYY_I6_
BOOL TMCCIsAlarmEnabled(TM_ALARM_ID_EN enModule);
#endif
/**
 *@brief To judge whether the IMS-enabled
 *@param[in] void
 *@return TRUE indicates enable else not.
 */
BOOL TMCCIsIMSCallEnabled(void);

/**
 *@brief To judge whether the VOIP Monitor-enabled.
 *@param[in] void.
 *@return TRUE indicates enable else not.
 */
BOOL  TMCCIsKeyOpenDoorEnabled(void);

/**
 *@brief To judge whether the VOIP Monitor-enabled
 *@param[in] void
 *@return TRUE indicates enable else not.
 */
BOOL TMCCIsVoipMonitorEnabled(void);

#if _TM_TYPE_ == _TR069_
/**
 *@brief Judge is valid Demo number.
 *@param[in] pcPhoneNum The Demo number.
 *@param[out] houseCode The house code return by the special phone number
 *@return TRUE indicate is valid else is not.
 */
BOOL TMCCIsValidDemoNum(const char* pcPhoneNum, char* pcHouseCode);
BOOL TMCCIsValidDemoPhoneNum(const char* pcPhoneNum, char* pcHouseCode);
#endif

/**@}*///For CC Module

/**
 *@name For NM module.
 *@{
 */
/**
 *@brief Get WAN info.
 *@param[in] pcUserName For received pcUserName.
 *@param[in] pcPassWord For received pcPassWord.
 *@return void.
 */
void TMNMGetWANInfo(char* pcUserName, char* pcPassWord);

/**@}*///For NM module.

/**
 *@brief Get IMS pcDomain.
 *@param[in] pcDomain For received data.
 *@return void.
 */
void TMGetIMSInfo(char* pcUserName, char* pcPassWord, char* pcProxy, int* piPort, char* pcDomain);

/**
 *@brief Send alarm.
 *@param[in] module Alarm module @see TM_ALARM_ID_EN
 *@param[in] state  '0' alarm restore '1' alarm happened.
 *@return void.
 */
#if _TM_TYPE_ == _YK069_
void TMUpdateAlarmState(TM_ALARM_ID_EN enModule, char cState);
#else
void TMUpdateAlarmState(TM_ALARM_ID_EN enModule, const char* cState);
#endif

/**
 *@name Query device id.
 *@param[in] id For received id.
 *@return void.
 */
void TMQueryDeviceID(char* pcID);

/**
 *@brief Query ftp info.
 *@param[in] host For received id. 
 *@param[in] host For received piPort. 
 *@param[in] host For received pcUserName. 
 *@param[in] host For received pcPassWord. 
 */
void TMQueryFtpInfo(char* pcHost, int*piPort, char* pcUserName, char* pcPassWord);

/**
 *@brief Query application version
 *@param[in] version For received version.
 *@return void.
 */
void TMQueryAppVersion(char* pcVersion);

/**
 *@name For XT module
 *@{
 */
#if _TM_TYPE_ == _TR069_
/**
 *@brief Judge is valid DoorPassword.
 *@param[in] Password.
 *@return TRUE indicate is valid else is not.
 */
BOOL TMXTIsValidDoorPassword(char* Password);
#endif

/**
 *@name For NM module
 *@{
 */
#if _TM_TYPE_ == _TR069_
/**
 *@brief get the SN
 *@param[in] pcSN.
 *@return TRUE indicate is valid else is not.
 */
BOOL TMNMGetSN(char* pcSN);

void TMSetICIDOpenDoor(const char* pcSerinalNumber);
#endif

/**
 *@name For SM module
 *@{
 */
/**
 *@brief Judge is valid serial number.
 *@param[in] pcSerinalNumber The serial number.
 *@return TRUE indicate is valid else is not.
 */
BOOL TMSMIsValidICIDCard(const char* pcSerinalNumber);

#if _TM_TYPE_ == _TR069_
/**
 *@brief Judge is valid PropertyCard number.
 *@param[in] pcSerinalNumber The PropertyCard number.
 *@return TRUE indicate is valid else is not.
 */
BOOL TMSMIsValidPropertyCard(const char* pcSerinalNumber);
#endif

/**
 *@brief To judge whether the IC open door enabled.
 *@param[in] void.
 *@return TRUE indicates enable else not.
 */
BOOL  TMSMIsICOpenDoorEnabled(void);

/**
 *@brief To judge whether the IC open door enabled.
 *@param[in] void.
 *@return TRUE indicates enable else not.
 */
BOOL  TMSMIsPasswordOpenDoorEnabled(void);

/**@}*///For SM module

/**
 *@name For Debug APIs
 *@{
 */

void TMCreateModel();

void TMRegister2TMP();
void TMUnRegisterFromTMP();
void TMSendWorkInfo();
void TMSendDeviceInfo();
void TMSendAlamEnable(); 
void TMSendCommunicateInfo();
void TMSendUserServices();
void TMSendOption();
void TMSendTransfer(const char* pcLocal, const char*pcRemote, char cType);

/**@}*///For Debug APIs

#define BUF_SIZE 100
char *TMGetDevInfo(void);
//char *TMGetVersionInfo(void);
int TMGetVersionInfo(char *pStr);
char *TMGetPlatformInfo(void);
char *TMGetSIPInfo(void);
int TMSetDevNum(const char *str);
int TMSetPlatformDomainName(const char *str);
int TMSetI5PlatformDomainName(const char *str);
int TMSetPlatformIP(const char *str);
int TMSetPlatformPort(const char *str);
int TMSetPlatformUserName(const char *str);
int TMSetPlatformPassword(const char *str);
int TMSetPPPOEUername(const char *str);
int TMSetPPPOEPassword(const char *str);
int TMSetManagementPassword(const char *str);
//char *TMGetManagementPassword(void);
int TMGetManagementPassword(const char *str);
int TMSetOpendoorPassword(const char *str);
void TMUseLocalSipInfo();


#ifdef __cplusplus
};
#endif
#endif//!__TM_INTERFACE_H__

/**@}*///TManage
