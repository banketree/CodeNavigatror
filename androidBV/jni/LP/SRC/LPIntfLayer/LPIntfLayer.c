
/**
 *@file    LPIntLayer.c
 *@brief   Linpone�ӿڷ�װ
 *
 *@author  chensq
 *@version 1.0
 *@date    2012-5-20
 */


#include <stdio.h>
#include <sal.h>
#include <string.h>
#include <linphonec.h>
#include <linphonecore.h>
#include <assert.h>
#include "LPIntfLayer.h"


#ifdef _YK_IMX27_AV_
#include "Imx27WdTask.h"
#endif

#include "pthread.h"
#include "LPMsgOp.h"
#include "LPUtils.h"
#include "YKMsgQue.h"
#include "IDBT.h"
#include "sal.h"
#include "private.h"
#include "sal_eXosip2.h"
#include "TRManage.h"
#include "LOGIntfLayer.h"
#include "pub.h"
#include "YKMsgQue.h"
#ifdef _I6_CC_
#include "I6CCTask.h"
#else
#include "CCTask.h"
#endif
#include <TMInterface.h>


#ifdef _YK_XT8126_BV_
#include "common.h"
#endif

#define LP_CMDLINE_BUFF_SIZE (256)                                   //linphone����������
#define LP_DTMF_MAX_COUNT    (20)									//can recv max dtmf after conn


typedef enum
{
	SIP_REG_CHECK_INIT  = 0     ,                      //init
	SIP_REG_CHECK_END           ,                      //net_ok|sip_on_line
	SIP_REG_CHECK_WAIT_401_407  ,                      //wait_401/407
	SIP_REG_CHECK_RECV_401_407  ,                      //recv_401_407
	SIP_REG_CHECK_RECV_403_603_200                     //recv_403_603_200
}SIP_REG_CHECK_EN;

static volatile CALL_DIR_EN g_enCallDir = CALL_DIR_OUTGOING;        //connect dir
static volatile long g_lCallId = 0;                                //connect call id
static volatile REG_STATUS_EN g_enCurrReg = REG_STATUS_OFFLINE;      //SIPע��״̬
static volatile REG_STATUS_EN g_enOldReg = -1;                      //SIP��һ��ע��״̬

static volatile NETWORK_STATUS_EN g_enNetworkStatus = NETWORK_STATUS_OFFLINE;

static YKHandle g_LPThreadId;
YK_MSG_QUE_ST*  g_pstLPMsgQ = NULL;//LP��Ϣ����
static volatile int g_iDtmfCount = 0;                               //����ͨ��dtmf�źŽ�������
extern YK_MSG_QUE_ST* g_pstXTIntfMsgQ;                            //XT��Ϣ����ָ��

LP_TIMER_ST g_stStopCallingTimeoutTimer;							//��ʱ�Ҷ�
LP_TIMER_ST g_stDtmfRecvTimeoutTimer;                              	//dtmf recv timeout timer
LP_TIMER_ST g_stSipRegTimeoutTimer;                              	//ע��ʧ�ܳ�ʱ
LP_TIMER_ST g_stSipIncommingTimeoutTimer;                              	//ע��ʧ�ܳ�ʱ
static SIP_REG_CHECK_EN g_enSipRegCheckFlg = SIP_REG_CHECK_INIT;
static volatile char g_cConnType = CONN_TYPE_AUDIO;


//CALL_PREVIEW�޸�Ϊȫ�ֱ�������
static volatile char g_cCallPreviewVersion = 0; //0����Ԥ�� 1:Ԥ��

//// 0:��Ԥ�� 1:Ԥ�� 2:Ԥ�����ѱ�������δ֪ͨCC���� 3:Ԥ�����ѱ�������֪ͨCC���� 4:CC��ʱ�Ҷ�
//#ifdef CALL_PREVIEW
//	int g_iCallPreviewFlg = 1;
//#else
//	int g_iCallPreviewFlg = 0;
//#endif
// 0:��Ԥ�� 1:Ԥ�� 2:Ԥ�����ѱ�������δ֪ͨCC���� 3:Ԥ�����ѱ�������֪ͨCC���� 4:CC��ʱ�Ҷ�
//#ifdef CALL_PREVIEW
//	int g_iCallPreviewFlg = 1;
//#else
	int g_iCallPreviewFlg = 0;
//#endif

pthread_mutex_t g_lpLock;

//extern int audioIncommingFlg;

typedef struct
{
	int  iRcvDtmfFlg;
	int  iRcvMsgFlg;
	int  iMsgSeq;
	char acSipInfo[USER_NAME_BUFF_LEN];
}ST_SIP_MSG_INFO;


static ST_SIP_MSG_INFO g_stCurSipMsgInfo = {0};

/**
 * @brief        ����ǰ����ͨ��״̬ʱ,ͨ���ýӿڵ�֪�Ǳ��л�������
 * @param[in]
 * @return       CALL_DIR_OUTGOING/CALL_DIR_INCOMMING
 */
CALL_DIR_EN LPGetCallDir(void)
{
	LOG_RUNLOG_DEBUG("LP LPGetCallDir call dir %d (outgoing=0;incomming=1)", g_enCallDir);
	return g_enCallDir;
}


//cFlg:0 ��Ԥ�� 1:Ԥ��
void LPSetCallViewVersion(char cFlg)
{
	g_cCallPreviewVersion = cFlg;
}

/**
 * @brief        ͨ���ýӿڵ�֪SIPע��״̬
 * @param[in]
 * @return       REG_STATUS_OFFLINE/REG_STATUS_ONLINE
 */
REG_STATUS_EN LPGetSipRegStatus(void)
{

	return g_enCurrReg;
}



void LPSetSipRegStatus(REG_STATUS_EN enStatus)
{
	LOG_RUNLOG_INFO("LP LPSetSipRegStatus enStatus:%d", enStatus);
	g_enCurrReg = enStatus;

#ifdef _YK_IMX27_AV_
	if(g_enCurrReg == REG_STATUS_ONLINE)
	{

			SMCtlSipLed(0);
	}
	else if(g_enCurrReg == REG_STATUS_OFFLINE)
	{
			SMCtlSipLed(1);

	}
#endif

	//add 20120828
	if(g_enOldReg != g_enCurrReg)
	{
		//SIP reg status changed
		LOG_RUNLOG_DEBUG("LP sip reg status changed(old:%d new:%d)", g_enOldReg, g_enCurrReg);

		g_enOldReg = g_enCurrReg;

		LPMsgSendSipABRegStatus(g_enCurrReg);

#ifndef LP_ONLY
		if(g_enCurrReg == REG_STATUS_ONLINE)
		{
#if _TM_TYPE_ == _YK069_
			TMUpdateAlarmState(TM_ALARM_IMS_REGISTER, '0');
#else
			TMUpdateAlarmState(TM_ALARM_IMS_REGISTER, "0");
#endif
		}
		else if(g_enCurrReg == REG_STATUS_OFFLINE)
		{
#if _TM_TYPE_ == _YK069_
			TMUpdateAlarmState(TM_ALARM_IMS_REGISTER, '1');
#else
			TMUpdateAlarmState(TM_ALARM_IMS_REGISTER, "1");
#endif
		}
#endif
	}
}

//?????????????????????????????????????????????????
void LPSetNetworkStatus(NETWORK_STATUS_EN enStatus)
{
	g_enNetworkStatus = enStatus;
	LPMsgSendSipABNetworkStatus(g_enNetworkStatus);
}

/**
 * @brief         ͨ���ýӿڱ��SIPע��״̬
 * @param[in]
 * @return       REG_STATUS_OFFLINE
 */
void LPSetSipRegStatusForBV(REG_STATUS_EN enStatus)
{
	if(g_iRunLPTaskFlag == TRUE)
	{
		LOG_RUNLOG_DEBUG("LP g_iRunLPTaskFlag is true, will exec LPSetSipRegStatus[%d]", REG_STATUS_OFFLINE);
		if(enStatus == REG_STATUS_OFFLINE)
		{
			linphonec_set_registered();
		}
	}
	else
	{
		LOG_RUNLOG_DEBUG("LP g_iRunLPTaskFlag is false, will no exec LPSetSipRegStatus[%d]", REG_STATUS_OFFLINE);
	}
	g_enCurrReg = REG_STATUS_OFFLINE;
}



/**
 * @brief        Linphone����͵��ܵ�
 * @param[in]    pcCommand:����
 * @return       ��
 */

 void LPProcessCommand(const char* pcCommand)
{


	 if(g_iRunLPTaskFlag == TRUE)
	 {

		pthread_mutex_lock(&g_lpLock);

		LOG_RUNLOG_DEBUG("LP LPProcessCommand:%s", pcCommand);
	    char path[512]={0x0};
	    ortp_pipe_t handle=NULL;
	    snprintf(path,sizeof(path)-1,"linphonec-%i",getuid());
	    handle = ortp_client_pipe_connect(path);
	    ortp_pipe_write(handle, (const uint8_t *)pcCommand, strlen(pcCommand));
	    ortp_client_pipe_close(handle);

	    pthread_mutex_unlock(&g_lpLock);

	 }
	 else
	 {
		 LOG_RUNLOG_WARN("LP g_iRunLPTaskFlag == FALSE will no exec cmd");
	 }
}


/**
 * @brief           Linphone����ע��
 * @param[in]
 * @return          ��
 */

static char* LPFirstRegister(void)
{
    char acUserName[USER_NAME_BUFF_LEN] = {0x00};
    char acProxy[PROXY_BUFF_LEN] = {0x00};
    char acRoute[ROUTE_BUFF_LEN] = {0x00};
    char acPassword[PASSWORD_BUFF_LEN] = {0x00};
    int iPort = 0;
    int iPrefixLen = strlen("sip:");

//	sprintf(acUserName, "%s", "sip:");
//	sprintf(acProxy, "%s", "sip:");
//	sprintf(acRoute, "%s", "sip:");
//   TMGetIMSInfo(acUserName+iPrefixLen,  acPassword, acRoute+iPrefixLen, &iPort,  acProxy+iPrefixLen);

    TMGetIMSInfo(acUserName,  acPassword, acRoute, &iPort,  acProxy);

    LOG_RUNLOG_DEBUG("LP LPFirstRegister info:acUserName(%s)/acPassword(%s)/acProxy(%s)/acRoute(%s)/iPort(%d)",
    		acUserName, acPassword, acProxy, acRoute, iPort);

//    if( strlen(acUserName) == 4 || strlen(acProxy) == 4 || strlen(acRoute) == 4 || strlen(acPassword) == 0 )
//    {
//    	LOG_RUNLOG_WARN("LP LPFirstRegister info: get ims info error");
//    	return NULL;
//    }
        if( strlen(acUserName) == 0 || strlen(acProxy) == 0 || strlen(acRoute) == 0 || strlen(acPassword) == 0 )
        {
        	LOG_RUNLOG_WARN("LP LPFirstRegister info: get ims info error");
        	return NULL;
        }


    LOG_RUNLOG_DEBUG("LP use default reg info(user:%s/passwd:%s/proxy:%s/route:%s/expires:%d)",
    		acUserName, acPassword, acProxy, acRoute, LP_EXPIRES_TIME);

    linphonec_push_proxy(acUserName, acPassword, acProxy, acRoute, LP_EXPIRES_TIME, TRUE);

    return NULL;
}





static void LPSipRegTimeOutCallback(void* pvPara)
{

	LOG_RUNLOG_INFO("LP LPSipRegTimeOutCallback %d", g_enSipRegCheckFlg);

	if(LPUtilsGetNetStatus() == 0)
	{
		//��ʱ��״̬���ǵȴ�401/407��Ĭ����Ϊ����401���ܶ�ȡ����,���˿�
		if(g_enSipRegCheckFlg == SIP_REG_CHECK_WAIT_401_407)
		{
			LOG_RUNLOG_WARN("LP SIP_REG_CHECK_WAIT_401_407 SipRegTimeOut will change port");
			sal_eXosip2_ports_change();
		}
		else if(g_enSipRegCheckFlg == SIP_REG_CHECK_RECV_401_407)
		{
			LOG_RUNLOG_WARN("LP SIP_REG_CHECK_RECV_401_407 SipRegTimeOut will change port");
			sal_eXosip2_ports_change();
		}
	}

	g_stSipRegTimeoutTimer.pstTimer = NULL;
}


static void LPSipIncommingTimeOutCallback(void* pvPara)
{

	LOG_RUNLOG_INFO("LP LPSipIncommingTimeOutCallbackd");

	g_stSipIncommingTimeoutTimer.pstTimer = NULL;
}

/**
 * @brief         ��ȡlinphone��Ϣ����(�����߳�)
 * @param[in]
 * @return
 */
static void *LPIntLayerRevMsgTask(void* arg)
{

    char acCmdBuff[LP_CMDLINE_BUFF_SIZE] =  { 0x00 };
    int iErrCode = 0;
    void* pvMsg = NULL;


    while(g_iRunLPTaskFlag)
    {


//    	//��Լ30S���һ������״̬
//    	static int iCount = 0;
//    	if(iCount++ > 60)//250 * 4 ms * 30 = 1s * 30  = 30s
//    	{
//    		iCount = 0;
//    		LPUtilsCheckNet();
//    	}

//    	#ifdef _YK_IMX27_AV_
//    				static int iTempCount = 0;
//    				if(iTempCount++ > 10)
//    				{
//    					iTempCount = 0;
//    					NotifyWDThreadFromLP();
//    				}
//    	#endif

    	//�z�ySIPδ��������������ʱ������ʱ��
    	if( g_enCurrReg == REG_STATUS_ONLINE || LPUtilsGetNetStatus() == -1 )
    	{
    		g_enSipRegCheckFlg = SIP_REG_CHECK_END;
    	}
    	else if(g_enCurrReg == REG_STATUS_OFFLINE && LPUtilsGetNetStatus() == 0 )
    	{

    		//������ʱ������Ƿ��յ�401/407
			if(g_stSipRegTimeoutTimer.pstTimer == NULL)
			{
				g_enSipRegCheckFlg = SIP_REG_CHECK_WAIT_401_407; //wait_401_407
	    		g_stSipRegTimeoutTimer.pstTimer = YKCreateTimer(LPSipRegTimeOutCallback, NULL, SIP_REG_TIMROUT,
	    					YK_TT_NONPERIODIC, (unsigned long *)&g_stSipRegTimeoutTimer.ulMagicNum);
	    		if(NULL == g_stSipRegTimeoutTimer.pstTimer)
	    		{
	    			LOG_RUNLOG_ERROR("LP create g_stSipRegTimeoutTimer failed!");
	    		}
	    		else
	    		{
	    			LOG_RUNLOG_INFO("LP g_stSipRegTimeoutTimer create!");
	    		}
			}
			else
			{
				//LOG_RUNLOG_DEBUG("LP g_stSipRegTimeoutTimer exist not need to create it!");
			}

    	}

//test code
//    	if(g_enCurrReg == REG_STATUS_ONLINE)
//    	{
//        	//0:unregistered 1:registered 2:calling 3:terminating 4:end
//        	if(g_iTestFlg == 0 || g_iTestFlg == 4)
//        	{
//        		g_iTestFlg = 2;
//        		g_lCount = 0;
//        		LPProcessCommand("terminate all");
//        		sleep(1);
//        		LPProcessCommand("call 6021");
//        	}
//        	else if(g_iTestFlg == 2)
//        	{
//        		g_lCount++;
//        		if(g_lCount >= 4*10) //10s
//        		{
//        			g_lCount = 0;
//        			g_iTestFlg = 3;
//        			LPProcessCommand("terminate all");
//        		}
//        	}
//        	else if(g_iTestFlg == 3)
//        	{
//        		g_lCount++;
//        		if(g_lCount >= 4) //1s
//        		{
//        			g_lCount = 0;
//        			g_iTestFlg = 4;
//        		}
//        	}
//    	}







    	memset(acCmdBuff, 0x00, sizeof(acCmdBuff));
        iErrCode = YKReadQue(g_pstLPMsgQ, &pvMsg, LP_QUE_READ_TIMEOUT);
        if ( 0 != iErrCode || NULL == pvMsg )
        {
            continue;
        }
        LOG_RUNLOG_DEBUG("LP LPIntLayerRevMsgTask recv a msg");
        switch ( *(( unsigned int *)pvMsg) )
         {
             case CCSIP_CALL:
             {
                 if(LPMsgCntProcessCcsipCall(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE) == 0)
                 {
                	 LPProcessCommand(acCmdBuff);
                 }
                 break;
             }
             case CCSIP_ANSWER:
             {
                 if(LPMsgCntProcessCcsipAnswer(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE) == 0)
                 {
                	 LPProcessCommand(acCmdBuff);
                 }
                 break;
             }
             case CCSIP_TERMINATE:
             {
                 if(LPMsgCntProcessCcsipTerminate(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE) == 0)
                 {
                	 LPProcessCommand(acCmdBuff);
                 }
                 break;
             }
             case CCSIP_CALL_TIMEOUT:
             {
            	 LOG_RUNLOG_DEBUG("***********************************************");
            	 if(g_iCallPreviewFlg == 2)
            	 {
            		 LOG_RUNLOG_DEBUG("####################################################");
            		 g_iCallPreviewFlg = 4;
            	 }
                 if(LPMsgCntProcessCcsipTerminate(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE) == 0)
                 {
                	 LPProcessCommand(acCmdBuff);
                 }

                 break;
             }
             case CCSIP_TERMINATE_ALL:
             {
                 if(LPMsgCntProcessCcsipTerminateAll(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE) == 0)
                 {
                	 LPProcessCommand(acCmdBuff);
                 }
                 break;
             }
             case TMSIP_REGISTER:
             case TMSIP_REGISTER_UPDATE:
             {
            	 //LPMsgSendSipccCallFailure(CALL_FAILURE_REASON_REREGISTER);
                 LPMsgCntProcessTmsipRegister(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE);
                 break;
             }
             case TMSIP_UNREGISTER:
             {
                 if(LPMsgCntProcessTmsipUnRegister(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE) == 0)
                 {
                	 LPProcessCommand(acCmdBuff);
                 }
                 break;
             }
             default:
             {
            	 LOG_RUNLOG_WARN("warn:recv unrecognizable moudle type(%d)", *(( unsigned int *)pvMsg));
                 break;
             }
         }
         free(pvMsg);
         LOG_RUNLOG_DEBUG("acCmdBuff value(%s)\n", acCmdBuff);
    }
}


///**
// * @brief         ��ȡlinphone��Ϣ����(��Linphone�̹߳���)==>20120714�Ѳ�ʹ�øýӿ�
// * @param[in]
// * @return
// */

//static char* LPRecvMsg(void)
//{
//    char acCmdBuff[LP_CMDLINE_BUFF_SIZE] =  { 0x00 };
//    int iErrCode = 0;
//    void* pvMsg = NULL;
//
//    if(NULL == g_pstLPMsgQ)
//    {
//        LOG_RUNLOG_WARN("LP g_pstLPMsgQ is null");
//        return NULL;
//    }
//
//    iErrCode = YKReadQue( g_pstLPMsgQ,  &pvMsg,  LP_QUE_READ_TIMEOUT);
//    if ( 0 != iErrCode || NULL == pvMsg )
//    {
//        //LOG_RUNLOG_WARN("LP YKReadQue error, iErrCode is %d or pvMsg is null\n", iErrCode);
//        return NULL;
//    }
//
//    //LOG_RUNLOG_DEBUG("LP YKReadQue g_pstLPMsgQ");
//    switch ( *(( unsigned int *)pvMsg) )
//    {
//        case CCSIP_CALL:
//        {
//            LPMsgCntProcessCcsipCall(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE);
//            break;
//        }
//        case CCSIP_ANSWER:
//        {
//            LPMsgCntProcessCcsipAnswer(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE);
//            break;
//        }
//        case CCSIP_TERMINATE:
//        {
//            LPMsgCntProcessCcsipTerminate(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE);
//            break;
//        }
//        case CCSIP_TERMINATE_ALL:
//        {
//            LPMsgCntProcessCcsipTerminateAll(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE);
//            break;
//        }
//        case TMSIP_REGISTER:
//        case TMSIP_REGISTER_UPDATE:
//        {
//            LPMsgCntProcessTmsipRegister(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE);
//            break;
//        }
//        case TMSIP_UNREGISTER:
//        {
//            LPMsgCntProcessTmsipUnRegister(pvMsg, acCmdBuff, LP_CMDLINE_BUFF_SIZE);
//            break;
//        }
//        default:
//        {
//            printf("warn:recv unrecognizable moudle type(%d) [LPRecvMsg]\n", *(( unsigned int *)pvMsg));
//            break;
//        }
//    }
//    free(pvMsg);
//    printf("[debug]:return acCmdBuff value[%s]\n", acCmdBuff);
//    return ms_strdup(acCmdBuff);
//}


#ifdef HINT_MUSIC
/**
 * @brief
 *
 * @param[in]
 * @return
 */
static void LPHintMusicDone(LinphoneCore *lc, LinphoneCall *call)
{
	LOG_RUNLOG_DEBUG("LP LPHintMusicDone");
	LPMsgSendSipccCalleeMusicEnd();
}
#endif



/**
 * @brief        Linphone����״̬���
 * @param[in]
 * @return
 */

static void LPCallStatusChanged(LinphoneCore *pstLpCore, LinphoneCall *pstLpCall, LinphoneCallState enCallState)
{
	//char *from=linphone_call_get_remote_address_as_string(pstLpCall);
	char *pcTemp = NULL;
    extern AT_TEST_ST g_stAutoTest;
    //int iIncommingLimit = 0;
    switch (enCallState)
    {
        case LinphoneCallIncomingReceived:
        {
        	long lId = (long)linphone_call_get_user_pointer(pstLpCall);

#ifdef _YK_IMX27_AV_
    		//������ʱ������Ƿ����糬ʱ
			if(g_stSipIncommingTimeoutTimer.pstTimer == NULL)
			{
				g_stSipIncommingTimeoutTimer.pstTimer = YKCreateTimer(LPSipIncommingTimeOutCallback, NULL, SIP_INCOMMING_TIMROUT,
	    					YK_TT_NONPERIODIC, (unsigned long *)&g_stSipIncommingTimeoutTimer.ulMagicNum);
	    		if(NULL == g_stSipIncommingTimeoutTimer.pstTimer)
	    		{
	    			LOG_RUNLOG_ERROR("LP create g_stSipIncommingTimeoutTimer failed!");
	    			iIncommingLimit = 0;
	    		}
	    		else
	    		{
	    			LOG_RUNLOG_INFO("LP g_stSipIncommingTimeoutTimer create!");
	    			iIncommingLimit = 0;
	    		}
			}
			else
			{
				LOG_RUNLOG_WARN("LP g_stSipIncommingTimeoutTimer exist not need to create it!");
				char temp[32] = {0x00};
				sprintf(temp, "terminate %d", lId);
				LPProcessCommand(temp);

				iIncommingLimit = 1;
			}
#endif


        	g_cConnType = CONN_TYPE_AUDIO;
        	LOG_RUNLOG_INFO("LP LinphoneCallIncomingReceived");



#ifndef LP_ONLY
#if _TM_TYPE_ == _YK069_
			TMUpdateAlarmState(TM_ALARM_CALL_STATE, '1'); //SIP����״̬æ��o
#else
			TMUpdateAlarmState(TM_ALARM_CALL_STATE, "1"); //SIP����״̬æ��o
#endif
#endif

            //����������
            osip_from_t *pstFrom =  (osip_from_t *)(pstLpCall->dir==LinphoneCallIncoming ? pstLpCall->log->from :  NULL);
            char acDIsplayName[USER_NAME_BUFF_LEN] = {0X00};
            if(pstFrom != NULL)
            {
            	//sip����
            	if( strcmp(pstFrom->url->scheme, "sip") == 0 )
            	{
            		strcpy(acDIsplayName, pstFrom->url->username);
            		LOG_RUNLOG_DEBUG("LP sip incomming phone:%s", acDIsplayName);
            	}
            	//�绰����
            	else if( strcmp(pstFrom->url->scheme, "tel") == 0 )
            	{
            		strcpy(acDIsplayName, pstFrom->url->string);
            		LOG_RUNLOG_DEBUG("LP tel incomming phone:%s", acDIsplayName);
            	}
            	else
            	{

            		strcpy(acDIsplayName, "scheme error");
            		LOG_RUNLOG_WARN("LP LinphoneCallIncomingReceived scheme error (%s)", acDIsplayName);
            	}
            }
            else
            {
            	strcpy(acDIsplayName, "null from");
            	LOG_RUNLOG_WARN("LP LinphoneCallIncomingReceived from error(%s)", acDIsplayName);
            }
            //char *pcDisplayName =  pstFrom != NULL ? pstFrom->displayname : "null";
            //�ж����������绹����Ƶ����
            //printf("==============================LP: receive call id:%ld\n", lId);
            if(NULL==sal_media_description_find_stream(pstLpCall->op->result, SalProtoRtpAvp, SalVideo)){
                LOG_RUNLOG_DEBUG("LP receive audio call (from:%s)(%s:%s@%s)",  acDIsplayName, "sip", pstFrom->url->username, pstFrom->url->host);
                 //֪ͨCC��Ƶ����
                if(strcmp(pstFrom->url->scheme, "sip") == 0)
                {
                	char acSipInfo[USER_NAME_BUFF_LEN] = {0x00};
                	sprintf(acSipInfo, "sip:%s@%s", pstFrom->url->username, pstFrom->url->host);
                	if( (pcTemp = strchr(acDIsplayName, ';')) != NULL)
                	{
                		*pcTemp = '\0';
                	}
                   	if( (pcTemp = strchr(acSipInfo, ';')) != NULL)
					{
						*pcTemp = '\0';
					}
                   	//if(iIncommingLimit == 1)
                   	//{
                   	//	LPSendOpenDoorResp(acSipInfo, -1);
                   	//}
                   	//else
                   	//{
                   		LPMsgSendSipccAudioCall(acDIsplayName,  lId, acSipInfo);
                   	//}
                }
                else
                {
                	if( (pcTemp = strchr(acDIsplayName, ';')) != NULL)
                	{
                		*pcTemp = '\0';
                	}
                	//if(iIncommingLimit != 1)
                	//{
                		LPMsgSendSipccAudioCall(acDIsplayName,  lId, NULL);
                	//}

                }

                //LPMsgSendSipccVideoCall(acDIsplayName,  lId);
            	//LOG_RUNLOG_DEBUG("LP: receive audio call [from:%s]",  from);
            	//LPMsgSendSipccAudioCall(from,  lId);
            }
            else
            {
            	LOG_RUNLOG_DEBUG("LP receive video call[from:%s]", acDIsplayName);

            	if( (pcTemp = strchr(acDIsplayName, ';')) != NULL)
            	{
            		*pcTemp = '\0';
            	}
               //֪ͨCC��Ƶ����
            	//if(iIncommingLimit != 1)
            	//{
            		//if(audioIncommingFlg == 1) //ʶ��ippbx��������
            		//{
                    //	char acSipInfo[USER_NAME_BUFF_LEN] = {0x00};
                    //	sprintf(acSipInfo, "sip:%s@%s", pstFrom->url->username, pstFrom->url->host);
                    //	LOG_RUNLOG_DEBUG("LP IPPBX audio incomming(%s)", acSipInfo);
                    //	LPMsgSendSipccAudioCall(acDIsplayName,  lId, acSipInfo);
            		//}
            		//else
            		//{
            			LPMsgSendSipccVideoCall(acDIsplayName,  lId);
            		//}
            	//}
            }
            break;
        }
        case LinphoneCallOutgoingInit:
        {
        	g_cConnType = CONN_TYPE_AUDIO;
        	long lId = (long)linphone_call_get_user_pointer(pstLpCall);
        	LOG_RUNLOG_DEBUG("LP LinphoneCallOutgoingInit process_event id   ==> %d", lId);
        	g_lCallId = lId;


//#ifdef CALL_PREVIEW
//	g_iCallPreviewFlg = 1;
//#else
//	g_iCallPreviewFlg = 0;
//#endif
	if(g_cCallPreviewVersion == 0)
	{
		g_iCallPreviewFlg = 0;
	}
	else
	{
		g_iCallPreviewFlg = 1;
	}

			//if(pstLpCore->audio_call_flg == 0)
			//{
				//#ifdef CALL_PREVIEW
					#if defined(YK_XT8126_BV_FKT) || defined(_YK_XT8126_BV_) || defined(_YK_XT8126_AV_) || defined(_YK_XT8126_DOOR_)
								YK8126ResetGaudioPlayPriority();
					#endif
				//#endif
			//}
			//else
			//{
				//LOG_RUNLOG_DEBUG("LP pstLpCore->audio_call_flg != 0");

			//}

#ifndef LP_ONLY
#if _TM_TYPE_ == _YK069_
			TMUpdateAlarmState(TM_ALARM_CALL_STATE, '1'); //SIP����״̬æ��o
#else
			TMUpdateAlarmState(TM_ALARM_CALL_STATE, "1"); //SIP����״̬æ��o
#endif
#endif
//            LPMsgSendSipABOutgoingInit(lId);
            break;
        }
        case LinphoneCallOutgoingProgress:
        {
        	long lId = (long)linphone_call_get_user_pointer(pstLpCall);
        	LOG_RUNLOG_DEBUG("LP LinphoneCallOutgoingProgress process_event id   ==> %d", lId);
        	g_lCallId = lId;

        	LPMsgSendSipccCalleeProcess(lId);

            break;
        }
        case LinphoneCallOutgoingRinging:
        {
        	long lId = (long)linphone_call_get_user_pointer(pstLpCall);
        	LOG_RUNLOG_DEBUG("LP LinphoneCallOutgoingRinging process_event id   ==> %d", lId);
        	g_lCallId = lId;

        	//LOG_RUNLOG_DEBUG("LP LinphoneCallOutgoingRinging");
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_SIP_CALL_RINGING, 0, "");
        	//===========add by hb===========
        	//ATTestOK(AT_TEST_OBJECT_LP);
        	//===============================

            LPMsgSendSipccCalleeRing(lId);

            break;
        }
        case LinphoneCallError:
        {
        	//LOG_RUNLOG_DEBUG("LP LinphoneCallError");

            //LPMsgSendSipccCallFailure();
            break;
        }
        case LinphoneCallOutgoingEarlyMedia:
        {
        	//LOG_RUNLOG_DEBUG("LP LinphoneCallOutgoingEarlyMedia");
            //֪ͨCC�յ�EarlyMedia
            LPMsgSendSipccRecvEarlyMedia();

            break;
        }
        case LinphoneCallConnected:
        {

        	//LPMsgSendSipABStopRing();
        	long lId = (long)linphone_call_get_user_pointer(pstLpCall);
        	LOG_RUNLOG_DEBUG("LP LinphoneCallConnected process_event id   ==> %d", lId);
        	g_enCallDir = linphone_call_get_dir(pstLpCall);
        	//LOG_RUNLOG_DEBUG("LP LinphoneCallConnected call dir %d (outgoing=0;incomming=1)", g_enCallDir);

        	g_iDtmfCount = 0;



            if(NULL==sal_media_description_find_stream(pstLpCall->op->result, SalProtoRtpAvp, SalVideo)){
                 LOG_RUNLOG_DEBUG("LP connect audio");
                 g_cConnType = CONN_TYPE_AUDIO;
             }
            else
            {
            	LOG_RUNLOG_DEBUG("LP connect video");
            	g_cConnType = CONN_TYPE_VIDEO;
            }


			osip_from_t* to =linphone_call_get_remote_address(pstLpCall);
        	char acSipInfo[USER_NAME_BUFF_LEN] = {0x00};
        	sprintf(acSipInfo, "sip:%s@%s", to->url->username, to->url->host);
    		LPMsgUpdateCallRespTo(acSipInfo);
    		g_lCallId = lId;

			if(g_enCallDir == LinphoneCallIncoming)
			{//����
//
				#if defined(YK_XT8126_BV_FKT) || defined(_YK_XT8126_BV_) || defined(_YK_XT8126_AV_) || defined(_YK_XT8126_DOOR_)
					YK8126DownGaudioPlayPriority();
				#endif //#if defined(YK_XT8126_BV_FKT) endif
				android_snd_card_set_playflg(0);
				//֪ͨCC�绰����
				LPMsgSendSipccCalleePickup(lId);
			}
			else
			{//ȥ��
				//#ifdef CALL_PREVIEW
				if(g_cCallPreviewVersion != 0) //!=0 Ԥ��
				{
					g_iCallPreviewFlg = 2;
					if(pstLpCore->audio_call_flg == 0)
					{
						LOG_RUNLOG_DEBUG("LP CALL_PREVIEW video call");
						//��Ԥ������Ƶ����
						LPProcessCmdFunc("mute");
						android_snd_card_set_playflg(0);


					}
					else
					{	//��Ԥ������Ƶ����
						LOG_RUNLOG_DEBUG("LP CALL_PREVIEW audio call");
						#if defined(YK_XT8126_BV_FKT) || defined(_YK_XT8126_BV_) || defined(_YK_XT8126_AV_) || defined(_YK_XT8126_DOOR_)
							YK8126UpGaudioPlayPriority();
						#endif
						//֪ͨCC�绰����
						android_snd_card_set_playflg(1);
						LPMsgSendSipccCalleePickup(lId);
					}
				}
				//#else //#ifdef CALL_PREVIEW else
				else
				{
					LOG_RUNLOG_DEBUG("LP no CALL_PREVIEW audio call");
					//����Ԥ��������Ƶ����
					#if defined(YK_XT8126_BV_FKT) || defined(_YK_XT8126_BV_) || defined(_YK_XT8126_AV_) || defined(_YK_XT8126_DOOR_)
						YK8126UpGaudioPlayPriority();
					#endif
					//֪ͨCC�绰����
					android_snd_card_set_playflg(1);
					LPMsgSendSipccCalleePickup(lId);


					if(g_stStopCallingTimeoutTimer.pstTimer)
					{
						LOG_RUNLOG_DEBUG("LP remove stop calling timer!");
						YKRemoveTimer(g_stStopCallingTimeoutTimer.pstTimer, g_stStopCallingTimeoutTimer.ulMagicNum);
						g_stStopCallingTimeoutTimer.pstTimer = NULL;
						LPProcessCmdFunc("unmute");
						LPMsgSendSipccCalleeMusicEnd();
					}

				//#endif //#ifdef CALL_PREVIEW end
				}

			}





	#ifdef HINT_MUSIC
		//�������û��������ʾ�����ڴ�ֱ��֪ͨ�ϲ���ʾ��������ϡ�
		if(linphone_core_is_hint_music_none(pstLpCall))
		{
			LPMsgSendSipccCalleeMusicEnd();
		}
	#else //HINT_MUSIC
		//LOG_RUNLOG_DEBUG("LP LinphoneCallConnected:hint music done(not define HINT_MUSIC)");
		LPMsgSendSipccCalleeMusicEnd();
	#endif //HINT_MUSIC



#ifndef LP_ONLY
#if _TM_TYPE_ == _YK069_
			TMUpdateAlarmState(TM_ALARM_SIP_CALL, '0');
#else
			TMUpdateAlarmState(TM_ALARM_SIP_CALL, "0");
#endif
#endif

            break;
        }
        case LinphoneCallEnd:
        {
        	long lId = (long)linphone_call_get_user_pointer(pstLpCall);
        	LOG_RUNLOG_INFO("LP LinphoneCallEnd");

			LOG_RUNLOG_DEBUG("LP LinphoneCallEnd event process");
			//if(g_enCallDir == CALL_DIR_OUTGOING)
			//{
			//	//֪ͨCC�绰�Ҷ�
			//	LPMsgSendSipccCalleeHandOff(lId);
			//}


//#ifdef CALL_PREVIEW //LPMsgSendSipccCallFailure(iValue+1000, id);
			if(g_cCallPreviewVersion != 0) //!=0 Ԥ����
			{
				if( g_enCallDir == LinphoneCallOutgoing && ( g_iCallPreviewFlg == 2 || g_iCallPreviewFlg == 4 ) && g_cConnType == CONN_TYPE_VIDEO)
				{
					if(g_iCallPreviewFlg == 2)
					{
						LPMsgSendSipccCallFailure(1021, lId);
					}
				}
				else
				{
					//֪ͨCC�绰�Ҷ�
					LPMsgSendSipccCalleeHandOff(lId);
				}
			}
//#else
			else
			{
				//֪ͨCC�绰�Ҷ�
				LPMsgSendSipccCalleeHandOff(lId);
			}
//#endif

			#ifndef LP_ONLY
			#if _TM_TYPE_ == _YK069_
				TMUpdateAlarmState(TM_ALARM_CALL_STATE, '0');//SIP����״̬���и�o
			#else
				TMUpdateAlarmState(TM_ALARM_CALL_STATE, "0");//SIP����״̬���и�o
			#endif
			#endif

            break;
        }
        default:
            break;
    }
    //ms_free(from);
}


/**
 * @brief         Linphoneע��״̬���
 * @param[in]
 * @return
 */
static void LPRegStatusChanged(LinphoneCore *pstLpCore, LinphoneProxyConfig *pstLpProxyConfig, LinphoneRegistrationState enRegState, const char *pcMsg)
{
    switch (enRegState)
    {
		case LinphoneRegistrationNone:
		{
			//LOG_RUNLOG_DEBUG("LP LPRegStatusChanged LinphoneRegistrationNone");
			break;
		}
		case LinphoneRegistrationProgress:
		{
			//LOG_RUNLOG_DEBUG("LP LPRegStatusChanged LinphoneRegistrationProgress");
			break;
		}
		case LinphoneRegistrationOk:
		{
			//SIPע��״̬�澯�ָ�
			//g_enCurrReg =  REG_STATUS_ONLINE;

			g_enCurrReg = linphonec_is_registered();
			LOG_RUNLOG_INFO("LP LinphoneRegistrationOk:%d", g_enCurrReg);

			if(g_enCurrReg == REG_STATUS_ONLINE)
			{

				LPSetSipRegStatus(REG_STATUS_ONLINE);

				//ע��ɹ�
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_IMS_REGISTER, 0, "");
				LOG_RUNLOG_DEBUG("LP g_enCurrReg=>REG_STATUS_ONLINE(%d))", LPGetSipRegStatus());

				#ifdef  _YK_XT8126_BV_
						Local.cRegFlg = 1;
				#endif

			}

			break;
		}
		case LinphoneRegistrationCleared:
		{
			//���ע����Ϣ
			//g_enCurrReg =  REG_STATUS_OFFLINE;
			g_enCurrReg = linphonec_is_registered();
			LOG_RUNLOG_INFO("LP LinphoneRegistrationCleared:%d", g_enCurrReg);

			if(g_enCurrReg == REG_STATUS_OFFLINE)
			{
				LPSetSipRegStatus(REG_STATUS_OFFLINE);
				LOG_RUNLOG_DEBUG("LP g_enCurrReg=>REG_STATUS_OFFLINE(%d))", LPGetSipRegStatus());
				//ע��ʧ��
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_IMS_REGISTER, 1, "");
				#ifdef  _YK_XT8126_BV_
						Local.cRegFlg = 0;
				#endif
			}

			//TMUpdateAlarmState(TM_ALARM_IMS_REGISTER, 1);
			break;
		}
		case LinphoneRegistrationFailed:
		{
			//SIPע��״̬�澯����
			//g_enCurrReg =  REG_STATUS_OFFLINE;
			g_enCurrReg = linphonec_is_registered();
			LOG_RUNLOG_INFO("LP LinphoneRegistrationFailed:%d", g_enCurrReg);

			if(g_enCurrReg == REG_STATUS_OFFLINE)
			{
				LPSetSipRegStatus(REG_STATUS_OFFLINE);


				//ע��ʧ��
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_IMS_REGISTER, 1, "");
				LOG_RUNLOG_DEBUG("LP g_enCurrReg=>REG_STATUS_OFFLINE(%d))", LPGetSipRegStatus());

				#ifdef  _YK_XT8126_BV_
						Local.cRegFlg = 0;
				#endif
			}
			break;
		}
		default:
			break;
	}
}

/**
 * @brief          Linphoneע��״̬(��ע���δע��)
 * @param[in]
 * @return
 */
static void LPRegFailureOrSuccess(LinphoneCore *pstLpCore, int iStatus, const char* pcCode)
{
    /* TRUE��ע��ɹ���FALSEע��ʧ��.�ڴ˽��и澯������ָ�. */
	if(iStatus)
    {
    	LOG_RUNLOG_DEBUG("LP LPRegFailureOrSuccess reg success");
    }
    else
    {
    	LOG_RUNLOG_DEBUG("LP LPRegFailureOrSuccess reg failed(error code %d)", pcCode);
    }
}

/**
 * @brief        Linphone����ʧ��
 * @param[in]
 * @return
 */
static void LPCallFailure(LinphoneCore *pstLpCore, LinphoneCall *pstCall, SalError enError, SalReason enReason, int iCode)
{
        long id = (long)linphone_call_get_user_pointer(pstCall);

        if (SalErrorNoResponse == enError)
        {
#ifdef LP_ONLY
        	LOG_RUNLOG_DEBUG("LP LPCallFailure(SalErrorNoResponse)");
#endif
        }
        else if (SalErrorProtocol == enError)
        {
#ifdef LP_ONLY
        	LOG_RUNLOG_DEBUG("LP LPCallFailure(SalErrorProtocol)");
#endif
        }
        else if (SalErrorFailure == enError)
        {
            switch(enReason)
            {
                case SalReasonDeclined:
                	LOG_RUNLOG_INFO("LP LPCallFailure(SalReasonDeclined/%d)", iCode);
                    break;
                case SalReasonBusy:
                	LOG_RUNLOG_INFO("LP LPCallFailure(SalReasonBusy/%d)", iCode);
                    break;
                case SalReasonRedirect:
                	LOG_RUNLOG_INFO("LP LPCallFailure(SalReasonRedirect/%d)", iCode);
                    break;
                case SalReasonTemporarilyUnavailable:
                	LOG_RUNLOG_INFO("LP LPCallFailure(SalReasonTemporarilyUnavailable/%d)", iCode);
                    break;
                case SalReasonNotFound:
                	LOG_RUNLOG_INFO("LP LPCallFailure(SalReasonNotFound/%d)", iCode);
                    break;
                case SalReasonDoNotDisturb:
                	LOG_RUNLOG_DEBUG("LP LPCallFailure(SalReasonDoNotDisturb/%d)", iCode);
                    break;
                case SalReasonMedia:
                	LOG_RUNLOG_INFO("LP LPCallFailure(SalReasonMedia/%d)", iCode);
                    break;
                default:
                	LOG_RUNLOG_INFO("LP LPCallFailure(errcode:%d)", iCode);
                    break;
            }

        		LOG_RUNLOG_DEBUG("LP LPCallFailure event process");
				//֪ͨCC����ʧ��
				LPMsgSendSipccCallFailure(iCode, id);

        }
}


static void LPStopCallingOutCallback(void *pvPara)
{
	LOG_RUNLOG_DEBUG("LP LPStopCallingOutCallback");
	g_stStopCallingTimeoutTimer.pstTimer = NULL;

	LPProcessCommand("terminate all");
	LPMsgSendSipccStopcalling();
}

static void LPDtmfRecvTimeOutCallback(void* pvPara)
{
	//YKRemoveTimer(g_stDtmfRecvTimeoutTimer.pstTimer, g_stDtmfRecvTimeoutTimer.ulMagicNum);
	g_stDtmfRecvTimeoutTimer.pstTimer = NULL;
	LOG_RUNLOG_DEBUG("LP LPDtmfRecvTimeOutCallback");
#ifdef _YK_IMX27_AV_
	if(SMGetOpendoorResult() == 1)				//dtmf���ųɹ�
	{
		if(g_stCurSipMsgInfo.iRcvMsgFlg == 1)
		{
			LPMsgSendDoorOpenResp(g_stCurSipMsgInfo.acSipInfo, g_stCurSipMsgInfo.iMsgSeq, 0);
			memset(&g_stCurSipMsgInfo, 0x00, sizeof(ST_SIP_MSG_INFO));
		}
	}
	else if(SMGetOpendoorResult() == 2)			//dtmf����ʧ��
	{
		if(g_stCurSipMsgInfo.iRcvMsgFlg == 1)
		{
			LPMsgSendDoorOpenResp(g_stCurSipMsgInfo.acSipInfo, g_stCurSipMsgInfo.iMsgSeq, -1);
			memset(&g_stCurSipMsgInfo, 0x00, sizeof(ST_SIP_MSG_INFO));
		}
	}
	else if(SMGetOpendoorResult() == 3)			//msg���ųɹ�
	{
		if(g_stCurSipMsgInfo.iRcvDtmfFlg == 1)
		{
			LPSendDtmf('#');
			memset(&g_stCurSipMsgInfo, 0x00, sizeof(ST_SIP_MSG_INFO));
		}
	}
	else if(SMGetOpendoorResult() == 4)			//msg����ʧ��
	{
		if(g_stCurSipMsgInfo.iRcvDtmfFlg == 1)
		{
			memset(&g_stCurSipMsgInfo, 0x00, sizeof(ST_SIP_MSG_INFO));
		}
	}
#endif

//#ifdef _YK_XT8126_
//	if(g_stCurSipMsgInfo.iRcvMsgFlg == 1)
//	{
//		LOG_RUNLOG_DEBUG("LP send msg.");
//		LPMsgSendDoorOpenResp(g_stCurSipMsgInfo.acSipInfo, g_stCurSipMsgInfo.iMsgSeq, 0);
//		memset(&g_stCurSipMsgInfo, 0x00, sizeof(ST_SIP_MSG_INFO));
//	}
//	else if(g_stCurSipMsgInfo.iRcvDtmfFlg ==1)
//	{
//		LOG_RUNLOG_DEBUG("LP send dtmf.");
//		LPSendDtmf('#');
//		memset(&g_stCurSipMsgInfo, 0x00, sizeof(ST_SIP_MSG_INFO));
//	}
//#endif
//
//
//	if(g_stCurSipMsgInfo.iRcvMsgFlg == 1)
//	{
//		LOG_RUNLOG_DEBUG("LP send msg.");
//		LPMsgSendDoorOpenResp(g_stCurSipMsgInfo.acSipInfo, g_stCurSipMsgInfo.iMsgSeq, 0);
//		memset(&g_stCurSipMsgInfo, 0x00, sizeof(ST_SIP_MSG_INFO));
//	}
//	else if(g_stCurSipMsgInfo.iRcvDtmfFlg ==1)
//	{
//		LOG_RUNLOG_DEBUG("LP send dtmf.");
//		LPSendDtmf('#');
//		memset(&g_stCurSipMsgInfo, 0x00, sizeof(ST_SIP_MSG_INFO));
//	}
}

/**
 * @brief        Linphone�յ�DTMF�ź�
 * @param[in]
 * @return
 */

static void LPDtmfRecv(LinphoneCore *pstLpCore, LinphoneCall *pstLpCall, int iDtmf)
{

	g_iDtmfCount++;
	LOG_RUNLOG_DEBUG("LP LPDtmfRecv(%d:0X%X)", iDtmf, g_stDtmfRecvTimeoutTimer.pstTimer);

    if(g_iDtmfCount >= LP_DTMF_MAX_COUNT)
    {
    	LOG_RUNLOG_WARN("LP LPDtmfRecv too many time[%d], so terminate all", g_iDtmfCount);
    	LPProcessCommand("terminate all");
    	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_SYSTEM_ERROR, 0, "104");
    	return;
    }

    if(pstLpCore->audio_call_flg == 0)
    {
//#ifdef CALL_PREVIEW
    	if(g_cCallPreviewVersion != 0)//!=0 Ԥ����
    	{
			if(iDtmf == '9' && g_enCallDir == LinphoneCallOutgoing)
			{
				g_iCallPreviewFlg = 3;
				LPSendDtmf(iDtmf);

				if(g_stStopCallingTimeoutTimer.pstTimer)
				{
					LOG_RUNLOG_DEBUG("LP remove stop calling timer!");
					YKRemoveTimer(g_stStopCallingTimeoutTimer.pstTimer, g_stStopCallingTimeoutTimer.ulMagicNum);
					g_stStopCallingTimeoutTimer.pstTimer = NULL;
				}


	#if defined(YK_XT8126_BV_FKT) || defined(_YK_XT8126_BV_) || defined(_YK_XT8126_AV_) || defined(_YK_XT8126_DOOR_)
				YK8126UpGaudioPlayPriority();
	#endif
			   //֪ͨCC�绰����
				android_snd_card_set_playflg(1);
				LPMsgSendSipccCalleePickup(g_lCallId);
				LPProcessCmdFunc("unmute");
				#ifdef HINT_MUSIC
							//�������û��������ʾ�����ڴ�ֱ��֪ͨ�ϲ���ʾ��������ϡ�
							if(linphone_core_is_hint_music_none(pstLpCall))
							{
								LPMsgSendSipccCalleeMusicEnd();
							}
				#else
							//LOG_RUNLOG_DEBUG("LP LinphoneCallConnected:hint music done(not define HINT_MUSIC)");
							LPMsgSendSipccCalleeMusicEnd();
				#endif
			}
//#endif
    	}

    }
    else
    {
    	LOG_RUNLOG_DEBUG("LP CALL_PREVIEW on but audio call");
    }

#ifdef DTMF_RESP
    if(sal_media_description_find_stream(pstLpCall->op->result, SalProtoRtpAvp, SalVideo))
    {
    	g_cConnType = CONN_TYPE_VIDEO;
     	LOG_RUNLOG_INFO("LP video call recv dtmf %c", iDtmf);
		//֪ͨCC DTMF����
		if(iDtmf == '#')
		{
			g_stCurSipMsgInfo.iRcvDtmfFlg = 1;
			if(g_stDtmfRecvTimeoutTimer.pstTimer == NULL)
			{
				g_stDtmfRecvTimeoutTimer.pstTimer = YKCreateTimer(LPDtmfRecvTimeOutCallback, NULL, DTMF_RECV_TIMROUT,
						YK_TT_NONPERIODIC, (unsigned long *)&g_stDtmfRecvTimeoutTimer.ulMagicNum);
				if(NULL == g_stDtmfRecvTimeoutTimer.pstTimer)
				{
					LOG_RUNLOG_ERROR("LP create dtmf recv timer failed!");
					return;
				}


				LPMsgSendSipccDtmf(iDtmf);
#ifndef _YK_IMX27_AV_
				LPSendDtmf(iDtmf);
#endif
				g_stCurSipMsgInfo.iRcvDtmfFlg = 0;

				LOG_RUNLOG_DEBUG("LP ***************************************************  111111111 ");
			}
			//timer exist and status isn't stop
			else if( g_stDtmfRecvTimeoutTimer.pstTimer)// && g_stDtmfRecvTimeoutTimer.pstTimer->enStatus == YK_TS_RUNNING
			{
#ifndef _YK_IMX27_AV_
				LPSendDtmf(iDtmf);
#endif
				LOG_RUNLOG_DEBUG("LP recv dtmf %c, but no need to open door(%d)", iDtmf, g_stDtmfRecvTimeoutTimer.pstTimer->enStatus);
			}
#ifdef _GYY_I6_
			if((CCGetCcStatus() == 0x03))
#else
			if((CCGetCcStatus() == 0x03) || (CCGetCcStatus() == 0x05))
#endif
			{
				LOG_RUNLOG_DEBUG("LP rev DTMF, cc status(calling), create stop calling timer(%x).", g_stStopCallingTimeoutTimer.pstTimer);
				if(!g_stStopCallingTimeoutTimer.pstTimer)
				{
					g_stStopCallingTimeoutTimer.pstTimer = YKCreateTimer(LPStopCallingOutCallback, NULL, STOP_CALLING_TIMEOUT,
											YK_TT_NONPERIODIC, (unsigned long *)&g_stStopCallingTimeoutTimer.ulMagicNum);
					if(NULL == g_stStopCallingTimeoutTimer.pstTimer)
					{
						LOG_RUNLOG_ERROR("LP create stop calling timer failed!");
						return;
					}
				}
			}
		}
		else
		{
			LPMsgSendSipccDtmf(iDtmf);
		}
    }
    else
    {
    	g_cConnType = CONN_TYPE_AUDIO;
     	LOG_RUNLOG_INFO("LP audio call recv dtmf %c", iDtmf);
		//֪ͨCC DTMF����
		if(iDtmf == '#')
		{
			if(g_stDtmfRecvTimeoutTimer.pstTimer == NULL)
			{
				g_stDtmfRecvTimeoutTimer.pstTimer = YKCreateTimer(LPDtmfRecvTimeOutCallback, NULL, DTMF_RECV_TIMROUT,
						YK_TT_NONPERIODIC, (unsigned long *)&g_stDtmfRecvTimeoutTimer.ulMagicNum);
				if(NULL == g_stDtmfRecvTimeoutTimer.pstTimer)
				{
					LOG_RUNLOG_ERROR("LP create dtmf recv timer failed!");
					return;
				}

				LPMsgSendSipccDtmf(iDtmf);

				//LPSendDtmf(iDtmf);
			}
			//timer exist and status isn't stop
			else if( g_stDtmfRecvTimeoutTimer.pstTimer)// && g_stDtmfRecvTimeoutTimer.pstTimer->enStatus == YK_TS_RUNNING
			{
				LOG_RUNLOG_DEBUG("LP recv dtmf %c, but no need to open door(%d)", iDtmf, g_stDtmfRecvTimeoutTimer.pstTimer->enStatus);
			}
		}
		else
		{
			LPMsgSendSipccDtmf(iDtmf);
		}
    }
#else
    LPMsgSendSipccDtmf(iDtmf);
#endif
}






/**
 * @brief       Linphone�յ�message�ź�
 * @param[in]
 * @return
 */
static void LPMessageRecv(LinphoneCore *lc, const LinphoneAddress *from, const char *pcMsg)
{
	const osip_from_t* osip_from = from;

	void* pvMsg = NULL;
	sd_domnode_t* pstRootNode = NULL;
	FILE *msgFile = NULL;
	LOG_RUNLOG_DEBUG("LP LPMessageRecv msgcnt:\n%s", pcMsg);

	//system("rm /tmp/sip_message -rf");
	if((msgFile = fopen("/data/data/com.fsti.ladder/sip_message", "w+") ) == NULL)
	{
		perror("LP LPMessageRecv");
		return;
	}
	LOG_RUNLOG_DEBUG("LP sip message content length:%d\n", fwrite(pcMsg, 1, strlen(pcMsg), msgFile));
	fclose(msgFile);
	system("busybox sed -i \"s/%61/=/g\" /data/data/com.fsti.ladder/sip_message");

	if((msgFile = fopen("/data/data/com.fsti.ladder/sip_message", "r") ) == NULL)
	{
		perror("LP LPMessageRecv");
		return;
	}
	//LOG_RUNLOG_DEBUG("LP sip message content length:%d\n", fwrite(pcMsg, 1, strlen(pcMsg), msgFile));
	//fseek(msgFile, 0, SEEK_SET);
	pstRootNode = LPUtilsMessageXmlLoadUseFile(msgFile);
	fclose(msgFile);


	if(pstRootNode == NULL)
	{
		return;
	}
	pvMsg = LPUtilsAnalyzeNode(pstRootNode);
	sd_domnode_delete(pstRootNode);
	if(pvMsg == NULL)
	{
		return;
	}
	switch ( *(( char*)pvMsg) )
	 {
		 case SIP_MSG_TYPE_MESSAGE:
		 {
			 LOG_RUNLOG_DEBUG("LP LPMessageRecv SIP_MSG_TYPE_MESSAGE");


			 if(strcmp( ((SIP_MESSAGE_OC_ST* )pvMsg)->acServiceType, "ACCESS_MSG") == 0 )
			 {
				 	 LOG_RUNLOG_DEBUG("LP LPMessageRecv ACCESS_MSG");
					char acSipInfo[USER_NAME_BUFF_LEN] = {0x00};
					sprintf(acSipInfo, "sip:%s@%s", osip_from->url->username, osip_from->url->host);

					g_stCurSipMsgInfo.iRcvMsgFlg = 1;
					strcpy(g_stCurSipMsgInfo.acSipInfo, acSipInfo);
					g_stCurSipMsgInfo.iMsgSeq = ((SIP_MESSAGE_OC_ST*)pvMsg)->iMsgSeq;

					if(g_stDtmfRecvTimeoutTimer.pstTimer == NULL)
					{
						g_stDtmfRecvTimeoutTimer.pstTimer = YKCreateTimer(LPDtmfRecvTimeOutCallback, (void *)pvMsg, DTMF_RECV_TIMROUT,
								YK_TT_NONPERIODIC, (unsigned long *)&g_stDtmfRecvTimeoutTimer.ulMagicNum);
						if(NULL == g_stDtmfRecvTimeoutTimer.pstTimer)
						{
							LOG_RUNLOG_ERROR("LP create msg recv timer failed!");
							return;
						}
						LPMsgSendSipccMessage(osip_from->url->username, acSipInfo, ((SIP_MESSAGE_OC_ST*)pvMsg)->iMsgSeq);
						g_stCurSipMsgInfo.iRcvMsgFlg = 0;
						LOG_RUNLOG_DEBUG("LP ***************************************************");
					}
					else if( g_stDtmfRecvTimeoutTimer.pstTimer)// && g_stDtmfRecvTimeoutTimer.pstTimer->enStatus == YK_TS_RUNNING
					{
#ifndef _YK_IMX27_AV_
						LPMsgSendDoorOpenResp(acSipInfo, ((SIP_MESSAGE_OC_ST*)pvMsg)->iMsgSeq, 0);
#endif
						LOG_RUNLOG_DEBUG("LP recv msg, but no need to open door(%d)", g_stDtmfRecvTimeoutTimer.pstTimer->enStatus);
					}
#ifdef _GYY_I6_
					if((CCGetCcStatus() == 0x03))
#else
					if((CCGetCcStatus() == 0x03) || (CCGetCcStatus() == 0x05))
#endif
					{
						LOG_RUNLOG_DEBUG("LP rev msg, cc status(calling), create stop calling timer.(%x)", g_stStopCallingTimeoutTimer.pstTimer);
						if(!g_stStopCallingTimeoutTimer.pstTimer)
						{
							g_stStopCallingTimeoutTimer.pstTimer = YKCreateTimer(LPStopCallingOutCallback, NULL, STOP_CALLING_TIMEOUT,
													YK_TT_NONPERIODIC, (unsigned long *)&g_stStopCallingTimeoutTimer.ulMagicNum);
							if(NULL == g_stStopCallingTimeoutTimer.pstTimer)
							{
								LOG_RUNLOG_ERROR("LP create stop calling timer failed!");
								return;
							}
						}
					}
			 }
			 else if(strcmp( ((SIP_MESSAGE_OC_ST* )pvMsg)->acServiceType, "CALL_MSG") == 0 )
			 {//call preview pick up really
			 	 LOG_RUNLOG_DEBUG("LP LPMessageRecv CALL_MSG");
				 if(g_stStopCallingTimeoutTimer.pstTimer)
				 {
					 LOG_RUNLOG_DEBUG("LP remove stop calling timer!");
					 YKRemoveTimer(g_stStopCallingTimeoutTimer.pstTimer, g_stStopCallingTimeoutTimer.ulMagicNum);
					 g_stStopCallingTimeoutTimer.pstTimer = NULL;
				 }
				 if(lc->audio_call_flg == 0)
				 {
//					#ifdef CALL_PREVIEW
					 if(g_cCallPreviewVersion != 0) //!=0 Ԥ��
					 {
					 	 	 g_iCallPreviewFlg = 3;
							if (linphone_core_get_calls(lc)==NULL){
								LOG_RUNLOG_DEBUG("LP No active calls so call resp is err");
								LPMsgSendCallResp( ((SIP_MESSAGE_OC_ST* )pvMsg)->iMsgSeq, -1);
								break;
							}
							if(g_enCallDir == LinphoneCallOutgoing)
							{
								//#ifdef _YK_XT8126_
								#if defined(YK_XT8126_BV_FKT) || defined(_YK_XT8126_BV_) || defined(_YK_XT8126_AV_) || defined(_YK_XT8126_DOOR_)
									YK8126UpGaudioPlayPriority();
								#endif
								//֪ͨCC�绰����
								android_snd_card_set_playflg(1);
								LPMsgSendSipccCalleePickup(g_lCallId);
								LPProcessCmdFunc("unmute");

					//#ifdef HINT_MUSIC
					//			//������û��������ʾ�����ڴ�ֱ��֪ͨ�ϲ���ʾ��������ϡ�?
					//			if(linphone_core_is_hint_music_none(pstLpCall))
					//			{
					//				LPMsgSendSipccCalleeMusicEnd();
					//			}
					//#else
								LPMsgSendSipccCalleeMusicEnd();

								LPMsgSendCallResp( ((SIP_MESSAGE_OC_ST* )pvMsg)->iMsgSeq, 0);
							}

					//#endif //#ifdef CALL_PREVIEW
					 }
				 }
			 }
			 break;
		 }
#ifdef _YK_GYY_
		 case SIP_MSG_TYPE_NOTIFY:
		 {
			 LOG_RUNLOG_DEBUG("LP LPMessageRecv SIP_MSG_TYPE_NOTIFY");
			 if( !strcmp( ((SIP_MESSAGE_DOORPLATFORM_ST *)pvMsg)->acMsgType, "accesssoftupgrade") )
			 {
				 LOG_RUNLOG_DEBUG("LP notify tm exec accesssoftupgrade");
				 LPMsgSendSipTmSoftUpgrade();
			 }
			 else if( !strcmp( ((SIP_MESSAGE_DOORPLATFORM_ST *)pvMsg)->acMsgType, "accessconfigupdate") )
			 {
				 LOG_RUNLOG_DEBUG("LP notify tm exec accessconfigupdate");
				 LPMsgSendSipTmConfigUpdate();
			 }
			 else if( !strcmp( ((SIP_MESSAGE_DOORPLATFORM_ST *)pvMsg)->acMsgType, "accesspwdupdate") )
			 {
				 LOG_RUNLOG_DEBUG("LP notify tm exec accesspwdupdate");
				 LPMsgSendSipTmPwdUpdate();
			 }
			 else if( !strcmp( ((SIP_MESSAGE_DOORPLATFORM_ST *)pvMsg)->acMsgType, "accesscardupdate") )
			 {
				 LOG_RUNLOG_DEBUG("LP notify tm exec accesscardupdate");
				 LPMsgSendSipTmCardUpdate();
			 }
			 else
			 {
				 LOG_RUNLOG_DEBUG("LP recv unrecognizable acMsgType(%s)", ((SIP_MESSAGE_DOORPLATFORM_ST *)pvMsg)->acMsgType);
			 }
			 break;
		 }
#endif
		 default:
		 {
			 LOG_RUNLOG_DEBUG("LP recv unrecognizable msg type(%d)", *(( char *)pvMsg));
			 break;
		 }
	 }
	 free(pvMsg);
}








//add by chensq 20121109
static void LPProcessQ850(char *value, char *note, long id)
{
	int iValue = atoi(value);
	LOG_RUNLOG_DEBUG("LP process_q850 value=%s(%s)", value, note);
	int iErr = 0;
	if(iValue == 52 || iValue == 54 || iValue == 57 || iValue == 52)
	{
		//OUTGOING_CALL_BARRED //INCOMING_CALL_BARRED
		//BEARERCAPABILITY_NOTAUTH //OUTGOING_CALL_BARRED
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "403");
		iErr = 1;
	}
	else if(iValue == 1 || iValue == 2 || iValue == 3)
	{
		//UNALLOCATED_NUMBER //NO_ROUTE_TRANSIT_NET //NO_ROUTE_DESTINATION
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "404");
		iErr = 1;
	}
	else if(iValue == 18)
	{
		//NO_USER_RESPONSE
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "408");
		iErr = 1;
	}
	else if(iValue == 22 || iValue == 23)
	{
		//NUMBER_CHANGED //REDIRECTION_TO_NEW_DESTINATION
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "410");
		iErr = 1;
	}
	else if(iValue == 19 || iValue == 20 || iValue == 31)
	{
		//NO_ANSWER //SUBSCRIBER_ABSENT
		//NORMAL_UNSPECIFIED
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "480");
		iErr = 1;
	}
	else if(iValue == 25)
	{
		//EXCHANGE_ROUTING_ERROR
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "483");
		iErr = 1;
	}
	else if(iValue == 28)
	{
		//INVALID_NUMBER_FORMAT
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "484");
		iErr = 1;
	}
	else if(iValue == 17)
	{
		//USER_BUSY
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "486");
		iErr = 1;
	}
	else if(iValue == 487)
	{
		//ORIGINATOR_CANCEL
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "487");
		iErr = 1;
	}
	else if(iValue == 85 || iValue == 88)
	{
		//BEARERCAPABILITY_NOTIMPL //INCOMPATIBLE_DESTINATION
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "488");
		iErr = 1;
	}

	else if(iValue == 69 || iValue == 79 || iValue == 29)
	{
		//FACILITY_NOT_IMPLEMENTED //SERVICE_NOT_IMPLEMENTED //FACILITY_REJECTED
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "501");
		iErr = 1;
	}
	else if(iValue == 27)
	{
		//DESTINATION_OUT_OF_ORDER
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "502");
		iErr = 1;
	}
	else if(iValue == 34 || iValue == 38 || iValue == 41 || iValue == 42 || iValue == 44 || iValue == 58)
	{
		//NORMAL_CIRCUIT_CONGESTION //NETWORK_OUT_OF_ORDER
		//NORMAL_TEMPORARY_FAILURE //SWITCH_CONGESTION
		//REQUESTED_CHAN_UNAVAIL //BEARERCAPABILITY_NOTAVAIL
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "503");
		iErr = 1;
	}
	else if(iValue == 102)
	{
		//RECOVERY_ON_TIMER_EXPIRE
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "504");
		iErr = 1;
	}

	else if(iValue == 21)
	{
		//CALL_REJECTED
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "603");
		iErr = 1;
	}
	else
	{
		LOG_RUNLOG_DEBUG("LP Q.850Code=%d(SipEquiv=%s)", iValue, "unkown");
	}

	if(iErr == 1)
	{
//		g_cProcessEndFlg = 0;
        //֪ͨCC����ʧ��
        LPMsgSendSipccCallFailure(iValue+1000, id);
	}
}





//add by chensq 20121109
static bool_t LPProcessEventOther(Sal *sal, eXosip_event_t *ev)
{
	SalOp *op=find_op(sal,ev);
	long id = 0;
	if (op==NULL)
	{
		LOG_RUNLOG_DEBUG("LP LPProcessEventOther error op is null");
	}
	else
	{

	}

	if(ev->request && MSG_IS_REQUEST(ev->request))
	{
		LOG_RUNLOG_INFO("LP process_event req  ==> %s %s@%s",
				ev->request->sip_method,
				ev->request->req_uri->username,
				ev->request->req_uri->host);
	}

	if(ev->response && MSG_IS_RESPONSE(ev->response))
	{
		int status_code = ev->response->status_code;

		LOG_RUNLOG_INFO("LP process_event_resp ==> %d %s", status_code, ev->response->reason_phrase);

		if( ev->request && MSG_IS_REQUEST(ev->request) && (strcmp(ev->request->sip_method, "REGISTER") == 0))
		{
			LOG_RUNLOG_DEBUG("LP process_event_resp REGISTER recv %d", status_code);
			if( ( g_enSipRegCheckFlg == SIP_REG_CHECK_WAIT_401_407) )
			{
				if(status_code == 401 || status_code == 407)
				{
					g_enSipRegCheckFlg = SIP_REG_CHECK_RECV_401_407;
				}
				else if(status_code == 403 || status_code == 603 || status_code == 200)
				{
					g_enSipRegCheckFlg = SIP_REG_CHECK_RECV_403_603_200;
				}
			}
		}


		if(status_code > 100)
		{

	#ifdef PRACK_SEND
				//LPC call 18965796616 --audio-only
				osip_header_t *evt_hdr = NULL;
				static int relCount = 0;
				osip_message_header_get_byname(ev->response, "Require", 0, &evt_hdr);
				if(evt_hdr != NULL)
				{
					LOG_RUNLOG_DEBUG("LP Require %s(%s)", evt_hdr->hname, evt_hdr->hvalue);
					char *tmp1 = strstr(evt_hdr->hvalue, "100rel");
					if(tmp1 != NULL)
					{
						//relCount++;
						osip_message_t *prack=NULL;
						printf("build prack result : %d \n", eXosip_call_build_prack(ev->tid,&prack));;
						printf("send prack result : %d \n", eXosip_call_send_prack(ev->tid,prack));
						LOG_RUNLOG_INFO("LP send method %s", "PRACK");

					}
				}
	#endif
		}

		if(status_code == 180)
		{
			LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_SIP_CALL_RINGING, 0, "");
        	//===========add by hb===========
        	ATTestOK(AT_TEST_OBJECT_LP);
        	//===============================
		}
		else if (status_code == 183)
		{
			LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
			if( call == NULL )
			{
				LOG_RUNLOG_DEBUG("LP call is null LPProcessEventOther");
				return -1;
			}
			id = (long)linphone_call_get_user_pointer(call);
			LOG_RUNLOG_DEBUG("LP process_event id   ==> %d", id);

			osip_header_t *evt_hdr = NULL;
			osip_message_header_get_byname(ev->response, "reason", 0, &evt_hdr);
			if(evt_hdr != NULL)
			{
				LOG_RUNLOG_DEBUG("LP 183 %s(%s)", evt_hdr->hname, evt_hdr->hvalue);
				//Q.850���� Reason: Q.850;cause=16;text="Normal call clearing"
				char *tmp1 = strstr(evt_hdr->hvalue, ";cause=");
				char *tmp2 = strstr(evt_hdr->hvalue, ";text=");
				int len = strlen(";cause=");
				char value[5] = {0x00};
				strncpy(value, tmp1+len, tmp2-tmp1-len);
				LPProcessQ850(value, tmp2+strlen(";text="), id);
			}
		}

	}
}


#ifdef _YK_XT8126_BV_
void LPReRegisterForBV()
{
	if( g_iRunLPTaskFlag == TRUE )
	{
		LPProcessCommand("unregister");
		sleep(2);
		LPProcessCommand("register");
	}
	else
	{
		LOG_RUNLOG_DEBUG("LP LPReRegisterForBV");
	}
}
#endif




/**
 * @brief        Linphoneģ����Դ��ʼ��
 * @param[in]    ��
 * @return       0:�ɹ�  <0:ʧ��
 */
int LPInit(void) {
	LinphoneCore *lp_core;

	pthread_mutex_init(&g_lpLock, NULL);

	//ע�����״̬����ص�����
	linphonec_set_call_state_changed_cb(LPCallStatusChanged);
	//ע��ע��״̬����ص�����
	linphonec_set_reg_state_changed_cb(LPRegStatusChanged);
	//ע��DTMF���ջص�����
	linphonec_set_dtmf_recv_cb(LPDtmfRecv);
	//ע�����ʧ�ܻص�����
	linphonec_set_call_failure_cb(LPCallFailure);

	//ע�������Ϣ����ص�����==>20120714��Ϊ�����̴߳���
	//linphonec_set_msg_handle_support(LPRecvMsg);

#ifdef HINT_MUSIC
	//ע����������ʾ���������֪ͨ
	linphonec_set_hint_music_done_cb(LPHintMusicDone);
	LOG_RUNLOG_DEBUG("LP define HINT_MUSIC: LPInit exec linphonec_set_hint_music_done_cb");
#endif

//#ifndef LP_ONLY
//	//ע��Ĭ��ע�᷽ʽ����ص�����
//	linphonec_set_default_reg(LPFirstRegister);
//#endif

	//linphonec_set_reg_okfailed_notify_cb(LPRegFailureOrSuccess);

//#ifdef _YK_GYY_
	//add by chensq 20121022 for gyy
	linphonec_set_message_recv_cb(LPMessageRecv);
//#endif

	//add by chensq 20121112
	linphonec_set_event_process_cb(LPProcessEventOther);


	//�ؽ������ļ�
	if ( !LPUtilsReCreateFile(LP_CONFIG_NAME) )
	{
		LOG_RUNLOG_ERROR("LP LPUtilsReCreateFile failed(%s)", LP_CONFIG_NAME);
		return -1;
	}
	else
	{
	    setenv("LP_CONFIG_PATH", LP_CONFIG_NAME, 1);
	}

	//����Linphone
	lp_core = linphonec_startup();

	if (!lp_core) {
		LOG_RUNLOG_ERROR("LP linphonec_startup failed");
		return -1;
	}




	g_pstLPMsgQ = YKCreateQue(LP_QUE_LEN);
	if( NULL == g_pstLPMsgQ )
	{
	    LOG_RUNLOG_ERROR("LP YKCreateQue failed (LPInit)\n");
	    return -1;
	}
	g_iRunLPTaskFlag = TRUE;
	g_LPThreadId = YKCreateThread(LPIntLayerRevMsgTask, NULL);

    if(NULL == g_LPThreadId)
    {
    	LOG_RUNLOG_ERROR("LP YKCreateThread LPIntLayerRevMsgTask failed");
        return -1;
    }


	LOG_RUNLOG_DEBUG("LP linphonec_startup success");


	return 0;
}



/**
 * @brief        Linphone��Դ����
 * @param[in]    ��
 * @return       ��
 */

void LPFini(void)
{
    g_iRunLPTaskFlag = FALSE;
    if(NULL != g_LPThreadId)
    {
        YKJoinThread(g_LPThreadId);
        YKDestroyThread(g_LPThreadId);
        LOG_RUNLOG_DEBUG("LP LPFini YKJoinThread and YKDestroyThread");
    }

	LPProcessCommand("terminate all");
	//sleep(3);
	LPProcessCommand("unregister");
	//sleep(3);
    LOG_RUNLOG_DEBUG("LP LPFini will linphonec_shutdown ... ");

    //�ر�linphoneִ�����߳�
    linphonec_shutdown();

    YKDeleteQue(g_pstLPMsgQ);

    pthread_mutex_destroy(&g_lpLock);

    LOG_RUNLOG_DEBUG("LP LPFini success");
}




//int LPRestart(void)
//{
//
//	LOG_RUNLOG_WARN("LP LPRestart begin");
//
//	LPFini();
//
//	if (-1 != LPInit())
//	{
//		LOG_RUNLOG_ERROR("LP LPRestart error");
//		//system("reboot");
//		return -1;
//	}
//
//	LOG_RUNLOG_WARN("LP LPRestart success");
//
//	return 0;
//}




/**
 * @brief ͨԒ��DTMF�źŴ���
 *
 * @param[in] value     dtmf�ź�ֵ
 * @return
 */

void LPSendDtmf(char value)
{
	if(g_cConnType == CONN_TYPE_AUDIO)
	{
		LOG_RUNLOG_DEBUG("LP no need to exec LPSentDtmf (CONN_TYPE_AUDIO)");
		return;
	}

	if ( isdigit(value) || value == '#' || value == '*' )
	{
		linphonec_send_dtmf2(value);
	}
	else
	{
		LOG_RUNLOG_WARN("LP LPSentDtmf input param error");
	}

}





/**
 * @brief һ������message��Ӧ
 *
 * @param[in] pcTo     ����Ŀ�귽
 * @param[in] cResult  0���ųɹ� -1����δ�ɹ�
 * @return
 */

void LPSendOpenDoorResp(char *pcTo, char cResult)
{
//	if(pcTo == NULL)
//	{
//		return;
//	}
//	char acResp[LP_MESSAGE_BODY_MAX_LEN+1+32] = {0x00};
//	if(cResult == 0)
//	{
//		snprintf(acResp, sizeof(acResp)-1, "chat %s %s", pcTo, "MsgType=\"OPEN_DOOR\" Result=\"OK\"");
//	}
//	else
//	{
//		snprintf(acResp, sizeof(acResp)-1, "chat %s %s", pcTo, "MsgType=\"OPEN_DOOR\" Result=\"ERROR\"");
//	}
//	LOG_RUNLOG_DEBUG("LP LPSendOpenDoorResp will exec:%s", acResp);
//	LPProcessCommand(acResp);
	LPMsgSendDoorOpenResp(pcTo, 0, cResult);
}
