#include <YKApi.h>
#include <LOGIntfLayer.h>
#include <FPInterface.h>
#include <FtpCore.h>

int  FPCommand(FP_CORE_ST* hHandle, FP_COMMAND_EN enType, const char* pcMessage, ...);

static unsigned long FPGetHostAddress(const char* pacHost)
{
    struct hostent* pstHost  = NULL;
    unsigned long	lHostIP = 0;
    int				hSock = -1;
    pstHost = gethostbyname(pacHost);
    if(pstHost)
        lHostIP = *((unsigned long*) (pstHost->h_addr));
    else
        lHostIP = inet_addr(pacHost);
    return lHostIP;
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

static void  FPDisConnect(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
    closesocket((SOCKET)hHandle);
#else
    close((int)hHandle);
#endif
}

static YKHandle  FPConnect(const char* pacHost, int iPort, unsigned int uiProto,unsigned int timeout)
{
    YKHandle hHandle = NULL;
    struct sockaddr_in stClientAddr;
    int ret = 0;
	int error;
	int flags;
    if((hHandle = (YKHandle)socket(AF_INET, SOCK_STREAM, uiProto)) < 0)
    {
        return NULL;
    }
    memset(&stClientAddr, 0x0, sizeof(stClientAddr));
    stClientAddr.sin_addr.s_addr = FPGetHostAddress(pacHost);
    stClientAddr.sin_family = AF_INET;
    stClientAddr.sin_port = htons( (unsigned short)iPort );
    
    do 
    {
//        struct timeval tv_timeout;
//        tv_timeout.tv_sec = timeout/1000;
//        tv_timeout.tv_usec = 0;
//        if (setsockopt((int)hHandle, SOL_SOCKET, SO_SNDTIMEO, (void *) &tv_timeout, sizeof(struct timeval)) < 0) {
//            FPDisConnect(hHandle);
//            hHandle = NULL;
//            break;
//        }
//        tv_timeout.tv_sec = timeout/1000;
//        tv_timeout.tv_usec = 0;
//        if (setsockopt((int)hHandle, SOL_SOCKET, SO_RCVTIMEO, (void *) &tv_timeout, sizeof(struct timeval)) < 0) {
//            FPDisConnect(hHandle);
//            hHandle = NULL;
//            break;
//        }
        flags = fcntl((int)hHandle,F_GETFL,0);
        fcntl((int)hHandle, F_SETFL, flags|O_NONBLOCK);
        if (connect( (int)hHandle, (struct sockaddr*) &stClientAddr, sizeof(stClientAddr) ) == -1)
        {
        	if (errno == EINPROGRESS)
			{ // it is in the connect process
				struct timeval tv;
				fd_set writefds;
				tv.tv_sec = 3;
				tv.tv_usec = 0;
				FD_ZERO(&writefds);
				FD_SET((int)hHandle, &writefds);
				if (select((int)hHandle + 1, NULL, &writefds, NULL, &tv) > 0)
				{
					int len = sizeof(int);
					getsockopt((int)hHandle, SOL_SOCKET, SO_ERROR, &error, &len);
					if (error == 0)
					{
						ret = 0;
						//»Ö¸´Ì×½Ó×Ö
						fcntl((int)hHandle, F_SETFL, flags);
					}
					else
						ret = -1;
				}
				else
					ret = -1; //timeout or error happen
			}
			else
				ret = -1;
        }
        else
        {
        	ret = 0;
        }

        if(ret == -1)
        {
        	if(hHandle)
        	{
                FPDisConnect(hHandle);
                hHandle=NULL;
        	}
        }
    } while (FALSE);
    return hHandle;
}

static int   FPRecv(YKHandle hHandle, void*pvBuffer, int iSize)
{
    return recv((int)hHandle, (char*)pvBuffer, iSize, 0);
}

static int   FPSend(YKHandle hHandle, void*pvBuffer, int iSize)
{
    return send((int)hHandle, (char*)pvBuffer, iSize, 0);
}

long  FPGetLocalFileSize(const char* pcLocal)
{
    FILE* hPf=fopen(pcLocal, "rb");
    long lSize=0;
    if(NULL!=hPf)
    {
        fseek(hPf, 0L,SEEK_END);
        lSize = ftell(hPf);
        fclose(hPf);
    }
    return lSize;
}

static BOOL  FPRightCompareNoCaseBy(const char* pcSrc, const char* pcDest, char cFlag)
{
    const char* pcSrcTemp=strrchr(pcSrc, cFlag);
    const char* pcDestTemp=strrchr(pcDest, cFlag);
    if(NULL!=pcSrcTemp && NULL!=pcDestTemp)
    {
        return 0==strcmp(pcSrcTemp, pcDestTemp)?TRUE:FALSE;
    }
    else
    {
        return 0==strcmp(pcSrc, pcDest)?TRUE:FALSE;
    }
    return FALSE;
}

static void FPSetTransferFileInfo(FP_CORE_ST* pstFPC, const char* pcLocal, const char* pcRemote, const char* pcMode)
{
    BOOL bSameFileName=FPRightCompareNoCaseBy(pcLocal, pcRemote, '/');
    pstFPC->TransInfo.lRemoteFileSize=FPCommand(pstFPC, FP_COMMAND_SIZE, pcRemote);
    pstFPC->TransInfo.lLocalFileSize=FPGetLocalFileSize(pcLocal);
    if(pstFPC->TransInfo.lLocalFileSize==pstFPC->TransInfo.lRemoteFileSize &&bSameFileName)
    {
        pstFPC->TransState=FP_TRANSFER_FINISH;
        return;
    }
    if(0==strcmp(pcMode, "wb") && 0==pstFPC->TransInfo.lRemoteFileSize)
    {
        pstFPC->TransState=FP_TRANSFER_ERROR;
        return;
    }
    pstFPC->LocalFile=fopen(pcLocal, pcMode);
    if(!pstFPC->LocalFile)
    {
        pstFPC->TransState = FP_TRANSFER_ERROR;
    }
}

static void FPCalcTransferSpeed(FP_CORE_ST* pstFPC)
{
    YK_TIME_VAL_ST stNow;
    if(pstFPC->PreCalcSpeed.lSeconds==0)
    {
        YKGetTimeOfDay(&pstFPC->PreCalcSpeed);
        return;
    }
     YKGetTimeOfDay(&stNow);
     if(pstFPC->TransInfo.lTotalSize ==pstFPC->TransInfo.lTransferredSize ||
        stNow.lSeconds-pstFPC->PreCalcSpeed.lSeconds>=1)
     {
         pstFPC->TransInfo.lTotalTime+=stNow.lSeconds-pstFPC->PreCalcSpeed.lSeconds;
         pstFPC->TransInfo.dAvlSpeed = (double)pstFPC->TransInfo.lTransferredSize/pstFPC->TransInfo.lTotalTime;
         if(pstFPC->Callback.TransSpeed)
         {
            pstFPC->Callback.TransSpeed(&pstFPC->TransInfo);
         }
         YKGetTimeOfDay(&pstFPC->PreCalcSpeed);
     }
}

static BOOL FPIsNeedPause(FP_CORE_ST* pstFPC)
{ 
    YK_TIME_VAL_ST stNow;
    if(pstFPC->Pause)
    {
        YKGetTimeOfDay(&stNow);
        if(stNow.lMilliSeconds-pstFPC->PrePauseVal.lMilliSeconds>pstFPC->PauseTime)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void FPDealWithTransferState(FP_CORE_ST* pstFPC)
{
    char acRecv[256]={0x0};
    if(pstFPC->TransState != FP_TRANSFER_CONTINUE)
    {
        if(pstFPC->LinkData)
        {
            FPDisConnect(pstFPC->LinkData);
            pstFPC->LinkData=NULL;
        }
        if(pstFPC->LocalFile)
        {
            fclose(pstFPC->LocalFile);
            pstFPC->LocalFile=NULL;
        }
        if(pstFPC->TransState==FP_TRANSFER_FINISH)
        {
            pstFPC->Recv(pstFPC->LinkCommand, acRecv, sizeof(acRecv));
        }
        memset(&(pstFPC->TransInfo), 0x0, sizeof(FP_TRANSFER_INFO_ST));
    }
}

YKHandle  FPOpen(const char* pacHost, int iPort, const char* pcUserName, const char* pcPassWord, FP_CALLBACK_VTABLE_ST* pstVtable)
{
    FP_CORE_ST* pstFPC=YKNew0(FP_CORE_ST, 1);

    if(pstFPC)
    {
        strcpy(pstFPC->Server, pacHost);
        pstFPC->PortCommand=iPort;
        pstFPC->LinkCommand=FPConnect(pstFPC->Server, pstFPC->PortCommand, 0, 15000);

        if(pstFPC->LinkCommand == NULL)
        {
        	return NULL;
        }
        pstFPC->Send=FPSend;
        pstFPC->Recv=FPRecv;
        //¶ÁÈ¡FTP Server Msg
        if(FPServerMsg(pstFPC, FP_COMMAND_HOST) != 0)
        {
            //ÕËºÅµÇÂ½
            if(FPCommand(pstFPC, FP_COMMAND_USER, pcUserName) && \
               FPCommand(pstFPC, FP_COMMAND_PASS, pcPassWord))
            {
               if(NULL!=pstVtable)
               {
                   memcpy(&pstFPC->Callback, pstVtable, sizeof(pstFPC->Callback));
               }
               return (YKHandle)pstFPC;
            }
        }

        FPDisConnect(pstFPC->LinkCommand);
        YKFree(pstFPC);
    }
    return NULL;
}

FP_TRANSFER_EN FPDownload(YKHandle hHandle, const char* pcLocal, const char* pcRemote, FP_TRANS_MODE_EN enMode,\
    BOOL bPause, int iPauseTime)
{
    FP_CORE_ST* pstFPC=(FP_CORE_ST*)hHandle;
    int iReconnect = 0;
    pstFPC->Pause=bPause;
    pstFPC->PauseTime=iPauseTime;
    pstFPC->TransState = FP_TRANSFER_START;
    FPSetTransferFileInfo(pstFPC, pcLocal, pcRemote, "wb");
    if(pstFPC->TransState==FP_TRANSFER_FINISH || pstFPC->TransState == FP_TRANSFER_ERROR)
    {
        return pstFPC->TransState;
    }
    pstFPC->TransInfo.lTotalSize=pstFPC->TransInfo.lRemoteFileSize;
    FPCommand(pstFPC, FP_COMMAND_TYPE, "%s", enMode==FP_TRANSFER_MODE_BINARY?"I":"A");
    do{

        FPCommand(pstFPC, FP_COMMAND_PASV, NULL);
        pstFPC->LinkData=FPConnect(pstFPC->Server, pstFPC->PortData, 0, 200);
    }while(pstFPC->LinkData == NULL && iReconnect++ < 1);
    if(pstFPC->LinkData == NULL)
    {
    	pstFPC->TransState=FP_TRANSFER_ERROR;
		return pstFPC->TransState;
    }

    if(!FPCommand(pstFPC, FP_COMMAND_RETR, pcRemote))
    {
        FPDealWithTransferState(pstFPC);
        pstFPC->TransState=FP_TRANSFER_ERROR;
        return pstFPC->TransState;
    }
    pstFPC->TransState = FP_TRANSFER_CONTINUE;
    return FPContinueDownload(pstFPC);
}

FP_TRANSFER_EN FPContinueDownload(YKHandle hHandle)
{
    FP_CORE_ST* pstFPC=(FP_CORE_ST*)hHandle;
    int iSize=0;
    char acRecv[1024] = {0x0};
    static int nTryRecv=0;
    YKGetTimeOfDay(&(pstFPC->PrePauseVal));
    if(pstFPC->TransState != FP_TRANSFER_CONTINUE)
    {
        return pstFPC->TransState;
    }
    while(pstFPC->LinkData)
    {
        iSize = pstFPC->Recv(pstFPC->LinkData, acRecv, sizeof(acRecv));
        if(iSize<=0)
        {
           if(pstFPC->TransInfo.lTransferredSize!=pstFPC->TransInfo.lTotalSize)
            {
                if(iSize<=0 && nTryRecv++<=30){
                	LOG_RUNLOG_DEBUG("FT Recv data(size = %d) timeout(errno=%d), try recv(%d)", iSize, errno, nTryRecv);
                    continue;
                }
                LOG_RUNLOG_DEBUG("FT Recv data(size = %d) failed(errno=%d), try recv(%d)", iSize, errno, nTryRecv);
                pstFPC->TransState = FP_TRANSFER_ERROR;
            }
            else if(pstFPC->TransInfo.lTransferredSize==pstFPC->TransInfo.lTotalSize)
            {
                pstFPC->TransState = FP_TRANSFER_FINISH;
                pstFPC->TransInfo.lTransferredSize+=iSize;
                FPCalcTransferSpeed(pstFPC);
            }
            break;
        }
        nTryRecv=0;
        pstFPC->TransInfo.lTransferredSize+=iSize;
        fwrite(acRecv, iSize, sizeof(char), pstFPC->LocalFile);
        FPCalcTransferSpeed(pstFPC);
        if(FPIsNeedPause(pstFPC))
        {
            return pstFPC->TransState;
        }
    }
    FPDealWithTransferState(pstFPC);
    return pstFPC->TransState;
}

FP_TRANSFER_EN FPUpload(YKHandle hHandle, const char* pcLocal, const char* pcRemote, FP_TRANS_MODE_EN enMode,\
    BOOL bPause, int iPauseTime)
{
    FP_CORE_ST* pstFPC=(FP_CORE_ST*)hHandle;
    int iReconnect = 0;
    pstFPC->Pause=bPause;
    pstFPC->PauseTime=iPauseTime;
    pstFPC->TransState = FP_TRANSFER_START;
    FPSetTransferFileInfo(pstFPC, pcLocal, pcRemote, "rb");
    if(pstFPC->TransState==FP_TRANSFER_FINISH || pstFPC->TransState == FP_TRANSFER_ERROR)
    {
        return pstFPC->TransState;
    }
    pstFPC->TransInfo.lTotalSize=pstFPC->TransInfo.lLocalFileSize;
    FPCommand(pstFPC, FP_COMMAND_TYPE, "%s", enMode==FP_TRANSFER_MODE_BINARY?"I":"A");
    do{
        FPCommand(pstFPC, FP_COMMAND_PASV, NULL);
        pstFPC->LinkData=FPConnect(pstFPC->Server, pstFPC->PortData, 0, 2000);
    }while(pstFPC->LinkData == NULL && iReconnect++ < 1);
    if(pstFPC->LinkData == NULL)
    {
    	pstFPC->TransState=FP_TRANSFER_ERROR;
		return pstFPC->TransState;
    }
    if(!FPCommand(pstFPC, FP_COMMAND_STOR, pcRemote))
    {
        FPDealWithTransferState(pstFPC);
        pstFPC->TransState=FP_TRANSFER_ERROR;
        return pstFPC->TransState;
    }
    pstFPC->TransState = FP_TRANSFER_CONTINUE;
    return FPContinueUpload(pstFPC);
}

FP_TRANSFER_EN FPContinueUpload(YKHandle hHandle)
{
    FP_CORE_ST* pstFPC=(FP_CORE_ST*)hHandle;
    int iSize=0;
    int iSend=0;
    int iTrySend=0;
    char acRecv[1024] = {0x0};
    if(pstFPC->TransState != FP_TRANSFER_CONTINUE)
    {
        return pstFPC->TransState;
    }
    YKGetTimeOfDay(&(pstFPC->PrePauseVal));
    while(pstFPC->LinkData)
    {
        iSize=fread(acRecv, sizeof(char), sizeof(acRecv), pstFPC->LocalFile);
        if(iSize <=0)
        {
            if(pstFPC->TransInfo.lTransferredSize!=pstFPC->TransInfo.lTotalSize)
            {
                pstFPC->TransState = FP_TRANSFER_ERROR;
            }
            else if(pstFPC->TransInfo.lTransferredSize==pstFPC->TransInfo.lTotalSize)
            {
                pstFPC->TransState = FP_TRANSFER_FINISH;
                pstFPC->TransInfo.lTransferredSize+=iSize;
                FPCalcTransferSpeed(pstFPC);
            }
            break;
        }
        iTrySend=0;
        while(TRUE){
            iSend = pstFPC->Send(pstFPC->LinkData, acRecv, iSize);
            if(iSend <=0){
                if(iTrySend++<=30)
                {
                	LOG_RUNLOG_DEBUG("FT send data(size = %d) timeout(errno=%d), try recv(%d)", iSize, errno, iTrySend);
                    continue;
                }
                else
                {
                	LOG_RUNLOG_DEBUG("FT send data(size = %d) timeout(errno=%d), try recv(%d)", iSize, errno, iTrySend);
                    pstFPC->TransState = FP_TRANSFER_ERROR;
                    FPDealWithTransferState(pstFPC);
                    break;
                }
            }
            break;
        }

        pstFPC->TransInfo.lTransferredSize+=iSize;
        FPCalcTransferSpeed(pstFPC);
        if(FPIsNeedPause(pstFPC))
        {
            return pstFPC->TransState;
        }
    }
    FPDealWithTransferState(pstFPC);
    return pstFPC->TransState;
}

FP_TRANSFER_EN FPCreateDir(const char* pcFolder)
{

}

FP_TRANSFER_EN FPRename(YKHandle handle, const char* src, const char* dest)
{
	FP_CORE_ST* fpc=(FP_CORE_ST*)handle;
    if(NULL == fpc){
        return FP_TRANSFER_ERROR;
    }
    if(!FPCommand(fpc, FP_COMMAND_RNFR, src) || !FPCommand(fpc, FP_COMMAND_RNTO, dest)){
        return FP_TRANSFER_ERROR;
    }
    return FP_TRANSFER_FINISH;
}

void FPClose(YKHandle hHandle)
{
    FP_CORE_ST* pstFPC=(FP_CORE_ST*)hHandle;
    if(pstFPC->LinkData)
    {
        FPDisConnect(pstFPC->LinkData);
    }
    if(pstFPC->LinkCommand)
    {
        FPDisConnect(pstFPC->LinkCommand);
    }
    if(pstFPC->LocalFile)
    {
        fclose(pstFPC->LocalFile);
    }
    YKFree(pstFPC);
}
