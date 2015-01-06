/***************************************************************
  Copyright (C), 1988-2012, YouKe Tech. Co., Ltd.
  @file:
  @author:			hb
  @date:			2012.9.11
  @description:		//描述
  @version:			//版本
  @history:         // 历史修改记录
  		<author>	<time>   	<version >   <desc>
		hb	   		2012/9/11     1.0      	创建模块
***************************************************************/

#include "../INC/AutoTest.h"
#include <IDBT.h>
#include <YKApi.h>
#include <YKMsgQue.h>
#include <LOGIntfLayer.h>

AT_TEST_ST g_stAutoTest;
AT_LOG_ST g_stLogUpdate;

static int ATCDisplay = FALSE;
static char g_acGetTestID[AT_TO_PC_ID_SIZE+1] = AT_TO_PC_ID ;
static char g_acSendTestID[AT_TO_PC_ID_SIZE+1] = AT_TO_PC_ID;
static BOOL g_bTestStop = 0;
static YK_MSG_QUE_ST*  g_pstATMsgQ = NULL;
static YKHandle	g_hConnectPC	= 0;
static YKHandle g_hAutoTestHndl = NULL;	//自动测试线程句柄
static YKHandle g_hAutoTestRecv = NULL;	//自动测试接收线程句柄
static AT_INFO_ST g_stAutoTestInfo;	//网络信息结构体
static AT_GLOBAL_ST g_stATGlobalInfo;
static AT_REGISTER_STATE_EN g_enRigisterState = AT_REGISTER_STATE_REJECT;

int ATIsRunning()
{
	return g_stATGlobalInfo.iState ;
}

int ATStringFindFirst(unsigned char* pvBuffer, int iSize, char c)
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

void ATSendQueue(YK_MSG_QUE_ST*q, void* pvBuffer, int iSize)
{
    AT_MSG_DATA_ST *pstData = YKNew0(AT_MSG_DATA_ST, 1);
    if(NULL==pstData)
    {
    	return;
    }
    pstData->pvBuffer = pvBuffer;
    pstData->iSize= iSize;
    YKWriteQue(q, pstData,1);
}

unsigned long ATGetHostAddress(const char* pcHost)
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

/**************************************************************
  @brief:       		ATGetConfig
  @Description:			获取配置文件
  @return:				RST_OK 成功 RST_ERR 失败
**************************************************************/
int ATGetConfig()
{
	FILE *hCfgFd;
	char buf[64];
	//打开配置文件
	hCfgFd=fopen(AT_CONFIG_PATH,"r");
	if(hCfgFd == NULL)
	{
		hCfgFd=fopen(AT_CONFIG_PATH,"w+");
		if(hCfgFd == NULL)
		{
			return RST_ERR;
		}
		else
		{
			g_stATGlobalInfo.iState = 0;
#ifndef _YK_PC_
			strcpy(g_stATGlobalInfo.acHost,"0.0.0.0");
#else
			strcpy(g_stATGlobalInfo.acHost,"196.196.190.74");
#endif
			strcpy(g_stATGlobalInfo.acPort,"5068");
			g_stATGlobalInfo.iReconnect = 0;

			fprintf(hCfgFd,"0=IDLE,1=RUNNING\n");
			//设置类型
			fprintf(hCfgFd,"State=%d\n",g_stATGlobalInfo.iState);
			//设置IP
			fprintf(hCfgFd,"Host=%s\n",g_stATGlobalInfo.acHost);
			//设置掩码
			fprintf(hCfgFd,"Port=%s\n",g_stATGlobalInfo.acPort);
			//设置一直重连
			fprintf(hCfgFd,"Reconnect=%d\n",g_stATGlobalInfo.iReconnect);

			//关闭PPPD配置文件
			fclose(hCfgFd);

			return RST_OK;
		}
	}
	else
	{
		fscanf(hCfgFd,"0=IDLE,1=RUNNING\n",buf);
		//设置类型
		fscanf(hCfgFd,"State=%d\n",&g_stATGlobalInfo.iState);
		//设置IP
		fscanf(hCfgFd,"Host=%s\n",g_stATGlobalInfo.acHost);
		//设置掩码
		fscanf(hCfgFd,"Port=%s\n",g_stATGlobalInfo.acPort);
		//设置一直重连
		fscanf(hCfgFd,"Reconnect=%d\n",&g_stATGlobalInfo.iReconnect);

		//关闭配置文件
		fclose(hCfgFd);

		return RST_OK;
	}
}

/**************************************************************
  @brief:       		ATSetConfig
  @Description:			设置配置文件
  @return:				RST_OK 成功 RST_ERR 失败
**************************************************************/
int ATSetConfig()
{
	FILE *hCfgFd;
	//打开配置文件
	hCfgFd=fopen(AT_CONFIG_PATH,"w+");
	if(hCfgFd == NULL)
	{
		return RST_ERR;
	}
	fprintf(hCfgFd,"0=IDLE,1=RUNNING\n");
	//设置类型
	fprintf(hCfgFd,"State=%d\n",g_stATGlobalInfo.iState);
	//设置IP
	fprintf(hCfgFd,"Host=%s\n",g_stATGlobalInfo.acHost);
	//设置掩码
	fprintf(hCfgFd,"Port=%s\n",g_stATGlobalInfo.acPort);
	//设置一直重连
	fprintf(hCfgFd,"Reconnect=%d\n",g_stATGlobalInfo.iReconnect);

	//关闭配置文件
	fclose(hCfgFd);

	return RST_OK;
}

void  ATDisConnect(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
    closesocket((SOCKET)hHandle);
#else
    if((int)hHandle > 0)
    {
        close((int)hHandle);
    }
#endif
}

YKHandle  ATConnect(const char* pcHost, int iPort, unsigned int uiProto)
{
//    BOOL ret = FALSE;
    YKHandle hHandle = 0;
    struct sockaddr_in clientAddr;
    int ret = 0;
	int error;
	int flags;
    if((hHandle = (YKHandle)socket(AF_INET, SOCK_STREAM, uiProto)) < 0)
    {
    	return NULL;
    }
    memset(&clientAddr, 0x0, sizeof(clientAddr));
    clientAddr.sin_addr.s_addr = ATGetHostAddress(pcHost);
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons( (unsigned short)iPort );

    flags = fcntl((int)hHandle,F_GETFL,0);
	fcntl((int)hHandle, F_SETFL, flags|O_NONBLOCK);
	if (connect( (int)hHandle, (struct sockaddr*) &clientAddr, sizeof(clientAddr) ) == -1)
	{
		if (errno == EINPROGRESS)
		{ // it is in the connect process
			struct timeval tv;
			fd_set writefds;
			tv.tv_sec = 10;
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
					//恢复套接字的文件状态
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
			ATDisConnect(hHandle);
			hHandle=NULL;
		}
	}
    return hHandle;
}

int  ATSelect(void* handle, unsigned long timeout)
{
    fd_set set;
    struct timeval timeval;
    int ret;
    timeval.tv_sec = timeout/1000;
    timeval.tv_usec = timeout % 1000;
    FD_ZERO(&set);
    FD_SET((int)handle, &set);
    return select((int)handle+1, &set, NULL, NULL, &timeval);//0 time out, -1 failed, other success
}

int ATRecv(YKHandle hHandle, void*pvBuffer, int iSize, int iTimeOut)
{
    switch(ATSelect(hHandle, iTimeOut))
    {
    case 0://time out
        return 0;
    case -1://failed
        return -1;
    default:
        return recv((int)hHandle, (char*)pvBuffer, iSize, 0);
    }
}

int ATRecvMsg(YKHandle handle, void* buffer, int size)
{
	if(NULL != handle)
	{
		return recv((int)handle, (char*)buffer, size, 0);
	}
	return -1;
}

int ATSendMessage(YKHandle handle, void* buffer, int size)
{
	if(NULL != handle)
	{
		return send((int)handle, (char*)buffer, size, 0);
		//return send((int)handle, "\n", strlen("\n"), 0);
	}
	return 0;
}

void ATRecvProcess(RECV_DATA_CTX_ST* ctx)
{
	char buffer[SIZE_1024/2] = {0};
	int recvDataLen = 0;
	int index;
	int beginIndex;
	char *ethData = NULL;
	if(ctx->counter >= RECV_LMT_TIMES)
	{
		ctx->len = 0;
		ctx->head = ctx->buf;
		ctx->counter = 0;
		ctx->state = BEGIN_FLAG_NOT_FOUND;
		return;
	}

//	recvDataLen = ATRecvMsg(g_hConnectPC, buffer, sizeof(buffer));
//#ifdef _YK_PC_
//		printf("====%d====%d====%d====\n",g_hConnectPC,errno,recvDataLen);
//#endif
//	if( errno == EAGAIN || errno == EWOULDBLOCK)
//	{
//		return;
//	}
//	if(recvDataLen <= 0)
//	{
//		ATDisConnect(g_hConnectPC);
//		g_hConnectPC = 0;
//		return;
//	}

	recvDataLen = ATRecv(g_hConnectPC, buffer, sizeof(buffer), 2000);
	switch(recvDataLen)
	{
	case 0:
		//printf("=====%s===%d========\n",__FILE__,__LINE__);
		return;
	case -1:
		if(ATCDisplay)
		{
			LOG_RUNLOG_DEBUG("AT connect_%d_errno_%d_recv_%d",g_hConnectPC,errno,recvDataLen);
		}
		if( errno == EAGAIN)
		{
			return;
		}
		else
		{
			ATDisConnect(g_hConnectPC);
			g_hConnectPC = 0;
			g_enRigisterState = AT_REGISTER_STATE_REJECT;
			return;
		}
	}

	if(&ctx->buf[SIZE_1024] < ctx->head + ctx->len + recvDataLen)
	{
		ctx->len = 0;
		ctx->head = ctx->buf;
		ctx->counter = 0;
		ctx->state = BEGIN_FLAG_NOT_FOUND;
	}
	memcpy(ctx->head + ctx->len, buffer, recvDataLen);
	ctx->len += recvDataLen;

	index = 0xFFFF;
	while((ctx->len > 0) && (index != 0))
	{
		index = ATStringFindFirst((unsigned char *)ctx->head, ctx->len, AT_CODEC_START_FLAG);
		if(index != 0)
		{
			if(ctx->state == BEGIN_FLAG_FOUND)
			{
				beginIndex = ATStringFindFirst((unsigned char *)ctx->head, index, AT_CODEC_END_FLAG);
				if(beginIndex != 0)
				{
					ctx->counter = 0; //
					ethData = (char *)malloc(beginIndex + 1);
					if(ethData != NULL)
					{
						*ethData = AT_CODEC_START_FLAG;
						memcpy(ethData + 1, ctx->head, beginIndex);
						ATSendQueue(g_pstATMsgQ,ethData,beginIndex + 1);
#ifdef _YK_PC_
						LOG_RUNLOG_DEBUG("AT Recv Data : %s", ethData);
#endif
					}
				}
			}

			ctx->head += index ;
			ctx->len  -= index ;
			ctx->state = BEGIN_FLAG_FOUND;
		}
		else
		{
			if(ctx->state == BEGIN_FLAG_FOUND)
			{
				index = ATStringFindFirst((unsigned char *)ctx->head, ctx->len, AT_CODEC_END_FLAG);
				if(index == 0) //
				{
					ctx->counter++;
				}
				else
				{
					ctx->counter = 0;     //
					ethData = (char *)malloc(index + 1);
					if(ethData != NULL)
					{
						*ethData = AT_CODEC_START_FLAG;
						memcpy(ethData + 1, ctx->head, index);
						ATSendQueue(g_pstATMsgQ,ethData,index + 1);
#ifdef _YK_PC_
						LOG_RUNLOG_DEBUG("AT Recv Data : %s", ethData);
#endif
					}

					ctx->head += index; //
					ctx->len  -= index;
					ctx->state = BEGIN_FLAG_NOT_FOUND;
				}
			}
			else    //(ctx->state == BEGIN_FLAG_NOT_FOUND)
			{
				ctx->head = ctx->buf;
				ctx->len  = 0;
			}
		}
	}

	if(ctx->head != ctx->buf)
	{
		memcpy(ctx->buf, ctx->head, ctx->len);
		ctx->head = ctx->buf;
	}
}

AT_CMD_EN ATCodecDecode(const char* buffer_in, int length, char**buffer_out)
{
	int iSize = 0;
	char temp[AT_CODEC_CMD_SIZE + 1] = {0x0};
	AT_CMD_EN cmd =AT_CODEC_ERROR;
    if(NULL == buffer_in)
    {
        return cmd;
    }

    do
    {
	    if(buffer_in[0] != AT_CODEC_START_FLAG || buffer_in[length - 1] != AT_CODEC_END_FLAG)
        {
            break;
        }

	    //^/1234567890/0/201/OK~
	    iSize = length - AT_CODEC_START_FLAG_SIZE - AT_TO_PC_ID_SIZE - AT_CODEC_APART_FLAG_SIZE
	    		- AT_CODEC_TYPE_FLAG_SIZE - AT_CODEC_APART_FLAG_SIZE - AT_CODEC_CMD_SIZE
	    		- AT_CODEC_APART_FLAG_SIZE - AT_CODEC_END_FLAG_SIZE;

	    if(iSize > 0 )
	    {
			*buffer_out = (char*)malloc(iSize);
			if(NULL == *buffer_out)
			{
				iSize = AT_CODEC_ERROR;
				break;
			}
			memset(*buffer_out, 0x0, iSize);
			sscanf(buffer_in,"^%10s/0/%3s/%[^~]",g_acGetTestID,temp,*buffer_out);
	    }
	    else
	    {
	    	sscanf(buffer_in,"^%10s/0/%3s/~"    ,g_acGetTestID,temp);
	    }

	    cmd = (AT_CMD_EN)atoi(temp);

    }while(FALSE);

    if(iSize == AT_CODEC_ERROR)
	{
		if(*buffer_out)
		{
			free(*buffer_out);
		}
		*buffer_out = NULL;
	}

    return cmd;
}

int ATCodecEncode(AT_CMD_EN cmd_type, const char* buffer_in, int length, char**buffer_out)
{
    int size = -1;
    char tmp[256]={0x0};
    if(cmd_type < 0)
    {
        return size;// cmd type error
    }
    *buffer_out = NULL;

    do
    {
    	//^/0000000000/0/100/SIP号码:ip地址~
        int codec_size = length + AT_CODEC_START_FLAG_SIZE + AT_TO_PC_ID_SIZE
        		+ AT_CODEC_APART_FLAG_SIZE + AT_CODEC_TYPE_FLAG_SIZE + AT_CODEC_APART_FLAG_SIZE
        		+ AT_CODEC_CMD_SIZE + AT_CODEC_APART_FLAG_SIZE + AT_CODEC_END_FLAG_SIZE;

        *buffer_out = (char*)malloc(codec_size);
        if(NULL == *buffer_out)
        {
             break;
        }
        memset(*buffer_out, 0x0, codec_size);

        if(length > 0)
        {
        	sprintf(*buffer_out,"^%10s/0/%03d/%s~",g_acSendTestID,cmd_type,buffer_in);
        }
        else
        {
        	sprintf(*buffer_out,"^%10s/0/%03d/~",g_acSendTestID,cmd_type);
        }
        strcpy(g_acSendTestID,AT_TO_PC_ID);

        size = codec_size;

    }while (FALSE);

    if(size < 0 && *buffer_out != NULL)
    {
        free((*buffer_out));
        *buffer_out = NULL;
    }
    return size;
}

BOOL ATSendEncodeMessage(AT_CMD_EN cmd, void*buffer, int size)
{
	if(0 == g_hConnectPC)
	{
		//printf("%d\n\n",errno);
		return FALSE;
	}

	BOOL status = FALSE;
	char* buf_out = NULL;
	int length = 0;
	length = ATCodecEncode(cmd, (const char*)buffer, size, &buf_out);
	if(0 == length)
	{
		return status;
	}

	if(ATSendMessage(g_hConnectPC, buf_out, length) > 0)
	{
		if(ATCDisplay)
		{
			LOG_RUNLOG_DEBUG("AT Send buffer Success : %s %d", (char*)buf_out,length);
		}
		status = TRUE;
	}
	else
	{
		if(ATCDisplay)
		{
			LOG_RUNLOG_DEBUG("ATSend buffer failed : %s , errno : %d", (char*)buf_out,errno);
		}
//		if(errno == EPIPE)
//		{
//			return status;
//		}
		ATDisConnect(g_hConnectPC);
		g_hConnectPC = 0;
		g_enRigisterState = AT_REGISTER_STATE_REJECT;
	}
	return status;
}

void ATProcessMessage(AT_MSG_DATA_ST* msg)
{
	if(NULL == msg || NULL == msg->pvBuffer)
	{
		return;
	}

	int timeout = 0;
	char tmp[128]={0x0};
	char build[64] = {0x0};
	char* buffer_out = NULL;
	AT_CMD_EN codec_status = AT_CODEC_ERROR;
	AT_CMD_EN cmd = ATCodecDecode((char*)msg->pvBuffer, msg->iSize, &buffer_out);

	char  operator_result = AT_CODEC_ERROR;
	char* encode_buffer = &operator_result;
	int   encode_size   = sizeof(operator_result);

	switch(cmd)
	{
	case AT_QUERY:
	case AT_ONEKEY_REQUEST:
	case AT_MONITOR_REQUEST:
	case AT_CALL_RESULT:
	case AT_LOG_RESULT:
		{
			encode_buffer = NULL;
			encode_size = 0;
		}
		break;
	case AT_REGISTER_REQUEST:
		{
			if(strcmp(buffer_out,"OK") == 0)
			{
				g_enRigisterState = AT_REGISTER_STATE_SUCCESS;
				LOG_RUNLOG_DEBUG("AT ==REGISTER_STATE_SUCCESS==");
			}
			else
			{
				g_enRigisterState = AT_REGISTER_STATE_WAIT;
				LOG_RUNLOG_DEBUG("AT ==REGISTER_STATE_FAIL==");
			}
		}
		break;
	case AT_CALL_TEST:
		{
			encode_buffer = buffer_out;
			encode_size = strlen(buffer_out);

			//进行呼叫测试
			if(g_stAutoTest.cTestStart != AT_TEST_STATE_SUCCESS)
			{
				g_stAutoTest.cTestStart = AT_TEST_STATE_SUCCESS;
				//g_stAutoTest.cLock = AT_TEST_STATE_SUCCESS;
				strcpy(g_stAutoTest.acTestID,g_acGetTestID);
				memcpy(build,buffer_out,4);
				build[4] = '\0';
				if(strcmp(build,"0000")==0)
				{
					sprintf(tmp,"call 0 %s",buffer_out);
				}
				else
				{
					sprintf(tmp,"call E %s",buffer_out);
				}
				//sprintf(tmp,"call 0 %s",buffer_out);
				strcpy(g_acSendTestID,g_acGetTestID);
				ATSendEncodeMessage(AT_CALL_TEST, AT_TO_PC_TEST_SUCCESS, AT_TO_PC_TEST_SUCCESS_SIZE);
				if(ATCDisplay)
				{
					LOG_RUNLOG_DEBUG("AT Command : %s",tmp);
				}
				//SMProcessCommand(tmp);
			}
			else
			{
				if(strcmp(g_stAutoTest.acTestID,g_acGetTestID) == 0 && strcmp(buffer_out,"STOP") == 0)
				{
					//strcpy(g_acSendTestID,g_acGetTestID);
					g_bTestStop = 1;
					//LPProcessCommand("terminate");
					//ATSendEncodeMessage(AT_CALL_TEST, AT_TO_PC_TEST_FAIL, AT_TO_PC_TEST_FAIL_SIZE);
				}
				else if(strcmp(g_stAutoTest.acTestID,g_acGetTestID) != 0 && strcmp(buffer_out,"STOP") != 0)
				{
					g_bTestStop = 2;
					//LPProcessCommand("terminate");
					strcpy(g_acSendTestID,g_acGetTestID);
					ATSendEncodeMessage(AT_CALL_TEST, AT_TO_PC_TEST_SUCCESS, AT_TO_PC_TEST_SUCCESS_SIZE);

					while(g_bTestStop)
					{
						YKSleep(1*1000);
						if(++timeout >= 10)
						{
							timeout = 0;
							break;
						}
						if(ATCDisplay)
						{
							LOG_RUNLOG_DEBUG("AT it's here");
						}
					}
					ATTestClean(&g_stAutoTest,sizeof(g_stAutoTest));
					g_stAutoTest.cTestStart = AT_TEST_STATE_SUCCESS;
					strcpy(g_stAutoTest.acTestID,g_acGetTestID);
					memcpy(build,buffer_out,4);
					build[4] = '\0';
					if(strcmp(build,"0000")==0)
					{
						sprintf(tmp,"call 0 %s",buffer_out);
					}
					else
					{
						sprintf(tmp,"call E %s",buffer_out);
					}
					//sprintf(tmp,"call 0 %s",buffer_out);
					if(ATCDisplay)
					{
						LOG_RUNLOG_DEBUG("AT Command : %s",tmp);
					}
					//SMProcessCommand(tmp);
				}
				else
				{
					strcpy(g_acSendTestID,g_acGetTestID);
					ATSendEncodeMessage(AT_CALL_TEST, AT_TO_PC_TEST_FAIL, AT_TO_PC_TEST_FAIL_SIZE);
				}
			}
		}
		break;
	case AT_LOG_UPDATE:
		{
			if(g_stLogUpdate.iTestStart == 0)
			{
				g_stLogUpdate.iTestStart = 1;
				if(buffer_out != NULL)
				{
					g_stLogUpdate.iLogFtpDefault = 1;
					sscanf(buffer_out,"%[^:]:%d:%[^:]:%s",g_stLogUpdate.acIP,&g_stLogUpdate.iPort,g_stLogUpdate.acUserName,g_stLogUpdate.acPasswd);
				}
				ATSendEncodeMessage(AT_LOG_UPDATE, AT_TO_PC_TEST_SUCCESS, AT_TO_PC_TEST_SUCCESS_SIZE);
				LOGUploadResPrepross();
				LOGUploadExec();
			}
			else
			{
				ATSendEncodeMessage(AT_LOG_UPDATE, AT_TO_PC_TEST_FAIL, AT_TO_PC_TEST_FAIL_SIZE);
			}
		}
		break;
	default:
		encode_buffer = "FAILED";
		encode_size = 6;
		break;
	}

	if(AT_CODEC_ERROR != cmd && AT_REGISTER_REQUEST != cmd && AT_ONEKEY_REQUEST != cmd
			&& AT_CALL_TEST != cmd && AT_CALL_RESULT != cmd && AT_MONITOR_REQUEST != cmd
			&& AT_LOG_UPDATE != cmd && AT_LOG_RESULT != cmd)
	{
#ifdef _YK_PC_
		LOG_RUNLOG_DEBUG("AT Recv Message Process : cmd = %03d", cmd);
#endif
		ATSendEncodeMessage(cmd, encode_buffer, encode_size);
	}
	else
	{
#ifdef _YK_PC_
		LOG_RUNLOG_DEBUG("AT Recv Message Not Process : cmd = %03d", cmd);
#endif
	}

	if(NULL != buffer_out)
	{
		free(buffer_out);
		buffer_out = NULL;
	}

	free(msg->pvBuffer);
	msg->pvBuffer = NULL;
}

BOOL ATReqRegister()
{
	char buf[256]={0x0};
	//TMGetIMSInfo(g_stAutoTestInfo.acSIP,NULL,NULL,0,NULL);
	//NMGetIpAddress(g_stAutoTestInfo.acIP);
	sprintf(buf,"%s:%s",g_stAutoTestInfo.acSIP,g_stAutoTestInfo.acIP);
	return ATSendEncodeMessage(AT_REGISTER_REQUEST, buf, strlen(buf));
}

void ATTestClean(void *pstAutoTest,int iLen)
{
	memset(pstAutoTest,AT_TEST_STATE_FAIL,iLen);
}

void ATTestOK(AT_TEST_OBJECT_EN penObject)
{
	if(g_stAutoTest.cTestStart != AT_TEST_STATE_SUCCESS)
	{
		return;
	}

	switch(penObject)
	{
	case AT_TEST_OBJECT_SM:
		g_stAutoTest.cSM = AT_TEST_STATE_SUCCESS;
		break;
	case AT_TEST_OBJECT_CC:
		g_stAutoTest.cCC = AT_TEST_STATE_SUCCESS;
			break;
	case AT_TEST_OBJECT_LP:
		g_stAutoTest.cLP = AT_TEST_STATE_SUCCESS;
			break;
	default:
		break;
	}
}

BOOL ATTestCheck(AT_TEST_ST *pstAutoTest)
{
	if(pstAutoTest->cSM == AT_TEST_STATE_SUCCESS
			&& pstAutoTest->cCC == AT_TEST_STATE_SUCCESS
			&& pstAutoTest->cLP == AT_TEST_STATE_SUCCESS)
	{
		return TRUE;
	}
	return FALSE;
}

void ATSendTestResult()
{
	if(g_stATGlobalInfo.iState != AT_STATE_RUNNING  && g_hConnectPC == 0)
	{
		return;
	}

	if(g_stAutoTest.cTestStart != AT_TEST_STATE_SUCCESS)
	{
		return;
	}

	strcpy(g_acSendTestID,g_stAutoTest.acTestID);

	if(g_bTestStop == 1)
	{
		ATSendEncodeMessage(AT_CALL_TEST, AT_TO_PC_TEST_SUCCESS, AT_TO_PC_TEST_SUCCESS_SIZE);
		g_bTestStop = 0;
	}
	else if(g_bTestStop == 0)
	{
		if(ATTestCheck(&g_stAutoTest) == TRUE)
		{
			ATSendEncodeMessage(AT_CALL_RESULT, AT_TO_PC_TEST_SUCCESS, AT_TO_PC_TEST_SUCCESS_SIZE);
		}
		else
		{
			ATSendEncodeMessage(AT_CALL_RESULT, AT_TO_PC_TEST_FAIL, AT_TO_PC_TEST_FAIL_SIZE);
		}
	}
	else
	{
		g_bTestStop = 0;
	}

	ATTestClean(&g_stAutoTest,sizeof(g_stAutoTest));
}

void ATSendOneKeyOpenResult(int iState,char *pcRoomNum,char *pcPhoneNum)
{
	if(g_stATGlobalInfo.iState != AT_STATE_RUNNING || g_hConnectPC == 0)
	{
		return;
	}

	if(iState)
	{
		ATSendEncodeMessage(AT_ONEKEY_REQUEST, AT_TO_PC_TEST_SUCCESS, AT_TO_PC_TEST_SUCCESS_SIZE);
	}
	else
	{
		ATSendEncodeMessage(AT_ONEKEY_REQUEST, AT_TO_PC_TEST_FAIL, AT_TO_PC_TEST_FAIL_SIZE);
	}
}

void ATSendMonitorResult(int iState,char *pcRoomNum,char *pcPhoneNum)
{
	if(g_stATGlobalInfo.iState != AT_STATE_RUNNING || g_hConnectPC == 0)
	{
		return;
	}

	if(iState)
	{
		ATSendEncodeMessage(AT_MONITOR_REQUEST, AT_TO_PC_TEST_SUCCESS, AT_TO_PC_TEST_SUCCESS_SIZE);
	}
	else
	{
		ATSendEncodeMessage(AT_MONITOR_REQUEST, AT_TO_PC_TEST_FAIL, AT_TO_PC_TEST_FAIL_SIZE);
	}
}

void ATSendLogResult()
{
	if(g_stATGlobalInfo.iState != AT_STATE_RUNNING || g_hConnectPC == 0)
	{
		return;
	}

	if(g_stLogUpdate.iTestStart == 0)
	{
		return;
	}

	if(g_stLogUpdate.iLogSuccess)
	{
		ATSendEncodeMessage(AT_LOG_RESULT, AT_TO_PC_TEST_SUCCESS, AT_TO_PC_TEST_SUCCESS_SIZE);
	}
	else
	{
		ATSendEncodeMessage(AT_LOG_RESULT, AT_TO_PC_TEST_FAIL, AT_TO_PC_TEST_FAIL_SIZE);
	}

	memset(&g_stLogUpdate,0,sizeof(g_stLogUpdate));
}

void ATQueryFtpInfo(char* pcHost, int*piPort, char* pcUserName, char* pcPassWord)
{
	strcpy(pcHost,g_stLogUpdate.acIP);
	strcpy(pcUserName,g_stLogUpdate.acUserName);
	strcpy(pcPassWord,g_stLogUpdate.acPasswd);
	*piPort = g_stLogUpdate.iPort;
}

int ATCheckLogDefault()
{
	return g_stLogUpdate.iLogFtpDefault;
}

int ATCheckLogTest()
{
	return g_stLogUpdate.iTestStart;
}

void  ATLogUpdateSuccess()
{
	g_stLogUpdate.iLogSuccess =1;
}

void ATRegister()
{
	static int i = 0;

	if(AT_REGISTER_STATE_WAIT == g_enRigisterState)
	{
		if(i++ >= 5)
		{
			g_enRigisterState = AT_REGISTER_STATE_REQUEST;
			i = 0;
		}
	}
	if(AT_REGISTER_STATE_REQUEST == g_enRigisterState)
	{
		if(ATReqRegister())
       	{
			g_enRigisterState = AT_REGISTER_STATE_WAIT;
		}
		else
		{
			//g_enRigisterState = AT_REGISTER_STATE_REJECT;
		}
	}
}

/**************************************************************
  @brief:       		ATEventHndl
  @Description:			自动测试线程
  @param[in]:			ctx---线程参数
**************************************************************/
void *ATEventHndl(void *ctx)
{
	int iErrCode = 0;
	int iConnectTimes = 0;
	int iCount = 20;
	int iTimeOut = 0;
	void* pvMsg = NULL;

//	TMGetIMSInfo(g_stAutoTestInfo.acSIP,NULL,NULL,0,NULL);
//	NMGetIpAddress(g_stAutoTestInfo.acIP);

	while(g_stATGlobalInfo.iState != AT_STATE_QUIT)
	{
		YKSleep(1*1000);

		if(g_stATGlobalInfo.iState == AT_STATE_IDLE)
		{
			if(g_hConnectPC != 0)
			{
				ATDisConnect(g_hConnectPC);
				g_hConnectPC = 0;
				g_enRigisterState = AT_REGISTER_STATE_REJECT;
			}
		}
		if(g_stATGlobalInfo.iState == AT_STATE_RUNNING)
		{
			//连接
			while(g_hConnectPC == 0)
			{
				if(iCount ++ < 3)
				{
					break;
				}
				iCount = 0;

				if(g_stATGlobalInfo.iReconnect == 0)
				{
					if(iConnectTimes ++ > AT_TRY_RECONNECT_TIMES)
					{
						g_stATGlobalInfo.iState = AT_STATE_IDLE;
						iConnectTimes = 0;
						ATSetConfig();
						break;
					}
				}
				g_hConnectPC = ATConnect(g_stATGlobalInfo.acHost,atoi(g_stATGlobalInfo.acPort),0);
				LOG_RUNLOG_DEBUG("AT connect_%d_errno_%d",g_hConnectPC,errno);
				if(g_hConnectPC > 0)
				{
					iConnectTimes = 0;
					g_enRigisterState = AT_REGISTER_STATE_REQUEST;
				}
			}

			ATRegister();

			if(AT_REGISTER_STATE_SUCCESS == g_enRigisterState)
			{
				if(iTimeOut ++ > AT_RECV_TIMEOUT)
				{
					ATDisConnect(g_hConnectPC);
					g_hConnectPC=0;
					g_enRigisterState = AT_REGISTER_STATE_REJECT;
				}
			}
			//处理
			iErrCode = YKReadQue(g_pstATMsgQ, &pvMsg, AT_QUE_READ_TIMEOUT);
			if ( 0 != iErrCode || NULL == pvMsg )
			{
				continue;
			}

			iTimeOut = 0;
			ATProcessMessage((AT_MSG_DATA_ST *)pvMsg);
		}
	}
}

/**************************************************************
  @brief:       		ATEventRecv
  @Description:			自动测试线程
  @param[in]:			ctx---线程参数
**************************************************************/
void *ATEventRecv(void *arg)
{
	RECV_DATA_CTX_ST ctx;
	memset(&ctx, 0x0, sizeof(RECV_DATA_CTX_ST));
	ctx.head = ctx.buf;
	ctx.len=0;
	ctx.state = BEGIN_FLAG_NOT_FOUND;

	while(g_stATGlobalInfo.iState != AT_STATE_QUIT)
	{
		while(g_stATGlobalInfo.iState == AT_STATE_IDLE || 0 == g_hConnectPC)
		{
			if(g_stATGlobalInfo.iState == AT_STATE_QUIT)
			{
				return NULL;
			}
			YKSleep(1*1000);
		}

		ATRecvProcess(&ctx);
	}

	return NULL;
}

/**************************************************************
  @brief:       		ATInit
  @Description:			模块进入
  @return:				RST_OK 成功  RST_ERR 失败
**************************************************************/
int ATInit(void)
{
	//清空网络信息
	memset(&g_stAutoTestInfo,0,sizeof(AT_INFO_ST));
	memset(&g_stATGlobalInfo,0,sizeof(AT_GLOBAL_ST));

	//清空测试状态
	ATTestClean(&g_stAutoTest,sizeof(g_stAutoTest));
	memset(&g_stLogUpdate,0,sizeof(g_stLogUpdate));

	//读取配置文件
	ATGetConfig();

	//创建消息队列
	g_pstATMsgQ = YKCreateQue(AT_QUE_LEN);
	if( NULL == g_pstATMsgQ )
	{
	    return FALSE;
	}

	//创建接收线程
	g_hAutoTestRecv = YKCreateThread(ATEventRecv,NULL);
	if(g_hAutoTestRecv == NULL)
	{
		return FALSE;
	}

	//创建处理线程
	g_hAutoTestHndl = YKCreateThread(ATEventHndl,NULL);
	if(g_hAutoTestHndl == NULL)
	{
		return FALSE;
	}

	return TRUE;
}

/**************************************************************
  @brief:       		ATFini
  @Description:			模块退出
**************************************************************/
void ATFini(void)
{
	g_stATGlobalInfo.iState = AT_STATE_QUIT;

	if(NULL != g_hAutoTestHndl)
	{
		YKJoinThread(g_hAutoTestHndl);
		YKDestroyThread(g_hAutoTestHndl);
	}
	if(NULL != g_hAutoTestRecv)
	{
		YKJoinThread(g_hAutoTestRecv);
		YKDestroyThread(g_hAutoTestRecv);
	}

	YKDeleteQue(g_pstATMsgQ);
}

/**************************************************************
 *@brief		ATProcessCommand 命令行支持.
 *@param[in]:	const char *cmd 命令
 *@return		RST_OK 成功  RST_ERR 失败
**************************************************************/
int ATProcessCommand(const char *cmd)
{
	char* argv[6];
	char* acIP[6];
	int iPort = 0;
	char temp[16];
	int i=0;
	int iChoice = -1;

	for(i=0;i<5;i++)
	{
		argv[i] = NULL;
	}

	argv[0] = strtok(cmd," ");
	for(i=0;i<5;i++)
	{
		if(argv[i] == NULL)
		{
			if(i == 0)
			{
				return RST_ERR;
			}
			break;
		}
		argv[i+1] = strtok(NULL," ");
	}
	if(strcmp(argv[0],"help") == 0)
	{
		iChoice = AT_CMD_LINE_HELP;
	}
	else if(strcmp(argv[0],"stop") == 0)
	{
		iChoice = AT_CMD_LINE_STOP;
	}
	else if(strcmp(argv[0], "test") == 0)
	{
		iChoice = AT_CMD_LINE_TEST;
	}
	else if(strcmp(argv[0], "state") == 0)
	{
		iChoice = AT_CMD_LINE_STATE;
	}
	else if(strcmp(argv[0], "display") == 0)
	{
		iChoice = AT_CMD_LINE_DISPLAY;
	}
	else if(strcmp(argv[0], "undisplay") == 0)
	{
		iChoice = AT_CMD_LINE_UNDISPLAY;
	}
	else
	{
		LOG_RUNLOG_DEBUG("AT ===================================================================\n");
		LOG_RUNLOG_DEBUG("AT input error command ,you can input \"ATC help\" to look for command\n");
		LOG_RUNLOG_DEBUG("AT ===================================================================\n");
	}

	switch(iChoice)
	{
	case AT_CMD_LINE_HELP:
		LOG_RUNLOG_DEBUG("AT ===================================================\n");
		LOG_RUNLOG_DEBUG("AT ---------------help----------------look for command\n");
		LOG_RUNLOG_DEBUG("AT ---------------stop-----------------------stop test\n");
		LOG_RUNLOG_DEBUG("AT ---------------test [IP] [PORT]----------start test\n");
		LOG_RUNLOG_DEBUG("AT ===================================================\n");
		break;

	case AT_CMD_LINE_STATE:
		LOG_RUNLOG_DEBUG("AT ===================================================\n");
		LOG_RUNLOG_DEBUG("AT =============socket %d=====================\n",g_hConnectPC);
		LOG_RUNLOG_DEBUG("AT =============state %d========================\n",g_stATGlobalInfo.iState);
		LOG_RUNLOG_DEBUG("AT =============IP %s======Port %s===========\n",g_stATGlobalInfo.acHost,g_stATGlobalInfo.acPort);
		LOG_RUNLOG_DEBUG("AT ===================================================\n");
		LOG_RUNLOG_DEBUG("AT ===================================================\n");
		break;

	case AT_CMD_LINE_TEST:
		do
		{
			if(argv[1] && argv[2])
			{
				strcpy(temp,argv[1]);
				acIP[0] = strtok(argv[1],".");
				iPort = atoi(argv[2]);
				for(i=0;i<4;i++)
				{
					if(acIP[i] == NULL)
					{
						break;
					}
					acIP[i+1] = strtok(NULL,".");
				}
				if(acIP[0]==NULL || acIP[1]==NULL || acIP[2]==NULL || acIP[3]==NULL
						|| strlen(acIP[0])==0 || strlen(acIP[1])==0 ||strlen(acIP[2])==0
						|| strlen(acIP[3])==0 || iPort > 0xFFFF || iPort < 0)
				{
					break;
				}
				strcpy(g_stATGlobalInfo.acHost,temp);
				strcpy(g_stATGlobalInfo.acPort,argv[2]);
				memset(&g_stAutoTest,0,sizeof(AT_TEST_ST));
				if(g_hConnectPC != 0)
				{
					ATDisConnect(g_hConnectPC);
					g_hConnectPC = 0;
					g_enRigisterState = AT_REGISTER_STATE_REJECT;
				}
				if(argv[3]!=NULL)
				{
					if(strcmp(argv[3],"1")==0)
					{
						g_stATGlobalInfo.iReconnect = 1;
					}
					if(strcmp(argv[3],"0")==0)
					{
						g_stATGlobalInfo.iReconnect = 0;
					}
				}
				g_stATGlobalInfo.iState = AT_STATE_RUNNING;
				ATSetConfig();

				LOG_RUNLOG_DEBUG("AT =================start auto test================\n");

				return RST_OK;
			}
		}while(0);

		LOG_RUNLOG_DEBUG("AT ===================================================");
		LOG_RUNLOG_DEBUG("AT ==============parameter is error===================");
		LOG_RUNLOG_DEBUG("AT ==============you can use like below===============");
		LOG_RUNLOG_DEBUG("AT ---------------test [IP] [PORT]----------start test");
		LOG_RUNLOG_DEBUG("AT ---------------test 196.196.196.145 5068-----------");
		LOG_RUNLOG_DEBUG("AT ---------------test [IP] [PORT] [FLAG]---allways reconnect");
		LOG_RUNLOG_DEBUG("AT ---------------test 196.196.196.145 5068 1---------");
		LOG_RUNLOG_DEBUG("AT ===================================================");

		//return RST_ERR;

		break;
	case AT_CMD_LINE_STOP:
		{
			g_stATGlobalInfo.iState = AT_STATE_IDLE;
			g_stATGlobalInfo.iReconnect = 0;
			//g_bAllwaysReconnect = 0;
			if(g_hConnectPC != 0)
			{
				ATDisConnect(g_hConnectPC);
				g_hConnectPC = 0;
				g_enRigisterState = AT_REGISTER_STATE_REJECT;
			}
			memset(&g_stAutoTest,0,sizeof(AT_TEST_ST));
			//memset(g_stATGlobalInfo.acHost,0,AT_ADD_IP_LEN);
			//memset(g_stATGlobalInfo.acPort,0,AT_PORT_LEN);
			ATSetConfig();

			LOG_RUNLOG_DEBUG("AT =================stop auto test================");
		}
		break;
	case AT_CMD_LINE_DISPLAY:
		{
			ATCDisplay = TRUE;
		}
		break;
	case AT_CMD_LINE_UNDISPLAY:
		{
			ATCDisplay = FALSE;
		}
		break;
	default:
		break;
	}

	return RST_OK;
}

#if 0
void main(void)
{
	TMXmlSetup();
	int iRst = RST_ERR;
	/* 网络模块初始化 */
	iRst = ATInit();
	if (RST_ERR == iRst) {
		//IDBT_CRIT("app main: failed to init network module.");
		goto EXIT;
	}
	while(1)
	{
		sleep(1);
	}
EXIT:
	ATFini();
	LOG_RUNLOG_DEBUG("AT %s : Exit NETWORK Module Successfully !", __FILE__);
	system("pause");
}
#endif
