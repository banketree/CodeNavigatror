/*
 * TMTr069.c
 *
 *  Created on: 2012-5-20
 *      Author: root
 */

#include <YKApi.h>
#include <IDBT.h>

#if _TM_TYPE_ == _GYY_I6_
#include <TMInterface.h>
#include <LOGIntfLayer.h>
#include "sd/domnode.h"
#include <GI6Interface.h>
#include <https.h>
#include <YKMsgQue.h>
#include <Des.h>
#include <evcpe.h>
#include "IDBTCfg.h"

extern YK_MSG_QUE_ST*  g_pstTMMsgQ;
extern TR_EVCPE_ST g_tr_evcpe_st;

sd_domnode_t* g_HouseInfo = NULL;
sd_domnode_t* g_HouseInfoTemp = NULL;
sd_domnode_t* g_Entry = NULL;
sd_domnode_t* g_NowCall = NULL;
sd_domnode_t* g_NextCall = NULL;
TM_I6NUM_INFO_ST stNowI6NumInfo;
int g_bGetCallBegin = 0;
int g_bGetCallEnd = 1;
int g_bReloadConfig = 0;
int g_bLoadingConfig = 0;
/**
 *@brief Free memory which malloc by TMCCGetHouseInfoByHouseCode
 *@param[in] house The handle of house.
 *@return void.
 */
int TMI6ConfigClose()
{
	if(g_HouseInfo)
	{
//		if(g_bGetCallEnd)
//		{
//			sd_domnode_delete(g_HouseInfo);
//		}
		g_HouseInfo = NULL;
		g_bReloadConfig = 1;
	}
}

void TMI6ConfigOpen()
{
	if(!g_bLoadingConfig)
	{
		g_bLoadingConfig = 1;
//		TMI6ConfigClose();

		g_HouseInfo = sd_domnode_new(NULL, NULL);
		if(-1 == sd_domnode_load(g_HouseInfo, I6_CONFIG_PATH))
		{
			g_bLoadingConfig = 0;
			sd_domnode_delete(g_HouseInfo);
			g_HouseInfo = NULL;
			return;
		}
		g_bReloadConfig = 1;
		g_bLoadingConfig = 0;
	}
}

#define I5_SERVER_URL	"http://172.16.9.8:80/door/servlet/acsServlet"
#define I6_SERVER_URL	"http://172.16.9.8:80/door/servlet/imsServlet"
#define I5_SERVER_PATH	"/door/servlet/acsServlet"
#define I6_SERVER_PATH	"/door/servlet/imsServlet"
//"http://183.62.12.22:8078/ims_ws/TerminalServlet","0814000015100000003","123456","YK","001E10","MTH1BAS","00100300YK210010030STF8W0A920311"

//序列号规则 安卓梯口机 前16 0000030000000010 后16 591001YYMMDD0NNN
//         BV		 前16 0000050000000010  后16 591001YYMMDD0NNN
//		   GSM梯口机  前16 0000080000000010  后16 591001YYMMDD0NNN

//配置文件
sd_domnode_t * NewI5Data()
{
	char *new_file = "\
<?xml version=\"1.0\"?>\r\n\
<Device>\r\n\
    <ManagementServer>\r\n\
        <I6URL><Value>"I6_SERVER_URL"</Value></I6URL>\r\n\
        <URL><Value>"I5_SERVER_URL"</Value></URL>\r\n\
        <Timeout><Value>30</Value></Timeout>\r\n\
        <Username><Value>newab</Value></Username>\r\n\
        <Password><Value>123456789</Value></Password>\r\n\
        <FaultCode><Value></Value></FaultCode>\r\n\
        <PeriodicInformEnable><Value>1</Value></PeriodicInformEnable>\r\n\
        <PeriodicInformInterval><Value>60</Value></PeriodicInformInterval>\r\n\
        <PeriodicInformTime><Value>0001-01-01T00:00:00</Value></PeriodicInformTime>\r\n\
        <ParameterKey><Value></Value></ParameterKey>\r\n\
        <UpgradesManaged><Value>0</Value></UpgradesManaged>\r\n\
        <Authentication><Value>BASIC</Value></Authentication>\r\n\
        <BootStrap><Value>0</Value></BootStrap>\r\n\
    </ManagementServer>\r\n\
    <Communication>\r\n\
        <WanUserName><Value></Value></WanUserName>\r\n\
        <WanPassword><Value></Value></WanPassword>\r\n\
        <SipAccount><Value>123</Value></SipAccount>\r\n"
#ifdef BASE64_3DES
"    	<SipPassword><Value>NEQwMkE5NUUyQjBERUMyOA==</Value></SipPassword>\r\n"
#else
"    	<SipPassword><Value>123</Value></SipPassword>\r\n"
#endif
"    	<ImsDomainName><Value>61.154.9.84</Value></ImsDomainName>\r\n\
        <ImsProxyIP><Value>61.154.9.84</Value></ImsProxyIP>\r\n\
        <ImsProxyPort><Value>5060</Value></ImsProxyPort>\r\n\
        <FtpIP><Value>42.120.52.115</Value></FtpIP>\r\n\
        <FtpPort><Value>21</Value></FtpPort>\r\n\
        <FtpUserName><Value>ladderlogs</Value></FtpUserName>\r\n\
        <FtpPassword><Value>123456</Value></FtpPassword>\r\n\
        <PlatformDomain><Value>42.120.52.115</Value></PlatformDomain>\r\n\
    </Communication>\r\n\
    <DeviceInfo>\r\n\
        <Manufacturer><Value>YK</Value></Manufacturer>\r\n\
        <OUI><Value>001E10</Value></OUI>\r\n\
        <ProductClass><Value>MTH1BAS</Value></ProductClass>\r\n\
        <SerialNumber><Value>00000300000000105910011308140001</Value></SerialNumber>\r\n\
        <FirmwareVersion><Value>"YK_APP_VERSION"</Value></FirmwareVersion>\r\n\
        <KernelVersion><Value>"YK_KERNEL_VERSION"</Value></KernelVersion>\r\n\
        <HardwareVersion><Value>"YK_KERNEL_VERSION"</Value></HardwareVersion>\r\n\
        <SoftwareVersion><Value>"YK_APP_VERSION"</Value></SoftwareVersion>\r\n\
    </DeviceInfo>\r\n\
</Device>\r\n";

	FILE *fp;
	sd_domnode_t*   document;
	if(-1 == access(BA_DATA_PATH,0))
	{
		document = sd_domnode_new(NULL, NULL);
		fp = fopen(BA_DATA_PATH,"w");
		fwrite(new_file,1,strlen(new_file),fp);
		fflush(fp);
		sd_domnode_fwrite(document,fp);
		close(fp);
		sd_domnode_delete(document);
	}
	return NULL;
}

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

//YKHandle TMCCGetHouseInfoByHouseCode(const char* pcHouseCode)
//{
//	return NULL;
//}
///**
// *@brief Free memory which malloc by TMCCGetHouseInfoByHouseCode
// *@param[in] house The handle of house.
// *@return void.
// */
//void TMCCDestroyHouseInfo(YKHandle hHouse)
//{
//	if(hHouse == NULL)
//	{
//		return ;
//	}
//	YKFree(hHouse);
//}
/**
 *@brief Judge is need to play early media.
 *@param[in] house The handle of house.
 *@return TRUE indicates need play early media else not.
 */
BOOL TMCCIsInCommingCallNotice(YKHandle hHouse)
{
	return FALSE;
}
/**
 *@brief Get the number of phone.
 *@param[in] house The handle of house.
 *@return The count of phone.
 */
int TMCCGetPhoneCount(YKHandle hHouse)
{
	return 0;
}
/**
 *@brief Get special phone info.
 *@param[in] house The handle of house.
 *@param[in] iIndex From 0.
 *@return The handle of phone info. NULL indicate failed else indicate success.
 */
YKHandle TMCCGetPhoneInfoByIndex(YKHandle hHouse, int iIndex)
{
	return NULL;
}
/**
 *@brief Judge is support VOIP.
 *@param[in] phone The handle of phone.
 *@return TRUE if is support VOIP.
 */
BOOL TMCCIsVoipEnable(YKHandle hPhone)
{
	return FALSE;
}

BOOL TMNMGetDomainName(char* pcDomain)
{
	return I6GetDomainName(pcDomain);
}
/**
 *@brief Get CS phone number.
 *@param[in] phone The handle of phone.
 *@return CS phone number.
 */
char g_acTMPhoneNum[128] = {0x0};

char *I6CCGeNumNoSort(const char* pcHouseCode)
{
//	sd_domnode_t*   document = sd_domnode_new(NULL, NULL);
	sd_domnode_t* phonebook;
	sd_domnode_t* child;
	sd_domnode_t* room = NULL;
	char acPhoneTemp[64] = {0x0};
	char acDomain[64] = {0x0};

	if(!g_HouseInfo)
	{
		TMI6ConfigOpen();
	}

	if(!g_HouseInfo)
	{
		return g_HouseInfo;
	}
//	if(-1 == sd_domnode_load(document, I6_CONFIG_PATH)){
//		return NULL;
//	}
	phonebook = sd_domnode_search(g_HouseInfo, "PHONE_BOOK");//find phone book node.
	child = sd_domnode_first_child(phonebook);
	while(child){
		if( child != NULL &&
			(room = sd_domnode_attrs_get(child, "Cornet")) &&
			strcmp(room->value, pcHouseCode) == 0){
				strcpy(acPhoneTemp, sd_domnode_attrs_get(child, "Code")->value);
				//判断是否有＠后缀
				if(strstr(acPhoneTemp,"@") == NULL)
				{
					if(TMNMGetDomainName(acDomain))
					{
						if(strlen(acPhoneTemp) == 18)
						{
							sprintf(g_acTMPhoneNum,"+86%s@%s",acPhoneTemp,acDomain);
							break;
						}
						sprintf(g_acTMPhoneNum,"%s@%s",acPhoneTemp,acDomain);
						break;
					}
				}
				strcpy(g_acTMPhoneNum,acPhoneTemp);
				break;
		}
		child = sd_domnode_next_child(phonebook, child);
	}
//	sd_domnode_delete(document);
}

const char* TMCCGetIMSPhoneNumber(const char* pcHouseCode)
{
    char acHouseCode[64];
	char acRoomCode[20] = {0x0};
	char acBuildingCode[8] = {0x0};
	char acUnitCode[8] = {0x0};

	LOG_RUNLOG_DEBUG("TMCCGetIMSPhoneNumber GET HOUSECODE :%s",pcHouseCode);
    if(pcHouseCode == NULL)
	{
		return NULL;
	}
    memset(g_acTMPhoneNum,0,128);
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
		return NULL;
	}
	else if(!strncmp(acHouseCode, "C", strlen("C")))
	{
		return NULL;
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

#ifdef CONFIG_SORT
	//如果还未排序 则调用 如果已排序
	if(IsSortConfigCornetOK())
	{
		I6CCGeNumSort(acRoomCode);
	}
	else
#endif
	{
		I6CCGeNumNoSort(acRoomCode);
	}

    if(g_acTMPhoneNum[0] == 0x0)
    {
    	return NULL;
    }
    return g_acTMPhoneNum;
}

const char* TMCCGetCsPhoneNumber(YKHandle hPhone)
{
	return NULL;
}
/**
 *@brief Get SIP phone number.
 *@param[in] phone The handle of phone.
 *@return SIP phone number.
 */
const char* TMCCGetSipPhoneNumber(YKHandle hPhone)
{
	return NULL;
}
/**
 *@brief Judge if is in not disturb time.
 *@param[in] phone The handle of phone.
 *@param[in] currentTime Current time.
 *@return TRUE indicate is in not disturb time else indicate not.
 */
BOOL TMCCIsNotDisturbTime(YKHandle hPhone)
{
	return FALSE;
}

#define HOUSE_CODE_LOG_LEN 12

BOOL I6CCIsValidNumNoSort(const char* pcPhoneNum, char* pcHouseCode)
{
//	sd_domnode_t*   document = sd_domnode_new(NULL, NULL);
	sd_domnode_t* phonebook;
	sd_domnode_t* child;
	sd_domnode_t* room = NULL;
	sd_domnode_t* call = NULL;
	sd_domnode_t* Cornet = NULL;
	BOOL bOk = FALSE;

	if(!g_HouseInfo)
	{
		TMI6ConfigOpen();
	}

	if(!g_HouseInfo)
	{
		return g_HouseInfo;
	}
//	if(-1 == sd_domnode_load(document, I6_CONFIG_PATH)){
//		return NULL;
//	}
	phonebook = sd_domnode_search(g_HouseInfo, "PHONE_BOOK");//find phone book node.
	child = sd_domnode_first_child(phonebook);
	while(child)
	{
		if( (room = sd_domnode_attrs_get(child, "Code")) &&
			strstr(room->value, pcPhoneNum) != NULL)
		{
				bOk = TRUE;
				break;
		}
		if( (call = sd_domnode_first_child( child )))
		{
			while(call)
			{
				if( (room = sd_domnode_attrs_get(call, "Num")) &&
						strstr(room->value, pcPhoneNum) != NULL )
				{
					bOk = TRUE;
					break;
				}

				call = sd_domnode_next_child(child, call);
			}
		}

		if(bOk == TRUE)
		{
			break;
		}

		child = sd_domnode_next_child(phonebook, child);
	}

	if(bOk == TRUE)
	{
		if((Cornet = sd_domnode_attrs_get(child, "Cornet")) != NULL)
		{
			if(Cornet->value != NULL && pcHouseCode !=NULL)
			{
				strcpy(pcHouseCode, Cornet->value);
			}
		}
	}

//	sd_domnode_delete(document);
	return bOk;
}

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

	if(pcPhoneNum == NULL)
	{
		return FALSE;
	}

	if(strlen(pcPhoneNum) == 0)
	{
		return FALSE;
	}

	LOG_RUNLOG_DEBUG("TM CCTM PhoneNum is %s", pcPhoneNum);

#ifdef CONFIG_SORT
	//如果还未排序 则调用 如果已排序
	if(IsSortConfigCodeOK())
	{
		return I6CCIsValidNumSort(pcPhoneNum,pcHouseCode);
	}
	else
#endif
	{
		return I6CCIsValidNumNoSort(pcPhoneNum,pcHouseCode);
	}
}

char g_acTMHouseCode[32] = {0x0};

const char* TMCCGetPhoneNumByType(int type)
{
//	sd_domnode_t*   document = sd_domnode_new(NULL, NULL);
	sd_domnode_t* phonebook;
	sd_domnode_t* child;
	sd_domnode_t* stType = NULL;

	if(type == 0)
	{
		return NULL;
	}

	memset(g_acTMHouseCode,0,32);

	if(!g_HouseInfo)
	{
		TMI6ConfigOpen();
	}

	if(!g_HouseInfo)
	{
		return g_HouseInfo;
	}
//	if(-1 == sd_domnode_load(document, I6_CONFIG_PATH)){
//		return NULL;
//	}
	phonebook = sd_domnode_search(g_HouseInfo, "PHONE_BOOK");//find phone book node.
	child = sd_domnode_first_child(phonebook);
	while(child){
		if( child != NULL &&
			(stType = sd_domnode_attrs_get(child, "Type"))){
				if(atoi(stType->value) == type)
				{
					strcpy(g_acTMHouseCode, sd_domnode_attrs_get(child, "Cornet ")->value);
					break;
				}
		}
		child = sd_domnode_next_child(phonebook, child);
	}
//	sd_domnode_delete(document);
	if(g_acTMHouseCode[0] == 0x0)
	{
		return NULL;
	}
	return g_acTMHouseCode;
}

/**
 *@brief Get to call timeout value.
 *@param[in] void.
 *@return timeout value.
 */
int  TMCCGetIMSCallTimeOut(void)
{
	return 60;
}

/**
 *@brief To judge whether the IMS-enabled
 *@param[in] void
 *@return TRUE indicates enable else not.
 */

/**
 *@brief To judge whether the IMS-enabled
 *@param[in] void
 *@return TRUE indicates enable else not.
 */
BOOL TMCCIsIMSCallEnabled(void)
{
	return TRUE;
}

/**
 *@brief To judge whether the VOIP Monitor-enabled.
 *@param[in] void.
 *@return TRUE indicates enable else not.
 */
BOOL  TMCCIsKeyOpenDoorEnabled(void)
{
	return TRUE;
}

/**
 *@brief To judge whether the VOIP Monitor-enabled
 *@param[in] void
 *@return TRUE indicates enable else not.
 */
BOOL TMCCIsVoipMonitorEnabled(void)
{
	return TRUE;
}

BOOL TMCCIsValidDemoPhoneNum(const char* pcPhoneNum, char* pcHouseCode)
{
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
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t *search,*child;

	if(-1 == sd_domnode_load(document, BA_DATA_PATH)){
		sd_domnode_delete(document);
		return ;
	}

	search = sd_domnode_search(document,"WanUserName");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value && pcUserName)
		{
			strcpy(pcUserName,child->value);
		}
	}

	search = sd_domnode_search(document,"WanPassword");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value && pcPassWord)
		{
			strcpy(pcPassWord,child->value);
		}
	}

	sd_domnode_delete(document);
	return ;
}
/**@}*///For NM module.

void TMGetIMSInfo2(char* pcUserName)
{
	I6GetIMSInfo2(pcUserName);
}


/**
 *@brief Get IMS pcDomain.
 *@param[in] pcDomain For received data.
 *@return void.
 */
void TMGetIMSInfo(char* pcUserName, char* pcPassWord, char* pcProxy, int* piPort, char* pcDomain)
{
	char acPwd[256];

	I6GetIMSInfo(pcUserName,acPwd,pcProxy,piPort,pcDomain);

#ifdef BASE64_3DES
	char acImsPwd[256] = {0x0};
	char* pcBase64;
	char* pcBaseHex;
	char* bufferout = (char *)malloc(1024);
	int j= 0,iBase64 = 0;
	memset(bufferout,0,1024);
	strcpy(acImsPwd,acPwd);
	pcBase64 = Base64Decode(acImsPwd);
//	LOG_RUNLOG_DEBUG("TM =%s==",pcBase64);
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
//		printf("%s--%s\n",pcPassWord,bufferout);
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
	strcpy(pcPassWord,acPwd);
#endif
}

void TMGetUrl(char *pcHost)
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t *search,*child;

	if(-1 == sd_domnode_load(document, BA_DATA_PATH)){
		sd_domnode_delete(document);
		return ;
	}

	search = sd_domnode_search(document,"I6URL");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value && pcHost)
		{
			strcpy(pcHost,child->value);
		}
	}
	else
	{
		strcpy(pcHost,I6_SERVER_URL);
	}

	sd_domnode_delete(document);
	return ;
}
//
//void TMGetHost(char *pcHost)
//{
//	char buf[128];
//	char *host;
//	TMGetUrl(buf);
//
//	if (!strncmp(buf, "http://", strlen("http://")))
//	{
//		host = strtok(buf+7,":");
//		strcpy(pcHost,host);
//	}
//	else if (!strncmp(buf, "https://", strlen("https://")))
//	{
//		host = strtok(buf+8,":");
//		strcpy(pcHost,host);
//	}
//}

void TMGetHost(char *pcHost,int *port)
{
    char *pA;
    char *pB;
    char acWeb[64];
    char acHost[64];
    int flag = 0;

    if (!pcHost)
    {
        return;
    }
    TMGetUrl(acHost);
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

BOOL TMNMGetSN(char* pcSN)
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t *search,*child;

	if(-1 == sd_domnode_load(document, BA_DATA_PATH)){
		sd_domnode_delete(document);
		return FALSE;
	}
	search = sd_domnode_search(document,"SerialNumber");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value && pcSN)
		{
			strcpy(pcSN,child->value);
		}
	}

	sd_domnode_delete(document);
	return TRUE;
}

/**
 *@name Query device id.
 *@param[in] id For received id.
 *@return void.
 */
void TMQueryDeviceID(char* pcID)
{
	TMNMGetSN(pcID);
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
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t *search,*child;

	if(-1 == sd_domnode_load(document, BA_DATA_PATH)){
		sd_domnode_delete(document);
		return ;
	}

	search = sd_domnode_search(document,"FtpIP");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value && pcHost)
		{
			strcpy(pcHost,child->value);
		}
	}

	search = sd_domnode_search(document,"FtpPort");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value && piPort)
		{
			sscanf(child->value,"%d",piPort);
		}
	}

	search = sd_domnode_search(document,"FtpUserName");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value && pcUserName)
		{
			strcpy(pcUserName,child->value);
		}
	}

	search = sd_domnode_search(document,"FtpPassword");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value && pcPassWord)
		{
			strcpy(pcPassWord,child->value);
		}
	}

	sd_domnode_delete(document);
	return ;
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
	strcpy(pcVersion, YK_APP_VERSION);
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
    sd_domnode_t* document = sd_domnode_new(NULL, NULL);
    sd_domnode_t* password , *state , *search;
    sd_domnode_t* child;
    char* parray[2] = {"commonPassword", "privatePassword"};
    int index = 0;
    BOOL bOk = FALSE;
    if(!Password)
    {
    	return FALSE;
    }
    if(-1 == sd_domnode_load(document, I6_PASSWORD_PATH)){
    	printf("password xml is error\n");
    	sd_domnode_delete(document);
        return FALSE;
    }

    search = sd_domnode_search(document, "message");
    child = sd_domnode_first_child(search);
    while(child)
    {
    	if(child->name && (strcmp(child->name,parray[0])== 0
    			|| strcmp(child->name,parray[1]) == 0))
    	{
    		state = sd_domnode_first_child(child);
    		if(state && state->value && strcmp(state->value, "1") == 0)
    		{
        		password = sd_domnode_next_child(child,state);
        		if(password && password->value && strcmp(password->value, Password) == 0){
                    bOk = TRUE;
                    break;
                }
    		}
    	}
    	child = sd_domnode_next_child(search, child);
    }
    sd_domnode_delete(document);
    return bOk;
}

/**
 *@name For SM module
 *@{
 */
char *findID(const sd_domnode_t* document,
const sd_domnode_t*node,
const char* attr,
const char* attrValue){

    sd_domnode_t* find;
    sd_domnode_t* child = sd_domnode_first_child(node);
    while(child){
        if( child != NULL &&
            (find = sd_domnode_attrs_get(child, attr)) &&
            strcmp(find->value, attrValue) == 0){
            return sd_domnode_attrs_get(child, "id")->value;
        }
        child = sd_domnode_next_child(node, child);
    }
    return NULL;
}

sd_domnode_t *findAttr(const sd_domnode_t* document,
const sd_domnode_t*node,
const char* attr,
const char* attrValue){

    sd_domnode_t* find;
    sd_domnode_t* child = sd_domnode_first_child(node);
    while(child){
        if( child != NULL &&
            (find = sd_domnode_attrs_get(child, attr)) &&
            strcmp(find->value, attrValue) == 0){
            return find;
        }
        child = sd_domnode_next_child(node, child);
    }
    return NULL;
}
/**
 *@brief Judge is valid serial number.
 *@param[in] pcSerinalNumber The serial number.
 *@return TRUE indicate is valid else is not.
 */

int TRMsgSendI6CardSwip(char *pcBuf, int len)
{
    int iRet = -1;

    if(NULL == g_pstTMMsgQ)
    {
        return -1;
    }

    TM_I6_CARD_SWIP_ST* pstMsg = (TM_I6_CARD_SWIP_ST*)malloc(sizeof(TM_I6_CARD_SWIP_ST));
    if( NULL == pstMsg )
    {
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(TM_I6_CARD_SWIP_ST));
    pstMsg->uiPrmvType = TM_I6_CARD_SWIP;
    memcpy(&pstMsg->aucCardInfo,pcBuf,len);

    iRet = YKWriteQue( g_pstTMMsgQ,  pstMsg,  250);


    return iRet;
}

BOOL TMSMIsValidICIDCard(const char* pcSerinalNumber)
{
	I6_CARD_LOG_ST card_log;
    sd_domnode_t* document = sd_domnode_new(NULL, NULL);
    sd_domnode_t* card;
    sd_domnode_t* find ,*cardswip,*bOK1;
    int ret = FALSE;
    char *p;
    if(-1 == sd_domnode_load(document, I6_CARD_PATH)){
    	printf("card xml is error\n");
    	sd_domnode_delete(document);
        return ret;
    }
    //in addcard?
    card = sd_domnode_search(document, "cardAdd");
    find = findAttr(document, card, "no", pcSerinalNumber);
    if(find)
    {
    	ret = TRUE;
    }
    while(find){
        card = sd_domnode_search(document, "blackCard");
        if(findAttr(document, card, "no", pcSerinalNumber)){
        	find = NULL;
        	ret = FALSE;
            break;
        }
        break;
    }

    //刷卡上报
	memset(&card_log,0,sizeof(I6_CARD_LOG_ST));
	do
	{
		card = sd_domnode_search(document, "cardAdd");
		find = findAttr(document, card, "no", pcSerinalNumber);
		if(find)
		{
			strcpy(card_log.acCardNo,pcSerinalNumber);
			if((p = findID(document, card, "no", pcSerinalNumber))!= NULL)
			{
				strcpy(card_log.acCardId,p);
			}
			strcpy(card_log.acStateCode,"1");
			break;
		}
		card = sd_domnode_search(document, "blackCard");
		find = findAttr(document, card, "no", pcSerinalNumber);
		if(find)
		{
			strcpy(card_log.acCardNo,pcSerinalNumber);
			if((p = findID(document, card, "no", pcSerinalNumber))!= NULL)
			{
				strcpy(card_log.acCardId,p);
			}
			strcpy(card_log.acStateCode,"0");
			break;
		}
	}while(0);

	if(find == NULL)
	{
		strcpy(card_log.acCardNo,pcSerinalNumber);
		strcpy(card_log.acCardId,"0");
		strcpy(card_log.acStateCode,"0");
	}

	//刷卡上報
	TRMsgSendI6CardSwip(&card_log, sizeof(I6_CARD_LOG_ST));

    sd_domnode_delete(document);
    return ret;
}

BOOL TMSMIsValidPropertyCard(const char* pcSerinalNumber)
{
	return FALSE;
}

BOOL  TMSMIsICOpenDoorEnabled(void)
{
	return TRUE;
}

BOOL  TMSMIsPasswordOpenDoorEnabled(void)
{
	return TRUE;
}

BOOL  TMCCIsPasswordOpendoorEnabled(void)
{
	return TRUE;
}

#define BUF_SIZE 1024
char pStrTmp[BUF_SIZE] = {0x0};
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

int TMGetVersionInfo(char *pStr)
{
	sprintf(pStr,"%s|%s|%s|",YK_APP_VERSION,YK_HARDWARE_VERSION,YK_KERNEL_VERSION);
	LOG_RUNLOG_DEBUG("TMGetVersionInfo : %s",pStr);
	return 0;
}

int TMSetPPPOEUername(const char *str)
{
	int ret;
	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}

	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.WanUserName", str, strlen(str));

	return ret;
}

int TMSetPPPOEPassword(const char *str)
{
	int ret;
	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}

	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.WanPassword", str, strlen(str));

	return ret;
}

char *TMGetPlatformInfo(void)
{
	const char *value;
	unsigned int len;
	char *p = pStrTmp;
	if(!pStrTmp) return NULL;
	memset(pStrTmp,0,BUF_SIZE);
	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.SerialNumber", &value, &len);
	if(value && len){
		sprintf(p,"%s",value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".ManagementServer.I6URL", &value, &len);
	if(value && len){
		sprintf(p,"%s",value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".ManagementServer.URL", &value, &len);
	if(value && len){
		sprintf(p,"%s",value);
		p+=len;
		*p++ = '|';
	}

	evcpe_repo_get(g_tr_evcpe_st.repo, ".ManagementServer.Username", &value, &len);
	if(value && len){
		sprintf(p,"%s",value);
		p+=len;
//		*p++ = '|';
	}
//
//	evcpe_repo_get(g_tr_evcpe_st.repo, ".ManagementServer.Password", &value, &len);
//	if(value && len){
//		sprintf(p,"%s",value);
//		p+=len;
//	}

	return pStrTmp;

}

int TMUpdateMac(const char *str)
{
	char acBuf[64] = {0x0};
	char acMac[18] = {0x0};
	if(strlen(str) == 32)
	{
		memcpy(acBuf,str + 16,16);
		acBuf[16] = 0x0;
		LOG_RUNLOG_DEBUG("TM TMUpdateMac local : %s, acBuf:%s", g_stIdbtCfg.acSn,acBuf);
		if(strcmp(g_stIdbtCfg.acSn,acBuf) != 0)
		{
			if (serialNumToMac(g_stIdbtCfg.acSn, acMac, 18) == 0)
			{
				strcpy(g_stIdbtCfg.acSn,acBuf);
				strcpy(g_stIdbtCfg.stNetWorkInfo.acMac, acMac);
				SaveIdbtConfig();
				LOG_RUNLOG_DEBUG("TM TMUpdateMac set acMac:%s", acMac);
				//通知更新
				notifyUpdateMac();
			}
		}
	}
}

//待补
int TMSetDevNum(const char *str)
{
	int ret;
	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".DeviceInfo.SerialNumber", str, strlen(str));
	evcpe_persister_persist(g_tr_evcpe_st.persist);

	TMUpdateMac(str);

	return ret;
}

int TMSetPlatformDomainName(const char *str)
{
	int ret;
	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.I6URL", str, strlen(str));
	evcpe_persister_persist(g_tr_evcpe_st.persist);
	return ret;
}

int TMSetI5PlatformDomainName(const char *str)
{
	int ret;
	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.URL", str, strlen(str));
	evcpe_persister_persist(g_tr_evcpe_st.persist);
	return ret;
}

int TMSetPlatformIP(const char *str)
{
	int ret;
	const char *value;
	unsigned int len;
	char acBuf[128] = {0x0};
	char acHost[64] = {0x0};
	int iPort = 0;

	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}
	TMGetHost(acHost,&iPort);
	sprintf(acBuf,"http://%s:%d%s",str,iPort,I6_SERVER_PATH);
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.I6URL", acBuf, strlen(acBuf));
	LOG_RUNLOG_DEBUG("TM TMSetPlatformIP Platform:%s ,ret:%d",acBuf,ret);
	sprintf(acBuf,"http://%s:%d%s",str,iPort,I5_SERVER_PATH);
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.URL", acBuf, strlen(acBuf));
	LOG_RUNLOG_DEBUG("TM TMSetPlatformIP Platform:%s ,ret:%d",acBuf,ret);
	evcpe_persister_persist(g_tr_evcpe_st.persist);
	return ret;
}

int TMSetPlatformPort(const char *str)
{
	int ret;
	const char *value;
	unsigned int len;
	char acBuf[128] = {0x0};
	char acHost[64] = {0x0};
	int iPort = 0;

	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}
	TMGetHost(acHost,&iPort);
	sprintf(acBuf,"http://%s:%d%s",acHost,atoi(str),I6_SERVER_PATH);
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.I6URL", acBuf, strlen(acBuf));
	LOG_RUNLOG_DEBUG("TM TMSetPlatformPort Platform:%s ,ret:%d",acBuf,ret);
	sprintf(acBuf,"http://%s:%d%s",acHost,atoi(str),I5_SERVER_PATH);
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.URL", acBuf, strlen(acBuf));
	evcpe_persister_persist(g_tr_evcpe_st.persist);
	LOG_RUNLOG_DEBUG("TM TMSetPlatformPort Platform:%s ,ret:%d",acBuf,ret);
	return ret;
}

int TMSetPlatformUserName(const char *str)
{
	int ret;
	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.Username", str, strlen(str));
	evcpe_persister_persist(g_tr_evcpe_st.persist);
	return ret;
}

int TMSetPlatformPassword(const char *str)
{
	int ret;
	if(!g_tr_evcpe_st.repo)
	{
		return NULL;
	}
	ret = evcpe_repo_set(g_tr_evcpe_st.repo, ".ManagementServer.Password", str, strlen(str));
	evcpe_persister_persist(g_tr_evcpe_st.persist);
	return ret;
}

char *TMGetSIPInfo(void)
{
	int iPort;
	char acUser[256];
	char acPwd[256];
	char acProxy[128];
	char acDomain[128];
	char *p = pStrTmp;

	I6GetIMSInfo(acUser,acPwd,acProxy,&iPort,acDomain);

	sprintf(pStrTmp,"%s|%s|%s|%d",acUser,acDomain,acProxy,iPort);

	return pStrTmp;
//	const char *value;
//	unsigned int len;
//	char *p = pStrTmp;
//	if(!pStrTmp) return NULL;
//	memset(pStrTmp,0,BUF_SIZE);
//	if(!g_tr_evcpe_st.repo)
//	{
//		return NULL;
//	}
//
//	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.SipAccount", &value, &len);
//	if(value && len<60){
//		sprintf(p,"%s",value);
//		p+=len;
//		*p++ = '|';
//	}
//
//	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.ImsDomainName", &value, &len);
//	if(value && len<60){
//		sprintf(p,"%s",value);
//		p+=len;
//		*p++ = '|';
//	}
//
//	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.ImsProxyIP", &value, &len);
//	if(value && len<60){
//		sprintf(p,"%s",value);
//		p+=len;
//		*p++ = '|';
//	}
//
//	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.ImsProxyPort", &value, &len);
//	if(value && len<60){
//		sprintf(p,"%s",value);
//		p+=len;
//	}
//
//	return pStrTmp;
}

void TMUseLocalSipInfo()
{
	I6UseLocalSipInfo();
}

#define ICID_LOG_SIZE 26
#define ICID_LOG_MAX_SIZE 1024

void TMSetICIDOpenDoor(const char* pcSerinalNumber)
{
	return;
}

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
      <schema name=\"I6URL\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"URL\" type=\"string\" constraint=\"256\" write=\"W\" inform=\"true\" notification=\"active\"/>\n\
      <extension name=\"Timeout\" type=\"unsignedInt\"/>\n\
      <schema name=\"Username\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"Password\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
      <schema name=\"FaultCode\" type=\"string\" constraint=\"256\" write=\"W\"/>\n\
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
      <schema name=\"KernelVersion\" type=\"string\" constraint=\"64\"/>\n\
      <schema name=\"HardwareVersion\" type=\"string\" constraint=\"64\" />\n\
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
	fp = fopen(BA_MODEL_PATH, "w+");
	fwrite(pcBaModel, sizeof(char), strlen(pcBaModel), fp);
	fclose(fp);
}

void TMCCDestroyHouseInfo(YKHandle hHouse)
{

}

//退出呼叫时候销毁內存数据
void TMDestroyHouseInfo()
{
//	 sd_domnode_delete(g_HouseInfo);
//	 g_HouseInfo = NULL;
	 g_Entry = NULL;
	 g_NowCall = NULL;
	 g_bGetCallBegin = 0;
	 g_bGetCallEnd = 0;
}

#define DEV_TYPE_LEN 1
#define BUILDING_CODE_LEN 2
#define UNIT_CODE_LEN  1
#define ROOM_CODE_LEN 4
#define HOUSE_CODE_LEN DEV_TYPE_LEN +BUILDING_CODE_LEN +UNIT_CODE_LEN+ ROOM_CODE_LEN

int TMCCFilterHouseCode(const char* pcHouseCode,char* pcHouseCodeOut)
{
	char acHouseCode[64];
	int len = 0;

	if(pcHouseCode == NULL)
	{
		return FALSE;
	}

	len = strlen(pcHouseCode) - 1;
	memcpy(acHouseCode, pcHouseCode + 1, len);
	acHouseCode[len] = 0x0;

	if(pcHouseCode[0] == '0')
	{
		if(len == ROOM_CODE_LEN)
		{
			strcpy(pcHouseCodeOut,acHouseCode);
		}
		else if(len < ROOM_CODE_LEN)
		{
			sprintf(pcHouseCodeOut,"%04d",atoi(acHouseCode));
		}
		else if(len > ROOM_CODE_LEN)
		{
			memcpy(pcHouseCodeOut, acHouseCode + len - ROOM_CODE_LEN, ROOM_CODE_LEN);
			pcHouseCodeOut[ROOM_CODE_LEN] = 0x0;
		}
	}
	else
	{
		strcpy(pcHouseCodeOut,acHouseCode);
	}

	LOG_RUNLOG_DEBUG("TMCCGetHouseInfoByHouseCode  acHouseCode == %s\n",pcHouseCodeOut);

	return TRUE;
}

//开始呼叫时候根据房号查找数据并创建数据 返回 FALSE 0 为未找到  TRUE 1 已找到
YKHandle TMCCGetHouseInfoByHouseCode(const char* pcHouseCode)
{
	char acHouseCode[64];
	sd_domnode_t* phonebook,*stType;
	sd_domnode_t* Cornet = NULL;
	BOOL bOk = FALSE;
	int type = 0;

	//清空数据
	TMDestroyHouseInfo();

	if(!pcHouseCode)
	{
		return FALSE;
	}

	LOG_RUNLOG_DEBUG("TMCCGetHouseInfoByHouseCode  pcHouseCode == %s\n",pcHouseCode);

	if(!g_HouseInfoTemp || g_bReloadConfig)
	{
		if(g_HouseInfoTemp)
		{
			sd_domnode_delete(g_HouseInfoTemp);
			g_HouseInfoTemp = NULL;
		}
		g_HouseInfoTemp = g_HouseInfo;
		g_bReloadConfig = 0;
		if(!g_HouseInfoTemp)
		{
			LOG_RUNLOG_DEBUG("TMCCGetHouseInfoByHouseCode  g_HouseInfoTemp == NULL\n");
			return FALSE;
		}
	}

	//房号处理
//	strcpy(acHouseCode,pcHouseCode);
	memset(&acHouseCode,0,64);

	if(TMCCFilterHouseCode(pcHouseCode,acHouseCode) == FALSE)
	{
		return FALSE;
	}

	if(strcmp(acHouseCode,"00009999") == 0)
	{
		type = 1;
	}
	else
	{
		phonebook = sd_domnode_search(g_HouseInfoTemp, "PHONE_BOOK");//find phone book node.
		if(!phonebook)
		{
			return FALSE;
		}
		g_Entry = sd_domnode_first_child(phonebook);
		while(g_Entry)
		{
			if((Cornet = sd_domnode_attrs_get(g_Entry, "Cornet")) != NULL)
			{
				if(Cornet->value != NULL)
				{
					if(strcmp(Cornet->value,acHouseCode) == 0)
					{
						LOG_RUNLOG_DEBUG("TMCCGetHouseInfoByHouseCode  check OK!");
						//获取CALL 头节点
						g_NowCall = sd_domnode_first_child(g_Entry);
						if(g_NowCall)
						{
							g_bGetCallBegin = 1;
							g_NowCall = NULL;
						}
						bOk = TRUE;
						break;
					}
				}
			}
			g_Entry = sd_domnode_next_child(phonebook, g_Entry);
		}
	}

	if(type)
	{
		phonebook = sd_domnode_search(g_HouseInfoTemp, "PHONE_BOOK");//find phone book node.
		if(!phonebook)
		{
			return FALSE;
		}
		g_Entry = sd_domnode_first_child(phonebook);
		while(g_Entry)
		{
			if( (stType = sd_domnode_attrs_get(g_Entry, "Type")) && stType->value)
			{
				if(atoi(stType->value) == type )
				{
					LOG_RUNLOG_DEBUG("TMCCGetHouseInfoByHouseCode  check OK!");
					//获取CALL 头节点
					g_NowCall = sd_domnode_first_child(g_Entry);
					if(g_NowCall)
					{
						g_bGetCallBegin = 1;
						g_NowCall = NULL;
					}
					bOk = TRUE;
					break;
				}
			}
			g_Entry = sd_domnode_next_child(phonebook, g_Entry);
		}
	}

	return bOk;
}

/**
 *@brief Get ring time by house handle.
 *@param[in] house The handle of house. @see TMCCGetHouseInfoByHouseCode
 *@return Ring time.
 */
int TMCCGetRingTime(YKHandle hHouse)
{
	sd_domnode_t *ring = NULL;
	if(!g_Entry)
	{
		return 0;
	}

	if((ring = sd_domnode_attrs_get(g_Entry, "RingTime")) != NULL)
	{
		if(ring->value != NULL)
		{
			return atoi(ring->value);
		}
	}
	else
	{
		//默认值
		return 30;
	}
}


/**
 *@brief Get Sip call delay time.
 *@param[in] house The handle of house.
 *@return Delay time.
 */
int TMCCGetSipCallDelay(YKHandle hHouse)
{
	sd_domnode_t *delay = NULL;
	if(!g_Entry)
	{
		return 0;
	}

	if((delay = sd_domnode_attrs_get(g_Entry, "CallDelay")) != NULL)
	{
		if(delay->value != NULL)
		{
			return atoi(delay->value);
		}
	}
	else
	{
		//默认值
		return 0;
	}
}
//
//BOOL TMCCIsVideoCall()
//{
//	sd_domnode_t *video = NULL;
//	if(!g_NowCall)
//	{
//		return FALSE;
//	}
//
//	if((video = sd_domnode_attrs_get(g_NowCall, "VideoCall")) != NULL)
//	{
//		if(video->value != NULL)
//		{
//			if(strcmp(video->value,"1") == 0)
//			{
//				return TRUE;
//			}
//		}
//	}
//	return FALSE;
//}

//获取号码: pcPhoneNum 传入参数进行赋值 ，pcPhoneNum［0］号码为0x0时 为无号码
//返回值 sernum值 为0 时候为无CALL信息 返回 Code号码  为-1 时为错误
int TMCCGetNumInfo(TM_I6NUM_INFO_ST **now_info)
{
	sd_domnode_t *count = NULL;
	sd_domnode_t *code = NULL;
	sd_domnode_t *sernum = NULL;
	sd_domnode_t *preview = NULL;
	sd_domnode_t *videocall = NULL;
	sd_domnode_t *ValidCall = NULL;
	int iSerNum = 0;

	memset(&stNowI6NumInfo,0,sizeof(TM_I6NUM_INFO_ST));

	*now_info = &stNowI6NumInfo;

	if(!g_Entry)
	{
		*now_info = NULL;
		return -1;
	}
	//判断是否有CALL字段
	if(!g_bGetCallBegin)
	{
		//取Code值返回
		if((code = sd_domnode_attrs_get(g_Entry, "Code")) != NULL)
		{
			if(code->value)
			{
				strcpy(stNowI6NumInfo.acPhoneNum,code->value);
				stNowI6NumInfo.iSerNum = 1;
				stNowI6NumInfo.ucCallType = MEDIA_TYPE_AUDIO_VIDEO;
				return 0;
			}
		}
		return -1;
	}
	else
	{
//		g_NowCall = sd_domnode_first_child(g_Entry);
		if(g_bGetCallEnd)
		{
			*now_info = NULL;
			return -1;
		}

		if(!g_NowCall)
		{
			ValidCall = sd_domnode_first_child(g_Entry);
			while(ValidCall)
			{
				if((sernum = sd_domnode_attrs_get(ValidCall, "SerNum")) != NULL)
				{
					if(sernum->value)
					{
						iSerNum = atoi(sernum->value);
						if(iSerNum == 0)
						{
							ValidCall = sd_domnode_next_child(g_Entry, ValidCall);
							continue;
						}
						else
						{
							break;
						}
					}
				}
			}

			//获取CALL 头节点
			g_NowCall = ValidCall;
			if(g_NowCall)
			{
				if((code = sd_domnode_attrs_get(g_NowCall, "Num")) != NULL)
				{
					if(code->value)
					{
						strcpy(stNowI6NumInfo.acPhoneNum,code->value);
					}
					else
					{
						return -1;
					}
				}
				else
				{
					return -1;
				}
				if((preview = sd_domnode_attrs_get(g_NowCall, "PreView")) != NULL && preview->value)
				{
					if(atoi(preview->value))
					{
						stNowI6NumInfo.ucPreView = 0x1;
					}
					else
					{
						stNowI6NumInfo.ucPreView = 0x0;
					}
				}
				else
				{
					stNowI6NumInfo.ucPreView = 0x0;
				}
				if((videocall = sd_domnode_attrs_get(g_NowCall, "VideoCall")) && videocall->value )
				{
					if(atoi(videocall->value))
					{
						stNowI6NumInfo.ucCallType = MEDIA_TYPE_AUDIO_VIDEO;
					}
					else
					{
						stNowI6NumInfo.ucCallType = MEDIA_TYPE_AUDIO;
					}
				}
				else
				{
					stNowI6NumInfo.ucCallType = MEDIA_TYPE_AUDIO_VIDEO;
				}
				strcpy(stNowI6NumInfo.acPhoneNum,code->value);
				stNowI6NumInfo.iSerNum = iSerNum;
				//下一个
				g_NowCall = sd_domnode_next_child(g_Entry, g_NowCall);
			}
			else
			{
				*now_info = NULL;
				return -1;
			}
		}
		else
		{
			if((code = sd_domnode_attrs_get(g_NowCall, "Num")) != NULL)
			{
				if(code->value)
				{
					strcpy(stNowI6NumInfo.acPhoneNum,code->value);
				}
				else
				{
					return -1;
				}
			}
			else
			{
				return -1;
			}
			if((sernum = sd_domnode_attrs_get(g_NowCall, "SerNum")) != NULL)
			{
				if(sernum->value)
				{
					stNowI6NumInfo.iSerNum = atoi(sernum->value);
				}
			}
			if((preview = sd_domnode_attrs_get(g_NowCall, "PreView")) != NULL && preview->value)
			{
				if(atoi(preview->value))
				{
					stNowI6NumInfo.ucPreView = 0x1;
				}
				else
				{
					stNowI6NumInfo.ucPreView = 0x0;
				}
			}
			else
			{
				stNowI6NumInfo.ucPreView = 0x0;
			}
			if((videocall = sd_domnode_attrs_get(g_NowCall, "VideoCall")) && videocall->value)
			{
				if(atoi(videocall->value))
				{
					stNowI6NumInfo.ucCallType = MEDIA_TYPE_AUDIO_VIDEO;
				}
				else
				{
					stNowI6NumInfo.ucCallType = MEDIA_TYPE_AUDIO;
				}
			}
			else
			{
				stNowI6NumInfo.ucCallType = MEDIA_TYPE_AUDIO_VIDEO;
			}
			g_NowCall = sd_domnode_next_child(g_Entry, g_NowCall);
		}

		if(!g_NowCall)
		{
			g_bGetCallEnd = 1;
		}
		return 0;
	}

}

void TMUpdateAlarmState(TM_ALARM_ID_EN enModule, const char* cState)
{
    if(NULL == g_pstTMMsgQ)
    {
        return;
    }

    TM_I6_ALARM_UPDATE_ST* pstMsg = (TM_I6_ALARM_UPDATE_ST*)malloc(sizeof(TM_I6_ALARM_UPDATE_ST));
    if( NULL == pstMsg )
    {
        return;
    }
    memset(pstMsg, 0x00, sizeof(TM_I6_ALARM_UPDATE_ST));
    pstMsg->uiPrmvType = TM_I6_ALARM_UPDATE;
    pstMsg->uiAlarmNo = (unsigned int)enModule;
    strcpy(pstMsg->auStateCode,cState);

    YKWriteQue( g_pstTMMsgQ,  pstMsg,  250);
}

BOOL TMSMIsValidAuthorizeCard(const char* pcSerinalNumber)
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t* card, *cardnum;
	sd_domnode_t* find;
	char acBuf[64];
	int ret = FALSE;
	if(-1 == sd_domnode_load(document, I6_CARD_PATH)){
		printf("card xml is error\n");
		sd_domnode_delete(document);
		return ret;
	}

	if(pcSerinalNumber)
	{
		sprintf(acBuf,"YKAC%s",pcSerinalNumber);
	}

	find = sd_domnode_search(document, "cardAdd");
	if(find)
	{
		card = sd_domnode_first_child(find);
		while(card)
		{
			cardnum = sd_domnode_attrs_get(card, "no");
			if(cardnum && cardnum->value && 0 == strcmp(cardnum->value,acBuf))
			{
				ret = TRUE;
			}
			card = sd_domnode_next_child(find,card);
		}

	}
	sd_domnode_delete(document);
	return ret;
}

#endif

/**@}*///For SM module
