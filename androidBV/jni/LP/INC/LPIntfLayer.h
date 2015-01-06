
/**
 *@file    LPIntLayer.h
 *@brief   Linpone�ӿڷ�װ
 *
 *@author  chensq
 *@version 1.0
 *@date    2012-5-20
 */

#ifndef LPINTLAYER_H_
#define LPINTLAYER_H_

#include "YKMsgQue.h"
#include "IDBT.h"


#define PRACK_SEND								//prack send support
#define DTMF_RESP                               	//DTMF����֧��
#define LP_EXPIRES_TIME                     (600) 	//600
#define LP_MESSAGE_BODY_MAX_LEN             (256)


typedef enum
{
	REG_STATUS_OFFLINE  = 0,                //ע��״̬����
	REG_STATUS_ONLINE,                      //ע��״̬����
}REG_STATUS_EN;

typedef enum
{
	NETWORK_STATUS_OFFLINE = 0,
	NETWORK_STATUS_ONLINE,
}NETWORK_STATUS_EN;

typedef enum
{
	CONN_TYPE_AUDIO = 0,
	CONN_TYPE_VIDEO
}CONN_TYPE_EN;

typedef enum
{
	CALL_DIR_OUTGOING  = 0,                 //ȥ��
	CALL_DIR_INCOMMING,                     //����
}CALL_DIR_EN;

//����ͨ�����̵�״̬����
typedef enum
{
	CALL_STATUS_INCOMING_RECEIVED          = 1,
	CALL_STATUS_INIT                       = 2,
	CALL_STATUS_IN_PROGRESS                = 3,
	CALL_STATUS_RINIGING                   = 4,
	CALL_STATUS_OUTGOING_EARLYMEDIA        = 5,
	CALL_STATUS_CONNECTED                  = 7,
	CALL_STATUS_ERROR                      = 12,
	CALL_STATUS_END                        = 13,
}CALL_STATUS_EN;


typedef enum
{
	CALL_ERR_NORESPONSES,
	CALL_ERR_PROTOCOL,
	CALL_ERR_FAILURE,
	CALL_ERR_OTHER,
}CALL_ERR_EN;


typedef enum
{
	CALL_FAILURE_REASON_DECLINED,
	CALL_FAILURE_REASON_BUSY,
	CALL_FAILURE_REASON_REDIRECT,
	CALL_FAILURE_REASON_TEMPORARILY_UNAVAILABLE,
	CALL_FAILURE_REASON_NOTFOUND,
	CALL_FAILURE_REASON_DONOT_DISTURB,
	CALL_FAILURE_REASON_MEDIA,
	CALL_FAILURE_REASON_OTHER,
}CALL_FAILURE_REASON;



//LP��ʱ��
typedef struct
{
	YK_TIMER_ST     *pstTimer;
	unsigned long	 ulMagicNum;
}LP_TIMER_ST;

#define STOP_CALLING_TIMEOUT		   (5*1000)    //5s
#define DTMF_RECV_TIMROUT              (5*1000)    //5s
#define SIP_REG_TIMROUT                (5*1000*60) //5min
#define SIP_INCOMMING_TIMROUT                (5*1000) //5s

#define CALL_FAILURE_REASON_SIPUNREGISTED    0x1001
#define CALL_FAILURE_REASON_REREGISTER       0x1002
#define CALL_FAILURE_REASON_TOO_MANY_DTMF    0x1003


#define LP_HINT_MUSIC_NAME    "LP/LPConfig/dooropen.wav"
//#define LP_CONFIG_NAME      "LP/config/lpconfig"                 //linphone�����ļ�����·��
//#define LP_IMS_SERVER_REALM      "fj.ctcims.cn"
//#define LP_IMS_SERVER_REALM_GZ   "zte.com"
//#define LP_IMS_SERVER_REALM_GZ2   "gd.ctcims.cn"
#define LP_INVALID_CALL_ID     (0)        	//��Ч����ID
#define LP_QUE_LEN             (256)      	//linphone��Ϣ���������
#define LP_QUE_WRITE_TIMEOUT   (250)      	//дlinphone��Ϣ������ʱ��ȴ�ʱ��(����)
#define LP_QUE_READ_TIMEOUT    (250)      	//��linphone��Ϣ���п�ʱ��ȴ�ʱ��(����)




extern YK_MSG_QUE_ST*  g_pstLPMsgQ;


/**
 * @brief        ����ǰ����ͨ��״̬ʱ,ͨ���ýӿڵ�֪�Ǳ��л�������
 * @param[in]
 * @return       CALL_DIR_OUTGOING/CALL_DIR_INCOMMING
 */
CALL_DIR_EN LPGetCallDir(void);

void LPSetCallViewVersion(char cFlg);

/**
 * @brief        ͨ���ýӿڵ�֪SIPע��״̬
 * @param[in]
 * @return       REG_STATUS_OFFLINE/REG_STATUS_ONLINE
 */
REG_STATUS_EN LPGetSipRegStatus(void);

/**
 * @brief        ͨ���ýӿڱ��SIPע��״̬
 * @param[in]
 * @return       REG_STATUS_OFFLINE
 */
void LPSetSipRegStatus(REG_STATUS_EN enStatus);

void LPSetNetworkStatus(NETWORK_STATUS_EN enStatus);

/**
 * @brief        ͨ���ýӿڱ��SIPע��״̬
 * @param[in]
 * @return       REG_STATUS_OFFLINE
 */
void LPSetSipRegStatusForBV(REG_STATUS_EN enStatus);

/**
 * @brief        Linphoneģ����Դ��ʼ��
 * @param[in]    ��
 * @return       0:�ɹ�  <0:ʧ��
 */
int LPInit(void);

/**
 * @brief       Linphone��Դ����
 * @param[in]   ��
 * @return      ��
 */
void LPFini(void);


/**
 * @brief        Linphone����͵��ܵ�
 * @param[in]    pcCommand:����
 * @return       ��
 */
void LPProcessCommand(const char* pcCommand);


void LPSendDtmf(char value);


void LPSendOpenDoorResp(char *pcTo, char cResult);

#endif /* LPINTLAYER_H_ */
