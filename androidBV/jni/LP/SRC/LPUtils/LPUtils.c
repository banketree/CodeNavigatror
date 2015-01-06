/**
*@file   LPUtils.c
*@brief 工具
*
*
*@author chensq
*@version 1.0
*@date 2012-5-17
*/

//#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <assert.h>
#include "LPUtils.h"
#include "LPMsgOp.h"
#include "LOGIntfLayer.h"
#include "LPIntfLayer.h"
#include "IDBTCfg.h"
/**
 * @brief        重建文件
 * @param[in]    pcFileName 文件名
 * @return       NULL代表创建失败，否则返回的是FILE*文件指针
 */
char* LPUtilsReCreateFile(const char* pcFileName)
{
	if (NULL == pcFileName)
	{
		LOG_RUNLOG_DEBUG("LP input param error");
		return NULL;
	}

	remove(pcFileName);
	FILE* p = fopen(pcFileName, "wb");
	if (NULL != p)
	{
		LOG_RUNLOG_DEBUG("LP create linphone config success(%s)", pcFileName);
		fclose(p);
	}
	return p;
}

//typedef struct
//{
// char chRIFF[4];
// unsigned int dwRIFFLen;
// char chWAVE[4];
//
// char chFMT[4];
// unsigned int dwFMTLen;
// unsigned short int wFormatTag;
// unsigned short int nChannels;
// unsigned int nSamplesPerSec;
// unsigned int nAvgBytesPerSec;
// unsigned short int nBlockAlign;
// unsigned short int wBitsPerSample;
//
// char chFACT[4];
// unsigned int dwFACTLen;
//
// char chDATA[4];
// unsigned int dwDATALen;
//}WAVE_FILE_HEADER_ST;
//
//
//
//int  LPUtilsLoadWav(char *pcFileName,  char *pcBuff, int pcBuffSize)
//{
//	int iRet = -1;
//
//	WAVE_FILE_HEADER_ST stWaveFileHeader;
//	FILE* pstWavFile;
//	int iReadBytes;
//
//	if(pcFileName == NULL || pcBuff == NULL)
//	{
//		printf("LPUtilsLoadWav input param error\n");
//		return -1;
//	}
//
//	if ((pstWavFile = fopen(pcFileName, "rb")) == NULL)
//	{
//		printf("LPUtilsLoadWav open wav file error:%s\n", pcFileName);
//		return -1;
//	}
//
//	 fread(stWaveFileHeader.chRIFF, sizeof(stWaveFileHeader.chRIFF), 1, pstWavFile);
//	 fread(&stWaveFileHeader.dwRIFFLen, sizeof(stWaveFileHeader.dwRIFFLen), 1, pstWavFile);
//	 fread(stWaveFileHeader.chWAVE, sizeof(stWaveFileHeader.chWAVE), 1, pstWavFile);
//	 fread(stWaveFileHeader.chFMT, sizeof(stWaveFileHeader.chFMT), 1, pstWavFile);
//	 fread(&stWaveFileHeader.dwFMTLen, sizeof(stWaveFileHeader.dwFMTLen), 1, pstWavFile);
//	 fread(&stWaveFileHeader.wFormatTag, sizeof(stWaveFileHeader.wFormatTag), 1, pstWavFile);
//	 fread(&stWaveFileHeader.nChannels, sizeof(stWaveFileHeader.nChannels), 1, pstWavFile);
//	 fread(&stWaveFileHeader.nSamplesPerSec, sizeof(stWaveFileHeader.nSamplesPerSec), 1, pstWavFile);
//	 fread(&stWaveFileHeader.nAvgBytesPerSec, sizeof(stWaveFileHeader.nAvgBytesPerSec), 1, pstWavFile);
//	 fread(&stWaveFileHeader.nBlockAlign, sizeof(stWaveFileHeader.nBlockAlign), 1, pstWavFile);
//	 fread(&stWaveFileHeader.wBitsPerSample, sizeof(stWaveFileHeader.wBitsPerSample), 1, pstWavFile);
//
//
//	printf("stWaveFileHeader.wBitsPerSample=%d\n", stWaveFileHeader.wBitsPerSample);
//	printf("stWaveFileHeader.nSamplesPerSec=%d\n", stWaveFileHeader.nSamplesPerSec);
//	printf("stWaveFileHeader.nChannels=%d\n", stWaveFileHeader.nChannels);
//
//	fseek(pstWavFile, sizeof(stWaveFileHeader.chRIFF) +
//			sizeof(stWaveFileHeader.dwRIFFLen) + sizeof(stWaveFileHeader.chWAVE) +
//			sizeof(stWaveFileHeader.chFMT) + sizeof(stWaveFileHeader.dwFMTLen) +
//			stWaveFileHeader.dwFMTLen, SEEK_SET);
//
//	fread(stWaveFileHeader.chDATA, sizeof(stWaveFileHeader.chDATA), 1, pstWavFile);
//	printf("stWaveFileHeader.chDATA=%s\n", stWaveFileHeader.chDATA);
//
//	if ((stWaveFileHeader.chDATA[0] == 'f')
//			&& (stWaveFileHeader.chDATA[1] == 'a')
//			&& (stWaveFileHeader.chDATA[2] == 'c')
//			&& (stWaveFileHeader.chDATA[3] == 't'))
//	{
//		fread(&stWaveFileHeader.dwFACTLen, sizeof(stWaveFileHeader.dwFACTLen), 1, pstWavFile);
//		fseek(pstWavFile, stWaveFileHeader.dwFACTLen, SEEK_CUR);
//		fread(stWaveFileHeader.chDATA, sizeof(stWaveFileHeader.chDATA), 1, pstWavFile);
//		printf("stWaveFileHeader.chDATA=%s\n", stWaveFileHeader.chDATA);
//	}
//
//	if ((stWaveFileHeader.chDATA[0] == 'd')
//			&& (stWaveFileHeader.chDATA[1] == 'a')
//			&& (stWaveFileHeader.chDATA[2] == 't')
//			&& (stWaveFileHeader.chDATA[3] == 'a'))
//	{
//		fread(&stWaveFileHeader.dwDATALen, sizeof(stWaveFileHeader.dwDATALen), 1, pstWavFile);
//		printf("stWaveFileHeader.dwDATALen=%d\n", stWaveFileHeader.dwDATALen);
//	}
//
//		if ((stWaveFileHeader.chRIFF[0] != 'R') || (stWaveFileHeader.chRIFF[1] != 'I') ||
//				(stWaveFileHeader.chRIFF[2] != 'F') || (stWaveFileHeader.chRIFF[3] != 'F') ||
//				(stWaveFileHeader.chWAVE[0] != 'W') || (stWaveFileHeader.chWAVE[1] != 'A') ||
//				(stWaveFileHeader.chWAVE[2] != 'V') || (stWaveFileHeader.chWAVE[3] != 'E') ||
//				(stWaveFileHeader.chFMT[0] != 'f') || (stWaveFileHeader.chFMT[1] != 'm') ||
//				(stWaveFileHeader.chFMT[2] != 't') || (stWaveFileHeader.chFMT[3] != ' '))
//		{
//			printf("error:wav file format error\n");
//			fclose(pstWavFile);
//			return -1;
//		}
//
//		if(pcBuffSize < stWaveFileHeader.dwDATALen)
//		{
//			printf("error:pcBuffSize too small(%d/%d)\n", pcBuffSize, stWaveFileHeader.dwDATALen);
//		}
//
//		iReadBytes = fread(pcBuff, stWaveFileHeader.dwDATALen, 1, pstWavFile);
//		if (iReadBytes != 1)
//		{
//			printf("error:read wav error:%s\n", pcFileName);
//			fclose(pstWavFile);
//			return -1;
//		}
//
//		iRet = stWaveFileHeader.dwDATALen;
//		fclose(pstWavFile);
//		printf("read data:%d\n",  iRet);
//		return iRet;
//}


/******************************************************************************/
sd_domnode_t* LPUtilsMessageXmlLoadUseFile(FILE *msgFile)
{
	LOG_RUNLOG_DEBUG("LP use LPUtilsMessageXmlLoadUseFile");
	int ret = 0;
	sd_list_iter_t* i = NULL;
	sd_domnode_t* node = NULL;
	sd_domnode_t* root_node = NULL;
	root_node = sd_domnode_new(NULL, NULL);
	if(root_node == NULL)
	{
		return NULL;
	}

	ret = __sd_domnode_xml_fread(&root_node, msgFile);
	//ret = sd_domnode_fread(root_node, msgFile); //LPMessageRecv sd_domnode_fread error
	if(ret == -1)
	{
		sd_domnode_delete(root_node);
		LOG_RUNLOG_WARN("LP LPMessageRecv sd_domnode_fread error");
		return NULL;
	}

	//sd_domnode_delete(root_node);
	return root_node;
}

//int LPUtilsMessageXmlLoadUseMemFile(const char* pcMsg)
//{
//	int c = 0;
//	//printf("debug:use LPUtilsMessageXmlLoadUseMemFile\n");
//	FILE* msgFile;
//	int ret = 0;
//	sd_list_iter_t* i = NULL;
//	sd_domnode_t* node = NULL;
//	sd_domnode_t* root_node = NULL;
//	root_node = sd_domnode_new(NULL, NULL);
//	if(root_node == NULL)
//	{
//		return -1;
//	}
//	if((msgFile = fmemopen(pcMsg, strlen(pcMsg), "r") ) == NULL)
//	{
//		LOG_RUNLOG_ERROR("LP LPMessageRecv fmemopen error");
//		return -1;
//	}
//
////	char buff[32] = {0x00};
////	printf("%s\n", "buff begin:");
////	while((c = fread(buff, 1, sizeof(buff), msgFile)) != 0)
////	{
////		printf("%d:%s", c, buff);
////		memset(buff, 0x00, sizeof(buff));
////	}
////	printf("\n%s\n", "buff end.");
////	fclose(msgFile);
//
//	ret = __sd_domnode_xml_fread(&root_node, msgFile);
////ret = sd_domnode_fread(root_node, msgFile); //LPMessageRecv sd_domnode_fread error
//	fclose(msgFile);
//	if(ret == -1)
//	{
//		sd_domnode_delete(root_node);
//		LOG_RUNLOG_ERROR("LP LPMessageRecv sd_domnode_fread error");
//		return -1;
//	}
//
//	//printf("root_node->name:%s\n", root_node->name);
//	for (i = sd_list_begin(root_node->attrs);
//			i != sd_list_end(root_node->attrs);
//			i = sd_list_iter_next(i))
//	{
//		sd_domnode_t* node_root = i->data;
//		//printf("root_node attrs->name:%s\n", node_root->name);
//		//printf("root_node attrs->value:%s\n", node_root->value);
//	}
//	//printf("\n");
//
//	sd_domnode_delete(root_node);
//
//	return 0;
//}




static void *LPUtilsMessageOCLoad(sd_domnode_t* pstRootNode)
{
	//printf("LPUtilsMessageOpenDoorLoad\n");
	sd_list_iter_t* i = NULL;

	for (i = sd_list_begin(pstRootNode->children); i != sd_list_end(pstRootNode->children);
			i = sd_list_iter_next(i))
	{
		sd_domnode_t* nodeChild = i->data;
		if (!strcmp(nodeChild->name, "HEADER"))
		{
			int iCheck = 0;
			SIP_MESSAGE_OC_ST* pstMsg = (SIP_MESSAGE_OC_ST*)malloc(sizeof(SIP_MESSAGE_OC_ST));
			if( NULL == pstMsg )
			{
				LOG_RUNLOG_ERROR("LP malloc SIP_MESSAGE_OPENDOOR failed");
				return NULL;
			}
			memset(pstMsg, 0x00, sizeof(SIP_MESSAGE_OC_ST));
			pstMsg->cCntType = SIP_MSG_TYPE_MESSAGE;

			for (i = sd_list_begin(nodeChild->attrs); i != sd_list_end(nodeChild->attrs);
					i = sd_list_iter_next(i))
			{
				sd_domnode_t* nodeAttr = i->data;
				if (!strcmp(nodeAttr->name, "ServiceType") && (!strcmp(nodeAttr->value, "ACCESS_MSG") || !strcmp(nodeAttr->value, "CALL_MSG"))) //GYY no item
				{
					iCheck++;
					strcpy(pstMsg->acServiceType, nodeAttr->value);
				}
				else if (!strcmp(nodeAttr->name, "MsgType") && (!strcmp(nodeAttr->value, "MSG_UNLOCK_REQ") || !strcmp(nodeAttr->value, "MSG_ANSWER_REQ")))
				{
					iCheck++;
					strcpy(pstMsg->acMsgType, nodeAttr->value);
				}
				else if (!strcmp(nodeAttr->name, "MsgSeq"))
				{
					iCheck++;
					pstMsg->iMsgSeq = atoi(nodeAttr->value);
				}
			}
			if(iCheck >= 2)
			{
				LOG_RUNLOG_DEBUG("LP LPUtilsMessageOCLoad check ok");
				LOG_RUNLOG_DEBUG("LP ----------------------------------------");
				LOG_RUNLOG_DEBUG("LP LPUtilsMessageOCLoad:");
				LOG_RUNLOG_DEBUG("LP cCntType: %d", pstMsg->cCntType);
				LOG_RUNLOG_DEBUG("LP acServiceType: %s", pstMsg->acServiceType);
				LOG_RUNLOG_DEBUG("LP acMsgType: %s", pstMsg->acMsgType);
				LOG_RUNLOG_DEBUG("LP iMsgSeq: %d", pstMsg->iMsgSeq);
				LOG_RUNLOG_DEBUG("LP ----------------------------------------");
				return pstMsg;
			}
			free(pstMsg);
			break;
		}
	}

	return NULL;
}

static void *LPUtilsMessageNotifyLoad(sd_domnode_t* pstRootNode)
{
	LOG_RUNLOG_DEBUG("LP LPUtilsMessageNotifyLoad");

	sd_list_iter_t* i = NULL;
	int iCheck = 0;

	SIP_MESSAGE_DOORPLATFORM_ST* pstMsg = (SIP_MESSAGE_DOORPLATFORM_ST*)malloc(sizeof(SIP_MESSAGE_DOORPLATFORM_ST));
	if( NULL == pstMsg )
	{
		LOG_RUNLOG_ERROR("LP malloc SIP_MESSAGE_DOORPLATFORM_ST failed");
		return NULL;
	}
	memset(pstMsg, 0x00, sizeof(SIP_MESSAGE_DOORPLATFORM_ST));

	for (i = sd_list_begin(pstRootNode->children); i != sd_list_end(pstRootNode->children);
			i = sd_list_iter_next(i))
	{
		pstMsg->cCntType = SIP_MSG_TYPE_NOTIFY;

		sd_domnode_t* nodeChild = i->data;
		if (!strcmp(nodeChild->name, "serviceType") && !strcmp(nodeChild->value, "notify"))
		{
			iCheck++;
			strcpy(pstMsg->acServiceType, nodeChild->value);
		}
		else if(!strcmp(nodeChild->name, "msgType"))
		{
			iCheck++;
			strcpy(pstMsg->acMsgType, nodeChild->value);
		}
		else if(!strcmp(nodeChild->name, "content"))
		{
		}
		else if(!strcmp(nodeChild->name, "description"))
		{
		}
		else if(!strcmp(nodeChild->name, "autoLink"))
		{
		}
		else if(!strcmp(nodeChild->name, "linkUrl"))
		{
		}
	}

	if(iCheck == 2)
	{
		LOG_RUNLOG_DEBUG("LP LPUtilsMessageNotifyLoad check ok");
		LOG_RUNLOG_DEBUG("LP ----------------------------------------");
		LOG_RUNLOG_DEBUG("LP LPUtilsMessageNotifyLoad:");
		LOG_RUNLOG_DEBUG("LP cCntType: %d", pstMsg->cCntType);
		LOG_RUNLOG_DEBUG("LP acServiceType: %s", pstMsg->acServiceType);
		LOG_RUNLOG_DEBUG("LP acMsgType: %s", pstMsg->acMsgType);
		LOG_RUNLOG_DEBUG("LP ----------------------------------------");
		return pstMsg;
	}
	free(pstMsg);
	return NULL;
}





void* LPUtilsAnalyzeNode(sd_domnode_t* pstRootNode)
{

	sd_list_iter_t* pstIter = NULL;
	if(pstRootNode == NULL || pstRootNode->name == NULL)
	{
		return NULL;
	}

	LOG_RUNLOG_DEBUG("LP pstRootNode->name:%s", pstRootNode->name);

	if(!strncmp(pstRootNode->name, "MESSAGE", 7))
	{
		return LPUtilsMessageOCLoad(pstRootNode);
	}
	if(!strncmp(pstRootNode->name, "notify", 6))
	{
		return LPUtilsMessageNotifyLoad(pstRootNode);
	}
//	if(!strncmp(pstRootNode->name, "pushInfo", 8))
//	{
//		return LPUtilsMessagePushInfoLoad(pstRootNode);
//	}

	return NULL;
}

//void LPUtilsMessageOpenDoorResponseOk(char *msgBody)
//{
//	snprintf(msgBody, LP_MESSAGE_BODY_MAX_LEN, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\r\n"
//			"<MESSAGE Version=\"1.0\"><MESSAGE Version=\"1.0\">\r\n\t"
//			"<HEADER ServiceType=\"ACCESS_MSG\" MsgType=\"MSG_UNLOCK_RSP\" MsgSeq=\"1\" />\r\n\t"
//			"<REUSULT Value=\"OK\" ErrorCode=\"0\"/>\r\n"
//			"</MESSAGE>");
//	LOG_RUNLOG_DEBUG("LP LPUtilsMessageOpenDoorResponseOk msgBody:\n%s", msgBody);
//}
//
//void LPUtilsMessageOpenDoorResponseError(char *msgBody)
//{
//	snprintf(msgBody, LP_MESSAGE_BODY_MAX_LEN, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\r\n"
//			"<MESSAGE Version=\"1.0\"><MESSAGE Version=\"1.0\">\r\n\t"
//			"<HEADER ServiceType=\"ACCESS_MSG\" MsgType=\"MSG_UNLOCK_RSP\" MsgSeq=\"1\" />\r\n\t"
//			"<REUSULT Value=\"ERROR\" ErrorCode=\"0\"/>\r\n"
//			"</MESSAGE>");
//	LOG_RUNLOG_DEBUG("LP LPUtilsMessageOpenDoorResponseError msgBody:\n%s", msgBody);
//}


//static int LPUtilsNetConnect(char *ip, int port) {
//	int fd = -1;
//	struct sockaddr_in sa;
//	int ret = 0;
//	int connected = -1;
//	int flag;
//
//	socklen_t sa_len = sizeof(struct sockaddr_in);
//	memset(&sa, 0, sizeof(struct sockaddr_in));
//	sa.sin_family = AF_INET;
//	sa.sin_addr.s_addr = inet_addr(ip);
//	sa.sin_port = htons(port);
//	fd = socket(AF_INET, SOCK_STREAM, 0);
//	LOG_RUNLOG_DEBUG("LP NET create socket %s : %d", ip, fd);
//
//
//	flag = fcntl(fd,F_GETFL,0);
//	flag |= O_NONBLOCK;
//	fcntl(fd,F_SETFL,flag);
//	connected = connect(fd, (struct sockaddr *) &sa, sa_len);
//	LOG_RUNLOG_DEBUG("LP NET connect value %s : %d %s", ip, connected, strerror(errno));
//	if (connected != 0 )
//	{
//		if(errno != EINPROGRESS)
//		{
//			LOG_RUNLOG_DEBUG("LP NET connect error %s : %s", ip, strerror(errno));
//			ret = -1;
//		}
//		else
//		{
//			struct timeval tm;
//			tm.tv_sec = 5;
//			tm.tv_usec = 0;
//			fd_set set,rset;
//			FD_ZERO(&set);
//			FD_ZERO(&rset);
//			FD_SET(fd,&set);
//			FD_SET(fd,&rset);
//			int res;
//			res = select(fd+1,&rset,&set,NULL,&tm);
//			if(res < 0)
//			{
//				LOG_RUNLOG_DEBUG("LP NET network error in connect %s", ip);
//				ret =  -1;
//			}
//			else if(res == 0)
//			{
//				LOG_RUNLOG_DEBUG("LP NET connect time out %s", ip);
//				ret = -2;
//			}
//			else if (1 == res)
//			{
//				if(FD_ISSET(fd,&set))
//				{
//					LOG_RUNLOG_DEBUG("LP NET connect ok %s", ip);
//					ret = 0;
//				}
//				else
//				{
//					ret = -3;
//					LOG_RUNLOG_DEBUG("LP NET other error when select %s : %s", ip, strerror(errno));
//				}
//			}
//		}
//	}
//
//	if(fd >= 0)
//	{
//		close(fd);
//		LOG_RUNLOG_DEBUG("LP NET close socket %s : %d", ip, fd);
//	}
//
//	return ret;
//}

int LPUtilsNetConnect(char *ip, int port)
{

	struct hostent* host = NULL;
	struct sockaddr_in saddr;
	unsigned int s = 0;
	int ret = 0;
	int error;
	host = gethostbyname(ip);
	if (host == NULL)
		return -1;

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr = *((struct in_addr*)host->h_addr);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return -1;
	}
//	LOG_RUNLOG_DEBUG("LP NET create socket %s : %d", ip, s);
	//printf("LP NET create socket %s : %d\n", ip, s);
	fcntl(s, F_SETFL, O_NONBLOCK);

	if (connect(s, (struct sockaddr*) &saddr, sizeof(saddr)) == -1)
	{
		if (errno == EINPROGRESS)
		{ // it is in the connect process
			struct timeval tv;
			fd_set writefds;
			tv.tv_sec = 7;
			tv.tv_usec = 0;
			FD_ZERO(&writefds);
			FD_SET(s, &writefds);
			if (select(s + 1, NULL, &writefds, NULL, &tv) > 0)
			{
				int len = sizeof(int);
				getsockopt(s, SOL_SOCKET, SO_ERROR, &error, &len);
				if (error == 0)
				{
					ret = 0;
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
		ret = 0;


	if(ret == 0)
	{
		LOG_RUNLOG_DEBUG("LP NET connect ok %s", ip);
		//printf("LP NET connect ok %s\n", ip);
	}
	else
	{
		LOG_RUNLOG_DEBUG("LP NET connect failed %s", ip);
		//printf("LP NET connect failed %s\n", ip);
	}

	if(s >= 0)
	{
		close(s);
//		LOG_RUNLOG_DEBUG("LP NET close socket %s : %d", ip, s);
		//printf("LP NET close socket %s : %d\n", ip, s);
	}
	return ret;
}

static int g_iLPNet = -1;

int LPUtilsGetNetStatus()
{
	return g_iLPNet;
}

int LPUtilsCheckNet()
{
//	char ip1[] = "196.196.196.1";
//	char ip2[] = "196.196.196.1";
	char ip1[] = LP_CHECK_NET_IP1;
	char ip2[] = LP_CHECK_NET_IP2;
	int port = 80;
	int connRes = 0;

	connRes = LPUtilsNetConnect(g_stIdbtCfg.stDestIpInfo.acDestIP1, g_stIdbtCfg.stDestIpInfo.port1);
	if (connRes < 0)
	{
		connRes = LPUtilsNetConnect(g_stIdbtCfg.stDestIpInfo.acDestIP2, g_stIdbtCfg.stDestIpInfo.port2);
	}

	if (connRes < 0)
	{
		//LOG_RUNLOG_DEBUG("LP NET offline");
		g_iLPNet = -1;
	}
	else
	{
		//LOG_RUNLOG_DEBUG("LP NET online");
		g_iLPNet = 0;
	}

	return g_iLPNet;
}

