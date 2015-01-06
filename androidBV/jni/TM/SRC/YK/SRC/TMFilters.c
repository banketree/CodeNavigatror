#include "../INC/TMFilters.h"
#include "../INC/TMProcotol.h"
#include "../INC/TMConfig.h"
#include "../../../INC/TMInterface.h"
#include <YKApi.h>
#include <IDBT.h>

#if _TM_TYPE_ == _YK069_

extern TM_FILTER_DESC_ST g_stConfigReadFilter;
extern TM_FILTER_DESC_ST g_stCommandRecvFilter;
extern TM_FILTER_DESC_ST g_stContextDecodeFilter;
extern TM_FILTER_DESC_ST g_stContxtCheckFilter;
extern TM_FILTER_DESC_ST g_stContextEncodeFilter;
extern TM_FILTER_DESC_ST g_stConfigWriteFilter;
extern TM_FILTER_DESC_ST g_stCommandSendFilter;

static TM_FILTER_DESC_ST* g_pstTMFilterDescs[]=
{
    &g_stConfigReadFilter,
    &g_stCommandRecvFilter, 
    &g_stContextDecodeFilter,
    &g_stContxtCheckFilter,
    &g_stContextEncodeFilter,
    &g_stConfigWriteFilter,
    &g_stCommandSendFilter,
};

static char g_acTMFirmVersion[SIZE_16];
static char g_acTMConfigPath[SIZE_256];
static char g_pcDeviceID[64] = {0x0};
static TM_REQ_ALARM_STATUS_ST g_stAlarmStatus;

#define TMQueryRegistateContext(x)\
    x==TM_REGISTER_ACCEPT?Text(TM_REGISTER_ACCEPT):\
    (x==TM_REGISTER_ERROR?Text(TM_REGISTER_ERROR):\
    (x==TM_REGISTER_REJECT?Text(TM_REGISTER_REJECT):Text(TM_REGISTER_UNREGISTER)))

TM_CONTEXT_STATE_EN TMDeviceInfoCheck(TM_SET_DEVICE_INFO_ST* pstInfo, int iSize);
TM_CONTEXT_STATE_EN TMCommunicateInfoCheck(TM_SET_COMMUNICATE_INFO_ST* pstInfo, int iSize);
TM_CONTEXT_STATE_EN TMWorkInfoCheck(TM_SET_WORK_INFO_ST* pstInfo, int iSize);
TM_CONTEXT_STATE_EN TMAlarmCheck(TM_SET_ALARM_ENABLE_ST*pstInfo, int iSize);
TM_CONTEXT_STATE_EN TMSysRebootChekc(TM_SET_SYS_REBOOT_ST*pstInfo, int iSize);
TM_CONTEXT_STATE_EN TMSetDownloadCheck(TM_SET_DOWNLOAD_ST*pstInfo, int iSize);
TM_CONTEXT_STATE_EN TMSetUploadCheck(TM_SET_UPLOAD_ST*pstInfo, int iSize);
TM_CONTEXT_STATE_EN TMSetTransferCheck(TM_SET_TRANSFER_ST*pstInfo, int iSize);
TM_CONTEXT_STATE_EN TMUserServiceCheck(TM_USER_SERVICE_INFO_ST*pstInfo, int iSize);
TM_CONTEXT_STATE_EN TMSetCallOutGoingCheck(TM_OUT_GOING_CALL_INFO*pstInfo, int iSize);

int  TMHexStringToDecimal(const char* pcHex);
BOOL TMCompareNoCase(const char* pcSrc, const char* pcDest, int iLen);
BOOL TMDecodePacket(TM_MSG_DATA_ST* pstIn, TM_MSG_DATA_ST**ppstOut, TM_CONTEXT_STATE_EN*penError);
BOOL TMEncodePacket(TM_MSG_DATA_ST*pstIn, TM_MSG_DATA_ST**ppstOut); 
TM_FILTER_DESC_ST*   TMQueryFilterDesc(TM_FILTER_DESC_ID_EN enID);
void TMQueryPlatformIp(char* pcIP);

void TMTrimNLeftSpace(char* pcDest, const char* pcValue, int iSize)
{
    int iIndex=0;
    while(pcValue[iIndex++]==' ' && iIndex <iSize);
    if(pcValue[iIndex-1]!=0x0 && iIndex-1 <iSize)
    {
        memcpy(pcDest, &pcValue[iIndex-1], iSize-iIndex+1);
    }
}

static void TMFreeFunc(void* pvData) 
{
    free(((TM_MSG_DATA_ST*)pvData)->pvBuffer); 
    free(pvData);
};

static unsigned long TMGetHostAddress(const char* pcHost)
{
    struct hostent* pstHost  = NULL;
    unsigned long	ulHostIP = 0;
    int				hSock = -1;
    pstHost = gethostbyname(pcHost);
    if(pstHost)
    {
    	ulHostIP = *((unsigned long*) (pstHost->h_addr));
    }
    else
    {
    	ulHostIP = inet_addr(pcHost);
    }
    return ulHostIP;
}

static void  TMDisConnect(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
    closesocket((SOCKET)hHandle);
#else
    close((int)hHandle);
#endif
}

static YKHandle  TMConnect(const char* pcHost, int iPort, unsigned int uiProto)
{
    BOOL ret = FALSE;
    YKHandle hHandle = NULL;
    struct sockaddr_in clientAddr;
    if((hHandle = (YKHandle)socket(AF_INET, SOCK_STREAM, uiProto)) < 0)
    {
    	return NULL;
    }
    memset(&clientAddr, 0x0, sizeof(clientAddr));
    clientAddr.sin_addr.s_addr = TMGetHostAddress(pcHost);
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons( (unsigned short)iPort );
    if (connect( (int)hHandle, (struct sockaddr*) &clientAddr, sizeof(clientAddr) ) < 0)
    {
        TMDisConnect(hHandle);
        hHandle=NULL;
    }
    return hHandle;
}

static int  TMSelect(void* handle, unsigned long timeout)
{
    fd_set set;
    struct timeval timeval;
    int ret;
    timeval.tv_sec = timeout/1000;
    timeval.tv_usec = 0;
    FD_ZERO(&set);
    FD_SET((int)handle, &set);
    return select((int)handle+1, &set, NULL, NULL, &timeval);//0 time out, -1 failed, other success
}

static int   TMRecv(YKHandle hHandle, void*pvBuffer, int iSize, int iTimeOut)
{
    switch(TMSelect(hHandle, iTimeOut))
    {
    case 0://time out
        return 0;
    case -1://failed
        return -1;
    default:
        return recv((int)hHandle, (char*)pvBuffer, iSize, 0);
    }
}

static int   TMSend(YKHandle hHandle, void*pvBuffer, int iSize)
{
    return send((int)hHandle, (char*)pvBuffer, iSize, 0);
}

static int TMCountFilters()
{
    return sizeof(g_pstTMFilterDescs)/sizeof(TM_FILTER_DESC_ST*);
}

static TM_FILTER_DESC_ST* TMFindFirstFilters()
{
    return (TMCountFilters()>0)? g_pstTMFilterDescs[0]:NULL;
}

static TM_FILTER_DESC_ST* TMFindNextFilters(TM_FILTER_DESC_ST* pstDesc)
{
    int i;
    for(i=0; i< TMCountFilters(); i++)
    {
        if(g_pstTMFilterDescs[i]==pstDesc && i+1<TMCountFilters())
        {
            return g_pstTMFilterDescs[i+1];
        }
    }
    return NULL;
}

static TM_FILTER_DESC_ST* TMFindLastFilters()
{
    return (TMCountFilters()>0)? g_pstTMFilterDescs[TMCountFilters()-1]:NULL; 
}

static TM_FILTER_DESC_ST* TMFindPrevFilters(TM_FILTER_DESC_ST* pstDesc)
{
    int i;
    for(i= TMCountFilters()-1; i>=0; i--)
    {
        if(g_pstTMFilterDescs[i]==pstDesc && i>0)
        {
            return g_pstTMFilterDescs[i-1];
        }
    }
    return NULL;
}

static void TMFilterLink(TM_FILTER_DESC_ST*pstSrc, TM_FILTER_DESC_ST* pstDesc)
{
    pstSrc->pstFilter->pstSend = pstDesc->pstFilter->pstRecv;
}

static void TMFilterShare(TM_FILTER_DESC_ST*pstSrc, TM_FILTER_DESC_ST* pstDesc)
{
    pstDesc->pstFilter->pvData = pstSrc->pstFilter->pvData;
}

static void TMFilterLinkVtable(TM_CORE_VTABLE_ST* callback)
{
    TM_FILTER_DESC_ST* pstDesc=NULL;
    pstDesc=TMFindFirstFilters();
    while(pstDesc)
    {
        pstDesc->pstFilter->pstCallBack=callback;
        pstDesc = TMFindNextFilters(pstDesc);
    }     
}

static void TMSendMessage(YK_MSG_QUE_ST*q, void* pvBuffer, int iSize, TM_MESSAGE_FOR_EN MsgType, TM_COMMAND_TYPE_EN enCMD)
{
    TM_MSG_DATA_ST *pstData = YKNew0(TM_MSG_DATA_ST, 1);
    if(NULL==pstData)
    {
    	return;
    }
    pstData->pvBuffer = pvBuffer;
    pstData->iSize= iSize;
    pstData->enMessageType = MsgType;
    pstData->enCommandType = enCMD;
    YKWriteQue(q, pstData,1);
}

static void TMCallbackConnectStateChanged(TM_FILTER_ST* filter, TM_CONNECT_STATE_EN enState, const char* pcMessage)
{
    if(filter && filter->pstCallBack->ConnectState)
    {
        filter->pstCallBack->ConnectState(enState, pcMessage);
    }
}

static void TMCallbackConfigStateChanged(TM_FILTER_ST* filter, TM_CONFIG_STATE_EN enState, const char* pcMessage)
{
    if(filter && filter->pstCallBack->ConfigState)
    {
        filter->pstCallBack->ConfigState(enState, pcMessage);
    }
}

static void TMCallbackRemoteTransfer(TM_FILTER_ST* filter,TM_COMMAND_TYPE_EN command, void* pvBuffer, int iSize)
{
    TM_TRANSFER_TYPE_EN enType=TM_REMOTE_UPLOAD;
    TM_SET_UPLOAD_ST* upload = (TM_SET_UPLOAD_ST*)pvBuffer;
    TM_SET_DOWNLOAD_ST* download=(TM_SET_DOWNLOAD_ST*)pvBuffer;
    TM_SET_TRANSFER_ST* trans=(TM_SET_TRANSFER_ST*)pvBuffer;
    char pcLocalUrl[SIZE_256]={0x0};
    char pcRemoteUrl[SIZE_256]={0x0};
    TM_TRANSFER_FILE_TYPE_EN enFileType=TM_TYPE_OTHER;

    switch(command)
    {
    case TM_COMMAND_SET_DOWNLOAD:
        TMTrimNLeftSpace(pcRemoteUrl, download->acRemoteUrl, sizeof(download->acRemoteUrl));
        switch(download->cType[0])
        {
        case '0':
            enFileType=TM_TYPE_CONFIG;
            strcpy(pcLocalUrl, g_acTMConfigPath);
            break;
        case '1':
            enFileType=TM_TYPE_RINGING;
            strcpy(pcLocalUrl, "Ringing");
            break;
        case '2':
            enFileType=TM_TYPE_UPDATE;
            strcpy(pcLocalUrl, "YK-IDBT.bz2");
            break;
        };
        enType=TM_REMOTE_DOWNLOAD;
        break;
    case TM_COMMAND_SET_UPLOAD:
        enType= TM_REMOTE_UPLOAD;
        TMTrimNLeftSpace(pcRemoteUrl, upload->acRemoteUrl, sizeof(upload->acRemoteUrl));
        switch(upload->cType[0])
        {
        case '0':
            enFileType=TM_TYPE_CONFIG;
            strcpy(pcLocalUrl, g_acTMConfigPath);
            break;
        case '1':
            enFileType=TM_TYPE_CONFIG;
            strcpy(pcLocalUrl, g_acTMConfigPath);
            break;
        case '2':
            enFileType=TM_TYPE_EVENT_LOG;
            strcpy(pcLocalUrl, LG_LOCAL_PATH);
            break;
        }
        break;
    case TM_COMMAND_SET_TRANSFER:
        if('0'==trans->cType[0])
        {
            enType= TM_REMOTE_DOWNLOAD;
        }
        enFileType=TM_TYPE_OTHER;
        TMTrimNLeftSpace(pcLocalUrl, trans->acLocalUrl, sizeof(trans->acLocalUrl));
        TMTrimNLeftSpace(pcRemoteUrl, trans->acRemoteUrl, sizeof(trans->acRemoteUrl));
        break;
    }
    if(filter && filter->pstCallBack->RemoteTransfer)
    {
        filter->pstCallBack->RemoteTransfer(enType, enFileType, pcLocalUrl, pcRemoteUrl);
    }
}

static void TMCallbackRegisterState(TM_FILTER_ST* filter,TM_REGISTER_STATE_EN enState, const char* pcMessage)
{
    if(filter && filter->pstCallBack->RegisterState)
    {
        filter->pstCallBack->RegisterState(enState, pcMessage);
    }
}

static void TMCallbackCommandSendRecv(TM_FILTER_ST* filter, TM_MESSAGE_FOR_EN enType, TM_COMMAND_TYPE_EN enCMD, const char* pvBuffer, int iSize)
{
    if(filter&&filter->pstCallBack->CommandState)
    {
        filter->pstCallBack->CommandState(enType, enCMD, pvBuffer);
    }
}

static void TMCallbackCommunicateChanged(TM_FILTER_ST* filter, TM_COMMUNICATE_CHANGED_EN enID)
{
    if(filter&&filter->pstCallBack->CommunicateChanged)
    {
        filter->pstCallBack->CommunicateChanged(enID);
    }
}

static void TMCallbackRebootSystem(TM_FILTER_ST* filter,TM_REBOOT_CODE_EN enCode, const char* pcMessage)
{
    if(filter&&filter->pstCallBack->Reboot)
    {
        filter->pstCallBack->Reboot(enCode, pcMessage);
    }
}

static void TMCallbackQuitModule(TM_FILTER_ST* filter,int iCode, const char* pcMessage)
{
    if(filter&&filter->pstCallBack->Quit)
    {
        filter->pstCallBack->Quit(iCode, pcMessage);
    }
}

///////////////////////////////////////////////////////////
//Filters.
typedef const char* (*CfgQueryString)(void* hHandle, const char* session, const char* pcKey);
typedef int         (*CfgQueryInt)(void* hHandle, const char* session, const char* pcKey);
typedef void        (*CfgSetString)(void* hHandle, const char* session, const char* pcKey, const char* pcValue);
typedef void        (*CfgSetInt)(void* hHandle, const char* session, const char* pcKey, int iValue);
typedef BOOL        (*CfgClearSection)(void* hHandle, const char *pcSection);
typedef struct __CfgReadWrite{
    YKHandle hCfg; 
    YKHandle hLock;
    CfgQueryString ReadString;
    CfgQueryInt    ReadInt;
    CfgSetString   WriteString;
    CfgSetInt      WriteInt;
    CfgClearSection CleanSession;
}CfgReadWrite;

static BOOL TMCfgClearSession(CfgReadWrite* hHandle, const char* session)
{
    char* pcValue=NULL;
    if(hHandle)
    {
        YKLockMutex(hHandle->hLock);
        if (TMConfigHasSection((TM_CFG_OBJECT_ST)hHandle->hCfg, session)) 
        {
            TMConfigCleanSection((TM_CFG_OBJECT_ST)hHandle->hCfg, session);
            YKUnLockMutex(hHandle->hLock);
            return TRUE;
        }
        YKUnLockMutex(hHandle->hLock);
    }
    return FALSE;
}

static const char* TMCfgReadString(CfgReadWrite* hHandle, const char* session, const char* pcKey)
{
    char* pcValue=NULL;
    if(hHandle)
    {
        YKLockMutex(hHandle->hLock);
        pcValue=TMConfigGetString((TM_CFG_OBJECT_ST)hHandle->hCfg, session, pcKey, NULL);
        YKUnLockMutex(hHandle->hLock);
    }
    return pcValue;
}

static int TMCfgReadInt(CfgReadWrite* hHandle, const char* session, const char* pcKey)
{
    int iValue=0;
    if(hHandle)
    {
        YKLockMutex(hHandle->hLock);
        iValue=TMConfigGetInt((TM_CFG_OBJECT_ST)hHandle->hCfg, session, pcKey, 0);
        YKUnLockMutex(hHandle->hLock);
    }
    return iValue;
}

static void TMCfgWriteString(CfgReadWrite* hHandle, const char* session, const char* pcKey, const char* pcValue)
{
    if(hHandle)
    {
        YKLockMutex(hHandle->hLock);
        TMConfigSetString((TM_CFG_OBJECT_ST)hHandle->hCfg, session, pcKey, pcValue);
        YKUnLockMutex(hHandle->hLock);
    }
}

static void TMCfgWriteInt(CfgReadWrite* hHandle, const char* session, const char* pcKey, int iValue)
{
    if(hHandle)
    {
        YKLockMutex(hHandle->hLock);
        TMConfigSetInt((TM_CFG_OBJECT_ST)hHandle->hCfg, session, pcKey, iValue);
        YKUnLockMutex(hHandle->hLock);
    }
}

static void TMFillRule(char*pcDest, const char*pcSrc, const char* fill,int iLen)
{
    char rule[SIZE_16]={0x0};
    char filld[SIZE_128]={0x0};
    strcpy(rule, "%");    
    strcat(rule,fill);                     
    sprintf(rule+strlen(rule), "%ds", iLen);
    if(NULL!=pcSrc)
    {                        
        sprintf(filld, rule, pcSrc);
    }
    else
    {
        sprintf(filld, rule, fill);
    }
    memcpy(pcDest, filld, iLen);
}

static TM_REQ_COMMUNICATE_INFO_ST*   TMReadCommunicateInfo(CfgReadWrite* rw, int*iSize)
{
    TM_REQ_COMMUNICATE_INFO_ST* pstInfo=YKNew0(TM_REQ_COMMUNICATE_INFO_ST, 1);
    *iSize=sizeof(TM_REQ_COMMUNICATE_INFO_ST);
    TMFillRule(pstInfo->acWanUserName, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(WanUserName)), " ", sizeof(pstInfo->acWanUserName));
    TMFillRule(pstInfo->acWanPassword, rw->ReadString(rw, Text(CommunicateInfo),\
        Text(WanPassword)), " ", sizeof(pstInfo->acWanPassword));
    TMFillRule(pstInfo->acSipUsername, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(SipUsername))," ", sizeof(pstInfo->acSipUsername));
    TMFillRule(pstInfo->acSipPassword, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(SipPassword)), " ",sizeof(pstInfo->acSipPassword));
    TMFillRule(pstInfo->acTr069Username, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(Tr069Username))," ", sizeof(pstInfo->acTr069Username));
    TMFillRule(pstInfo->acTr069Password, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(Tr069Password))," ",sizeof(pstInfo->acTr069Password));
    TMFillRule(pstInfo->acImsDomain, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(ImsDomain)), " ",sizeof(pstInfo->acImsDomain));
    TMFillRule(pstInfo->acImsProxyIP, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(ImsProxyIP)), " ",sizeof(pstInfo->acImsProxyIP));
    TMFillRule(pstInfo->acImsPorxyPort, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(ImsPorxyPort))," ", sizeof(pstInfo->acImsPorxyPort));
    TMFillRule(pstInfo->acPlatformIP, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(PlatformIP)), " ",sizeof(pstInfo->acPlatformIP));
    TMFillRule(pstInfo->acPlatformPort, rw->ReadString(rw, Text(CommunicateInfo),\
        Text(PlatformPort))," ",sizeof(pstInfo->acPlatformPort));
    TMFillRule(pstInfo->acFtpIp, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(FtpIp))," ",sizeof(pstInfo->acFtpIp));
    TMFillRule(pstInfo->acFtpPort, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(FtpPort))," ",sizeof(pstInfo->acFtpPort));
    TMFillRule(pstInfo->acFtpUsername, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(FtpUsername))," ",sizeof(pstInfo->acFtpUsername));
    TMFillRule(pstInfo->acFtpPassword, rw->ReadString(rw, Text(CommunicateInfo), \
        Text(FtpPassword))," ",sizeof(pstInfo->acFtpPassword));
    return pstInfo;
}

static TM_REQ_WORK_INFO_ST*  TMReadWorkInfo(CfgReadWrite* rw, int*iSize)
{
    TM_REQ_WORK_INFO_ST* pstInfo=YKNew0(TM_REQ_WORK_INFO_ST, 1);
    *iSize=sizeof(TM_REQ_WORK_INFO_ST);
    TMFillRule(pstInfo->acSipCallTimeout, rw->ReadString(rw, Text(WorkInfo), Text(SipCallTimeout)), " ",\
        sizeof(pstInfo->acSipCallTimeout));
    TMFillRule(pstInfo->cImsCallEnable, rw->ReadString(rw, Text(WorkInfo), Text(ImsCallEnable)), " ",\
        sizeof(pstInfo->cImsCallEnable));
    TMFillRule(pstInfo->cIcOpenDoorEnable, rw->ReadString(rw, Text(WorkInfo), Text(IcOpenDoorEnable))," ",\
        sizeof(pstInfo->cIcOpenDoorEnable));
    TMFillRule(pstInfo->cWisdomOpenDoorEnable, rw->ReadString(rw, Text(WorkInfo), Text(WisdomOpenDoorEnable))," ",\
        sizeof(pstInfo->cWisdomOpenDoorEnable));
    TMFillRule(pstInfo->cMonitorEnable, rw->ReadString(rw, Text(WorkInfo), Text(MonitorEnable))," ",\
        sizeof(pstInfo->cMonitorEnable));
    return pstInfo;
}

static TM_REQ_ALARM_ENABLE_ST*   TMReadAlarmEnable(CfgReadWrite* rw, int*iSize)
{
    TM_REQ_ALARM_ENABLE_ST* pstInfo=YKNew0(TM_REQ_ALARM_ENABLE_ST, 1);
    *iSize=sizeof(TM_REQ_ALARM_ENABLE_ST);
    TMFillRule(pstInfo->cImsRegisterFail, rw->ReadString(rw, Text(AlarmEnable), Text(ImsRegisterFail)), " ",\
        sizeof(pstInfo->cImsRegisterFail));
    TMFillRule(pstInfo->cSipCallFail, rw->ReadString(rw, Text(AlarmEnable), Text(SipCallFail))," ",\
        sizeof(pstInfo->cSipCallFail));
    TMFillRule(pstInfo->cIfBoardComFail, rw->ReadString(rw, Text(AlarmEnable), Text(IfBoardComFail))," ",\
        sizeof(pstInfo->cIfBoardComFail));
    TMFillRule(pstInfo->cFileTransferFail, rw->ReadString(rw, Text(AlarmEnable), Text(FileTransferFail)), " ",\
        sizeof(pstInfo->cFileTransferFail));
    return pstInfo;
}

static TM_REQ_DEVICE_INFO_ST* TMReadDeviceInfo(CfgReadWrite* rw, int*iSize)
{
    TM_REQ_DEVICE_INFO_ST* pstInfo=YKNew0(TM_REQ_DEVICE_INFO_ST, 1);
    *iSize=sizeof(TM_REQ_DEVICE_INFO_ST);
    TMFillRule(pstInfo->acManufacturer, rw->ReadString(rw, Text(DeviceInfo), Text(Manufacturer))," ",\
        sizeof(pstInfo->acManufacturer));
    TMFillRule(pstInfo->acOui, rw->ReadString(rw, Text(DeviceInfo), Text(Oui)), " ",\
        sizeof(pstInfo->acOui));
    TMFillRule(pstInfo->acProductClass, rw->ReadString(rw, Text(DeviceInfo), Text(ProductClass))," ",\
        sizeof(pstInfo->acProductClass));
    TMFillRule(pstInfo->acSerialNumber, rw->ReadString(rw, Text(DeviceInfo), Text(SerialNumber))," ",\
        sizeof(pstInfo->acSerialNumber));
    TMFillRule(pstInfo->acFirmwareVersion, g_acTMFirmVersion, " ", sizeof(pstInfo->acFirmwareVersion));
    return pstInfo;
}

static void TMReadHouseInfo(CfgReadWrite* rw, TM_HOUSE_INFO_ST*pstInfo, int iHouseIndex)
{
    char session[SIZE_64]={0X0};
    sprintf(session, "%s_%d", Text(HouseInfo), iHouseIndex);
    TMFillRule(pstInfo->acHouseCode, rw->ReadString(rw, session, Text(HouseCode))," ",sizeof(pstInfo->acHouseCode));
    TMFillRule(pstInfo->acRingTime, rw->ReadString(rw, session, Text(RingTime)), " ",sizeof(pstInfo->acRingTime));
    TMFillRule(pstInfo->cInCommingcCallNotice, rw->ReadString(rw, session, Text(InCommingcCallNotice))," ",sizeof(pstInfo->cInCommingcCallNotice));
    TMFillRule(pstInfo->acSIPCallDelay, rw->ReadString(rw, session, Text(SIPCallDelay)), " ",sizeof(pstInfo->acSIPCallDelay));
    TMFillRule(pstInfo->cPhoneCount, rw->ReadString(rw, session, Text(PhoneCount)), " ",sizeof(pstInfo->cPhoneCount));
    TMFillRule(pstInfo->cCardCount, rw->ReadString(rw, session, Text(CardCount))," ",sizeof(pstInfo->cCardCount));
}

static void TMReadPhoneInfo(CfgReadWrite* rw, TM_HOUSE_INFO_ST*pstInfo, int iHouseIndex, int iPhoneIndex)
{
    char session[SIZE_64]={0X0};
    int count=0;
    char acTmp[SIZE_16]={0x0};
    int notDisturbIndex=0;
    sprintf(session, "%s_%d_%d", Text(PhoneInfo), iHouseIndex, iPhoneIndex);
    TMFillRule(pstInfo->stPhoneInfo[iPhoneIndex].cVoipEnable, rw->ReadString(rw, session, Text(VoipEnable)),\
        "0", sizeof(pstInfo->stPhoneInfo[iPhoneIndex].cVoipEnable));
    TMFillRule(pstInfo->stPhoneInfo[iPhoneIndex].acCsPhoneNumber, rw->ReadString(rw, session, Text(CsPhoneNumber)),\
        " ", sizeof(pstInfo->stPhoneInfo[iPhoneIndex].acCsPhoneNumber));
    TMFillRule(pstInfo->stPhoneInfo[iPhoneIndex].acSipPhoneNumber, rw->ReadString(rw, session, Text(SipPhoneNumber)),\
        " ", sizeof(pstInfo->stPhoneInfo[iPhoneIndex].acSipPhoneNumber));
    TMFillRule(pstInfo->stPhoneInfo[iPhoneIndex].cNotDisturbCount, rw->ReadString(rw, session, Text(NotDisturbCount)),\
        "0", sizeof(pstInfo->stPhoneInfo[iPhoneIndex].cNotDisturbCount));
    memcpy(acTmp, pstInfo->stPhoneInfo[iPhoneIndex].cNotDisturbCount, sizeof(pstInfo->stPhoneInfo[iPhoneIndex].cNotDisturbCount));
    count = TMHexStringToDecimal(acTmp);
    for(notDisturbIndex=0; notDisturbIndex<TM_NOTDISTURBTIME_MAX_COUNT; notDisturbIndex++)
    {
        sprintf(session, "%s_%d_%d_%d", Text(NotDisturbTime), iHouseIndex, iPhoneIndex, notDisturbIndex);
        TMFillRule(pstInfo->stPhoneInfo[iPhoneIndex].stNotDisturbTime[notDisturbIndex].acTime, \
            rw->ReadString(rw, session, Text(Time)),"0", \
            sizeof(pstInfo->stPhoneInfo[iPhoneIndex].stNotDisturbTime[notDisturbIndex].acTime));
    }
}

static void TMReadICIDCardInfo(CfgReadWrite* rw, TM_HOUSE_INFO_ST*pstInfo, int iHouseIndex, int cardIndex)
{
    char session[SIZE_64]={0X0};
    sprintf(session, "%s_%d_%d", Text(CardCount), iHouseIndex, cardIndex);
    TMFillRule(pstInfo->stCard[cardIndex].acSerinalNumber, \
        rw->ReadString(rw, session, Text(SerinalNumber))," ", sizeof(pstInfo->stCard[cardIndex].acSerinalNumber));
}

static void* TMReadUserServiceInfo(CfgReadWrite* rw, int*iSize)
{
    TM_USER_SERVICE_INFO_ST pstInfo;
    TM_HOUSE_INFO_ST pstHouseInfo;
    char acTmp[sizeof(pstInfo.acUserCount)+1]={0x0};
    int iHouseIndex, iPhoneIndex, cardIndex;
    int userCount, iPhoneCount, cardCount;
    char session[SIZE_64]={0X0};
    void* sendInfo=NULL;
    TMFillRule(acTmp, rw->ReadString(rw, Text(UserServiceInfo), Text(UserCount))," ",sizeof(pstInfo.acUserCount));
    userCount=TMHexStringToDecimal(acTmp);

    *iSize=userCount*sizeof(TM_HOUSE_INFO_ST)+sizeof(pstInfo.acUserCount);
    sendInfo=(void*)YKNew(char, *iSize);
    memcpy(sendInfo, acTmp, sizeof(pstInfo.acUserCount));

    if(NULL==sendInfo)
    {
        YKFree(sendInfo);
        return NULL;
    }
    for(iHouseIndex=0; iHouseIndex<userCount; iHouseIndex++)
    {
        memset(&pstHouseInfo, 0x0, sizeof(TM_HOUSE_INFO_ST));
        TMReadHouseInfo(rw, &pstHouseInfo, iHouseIndex); 

        memset(acTmp, 0x0, sizeof(acTmp));
        memcpy(acTmp, pstHouseInfo.cPhoneCount, sizeof(pstHouseInfo.cPhoneCount));
        iPhoneCount = TMHexStringToDecimal(acTmp);
        for(iPhoneIndex=0; iPhoneIndex<TM_PHONE_INFO_MAX_COUNT; iPhoneIndex++)
        {
            TMReadPhoneInfo(rw, &pstHouseInfo, iHouseIndex, iPhoneIndex);
        }

        memset(acTmp, 0x0, sizeof(acTmp));
        memcpy(acTmp, pstHouseInfo.cCardCount, sizeof(pstHouseInfo.cCardCount));
        cardCount = TMHexStringToDecimal(acTmp);
        for(cardIndex=0; cardIndex<TM_CARD_INFO_MAX_COUNT; cardIndex++)
        {
            TMReadICIDCardInfo(rw, &pstHouseInfo, iHouseIndex, cardIndex);
        }
        memcpy((char*)sendInfo+sizeof(pstInfo.acUserCount)+iHouseIndex*sizeof(TM_HOUSE_INFO_ST), &pstHouseInfo, sizeof(TM_HOUSE_INFO_ST));
    }
    return sendInfo;
}

static void TMCfgReadForSend(int iCMD, TM_MSG_DATA_ST*pstOut, CfgReadWrite*rw)
{
    switch(iCMD)
    {
    case TM_COMMAND_HEART:
    case TM_COMMAND_QUIT:
    case TM_COMMAND_REGISTER:
        pstOut->pvBuffer=NULL;
        pstOut->iSize = 0;
        break;
    case TM_COMMAND_REQ_DEVICE_INFO:
        pstOut->pvBuffer=TMReadDeviceInfo(rw, &pstOut->iSize);
        break;
    case TM_COMMAND_REQ_COMMUNICATE_INFO:
        pstOut->pvBuffer=TMReadCommunicateInfo(rw, &pstOut->iSize);
        break;
    case TM_COMMAND_REQ_WORK_INFO :
        pstOut->pvBuffer=TMReadWorkInfo(rw, &pstOut->iSize); 
        break;
    case TM_COMMAND_REQ_ALARM_ENABLE_INFO:
        pstOut->pvBuffer=TMReadAlarmEnable(rw, &pstOut->iSize);
        break;
    case TM_COMMAND_REQ_ALARM_ENABLE_STATE:
        pstOut->pvBuffer=(void*)YKNew(TM_REQ_ALARM_STATUS_ST, 1);
        if(g_stAlarmStatus.cFileTransferFail[0]==0)
        {
            memset(&g_stAlarmStatus, '0', sizeof(TM_REQ_ALARM_STATUS_ST));
        }
        memcpy(pstOut->pvBuffer, &g_stAlarmStatus, sizeof(TM_REQ_ALARM_STATUS_ST));
        pstOut->iSize=sizeof(TM_REQ_ALARM_STATUS_ST);
        break;
    case TM_COMMAND_REQ_USER_SERVICES_INFO:
        pstOut->pvBuffer=TMReadUserServiceInfo(rw, &pstOut->iSize);
        break;
    default:
        break;
    }
}

static void TMConfigReadFilterInit(TM_FILTER_ST* filter)
{
    TM_FILTER_ST*f=YKNew0(TM_FILTER_ST, 1);
    CfgReadWrite* read=NULL;
    f->pstSend = NULL;
    f->pstRecv = YKCreateQue(SIZE_1024);
    read = YKNew0(CfgReadWrite, 1);
    read->hCfg = (YKHandle)TMConfigNew(g_acTMConfigPath);
    read->hLock = YKCreateMutex(NULL);
    read->ReadString = (CfgQueryString)TMCfgReadString;
    read->ReadInt = TMCfgReadInt;
    read->WriteInt = TMCfgWriteInt;
    read->WriteString = TMCfgWriteString;
    read->CleanSession = TMCfgClearSession;
    f->pvData=(void*)read;
    TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID)->pstFilter=f;
}

static void TMConfigReadFilterPreprocess(TM_FILTER_ST* filter)
{

}

static void TMConfigReadFilterProcess(TM_FILTER_ST* filter)
{
    TM_MSG_DATA_ST* msg=NULL;
    CfgReadWrite* d=(CfgReadWrite*)filter->pvData;
    while(0 == YKReadQue(filter->pstRecv, &msg, 1))
    {
        if(msg->enMessageType==MESSAGE_FOR_ENCODE && msg->enCommandType<TM_COMMAND_UNKNOW)
        {
        	//Start to encode
            TMCfgReadForSend((int)msg->enCommandType, msg, d);
            YKWriteQue(filter->pstSend, msg,1);
            if(msg->iSize>0)
            {
                TMCallbackConfigStateChanged(filter, TM_CONFIG_READ, Text(TM_CONFIG_READ));
            }
            continue;
        }
        TMFreeFunc(msg);
        msg=NULL;
    }
}

static void TMConfigReadFilterPostprocess(TM_FILTER_ST* filter)
{
    CfgReadWrite* read = (CfgReadWrite*)filter->pvData;
    TMConfigSync((TM_CFG_OBJECT_ST)read->hCfg);
    TMConfigDestroy((TM_CFG_OBJECT_ST)read->hCfg);
    YKDestroyMutex(read->hLock);
    YKFree(read);
    YKDeleteQue(filter->pstRecv);
    YKFree(filter);
    TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID)->pstFilter=NULL;
}

TM_FILTER_DESC_ST g_stConfigReadFilter=
{
    TMConfigReadFilterInit,
    TMConfigReadFilterPreprocess,
    TMConfigReadFilterProcess,
    TMConfigReadFilterPostprocess,
    NULL,
    TM_FILTER_CONFIG_READ_ID,
};
/////////////////////////////////////////////////////////
static void TMTrimSpcaseAndSync(const char* pcValue, int iSize, const char* session, const char* pcKey, CfgReadWrite* rw)
{
    char acTmp[SIZE_128]={0x0};
    int iIndex=0;
    memcpy(acTmp, pcValue, iSize);
    while(acTmp[iIndex++]==' ' && iIndex <iSize);
    if(pcValue[iIndex-1]!=0x0 && iIndex-1 <iSize)
    {
        rw->WriteString(rw, session, pcKey, &acTmp[iIndex-1]);
    }
}

static BOOL TMCheckContextChanged(TM_SET_COMMUNICATE_INFO_ST* pstInfo, CfgReadWrite* rw, TM_COMMUNICATE_CHANGED_EN enID)
{
    char* pcDest=NULL;
    BOOL bChanged=FALSE;
    char temp[256]={0x0};
    switch(enID)
    {
    case TM_CONFIG_SIP_CHANGED:
    	memcpy(temp, pstInfo->acSipUsername, sizeof(pstInfo->acSipUsername));
        pcDest=(char*)rw->ReadString(rw, Text(CommunicateInfo), Text(SipUsername));
        if(NULL!=pcDest && NULL==strstr(temp, pcDest))
        {
        	bChanged=TRUE;
            break;
        }
        memset(temp, 0x0, sizeof(temp));
        memcpy(temp, pstInfo->acSipPassword, sizeof(pstInfo->acSipPassword));
        pcDest=(char*)rw->ReadString(rw, Text(CommunicateInfo), Text(SipPassword));
        if(NULL!=pcDest && NULL==strstr(temp, pcDest))
        {
            bChanged=TRUE;
            break;
        }
        memset(temp, 0x0, sizeof(temp));
        memcpy(temp, pstInfo->acImsProxyIP, sizeof(pstInfo->acImsProxyIP));
        pcDest=(char*)rw->ReadString(rw, Text(CommunicateInfo), Text(ImsProxyIP));
        if(NULL!=pcDest && NULL==strstr(temp, pcDest))
        {
            bChanged=TRUE;
            break;
        }
        memset(temp, 0x0, sizeof(temp));
        memcpy(temp, pstInfo->acImsDomain, sizeof(pstInfo->acImsDomain));
        pcDest=(char*)rw->ReadString(rw, Text(CommunicateInfo), Text(ImsDomain));
        if(NULL!=pcDest && NULL==strstr(temp, pcDest))
        {
            bChanged=TRUE;
            break;
        }
        memset(temp, 0x0, sizeof(temp));
        memcpy(temp, pstInfo->acImsPorxyPort, sizeof(pstInfo->acImsPorxyPort));
        pcDest=(char*)rw->ReadString(rw, Text(CommunicateInfo), Text(ImsPorxyPort));
        if(NULL!=pcDest && NULL==strstr(temp, pcDest))
        {
            bChanged=TRUE;
            break;
        }
        break;
    case TM_CONFIG_FTP_CHANGED:
        memset(temp, 0x0, sizeof(temp));
        memcpy(temp, pstInfo->acFtpPort, sizeof(pstInfo->acFtpPort));
        pcDest=(char*)rw->ReadString(rw, Text(CommunicateInfo), Text(FtpPort));
        if(NULL!=pcDest && NULL==strstr(temp, pcDest))
        {
            bChanged=TRUE;
            break;
        }
        memset(temp, 0x0, sizeof(temp));
        memcpy(temp, pstInfo->acFtpIp, sizeof(pstInfo->acFtpIp));
        pcDest=(char*)rw->ReadString(rw, Text(CommunicateInfo), Text(FtpIp));
        if(NULL!=pcDest && NULL==strstr(temp, pcDest))
        {
            bChanged=TRUE;
            break;
        }
        break;
    case TM_CONFIG_WAN_CHANGED:
        memset(temp, 0x0, sizeof(temp));
        memcpy(temp, pstInfo->acWanUserName, sizeof(pstInfo->acWanUserName));
        pcDest=(char*)rw->ReadString(rw, Text(CommunicateInfo), Text(WanUserName));
        if(NULL!=pcDest && NULL==strstr(temp, pcDest))
        {
            bChanged=TRUE;
            break;
        }
        memset(temp, 0x0, sizeof(temp));
        memcpy(temp, pstInfo->acWanPassword, sizeof(pstInfo->acWanPassword));
        pcDest=(char*)rw->ReadString(rw, Text(CommunicateInfo), Text(WanPassword));
        if(NULL!=pcDest && NULL==strstr(temp, pcDest))
        {
            bChanged=TRUE;
            break;
        }
        break;
    }
    return bChanged;
}

static void   TMSyncCommunicateInfo(TM_SET_COMMUNICATE_INFO_ST* pstInfo, CfgReadWrite* rw)
{
	BOOL bSipChanged=FALSE, bWanChanged=FALSE;

	bWanChanged=TMCheckContextChanged(pstInfo, rw, TM_CONFIG_WAN_CHANGED);
    TMTrimSpcaseAndSync(pstInfo->acWanUserName, sizeof(pstInfo->acWanUserName), Text(CommunicateInfo), Text(WanUserName), rw);    
    TMTrimSpcaseAndSync(pstInfo->acWanPassword, sizeof(pstInfo->acWanPassword), Text(CommunicateInfo), Text(WanPassword), rw);
    TMTrimSpcaseAndSync(pstInfo->acTr069Username, sizeof(pstInfo->acTr069Username), Text(CommunicateInfo), Text(Tr069Username), rw);
    TMTrimSpcaseAndSync(pstInfo->acTr069Password, sizeof(pstInfo->acTr069Password), Text(CommunicateInfo), Text(Tr069Password), rw);

    bSipChanged=TMCheckContextChanged(pstInfo, rw, TM_CONFIG_SIP_CHANGED);
    TMTrimSpcaseAndSync(pstInfo->acSipUsername, sizeof(pstInfo->acSipUsername), Text(CommunicateInfo), Text(SipUsername), rw);
    TMTrimSpcaseAndSync(pstInfo->acSipPassword, sizeof(pstInfo->acSipPassword), Text(CommunicateInfo), Text(SipPassword), rw);
    TMTrimSpcaseAndSync(pstInfo->acImsProxyIP, sizeof(pstInfo->acImsProxyIP), Text(CommunicateInfo), Text(ImsProxyIP), rw);
    TMTrimSpcaseAndSync(pstInfo->acImsPorxyPort, sizeof(pstInfo->acImsPorxyPort), Text(CommunicateInfo), Text(ImsPorxyPort), rw);
    TMTrimSpcaseAndSync(pstInfo->acImsDomain, sizeof(pstInfo->acImsDomain), Text(CommunicateInfo), Text(ImsDomain), rw);
    TMTrimSpcaseAndSync(pstInfo->acPlatformPort, sizeof(pstInfo->acPlatformPort), Text(CommunicateInfo), Text(PlatformPort), rw);
    TMTrimSpcaseAndSync(pstInfo->acPlatformIP, sizeof(pstInfo->acPlatformIP), Text(CommunicateInfo), Text(PlatformIP), rw);
    TMTrimSpcaseAndSync(pstInfo->acFtpPort, sizeof(pstInfo->acFtpPort), Text(CommunicateInfo), Text(FtpPort), rw);
    TMTrimSpcaseAndSync(pstInfo->acFtpIp, sizeof(pstInfo->acFtpIp), Text(CommunicateInfo), Text(FtpIp), rw);
    TMTrimSpcaseAndSync(pstInfo->acFtpUsername, sizeof(pstInfo->acFtpUsername), Text(CommunicateInfo), Text(FtpUsername), rw);
    TMTrimSpcaseAndSync(pstInfo->acFtpPassword, sizeof(pstInfo->acFtpPassword), Text(CommunicateInfo), Text(FtpPassword), rw);
    if(bWanChanged)
    {
        TMCallbackCommunicateChanged(TMQueryFilterDesc(TM_FILTER_CONFIG_WRITE_ID)->pstFilter, TM_CONFIG_WAN_CHANGED);
    }
    if(bSipChanged)
    {
        TMCallbackCommunicateChanged(TMQueryFilterDesc(TM_FILTER_CONFIG_WRITE_ID)->pstFilter, TM_CONFIG_SIP_CHANGED);
    }
 }

static void  TMSyncWorkInfo(TM_SET_WORK_INFO_ST* pstInfo, CfgReadWrite* rw)
{
    TMTrimSpcaseAndSync(pstInfo->acSipCallTimeout, sizeof(pstInfo->acSipCallTimeout), Text(WorkInfo), Text(SipCallTimeout), rw);
    TMTrimSpcaseAndSync(pstInfo->cIcOpenDoorEnable, sizeof(pstInfo->cIcOpenDoorEnable), Text(WorkInfo), Text(IcOpenDoorEnable), rw);
    TMTrimSpcaseAndSync(pstInfo->cImsCallEnable, sizeof(pstInfo->cImsCallEnable), Text(WorkInfo), Text(ImsCallEnable), rw);
    TMTrimSpcaseAndSync(pstInfo->cMonitorEnable, sizeof(pstInfo->cMonitorEnable), Text(WorkInfo), Text(MonitorEnable), rw);
    TMTrimSpcaseAndSync(pstInfo->cWisdomOpenDoorEnable, sizeof(pstInfo->cWisdomOpenDoorEnable), Text(WorkInfo), Text(WisdomOpenDoorEnable), rw);
}

static void   TMSyncAlarmEnable(TM_SET_ALARM_ENABLE_ST* pstInfo, CfgReadWrite* rw)
{
    TMTrimSpcaseAndSync(pstInfo->cFileTransferFail, sizeof(pstInfo->cFileTransferFail), Text(AlarmEnable), Text(FileTransferFail), rw);
    TMTrimSpcaseAndSync(pstInfo->cIfBoardComFail, sizeof(pstInfo->cIfBoardComFail), Text(AlarmEnable), Text(IfBoardComFail), rw);
    TMTrimSpcaseAndSync(pstInfo->cImsRegisterFail, sizeof(pstInfo->cImsRegisterFail), Text(AlarmEnable), Text(ImsRegisterFail), rw);
    TMTrimSpcaseAndSync(pstInfo->cSipCallFail, sizeof(pstInfo->cSipCallFail), Text(AlarmEnable), Text(SipCallFail), rw);
}

static void TMSyncDeviceInfo(TM_SET_DEVICE_INFO_ST* pstInfo, CfgReadWrite* rw)
{
    TMTrimSpcaseAndSync(pstInfo->acDeviceID, sizeof(pstInfo->acDeviceID), Text(DeviceInfo), Text(DeviceID), rw);
    TMTrimSpcaseAndSync(pstInfo->acManufacturer, sizeof(pstInfo->acManufacturer), Text(DeviceInfo), Text(Manufacturer), rw);
    TMTrimSpcaseAndSync(pstInfo->acOui, sizeof(pstInfo->acOui), Text(DeviceInfo), Text(Oui), rw);
    TMTrimSpcaseAndSync(pstInfo->acProductClass, sizeof(pstInfo->acProductClass), Text(DeviceInfo), Text(ProductClass), rw);
    TMTrimSpcaseAndSync(pstInfo->acSerialNumber, sizeof(pstInfo->acSerialNumber), Text(DeviceInfo), Text(SerialNumber), rw);
}

static void TMSyncHouseInfo(CfgReadWrite* rw, TM_HOUSE_INFO_ST*pstInfo, int iHouseIndex)
{
    char session[SIZE_64]={0X0};
    sprintf(session, "%s_%d", Text(HouseInfo), iHouseIndex);
    TMTrimSpcaseAndSync(pstInfo->acHouseCode, sizeof(pstInfo->acHouseCode), session, Text(HouseCode), rw);
    TMTrimSpcaseAndSync(pstInfo->acRingTime, sizeof(pstInfo->acRingTime), session, Text(RingTime), rw);
    TMTrimSpcaseAndSync(pstInfo->cInCommingcCallNotice, sizeof(pstInfo->cInCommingcCallNotice), session, Text(InCommingcCallNotice), rw);
    TMTrimSpcaseAndSync(pstInfo->acSIPCallDelay, sizeof(pstInfo->acSIPCallDelay), session, Text(SIPCallDelay), rw);
    TMTrimSpcaseAndSync(pstInfo->cPhoneCount, sizeof(pstInfo->cPhoneCount), session, Text(PhoneCount), rw);
    TMTrimSpcaseAndSync(pstInfo->cCardCount, sizeof(pstInfo->cCardCount), session, Text(CardCount), rw);
}

static void TMSyncPhoneInfo(CfgReadWrite* rw, TM_HOUSE_INFO_ST*pstInfo, int iHouseIndex, int iPhoneIndex)
{
    char session[SIZE_64]={0X0};
    int count=0;
    char acTmp[SIZE_16]={0x0};
    int notDisturbIndex=0;
    sprintf(session, "%s_%d_%d",Text(PhoneInfo), iHouseIndex, iPhoneIndex);
    TMTrimSpcaseAndSync(pstInfo->stPhoneInfo[iPhoneIndex].cVoipEnable, \
        sizeof(pstInfo->stPhoneInfo[iPhoneIndex].cVoipEnable), session, Text(VoipEnable), rw);
    TMTrimSpcaseAndSync(pstInfo->stPhoneInfo[iPhoneIndex].acCsPhoneNumber,\
        sizeof(pstInfo->stPhoneInfo[iPhoneIndex].acCsPhoneNumber), session, Text(CsPhoneNumber), rw);
    TMTrimSpcaseAndSync(pstInfo->stPhoneInfo[iPhoneIndex].acSipPhoneNumber,\
        sizeof(pstInfo->stPhoneInfo[iPhoneIndex].acSipPhoneNumber), session, Text(SipPhoneNumber), rw);
    TMTrimSpcaseAndSync(pstInfo->stPhoneInfo[iPhoneIndex].cNotDisturbCount, \
        sizeof(pstInfo->stPhoneInfo[iPhoneIndex].cNotDisturbCount),session, Text(NotDisturbCount), rw);
    memcpy(acTmp, pstInfo->stPhoneInfo[iPhoneIndex].cNotDisturbCount,\
        sizeof(pstInfo->stPhoneInfo[iPhoneIndex].cNotDisturbCount));
    for(notDisturbIndex=0; notDisturbIndex<TM_NOTDISTURBTIME_MAX_COUNT; notDisturbIndex++)
    {
        sprintf(session, "%s_%d_%d_%d", Text(NotDisturbTime), iHouseIndex, iPhoneIndex, notDisturbIndex);
        TMTrimSpcaseAndSync(pstInfo->stPhoneInfo[iPhoneIndex].stNotDisturbTime[notDisturbIndex].acTime,\
        sizeof(pstInfo->stPhoneInfo[iPhoneIndex].stNotDisturbTime[notDisturbIndex].acTime), session, Text(Time), rw);
    }
}

static void TMSyncICIDCardInfo(CfgReadWrite* rw, TM_HOUSE_INFO_ST*pstInfo, int iHouseIndex, int cardIndex)
{
    char session[SIZE_64]={0X0};
    sprintf(session, "%s_%d_%d", Text(CardCount), iHouseIndex, cardIndex);
    TMTrimSpcaseAndSync(pstInfo->stCard[cardIndex].acSerinalNumber, \
        sizeof(pstInfo->stCard[cardIndex].acSerinalNumber), session, Text(SerinalNumber), rw);
}

static BOOL TMClearHouseInfo(CfgReadWrite* rw, int iHouseIndex)
{
    char session[SIZE_64]={0X0};
    sprintf(session, "%s_%d", Text(HouseInfo), iHouseIndex);
    return rw->CleanSession(rw, session);
}

static BOOL TMCleanPhoneInfo(CfgReadWrite* rw, int iHouseIndex, int iPhoneIndex)
{
    char session[SIZE_64]={0X0};
    char acTmp[SIZE_16]={0x0};
    int count=0;
    int notDisturbIndex=0;
    sprintf(session, "%s_%d_%d",Text(PhoneInfo), iHouseIndex, iPhoneIndex);
    rw->CleanSession(rw, session);
    for(notDisturbIndex=0; notDisturbIndex<TM_NOTDISTURBTIME_MAX_COUNT; notDisturbIndex++)
    {
        sprintf(session, "%s_%d_%d_%d", Text(NotDisturbTime), iHouseIndex, iPhoneIndex, notDisturbIndex);
        rw->CleanSession(rw, session);
     }
}

static BOOL TMClearICIDCard(CfgReadWrite* rw, int iHouseIndex, int cardIndex)
{
    char session[SIZE_64]={0X0};
    sprintf(session, "%s_%d_%d", Text(CardCount), iHouseIndex, cardIndex);
}

static void TMClearOtherHouseInfos(CfgReadWrite* rw, int iStart)
{
    int iHouseIndex, iPhoneIndex=0, candIndex=0, bClearHouseInfoDone=FALSE;
    for(iHouseIndex=iStart; iHouseIndex<0xFFFFF; iHouseIndex++)
    {
        if(!TMClearHouseInfo(rw, iHouseIndex))
        {
            bClearHouseInfoDone=TRUE;
        }
        for(iPhoneIndex=0; iPhoneIndex<TM_PHONE_INFO_MAX_COUNT; iPhoneIndex++)
        {
            TMCleanPhoneInfo(rw, iHouseIndex, iPhoneIndex);
        }
        for(candIndex=0; candIndex<TM_CARD_INFO_MAX_COUNT; candIndex++)
        {
            TMClearICIDCard(rw, iHouseIndex, candIndex);
        }
        if(bClearHouseInfoDone)
        {
            break;
        }
    }
}

static void TMSyncUserServiceInfo(void* d, CfgReadWrite* rw)
{
    TM_USER_SERVICE_INFO_ST pstInfo;
    TM_HOUSE_INFO_ST stHouseInfo;

    char acTmp[sizeof(pstInfo.acUserCount)+1]={0x0};
    int iHouseIndex, iPhoneIndex, cardIndex;
    int userCount, iPhoneCount, cardCount;
    char session[SIZE_64]={0X0};
    char acKey[SIZE_64]={0x0};
    memcpy(acTmp, d, sizeof(pstInfo.acUserCount));
    TMTrimSpcaseAndSync(acTmp, sizeof(pstInfo.acUserCount), Text(UserServiceInfo), Text(UserCount), rw);
    userCount=TMHexStringToDecimal(acTmp);
    for(iHouseIndex=0; iHouseIndex<userCount; iHouseIndex++)
    {
        memset(&stHouseInfo, 0x0, sizeof(stHouseInfo));
        memcpy(&stHouseInfo, (char*)d + sizeof(pstInfo.acUserCount)+ iHouseIndex* sizeof(stHouseInfo), sizeof(stHouseInfo));
        TMSyncHouseInfo(rw, &stHouseInfo, iHouseIndex); 

        memset(acTmp, 0x0, sizeof(acTmp));
        memcpy(acTmp, stHouseInfo.cPhoneCount, sizeof(stHouseInfo.cPhoneCount));
        iPhoneCount = TMHexStringToDecimal(acTmp);
        for(iPhoneIndex=0; iPhoneIndex<TM_PHONE_INFO_MAX_COUNT; iPhoneIndex++)
        {
            TMSyncPhoneInfo(rw, &stHouseInfo, iHouseIndex, iPhoneIndex);
        }

        memset(acTmp, 0x0, sizeof(acTmp));
        memcpy(acTmp, stHouseInfo.cCardCount, sizeof(stHouseInfo.cCardCount));
        cardCount = TMHexStringToDecimal(acTmp);
        for(cardIndex=0; cardIndex<TM_CARD_INFO_MAX_COUNT; cardIndex++)
        {
            TMSyncICIDCardInfo(rw, &stHouseInfo, iHouseIndex, cardIndex);
        }
    }
    TMClearOtherHouseInfos(rw, iHouseIndex);
}

static void TMCfgSyncData(int iCMD, CfgReadWrite*rw, TM_MSG_DATA_ST* msg)
{
    switch(iCMD)
    {
    case TM_COMMAND_SET_DEVICE_INFO:
        TMSyncDeviceInfo((TM_SET_DEVICE_INFO_ST*)msg->pvBuffer,rw);
        break;
    case TM_COMMAND_SET_COMMUNICATE_INFO:
        TMSyncCommunicateInfo((TM_SET_COMMUNICATE_INFO_ST*)msg->pvBuffer, rw); 
        break;
    case TM_COMMAND_SET_WORK_INFO:
        TMSyncWorkInfo((TM_REQ_WORK_INFO_ST*)msg->pvBuffer, rw);
        break;
    case TM_COMMAND_SET_ALARM_ENABLE_INFO:
        TMSyncAlarmEnable((TM_REQ_ALARM_ENABLE_ST*)msg->pvBuffer, rw); 
        break;
    case TM_COMMAND_SET_USER_SERVICES_INFO:
        TMSyncUserServiceInfo((TM_USER_SERVICE_INFO_ST*)msg->pvBuffer, rw);
        break;
    }
    TMConfigSync((TM_CFG_OBJECT_ST)rw->hCfg);
}

static void TMConfigWriteFilterInit(TM_FILTER_ST* filter)
{
    TM_FILTER_ST*f=YKNew0(TM_FILTER_ST, 1);
    f->pstSend = NULL;
    f->pstRecv = YKCreateQue(SIZE_1024);
    f->pvData=NULL;
    TMQueryFilterDesc(TM_FILTER_CONFIG_WRITE_ID)->pstFilter=f;
}

static void TMConfigWriteFilterPreprocess(TM_FILTER_ST* filter)
{

}

static void TMConfigWriteFilterProcess(TM_FILTER_ST* filter)
{
    TM_MSG_DATA_ST* msg=NULL;
    CfgReadWrite* d=(CfgReadWrite*)filter->pvData;
    while(0 == YKReadQue(filter->pstRecv,&msg,1))
    {
        if(msg->enMessageType==MESSAGE_FOR_DECODE)
        {
            //Start to sync
            TMCfgSyncData(msg->enCommandType, d, msg);
            TMFreeFunc(msg);
            TMCallbackConfigStateChanged(filter, TM_CONFIG_WRITE, Text(TM_CONFIG_WRITE));
        }
        else
        {
            YKWriteQue(filter->pstSend, msg,1);
        }
    }
}

static void TMConfigWriteFilterPostprocess(TM_FILTER_ST* filter)
{
    YKDeleteQue(filter->pstRecv);
    YKFree(filter);
    TMQueryFilterDesc(TM_FILTER_CONFIG_WRITE_ID)->pstFilter=NULL;
}

TM_FILTER_DESC_ST g_stConfigWriteFilter=
{
    TMConfigWriteFilterInit,
    TMConfigWriteFilterPreprocess,
    TMConfigWriteFilterProcess,
    TMConfigWriteFilterPostprocess,
    NULL,
    TM_FILTER_CONFIG_WRITE_ID,
};
/////////////////////////////////////////////////////////////
typedef struct __RecvSendData
{
    YKHandle hSocket;
    YKHandle hThread;
    BOOL             bRecvThreadRun;
    BOOL             bHasSendDeviceInfo;
    TM_REGISTER_STATE_EN  RegisterState;
    TM_CONNECT_STATE_EN   ConnectState;
    YK_TIME_VAL_ST        LatestOption;
    int              SendOptionCount;
    int              SendOptionInterval;
    YK_TIME_VAL_ST        LatestSendRegister;
    int              SendRegisterInterval;
    YK_MSG_QUE_ST*  recvQue;
}RecvSendData;

#define RECV_LMT_TIMES          5
#define BEGIN_FLAG_NOT_FOUND    0
#define BEGIN_FLAG_FOUND        1

typedef struct __RecvDataCtx
{
    char buf[SIZE_1024];
    int iLen;
    char *head;
    unsigned char counter;
    unsigned char cState;
    int lens;
}RecvDataCtx;

static BOOL TMFilterQueryIsDone()
{
    RecvSendData* rs=(RecvSendData*)TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter->pvData;
    return rs->bRecvThreadRun;
}

static BOOL TMFilterNotifyDone()
{
    RecvSendData* rs=(RecvSendData*)TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter->pvData;
    return rs->bRecvThreadRun=FALSE;
}

static TM_REGISTER_STATE_EN TMQueryRegistate()
{
    RecvSendData* rs=(RecvSendData*)TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter->pvData;
    return rs->RegisterState;
}

static void TMSetRegisterState(TM_REGISTER_STATE_EN enState)
{
    TM_FILTER_ST* filter = (TM_FILTER_ST*)TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter;
    RecvSendData* rs= (RecvSendData*)filter->pvData;
    rs->RegisterState=enState;
    TMCallbackRegisterState(filter, rs->RegisterState, TMQueryRegistateContext(rs->RegisterState));
}

static void TMReConnectServerFunc()
{
    TM_FILTER_ST* filter = (TM_FILTER_ST*)TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter;
    RecvSendData* rs= (RecvSendData*)filter->pvData;
    rs->RegisterState=TM_REGISTER_UNREGISTER;
    if(rs->hSocket)
    {
        TMDisConnect(rs->hSocket);
        rs->hSocket=NULL;
        rs->ConnectState=TM_RECONNECT_SERVER_START;
        TMCallbackConnectStateChanged(filter, rs->ConnectState, Text(TM_RECONNECT_SERVER_START));
    }
} 

static int TMStringFindFirst(unsigned char* pvBuffer, int iSize, char c)
{
    int i = 0;
    for(i = 0; i < iSize; i++)
    {
        if(pvBuffer[i] == c)
        {
            return i + 1;
        }
    }
    return 0;
}

static void TMPutRecvMessage(RecvDataCtx* ctx, char* pvBuffer, int iSize, YK_MSG_QUE_ST* msg, char flag, TM_FILTER_ST* filter)
{
    char* pSend=NULL;
    int sizes;

    if(TM_PROCOTOL_START_FLAG == flag)
    {
        sizes = iSize + sizeof(char);
        pSend = YKNew0(char, sizes);
        *pSend = TM_PROCOTOL_START_FLAG;
        memcpy(pSend+1, pvBuffer, iSize);
    }
    else
    {
        sizes = iSize;
        pSend = YKNew0(char, sizes);
        memcpy(pSend, pvBuffer, iSize);
    }
    TMCallbackCommandSendRecv(filter, MESSAGE_FOR_DECODE, TM_COMMAND_UNKNOW, pSend, sizes);
    TMSendMessage(msg, pSend, sizes, MESSAGE_FOR_DECODE, TM_COMMAND_UNKNOW);
    
    ctx->lens += sizes;                    
}

static int TMGetRecvMessage(char* pRecv, YK_MSG_QUE_ST* msg)
{
    TM_MSG_DATA_ST* recv=NULL;
    int iSize = 0;
    while(0 == YKReadQue(msg,&recv,1))
    {
        memcpy(pRecv+iSize, recv->pvBuffer, recv->iSize);
        iSize += recv->iSize;
        TMFreeFunc(recv);
        recv=NULL;
    }
    return iSize;
}

BOOL TMRecvProcess(RecvDataCtx* ctx, YKHandle hHandle, YK_MSG_QUE_ST* q, TM_FILTER_ST* filter)
{
    char acBuffer[SIZE_1024] = {0};
    int recvDataLen = 0;
    int iIndex;
    int beginIndex;
    char* pSend=NULL;
    char beginFlag;

    RecvSendData*rs=(RecvSendData*)filter->pvData;

    if(ctx->counter >= SIZE_1024) 
    {
        ctx->iLen = 0;
        ctx->head = ctx->buf;
        ctx->counter = 0;
        ctx->cState = BEGIN_FLAG_NOT_FOUND;
        ctx->lens = 0;
        return TRUE;
    }
    recvDataLen = TMRecv(hHandle, acBuffer, sizeof(acBuffer), 2000);
    switch(recvDataLen)
    {
    case 0:
    	return TRUE;
    case -1:
        return FALSE;
    }
    memcpy(ctx->head, acBuffer, recvDataLen);
    ctx->iLen = recvDataLen;
    beginFlag = 0;

    iIndex = 0xFFFF;
    while((ctx->iLen > 0) && (iIndex != 0))
    {
        iIndex = TMStringFindFirst((unsigned char *)ctx->head, ctx->iLen, TM_PROCOTOL_START_FLAG);
        if(iIndex != 0) 
        {
            if(ctx->cState == BEGIN_FLAG_FOUND)
            {
                beginIndex = TMStringFindFirst((unsigned char *)ctx->head, iIndex, TM_PROCOTOL_END_FLAG);
                if(beginIndex != 0)
                {
                    ctx->counter = 0;
                    TMPutRecvMessage(ctx, ctx->head, beginIndex, rs->recvQue, beginFlag, filter);
                    pSend = YKNew0(char, ctx->lens);
                    TMGetRecvMessage(pSend, rs->recvQue);
                    TMSendMessage(q, pSend, ctx->lens, MESSAGE_FOR_DECODE, TM_COMMAND_UNKNOW);
                }
            }
            ctx->head += iIndex ;
            ctx->iLen  -= iIndex ;
            ctx->cState = BEGIN_FLAG_FOUND;
            ctx->lens = 0;
            beginFlag = TM_PROCOTOL_START_FLAG;
        }
        else
        {
            if(ctx->cState == BEGIN_FLAG_FOUND)
            {
                iIndex = TMStringFindFirst((unsigned char *)ctx->head, ctx->iLen, TM_PROCOTOL_END_FLAG);
                if(iIndex == 0)  
                {
                    ctx->counter++;
                    TMPutRecvMessage(ctx, ctx->head, ctx->iLen, rs->recvQue, beginFlag, filter);
                }
                else
                {
                    ctx->counter = 0;     //
                    TMPutRecvMessage(ctx, ctx->head, iIndex, rs->recvQue, beginFlag, filter);
                    pSend = YKNew0(char, ctx->lens);
                    TMGetRecvMessage(pSend, rs->recvQue);
                    TMSendMessage(q, pSend, ctx->lens, MESSAGE_FOR_DECODE, TM_COMMAND_UNKNOW);
                    ctx->head += iIndex; //
                    ctx->iLen  -= iIndex;
                    ctx->cState = BEGIN_FLAG_NOT_FOUND;
                    ctx->lens = 0;
                    beginFlag = 0;
                }
            }
            else
            {
                ctx->head = ctx->buf;
                ctx->iLen  = 0;
            }
        }
    }

    if(ctx->head != ctx->buf)
    {
        memcpy(ctx->buf, ctx->head, ctx->iLen);
        ctx->head = ctx->buf;
    }
    return TRUE;
}

static void* TMRecvThread(void* arg)
{
    TM_FILTER_ST* filter=(TM_FILTER_ST*)arg;
    RecvSendData* d = (RecvSendData*)filter->pvData; 
    RecvDataCtx ctx;

    char pacHost[SIZE_64]={0x0};
    int  iPort =TMQueryPlatformPort(); 
    TMQueryPlatformIp(pacHost);
    d->bRecvThreadRun=TRUE;
    d->ConnectState=TM_START_TO_CONNECT_SERVER;
    TMCallbackConnectStateChanged(filter, d->ConnectState, Text(TM_START_TO_CONNECT_SERVER));
    if(!(d->hSocket=TMConnect(pacHost, iPort, 0)))
    {
        d->ConnectState=TM_CONNECT_SERVER_FAILED;
        TMCallbackConnectStateChanged(filter, d->ConnectState, Text(TM_CONNECT_SERVER_FAILED));
    }
    else
    {
        d->ConnectState=TM_CONNECT_SERVER_SUCCESS;
        TMCallbackConnectStateChanged(filter, d->ConnectState, Text(TM_CONNECT_SERVER_SUCCESS));
    }

    memset(&ctx, 0x0, sizeof(ctx));
    ctx.head = ctx.buf;
    ctx.iLen=0;
    ctx.cState = BEGIN_FLAG_NOT_FOUND;

    while(d->bRecvThreadRun)
    {
        if(NULL== d->hSocket || !TMRecvProcess(&ctx, d->hSocket, filter->pstRecv, filter)&&d->bRecvThreadRun)
        {
            if(TM_REGISTER_QUIT==d->RegisterState)
            {
            	break;
            }
			
            if(d->hSocket)
            {
                TMDisConnect(d->hSocket);
                d->hSocket=NULL;
                d->ConnectState=TM_DISCONNECT_BY_REMOTE;
                TMCallbackConnectStateChanged(filter, d->ConnectState, Text(TM_DISCONNECT_BY_REMOTE));
                TMSetRegisterState(TM_REGISTER_PROCESS);
            }
            if(!(d->hSocket=TMConnect(pacHost, iPort, 0)))
            {
                d->ConnectState=TM_CONNECT_SERVER_FAILED;
                TMCallbackConnectStateChanged(filter, d->ConnectState, Text(TM_CONNECT_SERVER_FAILED));
                YKSleep(1000);
            }
            else
            {
                d->ConnectState=TM_CONNECT_SERVER_SUCCESS;
                TMCallbackConnectStateChanged(filter, d->ConnectState, Text(TM_CONNECT_SERVER_SUCCESS));
            }
        }
    }
	TMSystemLogOut(TM_DEBUG, "Exit Recv Command Thread.\n");
    d->bRecvThreadRun=FALSE;
    return NULL;
}

static YKHandle RestartRecvThread(TM_FILTER_ST* filter)
{
    RecvSendData* d = (RecvSendData*)filter->pvData;
    d->bRecvThreadRun=FALSE;
    if(d->hThread)
    {
        YKJoinThread(d->hThread);
        YKDestroyThread(d->hThread);
    }
    if(d->hSocket)
    {
        TMDisConnect(d->hSocket);
    }
    d->hThread=YKCreateThread(TMRecvThread, filter);
}

static void TMRecvFilterInit(TM_FILTER_ST* filter)
{
    TM_FILTER_ST*f=YKNew0(TM_FILTER_ST, 1);
    RecvSendData* d=YKNew0(RecvSendData, 1);
    f->pstSend = NULL;
    f->pstRecv = YKCreateQue(SIZE_1024);
    f->pvData = d;
    d->RegisterState = TM_REGISTER_UNREGISTER;
    d->SendOptionCount=0;
    d->SendOptionInterval=120;
    d->SendRegisterInterval=30;
    d->bHasSendDeviceInfo=FALSE;
    d->recvQue = YKCreateQue(SIZE_1024);
    TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter=f;
}

static void TMRecvFilterPreprocess(TM_FILTER_ST* filter)
{
    RecvSendData* d = (RecvSendData*)filter->pvData;
    CfgReadWrite*rw=(CfgReadWrite*)(TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID)->pstFilter->pvData);
    RestartRecvThread(filter);
}

static void TMRecvFilterProcess(TM_FILTER_ST* filter)
{
    TM_MSG_DATA_ST* msg=NULL;
    RecvSendData*rs=(RecvSendData*)filter->pvData; 
    YK_TIME_VAL_ST now;
    BOOL bHasRecvMessageForDecode=FALSE;
    while(0 == YKReadQue(filter->pstRecv,&msg,1))
    {
        if(TM_CONNECT_SERVER_SUCCESS!=rs->ConnectState || rs->RegisterState == TM_REGISTER_QUIT)
        {
            TMFreeFunc(msg);
            continue;
        }
        if(msg&&msg->enMessageType==MESSAGE_FOR_DECODE)
        {
            rs->SendOptionCount=0;
            YKGetTimeOfDay(&rs->LatestOption);
            bHasRecvMessageForDecode=TRUE;
        }
        YKWriteQue(filter->pstSend, msg,1);
    }
    if(bHasRecvMessageForDecode\
		|| TM_CONNECT_SERVER_SUCCESS!=rs->ConnectState \
		|| TM_REGISTER_QUIT == rs->RegisterState)
    {
        //socket not connected or has received message from socket. return.
        return;
    }
    YKGetTimeOfDay(&now);
    if(rs->RegisterState!=TM_REGISTER_ACCEPT)
    {
        //if device has not registered on server
        if(rs->RegisterState!=TM_REGISTER_PROCESS)
        {	
            TMSetRegisterState(TM_REGISTER_PROCESS);
            TMSendCommand(TM_COMMAND_REGISTER);
            YKGetTimeOfDay(&rs->LatestSendRegister);
        }
        else if(now.lSeconds-rs->LatestSendRegister.lSeconds>=rs->SendRegisterInterval)
        {
            TMSendCommand(TM_COMMAND_REGISTER);
            YKGetTimeOfDay(&rs->LatestSendRegister);
            TMSystemLogOut(TM_DEBUG, "Send Register After %d Seconds Not pstRecv Register Respond\n", rs->SendRegisterInterval);
        }
        return;
    }
    if(now.lSeconds-rs->LatestOption.lSeconds>=rs->SendOptionInterval)
    {
        if(rs->SendOptionCount++ < 3)
        {
            // pstSend heart option if after $(SendOptionInterval) seconds not received packet. 
            YKGetTimeOfDay(&rs->LatestOption);//reset time.
            TMSendCommand(TM_COMMAND_HEART);
            TMSystemLogOut(TM_DEBUG, "Send Heart Packet After %d Seconds Not pstRecv Packet\n", rs->SendOptionInterval);
        }
        else
        {
            //when 3 times send heart option not received respond, now start to reconnect server.
            rs->SendOptionCount=0;
            YKGetTimeOfDay(&rs->LatestOption);
            if(rs->hSocket)
            {
                // unlink by no recv option.
                TMDisConnect(rs->hSocket);
                rs->hSocket=NULL;
            }
	    TMSetRegisterState(TM_REGISTER_UNREGISTER);
            rs->ConnectState=TM_DISCONNECT_BY_NO_OPTION;
            TMCallbackConnectStateChanged(filter, rs->ConnectState, Text(TM_DISCONNECT_BY_NO_OPTION));
            TMSystemLogOut(TM_DEBUG, "When 3 Times Send Heart Option But Not received respond, now start to reconnect server\n");
        }
    }
    if(!rs->bHasSendDeviceInfo)
    {
        TMSendCommand(TM_COMMAND_REQ_DEVICE_INFO);
        rs->bHasSendDeviceInfo=TRUE;
    }
}

static void TMRecvFilterPostprocess(TM_FILTER_ST* filter)
{
    RecvSendData* d = (RecvSendData*)filter->pvData;
    d->bRecvThreadRun=FALSE;
    d->RegisterState = TM_REGISTER_QUIT;
    if(d->hSocket)
    {
        // unlink by local. exit TM module
        TMDisConnect(d->hSocket);
        d->hSocket=NULL;
        d->ConnectState=TM_DISCONNECT_BY_LOCAL;
        TMCallbackConnectStateChanged(filter, d->ConnectState, Text(TM_DISCONNECT_BY_LOCAL));
    }
    if(d->hThread)
    {
        YKJoinThread(d->hThread);
        YKDestroyThread(d->hThread);
        d->hThread=NULL;
    }
    YKDeleteQue(d->recvQue);
    YKDeleteQue(filter->pstRecv);
    YKFree(d);
    YKFree(filter);
    TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter=NULL;
}

TM_FILTER_DESC_ST g_stCommandRecvFilter=
{
    TMRecvFilterInit,
    TMRecvFilterPreprocess,
    TMRecvFilterProcess,
    TMRecvFilterPostprocess,
    NULL,
    TM_FILTER_RECV_ID,
};
///////////////////////////////////////////////////////////////
static void TMSendFilterInit(TM_FILTER_ST* filter)
{
    TM_FILTER_ST*f=YKNew0(TM_FILTER_ST, 1);
    f->pstSend = NULL;
    f->pstRecv = YKCreateQue(SIZE_1024);
    f->pvData  = NULL;
    TMQueryFilterDesc(TM_FILTER_SEND_ID)->pstFilter=f;
}

static void TMSendFilterPreprocess(TM_FILTER_ST* filter)
{

}

static void TMSendFilterProcess(TM_FILTER_ST* filter)
{
    TM_MSG_DATA_ST* msg=NULL;
    RecvSendData* d=(RecvSendData*)filter->pvData;
    while(0 == YKReadQue(filter->pstRecv,&msg ,1))
    {
        TMSend(d->hSocket, msg->pvBuffer, msg->iSize);
        TMSend(d->hSocket, "\r\n", strlen("\r\n"));
        TMCallbackCommandSendRecv(filter, MESSAGE_FOR_ENCODE, TM_COMMAND_UNKNOW, (const char*)msg->pvBuffer, msg->iSize);
        TMFreeFunc(msg);
        msg=NULL;
    }
}

static void TMSendFilterPostprocess(TM_FILTER_ST* filter)
{
    filter->pvData = NULL;
    YKDeleteQue(filter->pstRecv);
    YKFree(filter);
    TMQueryFilterDesc(TM_FILTER_SEND_ID)->pstFilter=NULL;
}

TM_FILTER_DESC_ST g_stCommandSendFilter=
{
    TMSendFilterInit,
    TMSendFilterPreprocess,
    TMSendFilterProcess,
    TMSendFilterPostprocess,
    NULL,
    TM_FILTER_SEND_ID,
};
/////////////////////////////////////////////////////////////
static void TMEncodeFilterInit(TM_FILTER_ST* filter)
{
    TM_FILTER_ST*f=YKNew0(TM_FILTER_ST, 1);
    f->pstSend = NULL;
    f->pstRecv = YKCreateQue(SIZE_1024);
    TMQueryFilterDesc(TM_FILTER_ENCODE_ID)->pstFilter=f;
}

static void TMEncodeFilterPreprocess(TM_FILTER_ST* filter)
{

}

static void TMEncodeFilterProcess(TM_FILTER_ST* filter)
{
    TM_MSG_DATA_ST* msg=NULL;
    TM_MSG_DATA_ST* send=NULL;
    RecvSendData* d=(RecvSendData*)filter->pvData;
    while(0 == YKReadQue(filter->pstRecv, &msg ,1))
    {
        if(msg->enMessageType==MESSAGE_FOR_ENCODE)
        {
            //Start to encode
            if(TMEncodePacket(msg, &send))
            {
                YKWriteQue(filter->pstSend, send,1);
            }
            TMFreeFunc(msg);
            msg=NULL;
        }
        else
        {
            YKWriteQue(filter->pstSend, msg,1);
        }
    }
}

static void TMEncodeFilterPostprocess(TM_FILTER_ST* filter)
{
    YKDeleteQue(filter->pstRecv);
    YKFree(filter);
    TMQueryFilterDesc(TM_FILTER_ENCODE_ID)->pstFilter=NULL;
}

TM_FILTER_DESC_ST g_stContextEncodeFilter=
{
    TMEncodeFilterInit,
    TMEncodeFilterPreprocess,
    TMEncodeFilterProcess,
    TMEncodeFilterPostprocess,
    NULL,
    TM_FILTER_ENCODE_ID,
};
//////////////////////////////////////////////////////////
static void TMSendRespondCommand(YK_MSG_QUE_ST*q, TM_CONTEXT_STATE_EN enState, TM_COMMAND_TYPE_EN command)
{
    char* respondBuf=YKNew0(char, 1);
    int respondSize = 1;
    *respondBuf = (enState==TM_CONTEXT_PASS)?'P':(enState==TM_CONTEXT_CRC)?'C':'E';
    TMSendMessage(q, respondBuf, respondSize, MESSAGE_FOR_ENCODE, command);
}

static void TMDecodeFilterInit(TM_FILTER_ST* filter)
{
    TM_FILTER_ST*f=YKNew0(TM_FILTER_ST, 1);
    f->pstSend = NULL;
    f->pstRecv = YKCreateQue(SIZE_1024);
    TMQueryFilterDesc(TM_FILTER_DECODE_ID)->pstFilter=f;
}

static void TMDecodeFilterPreprocess(TM_FILTER_ST* filter)
{

}

static void TMDecodeFilterProcess(TM_FILTER_ST* filter)
{
    TM_MSG_DATA_ST* msg=NULL;
    TM_MSG_DATA_ST* send=NULL;
    RecvSendData* d=(RecvSendData*)filter->pvData;
    TM_CONTEXT_STATE_EN enState;
    while(0 == YKReadQue(filter->pstRecv, &msg ,1))
    {
        if(msg->enMessageType==MESSAGE_FOR_DECODE)
        {
            //Start to decode packet.
            if(TMDecodePacket(msg, &send, &enState))
            {
                //packet decode success.
                YKWriteQue(filter->pstSend, send,1);
            }
            else
            {
                //packet decode error.
                TMSendRespondCommand(filter->pstSend, enState, msg->enCommandType);
            }
            TMFreeFunc(msg);
            msg=NULL;
        }
        else
        {
            YKWriteQue(filter->pstSend, msg,1);
        }
    }
}

static void TMDecodeFilterPostprocess(TM_FILTER_ST* filter)
{
    YKDeleteQue(filter->pstRecv);
    YKFree(filter);
    TMQueryFilterDesc(TM_FILTER_DECODE_ID)->pstFilter=NULL;
}

TM_FILTER_DESC_ST g_stContextDecodeFilter=
{
    TMDecodeFilterInit,
    TMDecodeFilterPreprocess,
    TMDecodeFilterProcess,
    TMDecodeFilterPostprocess, 
    NULL,
    TM_FILTER_DECODE_ID,
};
////////////////////////////////////////////////////////////////////
static BOOL TMContextCheckAndSend(TM_MSG_DATA_ST* pstIn, YK_MSG_QUE_ST* q)
{
    TM_COMMAND_TYPE_EN enCMD = pstIn->enCommandType;
    char* respondBuf=NULL;
    int   respondSize=0;
    TM_CONTEXT_STATE_EN  enState=TM_CONTEXT_ERROR;
    TM_REGISTER_STATE_EN registerState=TMQueryRegistate();
    if(TM_COMMAND_REGISTER==enCMD && TM_REGISTER_ACCEPT != registerState)
    {
        switch(((char*)pstIn->pvBuffer)[0])
        {
        case 'a':
        case 'A':
            TMSetRegisterState(TM_REGISTER_ACCEPT);
            break;
        case 'e':
        case 'E':
            TMSetRegisterState(TM_REGISTER_ERROR);
            break;
        case 'r':
        case 'R':
            TMSetRegisterState(TM_REGISTER_REJECT);
            break;
        default:
            TMSetRegisterState(TM_REGISTER_UNREGISTER);
            break;
        }
        return TRUE;
    }
    if(TM_REGISTER_ACCEPT != registerState)
    {
        return FALSE;
    }
    switch(enCMD)
    {
    case TM_COMMAND_QUIT:
    	TMSetRegisterState(TM_REGISTER_QUIT);
    	return TM_CONTEXT_PASS;
    case TM_COMMAND_OPTION:
        TMSendMessage(q, NULL, 0, MESSAGE_FOR_ENCODE, enCMD);
        return TM_CONTEXT_PASS;
    case TM_COMMAND_SET_DEVICE_INFO:
        enState = TMDeviceInfoCheck((TM_SET_DEVICE_INFO_ST*)pstIn->pvBuffer, pstIn->iSize); 
        break;
    case TM_COMMAND_SET_COMMUNICATE_INFO:
        enState = TMCommunicateInfoCheck((TM_SET_COMMUNICATE_INFO_ST*)pstIn->pvBuffer, pstIn->iSize);
        break;
    case TM_COMMAND_SET_WORK_INFO:
        enState = TMWorkInfoCheck((TM_SET_WORK_INFO_ST*)pstIn->pvBuffer, pstIn->iSize); 
        break;   
    case TM_COMMAND_SET_ALARM_ENABLE_INFO:
        enState = TMAlarmCheck((TM_SET_ALARM_ENABLE_ST*)pstIn->pvBuffer, pstIn->iSize);
        break;
    case TM_COMMAND_SET_SYSTEM_REBOOT:
        enState = TMSysRebootChekc((TM_SET_SYS_REBOOT_ST*)pstIn->pvBuffer, pstIn->iSize);
        TMCallbackRebootSystem(TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter, TM_REBOOT_BY_REMOTE, Text(TM_REBOOT_BY_REMOTE));
        break;      
    case TM_COMMAND_SET_USER_SERVICES_INFO:
        enState = TMUserServiceCheck((TM_USER_SERVICE_INFO_ST*)pstIn->pvBuffer, pstIn->iSize);
        break;
    case TM_COMMAND_SET_DOWNLOAD:
        enState = TMSetDownloadCheck((TM_SET_DOWNLOAD_ST*)pstIn->pvBuffer, pstIn->iSize);
        if(enState==TM_CONTEXT_PASS)
        {
        	TMCallbackRemoteTransfer(TMQueryFilterDesc(TM_FILTER_SEND_ID)->pstFilter, enCMD, pstIn->pvBuffer, pstIn->iSize);
        }
        break;
    case TM_COMMAND_SET_UPLOAD:
        enState = TMSetUploadCheck((TM_SET_UPLOAD_ST*)pstIn->pvBuffer, pstIn->iSize);
        if(enState==TM_CONTEXT_PASS)
        {
        	TMCallbackRemoteTransfer(TMQueryFilterDesc(TM_FILTER_SEND_ID)->pstFilter, enCMD, pstIn->pvBuffer, pstIn->iSize);
        }
        break;
    case TM_COMMAND_SET_TRANSFER:
        enState = TMSetTransferCheck((TM_SET_TRANSFER_ST*)pstIn->pvBuffer, pstIn->iSize);
        if(enState==TM_CONTEXT_PASS)
        {
        	TMCallbackRemoteTransfer(TMQueryFilterDesc(TM_FILTER_SEND_ID)->pstFilter, enCMD, pstIn->pvBuffer, pstIn->iSize);
        }
        break;
    case TM_COMMAND_SET_CALL_OUTGOING:
    	{
    		char outgoing[256]={0x0};
    		char phone[256]={0x0};
    		TM_OUT_GOING_CALL_INFO* info=(TM_OUT_GOING_CALL_INFO*)pstIn->pvBuffer;
    		if((enState = TMSetCallOutGoingCheck(info, pstIn->iSize))==TM_CONTEXT_PASS)
    		{
    			TMTrimNLeftSpace(phone, info->acPhoneNumber, sizeof(info->acPhoneNumber));
	    		if(info->cVoipEnable[0]=='0')
    			{
    				sprintf(outgoing, "call %s --audio-only", phone);
    			}
    			else
    			{
    				sprintf(outgoing, "call %s", phone);
    			}
	    		LPProcessCommand(outgoing);	
    		}
    	}
    	break;
    case TM_COMMAND_REQ_DEVICE_INFO:
        return TMSendCommand(TM_COMMAND_REQ_DEVICE_INFO); 
    case TM_COMMAND_REQ_COMMUNICATE_INFO:
        return TMSendCommand(TM_COMMAND_REQ_COMMUNICATE_INFO);
    case TM_COMMAND_REQ_WORK_INFO: 
        return TMSendCommand(TM_COMMAND_REQ_WORK_INFO);
    case TM_COMMAND_REQ_ALARM_ENABLE_INFO:
        return TMSendCommand(TM_COMMAND_REQ_ALARM_ENABLE_INFO);
    case TM_COMMAND_REQ_ALARM_ENABLE_STATE:
        return TMSendCommand(TM_COMMAND_REQ_ALARM_ENABLE_STATE);
    case TM_COMMAND_REQ_USER_SERVICES_INFO:
        return TMSendCommand(TM_COMMAND_REQ_USER_SERVICES_INFO);
    default:
        return FALSE;
    }
    TMSendRespondCommand(q, enState, enCMD);
    if(enState==TM_CONTEXT_PASS)
    {
        TMSendMessage(q, pstIn->pvBuffer, pstIn->iSize, MESSAGE_FOR_DECODE, enCMD);
        pstIn->pvBuffer=NULL;
        pstIn->iSize = 0;
    }
    return TM_CONTEXT_PASS==enState;
}

static void TMContextCheckFilterInit(TM_FILTER_ST* filter)
{
    TM_FILTER_ST*f=YKNew0(TM_FILTER_ST, 1);
    f->pstSend = NULL;
    f->pstRecv = YKCreateQue(SIZE_1024);
    TMQueryFilterDesc(TM_FILTER_CONTEXT_CHECK_ID)->pstFilter=f;
}

static void TMContextCheckFilterPreprocess(TM_FILTER_ST* filter)
{
}

static void TMContextCheckFilterProcess(TM_FILTER_ST* filter)
{
    TM_MSG_DATA_ST* msg=NULL;
    TM_MSG_DATA_ST* send=NULL;
    RecvSendData* d=(RecvSendData*)filter->pvData;
    while(0 == YKReadQue(filter->pstRecv, &msg, 1))
    {
        if(msg->enMessageType==MESSAGE_FOR_DECODE)
        {
            //Start to Version Check.
            TMContextCheckAndSend(msg, filter->pstSend);
            TMFreeFunc(msg);
            msg=NULL;
        }
        else
        {
            YKWriteQue(filter->pstSend, msg,1);
        }
    }
}

static void TMContextCheckFilterPostprocess(TM_FILTER_ST* filter)
{
    YKDeleteQue(filter->pstRecv);
    filter->pstRecv=NULL;
    filter->pstSend = NULL;
    YKFree(filter);
    TMQueryFilterDesc(TM_FILTER_CONTEXT_CHECK_ID)->pstFilter=NULL;
}

TM_FILTER_DESC_ST g_stContxtCheckFilter=
{
    TMContextCheckFilterInit,
    TMContextCheckFilterPreprocess,
    TMContextCheckFilterProcess,
    TMContextCheckFilterPostprocess, 
    NULL,
    TM_FILTER_CONTEXT_CHECK_ID,
};
//////////////////////////////////////////////////////////////////
//public interface
void TMFiltersInit(TM_CORE_VTABLE_ST* pstCoreVtable)
{
    TM_FILTER_DESC_ST* pstDesc=NULL;
    pstDesc=TMFindFirstFilters();
    do
    {
        if(!pstDesc)
        {
        	break;
        }
        if(pstDesc->Init)
        {
        	pstDesc->Init(pstDesc->pstFilter);
        }
    }while(pstDesc = TMFindNextFilters(pstDesc));

    TMFilterLink(TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID), TMQueryFilterDesc(TM_FILTER_RECV_ID));
    TMFilterLink(TMQueryFilterDesc(TM_FILTER_RECV_ID), TMQueryFilterDesc(TM_FILTER_DECODE_ID)); 
    TMFilterLink(TMQueryFilterDesc(TM_FILTER_DECODE_ID), TMQueryFilterDesc(TM_FILTER_CONTEXT_CHECK_ID));
    TMFilterLink(TMQueryFilterDesc(TM_FILTER_CONTEXT_CHECK_ID), TMQueryFilterDesc(TM_FILTER_ENCODE_ID)); 
    TMFilterLink(TMQueryFilterDesc(TM_FILTER_ENCODE_ID), TMQueryFilterDesc(TM_FILTER_CONFIG_WRITE_ID)); 
    TMFilterLink(TMQueryFilterDesc(TM_FILTER_CONFIG_WRITE_ID), TMQueryFilterDesc(TM_FILTER_SEND_ID));

    TMFilterShare(TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID),TMQueryFilterDesc(TM_FILTER_CONFIG_WRITE_ID));
    TMFilterShare(TMQueryFilterDesc(TM_FILTER_RECV_ID),TMQueryFilterDesc(TM_FILTER_SEND_ID));
    TMFilterShare(TMQueryFilterDesc(TM_FILTER_RECV_ID),TMQueryFilterDesc(TM_FILTER_DECODE_ID));
    TMFilterShare(TMQueryFilterDesc(TM_FILTER_RECV_ID),TMQueryFilterDesc(TM_FILTER_CONTEXT_CHECK_ID));
    TMFilterLinkVtable(pstCoreVtable);
}

void TMFiltersPreProcess()
{
    TM_FILTER_DESC_ST* pstDesc=TMFindFirstFilters();
    do
    {
        if(!pstDesc)
        {
        	break;
        }
        if(pstDesc->PreProcess)
        {
        	pstDesc->PreProcess(pstDesc->pstFilter);
        }
    }while(pstDesc = TMFindNextFilters(pstDesc));
}

void TMFiltersProcess()
{
    TM_FILTER_DESC_ST*pstDesc=TMFindFirstFilters();
    do
    {
        if(!pstDesc)
        {
        	break;
        }
        if(pstDesc->Process)
        {
        	pstDesc->Process(pstDesc->pstFilter);
        }
    }while(pstDesc = TMFindNextFilters(pstDesc));  
}

void TMFiltersPostProcess()
{
    TM_FILTER_DESC_ST*pstDesc=TMFindLastFilters();
    do
    {
        if(!pstDesc)
        {
        	break;
        }
        if(pstDesc->PostProcess)
        {
        	pstDesc->PostProcess(pstDesc->pstFilter);
        }
    }while(pstDesc = TMFindPrevFilters(pstDesc)); 
}

TM_FILTER_DESC_ST*   TMQueryFilterDesc(TM_FILTER_DESC_ID_EN enID)
{
    TM_FILTER_DESC_ST* pstDesc=NULL;
    pstDesc=TMFindFirstFilters();
    while(pstDesc)
    {
        if(pstDesc->enID==enID)
        {
            return pstDesc;
        }
        pstDesc = TMFindNextFilters(pstDesc);
    }
    return NULL;
}

BOOL TMSendCommand(TM_COMMAND_TYPE_EN enCMD)
{
    YK_MSG_QUE_ST* q=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return FALSE;
    }
    q = filter->pstFilter->pstRecv;
    TMSendMessage(q, NULL, 0, MESSAGE_FOR_ENCODE, enCMD);
    return TRUE;
}

void  TMSendAlarm(TM_ALARM_ID_EN enModule, char cValue)
{
    TM_REQ_ALARM_STATUS_ST*alarm=NULL;
    YK_MSG_QUE_ST* q=NULL;
    CfgReadWrite* rw;
    BOOL bImsAlarmEnable, bSipCallEnable, bTransferEnable, bIfboardEnable;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return ;
    }
    q=filter->pstFilter->pstRecv;
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    bImsAlarmEnable=rw->ReadInt(rw, Text(AlarmEnable), Text(ImsRegisterFail));
    bIfboardEnable=rw->ReadInt(rw, Text(AlarmEnable), Text(IfBoardComFail));
    bTransferEnable=rw->ReadInt(rw, Text(AlarmEnable), Text(FileTransferFail));
    bSipCallEnable=rw->ReadInt(rw, Text(AlarmEnable), Text(SipCallFail));
    if(g_stAlarmStatus.cFileTransferFail[0]==0)
    {
        memset(&g_stAlarmStatus, '0', sizeof(TM_REQ_ALARM_STATUS_ST));
    }
    switch(enModule)
    {
    case TM_ALARM_IMS_REGISTER:
        if(bImsAlarmEnable && cValue!=g_stAlarmStatus.cImsRegisterFail[0])
        {
        	alarm=YKNew0(TM_REQ_ALARM_STATUS_ST, 1);
        	memcpy(alarm, &g_stAlarmStatus, sizeof(TM_REQ_ALARM_STATUS_ST));
            alarm->cImsRegisterFail[0]=g_stAlarmStatus.cImsRegisterFail[0]=cValue;
        }
        break;
    case TM_ALARM_SIP_CALL: 
        if(bSipCallEnable && cValue != g_stAlarmStatus.cSipCallFail[0])
        {
        	alarm=YKNew0(TM_REQ_ALARM_STATUS_ST, 1);
        	memcpy(alarm, &g_stAlarmStatus, sizeof(TM_REQ_ALARM_STATUS_ST));
            alarm->cSipCallFail[0]=g_stAlarmStatus.cSipCallFail[0]=cValue;
        }
        break;
    case TM_ALARM_IF_BOARD:
        if(bIfboardEnable && cValue != g_stAlarmStatus.cIfBoardComFail[0])
        {
        	alarm=YKNew0(TM_REQ_ALARM_STATUS_ST, 1);
        	memcpy(alarm, &g_stAlarmStatus, sizeof(TM_REQ_ALARM_STATUS_ST));
            alarm->cIfBoardComFail[0]=g_stAlarmStatus.cIfBoardComFail[0]=cValue;
        }
        break;
    case TM_ALARM_TRANSFER:
        if(bTransferEnable && cValue != g_stAlarmStatus.cFileTransferFail[0])
        {
        	alarm=YKNew0(TM_REQ_ALARM_STATUS_ST, 1);
        	memcpy(alarm, &g_stAlarmStatus, sizeof(TM_REQ_ALARM_STATUS_ST));
            alarm->cFileTransferFail[0]=g_stAlarmStatus.cFileTransferFail[0]=cValue;
        }
        break;
    default:
        break;
    }
    if(NULL!=alarm && (bImsAlarmEnable||bSipCallEnable||bIfboardEnable||bTransferEnable))
    {
    	TMSendMessage(q, alarm, sizeof(TM_REQ_ALARM_STATUS_ST), MESSAGE_FOR_ENCODE, TM_COMMAND_RES_ALARM);
    }
}

void  TMQuerySipInfo(char* pcUserName, char* pcPassWord, char*pcProxy, int* piPort, char* pcDomain)
{
	CfgReadWrite* rw=NULL;
	TM_CFG_OBJECT_ST obj=NULL;
    char* pcValue=NULL;
	static int iFirstLoad = 1;

	if(iFirstLoad)
	{
		iFirstLoad = 0;
		obj=TMConfigNew(TM_CONFIG_PATH);
		if(obj)
		{
			if(pcUserName && (pcValue=TMConfigGetString(obj, Text(CommunicateInfo), Text(SipUsername), NULL)))
			{
				strcpy(pcUserName, pcValue);
			}
			if(pcPassWord && (pcValue=TMConfigGetString(obj, Text(CommunicateInfo), Text(SipPassword), NULL)))
			{
				strcpy(pcPassWord, pcValue);
			}
			if(pcProxy && (pcValue=TMConfigGetString(obj, Text(CommunicateInfo), Text(ImsProxyIP), NULL)))
			{
				strcpy(pcProxy, pcValue);
			}
			if(piPort)
			{
				*piPort = TMConfigGetInt(obj, Text(CommunicateInfo), Text(ImsPorxyPort), NULL);
			}
			if(pcDomain && (pcValue=TMConfigGetString(obj, Text(CommunicateInfo), Text(ImsDomain), NULL)))
			{
				strcpy(pcDomain, pcValue);
			}
			TMConfigDestroy(obj);
		}
		return;
	}

    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return ;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    if(pcUserName&&(pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(SipUsername))))
    {
        strcpy(pcUserName, pcValue);
    }
    if(pcPassWord && (pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(SipPassword))))
    {
        strcpy(pcPassWord, pcValue);
    }
    if(pcProxy && (pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(ImsProxyIP))))
    {
        strcpy(pcProxy, pcValue);
    }
    if(pcDomain && (pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(ImsDomain))))
    {
        strcpy(pcDomain, pcValue);
    }
    if(piPort)
    {
        *piPort=rw->ReadInt(rw, Text(CommunicateInfo), Text(ImsPorxyPort));
    }
}

void TMQueryWanInfo(char* pcUserName, char* pcPassWord)
{
    CfgReadWrite* rw=NULL;
    TM_CFG_OBJECT_ST obj=NULL;
    char* pcValue=NULL;
    static int iFirstLoad = 1;

    if(iFirstLoad)
    {
    	iFirstLoad = 0;
    	obj=TMConfigNew(TM_CONFIG_PATH);
		if(obj)
		{
			if(pcUserName && (pcValue=TMConfigGetString(obj, Text(CommunicateInfo), Text(WanUserName), NULL)))
			{
				strcpy(pcUserName, pcValue);
			}
			if(pcPassWord && (pcValue=TMConfigGetString(obj, Text(CommunicateInfo), Text(WanPassword), NULL)))
			{
				strcpy(pcPassWord, pcValue);
			}
			TMConfigDestroy(obj);
		}
		return;
    }

    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return ;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    if(pcUserName && (pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(WanUserName))))
    {
        strcpy(pcUserName, pcValue);
    }
    if(pcPassWord && (pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(WanPassword))))
    {
        strcpy(pcPassWord, pcValue);
    }
}

void    TMQueryFtpInfo(char* pcHost, int*piPort, char* pcUserName, char* pcPassWord)
{
	TM_CFG_OBJECT_ST obj=NULL;
	char* pcValue=NULL;

	obj=TMConfigNew(TM_CONFIG_PATH);
	if(obj)
	{
		if(pcUserName && (pcValue=TMConfigGetString(obj, Text(CommunicateInfo), Text(FtpUsername), NULL)))
		{
			strcpy(pcUserName, pcValue);
		}
		if(pcPassWord && (pcValue=TMConfigGetString(obj, Text(CommunicateInfo), Text(FtpPassword), NULL)))
		{
			strcpy(pcPassWord, pcValue);
		}
		if(pcHost && (pcValue=TMConfigGetString(obj, Text(CommunicateInfo), Text(FtpIp), NULL)))
		{
			strcpy(pcHost, pcValue);
		}
		if(piPort)
		{
			*piPort = TMConfigGetInt(obj, Text(CommunicateInfo), Text(FtpPort), NULL);
		}
		TMConfigDestroy(obj);
	}


//	TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
//	if(!filter)
//	{
//		return ;
//	}
//	rw=(CfgReadWrite*)filter->pstFilter->pvData;
//	if(NULL!=pcHost && (pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(FtpIp))))
//	{
//		strcpy(pcHost, pcValue);
//	}
//	if(piPort)*piPort=rw->ReadInt(rw, Text(CommunicateInfo), Text(FtpPort));
//	if(pcUserName && (pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(FtpUsername))))
//	{
//		strcpy(pcUserName, pcValue);
//	}
//	if(pcPassWord && (pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(FtpPassword))))
//	{
//		strcpy(pcPassWord, pcValue);
//	}
}

void    TMQueryAppVersion(char* pcVersion)
{
	if(NULL==pcVersion)
	{
		return;
	}
    strcpy(pcVersion, g_acTMFirmVersion);
}

void TMQueryPlatformIp(char* pcIP)
{
    CfgReadWrite* rw=NULL;
    char* pcValue=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return ;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    if(pcIP && (pcValue=rw->ReadString(rw, Text(CommunicateInfo), Text(PlatformIP))))
    {
        strcpy(pcIP, pcValue);
    }
}

int  TMQueryPlatformPort()
{
    CfgReadWrite* rw=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return 0;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    return rw->ReadInt(rw, Text(CommunicateInfo), Text(PlatformPort));
}

void TMQueryDeviceID(char* pcID)
{
	TM_CFG_OBJECT_ST obj=NULL;
	char* value=NULL;
	if(g_pcDeviceID[0] != 0x0)
	{
		strcpy(pcID, g_pcDeviceID);
		return;
	}
	obj=TMConfigNew(TM_CONFIG_PATH);
	if(obj)
	{
		if(pcID && (value=TMConfigGetString(obj, Text(DeviceInfo), Text(DeviceID), NULL)))
		{
			strcpy(g_pcDeviceID, value);
			strcpy(pcID, g_pcDeviceID);
		}
		TMConfigDestroy(obj);
	}
}

BOOL TMInitTerminateManage(const char* pcVersion, const char* pcConfig)
{
    if(NULL==pcVersion || NULL==pcConfig)
    {
        return FALSE;
    }
    if((access(pcConfig, 0/*file is exist*/)!=0 && access(pcConfig, 2/*write permission*/)!=0)  || 
        strlen(pcVersion)>=sizeof(g_acTMFirmVersion) || strlen(pcConfig)>=sizeof(g_acTMConfigPath))
    {
            return FALSE;
    }
    memset(g_acTMFirmVersion, 0x0, sizeof(g_acTMFirmVersion));
    memset(g_acTMConfigPath, 0x0, sizeof(g_acTMConfigPath));
    memcpy(g_acTMFirmVersion, pcVersion, sizeof(g_acTMFirmVersion));
    memcpy(g_acTMConfigPath, pcConfig, sizeof(g_acTMConfigPath));
    return TRUE;
}

/**@brief Query house pstInfo.*/
int     TMQueryHouseCount()
{
    TM_USER_SERVICE_INFO_ST pstInfo;
    char acTmp[sizeof(pstInfo.acUserCount)+1]={0x0};
    CfgReadWrite* rw=NULL;
    char* pcValue=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return 0;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    TMFillRule(acTmp, rw->ReadString(rw, Text(UserServiceInfo), Text(UserCount))," ",sizeof(pstInfo.acUserCount));
    return TMHexStringToDecimal(acTmp);
}

BOOL     TMQueryHouseInfo(TM_HOUSE_INFO_ST* pstHouse, int iHouseIndex)
{
    int iPhoneCount,cardCount, cardIndex, iPhoneIndex;
    char acTmp[SIZE_32]={0X0};
    CfgReadWrite* rw=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return FALSE;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    if(NULL==pstHouse)
    {
        return FALSE; 
    }
    memset(pstHouse, 0x0, sizeof(TM_HOUSE_INFO_ST));
    TMReadHouseInfo(rw, pstHouse, iHouseIndex); 

    memset(acTmp, 0x0, sizeof(acTmp));
    memcpy(acTmp, pstHouse->cPhoneCount, sizeof(pstHouse->cPhoneCount));
    iPhoneCount = TMHexStringToDecimal(acTmp);
    for(iPhoneIndex=0; iPhoneIndex<iPhoneCount; iPhoneIndex++)
    {
        TMReadPhoneInfo(rw, pstHouse, iHouseIndex, iPhoneIndex);
    }

    memset(acTmp, 0x0, sizeof(acTmp));
    memcpy(acTmp, pstHouse->cCardCount, sizeof(pstHouse->cCardCount));
    cardCount = TMHexStringToDecimal(acTmp);
    for(cardIndex=0; cardIndex<cardCount; cardIndex++)
    {
        TMReadICIDCardInfo(rw, pstHouse, iHouseIndex, cardIndex);
    }
    return TRUE;
}

BOOL     TMQueryHouseByHouseCode(TM_HOUSE_INFO_ST* pstHouse, const char* pcHouseCode)
{
    TM_USER_SERVICE_INFO_ST pstInfo;
    char acTmp[SIZE_32]={0x0};
    char session[SIZE_32]={0x0};
    int iHouseIndex, userCount,iHouseCode,iHouse;
    CfgReadWrite* rw=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return FALSE;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    TMFillRule(acTmp, rw->ReadString(rw, Text(UserServiceInfo), Text(UserCount))," ",sizeof(pstInfo.acUserCount));
    userCount=TMHexStringToDecimal(acTmp);
    for(iHouseIndex=0; iHouseIndex<userCount; iHouseIndex++)
    {
        sprintf(session, "%s_%d", Text(HouseInfo), iHouseIndex);
        iHouse = atoi(rw->ReadString(rw, session, Text(HouseCode)));
        iHouseCode = atoi(pcHouseCode);
        if(iHouse == iHouseCode)
        {
            return TMQueryHouseInfo(pstHouse, iHouseIndex);
        }
    }
    return FALSE;
}

/**@brief Query phone pstInfo.*/
int     TMQueryPhoneCount(TM_HOUSE_INFO_ST* pstHouse)
{
    char acTmp[SIZE_32]={0x0};
    if(NULL==pstHouse)
    {
        return 0;
    }
    memcpy(acTmp, pstHouse->cPhoneCount, sizeof(pstHouse->cPhoneCount));
    return TMHexStringToDecimal(acTmp);
}

TM_PHONE_INFO_ST*  TMQueryPhoneInfo(TM_HOUSE_INFO_ST* pstHouse, int iPhoneIndex)
{
    if(NULL==pstHouse||iPhoneIndex>=TMQueryPhoneCount(pstHouse)||iPhoneIndex<0)
    {
        return NULL;
    }
    return &pstHouse->stPhoneInfo[iPhoneIndex];
}
/**@brief Query not disturb time.*/
int  TMQueryNotDisturbTimeCount(TM_PHONE_INFO_ST* pstPhone)
{
    char acTmp[SIZE_32]={0x0};
    if(NULL==pstPhone)
    {
        return 0;
    }
    memcpy(acTmp, pstPhone->cNotDisturbCount, sizeof(pstPhone->cNotDisturbCount));
    return TMHexStringToDecimal(acTmp);
}

BOOL    TMQueryNotDisturbTime(char* disturb, TM_PHONE_INFO_ST* pstPhone, int iIndex)
{
    int i=0;
    char time[SIZE_32]={0X0};
    int iSize=0;
    if(NULL==disturb || NULL==pstPhone || iIndex>=TMQueryNotDisturbTimeCount(pstPhone))
    {
        return FALSE;
    }
    iSize=sizeof(pstPhone->stNotDisturbTime[iIndex].acTime);
    TMTrimNLeftSpace(disturb, pstPhone->stNotDisturbTime[iIndex].acTime, iSize);
    return TRUE;
}

BOOL TMIsNotDisturbTime(TM_PHONE_INFO_ST* pstPhone)
{
    int disturbTimeIndex=0;
    int disturbCount=TMQueryNotDisturbTimeCount(pstPhone);
    char time[SIZE_16]={0x0};
    char acTmp[SIZE_16] = {0x0};
    int currtime;
    int begin=0, end=0;
    YK_CURTIME_ST stCurtime;
    YKGetCurrtime(&stCurtime);
    currtime = stCurtime.iHour*100+stCurtime.iMin;
    for(disturbTimeIndex=0; disturbTimeIndex<disturbCount; disturbTimeIndex++)
    {
        if(TMQueryNotDisturbTime(time, pstPhone, disturbTimeIndex))
        {
            memcpy(acTmp, time, 4);
            begin=atoi(acTmp);
            memset(acTmp, 0x0, sizeof(acTmp));
            memcpy(acTmp, time+4, 4);
            end = atoi(acTmp);
            if(currtime>begin && currtime < end)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/**@brief Query IC/ID card.*/
int     TMQueryICIDCardCount(TM_HOUSE_INFO_ST* pstHouse)
{
    char acTmp[SIZE_32]={0x0};
    if(NULL==pstHouse)
    {
        return 0;
    }
    memcpy(acTmp, pstHouse->cCardCount, sizeof(pstHouse->cCardCount));
    return TMHexStringToDecimal(acTmp);
}

BOOL    TMQueryICIDCardInfo(char* pcSerialNumber, TM_HOUSE_INFO_ST* pstHouse, int iIndex)
{
    int i=0;
    if(NULL==pcSerialNumber||NULL==pstHouse)
    {
        return FALSE;
    }
    TMTrimNLeftSpace(pcSerialNumber, pstHouse->stCard[iIndex].acSerinalNumber, sizeof(pstHouse->stCard[iIndex].acSerinalNumber));
    return TRUE;
}

int TMQueryCallTimeOut()
{
    CfgReadWrite* rw=NULL;
    char acValue[8]={0x0};
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return 0;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    strcpy(acValue, rw->ReadString(rw, Text(WorkInfo), Text(SipCallTimeout)));
    return TMHexStringToDecimal(acValue);
}

BOOL TMQueryIMSIsEnabled()
{
    CfgReadWrite* rw=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return 0;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    return rw->ReadInt(rw, Text(WorkInfo), Text(ImsCallEnable));
}

BOOL TMQueryKeyOpenDoorEnabled()
{
    CfgReadWrite* rw=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return 0;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    return rw->ReadInt(rw, Text(WorkInfo), Text(WisdomOpenDoorEnable));
}

BOOL TMQueryMonitorEnabled()
{
    CfgReadWrite* rw=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return 0;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    return rw->ReadInt(rw, Text(WorkInfo), Text(MonitorEnable));
}

BOOL TMQueryICOpenDoorEnabled()
{
    CfgReadWrite* rw=NULL;
    TM_FILTER_DESC_ST* filter=TMQueryFilterDesc(TM_FILTER_CONFIG_READ_ID);
    if(!filter)
    {
        return 0;
    }
    rw=(CfgReadWrite*)filter->pstFilter->pvData;
    return rw->ReadInt(rw, Text(WorkInfo), Text(IcOpenDoorEnable));
}

void   TMSendTransfer(const char* pcLocal, const char*pcRemote, char cType)
{
    TM_SET_TRANSFER_ST* transfer=YKNew0(TM_SET_TRANSFER_ST, 1);
    TM_MSG_DATA_ST* pstOut=NULL;
    TM_MSG_DATA_ST *pstData=NULL;
    YK_MSG_QUE_ST* q=NULL;
    if(NULL==transfer)
    {
    	return;
    }
    pstData = YKNew0(TM_MSG_DATA_ST, 1);
    if(NULL==pstData)
    {
    	free(transfer);
    }
    q = TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter->pstRecv;
    TMFillRule(transfer->acLocalUrl, pcLocal, " ", sizeof(transfer->acLocalUrl));
    TMFillRule(transfer->acRemoteUrl, pcRemote, " ", sizeof(transfer->acRemoteUrl));
    transfer->cType[0]=cType;
    pstData->pvBuffer = transfer;
    pstData->iSize= sizeof(TM_SET_TRANSFER_ST);
    pstData->enMessageType = MESSAGE_FOR_DECODE;
    pstData->enCommandType = TM_COMMAND_SET_TRANSFER; 
    TMEncodePacket(pstData, &pstOut);
    TMFreeFunc(pstData);
    YKWriteQue(q, pstOut,1);
}

void SendDecodePacket(char* pcBuffer, int iSize)
{
    YK_MSG_QUE_ST* q = TMQueryFilterDesc(TM_FILTER_RECV_ID)->pstFilter->pstRecv;
    TM_MSG_DATA_ST *pstData = YKNew0(TM_MSG_DATA_ST, 1);
    pstData->pvBuffer=pcBuffer;
    pstData->iSize=iSize;
    pstData->enMessageType = MESSAGE_FOR_DECODE;
    pstData->enCommandType = TM_COMMAND_SET_USER_SERVICES_INFO;
    YKWriteQue(q, pstData,1);
}

#endif
