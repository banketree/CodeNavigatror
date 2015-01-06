/*
 * IDBTCfg.h
 *
 *  Created on: 2013-4-7
 *      Author: root
 */

#ifndef IDBTCFG_H_
#define IDBTCFG_H_



#ifdef __cplusplus
extern "C" {
#endif



typedef enum
{
	VOL_COMMAND_HELP,
	VOL_COMMAND_INPUT,
	VOL_COMMAND_OUTPUT,
	VOL_COMMAND_OUTPUT_LOCAL_WAV        //������ʾ���ļ�
}VOL_COMMAND_EN;



//�������Ͷ���.
#define NET_NULL    	0
#define NET_DHCP		'1'
#define NET_PPPOE		'2'
#define NET_STATIC		'3'
#define ETH_NAME 		"eth0"
#define SN_NULL "0000000000000000"
#define SN_LEN			32
#define IDBT_CONFIG_PATH		"/data/data/com.fsti.ladder/idbt.cfg"


//������Ϣ���ȶ���.
#define NM_USER_LEN		 		32
#define NM_PWD_LEN				32
#define NM_NET_FLAG_LEN			4
#define NM_ADD_IP_LEN			16
#define NM_DOMAIN_NAME_LEN		64
#define NM_PORT_LEN				5
#define MANAGEMENT_PASSWORD_LEN	6
#define OPENDOOR_PASSWORD_LEN	6


//����������Ϣ�ṹ��
typedef struct
{
	char acNetFlag[NM_NET_FLAG_LEN];			/**<! ��������. */
	char acStaticIP[NM_ADD_IP_LEN];				/**<! ��̬ IP. */
	char acStaticMask[NM_ADD_IP_LEN];			/**<! ��������. */
	char acStaticGateWay[NM_ADD_IP_LEN];		/**<! ���ص�ַ. */
	char acStaticDNS[NM_ADD_IP_LEN];			/**<! DNS��ַ. */
	char acMac[20];								/**<! MAC��ַ. */
	char acWanUserName[NM_USER_LEN];			/**<! ����˺�. */
	char acWanPassword[NM_PWD_LEN];				/**<! �������. */
	char acIMSProxyIP[NM_ADD_IP_LEN];			/**<! IMS IP. */
	char acIMSDomainName[NM_DOMAIN_NAME_LEN];	/**<! IMS����. */
	char acIMSProxyPort[NM_PORT_LEN];			/**<! IMS�˿�. */
}NM_NETWORK_INFO_ST;

//�������б�
typedef struct
{
	char acDestIP1[NM_ADD_IP_LEN];
	int port1;
	char acDestIP2[NM_ADD_IP_LEN];
	int port2;
}NM_DEST_IP_INFO_ST;


//IDBT�������
typedef struct
{
	char acSn[SN_LEN];
	int iInputVol;				//XT8126ͨ������������-50 ~ +30 dB��
	int iOutputCallVol;			//XT8126ͨ�����������-40 ~ +6 dB��
	int iOutputPlayWavVol;		//XT8126���ز���������-40 ~ +6 dB��
	int iVedioQuality;			//��׿�ݿڻ���Ƶ����  (0~50)
	int iVedioFps;				//֡��
	int iVedioBr;				//����
	int iVedioFmt;				//CIF��0/D1��1
	int iNegotiateMode;			//�ֶ�ģʽ��0/�Զ�ģʽ��1
	int iLogUploadMode;			//��־�ϴ�ģʽ��1-��ʱ�ϴ� 2-�����ϴ�  Ĭ��Ϊ2��
	int iLogUploadCycleTime;	//��־�ϴ�����
	int iRotation;
	int iNetMode;
	NM_NETWORK_INFO_ST stNetWorkInfo;	//�������������Ϣ
	NM_DEST_IP_INFO_ST stDestIpInfo;
	char acManagementPassword[MANAGEMENT_PASSWORD_LEN + 1];
	char acOpendoorPassword[OPENDOOR_PASSWORD_LEN + 1];
	char acPositionCode[16];
}IDBT_CONFIG_ST;

extern IDBT_CONFIG_ST g_stIdbtCfg;

#ifdef __cplusplus
}
#endif


#endif /* IDBTCFG_H_ */
