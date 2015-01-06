/*
 * TMTr069.c
 *
 *  Created on: 2012-5-20
 *      Author: root
 */

#include <YKApi.h>
#include <IDBT.h>
#include <TMInterface.h>
#include <IDBTIntfLayer.h>
#include <LOGIntfLayer.h>

#if _TM_TYPE_ == _TR069_
//#include "../../../INC/TMInterface.h"
#include "evcpe.h"
#include "class_xml.h"
#include "obj_xml.h"
#include "attr_xml.h"
#include "repo.h"
#include "fault.h"
#include "accessor.h"
#include <Des.h>
#include "cpe.h"
#include "sd/domnode.h"
#include <GI6Interface.h>
#include <https.h>


static struct evbuffer *pstBuf;
static struct evcpe_class *pstCls;
static struct evcpe_obj *pstObj;
static struct evcpe_repo *pstRepo;
struct evcpe_inform *pstInform;
char g_acTRFirmVersion[SIZE_64];
char g_acTRHardwareVersion[SIZE_64];
char g_acTRKernelVersion[SIZE_64];
extern EVCPE_TEMP_ST g_stTemp ;

#define BUF_SIZE 256
char pStrTmp[BUF_SIZE] = {0x0};

extern int evcpe_persister_persist(struct evcpe_persister *persist);

#endif


static volatile LINK_STATUS_EN g_trCurrLink = LINK_STATUS_OFFLINE;     //tr注册状态
static volatile LINK_STATUS_EN g_trOldLink = -1;                      //tr上一次注册状态

int TMMsgSendTRABRegStatus(LINK_STATUS_EN enRegStatus)
{
	LOG_RUNLOG_WARN("TM TMMsgSendTRABRegStatus");

	int iRet = -1;
	if(NULL == g_pstABMsgQ)
	{
		LOG_RUNLOG_WARN("TM g_pstABMsgQ is null");
		return -1;
	}
	SIPAB_REG_STATUS_ST* pstMsg= (TMAB_TM_LINK_STATUS_ST*)malloc(sizeof(TMAB_TM_LINK_STATUS_ST));
	if(pstMsg == NULL)
	{
		LOG_RUNLOG_ERROR("TM malloc TMMsgSendTRABRegStation failed");
		return -1;
	}
	memset(pstMsg, 0x00, sizeof(TMAB_TM_LINK_STATUS_ST));

	pstMsg->uiRegStatus = (unsigned int)enRegStatus;

	pstMsg->uiPrmvType = TMAB_TM_LINK_STATUS;

	LOG_RUNLOG_WARN("TM TMMsgSendTRABRegStatus &&&&&1");
	iRet = YKWriteQue( g_pstABMsgQ,  pstMsg,  TM_QUE_WRITE_TIMEOUT);

	LOG_RUNLOG_DEBUG("TM TMMsgQ send msg:\nuiPrmvType:TMAB_TM_LINK_STATUS | 0x%04x\n", TMAB_TM_LINK_STATUS);

	return iRet;
}

void TMSetLinkStatus(LINK_STATUS_EN enStatus)
{
	g_trCurrLink = enStatus;

	if(g_trOldLink != g_trCurrLink)
	{
		LOG_RUNLOG_DEBUG("TM Link status changed(old:%d new:%d)", g_trOldLink, g_trCurrLink);

		g_trOldLink = g_trCurrLink;

		TMMsgSendTRABRegStatus(g_trCurrLink);

	}
}

#if _TM_TYPE_ == _TR069_

int TMHexStringToDecimal(const char* pcHex){
    char* pcTmp = NULL;
    return strtol(pcHex, &pcTmp, 16);
}

void TMTrimNLeftSpace(char* pcDest, const char* pcValue, int iSize){
    int iIndex=0;
    while(pcValue[iIndex++]==' ' && iIndex <iSize);
    if(pcValue[iIndex-1]!=0x0 && iIndex-1 <iSize){
        memcpy(pcDest, &pcValue[iIndex-1], iSize-iIndex+1);
    }
}

void TMXmlLoad(const char *filename)
{
	FILE *hFile;
	int iFd,iLen;

	hFile = fopen(filename, "r");
	iFd = fileno(hFile);
	evbuffer_drain(pstBuf, EVBUFFER_LENGTH(pstBuf));
	while ((iLen = evbuffer_read(pstBuf, iFd, -1)));
	fclose(hFile);
}

void TMXmlSetup(char *pcConfig,char *pcModel)
{
	char acCommandLine[512]={0x0};
	char acDataName[128];
	char* pDataName;
	pstBuf = evbuffer_new();
	pstCls = evcpe_class_new(NULL);
	pstInform = evcpe_inform_new();
	TMXmlLoad(pcModel);
	evcpe_class_from_xml(pstCls, pstBuf);
	pstObj = evcpe_obj_new(pstCls, NULL);
	evcpe_obj_init(pstObj);
	TMXmlLoad(pcConfig);
	//--------------add by zlin
	/*重新加载data备份配置文件*/
	if (pstBuf->off <= 1000) {
		strcpy(acDataName,pcConfig);
		pDataName = strtok(acDataName,".");
		sprintf(acCommandLine, "busybox cp -rf %s_bak.xml %s\n", pDataName, pcConfig);
	    //sprintf(acCommandLine, "cp -rf %s %s\n", TM_DATA_BACKUP_PATH, TM_DATA_PATH);
	    system(acCommandLine);
		TMXmlLoad(pcConfig);
	}
	//---------------
	evcpe_obj_from_xml(pstObj, pstBuf);
	pstRepo = evcpe_repo_new(pstObj);
}

void TMGetUrl(char *pcHost)
{
    const char *value;
    char acHost[128];
    unsigned int len;
    int i;

    if(!g_tr_evcpe_st.repo)
    {
    	return;
    }

    evcpe_repo_get(g_tr_evcpe_st.repo, ".ManagementServer.URL", &value, &len);
    if(len > 0)
	{
    	strcpy(acHost,value);
    	mystrstr(acHost,"acsServlet",&i);
		memcpy(pcHost,acHost,i);
		pcHost[i] = 0x0;
		strcat(pcHost+i,"imsServlet");
	}
}

void TMGetHost(char *pcHost,int *port)
{
    const char *value;
    char acHost[128];
    char *host;
    unsigned int len;
    char *pA;
    char *pB;
    char acWeb[64];
    int flag = 0;


    if(!pcHost)
    {
    	return;
    }

	if(!pstRepo)
	{
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
		TMXmlSetup(TM_DATA_RUNNING,TM_MODEL_PATH);
#else
		TMXmlSetup(TM_DATA_PATH,TM_MODEL_PATH);
#endif
	}

    evcpe_repo_get(pstRepo, ".ManagementServer.URL", &value, &len);

    if (!(*pcHost))
    {
        return;
    }
    strcpy(acHost,value);
    pA = acHost;
    if (!strncmp(pA, "http://", strlen("http://")))
    {
        pA = acHost + strlen("http://");
		flag = 0;
    }
    else if (!strncmp(pA, "https://", strlen("https://")))
    {
        pA = acHost + strlen("https://");
		flag = 1;
    }
    else
    {
    	return;
    }
    pB = strchr(pA, '/');
    if (pB)
    {
        strcpy(acWeb, pA);
        acWeb[strlen(pA) - strlen(pB)] = 0x0;
    }
    else
    {
        strcpy(acWeb, pA);
    }

    pA = strchr(acWeb,':');
    if(pA)
	{
		strcpy(pcHost,acWeb);
		pcHost[strlen(pcHost) - strlen(pA)] = 0x0;
		if(port)
		{
			*port = atoi(pA + 1);
		}
	}
    else
    {
    	strcpy(pcHost,acWeb);
    	if(flag == 1)
    	{
    		if(port)
    		{
    			*port = 443;
    		}
    	}
    	else
    	{
    		if(port)
    		{
                *port = 80;
    		}
    	}
    }

    return ;
}

/**
 *@name For CC Module
 *@{
 */
void TMReadHouseInfo(TM_HOUSE_INFO_ST*pstInfo, int iHouseIndex){
    char session[128]={0X0};
    const char *value;
	unsigned int len;
	int ringtime;

	if(pstInfo == NULL || iHouseIndex < 0)
	{
		return;
	}

	sprintf(session, ".Business.UserInfo.%d.Num",iHouseIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value == NULL || strcmp(value,"0")== 0)
	{
		return;
	}
	sprintf(session, ".Business.UserInfo.%d.BuildingNum",iHouseIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		strcpy(pstInfo->acBuildingNum,value);
	}

	sprintf(session, ".Business.UserInfo.%d.UnitNum",iHouseIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		strcpy(pstInfo->acUnitNum,value);
	}

    sprintf(session, ".Business.UserInfo.%d.RoomNumber",iHouseIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		strcpy(pstInfo->acHouseCode,value);
	}

	sprintf(session, ".Business.UserInfo.%d.RingingTime",iHouseIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		ringtime = atoi(value);
		sprintf(pstInfo->acRingTime,"%02x",ringtime);
	}

	sprintf(session, ".Business.UserInfo.%d.IncomingCallNotice",iHouseIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		strcpy(pstInfo->cInCommingcCallNotice,value);
	}

	sprintf(session, ".Business.UserInfo.%d.TimeDelay",iHouseIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		ringtime = atoi(value);
		sprintf(pstInfo->acSIPCallDelay,"%02x",ringtime);
	}

	sprintf(session, ".Business.UserInfo.%d.PhoneCount",iHouseIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		sscanf(value,"%d",&pstInfo->iPhoneCount);
		if(pstInfo->iPhoneCount > TM_PHONE_INFO_MAX_COUNT)
		{
			pstInfo->iPhoneCount = TM_PHONE_INFO_MAX_COUNT;
		}
	}

	sprintf(session, ".Business.UserInfo.%d.CardCount",iHouseIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		sscanf(value,"%d",&pstInfo->iCardCount);
		if(pstInfo->iCardCount > TM_CARD_INFO_MAX_COUNT)
		{
			pstInfo->iCardCount = TM_CARD_INFO_MAX_COUNT;
		}
	}
}

void TMReadPhoneInfo(TM_HOUSE_INFO_ST*pstInfo, int iHouseIndex, int iPhoneIndex){
    char session[128]={0X0};
    const char *value;
    unsigned int len;

    if(pstInfo == NULL || iHouseIndex < 0|| iPhoneIndex < 0)
    {
    	return;
    }

    sprintf(session, ".Business.UserInfo.%d.PhoneInfo.%d.Num",iHouseIndex+1,iPhoneIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value == NULL || strcmp(value,"0")== 0)
	{
		return;
	}
	sprintf(session, ".Business.UserInfo.%d.PhoneInfo.%d.VoipEn",iHouseIndex+1,iPhoneIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		strcpy(pstInfo->stPhoneInfo[pstInfo->iPhoneCount].cVoipEnable,value);
	}

	sprintf(session, ".Business.UserInfo.%d.PhoneInfo.%d.CsPhoneNum",iHouseIndex+1,iPhoneIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		strcpy(pstInfo->stPhoneInfo[pstInfo->iPhoneCount].acCsPhoneNumber,value);
	}

	sprintf(session, ".Business.UserInfo.%d.PhoneInfo.%d.SipAccount",iHouseIndex+1,iPhoneIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		strcpy(pstInfo->stPhoneInfo[pstInfo->iPhoneCount].acSipPhoneNumber,value);
	}

	sprintf(session, ".Business.UserInfo.%d.PhoneInfo.%d.TimeNotDisturb",iHouseIndex+1,iPhoneIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		strcpy(pstInfo->stPhoneInfo[pstInfo->iPhoneCount].acNotDisturbTime,value);
	}

	pstInfo->iPhoneCount ++;
}

static void TMReadICIDCardInfo(TM_HOUSE_INFO_ST*pstInfo, int iHouseIndex, int cardIndex){
    char session[128]={0X0};
    const char *value;
	unsigned int len;

	if(!g_tr_evcpe_st.repo)
	{
		return;
	}

	if(pstInfo == NULL || iHouseIndex < 0|| cardIndex < 0)
	{
		return;
	}

	sprintf(session, ".Business.UserInfo.%d.CardInfo.%d.Num",iHouseIndex+1,cardIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value == NULL || strcmp(value,"0")== 0)
	{
		return;
	}
	sprintf(session, ".Business.UserInfo.%d.CardInfo.%d.CardNum",iHouseIndex+1,cardIndex+1);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	if(value)
	{
		strcpy(pstInfo->stCard[pstInfo->iCardCount].acSerinalNumber,value);
	}

	pstInfo->iCardCount ++;
}

BOOL TMQueryHouseInfo(TM_HOUSE_INFO_ST* pstHouse, int iHouseIndex)
{
	int iPhoneCount,cardCount, cardIndex, iPhoneIndex;
	char acTmp[32]={0X0};
	if(NULL==pstHouse || iHouseIndex < 0)
	{
		return FALSE;
	}
	memset(pstHouse, 0x0, sizeof(TM_HOUSE_INFO_ST));
	TMReadHouseInfo(pstHouse, iHouseIndex);

	iPhoneCount = pstHouse->iPhoneCount;
	pstHouse->iPhoneCount = 0;
	for(iPhoneIndex=0; iPhoneIndex<iPhoneCount && iPhoneIndex<TM_PHONE_INFO_MAX_COUNT; iPhoneIndex++)
	{
		TMReadPhoneInfo(pstHouse, iHouseIndex, iPhoneIndex);
	}

	cardCount = pstHouse->iCardCount;
	pstHouse->iCardCount = 0;
	for(cardIndex=0; cardIndex<cardCount && cardIndex<TM_CARD_INFO_MAX_COUNT; cardIndex++)
	{
		TMReadICIDCardInfo(pstHouse, iHouseIndex, cardIndex);
	}
	return TRUE;
}

/**
 *@brief Get handle of house info by special house code.
 *@param[in] pcHouseCode The code of house.
 *@return The handle of house info which used by other APIs[eg. @see TMCCGetRingTime].
 */
#define DEV_TYPE_LEN 1
#define BUILDING_CODE_LEN 3
#define UNIT_CODE_LEN  1
#define ROOM_CODE_LEN 4
#define HOUSE_CODE_LEN DEV_TYPE_LEN + BUILDING_CODE_LEN + UNIT_CODE_LEN + ROOM_CODE_LEN

int TMCCFilterHouseCode(const char* pcHouseCode,int *piUserCount)
{
	const char *value;
	unsigned int len;
	char acCommand[256];
	char acHouseCode[64];
	char acRoomCode[20] = {0x0};
	char acBuildingCode[8] = {0x0};
	char acUnitCode[8] = {0x0};
	int iCount, iUserCount;
	//int bBuilding=0,bUnit=0;
	if(pcHouseCode == NULL)
	{
		return FALSE;
	}
	//长度 待修改
	if(!strncmp(pcHouseCode, "F", strlen("F")))
	{
		strcpy(acHouseCode, pcHouseCode);
	}
	else if(strlen(pcHouseCode) < HOUSE_CODE_LEN)
	{
		memset(acHouseCode,'0',64);
		memcpy(acHouseCode+HOUSE_CODE_LEN-strlen(pcHouseCode),pcHouseCode,strlen(pcHouseCode));
		acHouseCode[HOUSE_CODE_LEN] = '\0';
	}
	else
	{
		strcpy(acHouseCode, pcHouseCode);
	}
	if(!strncmp(acHouseCode, "M", strlen("M")))
	{
		return FALSE;
	}
	else if(!strncmp(acHouseCode, "C", strlen("C")))
	{
		return FALSE;
	}
	else if(!strncmp(acHouseCode, "F", strlen("F")))
	{
		//memcpy(acRoomCode, acHouseCode + DEV_TYPE_LEN , ROOM_CODE_LEN);
		sscanf(acHouseCode,"F%s",acRoomCode);
	}
	else if(!strncmp(acHouseCode, "E", strlen("E")))
	{
		memcpy(acBuildingCode, acHouseCode + DEV_TYPE_LEN, BUILDING_CODE_LEN);
		memcpy(acUnitCode, acHouseCode + DEV_TYPE_LEN + BUILDING_CODE_LEN, UNIT_CODE_LEN);
		memcpy(acRoomCode, acHouseCode + DEV_TYPE_LEN + BUILDING_CODE_LEN + UNIT_CODE_LEN, ROOM_CODE_LEN);
	}
	else
	{
		memcpy(acRoomCode, acHouseCode + DEV_TYPE_LEN + BUILDING_CODE_LEN + UNIT_CODE_LEN, ROOM_CODE_LEN);
	}
	evcpe_repo_get(g_tr_evcpe_st.repo, ".Business.UserCount", &value, &len);
	if(value)
	{
		sscanf(value,"%d",&iCount);
		for(iUserCount=0; iUserCount<iCount; iUserCount++)
		{
			sprintf(acCommand, ".Business.UserInfo.%d.Num", iUserCount+1);
			evcpe_repo_get(g_tr_evcpe_st.repo, acCommand, &value, &len);
			if(value == NULL || strcmp(value,"0")== 0)
			{
				continue;
			}
			if(strlen(acBuildingCode) > 0)
			{
				sprintf(acCommand, ".Business.UserInfo.%d.BuildingNum", iUserCount+1);
				evcpe_repo_get(g_tr_evcpe_st.repo, acCommand, &value, &len);
				if(value && 0==strcmp(value, acBuildingCode));
				else continue;
			}
			if(strlen(acUnitCode) > 0)
			{
				sprintf(acCommand, ".Business.UserInfo.%d.UnitNum", iUserCount+1);
				evcpe_repo_get(g_tr_evcpe_st.repo, acCommand, &value, &len);
				if(value && 0==strcmp(value, acUnitCode));
				else continue;
			}
			sprintf(acCommand, ".Business.UserInfo.%d.RoomNumber", iUserCount+1);
			evcpe_repo_get(g_tr_evcpe_st.repo, acCommand, &value, &len);
			if(value && 0==strcmp(value, acRoomCode))
			{
				*piUserCount = iUserCount;
				return TRUE;
			}
			else continue;
		}
	}
	return FALSE;
}

YKHandle TMCCGetHouseInfoByHouseCode(const char* pcHouseCode)
{
	int iUserCount = 0;

	if(TMCCFilterHouseCode(pcHouseCode,&iUserCount) == TRUE)
	{
		TM_HOUSE_INFO_ST* pstHouseInfo=YKNew0(TM_HOUSE_INFO_ST, 1);
		if(FALSE == TMQueryHouseInfo(pstHouseInfo, iUserCount))
		{
			YKFree(pstHouseInfo);
			pstHouseInfo=NULL;
			return pstHouseInfo;
		}
		else return pstHouseInfo;
	}
	else
	{
		return NULL;
	}
}
/**
 *@brief Free memory which malloc by TMCCGetHouseInfoByHouseCode
 *@param[in] house The handle of house.
 *@return void.
 */
void TMCCDestroyHouseInfo(YKHandle hHouse)
{
	if(hHouse == NULL)
	{
		return ;
	}
	YKFree(hHouse);
}
/**
 *@brief Get ring time by house handle.
 *@param[in] house The handle of house. @see TMCCGetHouseInfoByHouseCode
 *@return Ring time.
 */
int TMCCGetRingTime(YKHandle hHouse)
{
	TM_HOUSE_INFO_ST* pstHouseInfo=(TM_HOUSE_INFO_ST*)hHouse;
	char acTmp[32]={0x0};
	if(NULL==pstHouseInfo)
	{
		return FALSE;
	}
	memcpy(acTmp, pstHouseInfo->acRingTime, sizeof(pstHouseInfo->acRingTime));
	return TMHexStringToDecimal(acTmp);
}
/**
 *@brief Judge is need to play early media.
 *@param[in] house The handle of house.
 *@return TRUE indicates need play early media else not.
 */
BOOL TMCCIsInCommingCallNotice(YKHandle hHouse)
{
	TM_HOUSE_INFO_ST* pstHouseInfo=(TM_HOUSE_INFO_ST*)hHouse;
	if(NULL==pstHouseInfo)
	{
		return FALSE;
	}
	return pstHouseInfo->cInCommingcCallNotice[0]=='0'?FALSE:TRUE;
}
/**
 *@brief Get Sip call delay time.
 *@param[in] house The handle of house.
 *@return Delay time.
 */
int TMCCGetSipCallDelay(YKHandle hHouse)
{
	TM_HOUSE_INFO_ST* pstHouseInfo=(TM_HOUSE_INFO_ST*)hHouse;
	char acTmp[32]={0x0};
	if(NULL==pstHouseInfo)
	{
		return FALSE;
	}
	memcpy(acTmp, pstHouseInfo->acSIPCallDelay, sizeof(pstHouseInfo->acSIPCallDelay));
	return TMHexStringToDecimal(acTmp);
}
/**
 *@brief Get the number of phone.
 *@param[in] house The handle of house.
 *@return The count of phone.
 */
int TMCCGetPhoneCount(YKHandle hHouse)
{
	TM_HOUSE_INFO_ST*pstHouse = (TM_HOUSE_INFO_ST*)hHouse;
	if(NULL==pstHouse)
	{
		return FALSE;
	}
	return pstHouse->iPhoneCount;
}
/**
 *@brief Get special phone info.
 *@param[in] house The handle of house.
 *@param[in] iIndex From 0.
 *@return The handle of phone info. NULL indicate failed else indicate success.
 */
YKHandle TMCCGetPhoneInfoByIndex(YKHandle hHouse, int iIndex)
{
	TM_HOUSE_INFO_ST*pstHouse = (TM_HOUSE_INFO_ST*)hHouse;
	if(NULL==pstHouse||iIndex>=TMCCGetPhoneCount(pstHouse)||iIndex<0)
	{
	    return NULL;
	}
	return &pstHouse->stPhoneInfo[iIndex];
}
/**
 *@brief Judge is support VOIP.
 *@param[in] phone The handle of phone.
 *@return TRUE if is support VOIP.
 */
BOOL TMCCIsVoipEnable(YKHandle hPhone)
{
	TM_PHONE_INFO_ST* phoneInfo=(TM_PHONE_INFO_ST*)hPhone;
	if(phoneInfo == NULL)
	{
		return FALSE;
	}
	return phoneInfo->cVoipEnable[0]=='0'?FALSE:TRUE;
}

BOOL TMNMGetDomainName(char* pcDomain)
{
	const char *value;
	unsigned int len;

	if(pcDomain == NULL)
	{
		return FALSE;
	}

	evcpe_repo_get(pstRepo, ".Communication.ImsDomainName", &value, &len);

	if(len > 0)
	{
		strcpy(pcDomain,value);
		return TRUE;
	}
	return FALSE;
}
/**
 *@brief Get CS phone number.
 *@param[in] phone The handle of phone.
 *@return CS phone number.
 */
char g_acTMPhoneNum[128] = {0x0};

const char* TMCCGetIMSPhoneNumber(const char* pcHouseCode)
{
	int iUserCount = 0;
	const char *value;
	unsigned int len;
	char acCommand[256];
	char number[TM_SIZE_PHONE_NUMBER]={0X0};

	if(TMCCFilterHouseCode(pcHouseCode,&iUserCount) == TRUE)
	{
		sprintf(acCommand, ".Business.UserInfo.%d.ImsPhoneNumber", iUserCount+1);
		evcpe_repo_get(g_tr_evcpe_st.repo, acCommand, &value, &len);
		if(value && len > 0)
		{
			strcpy(number,value);
			return number;
		}
	}
	return NULL;
}

const char* TMCCGetCsPhoneNumber(YKHandle hPhone)
{
	TM_PHONE_INFO_ST* phoneInfo=(TM_PHONE_INFO_ST*)hPhone;
	char number[TM_SIZE_PHONE_NUMBER]={0X0};
	if(phoneInfo == NULL)
	{
		return NULL;
	}
	TMTrimNLeftSpace(number, phoneInfo->acCsPhoneNumber, sizeof(phoneInfo->acCsPhoneNumber));
	if(number[0] == 0x0)
		return NULL;
	else
		return number;
}
/**
 *@brief Get SIP phone number.
 *@param[in] phone The handle of phone.
 *@return SIP phone number.
 */
const char* TMCCGetSipPhoneNumber(YKHandle hPhone)
{
	TM_PHONE_INFO_ST* phoneInfo=(TM_PHONE_INFO_ST*)hPhone;
	char number[TM_SIZE_PHONE_NUMBER]={0X0};
	if(phoneInfo == NULL)
	{
		return NULL;
	}
	TMTrimNLeftSpace(number, phoneInfo->acSipPhoneNumber, sizeof(phoneInfo->acSipPhoneNumber));
	if(number[0] == 0x0)
		return NULL;
	else
		return number;
}
/**
 *@brief Judge if is in not disturb time.
 *@param[in] phone The handle of phone.
 *@param[in] currentTime Current time.
 *@return TRUE indicate is in not disturb time else indicate not.
 */
BOOL TMCCIsNotDisturbTime(YKHandle hPhone)
{
	TM_PHONE_INFO_ST* pstPhone = (TM_PHONE_INFO_ST*)hPhone ;
	char* pcDisturbTime;
    YK_CURTIME_ST stCurtime;
    char acTmp[SIZE_16] = {0x0};
    int currtime;
    int begin=0, end=0;
	if(hPhone == NULL || strlen(pstPhone->acNotDisturbTime) == 0)
	{
		return FALSE;
	}
    YKGetCurrtime(&stCurtime);
    currtime = stCurtime.iHour*100+stCurtime.iMin;
	pcDisturbTime = strtok(pstPhone->acNotDisturbTime,",\0");
	while(pcDisturbTime != NULL)
	{
		memcpy(acTmp, pcDisturbTime, 4);
		begin=atoi(acTmp);
		memset(acTmp, 0x0, sizeof(acTmp));
		memcpy(acTmp, pcDisturbTime+4, 4);
		end = atoi(acTmp);
		if(currtime>begin && currtime < end)
		{
			return TRUE;
		}
		pcDisturbTime = strtok(NULL,",\0");
	}
	return FALSE;
}

#define HOUSE_CODE_LOG_LEN 12

BOOL TMCCIsValidPhoneNum(const char* pcPhoneNum, char* pcHouseCode)
{
	char session[128]={0X0};
	char session2[128]={0X0};
	const char *value;
	const char *value2;
	char acHouseCode[64] = {0x0};
	unsigned int len;
	unsigned int len2;
	int iUserCount = 0,iPhoneCount = 0,iUserIndex = 0,iPhoneIndex = 0;

	if(!g_tr_evcpe_st.repo)
	{
		return FALSE;
	}

	if(pcPhoneNum == NULL)
	{
		return FALSE;
	}

	if(strlen(pcPhoneNum) == 0)
	{
		return FALSE;
	}

	LOG_RUNLOG_DEBUG("TM CCTM PhoneNum is %s", pcPhoneNum);

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Business.UserCount", &value, &len);
	if(value)
	{
		sscanf(value,"%d",&iUserCount);
	}

	for(iUserIndex = 1;iUserIndex <= iUserCount;iUserIndex++)
	{
		sprintf(session, ".Business.UserInfo.%d.Num",iUserIndex);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value == NULL || strcmp(value,"0")== 0)
		{
			continue;
		}

		sprintf(session, ".Business.UserInfo.%d.PhoneCount",iUserIndex);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value)
		{
			sscanf(value,"%d",&iPhoneCount);
		}
		for(iPhoneIndex = 1;iPhoneIndex <= iPhoneCount;iPhoneIndex++)
		{
			sprintf(session, ".Business.UserInfo.%d.PhoneInfo.%d.Num",iUserIndex,iPhoneIndex);
			evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
			if(value == NULL || strcmp(value,"0")== 0)
			{
				continue;
			}

			sprintf(session, ".Business.UserInfo.%d.PhoneInfo.%d.SipAccount",iUserIndex,iPhoneIndex);
			evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
			if(value && strlen(value)>0)
			{
				if(NULL != strstr(value,pcPhoneNum))
				{
					goto final;
				}

				if(NULL != strstr(pcPhoneNum, value))
				{
					goto final;
				}
			}
		}
	}
	return FALSE;
final:
	sprintf(session, ".Business.UserInfo.%d.RoomNumber",iUserIndex);
	evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
	memset(acHouseCode,0,64);
	if(value && len <= 4)
	{
		sprintf(session2, ".Business.UserInfo.%d.BuildingNum",iUserIndex);
		evcpe_repo_get(g_tr_evcpe_st.repo, session2, &value2, &len2);
		if(value2 && len2>=3)
		{
			strncpy(acHouseCode, "0", 1);
			strncpy(acHouseCode + 1 , value2, 3);
		}
		else
		{
			strncpy(acHouseCode, "0000", 4);
		}
		sprintf(session2, ".Business.UserInfo.%d.UnitNum",iUserIndex);
		evcpe_repo_get(g_tr_evcpe_st.repo, session2, &value2, &len2);
		if(value2 && len2>=1)
		{
			strncpy(acHouseCode + 4, value2, 1);
		}
		else
		{
			strncpy(acHouseCode + 4, "0", 1);
		}
		strncpy(acHouseCode + 5, value, 4);
	}
	else if(value && len > 4)
	{
		strncpy(acHouseCode, "F", 1);
		strcpy(acHouseCode +1, value);
	}
	LOG_RUNLOG_DEBUG("TM acHouseCode is %s", acHouseCode);
	if(pcHouseCode && acHouseCode[0] != 0x0)
	{
		strcpy(pcHouseCode , acHouseCode);
	}
	return TRUE;
}

/**
 *@brief Get to call timeout value.
 *@param[in] void.
 *@return timeout value.
 */
int  TMCCGetIMSCallTimeOut(void)
{
	const char *value;
	unsigned int len;

	if(!g_tr_evcpe_st.repo)
	{
		return 0;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.SipCallTimeout", &value, &len);
	if(value)
	{
		return atoi(value);
	}
}

/**
 *@brief To judge whether the IMS-enabled
 *@param[in] void
 *@return TRUE indicates enable else not.
 */
BOOL TMCCIsAlarmEnabled(TM_ALARM_ID_EN enModule)
{
	const char *value;
	unsigned int len;

	if(!g_tr_evcpe_st.repo)
	{
		return FALSE;
	}

	switch(enModule)
	{
	case TM_ALARM_IMS_REGISTER:
		evcpe_repo_get(g_tr_evcpe_st.repo, ".AlarmEnable.ImsRegisterAlarmEnable", &value, &len);
		break;
	case TM_ALARM_SIP_CALL:
		evcpe_repo_get(g_tr_evcpe_st.repo, ".AlarmEnable.SipCallAlarmEnable", &value, &len);
		break;
	case TM_ALARM_IF_BOARD:
		evcpe_repo_get(g_tr_evcpe_st.repo, ".AlarmEnable.IfBoardComAlarmEnable", &value, &len);
		break;
	case TM_ALARM_TRANSFER:
		evcpe_repo_get(g_tr_evcpe_st.repo, ".AlarmEnable.FileTransAlarmEnable", &value, &len);
		break;
	case TM_ALARM_UPDATERESULT:
		evcpe_repo_get(g_tr_evcpe_st.repo, ".AlarmEnable.UpdateAlarmEnable", &value, &len);
		break;
	case TM_ALARM_SOUTH_INTERFACE:
		evcpe_repo_get(g_tr_evcpe_st.repo, ".AlarmEnable.SouthInterfaceAlarmEnable", &value, &len);
		break;
	default:
		break;
	}
	if(value)
	{
		if(strcmp(value, "1") == 0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

/**
 *@brief To judge whether the IMS-enabled
 *@param[in] void
 *@return TRUE indicates enable else not.
 */
BOOL TMCCIsIMSCallEnabled(void)
{
	const char *value;
	unsigned int len;

	if(!g_tr_evcpe_st.repo)
	{
		return FALSE;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.ImsCallEn", &value, &len);
	if(value)
	{
		if(strcmp(value, "1") == 0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

BOOL  TMCCIsPasswordOpendoorEnabled(void)
{
	const char *value;
	unsigned int len;

	if(!g_tr_evcpe_st.repo)
	{
		return FALSE;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.PasswordOpendoorEn", &value, &len);
	if(value)
	{
		if(strcmp(value, "1") == 0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}
/**
 *@brief To judge whether the VOIP Monitor-enabled.
 *@param[in] void.
 *@return TRUE indicates enable else not.
 */
BOOL  TMCCIsKeyOpenDoorEnabled(void)
{
	const char *value;
	unsigned int len;

	if(!g_tr_evcpe_st.repo)
	{
		return FALSE;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.KeyOpendoorEn", &value, &len);
	if(value)
	{
		if(strcmp(value, "1") == 0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

/**
 *@brief To judge whether the VOIP Monitor-enabled
 *@param[in] void
 *@return TRUE indicates enable else not.
 */
BOOL TMCCIsVoipMonitorEnabled(void)
{
	const char *value;
	unsigned int len;

	if(!g_tr_evcpe_st.repo)
	{
		return FALSE;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.MonitorEn", &value, &len);
	if(value)
	{
		if(strcmp(value, "1") == 0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

BOOL TMCCIsValidDemoPhoneNum(const char* pcPhoneNum, char* pcHouseCode)
{
	char session[128]={0X0};
	const char *value;
	unsigned int len;
	int iPhoneCount = 0,iPhoneIndex = 0;

	if(pcPhoneNum == NULL)
	{
		return FALSE;
	}

	if(strlen(pcPhoneNum) == 0)
	{
		return FALSE;
	}

//	int i= 0;
//	sscanf(pcPhoneNum,"%16lf",&dPhoneNum);
//	return binarysearch(DemoNumSort,dPhoneNum,g_iDemoNumNow);
	evcpe_repo_get(g_tr_evcpe_st.repo, ".Demo.PhoneCount", &value, &len);
	if(value)
	{
		sscanf(value,"%d",&iPhoneCount);
	}

	for(iPhoneIndex = 0;iPhoneIndex < iPhoneCount;iPhoneIndex++)
	{
		sprintf(session, ".Demo.PhoneInfo.%d.Num",iPhoneIndex+1);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value == NULL || strcmp(value,"0")== 0)
		{
			continue;
		}
		sprintf(session, ".Demo.PhoneInfo.%d.SipAccount",iPhoneIndex+1);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value && len > 0)
		{
			//=============================edit by hb
			if(NULL != strstr(value,pcPhoneNum))
			{
				if(pcHouseCode)
				{
					strcpy(pcHouseCode , "Demo");
				}
				return TRUE;
			}

			if(NULL != strstr(pcPhoneNum, value))
			{
				if(pcHouseCode)
				{
					strcpy(pcHouseCode , "Demo");
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

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
void TMNMGetWANInfo(char* pcUserName, char* pcPassWord)
{
	const char *value;
	unsigned int len;
	struct evcpe_repo *pstRepoTmp;
	static int iFirstLoad = 1;

	if(iFirstLoad)
	{
		iFirstLoad = 0;
		pstRepoTmp = pstRepo;
	}
	else
	{
		pstRepoTmp = g_tr_evcpe_st.repo;
	}

	evcpe_repo_get(pstRepoTmp, ".Communication.WanUserName", &value, &len);
	if(pcUserName && value && len)
	{
		strcpy(pcUserName,value);
	}
	evcpe_repo_get(pstRepoTmp, ".Communication.WanPassword", &value, &len);
	if(pcPassWord && value && len)
	{
		strcpy(pcPassWord,value);
	}
}
/**@}*///For NM module.

void TMGetIMSInfo2(char* pcUserName)
{
    const char *value;
    unsigned int len;
    struct evcpe_repo *pstRepoTmp;
    char acPhoneTemp[64] = {0x0};
    char acDomain[64] = {0x0};
    pstRepoTmp = g_tr_evcpe_st.repo;

    evcpe_repo_get(pstRepoTmp, ".Communication.SipAccount", &value, &len);

    if(pcUserName && value)
    {
        strcpy(pcUserName,value);
    }
}


/**
 *@brief Get IMS pcDomain.
 *@param[in] pcDomain For received data.
 *@return void.
 */
void TMGetIMSInfo(char* pcUserName, char* pcPassWord, char* pcProxy, int* piPort, char* pcDomain)
{
	const char *value;
	unsigned int len;
	struct evcpe_repo *pstRepoTmp;
	char acPhoneTemp[64] = {0x0};
	char acDomain[64] = {0x0};
//	static int iFirstLoad = 1;

//	if(iFirstLoad)
//	{
//		iFirstLoad = 0;
//		pstRepoTmp = pstRepo;
//	}
//	else
//	{
		pstRepoTmp = g_tr_evcpe_st.repo;
		LOG_RUNLOG_DEBUG("TM =get ims info==");
//	}

	evcpe_repo_get(pstRepoTmp, ".Communication.SipAccount", &value, &len);

	if(pcUserName && value)
	{
		//判断是否有＠后缀
		strcpy(acPhoneTemp,value);
		if(strstr(acPhoneTemp,"@") == NULL)
		{
			if(TMNMGetDomainName(acDomain))
			{
				if(strlen(acPhoneTemp) == 18)
				{
					sprintf(pcUserName,"+86%s@%s",acPhoneTemp,acDomain);
				}
				else
				{
					sprintf(pcUserName,"%s@%s",acPhoneTemp,acDomain);
				}
			}
		}
		else
		{
			strcpy(pcUserName,acPhoneTemp);
		}
	}
	LOG_RUNLOG_DEBUG("TM =%s==",value);
	evcpe_repo_get(pstRepoTmp, ".Communication.SipPassword", &value, &len);
	if(pcPassWord && value)
	{
#ifdef BASE64_3DES
		char acImsPwd[256] = {0x0};
		char* pcBase64;
		char* pcBaseHex;
		char* bufferout = (char *)malloc(1024);
		int j= 0,iBase64 = 0;
		memset(bufferout,0,1024);
		strcpy(acImsPwd,value);
		pcBase64 = Base64Decode(acImsPwd);
		LOG_RUNLOG_DEBUG("TM =%s==",pcBase64);
		if(NULL != pcBase64)
		{
			//转为hex
			iBase64 =strlen(pcBase64);
			pcBaseHex = Base64FromHex(pcBase64,iBase64);

			//3des
			XP3Des(DECRYPT,CBC,pcBaseHex,iBase64/2+1,pcDesKey,strlen(pcDesKey),bufferout,1024,acDesIv,iPadMode);
			//不可识别码处理
			for(j=0; j<1024; j++)
			{
				if( bufferout[j] == 0x0)
				{
					break;
				}
				if(bufferout[j] >= 0x20 && bufferout[j] < 0x7f)
				{
					continue;
				}
				else
				{
					bufferout[j] = 0x0;
					break;
				}
			}

			strcpy(pcPassWord,bufferout);
			//printf("%s--%s\n",pcPassWord,bufferout);
			free(pcBase64);
			free(pcBaseHex);
		}
		else
		{
			//strcpy(pcPassWord,acImsPwd);
			LOG_RUNLOG_DEBUG("TM =error==");
			pcPassWord[0] = 0x0;
		}
		free(bufferout);
#else
		strcpy(pcPassWord,value);
#endif
	}
	LOG_RUNLOG_DEBUG("TM =%s==",value);
	evcpe_repo_get(pstRepoTmp, ".Communication.ImsProxyIP", &value, &len);
	if(pcProxy && value)
	{
		strcpy(pcProxy,value);
	}
	evcpe_repo_get(pstRepoTmp, ".Communication.ImsProxyPort", &value, &len);
	if(piPort && value)
	{
		sscanf(value,"%d",piPort);
	}
	evcpe_repo_get(pstRepoTmp, ".Communication.ImsDomainName", &value, &len);
	if(pcDomain && value)
	{
		strcpy(pcDomain,value);
	}
}
/**
 *@brief Send alarm.
 *@param[in] module Alarm module @see TM_ALARM_ID_EN
 *@param[in] state  '0' alarm restore '1' alarm happened.
 *@return void.
 */
//void TMUpdateAlarmState(TM_ALARM_ID_EN enModule, char cState)
//{
//
//}
/**
 *@name Query device id.
 *@param[in] id For received id.
 *@return void.
 */
void TMQueryDeviceID(char* pcID)
{
	static char *pcDeviceID=NULL;
	unsigned int len;

//	evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.DeviceNo", &value, &len);
//	if(pcID && value)
//	{
//		strcpy(pcID,value);
//	}
	if(pcDeviceID == NULL)
	{
		if(!pstRepo)
		{
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
			TMXmlSetup(TM_DATA_RUNNING,TM_MODEL_PATH);
#else
			TMXmlSetup(TM_DATA_PATH,TM_MODEL_PATH);
#endif
		}

		evcpe_repo_get(pstRepo, ".DeviceInfo.DeviceNo", &pcDeviceID, &len);
		if(pcID && pcDeviceID)
		{
			strcpy(pcID,pcDeviceID);
		}
	}
	else
	{
		if(pcID)
		{
			strcpy(pcID,pcDeviceID);
		}
	}
}
/**
 *@brief Query ftp info.
 *@param[in] host For received id.
 *@param[in] host For received piPort.
 *@param[in] host For received pcUserName.
 *@param[in] host For received pcPassWord.
 */
void TMQueryFtpInfo(char* pcHost, int*piPort, char* pcUserName, char* pcPassWord)
{
	const char *value;
	unsigned int len;
	struct evcpe_repo *pstRepoTmp;

	if(g_tr_evcpe_st.repo == NULL)
	{
		pstRepoTmp = pstRepo;
	}
	else
	{
		pstRepoTmp = g_tr_evcpe_st.repo;
	}

	evcpe_repo_get(pstRepoTmp, ".Communication.FtpIP", &value, &len);
	if(pcHost && value)
	{
		strcpy(pcHost,value);
	}
	evcpe_repo_get(pstRepoTmp, ".Communication.FtpPort", &value, &len);
	if(piPort && value)
	{
		sscanf(value,"%d",piPort);
	}
	evcpe_repo_get(pstRepoTmp, ".Communication.FtpUserName", &value, &len);
	if(pcUserName && value)
	{
		strcpy(pcUserName,value);
	}
	evcpe_repo_get(pstRepoTmp, ".Communication.FtpPassword", &value, &len);
	if(pcPassWord && value)
	{
		strcpy(pcPassWord,value);
	}
}
/**
 *@brief Query application version
 *@param[in] version For received version.
 *@return void.
 */
void TMQueryAppVersion(char* pcVersion)
{
	if(NULL==pcVersion)
	{
		return;
	}
	strcpy(pcVersion, g_acTRFirmVersion);
}

/**
 *@name For XT module
 *@{
 */
/**
 *@brief Judge is valid DoorPassword.
 *@param[in] Password.
 *@return TRUE indicate is valid else is not.
 */
BOOL TMXTIsValidDoorPassword(char* Password)
{
	const char *value;
	unsigned int len;

	if(!g_tr_evcpe_st.repo)
	{
		return FALSE;
	}

	if(Password == NULL)
	{
		return FALSE;
	}

//	if(TMSMIsPasswordOpenDoorEnabled() == FALSE)
//	{
//		LOG_RUNLOG_DEBUG("TM password disable ");
//		return FALSE;
//	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.PasswordOpendoor", &value, &len);

	if(value && strcmp(value,"")!= 0)
	{
		if(strcmp(Password,value) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/**
 *@name For SM module
 *@{
 */
/**
 *@brief Judge is valid serial number.
 *@param[in] pcSerinalNumber The serial number.
 *@return TRUE indicate is valid else is not.
 */

BOOL TMSMIsValidPropertyCard(const char* pcSerinalNumber)
{
	char session[128]={0X0};
	const char *value;
	unsigned int len;
	int iCardCount = 0,iCardIndex = 0;

	if(pcSerinalNumber == NULL)
	{
		return FALSE;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".PropertyCard.CardCount", &value, &len);
	if(value)
	{
		sscanf(value,"%d",&iCardCount);
	}

	for(iCardIndex = 0;iCardIndex < iCardCount;iCardIndex++)
	{
		sprintf(session, ".PropertyCard.CardInfo.%d.Num",iCardIndex+1);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value == NULL || strcmp(value,"0")== 0)
		{
			continue;
		}
		sprintf(session, ".PropertyCard.CardInfo.%d.CardNum",iCardIndex+1);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value)
		{
			if(0 == strcmp(pcSerinalNumber,value))
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL TMSMIsValidAuthorizeCard(const char* pcSerinalNumber)
{
	char session[128]={0X0};
	const char *value;
	unsigned int len;
	int iCardCount = 0,iCardIndex = 0;

	if(pcSerinalNumber == NULL)
	{
		return FALSE;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".PropertyCard.CardCount", &value, &len);
	if(value )
	{
		sscanf(value,"%d",&iCardCount);
	}

	for(iCardIndex = 0;iCardIndex < iCardCount;iCardIndex++)
	{
		sprintf(session, ".PropertyCard.CardInfo.%d.Num",iCardIndex+1);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value == NULL || strcmp(value,"0")== 0)
		{
			continue;
		}
		sprintf(session, ".PropertyCard.CardInfo.%d.CardNum",iCardIndex+1);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value && len >0)
		{
			if(0 == strncmp(value,"YKAC",4))
			{
				if(0 == strcmp(pcSerinalNumber,value+4))
				{
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

BOOL TMSMIsValidICIDCard(const char* pcSerinalNumber)
{
	char session[128]={0X0};
	const char *value;
	unsigned int len;
	int iUserCount = 0,iCardCount = 0,iUserIndex = 0,iCardIndex = 0;

	if(!g_tr_evcpe_st.repo)
	{
		return FALSE;
	}

	if(pcSerinalNumber == NULL)
	{
		return FALSE;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Business.UserCount", &value, &len);
	if(value)
	{
		sscanf(value,"%d",&iUserCount);
	}

	for(iUserIndex = 0;iUserIndex < iUserCount;iUserIndex++)
	{
		sprintf(session, ".Business.UserInfo.%d.Num",iUserIndex+1);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value == NULL || strcmp(value,"0")== 0)
		{
			continue;
		}
		sprintf(session, ".Business.UserInfo.%d.CardCount",iUserIndex+1);
		evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
		if(value)
		{
			sscanf(value,"%d",&iCardCount);
		}
		for(iCardIndex = 0;iCardIndex < iCardCount;iCardIndex++)
		{
			sprintf(session, ".Business.UserInfo.%d.CardInfo.%d.Num",iUserIndex+1,iCardIndex+1);
			evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
			if(value == NULL || strcmp(value,"0")== 0)
			{
				continue;
			}
			sprintf(session, ".Business.UserInfo.%d.CardInfo.%d.CardNum",iUserIndex+1,iCardIndex+1);
			evcpe_repo_get(g_tr_evcpe_st.repo, session, &value, &len);
			if(value)
			{
				if(0 == strcmp(pcSerinalNumber,value))
				{
					return TRUE;
				}
			}
		}
	}
	if(TMSMIsValidPropertyCard(pcSerinalNumber) == TRUE)
	{
		return TRUE;
	}

	return FALSE;
}

BOOL  TMSMIsICOpenDoorEnabled(void)
{
	const char *value;
	unsigned int len;

	if(!g_tr_evcpe_st.repo)
	{
		return FALSE;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.ICOpendoorEn", &value, &len);
	if(value)
	{
		if(strcmp(value,"1") == 0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

char *TMGetDevInfo(void)
{
	unsigned int len;
	const char *value;
	char *p = pStrTmp;

	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}

	memset(pStrTmp,0,BUF_SIZE);

	evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.SerialNumber", &value, &len);
	if(value && len<60){
		strcpy(p, value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.Manufacturer", &value, &len);
	if(value && len<60){
		strcpy(p, value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.OUI", &value, &len);
	if(value && len<60){
		strcpy(p, value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.ProductClass", &value, &len);
	if(value && len<60){
		strcpy(p, value);
		p+=len;
	}

	return pStrTmp;
}

//char *TMGetVersionInfo(void)
int TMGetVersionInfo(char *pStr)
{
	const char *value;
	unsigned int len;
	//char *pStr = (char *)malloc(BUF_SIZE);
	char *p = pStr;
	int ret;

	//if(!pStr) return NULL;
	//memset(pStr,0,BUF_SIZE);
	
	sprintf(p,"%s|%s|%s",YK_APP_VERSION,YK_KERNEL_VERSION,YK_HARDWARE_VERSION);

	return 0;

//	ret = evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.FirmwareVersion", &value, &len);
//	if(ret==0){
//		if(value && len<60){
//			sprintf(p,"%s",value);
//			p+=len;
//		}
//		else{
//			sprintf(p,"%s"," ");
//			p+=1;
//		}
//		*p++ = '|';
//	}
//
//	ret = evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.HardwareVersion", &value, &len);
//	if(ret==0){
//		if(value && len<60){
//			sprintf(p,"%s",value);
//			p+=len;
//		}
//		else{
//			sprintf(p,"%s"," ");
//			p+=1;
//		}
//			*p++ = '|';
//	}
//
//	ret = evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.KernelVersion", &value, &len);
//	if(ret==0){
//		if(value && len<60){
//			sprintf(p,"%s",value);
//			p+=len;
//		}
//		else{
//			sprintf(p,"%s"," ");
//			p+=1;
//		}
//	}
//
//	return ret;

	//return pStr;
}

char *TMGetPlatformInfo(void)
{
	const char *value;
	unsigned int len;
	char *p = pStrTmp;
	if(!pStrTmp) return NULL;
	memset(pStrTmp,0,BUF_SIZE);
	
	evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.DeviceNo", &value, &len);
	if(value && len<60){
		sprintf(p,"%s",value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".ManagementServer.URL", &value, &len);
	if(value && len<60){
		sprintf(p,"%s",value);
		p+=len;
	}

	return pStrTmp;
}

int TMSetI5PlatformDomainName(const char *str)
{
	return 0;
}

int TMSetPlatformUserName(const char *str)
{
	return 0;
}

int TMSetPlatformPassword(const char *str)
{
	return 0;
}



char *TMGetSIPInfo(void){
	const char *value;
	unsigned int len;
	char *p = pStrTmp;
	if(!pStrTmp) return NULL;
	memset(pStrTmp,0,BUF_SIZE);
	
	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.SipAccount", &value, &len);
	if(value && len<60){
		sprintf(p,"%s",value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.ImsDomainName", &value, &len);
	if(value && len<60){
		sprintf(p,"%s",value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.ImsProxyIP", &value, &len);
	if(value && len<60){
		sprintf(p,"%s",value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.ImsProxyPort", &value, &len);
	if(value && len<60){
		sprintf(p,"%s",value);
		p+=len;
	}

	return pStrTmp;
}

int TMSetDevNum(const char *str)
{
	int ret;
	char *value;

	if(!g_tr_evcpe_st.repo)
	{
		return -1;
	}

	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".DeviceInfo.DeviceNo", str, strlen(str));
	value = evcpe_ltoa(0);
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.BootStrap", value, strlen(value));
	evcpe_persister_persist(g_tr_evcpe_st.persist);
	return ret;
}

int TMSetPlatformDomainName(const char *str)
{
	int ret;
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.URL", str, strlen(str));

	return ret;
}

int TMSetPPPOEUername(const char *str)
{
	int ret;
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.WanUserName", str, strlen(str));

	return ret;
}
	
int TMSetPPPOEPassword(const char *str)
{
	int ret;
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.WanPassword", str, strlen(str));

	return ret;
}

int TMGetManagementPassword(const char *str)
{
	unsigned int len;
	//char value[TM_SIZE_PHONE_NUMBER]={0x0};
	const char *value;
	//char *value = (char *)malloc(BUF_SIZE);

	int ret;
	ret = evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.PasswordManagement", &value, &len);
	/*if(len != 6)
	{
		return RST_ERR;
	}*/
	memcpy(str, value, len);
	__android_log_print("", "JNIMsg", "ret = %d managementPassword[%d] = %s",ret, len, str);
	return ret;
}

int TMSetManagementPassword(const char *str)
{
	int ret;

	if(strlen(str) != 6)
	{
		LOG_RUNLOG_DEBUG("TMSetManagementPassword strlen is:%d", strlen(str));
		return RST_ERR;
	}

	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".Config.PasswordManagement", str, strlen(str));

	return ret;
}

int TMSetOpendoorPassword(const char *str)
{
	int ret;

	if(strlen(str) != 6)

	{
		LOG_RUNLOG_DEBUG("TMSetOpendoorPassword strlen is:%d", strlen(str));
		return RST_ERR;
	}

	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".Config.PasswordOpendoor", str, strlen(str));

	return ret;
}

BOOL  TMSMIsPasswordOpenDoorEnabled(void)
{
	const char *value;
	unsigned int len;

	evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.PasswordOpendoorEn", &value, &len);
	if(value)
	{
		if(strcmp(value,"1") == 0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

BOOL TMNMGetSN(char* pcSN)
{
	const char *value;
	unsigned int len;

	if(pcSN == NULL)
	{
		return FALSE;
	}

	if(!pstRepo)
	{
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
		TMXmlSetup(TM_DATA_RUNNING,TM_MODEL_PATH);
#else
		TMXmlSetup(TM_DATA_PATH,TM_MODEL_PATH);
#endif
	}

	evcpe_repo_get(pstRepo, ".DeviceInfo.SerialNumber", &value, &len);

	if(value && strcmp(value,"") != 0)
	{
		strcpy(pcSN,value);
		return TRUE;
	}
	return FALSE;
}

#define ICID_LOG_SIZE 26
#define ICID_LOG_MAX_SIZE 1024

void TMSetICIDOpenDoor(const char* pcSerinalNumber)
{
	char buf[ICID_LOG_MAX_SIZE+1];
	char new_buf[64] = {0x0};
	const char *value = NULL;
	unsigned int len = 0;
	time_t   now;         //实例化time_t结构
	struct   tm     *timenow;         //实例化tm结构指针

	if(pcSerinalNumber == NULL)
	{
		return;
	}

	now = time(NULL);
	timenow  = localtime(&now);

	sprintf(new_buf,"%02d%02d%02d%02d%02d%02d:%s;",timenow->tm_year%100,timenow->tm_mon+1,
			timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec,pcSerinalNumber);
#ifdef TM_CONFIG_DOWNLOAD
	len = strlen(g_stTemp.acICIDLog);
	if(len >0)
	{
		if(len + strlen(new_buf) < ICID_LOG_MAX_SIZE)
		{
			sprintf(buf,"%s%s",g_stTemp.acICIDLog,new_buf);
		}
	}
#else
	evcpe_repo_get(g_tr_evcpe_st.repo, ".Config.ICIDOpenLog", &value, &len);
	if(value && len >0)
	{
		if(len + strlen(new_buf) < ICID_LOG_MAX_SIZE)
		{
			sprintf(buf,"%s%s",value,new_buf);
		}
	}
#endif
	else
	{
		if(strlen(new_buf) < ICID_LOG_MAX_SIZE)
		{
			sprintf(buf,"%s",new_buf);
		}
	}
	LOG_RUNLOG_DEBUG("TM ICIDOpenLog = %s",buf);
#ifdef TM_CONFIG_DOWNLOAD
	//写入结构体
	strcpy(g_stTemp.acICIDLog,buf);
	//写入文件
	evcpe_index_save();
#else
	evcpe_repo_set(g_tr_evcpe_st.repo, ".Config.ICIDOpenLog", buf, strlen(buf));
#endif
	g_tr_evcpe_st.cpe->icidflag = 1;

}

static char *pcModel ="\
<?xml version=\"1.0\"?>\n\
<model>\n\
  <schema name=\"InternetGatewayDevice\" type=\"object\">\n\
    <extension name=\"Event\" type=\"multipleObject\">\n\
      <extension name=\"EventCode\" type=\"string\"/>\n\
      <extension name=\"CommandKey\" type=\"string\"/>\n\
    </extension>\n\
    <schema name=\"DeviceSummary\" type=\"string\" constraint=\"1024\"/>\n\
    <schema name=\"LANDeviceNumberOfEntries\" type=\"unsignedInt\"/>\n\
    <schema name=\"WANDeviceNumberOfEntries\" type=\"unsignedInt\"/>\n\
    <schema name=\"ManagementServer\" type=\"object\">\n\
      <schema name=\"URL\" type=\"string\" constraint=\"256\" write=\"W\" inform=\"true\" notification=\"active\"/>\n\
      <extension name=\"Timeout\" type=\"unsignedInt\"/>\n\
      <schema name=\"Username\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"Password\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"PeriodicInformEnable\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"PeriodicInformInterval\" type=\"unsignedInt\" constraint=\"1:\" write=\"W\"/>\n\
      <schema name=\"PeriodicInformTime\" type=\"dateTime\" write=\"W\"/>\n\
      <schema name=\"ParameterKey\" type=\"string\" constraint=\"32\"/>\n\
      <schema name=\"ConnectionRequestURL\" type=\"string\" constraint=\"256\" />\n\
      <schema name=\"ConnectionRequestUsername\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"ConnectionRequestPassword\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <extension name=\"ConnectionRequestInterval\" type=\"unsignedInt\"/>\n\
      <schema name=\"UpgradesManaged\" type=\"boolean\" write=\"W\"/>\n\
      <extension name=\"Authentication\" type=\"string\" constraint=\"NONE|BASIC|DIGEST\"/>\n\
      <extension name=\"ProxyURL\" type=\"string\" constraint=\"256\"/>\n\
      <extension name=\"ProxyUsername\" type=\"string\" constraint=\"256\"/>\n\
      <extension name=\"ProxyPassword\" type=\"string\" constraint=\"256\"/>\n\
      <schema name=\"BootStrap\" type=\"boolean\" write=\"W\"/>\n\
    </schema>\n\
    <schema name=\"Communication\" type=\"object\">\n\
      <schema name=\"WanUserName\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"WanPassword\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"SipAccount\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"SipPassword\" type=\"string\" constraint=\"256\"/>\n\
      <schema name=\"ImsDomainName\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"ImsProxyIP\" type=\"string\" constraint=\"15\"/>\n\
      <schema name=\"ImsProxyPort\" type=\"string\" constraint=\"5\"/>\n\
      <schema name=\"FtpIP\" type=\"string\" constraint=\"15\"/>\n\
      <schema name=\"FtpPort\" type=\"string\" constraint=\"5\"/>\n\
      <schema name=\"FtpUserName\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"FtpPassword\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"PlatformDomain\" type=\"string\" constraint=\"64\"/>\n\
    </schema>\n\
    <schema name=\"DeviceInfo\" type=\"object\">\n\
      <schema name=\"Manufacturer\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"OUI\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"ProductClass\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"SerialNumber\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"FirmwareVersion\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"KernelVersion\" type=\"string\" constraint=\"64\" />\n\
      <schema name=\"HardwareVersion\" type=\"string\" constraint=\"64\" />\n\
      <schema name=\"DeviceNo\" type=\"string\" constraint=\"64\"/>\n\
    </schema>\n\
    <schema name=\"Config\" type=\"object\">\n\
      <schema name=\"SipCallTimeout\" type=\"string\" constraint=\"2\"/>\n\
      <schema name=\"ImsCallEn\" type=\"string\" constraint=\"0|1\"/>\n\
      <schema name=\"PasswordOpendoorEn\" type=\"string\" constraint=\"0|1\"/>\n\
      <schema name=\"ICOpendoorEn\" type=\"string\" constraint=\"0|1\"/>\n\
      <schema name=\"KeyOpendoorEn\" type=\"string\" constraint=\"0|1\"/>\n\
      <schema name=\"MonitorEn\" type=\"string\" constraint=\"0|1\"/>\n\
      <schema name=\"PasswordOpendoor\" type=\"string\" constraint=\"16\"/>\n\
      <schema name=\"PasswordManagement\" type=\"string\" constraint=\"16\"/>\n\
      <schema name=\"CRC\" type=\"string\" constraint=\"10\"/>\n\
      <schema name=\"ICIDOpenLog\" type=\"string\" constraint=\"1024\" write=\"W\"/>\n\
    </schema>\n\
    <schema name=\"AlarmStatus\" type=\"object\">\n\
      <schema name=\"ImsRegisterFail\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"SipCallFail\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"IfBoardComFail\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"FileTransFail\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"UpdateResult\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"SipCallState\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"DeviceBoot\" type=\"boolean\" write=\"W\"/>\n\
    </schema>\n\
    <schema name=\"AlarmEnable\" type=\"object\">\n\
      <schema name=\"ImsRegisterAlarmEnable\" type=\"string\" constraint=\"1\" write=\"W\"/>\n\
      <schema name=\"SipCallAlarmEnable\" type=\"string\" constraint=\"1\" write=\"W\"/>\n\
      <schema name=\"IfBoardComAlarmEnable\" type=\"string\" constraint=\"1\" write=\"W\"/>\n\
      <schema name=\"FileTransAlarmEnable\" type=\"string\" constraint=\"1\" write=\"W\"/>\n\
      <schema name=\"UpdateAlarmEnable\" type=\"string\" constraint=\"1\" write=\"W\"/>\n\
      <schema name=\"SouthInterfaceAlarmEnable\" type=\"string\" constraint=\"1\" write=\"W\"/>\n\
    </schema>\n\
    <schema name=\"Demo\" type=\"object\">\n\
      <schema name=\"VoipEn\" type=\"string\" constraint=\"0|1\" write=\"W\"/>\n\
      <schema name=\"RoomNum\" type=\"string\" constraint=\"20\" write=\"W\"/>\n\
      <schema name=\"PhoneNum\" type=\"string\" constraint=\"20\" write=\"W\"/>\n\
      <schema name=\"PhoneCount\" type=\"unsignedInt\"/>\n\
      <schema name=\"PhoneInfo\" type=\"multipleObject\" write=\"W\" number=\"PhoneCount\">\n\
        <schema name=\"Num\" type=\"unsignedInt\"/>\n\
        <schema name=\"SipAccount\" type=\"string\" constraint=\"20\"/>\n\
      </schema>\n\
    </schema>\n\
    <schema name=\"PropertyCard\" type=\"object\">\n\
      <schema name=\"CardCount\" type=\"unsignedInt\"/>\n\
      <schema name=\"CardInfo\" type=\"multipleObject\" write=\"W\" number=\"CardCount\">\n\
        <schema name=\"Num\" type=\"unsignedInt\"/>\n\
        <schema name=\"CardNum\" type=\"string\" constraint=\"100\"/>\n\
      </schema>\n\
    </schema>\n\
    <schema name=\"Business\" type=\"object\">\n\
      <schema name=\"UserCount\" type=\"unsignedInt\"/>\n\
      <schema name=\"UserInfo\" type=\"multipleObject\" write=\"W\" number=\"UserCount\">\n\
        <schema name=\"Num\" type=\"unsignedInt\"/>\n\
        <schema name=\"BuildingNum\" type=\"string\" constraint=\"6\"/>\n\
        <schema name=\"UnitNum\" type=\"string\" constraint=\"6\"/>\n\
        <schema name=\"RoomNumber\" type=\"string\" constraint=\"20\"/>\n\
        <schema name=\"RingingTime\" type=\"string\" constraint=\"2\"/>\n\
        <schema name=\"IncomingCallNotice\" type=\"string\" constraint=\"0|1\"/>\n\
        <schema name=\"TimeDelay\" type=\"string\" constraint=\"2\"/>\n\
		<schema name=\"ImsPhoneNumber\" type=\"string\" constraint=\"32\"/>\n\
        <schema name=\"PhoneCount\" type=\"unsignedInt\"/>\n\
        <schema name=\"PhoneInfo\" type=\"multipleObject\" write=\"W\" number=\"PhoneCount\">\n\
          <schema name=\"Num\" type=\"unsignedInt\"/>\n\
          <schema name=\"VoipEn\" type=\"string\" constraint=\"0|1\"/>\n\
          <schema name=\"CsPhoneNum\" type=\"string\" constraint=\"20\"/>\n\
          <schema name=\"SipAccount\" type=\"string\" constraint=\"20\"/>\n\
          <schema name=\"TimeNotDisturb\" type=\"string\" constraint=\"60\"/>\n\
        </schema>\n\
        <schema name=\"CardCount\" type=\"unsignedInt\"/>\n\
        <schema name=\"CardInfo\" type=\"multipleObject\" write=\"W\" number=\"CardCount\">\n\
          <schema name=\"Num\" type=\"unsignedInt\"/>\n\
          <schema name=\"CardNum\" type=\"string\" constraint=\"100\"/>\n\
        </schema>\n\
      </schema>\n\
    </schema>\n\
    <schema name=\"UserInterface\" type=\"object\">\n\
	  <schema name=\"PasswordRequired\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"PasswordUserSelectable\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"UpgradeAvailable\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"WarrantyDate\" type=\"dateTime\" write=\"W\"/>\n\
      <schema name=\"ISPName\" type=\"string\" constraint=\"64\" write=\"W\"/>\n\
      <schema name=\"ISPHelpDesk\" type=\"string\" constraint=\"32\" write=\"W\"/>\n\
      <schema name=\"ISPHomePage\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"ISPHelpPage\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"ISPLogo\" type=\"base64\" constraint=\"5460\" write=\"W\"/>\n\
      <schema name=\"ISPLogoSize\" type=\"unsignedInt\" constraint=\"0:4095\" write=\"W\"/>\n\
      <schema name=\"ISPMailServer\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"TextColor\" type=\"string\" constraint=\"6\" write=\"W\"/>\n\
      <schema name=\"BackgroundColor\" type=\"string\" constraint=\"6\" write=\"W\"/>\n\
      <schema name=\"ButtonColor\" type=\"string\" constraint=\"6\" write=\"W\"/>\n\
      <schema name=\"ButtonTextColor\" type=\"string\" constraint=\"6\" write=\"W\"/>\n\
      <schema name=\"AutoUpdateServer\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"UserUpdateServer\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"ExampleLogin\" type=\"string\" constraint=\"40\" write=\"W\"/>\n\
      <schema name=\"ExamplePassword\" type=\"string\" constraint=\"30\" write=\"W\"/>\n\
    </schema>\n\
  </schema>\n\
</model>\n\
";

char *pcBaModel = "\
<?xml version=\"1.0\"?>\n\
<model>\n\
  <schema name=\"Device\" type=\"object\">\n\
    <extension name=\"Event\" type=\"multipleObject\">\n\
      <extension name=\"EventCode\" type=\"string\"/>\n\
      <extension name=\"CommandKey\" type=\"string\"/>\n\
    </extension>\n\
    <schema name=\"DeviceSummary\" type=\"string\" constraint=\"1024\"/>\n\
    <schema name=\"LANDeviceNumberOfEntries\" type=\"unsignedInt\"/>\n\
    <schema name=\"WANDeviceNumberOfEntries\" type=\"unsignedInt\"/>\n\
    <schema name=\"ManagementServer\" type=\"object\">\n\
      <schema name=\"URL\" type=\"string\" constraint=\"256\" write=\"W\" inform=\"true\" notification=\"active\"/>\n\
      <extension name=\"Timeout\" type=\"unsignedInt\"/>\n\
      <schema name=\"Username\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"Password\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"FaultCode\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"PeriodicInformEnable\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"PeriodicInformInterval\" type=\"unsignedInt\" constraint=\"1:\" write=\"W\"/>\n\
      <schema name=\"PeriodicInformTime\" type=\"dateTime\" write=\"W\"/>\n\
      <schema name=\"ParameterKey\" type=\"string\" constraint=\"32\"/>\n\
      <schema name=\"ConnectionRequestURL\" type=\"string\" constraint=\"256\" inform=\"true\"/>\n\
      <schema name=\"ConnectionRequestUsername\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"ConnectionRequestPassword\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <extension name=\"ConnectionRequestInterval\" type=\"unsignedInt\"/>\n\
      <schema name=\"UpgradesManaged\" type=\"boolean\" write=\"W\"/>\n\
      <extension name=\"Authentication\" type=\"string\" constraint=\"NONE|BASIC|DIGEST\"/>\n\
      <extension name=\"ProxyURL\" type=\"string\" constraint=\"256\"/>\n\
      <extension name=\"ProxyUsername\" type=\"string\" constraint=\"256\"/>\n\
      <extension name=\"ProxyPassword\" type=\"string\" constraint=\"256\"/>\n\
      <schema name=\"BootStrap\" type=\"boolean\" write=\"W\"/>\n\
    </schema>\n\
    <schema name=\"Communication\" type=\"object\">\n\
      <schema name=\"WanUserName\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"WanPassword\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"SipAccount\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"SipPassword\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"ImsDomainName\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"ImsProxyIP\" type=\"string\" constraint=\"15\"/>\n\
      <schema name=\"ImsProxyPort\" type=\"string\" constraint=\"5\"/>\n\
      <schema name=\"FtpIP\" type=\"string\" constraint=\"15\"/>\n\
      <schema name=\"FtpPort\" type=\"string\" constraint=\"5\"/>\n\
      <schema name=\"FtpUserName\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"FtpPassword\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"PlatformDomain\" type=\"string\" constraint=\"64\"/>\n\
    </schema>\n\
    <schema name=\"DeviceInfo\" type=\"object\">\n\
      <schema name=\"Manufacturer\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"OUI\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"ProductClass\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"SerialNumber\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"FirmwareVersion\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"HardwareVersion\" type=\"string\" constraint=\"64\" inform=\"true\"/>\n\
      <schema name=\"SoftwareVersion\" type=\"string\" constraint=\"64\" inform=\"true\"/>\n\
      <schema name=\"DeviceNo\" type=\"string\" constraint=\"64\"/>\n\
    </schema>\n\
    <schema name=\"UserInterface\" type=\"object\">\n\
      <schema name=\"PasswordRequired\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"PasswordUserSelectable\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"UpgradeAvailable\" type=\"boolean\" write=\"W\"/>\n\
      <schema name=\"WarrantyDate\" type=\"dateTime\" write=\"W\"/>\n\
      <schema name=\"ISPName\" type=\"string\" constraint=\"64\" write=\"W\"/>\n\
      <schema name=\"ISPHelpDesk\" type=\"string\" constraint=\"32\" write=\"W\"/>\n\
      <schema name=\"ISPHomePage\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"ISPHelpPage\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"ISPLogo\" type=\"base64\" constraint=\"5460\" write=\"W\"/>\n\
      <schema name=\"ISPLogoSize\" type=\"unsignedInt\" constraint=\"0:4095\" write=\"W\"/>\n\
      <schema name=\"ISPMailServer\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"TextColor\" type=\"string\" constraint=\"6\" write=\"W\"/>\n\
      <schema name=\"BackgroundColor\" type=\"string\" constraint=\"6\" write=\"W\"/>\n\
      <schema name=\"ButtonColor\" type=\"string\" constraint=\"6\" write=\"W\"/>\n\
      <schema name=\"ButtonTextColor\" type=\"string\" constraint=\"6\" write=\"W\"/>\n\
      <schema name=\"AutoUpdateServer\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"UserUpdateServer\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"ExampleLogin\" type=\"string\" constraint=\"40\" write=\"W\"/>\n\
      <schema name=\"ExamplePassword\" type=\"string\" constraint=\"30\" write=\"W\"/>\n\
    </schema>\n\
  </schema>\n\
</model>\n\
";

void TMCreateModel()
{
	FILE *fp;
	//Model生成
	fp = fopen(TM_MODEL_PATH, "w+");
	fwrite(pcModel, sizeof(char), strlen(pcModel), fp);
	fclose(fp);
#ifdef _GYY_I5_
	fp = fopen(BA_MODEL_PATH, "w+");
	fwrite(pcBaModel, sizeof(char), strlen(pcBaModel), fp);
	fclose(fp);
#endif
}

/*
void TMUpdateAlarmState(TM_ALARM_ID_EN enModule, char cState)
{
    char* name;
    char* value;
    switch(enModule)
    {
    case TM_ALARM_IMS_REGISTER:
        name = ".AlarmStatus.ImsRegisterFail";
        break;
    case TM_ALARM_SIP_CALL:
        name = ".AlarmStatus.SipCallFail";
        break;
    case TM_ALARM_IF_BOARD:
        name = ".AlarmStatus.IfBoardComFail";
        break;
    case TM_ALARM_TRANSFER:
        name = ".AlarmStatus.FileTransFail";
        break;
    case TM_ALARM_UPDATERESULT:
        name = ".AlarmStatus.UpdateResult";
        break;
    default:
        break;
    }
    if(cState == 0)
        value = "0";
    else
        value = "1";
    evcpe_repo_set(this.cpe->repo, name, value, sizeof(char));
    this.cpe->alarm_change = 1;
}*/

#endif

/**@}*///For SM module
