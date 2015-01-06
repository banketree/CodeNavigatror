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
#ifndef IDBT_H_
#define IDBT_H_

//#include "log4c.h"
#include "YKTimer.h"
#include "../../AT/INC/AutoTest.h"

#ifndef RST_OK
#define RST_OK  0
#endif

#ifndef RST_ERR
#define RST_ERR  -1
#endif

#ifndef RST_FREE
#define RST_FREE 1
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef MAX_INPUT
#define MAX_INPUT 256
#endif


#define VERSION_NAME_LEN 11
#define HARDWARE_VERSION_LEN 1
#define LINE_LEFT_HEAD         "<"
#define LINE_LEFT_LINE_HEAD    "</"
#define LINE_TAIL              ">"


//���¶������ն���ʹ�õ�ͨѶЭ�����ͣ����������ͣ�I6,TR069��YK069
//ͨ�� _TM_TYPE_ �����ͽ�������
#define 	_TR069_ 			0
#define 	_YK069_ 			1
#define		_GYY_I6_			2

#ifndef 	_TM_TYPE_
#define 	_TM_TYPE_ 			_GYY_I6_
#endif

//#define IDBT_AUTO_TEST     	//�Զ�����
//#define BASE64_3DES			//����
//#define TM_TAKE_PIC  			//���ƹ���
//#define _CAPTURE_PIC_			//���չ���
#define CALL_PREVIEW          //����Ԥ��(ֻ֧��AV2.0/AVDoor/BV/BVFKJ)
#define NEW_CARD_READER			//�¶���ģ��

#if _TM_TYPE_ == _GYY_I6_
#define CARD_INCREMENT
#define _GYY_I5_
#define _I6_CC_
//#define CONFIG_SORT
//#define BASE64_3DES			//����
#endif

//���¶�������Բ�ͬӲ��ƽ̨��ʹ�õ����ģ�����ͣ��ܹ�����������
//ͨ������Ӧ�ĺ�����Ϊ1���������ģ�����ͣ�ע��ֻ����һ��ģ�屻
//����Ϊ1��ģ��������ԭ����"_��˾����_ƽ̨����_��������_"
//�������͵�����ԭ��
//YK		YouKe				�ʿƹ�˾�汾
//MN2440	Mini2440			Mini2440ƽ̨
//XT8126	XinTel8126			�����Ƽ���8126ƽ̨
//IMX27		IMX27				�ݲ���IMX27ƽ̨
//AA		Adapt Audio  		�����������Ƶ�汾
//AV		Adapt Vedio			�����������Ƶ�汾
//BV		New-Build Vedio		�½��ݿڻ���Ƶ�汾
//#define 	_YK_MN2440_AA_
//#define 	_YK_XT8126_AA_
//#define 	_YK_XT8126_AV_H264_
//#define 	_YK_XT8126_AV_MPEG4_
//#define 	_YK_XT8126_BV_H264_
//#define 	_YK_IMX27_AV_MPEG4_
//#define 	_YK_IMX27_BV_H264_

//�����������ͣ���AV��BV���ֹ������͵Ĺ����У����ܴ��������ֱ���뷽ʽ
//#define 	_CODEC_H264_		0
//#define	_CODEC_MPEG4_		0

#ifndef SAFE_FREE
#define SAFE_FREE(x) \
do { \
	if (!x) assert(0); \
	free(x); \
	x = NULL; \
} while(0)
#endif

//���¶��������ļ���·�������ڲ�ͬ��ƽ̨����������ļ���·�����ܲ�ͬ��
//ͨ����ͬ��ƽ̨���Ͷ��岻ͬ�������ļ�·����·����ʵ�������ļ�Ϊ����
//�豸�еľ���·��������PC��Debugģʽ�£����������ļ���·������һ����
#ifdef TARGET_BOARD

	#ifdef _YK_MN2440_AA_

	#endif

	#ifdef _YK_XT8126_AA_

	#endif
	
	#ifdef _YK_S5PV210_
		#define LOG_FLASH_PATH          "/data/data/com.fsti.ladder/"
		#define VOLUME_CONFIG_PATH      "/data/data/com.fsti.ladder/VLMConfig.ini"
		#define IDBT_CONFIG_PATH		"/data/data/com.fsti.ladder/idbt.cfg"
		#define TM_CONFIG_PATH			"/data/data/com.fsti.ladder/TMConfig.ini"
		#define TM_DATA_CONFIG			"/data/data/com.fsti.ladder/TMConfig.xml"
		#define TM_DATA_PATH			"/data/data/com.fsti.ladder/TMData.xml"
		#define BA_DATA_PATH			"/data/data/com.fsti.ladder/BAData.xml"
		#define BA_MODEL_PATH			"/data/data/com.fsti.ladder/BAModel.xml"
		#define TM_DATA_BACKUP_PATH		"/data/data/com.fsti.ladder/TMData_bak.xml"
		#define TM_MODEL_PATH			"/data/data/com.fsti.ladder/TMModel.xml"

		#define LP_CONFIG_NAME			"/data/data/com.fsti.ladder/LPConfig"
		#define LOG4C_RCPATH            "/data/data/com.fsti.ladder"

		#define NM_CONFIG				"/data/data/com.fsti.ladder/NMConfig.ini"
		#define LG_LOCAL_PATH			"/data/data/com.fsti.ladder/LogDir/Log/"
		#define LG_LOCAL_PATH_1			"/data/data/com.fsti.ladder/LogDir/Log"
		#define LG_REMOTE_PATH			"./Log/"
		#define AT_CONFIG_PATH			"/data/data/com.fsti.ladder/ATConfig.ini"

		#define TR_INDEX_TMP			"/data/data/com.fsti.ladder/Tr069Temp"
		#define IMS_SIP_PATH			"/data/data/com.fsti.ladder/ImsSip.xml"

		#define I6_CONFIG_PATH			"/data/data/com.fsti.ladder/I6Config.xml"
		#define I6_PASSWORD_PATH		"/data/data/com.fsti.ladder/I6Password.xml"
		#define I6_CARD_PATH			"/data/data/com.fsti.ladder/I6Card.xml"

		#define I6_CONFIG_PATH_TEMP		"/data/data/com.fsti.ladder/I6ConfigTemp.xml"
		#define I6_PASSWORD_PATH_TEMP	"/data/data/com.fsti.ladder/I6PasswordTemp.xml"
		#define I6_CARD_PATH_TEMP		"/data/data/com.fsti.ladder/I6CardTemp.xml"
		#define I6_DATA_PATH			"/data/data/com.fsti.ladder/I6Data.xml"
		#define I6_RECV_TEMP			"/data/data/com.fsti.ladder/I6Temp.xml"
	#endif

	#ifdef _YK_XT8126_AV_

		#define LOG_FLASH_PATH          "/mnt/mtd"
		#define IDBT_CONFIG_PATH		"/mnt/mtd/config-idbt/idbt.cfg"
		#define TM_CONFIG_PATH			"/mnt/mtd/config-idbt/TMConfig.ini"
		#define TM_DATA_PATH			"/mnt/mtd/config-idbt/TMData.xml"
		#define BA_DATA_PATH			"/mnt/mtd/config-idbt/BAData.xml"
		#define BA_MODEL_PATH			"/mnt/mtd/config-idbt/BAModel.xml"
		#define TM_DATA_BACKUP_PATH		"/mnt/mtd/config-idbt/TMData_bak.xml"
		#define TM_MODEL_PATH			"/mnt/mtd/config-idbt/TMModel.xml"
		#define TM_DATA_CONFIG			"/tmp/TMConfig.xml"
		#define LP_CONFIG_NAME			"/mnt/mtd/config-idbt/LPConfig"
		#define LOG4C_RCPATH            "/mnt/mtd/config-idbt"

		#define LG_LOCAL_PATH_1			"/mnt/mtd/LogDir/Log"
		#define LG_LOCAL_PATH			"/mnt/mtd/LogDir/Log/"
		#define LG_REMOTE_PATH			"./Log/"
		#define AT_CONFIG_PATH			"/mnt/mtd/config-idbt/ATConfig.ini"

		#define TR_INDEX_TMP			"/mnt/mtd/config-idbt/Tr069Temp"
		#define IMS_SIP_PATH			"/mnt/mtd/config-idbt/ImsSip.xml"

		#define I6_CONFIG_PATH			"/mnt/mtd/config-idbt/I6Config.xml"
		#define I6_PASSWORD_PATH		"/mnt/mtd/config-idbt/I6Password.xml"
		#define I6_CARD_PATH			"/mnt/mtd/config-idbt/I6Card.xml"
		#define I6_CONFIG_PATH_TEMP		"/tmp/I6ConfigTemp.xml"
		#define I6_PASSWORD_PATH_TEMP	"/tmp/I6PasswordTemp.xml"
		#define I6_CARD_PATH_TEMP		"/tmp/I6CardTemp.xml"
		#define I6_DATA_PATH			"/mnt/mtd/config-idbt/I6Data.xml"
		#define I6_RECV_TEMP			"/tmp/I6Temp.xml"
	#endif

	#ifdef _YK_XT8126_BV_

		#define LOG_FLASH_PATH          "/mnt/mtd"
		#define IDBT_CONFIG_PATH		"/mnt/mtd/config-idbt/idbt.cfg"
		#define TM_CONFIG_PATH			"/mnt/mtd/config-idbt/TMConfig.ini"
		#define TM_DATA_CONFIG			"/tmp/TMConfig.xml"
	#ifndef DATA_BZIP
		#define TM_DATA_PATH			"/mnt/mtd/config-idbt/TMData.xml"
		#define TM_DATA_BACKUP_PATH		"/mnt/mtd/config-idbt/TMData_bak.xml"
	#else
		#define TM_DATA_PATH			"/mnt/mtd/config-idbt/TMData.xml.bz2"
		#define TM_DATA_BACKUP_PATH		"/mnt/mtd/config-idbt/TMData_bak.xml.bz2"
		#define TM_DATA_UPLOAD			"/tmp/TMData.xml"
	#endif
		#define BA_DATA_PATH			"/mnt/mtd/config-idbt/BAData.xml"
		#define BA_MODEL_PATH			"/mnt/mtd/config-idbt/BAModel.xml"
		#define TM_MODEL_PATH			"/mnt/mtd/config-idbt/TMModel.xml"

		#define LP_CONFIG_NAME			"/mnt/mtd/config-idbt/LPConfig"
		#define LOG4C_RCPATH            "/mnt/mtd/config-idbt"

		#define LG_LOCAL_PATH_1			"/mnt/mtd/LogDir/Log"
		#define LG_LOCAL_PATH			"/mnt/mtd/LogDir/Log/"
		#define LG_REMOTE_PATH			"./Log/"
		#define AT_CONFIG_PATH			"/mnt/mtd/config-idbt/ATConfig.ini"

		#define TR_INDEX_TMP			"/mnt/mtd/config-idbt/Tr069Temp"
		#define IMS_SIP_PATH			"/mnt/mtd/config-idbt/ImsSip.xml"

		#define TM_DATA_RUNNING			"/TMData.xml"
		#define TM_DATA_BAK_RUNNING		"/TMData_bak.xml"

	#ifndef DATA_BZIP
		#define I6_CONFIG_PATH			"/mnt/mtd/config-idbt/I6Config.xml"
		#define I6_PASSWORD_PATH		"/mnt/mtd/config-idbt/I6Password.xml"
		#define I6_CARD_PATH			"/mnt/mtd/config-idbt/I6Card.xml"
	#else
		#define I6_CONFIG_BZIP			"/mnt/mtd/config-idbt/I6Config.xml.bz2"
		#define I6_PASSWORD_BZIP		"/mnt/mtd/config-idbt/I6Password.xml.bz2"
		#define I6_CARD_BZIP			"/mnt/mtd/config-idbt/I6Card.xml.bz2"
		#define I6_CONFIG_PATH			"/tmp/I6Config.xml"
		#define I6_PASSWORD_PATH		"/tmp/I6Password.xml"
		#define I6_CARD_PATH			"/tmp/I6Card.xml"
	#endif
		#define I6_CONFIG_PATH_TEMP		"/tmp/I6ConfigTemp.xml"
		#define I6_PASSWORD_PATH_TEMP	"/tmp/I6PasswordTemp.xml"
		#define I6_CARD_PATH_TEMP		"/tmp/I6CardTemp.xml"
		#define I6_DATA_PATH			"/mnt/mtd/config-idbt/I6Data.xml"
		#define I6_RECV_TEMP			"/tmp/I6Temp.xml"
	#endif

	#ifdef _YK_IMX27_AV_
		#define LOG_FLASH_PATH          "/home/YK-IDBT"
		#define IDBT_CONFIG_PATH		"/home/YK-IDBT/config-idbt/idbt.cfg"
		#define TM_CONFIG_PATH			"/home/YK-IDBT/config-idbt/TMConfig.ini"
		#define TM_DATA_PATH			"/home/YK-IDBT/config-idbt/TMData.xml"
		#define BA_DATA_PATH			"/home/YK-IDBT/config-idbt/BAData.xml"
		#define BA_MODEL_PATH			"/home/YK-IDBT/config-idbt/BAModel.xml"
		#define TM_DATA_BACKUP_PATH		"/home/YK-IDBT/config-idbt/TMData_bak.xml"
		#define TM_MODEL_PATH			"/home/YK-IDBT/config-idbt/TMModel.xml"
		#define TM_DATA_CONFIG			"/tmp/TMConfig.xml"
		#define LP_CONFIG_NAME			"/home/YK-IDBT/config-idbt/LPConfig"
		#define LOG4C_RCPATH            "/home/YK-IDBT/config-idbt"

		#define LG_LOCAL_PATH			"./LogDir/Log/"
		#define LG_LOCAL_PATH_1			"./LogDir/Log"
		#define LG_REMOTE_PATH			"./Log/"
		#define AT_CONFIG_PATH			"/home/YK-IDBT/config-idbt/ATConfig.ini"

		#define TR_INDEX_TMP			"/home/YK-IDBT/config-idbt/Tr069Temp"
		#define IMS_SIP_PATH			"/home/YK-IDBT/config-idbt/ImsSip.xml"

		#define I6_CONFIG_PATH			"/home/YK-IDBT/config-idbt/I6Config.xml"
		#define I6_PASSWORD_PATH		"/home/YK-IDBT/config-idbt/I6Password.xml"
		#define I6_CARD_PATH			"/home/YK-IDBT/config-idbt/I6Card.xml"
		#define I6_CONFIG_PATH_TEMP		"/tmp/I6ConfigTemp.xml"
		#define I6_PASSWORD_PATH_TEMP	"/tmp/I6PasswordTemp.xml"
		#define I6_CARD_PATH_TEMP		"/tmp/I6CardTemp.xml"
		#define I6_DATA_PATH			"/home/YK-IDBT/config-idbt/I6Data.xml"
		#define I6_RECV_TEMP			"/home/YK-IDBT/config-idbt/I6Temp.xml"
	#endif

	#ifdef _YK_IMX27_BV_

	#endif

#else
#define LOG_FLASH_PATH          "."
#define IDBT_CONFIG_PATH		"IDBT/idbt.cfg"
#define LP_CONFIG_NAME			"LP/LPConfig/LPConfig"
#define LOG4C_RCPATH            "LOG/LOGConfig"
#define TM_CONFIG_PATH			"TM/TMConfig.ini"
#define TM_DATA_PATH			"TM/TMData.xml"
#define TM_DATA_BACKUP_PATH		"TM/TMData_bak.xml"
#define TM_MODEL_PATH			"TM/TMModel.xml"
#define LG_LOCAL_PATH			"./LogDir/Log/"
#define LG_LOCAL_PATH_1			"./LogDir/Log"
#define LG_REMOTE_PATH			"./cLog/"
#define AT_CONFIG_PATH			"AT/ATConfig.ini"
#define TR_INDEX_TMP			"TM/Tr069Temp"
#define BA_DATA_PATH			"TM/BAData.xml"
#define BA_MODEL_PATH			"TM/BAModel.xml"
#define IMS_SIP_PATH			"TM/ImsSip.xml"
#define TM_DATA_CONFIG			"TM/TMConfig.xml"
#define I6_CONFIG_PATH			"TM/I6Config.xml"
#define I6_PASSWORD_PATH		"TM/I6Password.xml"
#define I6_CARD_PATH			"TM/I6Card.xml"
#define I6_CONFIG_PATH_TEMP		"TM/I6ConfigTemp.xml"
#define I6_PASSWORD_PATH_TEMP	"TM/I6PasswordTemp.xml"
#define I6_CARD_PATH_TEMP		"TM/I6CardTemp.xml"
#define I6_DATA_PATH			"TM/I6Data.xml"
#define I6_RECV_TEMP			"TM/I6Temp.xml"
#endif



/**@brief Application version. HV.BS.MS.SS*/
#ifdef _YK_XT8126_BV_

#ifdef YK_XT8126_BV_FKT
	#define YK_APP_VERSION   "2.1.0.6ghf" /**XX.XX.XX.XX*/
#else
	#ifdef _CODEC_H264_
		#define YK_APP_VERSION  "2.0.0.18ghb" /**XX.XX.XX.XX*/
	#endif
	#ifdef _CODEC_MPEG4_
		#define YK_APP_VERSION "2.0.0.3gmb"
	#endif
#endif

#endif

#ifdef _YK_XT8126_AV_

#ifdef _CODEC_H264_

#ifdef _YK_XT8126_DOOR_

#define YK_APP_VERSION "2.1.0.2gho" /**XX.XX.XX.XX*/

#else

#define YK_APP_VERSION  "2.1.0.3gha" /**XX.XX.XX.XX*/

#endif
#endif

#ifdef _CODEC_MPEG4_
#define YK_APP_VERSION "2.2.1gma" /**XX.XX.XX.XX*/
#endif

#endif

#ifdef _YK_IMX27_AV_

	#ifdef _YK_IMX27_SINGLE_
		#define YK_APP_VERSION   "2.1.0.1fhs" /**XX.XX.XX.XX*/
	#else
		#ifdef _YK_IMX27_GYY_
			#define YK_APP_VERSION   "2.1.0.0fht" /**XX.XX.XX.XX*/
		#else
			#define YK_APP_VERSION   "2.3.2.1fh" /**XX.XX.XX.XX*/
		#endif
	#endif
#endif

#ifdef _YK_S5PV210_

	 #define YK_APP_VERSION "2.1.3.1ab" /**XX.XX.XX.XX*/
	 #define YK_KERNEL_VERSION "1.0.0"
	  #define YK_HARDWARE_VERSION "1.0.0"
#endif

#ifdef _YK_PC_

#define YK_APP_VERSION "2.2.1" /**XX.XX.XX.XX*/

#endif


extern int g_iUpdateConfigFlag;
extern int g_iRunSystemFlag;
extern int g_iRunMCTaskFlag;
extern int g_iRunSMTaskFlag;
extern int g_iRunNMTaskFlag;
extern int g_iRunLPTaskFlag;
extern int g_iRunCCTaskFlag;
extern int g_iRunLGTaskFlag;
extern int g_iRunTMTaskFlag;
extern int g_iRunXTTaskFlag;
extern int g_iRunLANTaskFlag;
extern int g_iRunWDTaskFlag;
extern int g_iRunABTaskFlag;

//����ʱ���¼
struct timeval g_PicBeginTime;
struct timeval g_PicEndTime;
struct timeval g_ClinetGetPicTime;
struct timeval g_CapturePictureStart;
struct timeval g_CapturePictureEnd;
//---------Interface  Message ID--------------
//Message ID  SM-CC format 0x01xx
#define	   SMCC_DOOR_CALL				0x0101
#define	   SMCC_INDOOR_PICK_UP			0x0102
#define	   SMCC_DOOR_HANG_OFF			0x0103
#define	   SMCC_DOOR_DET_CANCEL		    0x0104
#define    SMCC_VEDIO_MONITOR_START     0x0105

#define	   ABCC_DOOR_CALL				0x0121
#define	   ABCC_DOOR_HANG_OFF			0x0122
//#define	   ABCC_DOOR_MONITOR_CANCEL	    0x0123
//#define    ABCC_VEDIO_MONITOR_START     0x0124

/*#define	   CCLAN_CALL     					0x0131
#define	   CCLAN_TERMINATE     				0x0132
#define	   CCLAN_WATCH_LINEUSE    			0x0133
#define	   CCLAN_WATCH_RESPON     			0x0134
#define	   LANCC_CALLEE_FAIL				0x0135
#define	   LANCC_CALLEE_RESPOND     		0x0136
#define	   LANCC_CALLEE_PICK_UP     		0x0137
#define	   LANCC_CALLEE_HANG_OFF     		0x0138
#define	   LANCC_CALLEE_LINEUSE    			0x0139
#define	   LANCC_CALLEE_OPENDOOR     		0x013a
#define	   LANCC_WATCH_INCOMING     		0x013b
#define	   LANCC_CALLEE_RING_TIMEOUT     	0x013c
#define	   LANCC_CALLEE_TALK_TIMEOUT		0x013d*/


//Message ID  CC-SM format 0x02xx
#define	   CCSM_CALLEE_PICK_UP			0x0201
#define	   CCSM_CALLEE_HANG_OFF		0x0202
#define	   CCSM_CALLEE_ERR				0x0203
#define	   CCSM_CALLEE_MUSIC_END		0x0204
#define	   CCSM_CALLEE_SEND_DTMF		0x0205
#define	   CCSM_OPENDOOR				0x0206
#define    CCSM_VEDIO_MONITOR			0x0207
#define    CCSM_VEDIO_MONITOR_CANCEL    0x0208

#define	   CCAB_CALLEE_PICK_UP			0x0221
#define	   CCAB_CALLEE_HANG_OFF		    0x0222
#define	   CCAB_CALLEE_ERR				0x0223
#define	   CCAB_OPENDOOR				0x0224
#define    CCAB_VEDIO_MONITOR			0x0225
#define	   CCAB_CALLEE_SEND_DTMF		0x0226
#define	   CCAB_CALLEE_MUSIC_END		0x0227

#define	   CCAB_CALLEE_ERR_ROOM_INVALID				0x0228
#define	   CCAB_CALLEE_ERR_ROOM_VALID				0x0229



//Message ID  CC-SIP format 0x03xx
#define	   CCSIP_CALL					0x0301
#define    CCSIP_ANSWER				    0x0302
#define	   CCSIP_TERMINATE				0x0303
#define	   CCSIP_TERMINATE_ALL			0x0304
#define	   CCSIP_CALL_TIMEOUT			0x0305
#define    AB_EXIT						0x0361

//Message ID  CC-SIP format 0x04xx
#define		SIPCC_CALLEE_PICK_UP		0x0401
#define		SIPCC_CALLEE_MUSIC_END		0x0402
#define		SIPCC_CALLEE_HANG_OFF		0x0403
#define		SIPCC_CALLEE_ERR			0x0404
#define		SIPCC_CALLEE_SEND_DTMF		0x0405
#define		SIPCC_VIDEO_INCOMING		0x0406
#define		SIPCC_AUDIO_INCOMING		0x0407
#define		SIPCC_RECV_EARLY_MEDIA		0x0408
#define	    SIPCC_MESSAGE_INCOMING      0x0409
#define     SIPCC_STOP_CALLING			0x0410
#define		SIPCC_PROCESS				0x0411
#define		SIPCC_RING					0x0412


#define 	SIPAB_REG_STATUS					0x0440
#define 	SIPAB_NETWORK_STATUS				0x0441
#define 	SMAB_RFIDCARD_VALIDATE_STATUS		0x0442
#define		TMAB_TM_LINK_STATUS					0x0443
#define		OTAB_REBOOT							0x0444
#define		OTAB_CONFIG_REBOOT					0x0445
#define 	SIPAB_STOP_RING						0x0446


//Message ID  CC-CC  format 0x05xx
#define		CCCC_TIMEROUT_EVENT			0x0501
#define     CCCC_CANCAL_EVENT           0X0502
#define     CCCC_PIC_RESULT_TIMEOUT_EVENT       0X0503

//Message ID  TM-SIP format 0x06xx
#define     TMSIP_REGISTER        		0x0601
#define     TMSIP_REGISTER_UPDATE       0x0602
#define     TMSIP_UNREGISTER            0x0603

//Message ID XT-SM format 0x07xx
#define 	XTSM_INVOKE_LP_CALL			0x0701
#define 	XTSM_TERMINATE_LP_CALL		0x0702
#define	XTSM_PICKUP_IDU				0x0703
#define    XTSM_MONITOR_CANCEL         0x0704

//Message ID  SM-XT format 0x09xx
#define 	SMXT_CALLEE_PICK_UP        	0x0901
#define 	SMXT_CALLEE_HANG_OFF       	0x0902
#define 	SMXT_CALLEE_ERR            	0x0903
#define 	SMXT_CALLEE_MUSIC_END      	0x0904
#define 	SMXT_OPEN_LED               0x0905
#define     SMXT_CLOSE_LED              0x0906
#define 	SMXT_OPENDOOR              	0x0907
#define 	SMXT_VIDEO_MONITOR			0x0908
#define    SMXT_VIDEO_MONITOR_CANCEL   0x0909
#define    SMXT_CALLEE_REJECT          0x0910
#define 	XT_EXIT						0x0920

//Message ID WD format 0x08xx
#define     SIPWD_WATCHDOG_FEED     	0x0801
#define     WD_TIMEOUT_EVENT        	0x0802
#define 	WD_CLOSE_TASK				0x0803
#define     XTWD_WATCHDOG_FEED     		0x0804
#define     TMWD_WATCHDOG_FEED     		0x0805

//Message ID TM-CC format 0x0axx
#define     TMCC_DEMO_CALL              	0x0a01
#define     TMCC_PIC_RESULT_PICKUP      	0x0a02	//����
#define     TMCC_PIC_RESULT_OPEN_DOOR  	    0x0a03	//����
#define     TMCC_PIC_RESULT_HANG_OFF     	0x0a04   	//�Ҷ�
#define     TMCC_PIC_RESULT_ERROR       	0x0a05 	//ƽ̨�ظ�����
#define     TMCC_PIC_SOCKET_ERROR       	0x0a06	//socket����
#define 	TMCC_PIC_CLINET_RECV_PIC_OK		0x0a07	//���ƿͻ����յ�ͼƬ�ɹ�
#define    TMAB_ADVERT_UPDATE	0x0a08	//

#define	    CCLAN_CALL						0x0b01
#define	    CCLAN_TERMINATE					0x0b03
#define	    CCLAN_WATCH_LINEUSE				0x0b04
#define	    CCLAN_WATCH_RESPOND				0x0b05

#define	    LANCC_CALLEE_FAIL				0x0c01
#define	    LANCC_CALLEE_RESPOND			0x0c02
#define	    LANCC_CALLEE_PICK_UP			0x0c03
#define	    LANCC_CALLEE_HANG_OFF			0x0c04
#define	    LANCC_CALLEE_LINEUSE			0x0c05
#define	    LANCC_CALLEE_OPENDOOR			0x0c06
#define	    LANCC_WATCH_INCOMING			0x0c07
#define	    LANCC_CALLEE_RING_TIMEOUT		0x0c08
#define	    LANCC_CALLEE_TALK_TIMEOUT		0x0c09
#define	    LANCC_CALLEE_TALK_END			0x0c0a

#define     DEMO_SM_CALL                    0x1108  //�����������

//Message ID SS format 0x0bxx
#define 	SS_CANCEL_EVENT					0X0B01	//�˳��߳�
#define 	SS_SNAPSHOT						0X0B02	//ץ��

#ifdef _YK_GYY_
//Message ID SIP-TM format 0x0b01
#define     SIPTM_SOFT_UPDATE     	0x0b01
#define     SIPTM_CONFIG_UPDATE     0x0b02
#define     SIPTM_PWD_UPDATE        0x0b03
#define     SIPTM_CARD_UPDATE       0x0b04
#endif

#ifdef _GYY_I6_
#define		TM_I6_BOOT				0x0b05
#define		TM_I6_CARD_SWIP			0x0b06
#define		TM_I6_HEART				0x0b07
#define		TM_I6_CONFIG			0x0b08
#define		TM_I6_PASSWORD			0x0b09
#define		TM_I6_CARD				0x0b10
#define		TM_I6_UPDATE			0x0b11
#define		TM_I6_GET_SIP			0x0b12
#define		TM_I6_ALARM_UPDATE		0X0b13
#define		TM_I6_ADV_DOWNLOAD		0X0b14
#define		TM_I6_ANN_DOWNLOAD		0X0b15
#endif

#define     ROOM_NUM_LEN                20
#define     PHONE_NUM_LEN               64
#define     USER_NAME_LEN               64
#define     PASSWORD_LEN                64
#define     PROXY_LEN                   64
#define     ROUTE_LEN                   32
#define     ROOM_NUM_BUFF_LEN           (ROOM_NUM_LEN  +1)
#define     PHONE_NUM_BUFF_LEN          (PHONE_NUM_LEN +1)
#define     USER_NAME_BUFF_LEN          (USER_NAME_LEN +1)
#define     PASSWORD_BUFF_LEN           (PASSWORD_LEN  +1)
#define     PROXY_BUFF_LEN              (PROXY_LEN     +1)
#define     ROUTE_BUFF_LEN              (ROUTE_LEN     +1)

#define 	CALLEE_ERROR_PHONENUM_NOT_EXIST	1
#define 	CALLEE_ERROR_CALL_DISABLE	2

//----------------Interface Struct--------------------------------

typedef enum
{
	REMOTE_REBOOT = 0,
	CONFIG_REBOOT
}JNI_REBOOT_EN;

//���ݽṹ����
typedef struct										//¥��Խ�ϵͳ����
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned char 	aucPhoneNum[PHONE_NUM_BUFF_LEN];//�绰����
	unsigned char 	aucMediaType;					//��ý������
}TMCC_DEMO_CALL_ST;

typedef struct										//¥��Խ�ϵͳ����
{
	unsigned int uiPrmvType;						//ԭ������
	unsigned char ucDevType;						//�豸����
	unsigned char	aucRoomNum[ROOM_NUM_BUFF_LEN];	//�����
}SMCC_DOOR_CALL_ST, ABCC_DOOR_CALL_ST, CCLAN_DOOR_CALL_ST;

typedef struct										//¥��Խ�ϵͳ����
{
	unsigned int 	uiPrmvType;						//ԭ������
}SMCC_INDOOR_PICK_UP_ST;

typedef struct										//¥��Խ�ϵͳ�һ�
{
	unsigned int 	uiPrmvType;						//ԭ������
}SMCC_DOOR_HANG_OFF_ST, ABCC_DOOR_HANG_OFF_ST;

typedef struct										//���ȡ��ָ��
{
	unsigned int 	uiPrmvType;						//ԭ������
}SMCC_DOOR_DET_CANCEL_ST, ABCC_DOOR_MONITOR_CANCEL_ST;

typedef struct                                    //��ʼ���ָ��
{
	unsigned int 	uiPrmvType;						//ԭ������
}SMCC_VEDIO_MONITOR_START_ST;

typedef struct										//���з�����
{
	unsigned int 	uiPrmvType;						//ԭ������
}CCSM_CALLEE_PICK_UP_ST, CCAB_CALLEE_PICK_UP_ST;

typedef struct										//���з������һ�
{
	unsigned int 	uiPrmvType;						//ԭ������
}CCSM_CALLEE_HANG_OFF_ST, CCAB_CALLEE_HANG_OFF_ST;

typedef struct										//���з��쳣
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned int	uiErrorCause;					//�쳣ԭ��
}CCSM_CALLEE_ERR_ST,CCAB_CALLEE_ERR_ST;

typedef struct										//���з��쳣
{
	unsigned int 	uiPrmvType;						//ԭ������
}CCAB_CALLEE_ERR_ROOM_INVALID_ST, CCAB_CALLEE_ERR_ROOM_VALID_ST;

typedef struct										//�������������
{
	unsigned int 	uiPrmvType;						//ԭ������
}CCSM_CALLEE_MUSIC_END_ST, CCAB_CALLEE_MUSIC_END_ST;

typedef struct										//���з�����DTMF�ź�
{
	unsigned int 	uiPrmvType;						//ԭ������
	int				iDtmfType;						//DTMF�ź�����
}CCSM_CALLEE_SEND_DTMF_ST, CCAB_CALLEE_SEND_DTMF_ST;

typedef struct										//һ������
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned char	aucUserName[USER_NAME_BUFF_LEN];
	unsigned int	uiMsgSeq;
	int 			iType;
}CCSM_OPENDOOR_ST, CCAB_OPENDOOR_ST;

typedef struct									//�ݿڻ��������
{
	unsigned int 	uiPrmvType;						//ԭ������
}CCSM_VEDIO_MONITOR_ST, CCAB_VEDIO_MONITOR_ST;

typedef struct									//�ݿڻ�ȡ���������
{
	unsigned int 	uiPrmvType;						//ԭ������
}CCSM_VEDIO_MONITOR_CANCEL_ST, CCAB_VEDIO_MONITOR_CANCEL_ST;

typedef struct										//����
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned char 	aucPhoneNum[PHONE_NUM_BUFF_LEN];//�绰����
	unsigned char 	aucMediaType;					//��ý������
	unsigned char 	aucHintMusicEn;					//��ʾ��
	unsigned char   aucCallPreview;					//0����Ԥ�� 1:Ԥ��
}CCSIP_CALL_ST;

typedef struct										//����
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned long   ulCallId;						//
}CCSIP_ANSWER_ST;

typedef struct										//ȡ������
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned long   ulCallId;						//
}CCSIP_TERMINATE_ST;

typedef struct										//ȡ�����к���
{
	unsigned int 	uiPrmvType;						//ԭ������
}CCSIP_TERMINATE_ALL_ST, CCSIP_CALL_TIMEOUT_ST;

typedef struct
{
	unsigned int 	uiPrmvType;
}SIPCC_RECV_EARLY_MEDIA_ST;

typedef struct										//���з�����
{
	unsigned int    uiPrmvType;						//ԭ������
	unsigned long   ulCallId;
}SIPCC_CALLEE_PICK_UP_ST, SIPCC_PROCESS_ST, SIPCC_RING_ST;

typedef struct										//���з���ʾ�����Ž���
{
	unsigned int    uiPrmvType;						//ԭ������
}SIPCC_CALLEE_MUSIC_END_ST;

typedef struct
{
	unsigned int    uiPrmvType;						//ԭ������
	unsigned int	uiRegStatus;					//ע��״̬
}SIPAB_REG_STATUS_ST;

typedef struct
{
	unsigned int    uiPrmvType;						//ԭ������
}SIPCC_STOP_CALLING_ST;

typedef struct
{
	unsigned int    uiPrmvType;						//ԭ������
}OTAB_REBOOT_ST;

typedef struct
{
	unsigned int    uiPrmvType;						//ԭ������
	unsigned int	uiNetworkStatus;				//����״̬
}SIPAB_NETWORK_STATUS_ST;

typedef struct
{
	unsigned int    uiPrmvType;						//ԭ������
}SIPAB_STOP_RING_ST;

typedef struct										//���з��һ�
{
	unsigned int    uiPrmvType;						//ԭ������
	unsigned long   ulCallId;
}SIPCC_CALLEE_HANG_OFF_ST;

typedef struct										//���з��쳣
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned int  	uiErrorCause;					//�쳣ԭ��
	unsigned long  ulCallId;						//�쳣ͨ��ID
}SIPCC_CALLEE_ERR_ST,LANCC_CALLEE_ERR_ST;

typedef struct										//���з�����DTMF
{
	unsigned int 	uiPrmvType;						//ԭ������
	int 			iDtmfType;						//DTMF�ź�����
}SIPCC_CALLEE_SEND_DTMF_ST;

//typedef struct
//{
//	unsigned int 	uiPrmvType;						//ԭ������
//}SIPAB_DTMF_OPENDOOR_ST;

typedef struct										//advert
{
	unsigned int 	uiPrmvType;						//ԭ������
}TMAB_ADVERT_UPDATE_ST;

typedef struct
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned int	uiLinkStatus;					//����״̬
}TMAB_TM_LINK_STATUS_ST;

typedef struct										//��Ƶ���� ��Ϣ���ݽṹ
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned long 	ulInComingCallId;				//��Ƶ���� Call ID
	unsigned char   aucPhoneNum[PHONE_NUM_BUFF_LEN];//��Ƶ���� �绰����
}SIPCC_VIDEO_INCOMING_ST;

typedef struct										//��Ƶ���� ��Ϣ���ݽṹ
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned long 	ulInComingCallId;				//�������� Call ID
	unsigned char   aucPhoneNum[PHONE_NUM_BUFF_LEN];//�������� �绰����
	unsigned char   aucUserName[USER_NAME_BUFF_LEN];
}SIPCC_AUDIO_INCOMING_ST;


typedef struct										//message ��Ϣ���ݽṹ
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned char   aucPhoneNum[PHONE_NUM_BUFF_LEN];//�������� �绰����
	unsigned char   aucUserName[USER_NAME_BUFF_LEN];//sip:user@localhost.com
	unsigned int    uiMsgSeq;					    //msg  seq
	unsigned int    uiUserDataType;					//data type
}SIPCC_MESSAGE_INCOMING_ST;

typedef struct					//��ץ
{
	unsigned int 	uiPrmvType;						//ԭ������
	const char *SipNum;
}SS_MSG_DATA_ST;

typedef struct
{
	YK_TIMER_ST     *pstTimer;
	unsigned long	ulMagicNum;
}CC_TIMER_ST,TM_TIMER_ST;

typedef struct
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned int	uiTimerId;						//��ʱ��ID
}CCCC_TIMEROUT_EVENT_ST;

typedef struct
{
	unsigned int uiPrmvType;
}CCCC_CANCAL_EVENT_ST;

typedef struct
{
    unsigned int    uiPrmvType;
    unsigned char   aucUserName[USER_NAME_BUFF_LEN];
    unsigned char   aucPassword[PASSWORD_BUFF_LEN];
    unsigned char   aucProxy[PROXY_BUFF_LEN];
    unsigned char   aucRoute[ROUTE_BUFF_LEN];
    unsigned int    uiExpires;
}TMSIP_REGISTER_ST;


typedef struct
{
    unsigned int       uiPrmvType;
}TMSIP_UNREGISTER_ST;


typedef struct
{
    unsigned int    uiPrmvType;
    unsigned char   aucUserName[USER_NAME_BUFF_LEN];
    unsigned char   aucPassword[PASSWORD_BUFF_LEN];
    unsigned char   aucProxy[PROXY_BUFF_LEN];
    unsigned char   aucRoute[ROUTE_BUFF_LEN];
    unsigned int    uiExpires;
}TMSIP_REGISTER_UPDATE_ST;

typedef struct
{
	unsigned int uiPrmvType;
}XTSM_PICKUPIDU_ST;

typedef struct
{
	unsigned int uiPrmvType;
	unsigned char aucCallAddr[20];
}XTSM_INVOKECALL_DATA_ST;

typedef struct
{
	unsigned int uiPrmvType;
	unsigned int uiCallID;
}XTSM_TERMINATECALL_DATA_ST;

typedef struct
{
	unsigned int uiPrmvType;
}XTSM_MONITOR_CANCEL_ST;

typedef struct
{
	unsigned int uiPrmvType;
}SMXT_EVENT_MSG_ST, XT_EVENT_MSG_ST, AB_EVENT_MSG_ST;

typedef struct
{
	unsigned int uiPrmvType;
}LAN_EVENT_MSG_ST;

typedef struct
{
	unsigned int uiPrmvType;
	char cFromIP[20];//IP
}LAN_WATCH_MSG_ST;

typedef struct _SMXT_CALLEE_ERROR_MSG_ST
{
	unsigned int uiPrmvType;
	unsigned int uiErrorCause;					//�쳣ԭ��
}SMXT_CALLEE_ERROR_MSG_ST;

typedef struct
{
	unsigned int uiPrmvType;
	unsigned int uiValidateStatus;
}
SMAB_RFIDCARD_VALIDATE_ST;

typedef struct										//¥��Խ�ϵͳ����
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned char 	aucRoomNum[ROOM_NUM_BUFF_LEN];//�����
}TMSM_DEMO_CALL_ST;


#ifdef _YK_GYY_
typedef struct                                      //�������֪ͨ
{
	unsigned int 	uiPrmvType;						//ԭ������
}SIPTM_SOFT_UPDATE_ST;

typedef struct                                      //���ò�������֪ͨ
{
	unsigned int 	uiPrmvType;						//ԭ������
}SIPTM_CONFIG_UPDATE_ST;

typedef struct                                      //�Ž��������֪ͨ
{
	unsigned int 	uiPrmvType;						//ԭ������
}SIPTM_PWD_UPDATE_ST;

typedef struct                                      //�Ž�������֪ͨ
{
	unsigned int 	uiPrmvType;						//ԭ������
}SIPTM_CARD_UPDATE_ST;
#endif

#if _TM_TYPE_ ==  _GYY_I6_
typedef struct                                      //I6����
{
	unsigned int 	uiPrmvType;						//ԭ������
}TM_I6_BOOT_ST;

typedef struct                                      //I6ˢ���ϱ�
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned char   aucCardInfo[132];
}TM_I6_CARD_SWIP_ST;

typedef struct                                      //I6�澯
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned int 	uiAlarmNo;						//�澯NO
	unsigned char   auStateCode[12];				//״̬
}TM_I6_ALARM_UPDATE_ST;
#endif

typedef struct                                      //I6����
{
	unsigned int 	uiPrmvType;						//ԭ������
	unsigned char   auAdvInfo[256];				//״̬
}TM_JNI_ADV_INFO_ST;

//Media Type
#define     MEDIA_TYPE_AUDIO            0x01
#define     MEDIA_TYPE_AUDIO_VIDEO      0x02
#define     MEDIA_TYPE_TEL		        0x03

//HintMusic Enable
#define		HINT_MUSIC_ENABLE           0x01
#define		HINT_MUSIC_DISABLE          0x02

extern int IDBTMain(int argc, char *argv[]);

extern int SaveIdbtConfig(void);


#endif /* IDBT_H_ */

