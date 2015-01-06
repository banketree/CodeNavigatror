/***************************************************************
  Copyright (C), 1988-2012, YouKe Tech. Co., Ltd.
  @file:
  @author:                  cs
  @date:	                    2012.10.16
  @description:     
  @version:          
  @history:
                <author>            <time>          <version >          <desc>
                    hb	   	        2012/9/11     1.0                	创建模块
***************************************************************/
#include <sys/ioctl.h>
#include <net/if.h>
#include "../INC/TestSoft.h"
#include <IDBT.h>
#include <YKApi.h>
#include <YKMsgQue.h>
#include <NetworkMonitor.h>

#define TS_FLASH_BLOCK_SIZE	 1024
#define TS_FLASH_RESERVE_BLOCK 100
#define TS_LINK_PC_CONDITION "192.168.69.20"
#define TS_HOST_IP           "192.168.69.10"
#define TS_CONNECT_PORT      "5068"

int                     g_iTSRunFlag = 0;
static YK_MSG_QUE_ST*   g_pstTSMsgQ = NULL;
static YKHandle         g_hTSProc = NULL;	        // 测试软件线程句柄
static YKHandle         g_hTSRecv = NULL;	        // 测试软件接收线程句柄
static YKHandle         g_hConnectPC;
static TS_INFO_ST       g_stTSGlobalInfo;
static TS_FILE_INFO_ST  g_stFileInfo;

#define _LINE_LENGTH        300
int TSGetDiskAvailable(const char* path)
{
	char line[_LINE_LENGTH];
	char temp[64] = {0};
	char* token;
	int available = 0;
	FILE* fp;

	sprintf(temp, "df %s", path);
	fp = popen(temp, "r");
	if (fp != NULL)
	{
		if (fgets(line, _LINE_LENGTH, fp) != NULL)
		{
			if (fgets(line, _LINE_LENGTH, fp) != NULL)
			{
				token = strtok(line, " ");
				if (token != NULL)
				{
					//printf("Filesystem = %s\n", token);
				}
				token = strtok(NULL, " ");
				if (token != NULL)
				{
					//printf("1K-blocks =%s\n", token);
				}
				token = strtok(NULL, " ");
				if (token != NULL)
				{
					//printf("Used = %s\n", token);
				}
				token = strtok(NULL, " ");
				if (token != NULL)
				{
					available = (int)atoi(token);
					printf("Available = %s\n", token);
				}
			}
		}
		pclose(fp);
	}

	return available;
}


size_t TSGetFileSize(const char* fileName)
{
	FILE * fp;
	size_t ret = 0;

	fp = fopen(fileName, "rb");
	if (fp == NULL)
	{
		return ret;
	}

	fseek(fp, 0, SEEK_END);
	ret = ftell(fp);
	fseek(fp,0,SEEK_SET);

	fclose(fp);

	return ret;
}


int TSGetMac(char* mac)
{
	struct ifreq tmp;
	int sockMac;
	char macAddr[30];

	sockMac = socket(AF_INET, SOCK_STREAM, 0);
	if (sockMac == -1)
	{
		printf("TSGetMac : creat socket fail\n");
		return FALSE;
	}

	memset(&tmp, 0, sizeof(tmp));
	strncpy(tmp.ifr_name, "eth0", sizeof(tmp.ifr_name)-1);

	if ((ioctl(sockMac, SIOCGIFHWADDR, &tmp)) < 0)
	{
		printf("TSGetMac : mac ioctl error\n");
		return FALSE;
	}

	sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", \
			(unsigned char)tmp.ifr_hwaddr.sa_data[0], \
			(unsigned char)tmp.ifr_hwaddr.sa_data[1], \
			(unsigned char)tmp.ifr_hwaddr.sa_data[2], \
			(unsigned char)tmp.ifr_hwaddr.sa_data[3], \
			(unsigned char)tmp.ifr_hwaddr.sa_data[4], \
			(unsigned char)tmp.ifr_hwaddr.sa_data[5]  \
			) ;
	printf("TSGetMac : MAC:%s\n", macAddr);
	close(sockMac);
	memcpy(mac, macAddr, strlen(macAddr));

	return TRUE;
}

void TSSendQueue(YK_MSG_QUE_ST*q, void* pvBuffer, int iSize)
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


int TSGetTestCondition(void)
{
	unsigned int slen;
	int ret;
#if 1
    NMGetIpAddress(g_stTSGlobalInfo.acIP);
    //printf("local IP : %s\n", g_stTSGlobalInfo.acIP);
    slen = strlen(TS_LINK_PC_CONDITION);
    ret = strncmp(g_stTSGlobalInfo.acIP, TS_LINK_PC_CONDITION, slen);
    if (ret == 0)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
#endif
    return TRUE;
}


void  TSDisConnect(YKHandle hHandle)
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


unsigned long TSGetHostAddress(const char* pcHost)
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


int TSCopyFile2Flash(const char* src, const char* dst, const char* path)
{
	char cmd[128] = {0};
	int ret = 0;
#ifndef _YK_IMX27_AV_
	int available = 0;
	int newSize = 0;
	int oldSize = 0;


    printf("src path:%s, dst path:%s\n", src, dst);

    if ((src == NULL) || (dst == NULL))             // add by cs 2012-11-13
    {
        return FALSE;
    }

	// check disk space
	available = TSGetDiskAvailable(path);
	newSize = TSGetFileSize(src);
	newSize /= TS_FLASH_BLOCK_SIZE;
	oldSize = TSGetFileSize(dst);
	oldSize /= TS_FLASH_BLOCK_SIZE;

	printf ("available = %d , newSize = %d, oldSize = %d\n",
			available, newSize, oldSize);

	do
	{
		if (oldSize > newSize)
		{
			break;
		}
		if (available < ((newSize - oldSize) + TS_FLASH_RESERVE_BLOCK))
		{
			printf("TSCopyFile2Flash : disk full\n");
			return FALSE;
		}

	}while(FALSE);

#endif

    sprintf(cmd, "rm %s", dst);                 // add by cs 2012-11-13
	system(cmd);                                // 删除目标文件
	
	sprintf(cmd, "mv %s %s", src, dst);
	ret = system(cmd);                          // 移动到目标路径
	if (ret != 0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}


YKHandle  TSConnect(const char* pcHost, int iPort, unsigned int uiProto)
{
    BOOL ret = FALSE;
    YKHandle hHandle = 0;
    struct sockaddr_in clientAddr;
    if((hHandle = (YKHandle)socket(AF_INET, SOCK_STREAM, uiProto)) < 0)
    {
    	return NULL;
    }
    memset(&clientAddr, 0x0, sizeof(clientAddr));
    clientAddr.sin_addr.s_addr = TSGetHostAddress(pcHost);
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons( (unsigned short)iPort );
    if (connect( (int)hHandle, (struct sockaddr*) &clientAddr, sizeof(clientAddr) ) < 0)
    {
        TSDisConnect(hHandle);
        hHandle=0;
    }
    return hHandle;
}

#if 0
int TSCodecEncode(int cmd, int type, const char* buffer_in, int length, char** buffer_out)
{
    int size = -1;
    int codec_size;
    //char tmp[256]={0x0};
    if(cmd < 0)
    {
        return size;// cmd type error
    }
    *buffer_out = NULL;

    do
    {
        if (cmd == 0)
        {
            codec_size = length;
            
            *buffer_out = (char*)malloc(codec_size);
            if(NULL == *buffer_out)
            {
                 break;
            }
            memset(*buffer_out, 0x0, codec_size);
            memcpy(*buffer_out, buffer_in, length);
        }
        else
        {
            codec_size = length + 6;
            
            *buffer_out = (char*)malloc(codec_size);
            if(NULL == *buffer_out)
            {
                 break;
            }
            memset(*buffer_out, 0x0, codec_size);
            sprintf(*buffer_out,"^%d/%d/%s~", cmd, type, buffer_in);
        }

        size = codec_size;

    }while (FALSE);

    if(size < 0 && *buffer_out != NULL)
    {
        free((*buffer_out));
        *buffer_out = NULL;
    }
    return size;
}
#else
int TSCodecEncode(int cmd, int type, const char* buffer_in, int length, char* buffer_out)
{
    int size = -1;
    int codec_size;

    if(cmd < 0)
    {
        return size;// cmd type error
    }
    if (length >1024)
    {
    	return size;	// data len error
    }

    do
    {
        if (cmd == 0)
        {
            codec_size = length;
            memcpy(buffer_out, buffer_in, length);
        }
        else
        {
            codec_size = length + 6;
            sprintf(buffer_out,"^%d/%d/%s~", cmd, type, buffer_in);
        }

        size = codec_size;

    }while (FALSE);

    return size;
}
#endif

int TSCodecDecode(const char* buffer_in, int length, TS_MSG_INFO_ST* buffer_out)
{
    char cmd[4] = {0};
    char type[4] = {0};

    if (NULL == buffer_in)
    {
        printf("TSCodecDecode : buffer in is NULL\n");
        return FALSE;
    }

    do 
    {
        if ((buffer_in[0] == TS_CODEC_START_FLAG) && \
            (buffer_in[length - 1] == TS_CODEC_END_FLAG) && (length < 100))
        {
            //printf("TSCodecDecode : cmd pack\n");
            if (length < 7)
            {
                printf("TSCodecDecode : cmd pake len = %d error\n", length);
                return FALSE;
            }
            //sscanf(buffer_in,"^%c/%c/%[^~]", cmd, type, buffer_out->acData);
            cmd[0] = buffer_in[1];
            type[0] = buffer_in[3];
            memset(buffer_out->acData, 0, 1024);
            memcpy(buffer_out->acData, &buffer_in[5], length-6);
            //printf("TSCodecDecode: char cmd=%s - type=%s\n", cmd, type);

            buffer_out->iCmd = (int)atoi(cmd);
            buffer_out->iType = (int)atoi(type);
            //printf("TSCodecDecode : CMD = %d - TYPE = %d - FILE : %s\n", \
            //    buffer_out->iCmd, buffer_out->iType, buffer_out->acData);
        }
        else
        {
            //printf("TSCodecDecode : data pack\n");
            memcpy(buffer_out->acData, buffer_in, length);
            buffer_out->iCmd = 0;
            buffer_out->iType = 0;
        }
        
    } while (FALSE);

    return TRUE;
}


int TSSendMessage(YKHandle handle, void* buffer, int size)
{
	if(NULL != handle)
	{
		return send((int)handle, (char*)buffer, size, 0);
	}
	return 0;
}


BOOL TSSendEncodeMessage(int cmd, int type, void*buffer, int size)
{
	BOOL status = FALSE;
	//char* buf_out = NULL;
	int length = 0;
	char bufOut[1024];

	if(0 == g_hConnectPC)
	{
		//printf("%d\n\n",errno);
		return FALSE;
	}
	memset(bufOut, 0, sizeof(bufOut));
	length = TSCodecEncode(cmd, type, (const char*)buffer, size, bufOut);
	if(0 == length)
	{
		return status;
	}
	//if (cmd != 0)
	//{
	//	printf("TSSendEncodeMessage : pack:%s\n", bufOut);
	//}

	if(TSSendMessage(g_hConnectPC, bufOut, length) > 0)
	{
		status = TRUE;
	}
	else
	{
		g_stTSGlobalInfo.enState = TS_STATE_IDLE;
	}
/*
    if(buf_out != NULL)
    {
        free(buf_out);
        buf_out = NULL;
    }
*/
	return status;
}


BOOL TSSendAckOK(int cmd, int type)
{
    char temp[12] = "OK";
    return TSSendEncodeMessage(cmd, type, temp, strlen(temp));
}

BOOL TSSendAckFail(int cmd, int type)
{
    char temp[12] = "FAILED";
    return TSSendEncodeMessage(cmd, type, (void*)temp, strlen(temp));
}

void TSProcessMessage(TS_MSG_DATA_ST* msg)
{
    if(NULL == msg || NULL == msg->pvBuffer)
	{
		return;
	}

    TS_MSG_INFO_ST out;
    int ret;
    size_t rsize;
    TS_FILE_INFO_ST * info = &g_stFileInfo;
    char temp1[128] = {0};
    char temp[1024] = {0};

    ret = TSCodecDecode((char*)msg->pvBuffer, msg->iSize, &out);
    if (ret == FALSE)
    {
        out.iCmd = -1;
    }

    switch (out.iCmd)
    {
        case TS_FILE_DATA:
            if ((info->iFlag == 1) && (info->fd != NULL))
            {
                rsize = fwrite(out.acData, 1, msg->iSize, info->fd);
                if (msg->iSize != rsize)
                {
                	printf("write faile\n");
                }
                if (rsize != 1024)
                {
                	//printf("over??\n");
                }
                info->uiSize += rsize;
                //printf("rec size %d\n", info->uiSize);
            }
            else
            {
            	printf("error data pack\n");
            }
            break;
            
        case TS_READ_DATAREQUEST:
            printf("pc req read file\n");
            
            if (info->iFlag == 0)
            {
                info->iType = out.iType;
                memset(info->acFileName, 0, sizeof(info->acFileName));
                memset(info->acFilePath, 0, sizeof(info->acFilePath));

                strcpy(info->acFileName, out.acData);
                if (out.iType == TS_FILE_TYPE_APP)
                {
                    sprintf(info->acFilePath, "%s%s", TS_APP_FILE_PATH, info->acFileName);
                }
                else
                {
                    sprintf(info->acFilePath, "%s%s", TS_CONFIG_FILE_PATH, info->acFileName);
                }                

                info->fd = fopen(info->acFilePath,"rb");
                if (info->fd == NULL)
                {
                	printf("fopen fail\n");
                    TSSendAckFail(out.iCmd, out.iType);
                	break;
                }
                else
                {
                    TSSendAckOK(out.iCmd, out.iType);
                }

                YKSleep(2000);
                ret = 0;	// debug
                while (1)
                {
                	memset(temp, 0, 1024);
					rsize = fread(temp, 1, 1024, info->fd);
					if (rsize > 0)
                    {
                    	ret += rsize;
						TSSendEncodeMessage(0, 0, temp, rsize);
						//printf("read len = %d\n", ret);
						YKSleep(10);
                    }
                  else
                    {
						fclose(info->fd);
						YKSleep(2000);

						info->uiSize = TSGetFileSize(info->acFilePath);
						sprintf(temp, "%d", info->uiSize);
						TSSendEncodeMessage(TS_READ_WRITE_OK, info->iType, temp, strlen(temp));
						memset(info, 0, sizeof(TS_FILE_INFO_ST));
						break;
                    }
                }
            }
            break;

        case TS_WRITE_DATAREQUEST:
        	printf("pc req write file\n");
        	  if (info->iFlag)				// 上一次写文件没结束
              {
        		  printf("TS_WRITE_DATAREQUEST : stop previous\n");
					if (info->fd != NULL)
					{
						fclose(info->fd);
					}
					sprintf(temp, "%s%s", TS_TEMP_FILE_PATH, info->acFileName);  // 临时文件路径
					sprintf(temp1, "rm %s", temp);
					system(temp1);							// del 临时文件
              }

            info->iType = out.iType;
            memset(info->acFileName, 0, sizeof(info->acFileName));
            memset(info->acFilePath, 0, sizeof(info->acFilePath));

            strcpy(info->acFileName, out.acData);
            if (out.iType == TS_FILE_TYPE_APP)
            {
                sprintf(info->acFilePath, "%s%s", TS_APP_FILE_PATH, info->acFileName);
            }
            else
            {
                sprintf(info->acFilePath, "%s%s", TS_CONFIG_FILE_PATH, info->acFileName);
            }

            sprintf(temp1, "%s%s", TS_TEMP_FILE_PATH, info->acFileName);

            info->fd = fopen(temp1,"wb");
            if (info->fd == NULL)
            {
            	printf("fopen fail\n");
                TSSendAckFail(out.iCmd, out.iType);
            	break;
            }
            else
            {
                TSSendAckOK(out.iCmd, out.iType);
            }
            info->iFlag = 1;
            info->uiSize = 0;
            
            break;

        case TS_READ_WRITE_OK:                      // 下载文件数据发送完发送传输长度
			if (info->fd != NULL)
			{
				fclose(info->fd);
			}

			sprintf(temp, "%s%s", TS_TEMP_FILE_PATH, info->acFileName);  // 临时文件路径

			if (info->iType == TS_FILE_TYPE_APP)
			{
				strcpy(temp1, TS_APP_FILE_PATH);
			}
			else if (info->iType == TS_FILE_TYPE_CONFIG)
			{
				strcpy(temp1, TS_CONFIG_FILE_PATH);
			}

			if (atoi(out.acData) == info->uiSize)
			{
				ret = TSCopyFile2Flash(temp, info->acFilePath, temp1);
				if (ret == FALSE)
				{
					printf("move file err !!!\n");
					TSSendAckFail(out.iCmd, out.iType);
					sprintf(temp1, "rm %s", temp);
					system(temp1);							// del 临时文件
				}
				else
				{
					TSSendAckOK(out.iCmd, out.iType);
				}
			}
			else
			{
				printf("file size error !!!\n");
				TSSendAckFail(out.iCmd, out.iType);
				sprintf(temp1, "rm %s", temp);
				system(temp1);							// del 临时文件
			}
			memset(info, 0, sizeof(TS_FILE_INFO_ST));

			break;

        case TS_GET_MAC:
        	 ret = TSGetMac(temp);
        	 if (ret == TRUE)
        	 {
        		 TSSendEncodeMessage(TS_GET_MAC, out.iType, temp, strlen(temp));
        	 }
        	 else
        	 {
        		 TSSendAckFail(out.iCmd, out.iType);
        	 }

        	 break;

        case TS_DEVICE_REBOOT:
        	 TSSendAckOK(out.iCmd, out.iType);
        	 YKSleep(2000);
        	 LOGUploadResPrepross();
        	 system("reboot");

        	 break;

        default:
        	 printf("TSProcessMessage : err cmd !!!!!");
        	 break;
    }

    free(msg->pvBuffer);
	msg->pvBuffer = NULL;
    
}


void *TSProcTask(void *ctx)
{
    int condition = 0;
    int iErrCode = 0;
	void* pvMsg = NULL;
    int iCount = 20;

    //printf("TSProcTask : begin\n");

    TSGetDiskAvailable(TS_APP_FILE_PATH);

    while(g_stTSGlobalInfo.enState != TS_STATE_QUIT)
	{
        if (g_stTSGlobalInfo.enState == TS_STATE_IDLE)
        {
            YKSleep(2*1000);
            g_iTSRunFlag = 0;
            
            if (g_hConnectPC)
            {
                // disconnect
            	printf("TSProcTask : disconnect\n");
                TSDisConnect(g_hConnectPC);
                g_hConnectPC = 0;
            }

            condition = TSGetTestCondition();       // 是否符合测试条件
            if (condition)
            {
                // 连接电脑
                while(g_hConnectPC == 0)
                {
                    if(iCount ++ < 3)
                    {
                        break;
                    }
                    iCount = 0;
                    //printf("TSProcTask : try connect\n");
                    g_hConnectPC = TSConnect(g_stTSGlobalInfo.acHost,atoi(g_stTSGlobalInfo.acPort),0);
                    if(g_hConnectPC > 0)
                    {
                    	printf("TSProcTask : connected\n");
                       g_stTSGlobalInfo.enState = TS_STATE_RUNNING;
                    }
                }
            }
        }

        if (g_stTSGlobalInfo.enState == TS_STATE_RUNNING)
        {
            g_iTSRunFlag = 1;
        }

        iErrCode = YKReadQue(g_pstTSMsgQ, &pvMsg, TS_QUE_READ_TIMEOUT);
		if ( 0 != iErrCode || NULL == pvMsg )
		{
			continue;
		}

		TSProcessMessage((TS_MSG_DATA_ST *)pvMsg);
    }

    return NULL;
}


int  TSSelect(void* handle, unsigned long timeout)
{
    fd_set set;
    struct timeval timeval;

    timeval.tv_sec = timeout/1000;
    timeval.tv_usec = 0;
    FD_ZERO(&set);
    FD_SET((int)handle, &set);
    return select((int)handle+1, &set, NULL, NULL, &timeval);//0 time out, -1 failed, other success
}


int TSRecv(YKHandle hHandle, void*pvBuffer, int iSize, int iTimeOut)
{
    switch(TSSelect(hHandle, iTimeOut))
    {
    case 0://time out
        return 0;
    case -1://failed
        return -1;
    default:
        return recv((int)hHandle, (char*)pvBuffer, iSize, 0);
    }
}


void TSRecvProcess(TS_RECV_DATA_ST* ctx)
{
    int recvLen = 0;
    char* buffer = ctx->buf;
    char *ethData = NULL;
    //size_t rsize;
    //TS_FILE_INFO_ST * info = &g_stFileInfo;
    
    recvLen = TSRecv(g_hConnectPC, buffer, 1024, 2000);
    if (recvLen < 0)
    {
        // recv error
        if( errno == EAGAIN)
		{
            printf("TSRecvProcess : recv err 1\n");
			return;
		}
		else
		{
            printf("TSRecvProcess : recv err 2\n");
            if (g_hConnectPC)
             {
                // disconnect
                printf("TSRecvProcess : disconnect\n");
                TSDisConnect(g_hConnectPC);
                g_hConnectPC = 0;
             }
			g_stTSGlobalInfo.enState = TS_STATE_IDLE;
			return;
		}
    }
    else if (recvLen == 0)
    {
        //printf("TSRecvProcess : recv time out\n");
        return;
    }

    if (recvLen < 20)
    {
        printf("TSRecvProcess : len=%d, DATA:%s\n", recvLen, buffer);
    }
#if 0
    	if ((buffer[0] == TS_CODEC_START_FLAG) && \
    	 (buffer[recvLen - 1] == TS_CODEC_END_FLAG))
    	{
    		if (recvLen > 20)
    		{
    			printf("error pack\n");
    		}
			ethData = (char *)malloc(recvLen);
			if (ethData != NULL)
			{
				memcpy(ethData, buffer, recvLen);
				TSSendQueue(g_pstTSMsgQ, ethData, recvLen);
				//printf("TSRecvProcess : send data to queue\n");

			}
    	}
    	else
    	{
    		if ((info->iFlag == 1) && (info->fd != NULL))
			{
    			rsize = recvLen;
				//rsize = fwrite(buffer, 1, recvLen, info->fd);
				if (recvLen != rsize)
				{
					printf("write faile\n");
				}
				if (rsize != 1024)
				{
					printf("over??\n");
				}
				info->uiSize += rsize;
				//printf("rec size %d\n", info->uiSize);
			}
			else
			{
				printf("error data pack\n");
			}
    	}
#endif
    	ethData = (char *)malloc(recvLen);
		if (ethData != NULL)
		{
			memcpy(ethData, buffer, recvLen);
			TSSendQueue(g_pstTSMsgQ, ethData, recvLen);
			//printf("TSRecvProcess : send data to queue\n");

		}

    	memset(buffer, 0, 1024);
    return;
}


void *TSRecvTask(void *arg)
{
    TS_RECV_DATA_ST ctx;
	memset(&ctx, 0x0, sizeof(TS_RECV_DATA_ST));
	ctx.head = ctx.buf;
	ctx.len=0;
	ctx.state = 0;

	while (g_stTSGlobalInfo.enState != TS_STATE_QUIT)
	{
		while ((g_stTSGlobalInfo.enState == TS_STATE_IDLE) || (0 == g_hConnectPC))
		{
			YKSleep(1*1000);
		}

		TSRecvProcess(&ctx);
	}

	return NULL;
}


/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
int TSInit(void)
{
	printf("TSInit : begin\n");
    /* 全局变量初始化 */
    g_hConnectPC = 0;
    memset(&g_stFileInfo, 0, sizeof(TS_FILE_INFO_ST));
    memset(&g_stTSGlobalInfo, 0, sizeof(TS_INFO_ST));

    strcpy(g_stTSGlobalInfo.acHost, TS_HOST_IP);
    strcpy(g_stTSGlobalInfo.acPort, TS_CONNECT_PORT);

    g_stTSGlobalInfo.enState = TS_STATE_IDLE;

    /* 创建接收数据队列 */
    g_pstTSMsgQ = YKCreateQue(TS_QUE_LEN);
    if (NULL == g_pstTSMsgQ)
    {
        return FALSE;
    }

    /* 创建数据接收线程 */
    g_hTSRecv = YKCreateThread(TSRecvTask,NULL);
    if(g_hTSRecv == NULL)
    {
        return FALSE;
    }

    /* 创建数据处理线程 */
    g_hTSProc = YKCreateThread(TSProcTask,NULL);
    if(g_hTSProc == NULL)
    {
        return FALSE;
    }
    printf("TSInit : OK\n");
    return TRUE;
}


/********************************************************************
*Function	:
*Input		:
*Output		:
*Description:
*Others		:
*********************************************************************/
void TSFini(void)
{
    if(NULL != g_hTSProc)
    {
        YKJoinThread(g_hTSProc);
        YKDestroyThread(g_hTSProc);
    }

    if(NULL != g_hTSRecv)
    {
        YKJoinThread(g_hTSRecv);
        YKDestroyThread(g_hTSRecv);
    }

    YKDeleteQue(g_pstTSMsgQ);
}


