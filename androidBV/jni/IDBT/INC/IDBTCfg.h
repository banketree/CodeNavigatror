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
	VOL_COMMAND_OUTPUT_LOCAL_WAV        //本地提示音文件
}VOL_COMMAND_EN;



//网络类型定义.
#define NET_NULL    	0
#define NET_DHCP		'1'
#define NET_PPPOE		'2'
#define NET_STATIC		'3'
#define ETH_NAME 		"eth0"
#define SN_NULL "0000000000000000"
#define SN_LEN			32
#define IDBT_CONFIG_PATH		"/data/data/com.fsti.ladder/idbt.cfg"


//网络信息长度定义.
#define NM_USER_LEN		 		32
#define NM_PWD_LEN				32
#define NM_NET_FLAG_LEN			4
#define NM_ADD_IP_LEN			16
#define NM_DOMAIN_NAME_LEN		64
#define NM_PORT_LEN				5
#define MANAGEMENT_PASSWORD_LEN	6
#define OPENDOOR_PASSWORD_LEN	6


//网络连接信息结构体
typedef struct
{
	char acNetFlag[NM_NET_FLAG_LEN];			/**<! 连接类型. */
	char acStaticIP[NM_ADD_IP_LEN];				/**<! 静态 IP. */
	char acStaticMask[NM_ADD_IP_LEN];			/**<! 子网掩码. */
	char acStaticGateWay[NM_ADD_IP_LEN];		/**<! 网关地址. */
	char acStaticDNS[NM_ADD_IP_LEN];			/**<! DNS地址. */
	char acMac[20];								/**<! MAC地址. */
	char acWanUserName[NM_USER_LEN];			/**<! 宽带账号. */
	char acWanPassword[NM_PWD_LEN];				/**<! 宽带密码. */
	char acIMSProxyIP[NM_ADD_IP_LEN];			/**<! IMS IP. */
	char acIMSDomainName[NM_DOMAIN_NAME_LEN];	/**<! IMS域名. */
	char acIMSProxyPort[NM_PORT_LEN];			/**<! IMS端口. */
}NM_NETWORK_INFO_ST;

//网络检测列表
typedef struct
{
	char acDestIP1[NM_ADD_IP_LEN];
	int port1;
	char acDestIP2[NM_ADD_IP_LEN];
	int port2;
}NM_DEST_IP_INFO_ST;


//IDBT相关配置
typedef struct
{
	char acSn[SN_LEN];
	int iInputVol;				//XT8126通话输入声音（-50 ~ +30 dB）
	int iOutputCallVol;			//XT8126通话输出声音（-40 ~ +6 dB）
	int iOutputPlayWavVol;		//XT8126本地播放声音（-40 ~ +6 dB）
	int iVedioQuality;			//安卓梯口机视频质量  (0~50)
	int iVedioFps;				//帧率
	int iVedioBr;				//码率
	int iVedioFmt;				//CIF：0/D1：1
	int iNegotiateMode;			//手动模式：0/自动模式：1
	int iLogUploadMode;			//日志上传模式（1-定时上传 2-周期上传  默认为2）
	int iLogUploadCycleTime;	//日志上传周期
	int iRotation;
	int iNetMode;
	NM_NETWORK_INFO_ST stNetWorkInfo;	//网络配置相关信息
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
