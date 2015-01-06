
#include <YKApi.h>
#include <TMInterface.h>
#include <IDBT.h>
#include <LOGIntfLayer.h>
#include <evcpe.h>
#include <sys/time.h>
#include "IDBTIntfLayer.h"
#include <FPInterface.h>
#include <FtpCore.h>

#define _YK_OS_ _YK_OS_LINUX_

typedef enum{
    FP_FILE_APP=0, 
    FP_FILE_RINGING,
    FP_FILE_LOG,
    FP_FILE_CONFIG,
    FP_FILE_ADVERT,
    FP_FILE_OTHER
}FP_FILE_TYPE_EN;

typedef enum{
    FP_MODE_PUT=0,
    FP_MODE_GET
}FP_TRANSFER_MODE_EN;

#define FP_VERSION_MARK "UpgradeVersion"

typedef struct _FPTransferData{
    char pacHost[256];
    int  iPort;
    char acUserName[256];
    char acPassword[256];
    char acLocalUrl[256];
    char acRemoteUrl[256];
    FP_FILE_TYPE_EN enType;
    FP_TRANSFER_MODE_EN enMode;
    YKHandle hThread;
    FPTransferDoneFunc Done;
    FPTransferProcessFunc Process;
    int iTryTransferCount;
}FP_TRANSFER_DATA_ST;

extern void ExitApp(void);
extern void UpdateConfig(void);
extern void PICFtpOk();      // FTP 上传完成
extern int PICTimeInterval(struct timeval stStartTime, struct timeval stEndTime);

int g_bUpdateFlag = 0;
#define UPDATA_PIC_CNT  3       //图片上传次数

static void FTPTransferProcess(FP_TRANSFER_INFO_ST* pstInfo){
	LOG_RUNLOG_DEBUG("FT use time(%d), size(%u/%u)",
			pstInfo->lTotalTime, pstInfo->lTransferredSize, pstInfo->lTotalSize);
}

static BOOL FPCheckAppVersion(const char* pcPre, const char* pcCurrent)
{
    int preLen = strlen(pcPre);
    int curLen = strlen(pcCurrent);
    printf("preVersion = %s, currentVersion = %s\n",pcPre, pcCurrent );
    if(strcmp(pcPre, pcCurrent) != 0 && strncmp(pcPre +preLen -2, pcCurrent + curLen - 2, 2 ) != 0){
    	return TRUE;
    }
    return FALSE;
}

static unsigned short FPAlgoCRC(const unsigned char* pData, unsigned long ulSize, unsigned short usInitValue)
{
    unsigned long i = 0;
    unsigned short usCrc = usInitValue;
    while(ulSize-- != 0)
    {
        for(i = 0x80; i != 0; i/=2) {
            if((usCrc & 0x8000) != 0)
            {
                usCrc *= 2;
                usCrc ^= 0x1021; //CRC
            }else{
                usCrc *= 2;
            }
            if((*pData & i) != 0)
            {
                usCrc ^= 0x1021; //CRC
            }
        }
        pData++;
    }
    return usCrc;
}

 BOOL FPCRCForAppImage(char* image){
    unsigned int size = 0;
    unsigned char* buffer=NULL;
    char value[512] = {0x0};
    const char* imagecrc=NULL;
    char command[512] = {0x0};
    char *p;
    FILE* pf = fopen(image, "rb");
    if(NULL != pf){

        fseek(pf, 0, SEEK_END);
        size = ftell(pf);
        fseek(pf, 0, SEEK_SET);
        buffer = (unsigned char*)malloc(size);
        if(size != fread(buffer, sizeof(char), size, pf)){
            fclose(pf);
            free(buffer);
            LOG_RUNLOG_WARN("FT Read image failed !\n");
            return FALSE;
        }

        imagecrc = strrchr(image, '.');
        sprintf(value, "%d", FPAlgoCRC(buffer, size, 0));
        LOG_RUNLOG_DEBUG("FT calc crc value : %s [%s]", value, imagecrc+1);
        free(buffer);
        fclose(pf);

        if(strcmp(imagecrc + 1, value) == 0){
        	strcpy(value,image);
        	p = strrchr(value,'.');
        	*p = '\0';
			sprintf(command, "busybox mv %s /data/fsti_tools/AndroidLadder.apk", image);
            system(command);
            LOG_RUNLOG_WARN("FT \t%s", command);
            sprintf(command, "busybox chmod 777 /data/fsti_tools/AndroidLadder.apk");
            system(command);
            LOG_RUNLOG_WARN("FT \tCRC ok...%s", command);

            return TRUE;
        }
    }else{
        LOG_RUNLOG_WARN("FT Open image failed!");
    }
    LOG_RUNLOG_WARN("FT \tCRC error.");
    return FALSE;
}

 BOOL  FPTryToRunApplication(const char* acApp)
{
    char acCommandLine[512]={0x0};
    FP_VERSION_CHECK_STATE_EN enState=FP_CHECK_ERROR;
#if _YK_OS_==_YK_OS_LINUX_
    int iStatus;
    char pAppName[128] = {0x0};
    strcpy(pAppName, acApp);
#ifdef _YK_XT8126_
    LOG_RUNLOG_DEBUG("FT \t %s \t", acCommandLine);
    sprintf(acCommandLine, "busybox cp -rf %s %s_bak\n", acApp, acApp);
    system(acCommandLine);
    LOG_RUNLOG_DEBUG("FT \t %s \t", acCommandLine);
    //bzip2
    sprintf(acCommandLine, "busybox bunzip2 -d %s\n", acApp);
    system(acCommandLine);
    LOG_RUNLOG_DEBUG("FT \t %s \t", acCommandLine);
    pAppName[strlen(acApp) - strlen(".bz2")]='\0';
#endif
    sprintf(acCommandLine,"chmod 777 %s",pAppName);
    system(acCommandLine);
    sprintf(acCommandLine, "%s %s %s", pAppName, FP_VERSION_MARK, YK_APP_VERSION);
    LOG_RUNLOG_DEBUG("FT \t %s \t", acCommandLine);
    iStatus = system(acCommandLine);
    if (WIFEXITED(iStatus))
    {
        //normal exit.
        enState=(FP_VERSION_CHECK_STATE_EN)WEXITSTATUS(iStatus);//get exit code.
        LOG_RUNLOG_WARN("FT enState = %d (%s)", enState,
        		enState==FP_CHECK_UPGRADE?"FP_CHECK_UPGRADE":"FP_CHECK_ERROR");
    }
#else
    STARTUPINFOA si; 
    PROCESS_INFORMATION pi;
    DWORD ret;
    ZeroMemory(&si,sizeof(STARTUPINFOA)); 
    si.cb=sizeof(STARTUPINFOA);
    sprintf(acCommandLine, "%s %s", acApp, FP_VERSION_MARK, YK_APP_VERSION);
    if(CreateProcessA(acApp,acCommandLine,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,NULL,&si,&pi))
    { 
        CloseHandle(pi.hThread); 
        WaitForSingleObject(pi.hProcess, 5000);
        GetExitCodeProcess(pi.hProcess,&ret);
        enState=(FP_VERSION_CHECK_STATE_EN)ret;
    }
#endif
    if(enState==FP_CHECK_UPGRADE)
    {
#ifdef _YK_XT8126_
        sprintf(acCommandLine, "busybox mv %s_bak /mnt/mtd/%s\n", acApp, acApp + strlen("./update/"));
        LOG_RUNLOG_DEBUG("FT \t %s \t", acCommandLine);
#else
        sprintf(acCommandLine, "busybox mv %s %s", acApp, acApp + strlen("./update/"));
        LOG_RUNLOG_DEBUG("FT \t %s \t", acCommandLine);
#endif
        if(system(acCommandLine)!=0)
        {
            enState=FP_CHECK_ERROR;
            LOG_RUNLOG_WARN("FT \t Update Error \t\n");
            return FALSE;
        }
        sprintf(acCommandLine, "rm -rf ./update/*\n");//update failed.
        LOG_RUNLOG_DEBUG("FT \t %s \t", acCommandLine);
        return TRUE;
    }
    sprintf(acCommandLine, "rm -rf ./update/*\n");//update failed.
    LOG_RUNLOG_DEBUG("FT \t %s \t", acCommandLine);
    return FALSE;
}

static void FPUpdateApplicationDone(YKHandle hHandle, BOOL enState, const char* pcLocal, const char* pcRemote )
{
	time_t curtime;
	struct tm *loctime;
	curtime = time(NULL);
	loctime = localtime(&curtime);
	int i = 0;

#if _TM_TYPE_ == _TR069_
	evcpe_event_transfer(0, loctime, loctime);
#endif
//	if(enState && FPCRCForAppImage(pcLocal) && FPTryToRunApplication("./update/YK-IDBT.bz2"))
	if(enState && FPCRCForAppImage(pcLocal))
    {
        LOG_RUNLOG_DEBUG("FT \tThis update application is ok.!\n");
        TMUpdateAlarmState(TM_ALARM_UPDATERESULT, "0");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_UPDATE, 0, pcLocal);


        notifyUpdateApp();

        //加入升级告警结果延时
        while(g_bUpdateFlag != 2)
        {
        	YKSleep(1000);
        	if(g_bUpdateFlag == 0)
        	{
        		continue;
        	}
        	if(++i >= 60)
        	{
        		LOG_RUNLOG_DEBUG("FT TM UpdateFlag timeout");
        		break;
        	}
        }
        ExitApp();
    }
    else
    {
        LOG_RUNLOG_DEBUG("FT \tThis update application is error.!");
        TMUpdateAlarmState(TM_ALARM_UPDATERESULT, "1");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_UPDATE, 1, pcLocal);
        remove(pcLocal);
    }
}

static void FPUpdateDone(YKHandle hHandle, BOOL enState, const char* pcLocal, const char* pcRemote )
{
	time_t curtime;
	struct tm *loctime;
	curtime = time(NULL);
	loctime = localtime(&curtime);

#if _TM_TYPE_ == _TR069_
	evcpe_event_transfer(0, loctime, loctime);
#endif
	if(enState)
    {
        LOG_RUNLOG_DEBUG("FT This update file is ok.!");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_DOWNLOAD, 0, pcLocal);
    }
    else
    {
        LOG_RUNLOG_DEBUG("FT This update file is error.!");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_DOWNLOAD, 1, pcLocal);
    }
}

static void FPUpdateConfigDone(YKHandle hHandle, BOOL enState, const char* pcLocal, const char* pcRemote )
{
	time_t curtime;
	char command[512]={0x0};
	struct tm *loctime;
	curtime = time(NULL);
	loctime = localtime(&curtime);
#if _TM_TYPE_ == _TR069_
	evcpe_event_transfer(0, loctime, loctime);
#endif
	if(enState)
    {
        LOG_RUNLOG_DEBUG("FT This Config file is success.!");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_DOWNLOAD, 0, pcLocal);
        UpdateConfig();
        ExitApp();
    }
    else
    {
        LOG_RUNLOG_DEBUG("FT This update file is error.!");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_DOWNLOAD, 1, pcLocal);
    }
}

static void FPUploadDone(YKHandle hHandle, BOOL enState, const char* pcLocal, const char* pcRemote )
{
	time_t curtime;
	struct tm *loctime;
	curtime = time(NULL);
	loctime = localtime(&curtime);

	//evcpe_event_transfer(enState, loctime, loctime);

	if(enState)
    {
        LOG_RUNLOG_DEBUG("FT This upload file is ok.!");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_UPLOAD, 0, pcLocal);
    }
    else
    {
        LOG_RUNLOG_DEBUG("FT This upload file is error.!");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_UPLOAD, 1, pcLocal);
    }
}


static void	TMABAdvertUpdate(void)
{
	int iErrCode;

	TMAB_ADVERT_UPDATE_ST *pstSendMsg = (TMAB_ADVERT_UPDATE_ST *)malloc(sizeof(TMAB_ADVERT_UPDATE_ST));

	if(pstSendMsg == NULL)
	{
		return;
	}

	pstSendMsg->uiPrmvType = TMAB_ADVERT_UPDATE;

	LOG_RUNLOG_DEBUG("FT [%s] TMABAdvertUpdate", __func__);
	iErrCode = YKWriteQue( g_pstABMsgQ , pstSendMsg,  0);

}

static void FPUpdateAdvertDone(YKHandle hHandle, BOOL enState, const char* pcLocal, const char* pcRemote)
{
	time_t curtime;
	char command[512]={0x0};
	struct tm *loctime;
	curtime = time(NULL);
	loctime = localtime(&curtime);

	//evcpe_event_transfer(enState, loctime, loctime);

	if(enState)
    {
		LOG_RUNLOG_DEBUG("FT [%s] Advert file is success.!", __func__);
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_DOWNLOAD, 0, pcLocal);
        TMABAdvertUpdate();
    }
    else
    {
    	LOG_RUNLOG_DEBUG("FT [%s] Advert file is error.!", __func__);
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_DOWNLOAD, 1, pcLocal);
        //TMABAdvertUpdate();
    }
}

static void FPUploadPicDone(YKHandle hHandle, BOOL enState, const char* pcLocal, const char* pcRemote )
{ // add by cs 2012-11-16
	time_t stTimep;
	struct tm *pstTm;
	char *pcValue = NULL;
	char acRemoteNewFileName[256] = {0};
	time(&stTimep);
	pstTm = localtime(&stTimep);
	unsigned int uiTime = 0;
	unsigned int uiPicTime = 0;
	char acPicTimeWrite[128]={0x0};

	if(enState)
    {
        LOG_RUNLOG_DEBUG("FT Upload picture file is ok.!time %02d%02d%02d", \
        		          pstTm->tm_hour, pstTm->tm_min, pstTm->tm_sec);


        LOG_RUNLOG_DEBUG("FT upload ftp ok time!");
        if ((NULL == pcRemote) && (NULL == pcLocal))
        	return ;
        LOG_RUNLOG_DEBUG("FT pcRemote temp Filename: [%s]",pcRemote);

        //去掉文件名前的 temp
        strcpy(acRemoteNewFileName,pcRemote);
		pcValue = strstr(pcRemote,"_");
		if (pcValue == NULL)
			return ;
		strcpy(acRemoteNewFileName + strlen("./"),pcValue+1);
		LOG_RUNLOG_DEBUG("FT pcRemote temp filename  [%s] ; remote new FileName is [%s]",
				pcRemote, acRemoteNewFileName);

		if (FP_TRANSFER_FINISH == FPRename(hHandle, pcRemote, acRemoteNewFileName))
		{
			LOG_RUNLOG_INFO("FT Rename UploadPicture success!");
#ifdef TM_TAKE_PIC
			gettimeofday(&g_PicEndTime, NULL);
			uiPicTime = PICTimeInterval(g_CapturePictureStart, g_CapturePictureEnd);
			uiTime = PICTimeInterval(g_CapturePictureStart, g_PicEndTime);

			sprintf(acPicTimeWrite, "echo first %02d%02d%02d    %d   %d   >> /home/YK-IDBT/config-idbt/time.log",
					pstTm->tm_hour, pstTm->tm_min, pstTm->tm_sec, uiPicTime, uiTime);
			printf("FTP **** %s\n", acPicTimeWrite);
//			system(acPicTimeWrite);

			PICFtpOk();      // FTP 上传完成
#endif
		}
		else
		{
			LOG_RUNLOG_WARN("FT Rename UploadPicture error!");

		}


    }
    else
    {
        LOG_RUNLOG_DEBUG("FT Upload  picture file is error !time  %02d%02d%02d", \
    	        		pstTm->tm_hour, pstTm->tm_min, pstTm->tm_sec);
    }
}


static void FPUploadLogsDone(YKHandle hHandle, BOOL enState, const char* pcLocal, const char* pcRemote )
{
	time_t curtime;
	struct tm *loctime;
	curtime = time(NULL);
	loctime = localtime(&curtime);

//	evcpe_event_transfer(0, loctime, loctime);

	if(enState)
    {
		//==========add by hb============
#ifdef IDBT_AUTO_TEST
		ATLogUpdateSuccess();
#endif
		//==========add by hb============
        LOG_RUNLOG_DEBUG("FT This upload file is ok.!");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_UPLOAD, 0, pcLocal);
        remove(pcLocal);
    }
    else
    {
        LOG_RUNLOG_DEBUG("FT This upload file is error.!");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_FTP_UPLOAD, 1, pcLocal);
    }
}

static FP_TRANSFER_EN FPUploadLogs(YKHandle hFTP, FP_TRANSFER_DATA_ST*pstData)
{
    FP_TRANSFER_EN enState=FP_TRANSFER_ERROR;
    YKHandle hHandle = YKFindFirstFile(pstData->acLocalUrl);
    char pcLocalUrl[256]={0x0};
    char acRemoteUrl[256]={0x0};
    char acFileName[256]={0x0};
    char *pcFileName;
    while(NULL!=hHandle)
    {
        if(YK_TYPE_FILE==YKGetFileType(hHandle))
        {
            sprintf(pcLocalUrl, "%s%s", pstData->acLocalUrl, YKGetFileName(hHandle));
#if _TM_TYPE_ == _GYY_I6_
            strcpy(acFileName,YKGetFileName(hHandle));
            if((pcFileName = strchr(acFileName,'.')))
            {
            	*pcFileName = 0X0;
            }
            sprintf(acRemoteUrl, "%s%s_%ld.log", pstData->acRemoteUrl, acFileName,FPGetLocalFileSize(pcLocalUrl));
#else
            sprintf(acRemoteUrl, "%s%s", pstData->acRemoteUrl, YKGetFileName(hHandle));
#endif
            enState=FPUpload(hFTP, pcLocalUrl, acRemoteUrl, FP_TRANSFER_MODE_BINARY, FALSE, 0);
            if(pstData->Done)
            {
                pstData->Done(hHandle, enState==FP_TRANSFER_FINISH, pcLocalUrl, acRemoteUrl);
            }
        }
        if(!YKFindNextFile(hHandle))
        {
            YKFindClose(hHandle);
            break;
        }
    }
    return enState;
}

static void*   FPTransferThreadFunc(void*pvData)
{
    FP_TRANSFER_DATA_ST* pstData=(FP_TRANSFER_DATA_ST* )pvData;
    FP_CALLBACK_VTABLE_ST stVtable={NULL, pstData->Process};
    YKHandle hHandle = FPOpen(pstData->pacHost, pstData->iPort, pstData->acUserName, pstData->acPassword, &stVtable);
    FP_TRANSFER_EN enState=FP_TRANSFER_ERROR;
    int tryCount = 0;
    if(NULL!=hHandle)
    {
        if(pstData->enMode==FP_MODE_GET)
        {
            enState = FPDownload(hHandle, pstData->acLocalUrl, pstData->acRemoteUrl, FP_TRANSFER_MODE_BINARY, FALSE, 0);   
        }
        else
        {
            if(pstData->enType==FP_FILE_LOG)
            {
                //browse folder.
                enState=FPUploadLogs(hHandle, pstData);

            	//==========add by hb============
#ifdef IDBT_AUTO_TEST
                ATSendLogResult();
#endif
            	//==========add by hb============
            }
            else
            {
                while(tryCount ++ < pstData->iTryTransferCount && enState != FP_TRANSFER_FINISH){
                
                    enState=FPUpload(hHandle, pstData->acLocalUrl, pstData->acRemoteUrl, FP_TRANSFER_MODE_BINARY, FALSE, 0);
                    LOG_RUNLOG_DEBUG("Current update count is [%d]", tryCount);
                }
            }
        }
#if _YK_OS_==_YK_OS_LINUX_
        {
        	char acCMD[256]={0x0};
        	sprintf(acCMD, "busybox chmod 777 %s", pstData->acLocalUrl);
        	system(acCMD);
        }
#endif
        if(FP_TRANSFER_FINISH!=enState)
        {
        	//Send Alarm.
#if _TM_TYPE_ == _YK069_
        	TMUpdateAlarmState(TM_ALARM_TRANSFER, '1');
#else
        	TMUpdateAlarmState(TM_ALARM_TRANSFER, "1");
#endif
        }
        else if(pstData->Done && FP_FILE_LOG!=pstData->enType)
        {
#if _TM_TYPE_ == _YK069_
        	TMUpdateAlarmState(TM_ALARM_TRANSFER, '0');
#else
        	TMUpdateAlarmState(TM_ALARM_TRANSFER, "0");
#endif
        }
        if(pstData->Done && pstData->enType != FP_FILE_LOG)
        {
        	pstData->Done(hHandle, enState==FP_TRANSFER_FINISH, pstData->acLocalUrl, pstData->acRemoteUrl);
        }
        YKDestroyThread(pstData->hThread);
        pstData->hThread=NULL;
        FPClose(hHandle);
    }else{
        if(pstData->Done)
        {
        	pstData->Done(hHandle,FALSE, pstData->acLocalUrl, pstData->acRemoteUrl);

        }
    }
    YKFree(pstData);
    return NULL;
}

static void FPStartTransfer(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FP_FILE_TYPE_EN enType, FP_TRANSFER_MODE_EN enMode, int nTryTransferCount,
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType)
{
	pthread_t threadId;
	pthread_attr_t attr;
    FP_TRANSFER_DATA_ST* pstData= YKNew0(FP_TRANSFER_DATA_ST, 1);
    strcpy(pstData->pacHost, pacHost);
    strcpy(pstData->acUserName, pcUserName);
    strcpy(pstData->acPassword, pcPassWord);
    strcpy(pstData->acLocalUrl, pcLocal);
    strcpy(pstData->acRemoteUrl, pcRemote);
    LOG_RUNLOG_DEBUG("FT [%s] pacHost = %s, pcUserName = %s, pcPassWord = %s, pcLocal = %s, pcRemote = %s ", __func__, pacHost, pcUserName, pcPassWord, pcLocal, pcRemote);
    pstData->iPort=iPort;
    pstData->enType = enType;
    pstData->enMode=enMode;
    pstData->Process=Process;
    pstData->Done=done;
    pstData->iTryTransferCount = nTryTransferCount;

    if(enTransferType == FP_TRANSFER_NOWAIT)
    {
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create((pthread_t *)(&pstData->hThread), &attr, FPTransferThreadFunc, pstData);
		pthread_attr_destroy(&attr);
    }
    else
    {
    	pstData->hThread=YKCreateThread(FPTransferThreadFunc, pstData);
		if(!pstData->hThread)
		{
			YKFree(pstData);
		}
		else
		{
			if(enTransferType == FP_TRANSFER_WAIT)
			{
				YKJoinThread(pstData->hThread);
			}
		}
    }
} 

/**
 *@brief Download application from pcRemote.
 *@param[in] pacHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] local Local file path.
 *@param[in] pcRemote Remote file path.
 */
void FPUpdateApplication(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType)
{
#if _YK_OS_==_YK_OS_LINUX_
    char temp[128] = {0x0};
    char* remotefilename = strrchr(pcRemote, '/');
    system("busybox mkdir /data/data/com.fsti.ladder/update");
    sprintf(temp, "/data/data/com.fsti.ladder/update/%s", NULL == remotefilename?pcRemote:remotefilename+1);
#endif
    FPStartTransfer(pacHost, iPort, pcUserName, pcPassWord, temp, 
        pcRemote, FP_FILE_APP, FP_MODE_GET, 1, Process, FPUpdateApplicationDone,enTransferType);
}

/**
 *@brief Download config from pcRemote.
 *@param[in] pacHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] pcLocal Local file path.
 *@param[in] pcRemote Remote file path.
 */
void FPUpdateConfig(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType)
{
    FPStartTransfer(pacHost, iPort, pcUserName, pcPassWord, pcLocal, 
        pcRemote, FP_FILE_CONFIG, FP_MODE_GET, 1, Process, FPUpdateConfigDone,enTransferType);
}

/**
 *@brief Download ringing file from pcRemote.
 *@param[in] pacHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] pcLocal Local file path.
 *@param[in] pcRemote Remote file path.
 */
void FPUpdateRinging(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType)
{
    FPStartTransfer(pacHost, iPort, pcUserName, pcPassWord, pcLocal, 
        pcRemote, FP_FILE_RINGING, FP_MODE_GET, 1, Process, FPUpdateDone,enTransferType);
}


void FPUpdateAdvert(
    const char* pacHost, int iPort,
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote,
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType)
{
    LOG_RUNLOG_DEBUG("FT FPUpdateAdvert[%s] pacHost = %s, pcUserName = %s, pcPassWord = %s, pcLocal = %s, pcRemote = %s", __func__, pacHost, pcUserName, pcPassWord, pcLocal, pcRemote);
    FPStartTransfer(pacHost, iPort, pcUserName, pcPassWord, pcLocal,
        pcRemote, FP_FILE_ADVERT, FP_MODE_GET, 1, Process, FPUpdateAdvertDone,enTransferType);
}

/**
 *@brief Upload logs to pcRemote.
 *@param[in] pacHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] pcLocal Local file path.
 *@param[in] pcRemote Remote file path.
 */
void FPPutLog(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType)
{
    FPStartTransfer(pacHost, iPort, pcUserName, pcPassWord, 
        pcLocal, pcRemote, FP_FILE_LOG, FP_MODE_PUT, 1, Process, FPUploadLogsDone,enTransferType);
}

/**
 *@brief Upload call capture Picture to pcRemote.
 *@param[in] pacHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] pcLocal Local file path.
 *@param[in] pcRemote Remote file path.
 *add by cs for capture picture 2012-11-16
 */
void FPPutPicture(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType)
{    
    FPStartTransfer(pacHost, iPort, pcUserName, pcPassWord, 
        pcLocal, pcRemote, FP_FILE_OTHER, FP_MODE_PUT, UPDATA_PIC_CNT, Process, FPUploadPicDone,enTransferType);
}

/**
 *@brief Upload file/folders to pcRemote.
 *@param[in] pacHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] pcLocal Local file path.
 *@param[in] pcRemote Remote file path.
 */
void FPPut(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType)
{
    FPStartTransfer(pacHost, iPort, pcUserName, pcPassWord, pcLocal, 
        pcRemote, FP_FILE_OTHER, FP_MODE_PUT, 1, Process, FPUploadDone,enTransferType);
}

/**
 *@brief Download file from pcRemote.
 *@param[in] pacHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] pcLocal Local file path.
 *@param[in] pcRemote Remote file path.
 */
void FPGet(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType)
{
    FPStartTransfer(pacHost, iPort, pcUserName, pcPassWord, 
        pcLocal, pcRemote, FP_FILE_OTHER, FP_MODE_GET, 1, Process, FPUpdateDone,enTransferType);
}

/**
 *@brief To detect the current version
 *
 *If is Upgrade version it will Comparison of old and new version 
 *number to determine whether you need to upgrade
 *@param[in] mark Upgraded version of the tag
 *@param[in] oldVersion Old version number
 *@return @see FP_VERSION_CHECK_STATE_EN 
 */
FP_VERSION_CHECK_STATE_EN FPCheckApplication(const char* pcMark, const char* pcOldVersion)
{
    FP_VERSION_CHECK_STATE_EN enState = FP_CHECK_NORMAL;

    if(pcMark && 0==strcmp(pcMark, FP_VERSION_MARK))
    {
    	printf("Start FPCheckAppVersion !\n");
        enState = FP_CHECK_UPGRADE;
    }

    if(FP_CHECK_UPGRADE == enState &&
    		FPCheckAppVersion(pcOldVersion, YK_APP_VERSION))
    {
    	printf("FPCheckAppVersion error !\n");
    	return FP_CHECK_ERROR;
    }
    printf("\t enState = %s\n", enState == FP_CHECK_UPGRADE?"UpdateVersion":"NormalVersion");
    return enState;
}

//
int FTPProcessCmdFunc(const char *cmd)
{
	LOG_RUNLOG_DEBUG("FT %s", cmd);
	//FPPutLog("196.196.196.145", 21, "guest", "guest", "./update/", "./", NULL, NULL, FP_TRANSFER_NOWAIT);
	//FPGet("196.196.196.145", 21, "guest", "guest", "1234.pdf", "1234.pdf", NULL, NULL, FP_TRANSFER_NOWAIT);
	//FPUpdateApplication("196.196.196.145", 21, "guest", "guest", "YK-IDBT.bz2", "YK-IDBT.bz2.53286", NULL, NULL, FP_TRANSFER_NOWAIT);
}

