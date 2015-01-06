/**
 *@file 	SMEventHndl.h
 *@brief 	����ӿ�ģ�������ؽӿ�
 *@author 	cx
 *@version 	1.0
 *@date 	2012-06-10
 */

#ifdef _YK_IMX27_AV_

#ifndef SM_EVENT_HNDL_IMX27_H_
#define SM_EVENT_HNDL_IMX27_H_

#include <YKSystem.h>
#include <YKTimer.h>
#include <YKMsgQue.h>

#define _SM_EVENT_DEBUG_

//SM��Ϣ���г���
#define SM_MSG_QUE_LEN          1024

//���ڽ��ջ�������С
#define COMM_DATA_RCV_SIZE      1024

//�������ݽ��ռ�������룩
#define RCV_COM_DATA_INTERVAL	 1000*10

//�豸���Ͷ���
#define DEV_TYPE_TTL       0x30
#define DEV_TYPE_R232      0x31
#define DEV_TYPE_SELFPROC  0x32
#define DEV_TYPE_N1        0x33
#define DEV_TYPE_N3        0x34
#define DEV_TYPE_N4        0x35
#define DEV_NO_TYPE        0x38

//�ǻ������ն������¼�����
#define SMIB_CALLEE_PICK_UP               0x30
#define SMIB_CALLEE_HANG_OFF              0x31
#define SMIB_CALL_AUDIO_CONNECT           0x32
#define SMIB_DTMF_SIGNAL                  0x33
#define SMIB_HEART_BEAT                   0x34
#define SMIB_INTERFACE_BOARD_REBOOT       0x35
#define SMIB_SET_REQUSET                  0x36
#define SMIB_MONITOR_REQUSET              0x37
#define SMIB_MONITOR_CANCEL               0x38

//�ն�����ӿ�ģ�������¼�����
#define IBSM_DOOR_MACHINE_CALL          0x30
#define IBSM_INDOOR_MACHINE_PICK_UP     0x31
#define IBSM_DOOR_MACHINE_HANG_OFF      0x32
#define IBSM_RESERVED                   0x33
#define IBSM_READ_CARD                  0x34
#define IBSM_OPEN_DOOR                  0x35
#define IBSM_MONITOR_CANCEL             0x36
#define IBSM_ALARM                      0x37

/*��Ϣ���Ͷ���*/
//���ڽ��յ�����
#define SM_SERIAL_DATA_RCV         0x6001
//��ʱ���¼�
#define SM_TIMER_EVENT             0x6002
//�˳���Ϣ�����߳�
#define SM_HNDL_MSG_CANCEL         0x6003
// SM����XT���������¼�
#define SM_HDNL_XT_EVENT           0x6004

//��ʱ����������������
#define TIMER_ID_HEARTBEAT              1
//Ӧ�����Ӧ��ʱ
#define TIMER_ID_PACKET_RSP_TIMEOUT     2

//ʱ�䶨��
#define HEARTBEAT_INTERVAL              20*1000		//�������
#define CLIENT_RSP_TIMROUT              80*1000	    //������ʱ

//���ڽ������ݽṹ��
typedef struct
{
	char acBuf[COMM_DATA_RCV_SIZE];
	char *pcHead;
	unsigned char ucCounter;
	unsigned char ucState;
	int iLen;
}__attribute__((packed)) COM_DATA_RCV_ST;

/* ����ͨѶ�� (�����豸)*/
//�ն������
typedef struct
{
	char acEventType[1];
	char acMachineType[1];
}__attribute__((packed)) HOST_REQ_PACKET_ST;

//�ն˷��ͱ�ע��Ϣ
typedef struct
{
	char acEventType[1];
	char acRemark[8];
}__attribute__((packed)) HOST_REMARK_PACKET_ST;

//�ǻ������ն�Ӧ���
typedef struct
{
	char cCmdContent;
}__attribute__((packed)) HOST_RSP_PACKET_ST;

//�ӿڰ������
typedef struct
{
	char acEventType[1];
	char acCallerId[8];
	char acCalleeId[8];
	char acRemark[16];
}__attribute__((packed)) CLIENT_REQ_PACKET_ST;

//�ӿڰ�Ӧ���
typedef struct
{
	char acCmdContent[1];
}__attribute__((packed)) CLIENT_RSP_PACKET_ST;

//���ڽ�������
typedef struct
{
	unsigned int 	uiPrmvType;				//ԭ������
	unsigned int uiLen; 					//���ݳ���
	char *pcData;	    					//���յĴ�������
}__attribute__((packed)) SM_RCV_DATA_ST;

//SM��ʱ��
typedef struct
{
	YK_TIMER_ST     *pstTimer;
	unsigned long	 ulMagicNum;
}SM_TIMER_ST;

//��ʱ���¼��ṹ��
typedef struct
{
	unsigned int uiPrmvType;
	unsigned int uiTimerId;
}SM_TIMER_EVENT_ST;

//�˳�SM�¼�����ṹ��
typedef struct
{
	unsigned int 	uiPrmvType;            //ԭ������
}SM_EXIT_MSG_ST;


extern YK_MSG_QUE_ST *g_pstSMMsgQ;

/**
 *@brief 		�����ǻ������ն�Ӧ��
 *@param[in] 	���������P-�ɹ� E-�Ƿ����� R-�ܾ����� C-CRCУ�����
 *@return 		TRUE-�ɹ�	FALSE-ʧ��
 */
BOOL SMSendHostRsq(char cContent);

/**
 *@brief 		�����ӿڰ�
 *@param[in] 	��
 *@return 		TRUE-�ɹ�	FALSE-ʧ��
 */
BOOL SMNMDevReboot(void);

/**
 *@brief 		����
 *@param[in] 	��
 *@return 		TRUE-�ɹ�	FALSE-ʧ��
 */
BOOL SMOpenDoor(void);

/**
 *@brief 		����ӿ�ģ���ʼ��
 *@param[in] 	��
 *@return 		RST_OK-�ɹ�	RST_ERR-ʧ��
 */
int SMInit(void);

/**
 *@brief 		�ͷ�����ӿ�ģ��ռ����Դ
 *@param[in] 	��
 *@return 		��
 */
void SMFini(void);

/**
 *@brief 		SM�����д���
 *@param[in] 	pcCmd ����������
 *@return 		��
 */
int SMProcessCommand(const char *pcCmd);


#endif /* SM_EVENT_HNDL_IMX27_H_ */

#endif /* _YK_IMX27_AV_ */
