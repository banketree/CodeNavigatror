/*
 * I6Interface.c
 *
 *  Created on: 2012-12-29
 *      Author: root
 */
#include <IDBT.h>
#include <MD5.h>
#include <https.h>
#include <YKMsgQue.h>
#include <FPInterface.h>
#include "sd/domnode.h"
#include <GI6Interface.h>
#include <sys/time.h>
#include <TMInterface.h>
#include <LPIntfLayer.h>
#include <LOGIntfLayer.h>
#include <IDBTCfg.h>
//#define _GYY_I6_
#if _TM_TYPE_ == _GYY_I6_

//POST / HTTP/1.1
//
//Host: www.wrox.com
//
//User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.7.6)
//
//Gecko/20050225 Firefox/1.0.1
//
//Content-Type: application/x-www-form-urlencoded
//
//Content-Length: 40
//
//Connection: Keep-Alive

#define POST_HEART "/door/webservice/notifyHeartbeat"
#define POST_GET_SIP "/door/webservice/visualPhoneUserLogin"

#define PRE_POST_REQUEST "\
POST %s HTTP/1.1\r\n\
User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n\
Content-Type: application/x-www-form-urlencoded\r\n\
Content-Length: %d\r\n\
Host: %s:%d\r\n\
Connection: Close\r\n\r\n\
%s\r\n\r\n\
"

#define POST_REQUEST "\
POST %s HTTP/1.1\r\n\
User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n\
Cookie: %s\r\n\
Content-Type: application/x-www-form-urlencoded\r\n\
Content-Length: %d\r\n\
Host: %s:%d\r\n\
Connection: Close\r\n\r\n\
%s\r\n\r\n\
"

#ifdef I6_HTTPS
#include "polarssl/config.h"
#include "polarssl/net.h"
#include "polarssl/ssl.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"
#include "polarssl/error.h"

ssl_context i6_ssl;
#endif

#define TM_QUE_LEN 256
//消息队列
static pthread_t g_hMsgRecv;
static pthread_t g_hMsgSend;
static pthread_t i6_obj;
YK_MSG_QUE_ST*  g_pstTMMsgQ = NULL;
TM_TIMER_ST g_stNotifySipTimer;
//连接套接字
int i6_server_fd = 0;
int i6_port;
char i6_host_addr[256];
char i6_path[1024];
char i6_cookie[64] = {0x0};
char i6_soft_url[256] = {0x0};
char i6_heart_url[256] = {0x0};
static char g_acTemp[1024] = {0x0};
static char g_acOldUser[128] = {0x0};
//相关结构题
I6_AUTHENTICATION_ST authentication;
I6_CONFIG_ST i6config;
I6_PASSWORD_ST i6password;
I6_SOFT_ST i6soft;
I6_CARD_ST i6card;
I6_DEVICE_ST i6device;
I6_SIP_INFO_ST I6SipInfo;
char RandomNum[17] = {0x0};
//是否保存文件
int g_bI6Config = 0;
int g_bI6Key = 0;
int g_bI6Password = 0;
int g_bI6Temp = 0;
//消息队列运行状态
int g_bTMMsgTaskFlg = TRUE;
//运行状态
int g_bSendHeart = 0;
int g_bBoot = 0;
int g_bGetSip = 0;
int g_bAppraise = 0;
int g_bSetTime = 0;
int g_bUseLocal = 0;
int g_bHaveAlarmUpdate = 0;
#if _TM_TYPE_== _GYY_I6_
#ifdef _YK_PC_
int displayFlag = TRUE;
#else
int displayFlag = FALSE;
#endif
#endif
//#include <iconv.h>
extern int evcpe_persister_persist(struct evcpe_persister *persist);

int I6ProcessCommand(const char *cmd)
{
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

	return SUCCESS;
}

////代码转换:从一种编码转为另一种编码
//int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int *outlen)
//{
//	iconv_t cd;
//	int rc;
//	char **pin = &inbuf;
//	char **pout = &outbuf;
//
//	cd = iconv_open(to_charset,from_charset);
//	if (cd==0) return -1;
//	memset(outbuf,0,outlen);
//	if (iconv(cd,pin,&inlen,pout,&outlen)==-1) return -1;
//	iconv_close(cd);
//	return 0;
//}
//
//int u2g(char *inbuf,int inlen,char *outbuf,int *outlen)
//{
//	return code_convert("UTF-8","gb2312",inbuf,inlen,outbuf,outlen);
//}

void I6RemoveXml()
{
	system("rm "I6_CONFIG_PATH);
	system("rm "I6_PASSWORD_PATH);
	system("rm "I6_CARD_PATH);
	TMI6ConfigClose();
	I6CreateCardXml();
}

//======================================================================================
void i6_get_datetime()
{
	time_t   timep;
	struct   tm     *timenow;
	//时间是否需要修正
	time   (&timep);
	timenow   =   localtime(&timep);
	LOG_RUNLOG_DEBUG( "i6_get_datetime now year : %d", timenow->tm_year+1900 );
	if(timenow->tm_year +1900 > 2012)
	{
		return;
	}
	g_bSetTime = 1;
}

void i6_set_datetime(const char *pcDatatime)
{
	char acDatatime[128];
	char acTime[64];
	char acMonth[64];
	char acYear[64];
	char *tmp;
	int iMonth;
	int iData;
	int iHour;
	int iMin;
	int iYear;
	static int iUpdateFirstFlg = 0;

	strcpy(acDatatime,pcDatatime);
	tmp = strtok(acDatatime," ");
	tmp = strtok(NULL," ");
	iData = atoi(tmp);
	tmp = strtok(NULL," ");
	strcpy(acMonth,tmp);
	if(strcmp(acMonth,"Jan") == 0)
	{
		iMonth = 1;
	}
	else if(strcmp(acMonth,"Feb") == 0)
	{
		iMonth = 2;
	}
	else if(strcmp(acMonth,"Mar") == 0)
	{
		iMonth = 3;
	}
	else if(strcmp(acMonth,"Apr") == 0)
	{
		iMonth = 4;
	}
	else if(strcmp(acMonth,"May") == 0)
	{
		iMonth = 5;
	}
	else if(strcmp(acMonth,"Jun") == 0)
	{
		iMonth = 6;
	}
	else if(strcmp(acMonth,"Jul") == 0)
	{
		iMonth = 7;
	}
	else if(strcmp(acMonth,"Aug") == 0)
	{
		iMonth = 8;
	}
	else if(strcmp(acMonth,"Sep") == 0)
	{
		iMonth = 9;
	}
	else if(strcmp(acMonth,"Oct") == 0)
	{
		iMonth = 10;
	}
	else if(strcmp(acMonth,"Nov") == 0)
	{
		iMonth = 11;
	}
	else if(strcmp(acMonth,"Dec") == 0)
	{
		iMonth = 12;
	}
	tmp = strtok(NULL," ");
	strcpy(acYear,tmp);
	tmp = strtok(NULL," ");
	strcpy(acTime,tmp);
	tmp = strtok(acTime,":");
	iHour = atoi(tmp);
	tmp = strtok(NULL,":");
	iMin = atoi(tmp);
	sprintf(acDatatime,"busybox date %02d%02d%02d%02d%s",iMonth,iData,iHour,iMin,acYear);
	notifySetSyncTime(acDatatime);
//	setenv("TZ", "CST", 1);
////	system("date");
	LOG_RUNLOG_DEBUG("i6_set_datetime notifySetSyncTime");
//	system(acDatatime);
//	setenv("TZ", "CST-8", 1);
//	system("busybox date");
	//修改日志时间
	if(iUpdateFirstFlg == 0)
	{
		sleep(2);
		LOG_RUNLOG_DEBUG("i6_set_datetime LOGUploadResPrepross");
		tzset();
		iUpdateFirstFlg++;
		LOGUploadResPrepross();
		LOGUploadExec(0);
	}
//	system("date");
}

void I6GetIMSInfo(char* pcUserName, char* pcPassWord, char* pcProxy, int* piPort, char* pcDomain)
{
	if(pcUserName)
	{
		strcpy(pcUserName,I6SipInfo.acSipAccont);
	}
	if(pcPassWord)
	{
		strcpy(pcPassWord,I6SipInfo.acSipPwd);
	}
	if(pcProxy)
	{
		strcpy(pcProxy,I6SipInfo.acProxy);
	}
	if(piPort)
	{
		*piPort = I6SipInfo.iPort;
	}
	if(pcDomain)
	{
		strcpy(pcDomain,I6SipInfo.acDomain);
	}
}

void I6GetIMSInfo2(char* pcUserName)
{
	if(pcUserName)
	{
		strcpy(pcUserName,I6SipInfo.acSipAccont);
	}
}

BOOL I6GetDomainName(char* pcDomain)
{
	if(pcDomain && I6SipInfo.acDomain[0] != 0x0)
	{
		strcpy(pcDomain,I6SipInfo.acDomain);
		return TRUE;
	}
	return FALSE;
}

void I6GetSipFromXml()
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t *search,*child;

	if(-1 == sd_domnode_load(document, BA_DATA_PATH)){
		sd_domnode_delete(document);
		return ;
	}

	search = sd_domnode_search(document,"SipAccount");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value)
		{
			strcpy(i6device.acSipAccont,child->value);
			strcpy(I6SipInfo.acSipAccont,child->value);
		}
	}

	search = sd_domnode_search(document,"SipPassword");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value)
		{
			strcpy(I6SipInfo.acSipPwd,child->value);
		}
	}

	search = sd_domnode_search(document,"ImsDomainName");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value)
		{
			strcpy(I6SipInfo.acDomain,child->value);
		}
	}

	search = sd_domnode_search(document,"ImsProxyIP");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value)
		{
			strcpy(I6SipInfo.acProxy,child->value);
		}
	}

	search = sd_domnode_search(document,"ImsProxyPort");
	if(search)
	{
		child = sd_domnode_first_child(search);
		if(child && child->value)
		{
			sscanf(child->value,"%d",&I6SipInfo.iPort);
		}
	}

	sd_domnode_delete(document);
	return ;
}

void I6SetSipInfo(char *pcUserName,char *pcPassWord,char *pcDomain ,char *pcProxy,char * pcPort)
{
	strcpy(i6device.acSipAccont,pcUserName);
	strcpy(I6SipInfo.acSipAccont,pcUserName);
	strcpy(I6SipInfo.acSipPwd,pcPassWord);
	strcpy(I6SipInfo.acDomain,pcDomain);
	strcpy(I6SipInfo.acProxy,pcProxy);
	I6SipInfo.iPort = atoi(pcPort);
//	I6StoreSipInfo();
	//判断原有用户数据配置文件中SIP号是否相同 不同则删除
	if(I6CheckConfigSip(i6device.acSipAccont) != 0)
	{
		I6RemoveXml();
		i6device.iKeyVersion = 0;
		i6device.iConfigVersion = 0;
		i6device.iPasswordVersion = 0;
	}
	//获取SIP号后操作
	I6HaveGetSip();
}

//－－－－－－－－－－－－－－－－－－消息队列发送－－－－－－－－－－－－－－－－－－－－－－－－－
int TRMsgSendI6Method(int iMethod)
{
    int iRet = -1;

    if(NULL == g_pstTMMsgQ)
    {
        return -1;
    }

    TM_I6_BOOT_ST* pstMsg = (TM_I6_BOOT_ST*)malloc(sizeof(TM_I6_BOOT_ST));
    if( NULL == pstMsg )
    {
        return -1;
    }
    memset(pstMsg, 0x00, sizeof(TM_I6_BOOT_ST));
    pstMsg->uiPrmvType = iMethod;

    iRet = YKWriteQue( g_pstTMMsgQ,  pstMsg,  250);


    return iRet;
}

//－－－－－－－－－－－－－－－－－－消息队列处理－－－－－－－－－－－－－－－－－－－－－－－－－
void I6CardSwipFunc(void *arg)
{
	int times = 0;
	times = 0;
	TM_I6_CARD_SWIP_ST *tmp = (TM_I6_CARD_SWIP_ST *)arg;

	while(1)
	{
		if(NMNetTest() == 0)
		{
			if(I6Authentication() != 0)
			{
				LOG_RUNLOG_DEBUG("I6Authentication error \n");
				break;
			}
			if(I6CardSwip(&tmp->aucCardInfo) != 0)
			{
				LOG_RUNLOG_DEBUG("I6CardSwip error \n");
			}
			I6QuitHttps();
			break;
		}
		if(times++ > 5)
		{
			break;
		}
		sleep(1);
	}
}

void I6AlarmUpdateFunc(void *arg)
{
	int times = 0;
	times = 0;

	while(1)
	{
		if(NMNetTest() == 0)
		{
			if(I6Authentication() != 0)
			{
				printf("I6Authentication error \n");
				break;
			}
			if(I6AlarmUpdate(arg) != 0)
			{
				printf("I6AlarmUpdate error \n");
			}
			I6QuitHttps();
			break;
		}
		if(times++ > 5)
		{
			break;
		}
		sleep(1);
	}
	if(g_bHaveAlarmUpdate == 1)
	{
		g_bHaveAlarmUpdate = 2;
	}
}

#if  _TM_TYPE_== _GYY_I6_
void TRNotifySipCallBack()
{
	char acUserName[64];
	char acPassword[64];
	char acProxy[64];
	char acDomain[64];
	int  iPort;
	int i = 0;

	TMGetIMSInfo(acUserName, acPassword, acProxy, &iPort, acDomain);
	if(g_acOldUser[0] == 0x0)
	{
		strcpy(g_acOldUser,acUserName);
	}
	else
	{
		if(strcmp(g_acOldUser,acUserName) == 0)
		{
			if(LPGetSipRegStatus() == REG_STATUS_ONLINE)
			{
				return ;
			}
		}
	}
	while(1)
	{
		if(LPMsgSendTmSipRegister(acUserName, acPassword, acDomain,acProxy) == 0)
		{
			strcpy(g_acOldUser,acUserName);
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

void I6HaveGetSip()
{
	//通知linphone注册
	g_stNotifySipTimer.pstTimer = YKCreateTimer(TRNotifySipCallBack,NULL,1000, YK_TT_NONPERIODIC,(unsigned long *)&g_stNotifySipTimer.ulMagicNum);
	//修改状态为已获取SIP号
	g_bGetSip = 1;
	g_bBoot = 0;
}
#endif
#if 0
int I6GetSip()
{
	//组包
	char *pcBuf;
	char *rendered;
	char buf[1024];
	char acBuf[1024] = {0x0};
	cJSON *root,*fmt1,*fmt2;
	int len;
	int rst = 0;
	time_t   now;         //实例化time_t结构
	struct   tm     *timenow;         //实例化tm结构指针

	now = time(NULL);
	timenow  = localtime(&now);
	root=cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "s_id", TERMINAL_TYPE);
	sprintf(acBuf,"%4d%02d%02d%02d%02d%02d",timenow->tm_year+1900,
			timenow->tm_mon+1,timenow->tm_mday,timenow->tm_hour,
			timenow->tm_min,timenow->tm_sec);
	cJSON_AddStringToObject(root, "time", acBuf);
	cJSON_AddStringToObject(root,"seque",   i6device.acSerialNum  );
//	cJSON_AddStringToObject(root,"v_code",       YK_APP_VERSION);
	sprintf(acBuf,"%4d%02d%02d%02d%02d%02d%04d",timenow->tm_year+1900,
			timenow->tm_mon+1,timenow->tm_mday,timenow->tm_hour,
			timenow->tm_min,timenow->tm_sec,rand()%65536);
	cJSON_AddStringToObject(root,"requ_id",       acBuf);
	cJSON_AddItemToObject(root,"call_info",   fmt1 = cJSON_CreateArray());
	cJSON_AddItemToArray(fmt1,fmt2 = cJSON_CreateObject());
	cJSON_AddItemToObject(fmt2,"content",   fmt1 = cJSON_CreateObject());
	cJSON_AddStringToObject(fmt1,"seque",   i6device.acSerialNum);
	rendered=cJSON_Print(root);
	if(rendered)
	{
		sprintf(acBuf,PRE_POST_REQUEST,POST_GET_SIP,strlen(rendered),
				i6_host_addr,i6_port,rendered);
	}
	cJSON_Delete(root);

	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return -1;
	}

	//解包
//	u2g(pcBuf,strlen(pcBuf),buf,1023);
	cJSON *root1 = cJSON_Parse(pcBuf);
	if(!root1)
	{
		return -1;
	}
	char *rendered1=cJSON_Print(root1);
	if(displayFlag == TRUE)
	{
		LOG_RUNLOG_DEBUG("%s\n",rendered1);
	}

	int result = cJSON_GetObjectItem(root1,"result")->valueint;
	if(!result)
	{
		cJSON *sip_info = cJSON_GetObjectItem(root1,"resp_info");
		int size = cJSON_GetArraySize(sip_info);
		cJSON *content1 = cJSON_GetArrayItem(sip_info,0);
		if(!content1)
		{
			return -1;
		}
		cJSON *content2 = cJSON_GetObjectItem(content1,"content");
		if(!content2)
		{
			return -1;
		}
		content1 = cJSON_GetObjectItem(content2,"user_info");
		if(!content1)
		{
			return -1;
		}
		content2 = cJSON_GetObjectItem(content1,"sipInfo");
		if(!content2)
		{
			return -1;
		}
		content1 = cJSON_GetObjectItem(content2,"sipAccount");
		if(!content1)
		{
			return -1;
		}
		strcpy(i6device.acSipAccont,content1->valuestring);
		strcpy(I6SipInfo.acSipAccont,content1->valuestring);
		content1 = cJSON_GetObjectItem(content2,"sipPort");
		if(!content1)
		{
			return -1;
		}
		I6SipInfo.iPort = content1->valueint;
		content1 = cJSON_GetObjectItem(content2,"sipAreaName");
		if(!content1)
		{
			return -1;
		}
		strcpy(I6SipInfo.acDomain,content1->valuestring);
		content1 = cJSON_GetObjectItem(content2,"sipPwd");
		if(!content1)
		{
			return -1;
		}
		strcpy(I6SipInfo.acSipPwd,content1->valuestring);
		content1 = cJSON_GetObjectItem(content2,"sipProxy");
		if(!content1)
		{
			return -1;
		}
		strcpy(I6SipInfo.acProxy,content1->valuestring);
		//更新配置信息到I6配置文件
		I6StoreSipInfo();
		//判断原有用户数据配置文件中SIP号是否相同 不同则删除
		if(I6CheckConfigSip(i6device.acSipAccont) != 0)
		{
			I6RemoveXml();
			i6device.iKeyVersion = 0;
			i6device.iConfigVersion = 0;
			i6device.iPasswordVersion = 0;
		}
		//获取SIP号后操作
		I6HaveGetSip();
		cJSON_Delete(root1);
		return 0;
	}
	cJSON_Delete(root1);
	return -1;
}
#endif

//－－－－－－－－－－－－－－－－－－消息收发－－－－－－－－－－－－－－－－－－－－－－－－－
void* TMMsgRecvEvent(void* arg)
{
	void *pvMsg = NULL;

	while(g_bTMMsgTaskFlg)
	{
		YKReadQue(g_pstTMMsgQ, &pvMsg, 200);			//阻塞读取
		if(NULL == pvMsg)
		{
			continue;
		}
		else
		{
			switch(*(( unsigned int *)pvMsg))
			{
			case TM_I6_CARD_SWIP:
				I6CardSwipFunc(pvMsg);
				break;
			case TM_I6_ALARM_UPDATE:
				I6AlarmUpdateFunc(pvMsg);
				break;
			case TM_I6_HEART:
				I6Heart();
				g_bSendHeart = 0;
				break;
			case TM_I6_GET_SIP:
//				I6GetSip();
				break;
#ifdef _YK_GYY_
			case SIPTM_CONFIG_UPDATE:
#endif
			case TM_I6_BOOT:
			case TM_I6_CONFIG:
				if(I6Authentication() != 0)
				{
					LOG_RUNLOG_DEBUG("I6Authentication error \n");
					break;
				}
				if(I6UpdateConfig() != 0)
				{
					LOG_RUNLOG_DEBUG("I6UpdateConfig error \n");
				}
				LOG_RUNLOG_DEBUG("TM ====CONFIG_UPDATE=====\n");
				if(g_bBoot == 1)
				{
					I6QuitHttps();
					break;
				}
#ifdef _YK_GYY_
			case SIPTM_PWD_UPDATE:
#endif
			case TM_I6_PASSWORD:
#if defined(_YK_XT8126_DOOR_)
				LOG_RUNLOG_DEBUG("TM ====PWD_NO_NEED_UPDATE=====\n");
				if(g_bBoot == 1)
				{
					I6QuitHttps();
					break;
				}
#else
				if(I6Authentication() != 0)
				{
					LOG_RUNLOG_DEBUG("I6Authentication error \n");
					break;
				}
				if(I6UpdatePassword() != 0)
				{
					LOG_RUNLOG_DEBUG("I6UpdatePassword error \n");
				}
				LOG_RUNLOG_DEBUG("TM ====PWD_UPDATE=====\n");
				if(g_bBoot == 1)
				{
					I6QuitHttps();
					break;
				}
#endif
#ifdef _YK_GYY_
			case SIPTM_SOFT_UPDATE:
#endif
			case TM_I6_UPDATE:

				if(I6Authentication() != 0)
				{
					LOG_RUNLOG_DEBUG("I6Authentication error \n");
					break;
				}
#ifndef _YK_PC_
				if(I6UpdateSoft() != 0)
				{
					LOG_RUNLOG_DEBUG("I6UpdateSoft error \n");
				}
#endif
				LOG_RUNLOG_DEBUG("TM ====SOFT_UPDATE=====\n");
				if(g_bBoot == 1)
				{
					I6QuitHttps();
					break;
				}
#ifdef _YK_GYY_
			case SIPTM_CARD_UPDATE:
#endif
			case TM_I6_CARD:
				if(I6Authentication() != 0)
				{
					LOG_RUNLOG_DEBUG("I6Authentication error \n");
					break;
				}
				if(I6UpdateCard() != 0)
				{
					LOG_RUNLOG_DEBUG("I6UpdateCard error \n");
				}
				I6QuitHttps();
				if(g_bBoot == 0)
				{
					g_bBoot = 1;
				}
				LOG_RUNLOG_DEBUG("TM ====CARD_UPDATE=====\n");
				break;
			default:
				break;
			}
		}

		if(pvMsg)
		{
			free(pvMsg);
			pvMsg = NULL;
		}
	}

	g_pstTMMsgQ = NULL;
	return NULL;

}

void I6UseLocalSipInfo()
{
	LOG_RUNLOG_DEBUG("TM I6UseLocalSipInfo\n");
	g_bUseLocal = 1;
}

void I5ConnectFail(int times)
{
	LOG_RUNLOG_DEBUG("TM I5ConnectFail %d\n",times);
}

void* TMMsgSendEvent(void* arg)
{
	void *pvMsg = NULL;
	int times = 30;
	int local = 0;

	TMI6ConfigOpen();

	if(g_stIdbtCfg.iNetMode == 1)
	{
		I6UseLocalSipInfo();
	}

	while(g_bTMMsgTaskFlg)
	{
		sleep(1);

		//判断是否已获得专用号码
		//多次连接失败，使用本地配置文件
		if(g_bGetSip == 0 && g_bUseLocal == 1)
		{
			LOG_RUNLOG_DEBUG("TM TMMsgSendEvent uselocal\n");
			//读取配置文件中的SIP数据
			I6GetSipFromXml();
			//判断原有用户数据配置文件中SIP号是否相同 不同则删除
			if(I6CheckConfigSip(i6device.acSipAccont) != 0)
			{
				I6RemoveXml();
				i6device.iKeyVersion = 0;
				i6device.iConfigVersion = 0;
				i6device.iPasswordVersion = 0;
			}
			//获取SIP操作
			I6HaveGetSip();
			LOG_RUNLOG_DEBUG("TM TMMsgSendEvent getsipformxml\n");
		}

		if(++ times  >= 60)
		{
			times = 0;

			LOG_RUNLOG_DEBUG("TM TMMsgSendEvent g_bGetSip : %d ,g_bUseLocal : %d\n",g_bGetSip,g_bUseLocal);

			if(NMNetTest() == -1)
			{
				TMSetLinkStatus(LINK_STATUS_OFFLINE);
				continue;
			}

			i6_get_datetime();

			//进行上电流程
			if(g_bGetSip == 1 && g_bBoot == 0)
			{
				TRMsgSendI6Method(TM_I6_BOOT);
			}

			if(g_bBoot == 1)
			{
				if(g_bSendHeart == 0)
				{
					g_bSendHeart = 1;
				}
				else if(g_bSendHeart == 1)
				{
					continue;
				}
				//进行心跳流程
				TRMsgSendI6Method(TM_I6_HEART);
			}
		}
	}
}

//－－－－－－－－－－－－－－－－－－参数获取及修改－－－－－－－－－－－－－－－－－－－－－－－－－
void I6CreateCardXml()
{
	FILE *fp = NULL;
	char *cardxml="\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<version seq=\"0\">\r\n\
<blackCard>\r\n\
</blackCard>\r\n\
<cardAdd>\r\n\
</cardAdd>\r\n\
</version>\r\n\
";
	fp = fopen(I6_CARD_PATH,"w+");
	fwrite(cardxml,1,strlen(cardxml),fp);
	fclose(fp);
}

int I6DeviceFill(I6_DEVICE_ST *device)
{
	char buf[32];
	char *p;
	int a=0,b=0,c=0;
	char ID[32];
	memset(device,0,sizeof(I6_DEVICE_ST));

	strcpy(buf,YK_APP_VERSION);
	if(strstr(buf,"gho"))
	{
		strcpy(device->acDeviceType,"gho");
	}
	else if(strstr(buf,"gha"))
	{
		strcpy(device->acDeviceType,"gha");
	}
	else if(strstr(buf,"ghf"))
	{
		strcpy(device->acDeviceType,"ghf");
	}
	else if(strstr(buf,"ghb"))
	{
		strcpy(device->acDeviceType,"ghb");
	}
	else if(strstr(buf,"fhs"))
	{
		strcpy(device->acDeviceType,"fhs");
	}
	else if(strstr(buf,"fht"))
	{
		strcpy(device->acDeviceType,"fht");
	}
	else if(strstr(buf,"fh"))
	{
		strcpy(device->acDeviceType,"fh");
	}
	else if(strstr(buf,"pc"))
	{
		strcpy(device->acDeviceType,"pc");
	}
#ifdef _YK_S5PV210_
	else
	{
		strcpy(device->acDeviceType,"ab");
	}
#endif

	p = strtok(buf,".");
	a = atoi(p) * 10000;
	p = strtok(NULL,".");
	b = atoi(p) * 100;
	p = strtok(NULL,".");
	c = atoi(p) * 1;
	device->iSoftVersion = a+b+c;
	device->iConfigVersion = I6ConfigVersion(I6_CONFIG_PATH);
	device->iKeyVersion = TMI6GetCardVersion(I6_CARD_PATH);
	if(device->iKeyVersion == 0)
	{
		I6CreateCardXml();
	}
	strcpy(device->acDevicePassword,"123456");
	TMNMGetSN(device->acSerialNum);
}

//－－－－－－－－－－－－－－－－－－连接断开－－－－－－－－－－－－－－－－－－－－－－－－－
int I6ConnectHttps()
{
	int ret = -1, len;
	unsigned char buf[1024];
	int i = 0;
#ifdef I6_HTTPS
	char *pers = "ssl_client1";
	entropy_context entropy;
	ctr_drbg_context ctr_drbg;
	memset( &i6_ssl, 0, sizeof( ssl_context ) );
#endif
	i6_server_fd = ATConnect(i6_host_addr,i6_port,0);
	if(!i6_server_fd)
	{
		LOG_RUNLOG_DEBUG( " failed\n  ! net_connect returned %d\n\n", ret );
		return -1;
	}
	ret = 0;

#ifdef I6_HTTPS
	entropy_init( &entropy );
	if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
							   (unsigned char *) pers, strlen( pers ) ) ) != 0 )
	{
		LOG_RUNLOG_DEBUG( " failed\n  ! ctr_drbg_init returned %d\n", ret );
		goto exit;
	}

	if( ( ret = ssl_init( &i6_ssl ) ) != 0 )
	{
		LOG_RUNLOG_DEBUG( " failed\n  ! ssl_init returned %d\n\n", ret );
		goto exit;
	}
	ssl_set_endpoint( &i6_ssl, SSL_IS_CLIENT );
	ssl_set_authmode( &i6_ssl, SSL_VERIFY_NONE );

	ssl_set_rng( &i6_ssl, ctr_drbg_random, &ctr_drbg );
//	ssl_set_dbg( &i6_ssl, my_debug, stdout );
	ssl_set_bio( &i6_ssl, net_recv, &i6_server_fd,net_send, &i6_server_fd );
#endif
exit:

    return( ret );
}

//断开
void I6DisConnectHttps()
{
	if(i6_server_fd)
	{
		ATDisConnect(i6_server_fd);
//		net_close( i6_server_fd );
		i6_server_fd = 0;
	}
#ifdef I6_HTTPS
	if(&i6_ssl != NULL)
	{
		ssl_close_notify( &i6_ssl );

		ssl_free( &i6_ssl );

		memset( &i6_ssl, 0, sizeof( ssl_context ) );
	}
#endif
}

void I6QuitHttps()
{
	I6DisConnectHttps();
	i6_cookie[0] = 0x0;
	RandomNum[0] = 0x0;
	g_bAppraise = 0;
	g_bI6Config = 0;
	g_bI6Key = 0;
	g_bI6Password = 0;
}

//－－－－－－－－－－－－－－－－－－版本获取－－－－－－－－－－－－－－－－－－－－－－－－－
int TMI6GetCardVersion(const char *pcFileName)
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t* card;
	sd_domnode_t* find;
	sd_domnode_t* child;
	int version = 0;
	if(-1 == sd_domnode_load(document, pcFileName)){
		LOG_RUNLOG_DEBUG("card xml is error\n");
		sd_domnode_delete(document);
		return 0;
	}

	card = sd_domnode_search(document, "version");
	if(card)
	{
		version = atoi(sd_domnode_attrs_get(card, "seq")->value);
	}

	sd_domnode_delete(document);

	return version;
}

int I6ConfigVersion(const char *pcFileName)
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t *search;
	int rst = 0;

	if(-1 == sd_domnode_load(document,pcFileName)){
		printf("xml is error\n");
		sd_domnode_delete(document);
		return rst;
	}

	search = sd_domnode_search(document, CONFIG_VERSION);
	if(search && search->value)
	{
		rst = atoi(search->value);
	}

	sd_domnode_delete(document);
	return rst;
}

//－－－－－－－－－－－－－－－－－－创建及销毁－－－－－－－－－－－－－－－－－－－－－－－－－
int I6Init()
{
	int flag = 0;
	int rst = 0;
	char url[128];
	//配置文件中url跟sn
	TMGetUrl(url);
	I6DeviceFill(&i6device);
	LOG_RUNLOG_DEBUG("url %s ,cfg v %d ,key v %d,dev psw %s\n",url,i6device.iConfigVersion,
			i6device.iKeyVersion,i6device.acDevicePassword);
	if(GetHost(url,i6_host_addr, i6_path, &i6_port,&flag) != 0)        /*分析网址、端口、文件名等 */
	{
		LOG_RUNLOG_DEBUG("i6 url is error");
		return 0;
	}

	//创建消息队列
	g_pstTMMsgQ = YKCreateQue(TM_QUE_LEN);
	if( NULL == g_pstTMMsgQ )
	{
		return -1;
	}

	rst = pthread_create(&g_hMsgRecv, NULL, TMMsgRecvEvent, NULL);
	if (0 != rst)
	{
		return -1;
	}

	rst = pthread_create(&g_hMsgSend, NULL, TMMsgSendEvent, NULL);
	if (0 != rst)
	{
		return -1;
	}
#ifdef CONFIG_SORT
	//数据索引
	SortInit();
	SortConfigCode();
	SortConfigCornet();
#endif
	return 0;
}

void I6Fini()
{
    g_bTMMsgTaskFlg = 0;
    pthread_join(g_hMsgSend, NULL);
    pthread_join(g_hMsgRecv, NULL);
    YKDeleteQue(g_pstTMMsgQ);
#ifdef CONFIG_SORT
    //索引退出
    SortFini();
#endif
}


//一回收发
int I6SendRecv(const char *buf_in,char **buf_out)
{
	int ret = 0;
	int i = 0,j= 0,k =0;
	int len = 0;
	int length = 0;
	int outlen = 0;
	char *pcBuf;
	char buf_gb[1024] = {0x0};
	char file[256] = {0x0};
	FILE *fp = NULL;
	char *p = NULL;
	char acTime[128];
	*buf_out = NULL;
	struct timeval tv;
	fd_set readfds;
	tv.tv_sec = 60;
	tv.tv_usec = 0;
	if (displayFlag == TRUE)
	{
		LOG_RUNLOG_DEBUG("send %s %d\n",buf_in,strlen(buf_in));
	}
	if((ret = I6ConnectHttps() != 0))
	{
#if _TM_TYPE_ == _GYY_I6_
		TMSetLinkStatus(LINK_STATUS_OFFLINE);
#endif
		goto exit;
	}
#ifdef I6_HTTPS
	while( ( ret = ssl_write( &i6_ssl, buf_in, strlen(buf_in) ) ) <= 0 )
	{
		if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
		{
			goto exit;
		}
	}
#else
	while( ( ret = send(i6_server_fd, buf_in, strlen(buf_in), 0)) <= 0 )
	{
		if( ret == -1)
		{
#if _TM_TYPE_ == _GYY_I6_
			TMSetLinkStatus(LINK_STATUS_OFFLINE);
#endif
			goto exit;
		}
	}
#endif
#if _TM_TYPE_ == _GYY_I6_
		TMSetLinkStatus(LINK_STATUS_ONLINE);
#endif

	do
	{
		len = sizeof( g_acTemp ) - 1;
		memset( g_acTemp, 0, sizeof( g_acTemp ) );
#ifdef I6_HTTPS
		ret = ssl_read( &i6_ssl, g_acTemp, len );

		if( ret == POLARSSL_ERR_NET_WANT_READ || ret == POLARSSL_ERR_NET_WANT_WRITE )
			continue;

		if( ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY )
		{
			ret = 0;
			break;
		}
#else
//#ifndef _YK_PC_
		ret = -1;
		if (setsockopt((int)i6_server_fd, SOL_SOCKET, SO_RCVTIMEO, (void *) &tv, sizeof(struct timeval)) < 0)
		{
			break;
		}
		ret = recv(i6_server_fd, g_acTemp, len, 0);
//		FD_ZERO(&readfds);
//		FD_SET(i6_server_fd, &readfds);
//		if (select(i6_server_fd + 1, &readfds, NULL, NULL, &tv) > 0)
//		{
//			ret = recv(i6_server_fd, g_acTemp, len, 0);
//		}
//		else
//		{
//			break;
//		}
//#else
////		ret = recv(i6_server_fd, g_acTemp, len, 0);
//		ret = recv(i6_server_fd, buf_gb, len, 0);
//#endif
#endif
		if( ret < 0 )
		{
			LOG_RUNLOG_DEBUG( "failed\n  ! ssl_read returned %d\n\n", ret );
			break;
		}

		if( ret == 0 )
		{
			LOG_RUNLOG_DEBUG( "\n\nEOF\n\n" );
			break;
		}

		//如果是配置文件則保存 否則內存
		if(g_bI6Config)
		{
			g_bI6Config = 0;
			strcpy(file,I6_CONFIG_PATH_TEMP);
		}
		else if(g_bI6Key)
		{
			g_bI6Key = 0;
			strcpy(file,I6_CARD_PATH_TEMP);
		}
		else if(g_bI6Password)
		{
			g_bI6Password = 0;
			strcpy(file,I6_PASSWORD_PATH_TEMP);
		}
		else if(g_bI6Temp)
		{
			g_bI6Temp = 0;
			strcpy(file,I6_RECV_TEMP);
		}
//#ifdef _YK_PC_
//		u2g(buf_gb,ret,g_acTemp,&outlen);
//#endif
		if (displayFlag == TRUE)
		{
			LOG_RUNLOG_DEBUG("recv %s\n",g_acTemp);
		}
		if(i6_cookie[0] == 0x0)
		{
			if(mystrstr1(g_acTemp,"Set-Cookie: ",&i) != NULL)
			{
				mystrstr(g_acTemp,"Path=/",&j);
				memcpy(&i6_cookie,g_acTemp+i,j-i);
				i6_cookie[j-i] = 0x0;
				i = 0;
			}
		}
		//时间同步
		if(g_bSetTime)
		{
			if(mystrstr1(g_acTemp,"Date: ",&k) != NULL)
			{
				mystrstr(g_acTemp+k,"\r\n",&j);
				memcpy(&acTime,g_acTemp+k,j);
				acTime[j] = 0x0;
				i6_set_datetime(acTime);
				g_bSetTime = 0;
			}
		}

		if (i == 0)
		{
			mystrstr1(g_acTemp,"\r\n\r\n",&i);
			if(file[0] != 0x0)
			{
				fp = fopen(file,"w+");
				fwrite(g_acTemp + i, ret - i, 1, fp);        /*将https主体信息写入文件 */
				fflush(fp);
			}
			else
			{
				*buf_out = g_acTemp + i;
			}
			if(ret < len)
			{
				ret = 0;
				break;
			}
			continue;
		}
		else
		{
			if(file[0] != 0x0 && fp != NULL)
			{
				//正常包不會太大
				fwrite(g_acTemp, 1, ret, fp);        /*将https主体信息写入文件 */
				fflush(fp);        /*每1K时存盘一次 */
			}
//			if(ret < len)
//			{
//				ret = 0;
//				break;
//			}
		}
	}
	while( 1 );

	if(fp != NULL)
	{
		fclose(fp);
	}

exit:
	if(i6_server_fd)
	{
		I6DisConnectHttps();
	}
	return ret;
}
//len = sprintf( (char *) buf, GET_REQUEST ,host_file,host_addr,port);
//#define POST_REQUEST "\
//POST %s HTTP/1.1\r\n\
//Content-Type: application/x-www-form-urlencoded\r\n\
//Content-Length: %d\r\n\
//Host: %s:%d\r\n\
//Connection: Close\r\n\r\n\
//%s\r\n\r\n\
//"
void I6PrePack(char *buf_in,char *msg)
{
	if(i6_cookie[0] == 0x0)
	{
		sprintf(buf_in,PRE_POST_REQUEST,i6_path,strlen(msg),i6_host_addr,i6_port,msg);
	}
	else
	{
		sprintf(buf_in,POST_REQUEST,i6_path,i6_cookie,strlen(msg),i6_host_addr,i6_port,msg);
	}
}

void I6Pack(I6_CMD_EN cmd,void *arg,char *buf_in,int len)
{
	char acMsg[1024] ={0x0};
	struct timeval pstTv;
	switch(cmd)
	{
	case I6_AUTHCODE_REQ:
		strcpy(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>AuthcodeRequest</msgType>\r\n\
</message>\r\n\
");
		I6PrePack(buf_in,acMsg);
		break;
	case I6_AUTHENTICATION_REQ:
		{
		I6_AUTHENTICATION_ST *tmp = (I6_AUTHENTICATION_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>AuthenticationRequest</msgType>\r\n\
    <userName>%s</userName>\r\n\
    <password>%s</password>\r\n\
</message>\r\n\
",tmp->acUserName,tmp->acPassword);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_CONFIG_REQ:
		{
		I6_CONFIG_ST *tmp = (I6_CONFIG_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>ConfigInformRequest</msgType>\r\n\
    <configVersion>%d</configVersion>\r\n\
    <dedicatedNumber>%s</dedicatedNumber>\r\n\
</message>\r\n\
",tmp->iConfigVersion,tmp->acDedicatedNumber);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_CONFIG_DOWNLOAD_REQ:
		{
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>DownloadConfigRequest</msgType>\r\n\
    <dedicatedNumber>%s</dedicatedNumber>\r\n\
</message>\r\n\
",(char *)arg);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_CONFIG_UPDATE_NOTIFY_REQ:
		{
		I6_CONFIG_ST *tmp = (I6_CONFIG_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>ConfigUpdateNotifyRequest</msgType>\r\n\
    <downloadStatus>%d</downloadStatus>\r\n\
    <configVersion>%d</configVersion>\r\n\
</message>\r\n\
",tmp->iDownloadStatus,tmp->iConfigVersion);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_PASSWORD_INFORM_REQ:
		{
		I6_PASSWORD_ST *tmp = (I6_PASSWORD_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>PasswordInformRequest</msgType>\r\n\
    <dedicatedNumber>%s</dedicatedNumber>\r\n\
</message>\r\n\
",tmp->acDedicatedNumber);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_CARD_INFROM_REQ:
		{
		I6_CARD_ST *tmp = (I6_CARD_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>CardInformRequest</msgType>\r\n\
    <version>%d</version>\r\n\
    <dedicatedNumber>%s</dedicatedNumber>\r\n\
</message>\r\n\
",tmp->iConfigVersion,tmp->acDedicatedNumber);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_DOWNLOAD_CARD_REQ:
		{
		I6_CARD_ST *tmp = (I6_CARD_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>DownloadCardRequest</msgType>\r\n\
    <downloadStatus>%d</downloadStatus>\r\n\
</message>\r\n\
",tmp->iDownloadStatus);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_CARD_SWIP_REQ:
		{
		gettimeofday(&pstTv, NULL);
		I6_CARD_LOG_ST *tmp = (I6_CARD_LOG_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>CardSwipRequest</msgType>\r\n\
    <account>%s</account>\r\n\
    <list>\r\n\
    <record cardId=\"%s\" cardNo=\"%s\"  stateCode=\"%s\" swipingCardTime =\"%ld%ld\"/>\r\n\
    </list>\r\n\
</message>\r\n\
",i6device.acSipAccont,tmp->acCardId,tmp->acCardNo,tmp->acStateCode,pstTv.tv_sec,pstTv.tv_usec/1000);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_SOFT_INFORM_REQ:
		{
		I6_SOFT_ST *tmp = (I6_SOFT_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>SoftwareInformRequest</msgType>\r\n\
    <softwareNumber>%s</softwareNumber>\r\n\
    <softwareVersion>%d</softwareVersion >\r\n\
</message>\r\n\
",tmp->acSoftwareNumber,tmp->iConfigVersion);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_SOFT_UPDATE_NOTIFY_REQ:
		{
		I6_SOFT_ST *tmp = (I6_SOFT_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>SoftwareUpdateNotifyRequest</msgType>\r\n\
    <downloadStatus>%d</downloadStatus>\r\n\
    <softwareVersion>%d</softwareVersion>\r\n\
</message>\r\n\
",tmp->iDownloadStatus,tmp->iConfigVersion);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_HEART_REQ:
		{
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
	<msgType>HeartBeat</msgType>\r\n\
    <dedicatedNumber>%s</dedicatedNumber>\r\n\
</message>\r\n\
",i6device.acSipAccont);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_ADVERTISE_UPDATE_REQ:
		{
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
	<msgType>AdvertiseUpdateRequest</msgType>\r\n\
    <dedicatedNumber>%s</dedicatedNumber>\r\n\
</message>\r\n\
",i6device.acSipAccont);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_INFORMATION_UPDATE_REQ:
		{
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
	<msgType>InformationUpdateRequest</msgType>\r\n\
    <dedicatedNumber>%s</dedicatedNumber>\r\n\
</message>\r\n\
",i6device.acSipAccont);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_ADVERTISE_UPDATE_NOTIFY_REQ:
		{
		I6_ADV_ST *tmp = (I6_ADV_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>AdvertiseUpdateNotifyRequest</msgType>\r\n\
    <fileID>%s</fileID>\r\n\
    <downloadStatus>%d</downloadStatus>\r\n\
</message>\r\n\
",tmp->acID,tmp->iState);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_INFORMATION_UPDATE_NOTIFY_REQ:
		{
		I6_ADV_ST *tmp = (I6_ADV_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>InformationUpdateNotifyRequest</msgType>\r\n\
    <fileID>%s</fileID>\r\n\
    <downloadStatus>%d</downloadStatus>\r\n\
</message>\r\n\
",tmp->acID,tmp->iState);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_FILE_UPLOAD_REQ:
		{
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
	<msgType>FileUploadRequest</msgType>\r\n\
    <dedicatedNumber>%s</dedicatedNumber>\r\n\
</message>\r\n\
",i6device.acSipAccont);
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_FILE_UPLOAD_NOTIFY_REQ:
		{
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
	<msgType>FileUploadNotifyRequest</msgType>\r\n\
    <downloadStatus>1</downloadStatus>\r\n\
</message>\r\n\
");
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_REBOOT:
		{
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
    <msgType>RestartNotifyRequest</msgType>\r\n\
    <downloadStatus>1</downloadStatus>\r\n\
</message>\r\n\
");
		I6PrePack(buf_in,acMsg);
		}
		break;
	case I6_ALARM_UPDATE_REQ:
		{
		gettimeofday(&pstTv, NULL);
		TM_I6_ALARM_UPDATE_ST *tmp = (TM_I6_ALARM_UPDATE_ST *)arg;
		sprintf(acMsg,"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<message>\r\n\
	<msgType>AlarmUpdateRequest</msgType>\r\n\
    <dedicatedNumber>%s</dedicatedNumber>\r\n\
    <list>\r\n\
    <alarm alarmNo=\"%d\" stateCode=\"%s\" alarmUpdateTime=\"%ld%ld\"/>\r\n\
    </list>\r\n\
</message>\r\n\
",i6device.acSipAccont,tmp->uiAlarmNo,tmp->auStateCode,pstTv.tv_sec,pstTv.tv_usec/1000);
		I6PrePack(buf_in,acMsg);
		}
		break;
	default:
		break;
	}
}

int I6Unpack(const char *buf_in)
{
	int ret = -1;
	int i = 0;
	int faultCode = 0;
	char acBuf[1024];
	I6_CMD_EN cmd = I6_ERROR;
	if(buf_in == NULL)
	{
		return ret;
	}
	strcpy(acBuf,buf_in);
	if((ret = FindParamFromBuf(buf_in,FAULT_CODE,acBuf)) == 0)
	{
		faultCode = atoi(acBuf);
		if(faultCode != 0)
		{
			I6QuitHttps();
			return faultCode;
		}
	}

	if((ret = FindParamFromBuf(buf_in,MSG_TYPE,acBuf)) != 0)
	{
		return ret;
	}

	if(strcmp(acBuf,MSG_TPYE_AUTHCODE_RES) == 0)
	{
		cmd = I6_AUTHCODE_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_AUTHENTICATION_RES) == 0)
	{
		cmd = I6_AUTHENTICATION_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_CONFIG_RES) == 0)
	{
		cmd = I6_CONFIG_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_CONFIG_UPDATE_NOTIFY_RES) == 0)
	{
		cmd = I6_CONFIG_UPDATE_NOTIFY_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_PASSWORD_INFORM_RES) == 0)
	{
		cmd = I6_PASSWORD_INFORM_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_CARD_INFROM_RES) == 0)
	{
		cmd = I6_CARD_INFROM_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_DOWNLOAD_CARD_RES) == 0)
	{
		cmd = I6_DOWNLOAD_CARD_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_CARD_SWIP_RES) == 0)
	{
		cmd = I6_CARD_SWIP_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_SOFT_INFORM_RES) == 0)
	{
		cmd = I6_SOFT_INFORM_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_SOFT_UPDATE_NOTIFY_RES) == 0)
	{
		cmd = I6_SOFT_UPDATE_NOTIFY_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_ALARM_UPDATE_RES) == 0)
	{
		cmd = I6_ALARM_UPDATE_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_ADVERTISE_UPDATE_RES) == 0)
	{
		cmd = I6_ADVERTISE_UPDATE_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_INFORMATION_UPDATE_RES) == 0)
	{
		cmd = I6_INFORMATION_UPDATE_RES;
	}
	else if(strcmp(acBuf,MSG_TPYE_FILE_UPLOAD_RES) == 0)
	{
		cmd = I6_FILE_UPLOAD_RES;
	}

	switch(cmd)
	{
	case I6_AUTHCODE_RES:
		{
			FindParamFromBuf(buf_in,SAFE_CODE,acBuf);
			strcpy(RandomNum,acBuf);
		}
		break;
	case I6_AUTHENTICATION_RES:
	case I6_CONFIG_UPDATE_NOTIFY_RES:
	case I6_DOWNLOAD_CARD_RES:
	case I6_CARD_SWIP_RES:
	case I6_SOFT_UPDATE_NOTIFY_RES:
	case I6_ALARM_UPDATE_RES:
		{
			return faultCode;
		}
		break;
	case I6_CONFIG_RES:
		{
			if(faultCode == 0)
			{
				FindParamFromBuf(buf_in,UPDRADE_STATUS,acBuf);
				i = atoi(acBuf);
				if(i == 1)
					return 0;
				else
					return 1;
			}
		}
		break;
	case I6_SOFT_INFORM_RES:
		if(faultCode == 0)
		{
			FindParamFromBuf(buf_in,UPDRADE_STATUS,acBuf);
			i = atoi(acBuf);
			if(i == 1)
			{
				FindParamFromBuf(buf_in,DOWNLOAD_URL,acBuf);
				strcpy(i6_soft_url,acBuf);
				return 0;
			}
			else
				return 1;
		}
		break;
	default:
		return -1;
		break;
	}

	return ret;
}

//－－－－－－－－－－－－－－－－－－－具体操作－－－－－－－－－－－－－－－－－－－－－－－－－－－
//鉴权
int I6Authentication()
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	if(g_bAppraise == 1)
	{
		return 0;
	}

	//获取安全随机码
	//组包
	I6Pack(I6_AUTHCODE_REQ,NULL,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	//LOG_RUNLOG_DEBUG("get %s\n",pcBuf);
	if((rst = I6Unpack(pcBuf)) != 0)
	{
		return rst;
	}

	//发起鉴权
	//MD5
	memset(&authentication,0,sizeof(I6_AUTHENTICATION_ST));
	strcpy(authentication.acUserName,i6device.acSipAccont);
	pcBuf = MD5_sign(i6device.acDevicePassword,6);
	sprintf(acBuf,"%s:%s",pcBuf,RandomNum);
	pcBuf = MD5_sign(acBuf,strlen(acBuf));
	strcpy(authentication.acPassword,pcBuf);
	//组包
	I6Pack(I6_AUTHENTICATION_REQ,&authentication,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	//LOG_RUNLOG_DEBUG("get %s\n",pcBuf);
	if((rst = I6Unpack(pcBuf)) != 0)
	{
		return rst;
	}

	g_bAppraise = 1;
	return rst;
}

//配置文件下發
int I6UpdateConfig()
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;
	int version = 0;

	//上報當前配置版本號，视频门禁专用号码
	//獲取當前配置版本號，视频门禁专用号码
	memset(&i6config,0,sizeof(I6_CONFIG_ST));
	i6config.iConfigVersion = i6device.iConfigVersion;
	strcpy(i6config.acDedicatedNumber,i6device.acSipAccont);
	//组包
	I6Pack(I6_CONFIG_REQ,&i6config,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	if((rst = I6Unpack(pcBuf)) != 0)
	{
		if(rst == 1)
		{
			return 0;
		}
		return rst;
	}

	//下載配置信息清求
	//组包
	g_bI6Config = 1;
	I6Pack(I6_CONFIG_DOWNLOAD_REQ,i6config.acDedicatedNumber,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}

	//配置文件版本修改进结构题
	version = I6ConfigVersion(I6_CONFIG_PATH_TEMP);
	if(version > 0)
	{
		i6device.iConfigVersion = version;
		i6config.iConfigVersion = version;
		i6config.iDownloadStatus = 1;
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
		//替换配置文件
		system("cp -rf "I6_CONFIG_PATH_TEMP" "I6_CONFIG_PATH);
		//进行压缩
		system("bzip2 "I6_CONFIG_PATH_TEMP);
		system("mv "I6_CONFIG_PATH_TEMP".bz2 "I6_CONFIG_BZIP);
#else
		system("mv "I6_CONFIG_PATH_TEMP" "I6_CONFIG_PATH);
#endif

#ifdef CONFIG_SORT
		//进行索引重徘
		SortConfigCode();
		SortConfigCornet();
#endif
		TMI6ConfigOpen();
	}
	else
	{
		i6config.iDownloadStatus = 0;
	}

	//上報配置下載情況
	//组包
	I6Pack(I6_CONFIG_UPDATE_NOTIFY_REQ,&i6config,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	if((rst = I6Unpack(pcBuf)) != 0)
	{
		return rst;
	}

	return rst;
}

//密码信息下发
int I6UpdatePassword()
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//上報當前配置版本號，视频门禁专用号码
	//獲取當前配置版本號，视频门禁专用号码
	memset(&i6password,0,sizeof(I6_PASSWORD_ST));
	strcpy(i6password.acDedicatedNumber,i6device.acSipAccont);
	//组包
	I6Pack(I6_PASSWORD_INFORM_REQ,&i6password,acBuf,1024);
	//收发
	g_bI6Password = 1;
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}

	if(I6PwdFaultCode() == 0)
	{
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
	//替换配置文件
	system("cp -rf "I6_PASSWORD_PATH_TEMP" "I6_PASSWORD_PATH);
	//进行压缩
	system("bzip2 "I6_PASSWORD_PATH_TEMP);
	system("mv "I6_PASSWORD_PATH_TEMP".bz2 "I6_PASSWORD_BZIP);
#else
	//替换配置文件
	system("mv "I6_PASSWORD_PATH_TEMP" "I6_PASSWORD_PATH);
#endif
	}
	else
	{
		I6QuitHttps();
	}

	return rst;
}

int I6CardSwip(I6_CARD_LOG_ST *card_log)
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//上報當前配置版本號，视频门禁专用号码
	//獲取當前配置版本號，视频门禁专用号码
	//组包
	I6Pack(I6_CARD_SWIP_REQ,card_log,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	if((rst = I6Unpack(pcBuf)) != 0)
	{
		return rst;
	}

	return rst;
}

int I6AlarmUpdate(void *alarm)
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//组包
	I6Pack(I6_ALARM_UPDATE_REQ,alarm,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	if((rst = I6Unpack(pcBuf)) != 0)
	{
		return rst;
	}

	return rst;
}

void I6AlarmAppUpdate(const char* cState)
{
    TM_I6_ALARM_UPDATE_ST* pstMsg = (TM_I6_ALARM_UPDATE_ST*)malloc(sizeof(TM_I6_ALARM_UPDATE_ST));
    if( NULL == pstMsg )
    {
        return;
    }
    memset(pstMsg, 0x00, sizeof(TM_I6_ALARM_UPDATE_ST));
    pstMsg->uiPrmvType = TM_I6_ALARM_UPDATE;
    pstMsg->uiAlarmNo = TM_ALARM_UPDATERESULT;
    strcpy(pstMsg->auStateCode,cState);
	I6AlarmUpdate(pstMsg);
}

//软件升级下發
int I6UpdateSoft()
{
	char acBuf[1024] = {0x0};
	char acSoftRoute[1024] = {0x0};
	char *pcBuf;
	int rst = 0;
	int i = 0;
	FILE *fp;
	char *p;
	int a=0,b=0,c=0;

	//上報當前配置版本號，视频门禁专用号码
	//獲取當前配置版本號，视频门禁专用号码
	memset(&i6soft,0,sizeof(I6_SOFT_ST));
//#ifdef _YK_PC_
//	strcpy(i6soft.acSoftwareNumber,"BV");
//#else
	strcpy(i6soft.acSoftwareNumber,i6device.acDeviceType);
//#endif
	i6soft.iConfigVersion = i6device.iSoftVersion;
	//组包
	I6Pack(I6_SOFT_INFORM_REQ,&i6soft,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	if((rst = I6Unpack(pcBuf)) != 0)
	{
		if(rst == 1)
		{
			return 0;
		}
		return rst;
	}

	//下載升级包
	strcpy(acBuf,i6_soft_url);
	pcBuf = strrchr(acBuf,'/');
#if 0
	sprintf(acSoftRoute,"./update%s",pcBuf);
	system("mkdir ./update -p");
#endif
	system("busybox mkdir /data/data/com.fsti.ladder/update");
	sprintf(acSoftRoute, "/data/data/com.fsti.ladder/update/%s", NULL == pcBuf?i6_soft_url:pcBuf+1);
	rst = GetFromHttps(i6_soft_url,NULL,NULL,acSoftRoute);
	LOG_RUNLOG_DEBUG("I6UpdateSoft GetFromHttps %d",rst);

	//成功 解压 重启 并退出 上報信息 重启后发
	if(rst == 0)
	{
#if 0
		if(FPCRCForAppImage(acSoftRoute) && FPTryToRunApplication("./update/YK-IDBT.bz2"))
#endif
		if(FPCRCForAppImage(acSoftRoute))
		{
			//成功 上報结果
			i6soft.iDownloadStatus = 1;
			LOG_RUNLOG_DEBUG("FT \tThis update application is ok.!\n");
			TMUpdateAlarmState(TM_ALARM_UPDATERESULT, "0");
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_UPDATE, 0, "/data/fsti_tools/AndroidLadder.apk");

			//加入升级告警结果延时
			LOG_RUNLOG_DEBUG("I6 g_bHaveAlarmUpdate alarm begin");
			I6AlarmAppUpdate("0");
			LOG_RUNLOG_DEBUG("I6 g_bHaveAlarmUpdate alarm end");

			notifyUpdateApp();
			return rst;
		}
		else
		{
			LOG_RUNLOG_DEBUG("FT \tThis update application is error.!");
			TMUpdateAlarmState(TM_ALARM_UPDATERESULT, "1");
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_UPDATE, 1, acSoftRoute);
			remove(acSoftRoute);
		}
	}
	else
	{
		LOG_RUNLOG_DEBUG("FT \tDownload APK failed.!");
		TMUpdateAlarmState(TM_ALARM_UPDATERESULT, "1");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_UPDATE, 1, acSoftRoute);
		remove(acSoftRoute);
	}

	//失败 上報结果
	i6soft.iDownloadStatus = 0;
	//组包
	I6Pack(I6_SOFT_UPDATE_NOTIFY_REQ,&i6soft,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	if((rst = I6Unpack(pcBuf)) != 0)
	{
		if(rst == 1)
		{
			return 0;
		}
		return rst;
	}

	remove(acSoftRoute);

	return rst;
}

int I6CheckConfigSip(const char *pcSip)
{
	char acBuf[1024] = {0x0};
	char acSip[128] = {0x0};
	int rst = 0;
	FILE* fp = 0;
	if((fp = fopen(I6_CONFIG_PATH,"r") ) == NULL)
	{
		return -1;
	}
	if(fread(acBuf,1,1023,fp) <= 0)
	{
		fclose(fp);
		return -1;
	}
	fclose(fp);
	if((rst = FindParamFromBuf(acBuf,"DEDICATED_NUMBER",acSip)) != 0)
	{
		return -1;
	}
	if(strcmp(pcSip,acSip) == 0)
	{
		return 0;
	}
	return -1;
}

int I6KeyFaultCode()
{
	char acBuf[1024] = {0x0};
	char acFaultCode[64] = {0x0};
	int rst = 0;
	FILE* fp = 0;
	if((fp = fopen(I6_CARD_PATH_TEMP,"r") ) == NULL)
	{
		return -1;
	}
	if(fread(acBuf,1,1023,fp) <= 0)
	{
		fclose(fp);
		return -1;
	}
	fclose(fp);
	if((rst = FindParamFromBuf(acBuf,FAULT_CODE,acFaultCode)) != 0)
	{
		return rst;
	}
	rst = atoi(acFaultCode);
	return rst;
}

int I6PwdFaultCode()
{
	char acBuf[1024] = {0x0};
	char acFaultCode[64] = {0x0};
	int rst = 0;
	FILE* fp = 0;
	if((fp = fopen(I6_PASSWORD_PATH_TEMP,"r") ) == NULL)
	{
		return -1;
	}
	if(fread(acBuf,1,1023,fp) <= 0)
	{
		fclose(fp);
		return -1;
	}
	fclose(fp);
	if((rst = FindParamFromBuf(acBuf,FAULT_CODE,acFaultCode)) == 0)
	{
		rst = atoi(acFaultCode);
		return rst;
	}
	if((rst = FindParamFromBuf(acBuf,MSG_TYPE,acFaultCode)) != 0)
	{
		return rst;
	}
	if(strcmp(acFaultCode,"PasswordInformResponse") == 0)
	{
		return 0;
	}
	return -1;
}

	//添加增量到配置文件
void I6CardIncrement()
{
	sd_domnode_t* i6card = sd_domnode_new(NULL, NULL);
	sd_domnode_t* i6cardtemp = sd_domnode_new(NULL, NULL);
	sd_domnode_t* card;
	sd_domnode_t* child;
	sd_domnode_t* cardtemp;
	sd_domnode_t* childtemp;

	if(-1 == sd_domnode_load(i6cardtemp, I6_CARD_PATH_TEMP)){
		LOG_RUNLOG_DEBUG("card temp xml is error\n");
		sd_domnode_delete(i6cardtemp);
		return;
	}
	if(-1 == sd_domnode_load(i6card, I6_CARD_PATH)){
		LOG_RUNLOG_DEBUG("card xml is error\n");
		sd_domnode_delete(i6card);
		return;
	}

	//修改版本号
	cardtemp = sd_domnode_search(i6cardtemp, "version");
	card = sd_domnode_search(i6card, "version");
	strcpy(sd_domnode_attrs_get(card, "seq")->value,sd_domnode_attrs_get(cardtemp, "seq")->value);

	//添加卡 1。黑名单中删除 2。添加
	card = sd_domnode_search(i6card, "cardDelete");
	cardtemp = sd_domnode_search(i6cardtemp, "cardAdd");
	if(cardtemp)
	{
		//查找
		childtemp = sd_domnode_first_child(cardtemp);
		while(childtemp)
		{
			//删除
			card = sd_domnode_search(i6card, "blackCard");
			sd_domnode_del_child_from_child(card,childtemp);
			//下一个
			childtemp = sd_domnode_next_child(cardtemp, childtemp);
		}
	}
	card = sd_domnode_search(i6card, "cardAdd");
	if(cardtemp)
	{
		//查找
		childtemp = sd_domnode_first_child(cardtemp);
		while(childtemp)
		{
			//添加
			sd_domnode_add_child_to_child(card,childtemp);
			//下一个
			childtemp = sd_domnode_next_child(cardtemp, childtemp);
		}
	}

	//添加黑名单 1。删除有效卡 2。添加黑名单
	card = sd_domnode_search(i6card, "cardAdd");
	cardtemp = sd_domnode_search(i6cardtemp, "blackCard");
	if(cardtemp)
	{
		//查找
		childtemp = sd_domnode_first_child(cardtemp);
		while(childtemp)
		{
			//删除
			card = sd_domnode_search(i6card, "cardAdd");
			sd_domnode_del_child_from_child(card,childtemp);
			//下一个
			childtemp = sd_domnode_next_child(cardtemp, childtemp);
		}
	}
	card = sd_domnode_search(i6card, "blackCard");
	if(cardtemp)
	{
		//查找
		childtemp = sd_domnode_first_child(cardtemp);
		while(childtemp)
		{
			//添加
			sd_domnode_add_child_to_child(card,childtemp);
			//下一个
			childtemp = sd_domnode_next_child(cardtemp, childtemp);
		}
	}

	//删除黑白名单中所有此卡相关
	cardtemp = sd_domnode_search(i6cardtemp, "cardDelete");
	if(cardtemp)
	{
		//查找
		childtemp = sd_domnode_first_child(cardtemp);
		while(childtemp)
		{
			//删除
			card = sd_domnode_search(i6card, "cardAdd");
			sd_domnode_del_child_from_child(card,childtemp);
			card = sd_domnode_search(i6card, "blackCard");
			sd_domnode_del_child_from_child(card,childtemp);
			//下一个
			childtemp = sd_domnode_next_child(cardtemp, childtemp);
		}
	}

	sd_domnode_store(i6card,I6_CARD_PATH);
	sd_domnode_delete(i6card);
	sd_domnode_delete(i6cardtemp);
}

//卡信息下發
int I6UpdateCard()
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//上報當前配置版本號，视频门禁专用号码
	//獲取當前配置版本號，视频门禁专用号码
	memset(&i6card,0,sizeof(I6_CARD_ST));
	strcpy(i6card.acDedicatedNumber,i6device.acSipAccont);
	i6card.iConfigVersion = i6device.iKeyVersion;
	//组包
	I6Pack(I6_CARD_INFROM_REQ,&i6card,acBuf,1024);
	//收发
	g_bI6Key = 1;
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	if((rst = I6KeyFaultCode()) == 0)
	{
		//添加增量到配置文件
		I6CardIncrement();
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
		//替换配置文件
		system("cp -rf "I6_CARD_PATH" "I6_CARD_PATH_TEMP);
		//进行压缩
		system("bzip2 "I6_CARD_PATH_TEMP);
		system("mv "I6_CARD_PATH_TEMP".bz2 "I6_CARD_BZIP);
#endif
		//组包
		i6card.iDownloadStatus = 1;
	}
	else
	{
		//组包
		i6card.iDownloadStatus = 0;
		I6QuitHttps();
		return rst;
	}
	//版本号更新
	i6device.iKeyVersion = TMI6GetCardVersion(I6_CARD_PATH);

	//发送下載结果
	I6Pack(I6_DOWNLOAD_CARD_REQ,&i6card,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	if((rst = I6Unpack(pcBuf)) != 0)
	{
		return rst;
	}

	return rst;
}

int I6InformationUpdateNotify(I6_INF_ST *arg)
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//组包
	I6Pack(I6_INFORMATION_UPDATE_NOTIFY_REQ,arg,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	return rst;
}

void I6InformationUnpack()
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t *search,*information,*fileInfo,*fault;
	char acBuf[64] = {0x0};
	char acFileName[64] = {0x0};
	char *pcBuf;
	int rst = 0;
	int type = 0;
	I6_INF_ST i6_adv;
	TM_JNI_ADV_INFO_ST *adv_info;
	FILE *fp;

	if(-1 == sd_domnode_load(document, I6_RECV_TEMP)){
		sd_domnode_delete(document);
		return;
	}
	fault = sd_domnode_search(document,"faultCode");
	if(fault && fault->value)
	{
		if(atoi(fault->value) != 0)
		{
			sd_domnode_delete(document);
			return;
		}
	}
	search = sd_domnode_search(document,"informations");
	information = sd_domnode_first_child(search);
	while(information)
	{
		memset(&i6_adv,0,sizeof(I6_INF_ST));
		adv_info = (TM_JNI_ADV_INFO_ST *)malloc(sizeof(TM_JNI_ADV_INFO_ST));
		memset(adv_info,0,sizeof(TM_JNI_ADV_INFO_ST));
		adv_info->uiPrmvType = TM_I6_ANN_DOWNLOAD;
		fileInfo = sd_domnode_attrs_get(information,"id");
		if(fileInfo && fileInfo->value )
		{
			strcpy(i6_adv.acID,fileInfo->value);
			strcat(adv_info->auAdvInfo,fileInfo->value);
			strcat(adv_info->auAdvInfo,"&");
		}
		fileInfo = sd_domnode_attrs_get(information,"type");
		if(fileInfo && fileInfo->value )
		{
			type = atoi(fileInfo->value);
		}

		acBuf[0] = 0x0;
		switch(type)
		{
		case 1:
			system("busybox mkdir -p "LOG_FLASH_PATH"/ann/ann_system");
			strcpy(acBuf,"/ann/ann_system/");
			break;
		case 2:
			system("busybox mkdir -p "LOG_FLASH_PATH"/ann/ann_property");
			strcpy(acBuf,"/ann/ann_property/");
			break;
		case 3:
			break;
		case 4:
			system("busybox mkdir -p "LOG_FLASH_PATH"/ann/ann_government");
			strcpy(acBuf,"/ann/ann_government/");
			break;
		case 5:
			system("busybox mkdir -p "LOG_FLASH_PATH"/ann/ann_live");
			strcpy(acBuf,"/ann/ann_live/");
			break;
		case 6:
			system("busybox mkdir -p "LOG_FLASH_PATH"/ann/ann_other");
			strcpy(acBuf,"/ann/ann_other/");
			break;
		default:
			break;
		}

		fileInfo = sd_domnode_attrs_get(information,"content");
		if(fileInfo && fileInfo->value && acBuf[0] != 0x0)
		{
			sprintf(acFileName,"%s%s%s.txt",LOG_FLASH_PATH,acBuf,i6_adv.acID);
			fp = fopen(acFileName,"w+");
			fwrite(fileInfo->value,1,strlen(fileInfo->value),fp);
			fclose(fp);
			strcat(adv_info->auAdvInfo,acFileName);
			strcat(adv_info->auAdvInfo,"&");
			i6_adv.iState = 1;
		}

		fileInfo = sd_domnode_attrs_get(information,"endtime");
		if(fileInfo && fileInfo->value )
		{
			strcat(adv_info->auAdvInfo,fileInfo->value);
			strcat(adv_info->auAdvInfo,"&");
		}

		I6InformationUpdateNotify(&i6_adv);

		//通知JAVA 或者 写入数据库
		if(i6_adv.iState == 1)
		{
			TMMsgCntProcessJniAdvInfo(adv_info);
		}
		else
		{
			free(adv_info);
		}
		information = sd_domnode_next_child(search,information);
	}

	sd_domnode_delete(document);
	return;
}

int I6InformationUpdate()
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//组包
	I6Pack(I6_INFORMATION_UPDATE_REQ,NULL,acBuf,1024);
	//收发
	g_bI6Temp = 1;
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	I6InformationUnpack();

	return rst;
}

int I6AdvertiseUpdateNotify(I6_ADV_ST *arg)
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//组包
	I6Pack(I6_ADVERTISE_UPDATE_NOTIFY_REQ,arg,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	return rst;
}

void I6AdvertiseUnpack()
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t *search,*advertises,*node,*fileInfo,*fault;
	char acBuf[64] = {0x0};
	char acUrl[256] = {0x0};
	char acFileName[128] = {0x0};
	char *pcBuf;
	int rst = 0;
	int type = 0;
	I6_ADV_ST i6_adv;
	TM_JNI_ADV_INFO_ST *adv_info;

	if(-1 == sd_domnode_load(document, I6_RECV_TEMP)){
		sd_domnode_delete(document);
		return;
	}
	fault = sd_domnode_search(document,"faultCode");
	if(fault && fault->value)
	{
		if(atoi(fault->value) != 0)
		{
			sd_domnode_delete(document);
			return;
		}
	}
	search = sd_domnode_search(document,"advertises");
	advertises = sd_domnode_first_child(search);
	while(advertises)
	{
		memset(&i6_adv,0,sizeof(I6_ADV_ST));
		node = sd_domnode_attrs_get(advertises,"id");
		if(node && node->value )
		{
			strcpy(i6_adv.acID,node->value);
		}

		node = sd_domnode_attrs_get(advertises,"type");
		if(node && node->value)
		{
			type = atoi(node->value);
		}

		i6_adv.iState = 1;
		fileInfo = sd_domnode_first_child(advertises);
		while(fileInfo)
		{
			acBuf[0] = 0x0;
			node = sd_domnode_attrs_get(fileInfo,"url");
			if(node && node->value )
			{
				switch(type)
				{
				case 1:
					system("busybox mkdir -p "LOG_FLASH_PATH"/adv/adv_call");
					strcpy(acBuf,"/adv/adv_call/");
					break;
				case 2:
					system("busybox mkdir -p "LOG_FLASH_PATH"/adv/adv_standby");
					strcpy(acBuf,"/adv/adv_standby/");
					break;
				default:
					break;
				}

				if(acBuf[0] != 0x0)
				{
					adv_info = (TM_JNI_ADV_INFO_ST *)malloc(sizeof(TM_JNI_ADV_INFO_ST));
					memset(adv_info,0,sizeof(TM_JNI_ADV_INFO_ST));
					adv_info->uiPrmvType = TM_I6_ADV_DOWNLOAD;
					strcat(adv_info->auAdvInfo,i6_adv.acID);
					strcat(adv_info->auAdvInfo,"&");

					strcpy(acFileName,node->value);
					pcBuf = strrchr(acFileName,'/');
					sprintf(acUrl,"%s%s%s",LOG_FLASH_PATH,acBuf,pcBuf+1);
					if(GetFromHttps(node->value,NULL,NULL,acUrl)==0)
					{
						strcat(adv_info->auAdvInfo,GetDownloadFileName());
						strcat(adv_info->auAdvInfo,"&");
						i6_adv.iState = 1;
					}
					else
					{
						i6_adv.iState = 0;
					}

					//通知JAVA 或者 写入数据库
					if(i6_adv.iState == 1)
					{
						node = sd_domnode_attrs_get(advertises,"endtime");
						if(node && node->value )
						{
							strcat(adv_info->auAdvInfo,node->value);
							strcat(adv_info->auAdvInfo,"&");
						}
						TMMsgCntProcessJniAdvInfo(adv_info);
					}
					else
					{
						free(adv_info);
					}
				}
			}

			fileInfo = sd_domnode_next_child(advertises,fileInfo);
		}

		I6AdvertiseUpdateNotify(&i6_adv);

		advertises = sd_domnode_next_child(search,advertises);
	}

	sd_domnode_delete(document);
	return;
}

int I6AdvertiseUpdate()
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//组包
	I6Pack(I6_ADVERTISE_UPDATE_REQ,NULL,acBuf,1024);
	//收发
	g_bI6Temp = 1;
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	I6AdvertiseUnpack();

	return rst;
}

void I6FileUploadUnpack()
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t *search,*child,*fileInfo,*fault;
	char acUserName[256], acPassWord[256], pacHost[256]; int iPort=21;
	char *p;
	char FileName[128];
	char buf[128];

	if(-1 == sd_domnode_load(document, I6_RECV_TEMP)){
		sd_domnode_delete(document);
		return;
	}

	fault = sd_domnode_search(document,"faultCode");
	if(fault && fault->value)
	{
		if(atoi(fault->value) != 0)
		{
			sd_domnode_delete(document);
			return;
		}
	}

    TMQueryFtpInfo(pacHost, &iPort, acUserName, acPassWord);

	search = sd_domnode_search(document,"log");
	child = sd_domnode_first_child(search);
	if(child && child->value)
	{
		LOGUploadResPrepross();
		FPPutLog(pacHost, iPort, acUserName, acPassWord, LG_LOCAL_PATH, child->value, NULL, NULL,FP_TRANSFER_NOWAIT);
	}

	search = sd_domnode_search(document,"config");
	child = sd_domnode_first_child(search);
	if(child && child->value )
	{
		sprintf(buf,"%s/I6Config.xml",child->value);
		FPPut(pacHost, iPort, acUserName, acPassWord, I6_CONFIG_PATH, buf, NULL, NULL,FP_TRANSFER_NOWAIT);
		sprintf(buf,"%s/I6Password.xml",child->value);
		FPPut(pacHost, iPort, acUserName, acPassWord, I6_PASSWORD_PATH, buf, NULL, NULL,FP_TRANSFER_NOWAIT);
		sprintf(buf,"%s/I6Card.xml",child->value);
		FPPut(pacHost, iPort, acUserName, acPassWord, I6_CARD_PATH, buf, NULL, NULL,FP_TRANSFER_NOWAIT);
	}

	sd_domnode_delete(document);
	return;
}

int I6FileUpload()
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//组包
	I6Pack(I6_FILE_UPLOAD_REQ,NULL,acBuf,1024);
	//收发
	g_bI6Temp = 1;
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	//解包
	I6FileUploadUnpack();
	//组包
	I6Pack(I6_FILE_UPLOAD_NOTIFY_REQ,NULL,acBuf,1024);
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}

	return rst;
}

int I6Reboot()
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;

	//组包
	I6Pack(I6_REBOOT,NULL,acBuf,1024);
	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}

	return rst;
}

int I6Heart()
{
	char acBuf[1024] = {0x0};
	char *pcBuf;
	int rst = 0;
	int i=0,j=0,k=0,l=0,o=0,p=0,q=0,r=0;

	if(g_bBoot == 0)
	{
		return -1;
	}
	if(NMNetTest() != 0)
	{
		return -1;
	}

	sprintf(acBuf,PRE_POST_REQUEST,POST_HEART,strlen(i6device.acSipAccont),
			i6_host_addr,i6_port,i6device.acSipAccont);

	//收发
	if((rst = I6SendRecv(acBuf,&pcBuf)) != 0)
	{
		return rst;
	}
	I6QuitHttps();
	//解包
	if(pcBuf == NULL)
	{
		return -1;
	}
	LOG_RUNLOG_DEBUG("I6 I6Heart %s\n",pcBuf);
	strcpy(acBuf,pcBuf);
	if(strlen(acBuf) > 20)
	{
		return -1;
	}
	pcBuf = strtok(acBuf,"&");
	if(pcBuf)
	{
		i = atoi(pcBuf);
	}
	pcBuf = strtok(NULL,"&");
	if(pcBuf)
	{
		j = atoi(pcBuf);
	}
	pcBuf = strtok(NULL,"&");
	if(pcBuf)
	{
		k = atoi(pcBuf);
	}
	pcBuf = strtok(NULL,"&");
	if(pcBuf)
	{
		l = atoi(pcBuf);
	}
	pcBuf = strtok(NULL,"&");
	if(pcBuf)
	{
		o = atoi(pcBuf);
	}
	pcBuf = strtok(NULL,"&");
	if(pcBuf)
	{
		p = atoi(pcBuf);
	}
	pcBuf = strtok(NULL,"&");
	if(pcBuf)
	{
		q = atoi(pcBuf);
	}
	pcBuf = strtok(NULL,"&");
	if(pcBuf)
	{
		r = atoi(pcBuf);
	}

	if(i || j || k || l|| o || p|| q ||r)
	{
		LOG_RUNLOG_DEBUG("I6 is start\n");

		if(i == 1)
		{
			if(I6Authentication() != 0)
			{
				LOG_RUNLOG_DEBUG("I6Authentication error \n");
				return -1;
			}
			if(I6UpdateConfig() != 0)
			{
				LOG_RUNLOG_DEBUG("I6UpdateConfig error \n");
			}
			LOG_RUNLOG_DEBUG("I6 I6Heart I6UpdateConfig OK\n");
		}

		if(k == 1)
		{
			if(I6Authentication() != 0)
			{
				LOG_RUNLOG_DEBUG("I6Authentication error \n");
				return -1;
			}
			if(I6UpdatePassword() != 0)
			{
				LOG_RUNLOG_DEBUG("I6UpdatePassword error \n");
			}
			LOG_RUNLOG_DEBUG("I6 I6Heart I6UpdatePassword OK\n");
		}

		if(j == 1)
		{
			if(I6Authentication() != 0)
			{
				LOG_RUNLOG_DEBUG("I6Authentication error \n");
				return -1;
			}
			if(I6UpdateSoft() != 0)
			{
				LOG_RUNLOG_DEBUG("I6UpdateSoft error \n");
			}
			LOG_RUNLOG_DEBUG("I6 I6Heart I6UpdateSoft OK\n");
		}

		if(l == 1)
		{
			if(I6Authentication() != 0)
			{
				LOG_RUNLOG_DEBUG("I6Authentication error \n");
				return -1;
			}
			if(I6UpdateCard() != 0)
			{
				LOG_RUNLOG_DEBUG("I6UpdateCard error \n");
			}
			LOG_RUNLOG_DEBUG("I6 I6Heart I6UpdateCard OK\n");
		}

		if(o == 1)
		{
			if(I6Authentication() != 0)
			{
				printf("I6Authentication error \n");
				return -1;
			}
			if(I6InformationUpdate() != 0)
			{
				printf("I6InformationUpdate error \n");
			}
			LOG_RUNLOG_DEBUG("I6 I6Heart I6InformationUpdate OK\n");
		}
		if(p == 1)
		{
			if(I6Authentication() != 0)
			{
				printf("I6Authentication error \n");
				return -1;
			}
			if(I6AdvertiseUpdate() != 0)
			{
				printf("I6AdvertiseUpdate error \n");
			}
			LOG_RUNLOG_DEBUG("I6 I6Heart I6AdvertiseUpdate OK\n");
		}

		if(q == 1)
		{
			if(I6Authentication() != 0)
			{
				LOG_RUNLOG_DEBUG("I6Authentication error \n");
				return -1;
			}
			if(I6FileUpload() != 0)
			{
				LOG_RUNLOG_DEBUG("I6FileUpload error \n");
			}
			LOG_RUNLOG_DEBUG("I6 I6Heart I6FileUpload OK\n");
		}

		if(r == 1)
		{
			if(I6Authentication() == 0)
			{
				if(I6Reboot() != 0)
				{
					LOG_RUNLOG_DEBUG("I6Reboot error \n");
				}
			}
			else
			{
				LOG_RUNLOG_DEBUG("I6Authentication error \n");
			}
			LOG_RUNLOG_DEBUG("I6 I6Heart I6Reboot OK\n");
			//ExitApp();
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_PLATFORM_REBOOT, 0, "");
			IDBTMsgSendABReboot(REMOTE_REBOOT);
			//退出函数
		}

		I6QuitHttps();
	}
	return 0;
}

#endif
