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
//#include <AutoTest.h>
#include <IDBT.h>
#include <YKApi.h>
#include <YKMsgQue.h>
#include <LOGIntfLayer.h>
#include <sys/time.h>

//#define TM_TAKE_PIC
#ifndef TM_TAKE_PIC
void PICTimeOut()
{

}
#endif

#ifdef TM_TAKE_PIC

typedef enum
{
	PIC_REGISTER_OK				=	21,
	PIC_REGISTER_FAILED			= 	20,
	PIC_ACT						=   3,
	PIC_ACT_OK					=   31,
	PIC_ACT_FAILED				=   30,
	PIC_TRANS_OK				=   41,
	PIC_TRANS_FAILED			=   40,
	PIC_TRANS					=   4,
	PIC_TIME_OK					=   51,
	PIC_TIME_FAILED				=   50,
	PIC_CONNECT					=   999
}PIC_CMD_EN;

static YK_MSG_QUE_ST*  g_pstPICMsgQ = NULL;
static YKHandle	g_hConnectServer	= 0;
static YKHandle g_hPICHndl = NULL;	//自动测试线程句柄
static YKHandle g_hPICRecv = NULL;	//自动测试接收线程句柄
static int g_iPICState = AT_STATE_IDLE;
static char g_acSipAccount[64] ={0x0};
static char g_acDeviceID[64] ={0x0};
static struct timeval g_stStartTime;
static struct timeval g_stGetPicTime;
static char g_acSid[32] = {0x0};
struct timeval g_stGetActTime;
struct timeval g_stCallBeginTime;
struct timeval g_stFtpOkTime;
struct timeval g_stSocketCreateTime;
static char acHost[64] = {0x0};

static BOOL g_iCancelCall = FALSE;
extern YK_MSG_QUE_ST  *g_pstCCMsgQ;
extern int ATStringFindFirst(unsigned char* pvBuffer, int iSize, char c);
extern void ATSendQueue(YK_MSG_QUE_ST*q, void* pvBuffer, int iSize);
extern int ATSendMessage(YKHandle handle, void* buffer, int size);
extern void TMQueryDeviceID(char* pcID);
extern int ATRecv(YKHandle hHandle, void*pvBuffer, int iSize, int iTimeOut);

void PICCancelCall(void);

int PICTimeInterval(struct timeval stStartTime, struct timeval stEndTime)
{
	int iSec = 0;
	int iInterval = 0;

	iSec = stEndTime.tv_sec - stStartTime.tv_sec ;
	if(iSec > 3000000 || iSec < 0)
	{
		return 0 ;
	}
	iInterval = (iSec*1000000 + stEndTime.tv_usec -stStartTime.tv_usec)/1000;

	return iInterval;
}

void PICCallBegin()
{
	LOG_RUNLOG_DEBUG("AT CallBegin");
	gettimeofday(&g_stCallBeginTime, NULL);
	g_iCancelCall = FALSE;
}

void PICFtpOk()
{
	int iInterval = 0;
	char acBuf[256] = {0x0};
	if(0 == g_hConnectServer)
	{
		return;
	}

	LOG_RUNLOG_DEBUG("AT FtpOk");
	gettimeofday(&g_stFtpOkTime, NULL);
	iInterval = PICTimeInterval(g_stCallBeginTime,g_stFtpOkTime);
	sprintf(acBuf,"^05&%s&02&%d~\r\n",g_acSid,iInterval);
	PICSendMessage(acBuf,strlen(acBuf));
}

void PICTakePicture(char *pcSipAccount)
{
	time_t   now;         //实例化time_t结构
	struct   tm     *timenow;         //实例化tm结构指针
    char num[64] = {0};
    char *tmp = NULL;
    int port = 0;
	if(pcSipAccount == NULL)
	{
		return;
	}
	if(acHost[0] == 0x0)
	{
	    memset(acHost,0,64);
	    TMGetHost(acHost,&port);
	}

	now = time(NULL);
	timenow  = localtime(&now);
    strcpy(num, pcSipAccount);
    tmp = strstr(num, "@");
    if (tmp != NULL)
    {
        *tmp = 0;
    }
	//生成SID
	sprintf(g_acSid,"%4d%02d%02d%02d%02d%02d%05d",timenow->tm_year+1900,timenow->tm_mon+1,
			timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec,rand()%65536);

	memset(g_acSipAccount,0,64);
	strcpy(g_acSipAccount,num);

	g_iPICState = AT_STATE_RUNNING;
	gettimeofday(&g_stStartTime, NULL);

	//发送消息队列
	char *pstMsg = (char*)malloc(5);
	if(NULL == pstMsg)
	{
		return;
	}

	memset(pstMsg, 0x0, 5);
	strcpy(pstMsg,"^999~");
	ATSendQueue(g_pstPICMsgQ,pstMsg,5);
	g_iCancelCall = TRUE;
}

void PICTimeOut()
{
	LOG_RUNLOG_DEBUG("AT TimeOut");
	LOG_RUNLOG_DEBUG("AT PICCancelCall %d",g_iCancelCall);
	PICCancelCall();
	ATDisConnect(g_hConnectServer);
	g_hConnectServer = 0;
	g_iPICState = AT_STATE_IDLE;
}

/*
 * 个推时，梯口机取消呼叫，并通知平台，格式：^06&设备编号&呼叫的客户端号码&sid ~
 */
void PICCancelCall(void)
{
    char acBuf[256] = {0x0};
    if (g_iCancelCall)
    {
        sprintf(acBuf,"^06&%s&%s&%s~\r\n",g_acDeviceID,g_acSipAccount,g_acSid);
        PICSendMessage(acBuf,strlen(acBuf));
        //sleep(1);
    }
    g_iCancelCall = FALSE;
}
void PIC2CC(unsigned int uiPrmvType)
{
	SMCC_INDOOR_PICK_UP_ST *pstMsg = (SMCC_INDOOR_PICK_UP_ST*)malloc(sizeof(SMCC_INDOOR_PICK_UP_ST));
	if(NULL == pstMsg)
	{
		return;
	}

	memset(pstMsg, 0x0, sizeof(SMCC_INDOOR_PICK_UP_ST));

	pstMsg->uiPrmvType = uiPrmvType;
	YKWriteQue(g_pstCCMsgQ , pstMsg,  0);
}

void PICRecvProcess(RECV_DATA_CTX_ST* ctx)
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

//	recvDataLen = ATRecvMsg(g_hConnectServer, buffer, sizeof(buffer));
//	if( errno == EAGAIN || errno == EWOULDBLOCK)
//	{
//		return;
//	}
//	if(recvDataLen <= 0)
//	{
//		ATDisConnect(g_hConnectServer);
//		g_hConnectServer = 0;
//		g_iPICState = AT_STATE_IDLE;
//		return;
//	}

	recvDataLen = ATRecv(g_hConnectServer, buffer, sizeof(buffer), 500);
	switch(recvDataLen)
	{
	case 0:
		return;
	case -1:
		LOG_RUNLOG_DEBUG("AT connect_%d_errno_%d_recv_%d",g_hConnectServer,errno,recvDataLen);
		if( errno == EAGAIN)
		{
			return;
		}
		else
		{
			ATDisConnect(g_hConnectServer);
			g_hConnectServer = 0;
			g_iPICState = AT_STATE_IDLE;
			return;
		}
	}

	LOG_RUNLOG_DEBUG("AT Recv Message from socket: %s", buffer);

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
						ATSendQueue(g_pstPICMsgQ,ethData,beginIndex + 1);
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
						ATSendQueue(g_pstPICMsgQ,ethData,index + 1);
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

int PICSendMessage(void*buffer, int size)
{
	if(0 == g_hConnectServer)
	{
		return FALSE;
	}

	int status = FALSE;
	LOG_RUNLOG_DEBUG("AT Send Message : %s", buffer);
	if(ATSendMessage(g_hConnectServer, buffer, size) > 0)
	{
		status = TRUE;
	}
	else
	{
		ATDisConnect(g_hConnectServer);
		g_hConnectServer = 0;
		g_iPICState = AT_STATE_IDLE;
	}
	return status;
}

void PICProcessFailed(unsigned int uiPrmvType)
{
	PIC2CC(uiPrmvType);
	ATDisConnect(g_hConnectServer);
	g_hConnectServer = 0;
	g_iPICState = AT_STATE_IDLE;
}

void PICProcessMessage(AT_MSG_DATA_ST* msg)
{
	if(NULL == msg || NULL == msg->pvBuffer)
	{
		return;
	}
	char* buffer_out = NULL;
	int iCmd = 0,iState = 0;
	PIC_CMD_EN enCmd;
	char acBuf[256]= {0x0};
	char acSip[128] = {0x0};
	int iInterval = 0;
	unsigned int uiAllTime = 0;
	unsigned int uiPartTime = 0;
	unsigned int uiPicUpdateOKTime = 0;
	char acPicTimeWrite[128] = {0};
	if(msg->iSize <= 5)
	{
		sscanf((char*)msg->pvBuffer,"^%d~",&iCmd);
	}
	else if(msg->iSize > 5)
	{
		sscanf((char*)msg->pvBuffer,"^%d&%d&%s~",&iCmd,&iState,acSip);
	}

	enCmd = (AT_CMD_EN)iCmd;

	LOG_RUNLOG_DEBUG("AT Recv Message Process :msg = %s, cmd = %03d, iState = %d",(char*)msg->pvBuffer ,enCmd,iState);

	switch(enCmd)
	{
	case PIC_REGISTER_OK:
	case PIC_TRANS:
		break;
	case PIC_TRANS_OK:
		gettimeofday(&g_stGetPicTime, NULL);
		iInterval = PICTimeInterval(g_stSocketCreateTime,g_stGetPicTime);
		sprintf(acBuf,"^05&%s&01&%d~\r\n",g_acSid,iInterval);
		PICSendMessage(acBuf,strlen(acBuf));
		PIC2CC(TMCC_PIC_CLINET_RECV_PIC_OK);
		
		gettimeofday(&g_ClinetGetPicTime,NULL);
		uiPicUpdateOKTime = PICTimeInterval(g_CapturePictureStart, g_PicEndTime);
		uiPartTime = PICTimeInterval(g_PicEndTime, g_ClinetGetPicTime);
		uiAllTime = PICTimeInterval(g_CapturePictureStart, g_ClinetGetPicTime);
		sprintf(acPicTimeWrite, "echo second  %d   %d   %d   >> /home/YK-IDBT/config-idbt/time.log",
				uiPicUpdateOKTime, uiPartTime	,uiAllTime);
		printf("FTP **** second %s\n", acPicTimeWrite);
//		system(acPicTimeWrite);
		//LOG_RUNLOG_DEBUG("AT GetPic time :sec %d,msec %d",g_stGetPicTime.tv_sec -  g_stStartTime.tv_sec,g_stGetPicTime.tv_usec -  g_stStartTime.tv_usec);
		break;
	case PIC_CONNECT:
		gettimeofday(&g_stSocketCreateTime, NULL);		//记录创建socket时间
		g_hConnectServer = ATConnect(acHost,3309,0);
		//LOG_RUNLOG_DEBUG("AT connect_%d_errno_%d",g_hConnectServer,errno);
		if(g_hConnectServer != 0)
		{
			//register
			sprintf(acBuf,"^02&%s&%s&%s~\r\n",g_acDeviceID,g_acSipAccount,g_acSid);
			PICSendMessage(acBuf,strlen(acBuf));
		}
		else
		{
			//失败调用
			PICProcessFailed(TMCC_PIC_SOCKET_ERROR);
		}
		break;
	case PIC_ACT:
		if(g_hConnectServer == 0)
		{
			break;
		}
		gettimeofday(&g_stGetActTime, NULL);
		//LOG_RUNLOG_DEBUG("AT GetAct time :sec %d,msec %d",g_stGetActTime.tv_sec -  g_stStartTime.tv_sec,g_stGetActTime.tv_usec -  g_stStartTime.tv_usec);
		PICSendMessage("^031~\r\n",strlen("^031~\r\n"));
		switch(iState)
		{
		case 1:
			//接听
			PIC2CC(TMCC_PIC_RESULT_PICKUP);
			break;
		case 2:
			//开门
			PIC2CC(TMCC_PIC_RESULT_OPEN_DOOR);
			break;
		case 3:
			//挂断
			PIC2CC(TMCC_PIC_RESULT_HANG_OFF);
            // add by cs 2013-05-17
			ATDisConnect(g_hConnectServer);
			g_hConnectServer = 0;
			g_iPICState = AT_STATE_IDLE;
			break;
		default :
			break;
		}
		// del by cs 2013-05-17
		//PICSendMessage("^031~\r\n",strlen("^031~\r\n"));
		//ATDisConnect(g_hConnectServer);
		//g_hConnectServer = 0;
		//g_iPICState = AT_STATE_IDLE;
		break;
	case PIC_REGISTER_FAILED:
	case PIC_TRANS_FAILED:
		PICProcessFailed(TMCC_PIC_RESULT_ERROR);
		break;
	default:
		break;
	}

	if(NULL != buffer_out)
	{
		free(buffer_out);
		buffer_out = NULL;
	}

	free(msg->pvBuffer);
	msg->pvBuffer = NULL;
	free(msg);
	msg = NULL;
}

/**************************************************************
  @brief:       		PICEventHndl
  @Description:			自动测试线程
  @param[in]:			ctx---线程参数
**************************************************************/
void *PICEventHndl(void *ctx)
{
	int iErrCode = 0;
	int iCount = 20;
	void* pvMsg = NULL;
	char acBuf[256] = {0x0};

	while(g_iPICState!= AT_STATE_QUIT)
	{
//		YKSleep(1*100);

		//处理
		iErrCode = YKReadQue(g_pstPICMsgQ, &pvMsg, AT_QUE_READ_TIMEOUT);
		if ( 0 != iErrCode || NULL == pvMsg )
		{
			continue;
		}

		PICProcessMessage((AT_MSG_DATA_ST *)pvMsg);
	}
}

/**************************************************************
  @brief:       		PICEventRecv
  @Description:			自动测试线程
  @param[in]:			ctx---线程参数
**************************************************************/
void *PICEventRecv(void *arg)
{
	RECV_DATA_CTX_ST ctx;
	memset(&ctx, 0x0, sizeof(RECV_DATA_CTX_ST));
	ctx.head = ctx.buf;
	ctx.len=0;
	ctx.state = BEGIN_FLAG_NOT_FOUND;

	while(g_iPICState!= AT_STATE_QUIT)
	{
		while(g_iPICState== AT_STATE_IDLE || 0 == g_hConnectServer)
		{
			if(g_iPICState== AT_STATE_QUIT)
			{
				return NULL;
			}
			YKSleep(1*100);
		}
//		YKSleep(1*100);		//add by zwx,2013-1-29。防止一直占住线程
		PICRecvProcess(&ctx);
	}

	return NULL;
}

/**************************************************************
  @brief:       		PICInit
  @Description:			模块进入
  @return:				RST_OK 成功  RST_ERR 失败
**************************************************************/
int PICInit(void)
{
	//获取设备ID
	TMQueryDeviceID(g_acDeviceID);

	//创建消息队列
	g_pstPICMsgQ = YKCreateQue(AT_QUE_LEN);
	if( NULL == g_pstPICMsgQ )
	{
	    return FALSE;
	}

	//创建接收线程
	g_hPICRecv = YKCreateThread(PICEventRecv,NULL);
	if(g_hPICRecv == NULL)
	{
		return FALSE;
	}

	//创建处理线程
	g_hPICHndl = YKCreateThread(PICEventHndl,NULL);
	if(g_hPICHndl == NULL)
	{
		return FALSE;
	}

	return TRUE;
}

/**************************************************************
  @brief:       		PICFini
  @Description:			模块退出
**************************************************************/
void PICFini(void)
{
	g_iPICState= AT_STATE_QUIT;

	if(NULL != g_hPICHndl)
	{
		YKJoinThread(g_hPICHndl);
		YKDestroyThread(g_hPICHndl);
	}
	if(NULL != g_hPICRecv)
	{
		YKJoinThread(g_hPICRecv);
		YKDestroyThread(g_hPICRecv);
	}

	YKDeleteQue(g_pstPICMsgQ);
}

#if 0
void main(void)
{
	int iRst = RST_ERR;
	/* 网络模块初始化 */
	iRst = PICInit();
	if (RST_ERR == iRst) {
		//IDBT_CRIT("app main: failed to init network module.");
		goto EXIT;
	}
	while(1)
	{
		sleep(1);
	}
EXIT:
	PICFini();
	system("pause");
}
#endif

#endif

