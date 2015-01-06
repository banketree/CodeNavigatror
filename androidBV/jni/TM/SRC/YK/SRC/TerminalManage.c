
#include <IDBT.h>

#if _TM_TYPE_ == _YK069_

#include "../INC/TMInterface.h"
#include "../INC/TerminalManage.h"
#include "../INC/TMProcotol.h"
#include "../INC/TMFilters.h"
#include <YKApi.h>
#include <YKSystem.h>
#include <FPInterface.h>
#include <LOGIntfLayer.h>

static TM_CORE_VTABLE_ST g_stTMCoreVtable;
static YKBool g_bTMRunning=FALSE;
static YKHandle g_hTMThread=NULL;
void ExitApp();

BOOL TMSendCommand(TM_COMMAND_TYPE_EN enCMD);
void TMTrimNLeftSpace(char* pcDest, const char* pcValue, int iSize);
BOOL TMCompareNoCase(const char* pcSrc, const char* pcDest, int iLen);
int TMHexStringToDecimal(const char* pcHex);

void TMSystemLogOut(TM_LOG_LEVEL_EN enLevel, const char* pcMessage, ...)
{
    char acTmp[1024*3]={0x0};
    switch(enLevel)
	{
	case TM_WARNING:
		strcpy(acTmp, Text([TM_WARNING] -  ));
		break;
	case TM_MESSAGE:
		strcpy(acTmp, Text([TM_MESSAGE] -  ));
		break;
	case TM_DEBUG:
		strcpy(acTmp, Text([TM_DEBUG] -  ));
		break;
	case TM_ERROR:
		strcpy(acTmp, Text([TM_ERROR] -  ));
		break;
    }
#if _YK_OS_==_YK_OS_LINUX_
    va_list va_prt;
    va_start(va_prt,pcMessage);
    vsprintf(acTmp+strlen(acTmp),pcMessage,va_prt);
    va_end(va_prt);
    strcat(acTmp, "\n");
    printf(acTmp);
    if(TM_DEBUG==enLevel)
    {
    	//LOG_RUNLOG_WARN(acTmp);
    }
#endif
}

#if _YK_OS_==_YK_OS_LINUX_
static void ConnectFunc(TM_CONNECT_STATE_EN enState,  const char* pcMessage)
{
	static TM_CONNECT_STATE_EN enStateBackup = TM_DISCONNECT_BY_NO_OPTION;
	if(enStateBackup == enState)
	{
		return;
	}
	enStateBackup = enState;
    switch(enState)
    {
    case TM_START_TO_CONNECT_SERVER:
    case TM_CONNECT_SERVER_SUCCESS:
    case TM_CONNECT_SERVER_FAILED:
    case TM_RECONNECT_SERVER_START:
    case TM_DISCONNECT_BY_LOCAL:
    case TM_DISCONNECT_BY_REMOTE:
    case TM_DISCONNECT_BY_NO_OPTION:
        TMSystemLogOut(TM_MESSAGE, pcMessage);
        //LOG_RUNLOG_WARN(pcMessage);
		break;
    default:
        break;
    }
}

static void ConfigFunc(TM_CONFIG_STATE_EN enState, const char* pcMessage)
{
	 switch(enState)
	{
	case TM_CONFIG_CREAT:
	case TM_CONFIG_READ:
	case TM_CONFIG_CLOSE:
		break;
	case TM_CONFIG_WRITE:
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM,LOG_EVT_RELOAD_CONFIG, 0, "");
		break;
	default:
		break;
	}
}

static void RegisterFunc(TM_REGISTER_STATE_EN enState, const char* pcMessage)
{
	static TM_REGISTER_STATE_EN enStateBackup = TM_REGISTER_UNREGISTER;
	if(enStateBackup == enState)
	{
		return;
	}
	enStateBackup = enState;
    switch(enState)
    {
    case TM_REGISTER_ACCEPT:
    case TM_REGISTER_ERROR:
    case TM_REGISTER_REJECT:
    case TM_REGISTER_PROCESS: 
    case TM_REGISTER_UNREGISTER:
        break;
    }
    TMSystemLogOut(TM_MESSAGE, pcMessage);
    //LOG_RUNLOG_WARN(pcMessage);
}

static void CommandFunc(int iType, int iCommand, const char* pcMessage)
{
    if(!iType)
    {
        TMSystemLogOut(TM_MESSAGE, "TMRecv : %s ", pcMessage);
    }
    else
    {
        TMSystemLogOut(TM_MESSAGE, "TMSend : %s ", pcMessage);
    }
}

static void CommunicateChangedFunc(TM_COMMUNICATE_CHANGED_EN enID)
{
	char acUserName[64];
	char acPassword[64];
	char acProxy[64];
	char acDomain[64];
	int  iPort;
	switch(enID)
	{
	case TM_CONFIG_SIP_CHANGED:
		TMGetIMSInfo(acUserName, acPassword, acProxy, &iPort, acDomain);
		LPMsgSendTmSipRegister(acUserName, acPassword, acDomain,acProxy);
		NMNotifyInfo();
		break;
	case TM_CONFIG_FTP_CHANGED:
		break;
	case TM_CONFIG_WAN_CHANGED:
		NMNotifyInfo();
		break;
	default:
		break;
	}
}

static void TranaferProcessFunc(FP_TRANSFER_INFO_ST* pstInfo)
{
    TMSystemLogOut(TM_MESSAGE, "dAvlSpeed:%f, Process: %d, totalTime: %d\n", 
        pstInfo->dAvlSpeed,
        pstInfo->lTransferredSize*100/pstInfo->lTotalSize, 
        pstInfo->lTotalTime);    
}

static void RemoteTransferFunc(TM_TRANSFER_TYPE_EN enType, TM_TRANSFER_FILE_TYPE_EN enFileType, const char* pcLocalUrl, const char* pcRemoteUrl)
{
    char acMessage[1024]={0x0};                    
    char acUserName[256], acPassWord[256], pacHost[256]; int iPort=21;
    sprintf(acMessage, "Transfer: %s Local://%s Remote://%s", enType==0?"Download":"Upload", pcLocalUrl, pcRemoteUrl);
    TMSystemLogOut(TM_MESSAGE, acMessage);
    TMQueryFtpInfo(pacHost, &iPort, acUserName, acPassWord);
    switch(enFileType)
    {
    case TM_TYPE_EVENT_LOG:
    case TM_TYPE_SYSTEM_LOG:
    	LOGUploadResPrepross();
    	FPPutLog(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        break;
    case TM_TYPE_CONFIG:
        if(enType==TM_REMOTE_DOWNLOAD)
        {
        	FPUpdateConfig(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        }
        else
        {
        	FPPut(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        }
        break;
    case TM_TYPE_RINGING:
        if(enType==TM_REMOTE_DOWNLOAD)
        {
        	FPUpdateRinging(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        }
        else
        {
        	FPPut(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        }
        break;
    case TM_TYPE_UPDATE:
        FPUpdateApplication(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        break;
    case TM_TYPE_OTHER:
        if(enType==TM_REMOTE_DOWNLOAD)
        {
        	FPGet(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        }
        else
        {
        	FPPut(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        }
        break;
    }
}

static void RebootSysFunc(TM_REBOOT_CODE_EN enCode, const char* pcMessage)
{
	 switch(enCode)
	{
	case TM_REBOOT_BY_REMOTE:
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_PLATFORM_REBOOT, 0, "");
		ExitApp();
		break;
	case TM_REBOOT_BY_UNKNOW:
		break;
	default:
		break;
	}
}

static void QuitModuleFunc(int iCode, const char* pcMessage)
{

}

static TM_CORE_VTABLE_ST g_CoreVtable=
{
    ConnectFunc,
    ConfigFunc,
    CommandFunc,
    RemoteTransferFunc,
    CommunicateChangedFunc,
    RegisterFunc,
    RebootSysFunc,
    QuitModuleFunc,
};

#endif//!_YK_OS_==_YK_OS_LINUX_

static void* TMThreadFunc(void*pvParam)
{
    TMFiltersPreProcess();
    while(g_bTMRunning)
    {
        TMFiltersProcess();
    }
    TMFiltersPostProcess();
#if _YK_OS_ == _YK_OS_WIN32_
    WSACleanup();
#endif
    return NULL;
}

/**
 *@name Public interface.
 *@{
 */

YKHandle TMCCGetHouseInfoByHouseCode(const char* pcHouseCode)
{
    TM_HOUSE_INFO_ST* pstHouseInfo=YKNew0(TM_HOUSE_INFO_ST, 1);
    if(pcHouseCode == NULL || pstHouseInfo == NULL)
    {
    	return (YKHandle)NULL;
    }
    if(!TMQueryHouseByHouseCode(pstHouseInfo, pcHouseCode))
    {
        YKFree(pstHouseInfo);
        pstHouseInfo=NULL;
    }
    return (YKHandle)pstHouseInfo;
}

void TMCCDestroyHouseInfo(YKHandle hHouse)
{
	if(hHouse ==NULL)
	{
		return;
	}
    YKFree(hHouse);
}

int TMCCGetRingTime(YKHandle hHouse)
{
    TM_HOUSE_INFO_ST* pstHouseInfo=(TM_HOUSE_INFO_ST*)hHouse;
    char acTmp[SIZE_16]={0x0};
	if(pstHouseInfo ==NULL)
	{
		return 0;
	}
    TMTrimNLeftSpace(acTmp, pstHouseInfo->acRingTime, sizeof(pstHouseInfo->acRingTime));
    return TMHexStringToDecimal(acTmp);
}

BOOL TMCCIsInCommingCallNotice(YKHandle hHouse)
{
    TM_HOUSE_INFO_ST* pstHouseInfo=(TM_HOUSE_INFO_ST*)hHouse;
	if(pstHouseInfo ==NULL)
	{
		return FALSE;
	}
    return pstHouseInfo->cInCommingcCallNotice[0]=='0'?FALSE:TRUE;
}

int TMCCGetSipCallDelay(YKHandle hHouse)
{
    TM_HOUSE_INFO_ST* pstHouseInfo=(TM_HOUSE_INFO_ST*)hHouse;
    char acTmp[SIZE_16]={0x0};
	if (pstHouseInfo == NULL)
	{
		return 0;
	}
    TMTrimNLeftSpace(acTmp, pstHouseInfo->acSIPCallDelay, sizeof(pstHouseInfo->acSIPCallDelay));
    return TMHexStringToDecimal(acTmp);
}

int TMCCGetPhoneCount(YKHandle hHouse)
{
	if (hHouse ==NULL)
	{
		return 0;
	}
    return TMQueryPhoneCount((TM_HOUSE_INFO_ST*)hHouse);
}

YKHandle TMCCGetPhoneInfoByIndex(YKHandle hHouse, int iIndex)
{
    return TMQueryPhoneInfo((TM_HOUSE_INFO_ST*)hHouse, iIndex);
}
BOOL TMCCIsVoipEnable(YKHandle hPhone)
{
    TM_PHONE_INFO_ST* phoneInfo=(TM_PHONE_INFO_ST*)hPhone;
    if (phoneInfo ==NULL)
	{
		return FALSE;
	}
    return phoneInfo->cVoipEnable[0]=='0'?FALSE:TRUE;
}

const char* TMCCGetCsPhoneNumber(YKHandle hPhone)
{
    TM_PHONE_INFO_ST* phoneInfo=(TM_PHONE_INFO_ST*)hPhone;
    char number[SIZE_32]={0X0};
	if (phoneInfo == NULL)
	{
		return NULL;
	}
    TMTrimNLeftSpace(number, phoneInfo->acCsPhoneNumber, sizeof(phoneInfo->acCsPhoneNumber));
    if(number[0] == ' ')
    {
        return NULL;
    }
    else
    {
    	TMSystemLogOut(TM_MESSAGE, "TMCCGetCsPhoneNumber %s\n", number);
    	return number;
    }
}
const char* TMCCGetSipPhoneNumber(YKHandle hPhone)
{
    TM_PHONE_INFO_ST* phoneInfo=(TM_PHONE_INFO_ST*)hPhone;
    char number[SIZE_32]={0X0};
	if (phoneInfo == NULL)
	{
		return NULL;
	}
    TMTrimNLeftSpace(number, phoneInfo->acSipPhoneNumber, sizeof(phoneInfo->acSipPhoneNumber));
    if(number[0] == ' ')
    {
        return NULL;
    }
    else
    {
    	TMSystemLogOut(TM_MESSAGE, "TMCCGetSipPhoneNumber %s\n", number);
    	return number;
    }
}

BOOL TMCCIsNotDisturbTime(YKHandle hPhone)
{
    return TMIsNotDisturbTime((TM_PHONE_INFO_ST*)hPhone);
}

void TMCCGetHouseCode(char* pcHouseCode, char* pcRoomNum, int len)
{
	int i= 0;
	for(i = 0; i< len;i++)
	{
		if(pcRoomNum[i] != ' ')
		{
			break;
		}
	}
	memcpy(pcHouseCode,&pcRoomNum[i], len-i);
	pcHouseCode[len-i] = '\0';
}

BOOL TMCCIsValidPhoneNum(const char* pcPhoneNum, char* pcHouseCode)
{
	TM_HOUSE_INFO_ST stHouseInfo;
	TM_PHONE_INFO_ST *stPhoneInfo = NULL;
	int houseCount=0, PhoneNumCount=0;
	int iHouseIndex=0, PhoneNumIndex=0;
	const char* pcNum = NULL;
	char acTemp[64]={0x0};
	if (NULL==pcPhoneNum)
	{
		return FALSE;
	}
	houseCount=TMQueryHouseCount();
	for(iHouseIndex=0; iHouseIndex< houseCount; iHouseIndex ++)
	{
		if(TMQueryHouseInfo(&stHouseInfo, iHouseIndex))
		{
			PhoneNumCount=TMQueryPhoneCount(&stHouseInfo);
			for(PhoneNumIndex=0; PhoneNumIndex< PhoneNumCount; PhoneNumIndex++)
			{
				stPhoneInfo = TMQueryPhoneInfo(&stHouseInfo,PhoneNumIndex);
				if (stPhoneInfo == NULL)
				{
					continue;
				}
				pcNum = TMCCGetCsPhoneNumber(stPhoneInfo);
				if (NULL != pcNum)
				{
					strcpy(acTemp, pcNum);
					TMSystemLogOut(TM_MESSAGE, "InComing PhoneNumber: %s, CsPhoneNumber :%s\n", pcPhoneNum, acTemp);
					if((strlen(acTemp) < strlen(pcPhoneNum))?\
						0==strcmp(pcPhoneNum+strlen(pcPhoneNum)-strlen(acTemp), acTemp):\
						0==strcmp(acTemp+strlen(acTemp)-strlen(pcPhoneNum), pcPhoneNum))
                    {
                        if(NULL!=pcHouseCode)
                        {

                            //memcpy(pcHouseCode, stHouseInfo.acHouseCode, sizeof(stHouseInfo.acHouseCode));
                        	TMCCGetHouseCode(pcHouseCode,stHouseInfo.acHouseCode, sizeof(stHouseInfo.acHouseCode));
                        }
						return TRUE;
					}
				}
				pcNum = TMCCGetSipPhoneNumber(stPhoneInfo);
				if (NULL!=pcNum)
				{
					strcpy(acTemp, pcNum);
					TMSystemLogOut(TM_MESSAGE, "InComing PhoneNumber: %s, CsPhoneNumber :%s\n", pcPhoneNum, acTemp);
					if((strlen(acTemp) < strlen(pcPhoneNum))?\
							0==strcmp(pcPhoneNum+strlen(pcPhoneNum)-strlen(acTemp), acTemp):\
							0==strcmp(acTemp+strlen(acTemp)-strlen(pcPhoneNum), pcPhoneNum))
                    {
                        if(NULL!=pcHouseCode)
                        {
                            //memcpy(pcHouseCode, stHouseInfo.acHouseCode, sizeof(stHouseInfo.acHouseCode));
                        	TMCCGetHouseCode(pcHouseCode,stHouseInfo.acHouseCode, sizeof(stHouseInfo.acHouseCode));
                        }
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

BOOL TMSMIsValidICIDCard(const char* pcSerinalNumber)
{
    TM_HOUSE_INFO_ST stHouseInfo;
    int houseCount=0, ICIDCardCount=0;
    int iHouseIndex=0, ICIDCardIndex=0;
	char acTmp[SIZE_32]={0X0};
	if (pcSerinalNumber == NULL)
	{
		return FALSE;
	}
	houseCount=TMQueryHouseCount();
    for(iHouseIndex=0; iHouseIndex< houseCount; iHouseIndex ++)
    {
        if(TMQueryHouseInfo(&stHouseInfo, iHouseIndex))
        {
            ICIDCardCount=TMQueryICIDCardCount(&stHouseInfo);
            for(ICIDCardIndex=0; ICIDCardIndex< ICIDCardCount; ICIDCardIndex++)
            {
                memset(acTmp, 0X0, sizeof(acTmp));
                if(TMQueryICIDCardInfo(acTmp, &stHouseInfo, ICIDCardIndex))
                {
                    if(TMCompareNoCase(pcSerinalNumber, acTmp, strlen(acTmp)))
                    {
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

int  TMCCGetIMSCallTimeOut(void)
{
    return TMQueryCallTimeOut();
}

BOOL TMCCIsIMSCallEnabled(void)
{
    return TMQueryIMSIsEnabled();
}

BOOL  TMCCIsKeyOpenDoorEnabled(void)
{
    return TMQueryKeyOpenDoorEnabled();
}

BOOL TMCCIsVoipMonitorEnabled(void)
{
    return TMQueryMonitorEnabled();
}

BOOL  TMSMIsICOpenDoorEnabled(void)
{
    return TMQueryICOpenDoorEnabled();
}

void TMNMGetWANInfo(char* pcUserName, char* pcPassWord)
{
    TMQueryWanInfo(pcUserName, pcPassWord);
}

void TMGetIMSInfo(char* pcUserName, char* pcPassWord, char*pcProxy, int* piPort, char* pcDomain)
{
    TMQuerySipInfo(pcUserName, pcPassWord, pcProxy, piPort, pcDomain);

}

void TMUpdateAlarmState(TM_ALARM_ID_EN enModule, char cState)
{
    TMSendAlarm(enModule, cState);
}


int TMStartTerminateManage(const char* pcVersion, const char* pcConfig)
{
#if _YK_OS_ == _YK_OS_WIN32_
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData) == 0)
    {
        printf("WSA Start up!\n");
    }
#endif
    if(!TMInitTerminateManage(pcVersion, pcConfig))
    {
        TMSystemLogOut(TM_ERROR, "TMInitTerminateManage failed !\n");
        return FALSE;
    }
    TMFiltersInit(&g_CoreVtable);
    g_bTMRunning = TRUE;
    g_hTMThread = YKCreateThread(TMThreadFunc, NULL);
    return NULL!=g_hTMThread;
}

void TMStopTerminateMange(void)
{
    TMSendCommand(TM_COMMAND_QUIT);
    YKSleep(1000);
    g_bTMRunning=FALSE;
    if(NULL!=g_hTMThread)
    {
        YKJoinThread(g_hTMThread);
        YKDestroyThread(g_hTMThread);
        g_hTMThread=NULL;
    }
}

//For Debug APIs
void TMRegister2TMP()
{
    TMSendCommand(TM_COMMAND_REGISTER);
}
void TMUnRegisterFromTMP()
{
    TMSendCommand(TM_COMMAND_QUIT);
}
void TMSendWorkInfo()
{
    TMSendCommand(TM_COMMAND_REQ_WORK_INFO);
}
void TMSendDeviceInfo()
{
    TMSendCommand(TM_COMMAND_REQ_DEVICE_INFO);
}
void TMSendAlamEnable()
{
    TMSendCommand(TM_COMMAND_REQ_ALARM_ENABLE_INFO);
} 
void TMSendCommunicateInfo()
{
    TMSendCommand(TM_COMMAND_REQ_COMMUNICATE_INFO);
}
void TMSendUserServices()
{
    TMSendCommand(TM_COMMAND_REQ_USER_SERVICES_INFO);
}

void TMSendOption()
{
    TMSendCommand(TM_COMMAND_HEART);
}
#endif
/**@}*///Public interface.
