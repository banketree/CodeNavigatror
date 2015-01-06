#include "../INC/FtpCore.h"
#include <YKSystem.h>
#include <LOGIntfLayer.h>

static int FPHostProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    char acRecv[256]={0x0};
    char *p1,*p2 = NULL;
    int len;
    memcpy(acResult, pvData, sizeof(acResult) -1);
    if(atoi(acResult) != iReturnCode || atoi(acResult)==220)
    {
    	p1 = pvData;
        do
        {
        	//处理一次多条
        	while(1)
        	{
            	p2 = strstr(p1,"\r\n");
            	p2 += 2;
            	if(*p2 == 0x0)
            	{
            		break;
            	}
            	p1 = p2;
        	}

        	if(p1[3] == '-')
        	{
                len = hHandle->Recv(hHandle->LinkCommand, acRecv, sizeof(acRecv));
                LOG_RUNLOG_DEBUG("FT [%s] Recv = %s", __func__, acRecv);
                p1 = acRecv;
                continue;
        	}
        	else if(p1[3] == ' ')
        	{
        		memcpy(acResult, p1, sizeof(acResult) -1);
        		break;
        	}
        }while(len > 0);
    }
    return atoi(acResult)==iReturnCode;
}

static int FPUserProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    memcpy(acResult, pvData, sizeof(acResult) -1);
    return atoi(acResult)==iReturnCode;
}

static int FPPassProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    memcpy(acResult, pvData, sizeof(acResult) -1);
    return atoi(acResult)==iReturnCode;
}

static int FPTypeProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    memcpy(acResult, pvData, sizeof(acResult) -1);
    return atoi(acResult)==iReturnCode;
}

static int FPPasvProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    char* pcTmp=NULL, *pcTmp2=NULL, *pcTmp3=(char*)pvData;
    memcpy(acResult, pvData, sizeof(acResult) -1);
    if(atoi(acResult)==iReturnCode)
    {
        pcTmp=strrchr(pcTmp3, ')');
        if(NULL!=pcTmp)
        {
            *(pcTmp)='\0';
            pcTmp2=strrchr(pcTmp3, ',');
            if(NULL!=pcTmp2)
            {
                hHandle->PortData=atoi(pcTmp2+1);
                *pcTmp2=0x0;
                pcTmp=strrchr(pcTmp3, ',');
                if(pcTmp)
                {
                    hHandle->PortData+=atoi(pcTmp+1)*256;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int FPSizeProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    int i=0;
    memcpy(acResult, pvData, sizeof(acResult) -1);
    if(atoi(acResult)==iReturnCode)
    {
        for(i=4; i< iSize; i++)
        {
            if(isdigit(((char*)pvData)[i]))
            {                                                 
                return hHandle->TransInfo.lRemoteFileSize=atoi(&((char*)pvData)[i]);
            }
        }
    }
    return 0;
}

static int FPRetrProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    memcpy(acResult, pvData, sizeof(acResult) -1);
    return atoi(acResult)==iReturnCode;
}

static int FPStorProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    memcpy(acResult, pvData, sizeof(acResult) -1);
    return atoi(acResult)==iReturnCode||atoi(acResult)==150;
}

static int FPAppeProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    memcpy(acResult, pvData, sizeof(acResult) -1);
    return atoi(acResult)==iReturnCode;
}

static int FPAborProcess(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode)
{
    char acResult[4] = {0x0};
    memcpy(acResult, pvData, sizeof(acResult) -1);
    return atoi(acResult)==iReturnCode;
}

static int FPRntfProcess(FP_CORE_ST* handle, void* data, int size, int returnCode){
    char ret[128] = {0x0};
    sprintf(ret, "%d", returnCode);
    return NULL!=strstr((const char* )data, ret);
}

static int FPRntoProcess(FP_CORE_ST* handle, void* data, int size, int returnCode){
    char ret[128] = {0x0};
    sprintf(ret, "%d", returnCode);
    return NULL!=strstr((const char* )data, ret);
}

static FP_COMMAND_ST g_stCommands[]={
    {FP_COMMAND_HOST, "host", 220, FPHostProcess},
    {FP_COMMAND_USER, "user", 331, FPUserProcess},  
    {FP_COMMAND_PASS, "pass", 230, FPPassProcess},
    {FP_COMMAND_TYPE, "type", 200, FPTypeProcess},
    {FP_COMMAND_PASV, "pasv", 227, FPPasvProcess},
    {FP_COMMAND_SIZE, "size", 213, FPSizeProcess},
    {FP_COMMAND_RETR, "retr", 150, FPRetrProcess},
    {FP_COMMAND_STOR, "stor", 125, FPStorProcess},
    {FP_COMMAND_APPE, "appe", 200, FPAppeProcess},
    {FP_COMMAND_ABOR, "abor", 226, FPAborProcess},
    {FP_COMMAND_RNFR, "rnfr", 350, FPRntfProcess},
    {FP_COMMAND_RNTO, "rnto", 250, FPRntoProcess},
};

static FP_COMMAND_ST* FPFindCommand(FP_COMMAND_EN enType)
{
    int i;
    for(i=0; i< sizeof(g_stCommands)/sizeof(FP_COMMAND_ST); i++)
    {
        if(g_stCommands[i].enType==enType)
        {
            return &g_stCommands[i];
        }
    }
    return NULL;
}

static void FPCallbackCommand(FP_CORE_ST* hHandle, FP_COMMAND_EN enType, const char* pcMessage)
{
    if(hHandle->Callback.CommandState)
    {
        hHandle->Callback.CommandState(enType, pcMessage);
    }
}

int FPServerMsg(FP_CORE_ST* hHandle, FP_COMMAND_EN enType)
{
    char acCommand[512]={0x0};
    FP_COMMAND_ST* pstFPC=FPFindCommand(enType);
    char acRecv[512] = {0x0};
    int iLen=0;
    int ret = 0;
    int tryCount = 0;
    if(NULL== pstFPC)
    {
        return 0;
    }

    do{
		iLen=hHandle->Recv(hHandle->LinkCommand, (void*)acRecv, sizeof(acRecv));
		LOG_RUNLOG_DEBUG("FT Recv:%s, tryCount = %d",acRecv, tryCount);
		ret = pstFPC->Process(hHandle, (void*)acRecv, iLen, pstFPC->ReturnCode);
		tryCount++;
	}while(0 == ret && tryCount < 2);
    LOG_RUNLOG_DEBUG("FT ret:%d,tryCount = %d",ret,tryCount);

    return ret;
}

int  FPCommand(FP_CORE_ST* hHandle, FP_COMMAND_EN enType, const char* pcMessage, ...)
{
    char acCommand[512]={0x0};
    FP_COMMAND_ST* pstFPC=FPFindCommand(enType);
    va_list va_prt;
    char acRecv[512] = {0x0};
    int iLen=0;
    int ret = 0;
    int tryCount = 0;
    if(NULL== pstFPC)
    {
        return 0;
    }
    sprintf(acCommand, "%s ", pstFPC->Context);
    if(NULL!=pcMessage)
    {
        va_start(va_prt,pcMessage);
        vsprintf(acCommand+strlen(acCommand),pcMessage,va_prt);
        va_end(va_prt);
    }
    strcat(acCommand, "\r\n");
    FPCallbackCommand(hHandle, enType, acCommand);

    do{
        hHandle->Send(hHandle->LinkCommand, (void*)acCommand, strlen(acCommand));
        iLen=hHandle->Recv(hHandle->LinkCommand, (void*)acRecv, sizeof(acRecv));
        LOG_RUNLOG_DEBUG("FT Send:%s, Recv:%s, tryCount = %d", acCommand, acRecv, tryCount);
        ret = pstFPC->Process(hHandle, (void*)acRecv, iLen, pstFPC->ReturnCode);
        tryCount++;

    }while(0 == ret && tryCount < 2);
    return ret;
}
