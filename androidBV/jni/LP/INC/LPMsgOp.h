/**
 *@file        LPMsgOp.h
 *@brief       LP��Ϣ��װ�ʹ���
 *@author      chensq
 *@version     1.0
 *@date        2012-5-21
 */


#ifndef LPBUSLAYER_H_
#define LPBUSLAYER_H_

#include "YKMsgQue.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef enum
{
	SIP_MSG_TYPE_MESSAGE = 1,
	SIP_MSG_TYPE_NOTIFY,
	SIP_MSG_TYPE_PUSHINFO,
	SIP_MSG_TYPE_CALL_MSG
}SIP_MSG_TYPE_EN;


//������Ϣ
typedef struct
{
	char cCntType;            //MESSAGE 1 / notify 2 / pushInfo 3
	char acServiceType[32];   //"ACCESS_MSG"
	char acMsgType[32];       //"MSG_UNLOCK_REQ"
	int  iMsgSeq;             //"1"
}SIP_MESSAGE_OC_ST;

//�Ž���Ϣ֪ͨ
typedef struct
{
	//MESSAGE 1 / notify 2 / pushInfo 3
	char cCntType;
	// "notify":    ���ն�֪ͨ��Ϣ
	// "report":    �ն��ϱ���Ϣ
	// "pushinfo":  ҵ�����ն�������Ϣ
	char acServiceType[32];
	// accesssoftupgrade:    ֪ͨ���������Ϣ
	// accessconfigupdate��  ֪ͨ���ò�������
	// accesspwdupdate:      �Ž��������
	// accesscardupdate:     �Ž�������
	char acMsgType[32];
	//char acContent[512];
	//char acDescription[128];
	//int  iAutoLink;
	//char acLinkUrl[255];
}SIP_MESSAGE_DOORPLATFORM_ST;







/**
 * @brief        ����CC��SIP���͵ĺ�����Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgCntProcessCcsipCall(void *pvMsg, char *pcBuff, int iBuffSize);


/**
 * @brief        ����CC��SIP���͵Ľ�����Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgCntProcessCcsipAnswer(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        ����CC��SIP���͵Ĺһ���Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgCntProcessCcsipTerminate(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        ����CC��SIP���͵ĹҶ����е���Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgCntProcessCcsipTerminateAll(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        ����TM��SIP���͵�ȡ��ע�����Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgCntProcessTmsipUnRegister(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        ����TM��SIP���͵�ע�����Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgCntProcessTmsipRegister(void *pvMsg, char *pcBuff, int iBuffSize);

///**
// * @brief        ����TM��SIP���͵ĸ���ע�����Ϣ
// * @param[in]    ��
// * @return       0:�ɹ�  ����:ʧ��
// */
//int LPMsgCntProcessTmsipRegisterUpdate(void *pvMsg, char *pcBuff, int iBuffSize);

/**
 * @brief        SIP��CC��������������Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgSendSipccAudioCall(char *pcDisplayName,  long lId, char *pcSipInfo);

/**
 * @brief        SIP��CC������Ƶ������Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgSendSipccVideoCall(char *pcDisplayName,  long lId);

/**
 * @brief        SIP��CC���ͱ��з�������Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgSendSipccCalleePickup(long lId);

/**
 * @brief        SIP��CC���ͱ��з���ʾ�����Ž�����Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgSendSipccCalleeMusicEnd(void);

/**
 * @brief        SIP��CC����ֹͣ����ָ��
 * @param[in]
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgSendSipccStopcalling(void);


/**
 * @brief        SIP��CC���ͱ��κ��л�ͨ���ѹһ���Ϣ
 * @param[in]    ��
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgSendSipccCalleeHandOff(long lId);

/**
 * @brief        SIP��CC���ͱ��κ��л�ͨ���ѹһ���Ϣ
 * @param[in]    iErrorCode:�������
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgSendSipccCallFailure(int iErrorCode, long lId);

/**
 * @brief        SIP��CC�����յ�DTMF�ź���Ϣ
 * @param[in]    iDtmf:DTMF�ź�ֵ
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgSendSipccDtmf(int iDtmf);



/**
 * @brief        ��TM���ã�TM��SIP����ע����Ϣ
 * @param[in]
 * @return       0:�ɹ�  ����:ʧ��
 */
int LPMsgSendTmSipRegister(const char *pcUserName, const char *pcPassword,
       const char *pcProxy, const char *pcRoute);



int LPMsgSendSipABStopRing();






#ifdef _YK_GYY_
int LPMsgSendSipTmSoftUpgrade(void);
int LPMsgSendSipTmConfigUpdate(void);
int LPMsgSendSipTmPwdUpdate(void);
int LPMsgSendSipTmCardUpdate(void);
#endif


#ifdef __cplusplus
};
#endif


#endif /* LPBUSLAYER_H_ */
