#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <TRManage.h>
#include <evcpe.h>
#include "session.h"
#include "cpe.h"
#include <YKMsgQue.h>
#include <GI6Interface.h>
#include "sd/domnode.h"
#include "FPInterface.h"

#ifndef XNMP_ENV
#include <LOGIntfLayer.h>
#include <LPIntfLayer.h>
#include "../INC/NetworkMonitor.h"
#include <IDBT.h>
#include <IDBTCfg.h>
#include <FPInterface.h>
#ifdef _I6_CC_
#include "I6CCTask.h"
#else
#include "CCTask.h"
#endif
#endif

#define TM_QUE_LEN 256
#define ITEM_BUF_SIZE 64
#define SIP_USERNAME	"SIP_Account_Username"
#define SIP_PASSWORD	"SIP_Account_Password"
#define SIP_DOMAIN		"User_Domain"
#define SIP_PROXY		"SIP_Proxy"
#define SIP_PORT		"Proxy_Port"

//TM事件处理结构体
typedef struct
{
	unsigned int 	uiPrmvType;            //原语类型
}TM_MSG_ST;

static pthread_t thr_monitor;
static pthread_t thr_obj;
static pthread_t ba_obj;
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
static char* pcTMargv[2]={TM_MODEL_PATH,TM_DATA_RUNNING};
#else
static char* pcTMargv[2]={TM_MODEL_PATH,TM_DATA_PATH};
#endif
static char* pcBAargv[2]={BA_MODEL_PATH,BA_DATA_PATH};
#if _TM_TYPE_== _TR069_
#ifdef _YK_PC_
int displayFlag = TRUE;
#else
int displayFlag = TRUE;
#endif
#endif
extern char g_acTRFirmVersion[64] = YK_APP_VERSION;
extern int g_bConfigUpdateFlag;

extern int evcpe_shutdown(int code);
extern int PICInit(void);
extern void PICFini(void);
extern int evcpe_persister_persist(struct evcpe_persister *persist);

void TRReboot(void)
{
#ifndef XNMP_ENV
	//写入日志
	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_PLATFORM_REBOOT, 0, "");
	g_iRunSystemFlag = FALSE;
	IDBTMsgSendABReboot(REMOTE_REBOOT);
#endif
}

void TRConfigUpdate(void)
{
#ifndef XNMP_ENV
	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_RELOAD_CONFIG, 0, "");
#endif
}

#ifndef XNMP_ENV
static void TranaferProcessFunc(FP_TRANSFER_INFO_ST* pstInfo)
{
    /*TMSystemLogOut(TM_MESSAGE, "dAvlSpeed:%f, Process: %d, totalTime: %d\n",
        pstInfo->dAvlSpeed,
        pstInfo->lTransferredSize*100/pstInfo->lTotalSize,
        pstInfo->lTotalTime);*/
}
#endif

void RemoteTransferFunc(enum TM_TRANSFER_TYPE_EN enType, enum TM_TRANSFER_FILE_TYPE_EN enFileType, const char* pcLocalUrl, const char* pcRemoteUrl)
{
#ifndef XNMP_ENV
    char acMessage[1024]={0x0};
    char acUserName[256], acPassWord[256], pacHost[256]; int iPort=21;
    sprintf(acMessage, "Transfer: %s Local://%s Remote://%s", enType==0?"Download":"Upload", pcLocalUrl, pcRemoteUrl);
    TMQueryFtpInfo(pacHost, &iPort, acUserName, acPassWord);
    LOG_RUNLOG_DEBUG("TM [%s] pacHost = %s, pcUserName = %s, pcPassWord = %s, pcLocal = %s, pcRemote = %s", __func__, pacHost, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl);
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
    case TM_TYPE_TR_MODEL:
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
            LOG_RUNLOG_DEBUG("TM [%s] pcLocal = %s, pcRemote = %s", __func__, pcLocalUrl, pcRemoteUrl);
            FPUpdateAdvert(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        	//FPUpdateRinging(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        }
        else
        {
        	FPPut(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
        }
        break;
    case TM_TYPE_ADVERT:
        if(enType==TM_REMOTE_DOWNLOAD)
        {
        	FPUpdateAdvert(pacHost, iPort, acUserName, acPassWord, pcLocalUrl, pcRemoteUrl, TranaferProcessFunc, NULL,FP_TRANSFER_NOWAIT);
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
#endif
}

#if _TM_TYPE_== _TR069_ || defined(_GYY_I5_)
void TRNotifyInfo()
{
	const char *value;
	unsigned int len;
#ifndef XNMP_ENV
	TRConfigUpdate();

	evcpe_repo_get(g_tr_evcpe_st.repo, ".DeviceInfo.DeviceNo", &value, &len);
	if(value)
	{
		if(len)
		{
			strcpy(g_stIdbtCfg.acSn,value);
		}
		else
		{
			strcpy(g_stIdbtCfg.acSn,SN_NULL);
		}
	}
	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.WanUserName", &value, &len);
	if(value && len)
	{
		strcpy(g_stIdbtCfg.stNetWorkInfo.acWanUserName,value);
	}
	evcpe_repo_get(g_tr_evcpe_st.repo, ".Communication.WanPassword", &value, &len);
	if(value && len)
	{
		strcpy(g_stIdbtCfg.stNetWorkInfo.acWanPassword,value);
	}
	SaveIdbtConfig();
#endif
}

void TRNotifyWan()
{
//	NMNotifyInfo();
}
#if  _TM_TYPE_== _TR069_
TM_TIMER_ST g_stNotifySipTimer;
void TRNotifySipCallBack()
{
	char acUserName[64];
	char acPassword[64];
	char acProxy[64];
	char acDomain[64];
	int  iPort;
	int i = 0;

	TMGetIMSInfo(acUserName, acPassword, acProxy, &iPort, acDomain);
	while(1)
	{
		if(LPMsgSendTmSipRegister(acUserName, acPassword, acDomain,acProxy) == 0)
		{
			break;
		}
		else
		{
			sleep(3);
			if(++i > 10)
			{
				break;
			}
		}
	}
}


void TRNotifySip()
{
	g_stNotifySipTimer.pstTimer = YKCreateTimer(TRNotifySipCallBack,NULL,1000, YK_TT_NONPERIODIC,(unsigned long *)&g_stNotifySipTimer.ulMagicNum);
	if(NULL == g_stNotifySipTimer.pstTimer)
	{
		LOG_RUNLOG_ERROR("SM: create client rsp timer failed!");
	}
}
#endif
void TRHndlDemoRoom(const char *pcRoomNum)
{
//#ifndef XNMP_ENV
//	TMSM_DEMO_CALL_ST *pstMsg = (TMSM_DEMO_CALL_ST*)malloc(sizeof(TMSM_DEMO_CALL_ST));
//	if(NULL == pstMsg)
//	{
//		return;
//	}
//
//	memset(pstMsg, 0x0, sizeof(TMSM_DEMO_CALL_ST));
//
//	pstMsg->uiPrmvType = DEMO_SM_CALL;
//	strncpy((char *)pstMsg->aucRoomNum, pcRoomNum, strlen(pcRoomNum));
//	YKWriteQue(g_pstSMMsgQ , pstMsg,  0);
//#endif
}

void TRHndlDemo(const char *acPhoneNumber, const char *cVoipEnable)
{
#ifndef XNMP_ENV
	TMCC_DEMO_CALL_ST *pstMsg = (TMCC_DEMO_CALL_ST*)malloc(sizeof(TMCC_DEMO_CALL_ST));
	if(NULL == pstMsg)
	{
		return;
	}

	memset(pstMsg, 0x0, sizeof(TMCC_DEMO_CALL_ST));

	pstMsg->uiPrmvType = TMCC_DEMO_CALL;
	strncpy((char *)pstMsg->aucPhoneNum, acPhoneNumber, strlen(acPhoneNumber));
	if(cVoipEnable[0]=='0')
	{
		pstMsg->aucMediaType = MEDIA_TYPE_AUDIO;
	}
	else if(cVoipEnable[0]=='1')
	{
		pstMsg->aucMediaType = MEDIA_TYPE_AUDIO_VIDEO;
	}
	else if(cVoipEnable[0]=='2')
	{
		pstMsg->aucMediaType = MEDIA_TYPE_TEL;
	}
	//pstMsg->aucMediaType = (cVoipEnable[0]=='1'?MEDIA_TYPE_AUDIO_VIDEO : MEDIA_TYPE_AUDIO);
	YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
#endif
}
sd_domnode_t* TRUtilsMessageXmlLoadUseFile(FILE *msgFile)
{
	LOG_RUNLOG_DEBUG("TM debug:use LPUtilsMessageXmlLoadUseFile");
    int ret = 0;
    sd_list_iter_t* i = NULL;
    sd_domnode_t*   node = NULL;
    sd_domnode_t*   root_node = NULL;
    root_node = sd_domnode_new(NULL, NULL);
    if(root_node == NULL)
    {
    	return NULL;
    }

	ret =  __sd_domnode_xml_fread(&root_node, msgFile);
    //ret = sd_domnode_fread(root_node, msgFile); //LPMessageRecv sd_domnode_fread error
    if(ret == -1)
    {
    	sd_domnode_delete(root_node);
    	LOG_RUNLOG_DEBUG("TM LPMessageRecv sd_domnode_fread error");
		return NULL;
    }

    //sd_domnode_delete(root_node);
	return root_node;
}
int FindParamFromFile2(sd_domnode_t* pstRootNode, const char *pItemStr, char *pItemValue)
{
	sd_list_iter_t* i = NULL;
	int len;

	for (i = sd_list_begin(pstRootNode->children); i != sd_list_end(pstRootNode->children);
	i = sd_list_iter_next(i))
	{
		sd_domnode_t* nodeChild = i->data;
		if (!strcmp(nodeChild->name, pItemStr))
		{
			//char* pstMsg = (char*)malloc(strlen(nodeChild->value));
			//memset(pstMsg, 0x00, strlen(nodeChild->value));
			len = strlen(nodeChild->value);
            memcpy(pItemValue, nodeChild->value, len);
            pItemValue[len] = 0x0;
			return TRUE;
		}
	}

	return FALSE;
}
#if  defined(_GYY_I5_) || defined(TM_CONFIG_DOWNLOAD)
extern int GetFromHttps(const char *pcConfigUrl,const char* pcUserName, const char* pcPasswd,const char* pcFileName);
#endif
//------------------add by hb
void TRGetSip(const char *pcConfigUrl,const char* pcUserName, const char* pcPasswd)
{
	sd_domnode_t* pstRootNode = NULL;
	char buf[256]={0x0};
	FILE *pFile = NULL;

	time_t curtime;
	struct tm *loctime;
	curtime = time(NULL);
	loctime = localtime(&curtime);

	char acUserName[ITEM_BUF_SIZE+1] = {0};
	char acPassWord[ITEM_BUF_SIZE+1] = {0};
	char acDomain[ITEM_BUF_SIZE+1] = {0};
	char acProxy[ITEM_BUF_SIZE+1] = {0};
	char acPort[ITEM_BUF_SIZE+1] = {0};
#if  defined(_GYY_I5_) || defined(TM_CONFIG_DOWNLOAD)
	LOG_RUNLOG_DEBUG("TM get ImsSip.xml begin! url = %s",pcConfigUrl);
	if(GetFromHttps(pcConfigUrl,pcUserName,pcPasswd,IMS_SIP_PATH) == 0)
#endif
	{
		evcpe_event_transfer(0, loctime, loctime);
		LOG_RUNLOG_DEBUG("TM get ImsSip.xml OK!");
		pFile = fopen(IMS_SIP_PATH, "r");
		pstRootNode = TRUtilsMessageXmlLoadUseFile(pFile);
		close(pFile);
		if(pstRootNode == NULL)
		{
			return;
		}
		if(pstRootNode->name == NULL)
		{
			return;
		}

		LOG_RUNLOG_DEBUG("TM pstRootNode->name:%s", pstRootNode->name);

		if(!strncmp(pstRootNode->name, "ConfigFile", sizeof("ConfigFile")))
		{
			if(FindParamFromFile2(pstRootNode,SIP_USERNAME,acUserName)==FALSE)
			{
				return;
			}

			if(FindParamFromFile2(pstRootNode,SIP_PASSWORD,acPassWord)==FALSE)
			{
				return;
			}
			if(FindParamFromFile2(pstRootNode,SIP_DOMAIN,acDomain)==FALSE)
			{
				return;
			}
			if(FindParamFromFile2(pstRootNode,SIP_PROXY,acProxy)==FALSE)
			{
				return;
			}
			if(FindParamFromFile2(pstRootNode,SIP_PORT,acPort)==FALSE)
			{
				return;
			}
		}

#if _TM_TYPE_ == _GYY_I6_
		//写入
		evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.SipAccount", acUserName, strlen(acUserName));
		evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.SipPassword", acPassWord, strlen(acPassWord));
		evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.ImsDomainName", acDomain, strlen(acDomain));
		evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.ImsProxyIP", acProxy, strlen(acProxy));
		evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.ImsProxyPort", acPort, strlen(acPort));
		I6SetSipInfo(acUserName,acPassWord,acDomain,acProxy,acPort);

		if(FindParamFromFile2(pstRootNode,"FTP_IP",acProxy)==TRUE)
		{
			evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.FtpIP", acProxy, strlen(acProxy));
		}
		if(FindParamFromFile2(pstRootNode,"FTP_Port",acPort)==TRUE)
		{
			evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.FtpPort", acPort, strlen(acPort));
		}
		if(FindParamFromFile2(pstRootNode,"FTP_Account",acUserName)==TRUE)
		{
			evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.FtpUserName", acUserName, strlen(acUserName));
		}
		if(FindParamFromFile2(pstRootNode,"FTP_Pwd",acPassWord)==TRUE)
		{
			evcpe_repo_set(g_tr_evcpe_st.repo, ".Communication.FtpPassword", acPassWord, strlen(acPassWord));
		}

		evcpe_persister_persist(g_tr_evcpe_st.persist);
#endif

		sd_domnode_delete(pstRootNode);
//		g_bConfigUpdateFlag = 1;
		//通知
		//TRNotifySip();
	}
//	evcpe_event_transfer(EVCPE_CPE_DOWNLOAD_FAILURE, loctime, loctime);
}
//------------------add by hb

//------------------add by hb
void  TMGetInfo(const char *pcConfigUrl,const char *pcFileName)
{
	FILE *pFile = NULL;
	char acFile[256] = {0x0};
	char acBuf[256] = {0x0};
	time_t curtime;
	struct tm *loctime;
	curtime = time(NULL);
	loctime = localtime(&curtime);
	sprintf(acFile,"%s.bz2",TM_DATA_CONFIG);
#ifdef TM_CONFIG_DOWNLOAD
	if(GetFromHttps(pcConfigUrl,NULL,NULL,acFile) == 0)
	{
		LOG_RUNLOG_DEBUG("TM get TMConfig.xml OK!");
		//配置文件
		sprintf(acBuf,"busybox rm %s",TM_DATA_CONFIG);
		system(acBuf);
		sprintf(acBuf,"busybox bunzip2 %s",acFile);
//		sprintf(acBuf,"busybox cp -rf %s %s",acFile,TM_DATA_CONFIG);
		system(acBuf);
		//CRC

		if(TMCheckCRC(TM_DATA_CONFIG,pcFileName) == TRUE)
		{
			evcpe_event_transfer(0, loctime, loctime);
			g_bConfigUpdateFlag = 1;
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
			sprintf(acBuf,"busybox bzip2 %s",TM_DATA_CONFIG);
			system(acBuf);
			sprintf(acBuf,"busybox mv %s %s",acFile,TM_DATA_PATH);
			system(acBuf);
#else
			sprintf(acBuf,"busybox mv %s %s",TM_DATA_CONFIG,TM_DATA_PATH);
			system(acBuf);
#endif
		}
		else
		{
			evcpe_event_transfer(9800, loctime, loctime);
		}
	}
	else
	{
		evcpe_event_transfer(EVCPE_CPE_DOWNLOAD_FAILURE, loctime, loctime);
	}
#endif
}
//------------------add by hb

int TMStartTerminateManage(const char* pcVersion, const char* pcConfig)
{
    int rst;

#if _TM_TYPE_== _TR069_
    evcpe_callback_reboot(TRReboot);
    evcpe_callback_load(RemoteTransferFunc);
    evcpe_callback_notifyinfo(TRNotifyInfo);
    evcpe_callback_notifysip(TRNotifySip);
    evcpe_callback_notifywan(TRNotifyWan);
    evcpe_callback_demo(TRHndlDemo);
    evcpe_callback_demo_room(TRHndlDemoRoom);
#ifdef TM_CONFIG_DOWNLOAD
    evcpe_callback_tr_config(TMGetInfo);
#endif
#endif
    evcpe_callback_config(TRGetSip);
#ifndef XNMP_ENV
    strcpy(g_acTRFirmVersion,pcVersion);
    LOG_RUNLOG_DEBUG("TM pcVersion = %s, %s", g_acTRFirmVersion, pcVersion);
#endif
#ifdef _GYY_I5_
    rst = pthread_create(&thr_obj, NULL, evcpe_startup, (void *)&pcBAargv);
	if (0 != rst)
	{
		return FALSE;
	}
#endif
#if  _TM_TYPE_== _TR069_
    rst = pthread_create(&thr_obj, NULL, evcpe_startup, (void *)&pcTMargv);
    if (0 != rst)
    {
        return FALSE;
    }
#endif
    sleep(10);

#ifdef TM_TAKE_PIC
    PICInit();
#endif
    return TRUE;
}


void TMStopTerminateMange(void)
{
    void *return_code;
//    g_bTMMsgTaskFlg = FALSE;
    evcpe_shutdown(0);
    pthread_join(thr_obj, &return_code);
#ifdef TM_TAKE_PIC
    PICFini();
#endif
}

#endif

int TMSetParamVersion(struct evcpe *cpe)
{
	int rc;

	if (evcpe_repo_set(cpe->repo, ".DeviceInfo.FirmwareVersion", YK_APP_VERSION, strlen(YK_APP_VERSION))) {
		evcpe_error(__func__, "failed to set param: %s", "FirmwareVersion");
		rc = 22;//EINVAL
		goto finally;
	}
	if (evcpe_repo_set(cpe->repo, ".DeviceInfo.KernelVersion", YK_KERNEL_VERSION, strlen(YK_KERNEL_VERSION))) {
		evcpe_error(__func__, "failed to set param: %s", "FirmwareVersion");
		rc = 22;//EINVAL
		goto finally;
	}
	if (evcpe_repo_set(cpe->repo, ".DeviceInfo.HardwareVersion", YK_HARDWARE_VERSION, strlen(YK_HARDWARE_VERSION))) {
		evcpe_error(__func__, "failed to set param: %s", "FirmwareVersion");
		rc = 22;//EINVAL
		goto finally;
	}

	rc = 0;
finally:
	return rc;
}

int TRProcessCommand(const char *cmd)
{
#if _TM_TYPE_== _TR069_
	char acTmp[20] = {0};
	char *pcTmp = NULL;
	char *pcCmdText = NULL;

	strcpy(acTmp,cmd);

	pcTmp = strchr(acTmp, ' ');
	if(NULL != pcTmp)
	{
		*pcTmp = '\0';
		pcCmdText = ++pcTmp;
	}

	if(0 == strcmp(acTmp, "display"))
	{
		displayFlag = TRUE;
	}
	else if(0 == strcmp(acTmp, "undisplay"))
	{
		displayFlag = FALSE;
	}
#endif
	return SUCCESS;
}

#ifdef XNMP_ENV
int main(int argc, char **argv)
{
    int rst;
    struct tm starttime;
    struct tm completetime;
    
	displayFlag = TRUE;
    rst = TMStartTerminateManage("1.0", "config.xml");	
    if (FALSE == rst) 
    {
    	LOG_RUNLOG_DEBUG("TM app main: failed to init tr069. ");
        goto EXIT; 
    }

    while(1)
    {
        sleep(60);
        LOG_RUNLOG_DEBUG("TM $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ ");
//        evcpe_event_transfer(0, starttime, completetime);

        //evcpe_event_change(EVCPE_EVENT_UPDATE);
    }
    
EXIT:
    TMStopTerminateMange();
    return 0;
}
#endif

