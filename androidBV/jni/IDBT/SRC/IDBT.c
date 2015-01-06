/*********************************************************************
*  Copyright (C), 2001 - 2012, �����ʿ�ͨ�ż������޹�˾.
*  File			:
*  Author		:xuqd
*  Version 		:
*  Date			:
*  Description	:
*  History		: ���¼�¼ .
*            	  <�޸�����>  <�޸�ʱ��>  <�޸�����>
**********************************************************************/
//ͷ�ļ�����
#include <YKSystem.h> 
#include <YKApi.h>
#include <IDBT.h>
#include <IDBTCfg.h>
#include <FPInterface.h>
//#include <NetworkMonitor.h>
//#include <SMEventHndl.h>
#include <LOGIntfLayer.h>
#include <LPIntfLayer.h>
#include <LANIntfLayer.h>
#include <CCTask.h>
//#include <WDTask.h>
#include <TMInterface.h>
#include "Snapshot.h"

#ifdef _I6_CC_
#include "I6CCTask.h"
#else
#include "CCTask.h"
#endif

#if _TM_TYPE_ == _YK069_
#include <TerminalManage.h>
#else
#include <TRManage.h>
#endif

//add by xuqd
#include "IDBTIntfLayer.h"

/*#ifdef _YK_IMX27_AV_
#include <SMEventHndlImx27.h>
#else
#include <SMEventHndl.h>
#endif*/

#include <string.h>
#include <jni.h>
#include <android/log.h>

#ifdef _YK_XT8126_BV_
#include <XTIntfLayer.h>
#endif

//CCģ���ͨ����Ϣ����ָ��
extern YK_MSG_QUE_ST* g_pstCCMsgQ;

#ifdef _YK_XT8126_BV_
extern YK_MSG_QUE_ST*  g_pstXTIntfMsgQ;
#endif

//�궨��
#define IDBT_PROMPT 		"IDBT> "
#define IDBT_INPUT_ERROR	"input cmd error!"
#define IDBT_MAX_SIZE   	512
#define IDBT_SIZE_32    	32
#define HELP_CMD			"help"
#define QUIT_CMD			"quit"
#define VERSION_CMD			"version"
#define FALSE 0
#define TRUE 1

#define _SUPPORT_QSA9000_

//#define LOG_RUNLOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, "com.fsti.ladder", __VA_ARGS__)

//�ṹ�嶨��
typedef BOOL (*CmdFunc)(const char* cmd);
typedef struct __tagDoorBellCmd{
    char*       pcCmd;
    CmdFunc 	ProcessFunc;
}IDBTCmd;

typedef struct __tagThreadData{
    BOOL        bHasReceived;
    char        acCmd[IDBT_MAX_SIZE];
    YKHandle    Lock;
    YKHandle    Thread;
}INPUT_THREAD_DATA_ST;

//��������
int g_iUpdateConfigFlag = FALSE;
int g_iRunSystemFlag 	= TRUE;
int g_iRunMCTaskFlag 	= FALSE;
int g_iRunSMTaskFlag 	= FALSE;
int g_iRunNMTaskFlag 	= FALSE;
int g_iRunLPTaskFlag 	= FALSE;
int g_iRunCCTaskFlag 	= FALSE;
int g_iRunLGTaskFlag 	= FALSE;
int g_iRunTMTaskFlag 	= FALSE;
int g_iRunI6TaskFlag 	= FALSE;
int g_iRunIPTaskFlag	= FALSE;
int g_iRunTITaskFlag 	= FALSE;
int g_iRunLANTaskFlag	= FALSE;
int g_iRunXTTaskFlag	= FALSE;
int g_iRunWDTaskFlag 	= FALSE;
int g_iRunIOTaskFlag	= FALSE;
int g_iRunATTaskFlag	= FALSE;
int g_iRunABTaskFlag 	= FALSE;
int g_iRunSSTaskFlag    = FALSE;


INPUT_THREAD_DATA_ST *g_pstInputData = NULL;
YKHandle g_TimerThreadId;

extern int user_fd;
extern int g_i8126IoFd;


int LPProcessCmdFunc(const char *cmd)
{
	LOG_RUNLOG_DEBUG("ID the cmd is sended to LP from terminal,the content is:%s", cmd);
	LPProcessCommand(cmd);
	return SUCCESS;
}

int TMProcessCmdFunc(const char *cmd)
{
	LOG_RUNLOG_DEBUG("ID the cmd is sended to TM,the content is:%s", cmd);
	//return TRProcessCommand(cmd);;
}

int SMProcessCmdFunc(const char *cmd)
{
	LOG_RUNLOG_DEBUG("ID the cmd is sended to CC from SM,the content is:%s", cmd);
	//return SMProcessCommand(cmd);
}

int XTProcessCmdFunc(const char *cmd)
{
	LOG_RUNLOG_DEBUG("ID the cmd is sended to SM from XT,the content is:%s", cmd);
	return SUCCESS;
}

int NMProcessCmdFunc(const char *cmd)
{
	LOG_RUNLOG_DEBUG("ID the cmd is sended to NM,the content is:%s", cmd);
	//return NMProcessCommand(cmd);
}

int ATProcessCmdFunc(const char *cmd)
{
	LOG_RUNLOG_DEBUG("ID the cmd is sended to AT,the content is:%s", cmd);
	//return ATProcessCommand(cmd);
}

int VOLStringToDecimal(char *pcString)
{
	int iVol = -255;
	sscanf(pcString,"%d",&iVol);
	return iVol;
}

/**************************************************************
 *@brief		VLProcessCommand ������֧��.
 *@param[in]:	const char *cmd ����
 *@return		RST_OK �ɹ�  RST_ERR ʧ��
**************************************************************/
int VLProcessCommand(const char *cmd)
{
	char acVolSet[128]={0x0};
	char* argv[6];
	int i=0;
	int iChoice = -1;
	int iVol;

	for(i=0;i<5;i++)
	{
		argv[i] = NULL;
	}

	argv[0] = strtok(cmd," \0");
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
		argv[i+1] = strtok(NULL," \0");
	}
	if(strcmp(argv[0],"help") == 0)
	{
		iChoice = VOL_COMMAND_HELP;
	}
	else if(strcmp(argv[0],"input") == 0)
	{
		iChoice = VOL_COMMAND_INPUT;
	}
	else if(strcmp(argv[0],"output") == 0)
	{
		iChoice = VOL_COMMAND_OUTPUT;
	}
	else if(strcmp(argv[0],"local") == 0)       //��������
	{
	    iChoice = VOL_COMMAND_OUTPUT_LOCAL_WAV;
	}
	
	else
	{
		LOG_RUNLOG_DEBUG("===================================================================\n");
		LOG_RUNLOG_DEBUG("input error command ,you can input \"VOL help\" to look for command\n");
		LOG_RUNLOG_DEBUG("===================================================================\n");
	}

	switch(iChoice)
	{
	case VOL_COMMAND_HELP:
		LOG_RUNLOG_DEBUG("===================================================");
		LOG_RUNLOG_DEBUG("---------------help----------------look for command");
		LOG_RUNLOG_DEBUG("---------------input----------------set inputvolume");
		LOG_RUNLOG_DEBUG("---------------output-------------set outputvolumue");
		LOG_RUNLOG_DEBUG("---------------local-------------set OutputLocalWavVolumue");
		LOG_RUNLOG_DEBUG("===================================================");
		break;

	case VOL_COMMAND_INPUT:
		if(argv[1] == NULL)
		{
			LOG_RUNLOG_DEBUG("ID the num is null !!!");
			break;
		}
		iVol = VOLStringToDecimal(argv[1]);
		if(iVol >= -30 && iVol <= 30)
		{
			g_stIdbtCfg.iInputVol = iVol;
			SaveIdbtConfig();
			sprintf(acVolSet,"busybox echo %d > /proc/asound/adda300_LADV",g_stIdbtCfg.iInputVol);
			system(acVolSet);
			LOG_RUNLOG_DEBUG("ID InputVolume now is ** %d ** dB",g_stIdbtCfg.iInputVol);
		}
		else
		{
			LOG_RUNLOG_DEBUG("ID the num is error !");
		}
		break;

	case VOL_COMMAND_OUTPUT:
		if(argv[1] == NULL)
		{
			LOG_RUNLOG_DEBUG("ID the num is null !");
			break;
		}
		iVol = VOLStringToDecimal(argv[1]);
		if(iVol >= -40 && iVol <= 6)
		{
			g_stIdbtCfg.iOutputCallVol = iVol;
			SaveIdbtConfig();
			sprintf(acVolSet,"busybox echo %d > /proc/asound/adda300_SPV",g_stIdbtCfg.iOutputCallVol);
			system(acVolSet);
			LOG_RUNLOG_DEBUG("ID OutputVolume now is ** %d ** dB",g_stIdbtCfg.iOutputCallVol);
		}
		else
		{
			LOG_RUNLOG_DEBUG("ID the num is error !");
		}
		break;
    case VOL_COMMAND_OUTPUT_LOCAL_WAV:
        if(argv[1] == NULL)
		{
        	LOG_RUNLOG_DEBUG("ID the num is null !");
			break;
		}
		iVol = VOLStringToDecimal(argv[1]);
		if(iVol >= -40 && iVol <= 6)
		{
			g_stIdbtCfg.iOutputPlayWavVol = iVol;
			SaveIdbtConfig();
			LOG_RUNLOG_DEBUG("ID OutputLocalWavVol now is ** %d ** dB",g_stIdbtCfg.iOutputPlayWavVol);
		}
		else
		{
			LOG_RUNLOG_DEBUG("ID the num is error !");
		}
		break;
	default:
		break;
	}

	return RST_OK;
}

int VLProcessCmdFunc(const char *cmd)
{
	LOG_RUNLOG_DEBUG("ID TEST:the cmd is sended to VOL,the content is:%s", cmd);
	LOG_RUNLOG_DEBUG("ID ================================");
	LOG_RUNLOG_DEBUG("ID InputVolume is from -30 ~ +30 dB");
	LOG_RUNLOG_DEBUG("ID OutputVolume is from -40 ~ +6 dB");
	LOG_RUNLOG_DEBUG("ID InputVolume now is ** %d ** dB",g_stIdbtCfg.iInputVol);
	LOG_RUNLOG_DEBUG("ID OutputVolume now is ** %d ** dB",g_stIdbtCfg.iOutputCallVol);
	LOG_RUNLOG_DEBUG("ID OutputLocalWavVol now is ** %d ** dB",g_stIdbtCfg.iOutputPlayWavVol);
	LOG_RUNLOG_DEBUG("ID ================================");
	return VLProcessCommand(cmd);
}

int SSProcessCmdFunc(const char *cmd)
{
	return(system(cmd));
}

//******************************************************************
//�����ʽʵ��
//�����һ��Ϊ����ģ�������ڶ������Ϊ����ģ��������������
//LPC call sip:73425645
//SMC call 1001
//******************************************************************
static  IDBTCmd g_InputCmds[]={
    {"LPC",     LPProcessCmdFunc},
    {"FPC",     FTPProcessCmdFunc},
    {"TMC",   	TMProcessCmdFunc},
    {"SMC",		SMProcessCmdFunc},
    {"XTC",		XTProcessCmdFunc},
    {"NMC",		NMProcessCmdFunc},
    {"VOL",		VLProcessCmdFunc},
    {"ATC",		ATProcessCmdFunc},
    {"SYS",		SSProcessCmdFunc},
    {HELP_CMD,	NULL},
    {QUIT_CMD,	NULL},
    {VERSION_CMD,	NULL},
};

/********************************************************************
*Function	:	StripBlanks
*Input		:	input ���ַ�ָ��ָ�����һ���ַ���
*Input			��������"LPC call sip:12345678"
*Output		:	SUCCESS������ɹ�
*Output			FAILURE������ʧ��
*Description:	�������ַ������д���ʹ�ַ�����ͷβ�����ֿո�
*Others		:
*********************************************************************/
static char *StripBlanks(char *input)
{
    char *iptr;
    /* Find first non-blank */
    while(*input && isspace(*input)) ++input;

    /* Find last non-blank */
    iptr=input+strlen(input);
    if (iptr > input) {
        while(isspace(*--iptr));
        *(iptr+1)='\0';
    }

    return input;
}

void SigHandle(int sig)
{
	////LOG_RUNLOG_DEBUG("i am here!!!!\n");
	pthread_exit(NULL);
}

/********************************************************************
*Function	:	IDBTInputThread
*Input		:	arg ָ��ָ���̵߳��������
*Output		:	NULL
*Description:	��ȡ����̨������������ŵ�ָ����λ��
*Others		:
*********************************************************************/
static void* IDBTInputThread(void* arg)
{
	INPUT_THREAD_DATA_ST* pstInputThreadData = (INPUT_THREAD_DATA_ST* )arg;
    char acCmd[IDBT_MAX_SIZE]={0x0};

	signal(SIGUNUSED, SigHandle);
    while(1)
    {
    	if(fgets(acCmd, IDBT_MAX_SIZE, stdin) != NULL)
    	{
            YKLockMutex(g_pstInputData->Lock);
            g_pstInputData->bHasReceived = TRUE;
            strcpy(g_pstInputData->acCmd, acCmd);
            YKUnLockMutex(g_pstInputData->Lock);
    	}
    }
    return NULL;
}

/********************************************************************
*Function	:	IDTBTimerThread
*Input		:	arg ָ��ָ���̵߳��������
*Output		:	NULL
*Description:	IDBT�Ķ�ʱ�������߳�
*Others		:
*********************************************************************/
static void IDBTTimerThread(void *arg)
{
	while(g_iRunTITaskFlag == TRUE)
	{
		YKTimerTask();
	}
}

///********************************************************************
//*Function	:	SaveIdbtConfig
//*Input		:	NULL
//*Output		:	NULL
//*Description:	��ȡ������С
//*Others		:
//*********************************************************************/
//int SaveIdbtConfig(void)
//{
//	FILE *pCfgFd = NULL;
//	char *pcHelp = "\
//	--------------------help info--------------------\n\
//	log_upload_mode(1-clock 2-cycle)\n\
//	log_upload_cycle_time(unit:hour)\n\
//	network_type(1-DHCP 2-PPPOE 3-STATIC)\n\
//	--------------------help info--------------------\n\
//	";
//
//	pCfgFd=fopen(IDBT_CONFIG_PATH,"w+");
//	if(pCfgFd != NULL)
//	{
//		fprintf(pCfgFd,"sn=%s\n",g_stIdbtCfg.acSn);
//		fprintf(pCfgFd,"log_upload_mode=%d\n",g_stIdbtCfg.iLogUploadMode);
//		fprintf(pCfgFd,"log_upload_cycle_time=%d\n",g_stIdbtCfg.iLogUploadCycleTime);
//		fprintf(pCfgFd,"management_password=%s\n",g_stIdbtCfg.acManagementPassword);
//		fprintf(pCfgFd,"opendoor_password=%s\n",g_stIdbtCfg.acOpendoorPassword);
//		//������ز���
//		fprintf(pCfgFd,"network_type=%s\n",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
//		fprintf(pCfgFd,"ip_addr=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
//		fprintf(pCfgFd,"net_mask=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
//		fprintf(pCfgFd,"gateway=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
//		fprintf(pCfgFd,"dns_server=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
//		fprintf(pCfgFd,"mac_addr=%s\n", g_stIdbtCfg.stNetWorkInfo.acMac);
//
//		fprintf(pCfgFd,"pppoe_user=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
//		fprintf(pCfgFd,"pppoe_passwd=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);
//
//		fprintf(pCfgFd,"dest_ip_1=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP1);
//		fprintf(pCfgFd,"port1=%d\n", g_stIdbtCfg.stDestIpInfo.port1);
//		fprintf(pCfgFd,"dest_ip_2=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP2);
//		fprintf(pCfgFd,"port2=%d\n", g_stIdbtCfg.stDestIpInfo.port2);
//
//
//		fprintf(pCfgFd,"%s\n", pcHelp);
//
//		//�ر������ļ�
//		fclose(pCfgFd);
//
//		return SUCCESS;
//	}
//	else
//	{
//		return FAILURE;
//	}
//}
//
///********************************************************************
//*Function	:	LoadVolumeConfig
//*Input		:	NULL
//*Output		:	NULL
//*Description:	��ȡ������С
//*Others		:
//*********************************************************************/
//static void LoadIdbtConfig(void)
//{
//	FILE *pCfgFd;
//
//	char *pcHelp = "\
//--------------------help info--------------------\n\
//log_upload_mode(1-clock 2-cycle)\n\
//log_upload_cycle_time(unit:hour)\n\
//network_type(1-DHCP 2-PPPOE 3-STATIC)\n\
//--------------------help info--------------------\n\
//";
//	strcpy(g_stIdbtCfg.acSn, SN_NULL);
////	g_stIdbtCfg.iInputVol = 5;
////	g_stIdbtCfg.iOutputCallVol = -12;
////	g_stIdbtCfg.iOutputPlayWavVol = 2;
////	g_stIdbtCfg.iVedioFps = 10;
////	g_stIdbtCfg.iVedioBr = 64;
//	g_stIdbtCfg.iLogUploadMode = 2;
//	g_stIdbtCfg.iLogUploadCycleTime = 6;
//	g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] = NET_DHCP;
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticIP,"196.196.197.213");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticMask,"255.255.0.0");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay,"196.196.196.1");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticDNS,"218.85.157.99");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acMac, "08:90:90:90:90:90");
//	strcpy(g_stIdbtCfg.acManagementPassword, "999999");
//	strcpy(g_stIdbtCfg.acOpendoorPassword, "123456");
//
//
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acWanUserName, "15306919497");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acWanPassword, "2012180");
//
//	strcpy(g_stIdbtCfg.stDestIpInfo.acDestIP1, "115.239.210.26");
//	g_stIdbtCfg.stDestIpInfo.port1 = 80;
//	strcpy(g_stIdbtCfg.stDestIpInfo.acDestIP2, "220.162.97.165");
//	g_stIdbtCfg.stDestIpInfo.port2 = 80;
//
//
//	//��IDBT�����ļ�
//	pCfgFd=fopen(IDBT_CONFIG_PATH,"r");
//
//	if(pCfgFd != NULL)
//	{
//		char buf[300];
//		fread(buf,300,1,pCfgFd);
//		fseek(pCfgFd,0,SEEK_SET);
//		fscanf(pCfgFd,"sn=%s\n",g_stIdbtCfg.acSn);
//		fscanf(pCfgFd,"log_upload_mode=%d\n",&g_stIdbtCfg.iLogUploadMode);
//		fscanf(pCfgFd,"log_upload_cycle_time=%d\n",&g_stIdbtCfg.iLogUploadCycleTime);
//		fscanf(pCfgFd,"management_password=%s\n",g_stIdbtCfg.acManagementPassword);
//		fscanf(pCfgFd,"opendoor_password=%s\n",g_stIdbtCfg.acOpendoorPassword);
//		//������ز���
//		fscanf(pCfgFd,"network_type=%s\n",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
//		fscanf(pCfgFd,"ip_addr=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
//		fscanf(pCfgFd,"net_mask=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
//		fscanf(pCfgFd,"gateway=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
//		fscanf(pCfgFd,"dns_server=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
//		fscanf(pCfgFd,"mac_addr=%s\n", g_stIdbtCfg.stNetWorkInfo.acMac);
//
//
//		fscanf(pCfgFd,"pppoe_user=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
//		fscanf(pCfgFd,"pppoe_passwd=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);
//
//		fscanf(pCfgFd,"dest_ip_1=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP1);
//		fscanf(pCfgFd,"port1=%d\n", &g_stIdbtCfg.stDestIpInfo.port1);
//		fscanf(pCfgFd,"dest_ip_2=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP2);
//		fscanf(pCfgFd,"port2=%d\n", &g_stIdbtCfg.stDestIpInfo.port2);
//
//		//�ر������ļ�
//		fclose(pCfgFd);
//		LOG_RUNLOG_DEBUG("read config file succeed!");
//		return;
//	}
//	else
//	{
//		LOG_RUNLOG_DEBUG("open config file failed!");
//	}
//
//
//	pCfgFd=fopen(IDBT_CONFIG_PATH,"w+");
//	if(pCfgFd != NULL)
//	{
//		fprintf(pCfgFd,"sn=%s\n", g_stIdbtCfg.acSn);
//		fprintf(pCfgFd,"log_upload_mode=%d\n",g_stIdbtCfg.iLogUploadMode);
//		fprintf(pCfgFd,"log_upload_cycle_time=%d\n",g_stIdbtCfg.iLogUploadCycleTime);
//		fprintf(pCfgFd,"management_password=%s\n",g_stIdbtCfg.acManagementPassword);
//		fprintf(pCfgFd,"opendoor_password=%s\n",g_stIdbtCfg.acOpendoorPassword);
//		//������ز���
//		fprintf(pCfgFd,"network_type=%s\n",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
//		fprintf(pCfgFd,"ip_addr=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
//		fprintf(pCfgFd,"net_mask=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
//		fprintf(pCfgFd,"gateway=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
//		fprintf(pCfgFd,"dns_server=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
//		fprintf(pCfgFd,"mac_addr=%s\n", g_stIdbtCfg.stNetWorkInfo.acMac);
//
//		fprintf(pCfgFd,"pppoe_user=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
//		fprintf(pCfgFd,"pppoe_passwd=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);
//
//		fprintf(pCfgFd,"dest_ip_1=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP1);
//		fprintf(pCfgFd,"port1=%d\n", g_stIdbtCfg.stDestIpInfo.port1);
//		fprintf(pCfgFd,"dest_ip_2=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP2);
//		fprintf(pCfgFd,"port2=%d\n", g_stIdbtCfg.stDestIpInfo.port2);
//
//
//		fprintf(pCfgFd,"%s\n", pcHelp);
//	}
//	//�ر������ļ�
//	fclose(pCfgFd);
//}


/********************************************************************
*Function	:	CreateIOTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����NM�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateIOTask(void)
{
#ifdef _YK_XT8126_
	#ifdef _YK_XT8126_AV_
	if(YK8126GpioInit() < 0)
	{
		return FAILURE;
	}
	#else
	if(user_fd > 0)
	{
		g_i8126IoFd = user_fd;
	}
	else
	{
		return FAILURE;
	}
	#endif
#endif

#ifdef _YK_IMX27_AV_
	IMX27GpioInit();
#endif

	g_iRunIOTaskFlag = TRUE;

	return SUCCESS;
}

/********************************************************************
*Function	:	CreateNMTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����NM�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateNMTask(void)
{
	/*if(NMInit() == TRUE)
	{
		g_iRunNMTaskFlag = TRUE;
		sleep(5);
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}*/
}

/********************************************************************
*Function	:	CreateTMTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����TM�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateTMTask(void)
{
#if _TM_TYPE_ == _YK069_
	if(TMStartTerminateManage(YK_APP_VERSION, TM_CONFIG_PATH))
	{
		g_iRunTMTaskFlag = TRUE;
	}
#elif _TM_TYPE_== _TR069_ || defined(_GYY_I5_)
	if(TMStartTerminateManage(YK_APP_VERSION, NULL))
	{
		g_iRunTMTaskFlag = TRUE;
	}
#endif

	return g_iRunTMTaskFlag==TRUE?SUCCESS:FAILURE;
}

/********************************************************************
*Function	:	CreateI6Task
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����TM�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateI6Task(void)
{
#if _TM_TYPE_ == _GYY_I6_
	if(I6Init() == 0)
	{
		g_iRunI6TaskFlag = TRUE;
	}
#endif
	return g_iRunI6TaskFlag==TRUE?SUCCESS:FAILURE;
}

/********************************************************************
*Function	:	CreateLOGTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����LOG�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateLGTask(void)
{
	//g_iRunLGTaskFlag = TRUE;
	if(LOGIntfInit() == SUCCESS)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

/********************************************************************
*Function	:	CreateSMTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����SM�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateSMTask(void)
{
#ifndef _YK_PC_
	if(SMInit() == SUCCESS)
	{
		g_iRunSMTaskFlag = TRUE;
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
#else
	g_iRunSMTaskFlag = TRUE;
	return SUCCESS;
#endif
}

/********************************************************************
*Function	:	CreateLPTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����LP�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateLPTask(void)
{
	//g_iRunLPTaskFlag = TRUE;
	if(LPInit() == SUCCESS)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

/********************************************************************
*Function	:	CreateTITask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����TI��ʱ���������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateTITask(void)
{
	YKInitTimerMgmt();
	g_iRunTITaskFlag = TRUE;
	g_TimerThreadId = YKCreateThread(IDBTTimerThread, NULL);
	if(NULL == g_TimerThreadId)
	{
		return FAILURE;
	}


	return SUCCESS;
}

/********************************************************************
*Function	:	CreateCCTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����CC�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateCCTask(void)
{
	if(CCTaskInit() == SUCCESS)
	{
		g_iRunCCTaskFlag = TRUE;
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

/********************************************************************
*Function	:	CreateMCTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����MC�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateMCTask(void)
{
	return SUCCESS;
}

/********************************************************************
*Function	:	CreateATTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����AT���񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateATTask(void)
{
	if(ATInit() == TRUE)
	{
		g_iRunATTaskFlag = TRUE;
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

/********************************************************************
*Function	:	CreateSSTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����Snapshot���񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateSSTask(void)
{
	if(SnapshotInit() == TRUE)
	{
		g_iRunSSTaskFlag = TRUE;
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}
/********************************************************************
*Function	:	CreateWDTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����WD�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateWDTask(void)
{
#ifdef _YK_XT8126_
	if(WDTaskInit() == FAILURE)
	{
		return FAILURE;
	}
#endif
	g_iRunWDTaskFlag = TRUE;

#ifdef _YK_IMX27_AV_
	if(Imx27WdTaskInit() == FAILURE)
	{
		return FAILURE;
	}
#endif

	return SUCCESS;
}

/********************************************************************
*Function	:	CreateInputTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����Input�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateIPTask(void)
{
	g_pstInputData = YKNew0(INPUT_THREAD_DATA_ST, 1);
	if(g_pstInputData == NULL)
	{
		return FAILURE;
	}

	g_pstInputData->Lock = YKCreateMutex(NULL);

	// �����������������߳�
	g_pstInputData->Thread = YKCreateThread(IDBTInputThread, NULL);
	if(g_pstInputData->Thread == NULL)
	{
		return FAILURE;
	}

	// ����ģ�����б�־
	g_iRunIPTaskFlag = TRUE;

	return SUCCESS;
}


/********************************************************************
*Function	:	CreateXTTask
*Input		:	void
*Output		:	SUCCESS
*Output			FAILURE
*Description:	����XT�������񣬳ɹ�����SUCCESS��ʧ�ܷ���FAILURE
*Others		:
*********************************************************************/
static int CreateLANTask(void)
{
#if (defined(_SUPPORT_XT8130_)||defined(_SUPPORT_QSA9000_))
	if(LANInit() == SUCCESS)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
#else
	return SUCCESS;
#endif
}

static int CreateABTask(void)
{
	if(IDBTABInit() == FAILURE)
	{
		return FAILURE;
	}

	g_iRunABTaskFlag = TRUE;

	return SUCCESS;
}

/********************************************************************
*Function	:	ExitNMTask
*Input		:	void
*Output		:	void
*Description:	�˳�NM��������
*Others		:
*********************************************************************/
void ExitNMTask(void)
{
	// ��ģ�����б�־���
	g_iRunNMTaskFlag = FALSE;
	//NMFini();
}

/********************************************************************
*Function	:	ExitATTask
*Input		:	void
*Output		:	void
*Description:	�˳�NM��������
*Others		:
*********************************************************************/
void ExitATTask(void)
{
	// ��ģ�����б�־���
	g_iRunATTaskFlag = FALSE;
	//ATFini();
}

/********************************************************************
*Function	:	ExitTMTask
*Input		:	void
*Output		:	void
*Description:	�˳�TM��������
*Others		:
*********************************************************************/
void ExitTMTask(void)
{
#if _TM_TYPE_== _TR069_ || defined(_GYY_I5_)
	TMStopTerminateMange();
#endif
}

/********************************************************************
*Function	:	ExitI6Task
*Input		:	void
*Output		:	void
*Description:	�˳�TM��������
*Others		:
*********************************************************************/
void ExitI6Task(void)
{
#if _TM_TYPE_ == _GYY_I6_
	I6Fini();
#endif
}

/********************************************************************
*Function	:	ExitSMTask
*Input		:	void
*Output		:	void
*Description:	�˳�SM��������
*Others		:
*********************************************************************/
void ExitSMTask(void)
{
	// ��ģ�����б�־���
	g_iRunSMTaskFlag = FALSE;
#ifndef _YK_PC_
	SMFini();
#endif
}

/********************************************************************
*Function	:	ExitSSTask
*Input		:	void
*Output		:	void
*Description:	�˳�SS��������
*Others		:
*********************************************************************/
void ExitSSTask(void)
{
	// ��ģ�����б�־���
	g_iRunSSTaskFlag = FALSE;
//	SSFini();
}


/********************************************************************
*Function	:	ExitLPTask
*Input		:	void
*Output		:	void
*Description:	�˳�LP��������
*Others		:
*********************************************************************/
void ExitLPTask(void)
{
	// ��ģ�����б�־���
	g_iRunLPTaskFlag = FALSE;

	LPFini();
}

/********************************************************************
*Function	:	ExitTITask
*Input		:	void
*Output		:	void
*Description:	�˳�TI��ʱ����������
*Others		:
*********************************************************************/
void ExitTITask(void)
{
	g_iRunTITaskFlag = FALSE;
	YKJoinThread(g_TimerThreadId);
	YKRemoveAllTimers();
	YKClearTimerMgmt();
}

/********************************************************************
*Function	:	ExitCCTask
*Input		:	void
*Output		:	void
*Description:	�˳�CC��������
*Others		:
*********************************************************************/
void ExitCCTask(void)
{
	CCTaskUninit();
}

/********************************************************************
*Function	:	ExitLOGTask
*Input		:	void
*Output		:	void
*Description:	�˳�LOG��������
*Others		:
*********************************************************************/
void ExitLGTask(void)
{
	LOGIntfFini();
}

/********************************************************************
*Function	:	ExitCCTask
*Input		:	void
*Output		:	void
*Description:	�˳�CC��������
*Others		:
*********************************************************************/
void ExitMCTask(void)
{
	// ���߳����б�־���
	g_iRunMCTaskFlag = FALSE;
}

/********************************************************************
*Function	:	ExitCCTask
*Input		:	void
*Output		:	void
*Description:	�˳�CC��������
*Others		:
*********************************************************************/
void ExitWDTask(void)
{
#ifdef _YK_XT8126_
	// ���߳����б�־���
	WDTaskFini();
#endif
	g_iRunWDTaskFlag = FALSE;
#ifdef _YK_IMX27_AV_
	// ���߳����б�־���
	Imx27WdTaskFini();
#endif

}

/********************************************************************
*Function	:	ExitInputTask
*Input		:	void
*Output		:	void
*Description:	�˳�Input��������
*Others		:
*********************************************************************/
void ExitIPTask(void)
{
	void *retCode;

	// ���߳����б�־���
	g_iRunIPTaskFlag = FALSE;

	if(g_pstInputData->Thread)
	{
		pthread_kill(g_pstInputData->Thread, SIGUNUSED);

		//YKJoinThread(g_pstInputData->Thread);
		pthread_join(g_pstInputData->Thread, &retCode);
	}

	// �ͷŻ�����
	YKDestroyMutex(g_pstInputData->Lock);

	// ɾ�����������߳�
	//YKDestroyThread(g_pstInputData);

	// �ͷ��ڴ�
	YKFree(g_pstInputData);
}


void ExitLANTask(void)
{
#if (defined(_SUPPORT_XT8130_)||defined(_SUPPORT_QSA9000_))
	//LANUninit();
#endif
}

void ExitXTTask(void)
{
	// ���߳����б�־���
//	g_iRunXTTaskFlag = FALSE;

#ifdef _YK_XT8126_BV_
	// ���̷߳����˳�����Ϣ
	XT_EVENT_MSG_ST *pstMsg = YKNew0(XT_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		LOG_RUNLOG_DEBUG("xt malloc error!\n");
	}
	pstMsg->uiPrmvType = XT_EXIT;
	YKWriteQue(g_pstXTIntfMsgQ, pstMsg, 0);

	// �ȴ��߳���Դ����
	YKJoinThread(g_XTIntfThreadId);
	YKJoinThread(g_XTTaskThreadId);

	sleep(4);

	// ɾ����Ϣ����
	YKDeleteQue(g_pstXTIntfMsgQ);
#endif
}

void ExitIOTask(void)
{
	// ��IO���б�־���
	g_iRunIOTaskFlag = FALSE;

#ifdef _YK_XT8126_
	YK8126GpioUninit();
#endif
#ifdef _YK_IMX27_AV_
	IMX27GpioUninit();
#endif
}

void ExitABTask(void)
{
	// ��IT���б�־���
	g_iRunABTaskFlag = FALSE;

	AB_EVENT_MSG_ST *pstMsg = YKNew0(AB_EVENT_MSG_ST, 1);
	if(NULL == pstMsg)
	{
		LOG_RUNLOG_DEBUG("AB malloc error!\n");
	}
	pstMsg->uiPrmvType = AB_EXIT;
	YKWriteQue(g_pstABMsgQ, pstMsg, 0);

	YKJoinThread(g_IDBTIntfLayerThreadId);

	// ɾ����Ϣ����
	if(g_pstABMsgQ)
	{
		YKDeleteQue(g_pstABMsgQ);
	}
}

void UpdateConfig(void)
{
	g_iUpdateConfigFlag = TRUE;
}

void ExitApp(void)
{
	g_iRunSystemFlag = FALSE;
}

/********************************************************************
*Function	:	ProcessHlepCmd
*Input		:	void
*Output		:	void
*Description:	���������help����
*Others		:
*********************************************************************/
void ProcessHlepCmd(void)
{
	fprintf(stdout, "/******************cmd type!*********************/\n");
	fprintf(stdout, "LPC cmd\n");
	fprintf(stdout, "FTP cmd\n");
	fprintf(stdout, "TMC cmd\n");
	fprintf(stdout, "SMC cmd\n");
	fprintf(stdout, "NMC cmd\n");
	fprintf(stdout, "help cmd\n");
	fprintf(stdout, "quit cmd!\n");
	fprintf(stdout, "version cmd!\n");
}

/********************************************************************
*Function	:	ProcessQuitCmd
*Input		:	void
*Output		:	void
*Description:	���������quit����
*Others		:
*********************************************************************/
void ProcessQuitCmd(void)
{
	g_iRunSystemFlag = FALSE;
}

/********************************************************************
*Function	:	IDBTProcessInputCmd
*Input		:	cmd ���ַ�ָ��ָ�����һ�������ַ�������ʽΪ��ģ���� ģ��������
*Inp			��������"LPC call sip:12345678"
*Output		:	SUCCESS������ɹ�
*Output			FAILURE������ʧ��
*Description:
*Others		:
*********************************************************************/
static int IDBTProcessInputCmd(const char *cmd)
{
	int i;
	int size=sizeof(g_InputCmds)/sizeof(IDBTCmd);
	char acCmd[IDBT_MAX_SIZE] = {0};
	char *pcCmd = NULL;
	int ret = FAILURE;

	// ���������ݴ�ŵ�������
	strcpy(acCmd, cmd);

	// Ѱ����������ĵ�һ���ո�
	pcCmd = strchr(acCmd, ' ');
	if(pcCmd != NULL)
	{
		// ���ո�����Ϊ��������ָ��ָ����һ���ַ�
		*pcCmd++ = '\0';

		// ָ��ָ����һ���ǿ��ַ�
		while(isspace(*pcCmd))
		{
			pcCmd ++;
			if(pcCmd > (acCmd + IDBT_MAX_SIZE))
			{
				// ��������������ո�����Ϊ������ģ��û�к�������������
				pcCmd = NULL;
				break;
			}
		}
	}
	else
	{
		if(strlen(acCmd) != 0)
		{
			// �жϴ����ֻ��һ��������Դ�������Ҳ���н���
			// ��ʱ��pcCmd = NULL
		}
		else
		{
			// �����Ϊ���ַ������򲻽��н���
			return SUCCESS;
		}
	}

	for(i=0; i<size; i++)
	{
		if(strcmp(g_InputCmds[i].pcCmd, acCmd) == 0)
		{
			if((g_InputCmds[i].ProcessFunc != NULL) && (pcCmd != NULL))
			{
				ret = g_InputCmds[i].ProcessFunc(pcCmd);
			}
			else
			{
				// ������ֻ��һ������
				if(strcmp(acCmd, HELP_CMD) == 0)
				{
					// ִ�а���ָ��
					ProcessHlepCmd();
					ret = SUCCESS;
				}
				else if(strcmp(acCmd, QUIT_CMD) == 0)
				{
					// ִ���˳�ָ��
					ProcessQuitCmd();
					ret = SUCCESS;
				}
				else if(strcmp(acCmd, VERSION_CMD) == 0)
				{
					LOG_RUNLOG_DEBUG("ID =========================");
					LOG_RUNLOG_DEBUG("ID App Version : %s\n", YK_APP_VERSION);
					system("cat VERSION");
					LOG_RUNLOG_DEBUG("\n");
					LOG_RUNLOG_DEBUG("=========================\n");
					ret = SUCCESS;
				}
				else
				{
					return FAILURE;
				}
			}

			// �Ѿ�Ѱ�ҵ���Ӧ������ģ�飬ִ�к��˳�
			break;
		}
	}

	return ret;
}

/********************************************************************
*Function	:	HandleInput
*Input		:	void
*Output		:	void
*Description:	���������ݽ��д�������ɹ�����ӡ��Ϣ������ʧ�ܴ�ӡʧ����Ϣ
*Others		:
*********************************************************************/
void HandleInput(void)
{
	int ret = SUCCESS;

	if(g_pstInputData->Thread > 0)
	{
		if(g_pstInputData->bHasReceived)
		{
			YKLockMutex(g_pstInputData->Lock);
			g_pstInputData->bHasReceived=FALSE;
			if(strlen(StripBlanks(g_pstInputData->acCmd)))
			{
				ret = IDBTProcessInputCmd(g_pstInputData->acCmd);
			}
			YKUnLockMutex(g_pstInputData->Lock);

			if(ret == SUCCESS)
			{
				// �����ʾ��
				fprintf(stdout, "%s",IDBT_PROMPT);
				fflush(stdout);
			}
			else
			{
				// ���������ʾ
				fprintf(stdout, "%s",IDBT_INPUT_ERROR);
				fflush(stdout);
			}
		}
	}
}

#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
	//�����ļ���ѹ����Ŀ¼
	void Bunzip2TMData()
	{
		char buf[256];
		sprintf(buf,"busybox cp %s /",TM_DATA_PATH);
		system(buf);
		sprintf(buf,"busybox bunzip2 %s.bz2",TM_DATA_RUNNING);
		system(buf);
		sprintf(buf,"busybox cp %s /",TM_DATA_BACKUP_PATH);
		system(buf);
		sprintf(buf,"busybox bunzip2 %s.bz2",TM_DATA_BAK_RUNNING);
		system(buf);
	}

	//�����ļ���ѹ����Ŀ¼
	void Bunzip2I6Data()
	{
		//��ѹuserinfo
		system("busybox cp "I6_CONFIG_BZIP" "I6_CONFIG_PATH".bz2");
		system("busybox bunzip2 "I6_CONFIG_PATH".bz2");
		//��ѹpwdinfo
		system("busybox cp "I6_PASSWORD_BZIP" "I6_PASSWORD_PATH".bz2");
		system("busybox bunzip2 "I6_PASSWORD_PATH".bz2");
		//��ѹcardinfo
		system("busybox cp "I6_CARD_BZIP" "I6_CARD_PATH".bz2");
		system("busybox bunzip2 "I6_CARD_PATH".bz2");
	}
#endif

#if 1
#ifndef LP_ONLY
/********************************************************************
*Function	:	main
*Input		:	arc
*Input		��	argv[]
*Output		:	SUCCESS
*Description:	������
*Others		:
*********************************************************************/
int IDBTMain(int argc, char*argv[])
{
	// ��ʼ�汾���
	char* mark=NULL;
	char* oldVersion=NULL;
//	char myVersion[256];

	if(argc>2)
	{
		mark=argv[1];
		oldVersion=argv[2];
	}
//#if _TM_TYPE_ == _GYY_I6_
//	//�汾д����ʱ�ļ�
//	sprintf(myVersion,"busybox echo \"%s\" > %s/myVersion",YK_APP_VERSION,LOG_FLASH_PATH);
//	system(myVersion);
//#endif
	switch(FPCheckApplication(mark, oldVersion))
	{
	    case FP_CHECK_UPGRADE:
	    	LOG_RUNLOG_DEBUG("ID This is Update Version");
	        exit(FP_CHECK_UPGRADE);
	    case FP_CHECK_NORMAL:
	    	LOG_RUNLOG_DEBUG("ID start the program!");
	        break;
	    case FP_CHECK_ERROR:
	        exit(FP_CHECK_ERROR);
	    default:
	    	break;
	}

//	system("/usr/sbin/telnetd &");
	//LOG_RUNLOG_DEBUG("telnetd started!\n");
	//��ʱ��
	setenv("TZ", "CST-8", 1);
	tzset();

#ifdef _YK_IMX27_AV_
	//("echo \"load driver begin\" >> insmod_test");
	// ������ع�������
	if(FAILURE == CreateWDTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID wd failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID wd is ok");
	//system("echo \"load driver success\" >> insmod_test");
	//exit(-1);
#endif

	//signal(SIGPIPE, SIG_IGN);
	//signal(SIGCLD,SIG_IGN);
	LOG_RUNLOG_DEBUG("line=%d\n",__LINE__);

	//��ȡIDBT������ò���
	LoadIdbtConfig();

#if _TM_TYPE_ == _GYY_I6_
	//����TMModel.xml
	TMCreateModel();
	NewI5Data();
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
	//�����ļ���ѹ����Ŀ¼
	Bunzip2I6Data();
#endif
#endif

#if _TM_TYPE_ == _TR069_
	//����TMModel.xml
	TMCreateModel();
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
	//�����ļ���ѹ����Ŀ¼
	Bunzip2TMData();
#endif
#endif
	if(FAILURE == CreateABTask())
	{
		LOG_RUNLOG_DEBUG("AB failed\n");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("AB is ok\n");

	// ������ʱ����������
	if(FAILURE == CreateTITask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID timer failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID timer is ok");

	if(FAILURE == CreateIOTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID io set failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID io is ok");

#ifdef _YK_XT8126_AV_
	// ������ع�������
	if(FAILURE == CreateWDTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID wd failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID wd is ok");
#endif

	// ������־��������
	if(FAILURE == CreateLGTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID log failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID log is ok");

	// ���������豸��������,ֻ����BVϵ��
	if(FAILURE == CreateLANTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID LAN failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID LAN is ok");

#ifdef _YK_XT8126_BV_
	while(g_iRunXTTaskFlag == FALSE);
	while(user_fd < 0);
#endif

	#if 0
#ifndef _YK_XT8126_BV_
	// ���������������
	if(FAILURE == CreateNMTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID nm failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID nm is ok");
#else
	setenv("TZ", "CST-8", 1);
#endif
#endif
	// ����Linphone��������
	if(FAILURE == CreateLPTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID lp failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID lp is ok");

#if _TM_TYPE_ == _GYY_I6_
	//����I6��������
	if(FAILURE == CreateI6Task())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID i6 failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID i6 is ok");
#endif

#if _TM_TYPE_== _TR069_ || defined(_GYY_I5_)
	// ����TM��������
	if(FAILURE == CreateTMTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID tm failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID tm is ok");
#endif

	//��������ӿڹ�������
	if(FAILURE == CreateSMTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID sm failed");
//		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID sm is ok");

	// �������й�������
	if(FAILURE == CreateCCTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID cc failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID cc is ok");

	// ������ع�������
	if(FAILURE == CreateMCTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID mc failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID mc is ok");

#ifdef IDBT_AUTO_TEST
	// �����Զ���������
	if(FAILURE == CreateATTask())
	{
		// ��¼��־
		LOG_RUNLOG_ERROR("ID at failed");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("ID at is ok");
#endif

    //if (FALSE == TSInit())
	//{
    //	LOG_RUNLOG_ERROR("ID ts failed");
	//	goto EXIT;
	//}

    if (FALSE == CreateSSTask())
	{
    	LOG_RUNLOG_ERROR("ID ss failed");
		goto EXIT;
	}

	// �������������������
	if(FAILURE == CreateIPTask())
	{
		// ��¼��־
		LOG_RUNLOG_DEBUG("input failed\n");
		goto EXIT;
	}
	LOG_RUNLOG_DEBUG("input is ok\n");

	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_SYSTEM_REBOOT, 0, "");

	sleep(1);

	return SUCCESS;

	// ���߳��ڴ˵ȴ���������
	while(g_iRunSystemFlag == TRUE)
	{
		usleep(100000);
		//HandleInput();
	}

EXIT:
#if (defined(_YK_IMX27_AV_))
	if(g_iRunWDTaskFlag == TRUE)
	{
		ExitWDTask();
		LOG_RUNLOG_DEBUG("ID WD task exit!");
	}
#endif

#ifdef IDBT_AUTO_TEST
	if(g_iRunATTaskFlag == TRUE)
	{
		ExitATTask();
		LOG_RUNLOG_DEBUG("ID AT task exit!");
	}
#endif
	if (g_iRunSSTaskFlag == TRUE)
	{
		ExitSSTask();
		LOG_RUNLOG_DEBUG("ID SS task exit!");
	}
	if(g_iRunSMTaskFlag == TRUE)
	{
		ExitSMTask();
		LOG_RUNLOG_DEBUG("ID SM task exit!");
	}

	if(g_iRunCCTaskFlag == TRUE)
	{
		ExitCCTask();
		LOG_RUNLOG_DEBUG("ID CC task exit!");
	}

	if(g_iRunLPTaskFlag == TRUE)
	{
		ExitLPTask();
		LOG_RUNLOG_DEBUG("ID LP task exit!");
	}

	if(g_iRunLGTaskFlag == TRUE)
	{
		ExitLGTask();
		LOG_RUNLOG_DEBUG("ID LG task exit!");
	}


	if(g_iRunIPTaskFlag == TRUE)
	{
		ExitIPTask();
		LOG_RUNLOG_DEBUG("ID IP task exit!");
	}

	if(g_iRunTMTaskFlag == TRUE)
	{
		ExitTMTask();
		LOG_RUNLOG_DEBUG("ID TM task exit!");
	}
//#ifndef _YK_XT8126_BV_
	if(g_iRunNMTaskFlag == TRUE)
	{
		ExitNMTask();
		LOG_RUNLOG_DEBUG("ID NM task exit!");
	}
//#endif

	if(g_iRunMCTaskFlag == TRUE)
	{
		ExitMCTask();
		LOG_RUNLOG_DEBUG("ID MC task exit!");
	}

	if(g_iRunIOTaskFlag == TRUE)
	{
		ExitIOTask();
		LOG_RUNLOG_DEBUG("ID IO task exit!");
	}

#if (defined(_YK_XT8126_AV_))
	if(g_iRunWDTaskFlag == TRUE)
	{
		ExitWDTask();
		LOG_RUNLOG_DEBUG("ID WD task exit!");
	}
#endif

	if(g_iRunTITaskFlag == TRUE)
	{
		ExitTITask();
		LOG_RUNLOG_DEBUG("ID TI task exit!");
	}

	if(g_iRunLANTaskFlag == TRUE)
	{
		ExitLANTask();
		LOG_RUNLOG_DEBUG("ID XT task exit!");
	}

	if(g_iRunABTaskFlag == TRUE)
	{
		ExitABTask();
		LOG_RUNLOG_DEBUG("AB task exit!\n");
	}

	LOG_RUNLOG_DEBUG("ID bye bye!");

#ifndef _YK_IMX27_AV_
#ifndef _YK_PC_
	system("reboot");
#endif
#endif

	return SUCCESS;
}
#endif

#endif
