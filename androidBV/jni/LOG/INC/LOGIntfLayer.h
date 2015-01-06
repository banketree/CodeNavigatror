 /**
	*@file   LOGIntLayer.h
	*@brief log4c��־�ӿ�
	*
	*		���ļ���Ҫ����:
	*		��ʼ��log4c��չ��־�����ʽ����־�ļ�ѭ�����ԣ���־������Դ��ʼ��������
	*
	*      ʹ�ü�Ҫ˵��(��־�ļ����¼���־��������־)��
	*
	*      1.ʹ��ǰ����log4crc�ļ���������־�Ĵ��Ŀ¼(logdir��ֵ)����FTP�ϴ�:
	*        <appender name="appender_eventlog"  logdir="/idbt/log" ...
    *        <appender name="appender_runlog"      logdir="/idbt/log" ...
    *
    *      2.ʹ��LOG_EVENTLOG_INFO��¼�¼���־:
    *        ��LOGIntfLayer.h��xx��˵��
    *        ʹ��LOG_RUNLOG_DEBUG/LOG_RUNLOG_INFO/LOG_RUNLOG_WARN/LOG_RUNLOG_ERROR��¼������־:
    *        ��LOGIntfLayer.h��xx��˵��
    *
    *      3.ͨ������<category name="r_runlog" priority="error"��priority���Կ��Կ���ʲô�ȼ�����־���Լ�¼��������־�ļ����ӡ����Ļ
    *        ��ע��priority��������Ϊdebug/info/warn/error(����ѡ����ʱ����), ��ָ���ȼ�������category������Ӧ����־��ӡ���¼��
	*        ֻ�д��ڵ���category��priority����ĵȼ�ʱ��Żᴥ����Ӧappender������־��¼��
	*
	*      4.��־�ȼ�˵����
    *		DEBUG    Level : ָ��ϸ������Ϣ�¼��Ե���Ӧ�ó����Ƿǳ��а����ġ�
    *		INFO     Level : ������Ϣ�ڴ����ȼ�����ͻ��ǿ��Ӧ�ó�������й��̡�
    *		WARN     Level : ���������Ǳ�ڴ�������Σ�����Ӱ��ϵͳ�ļ������С�
    *		ERROR    Level : ָ�����������¼�����Ӱ��ϵͳ�ļ������С�
    *
    *
    *
    *
    ��ע�������ļ�ע��˵����
    <!-- �¼���־�����ʽ���� ���������¼���־��¼�ᴥ��������־��¼�Լ���Ļ��ӡ-->
	<!-- �����޸����� ������ʹ��LOG_EVENTLOG_INFO�������־��¼�������������������־�ļ�/�¼���־�ļ�/��Ļ-->
   	<category name="e_runlog" priority="info" appender="appender_runlog" /> <!-- ����������־������� -->
   	<category name="e_runlog.e_eventlog" priority="info" appender="appender_eventlog" /><!-- �����¼���־������� -->
  	<category name="e_runlog.e_eventlog.e_screen" priority="info" appender="stdout" /> <!-- ������Ļ�������-->


  	<!--
		��չ����ʹ�ö��壺datednamelog(�ļ������ɲ���)
	-->
     <rollingpolicy name="rollingpolicy_eventlog" type="datednamelog" />
     <rollingpolicy name="rollingpolicy_runlog" type="datednamelog"  />

   	<!--
		��չ���Ŀ�����ʹ�ö��壺datednamelog(�ļ������ɲ���)
	-->
    <appender name="appender_eventlog" type="rollingfile" logdir="./LogDir/Log" prefix="eventlog"  layout="ext_eventlog" rollingpolicy="rollingpolicy_eventlog" />
    <appender name="appender_runlog" type="rollingfile" logdir="./LogDir/Log" prefix="runlog"  layout="ext_runlog" rollingpolicy="rollingpolicy_runlog"  />

  	<!-- ϵͳĬ�����Ŀ�궨��-->
  	<appender name="stdout" type="stream" layout="basic"/>

	 <!--
		��չ�������ʹ�ö��壺ext_eventlog(�¼���־�������)/ext_runlog(������־�������)
	-->
	<layout name="ext_eventlog" type="ext_eventlog"/>
	<layout name="ext_runlog" type="ext_runlog"/>

	<!-- ϵͳĬ��������ֶ���-->
	<layout name="basic" type="basic"/>
	*
	*@author chensq
	*@version 1.0
	*@date 2012-4-23
	*/

#ifndef LOG4C_INTF_LAYER_H_
#define LOG4C_INTF_LAYER_H_

#include <pthread.h>
#include "log4c.h"
#include "IDBT.h"
#include "FPInterface.h"

#include <string.h>
#include <jni.h>
#include <android/log.h>

/**@brief log4c �����ļ�·�� */
//#define LOG4C_RCPATH "LOG/LOGConfig"
/**@brief �¼���־category���� */
#define EVENTLOG_CATEGORY_NAME      "e_runlog.e_eventlog.e_screen"
/**@brief ������־category���� */
#define RUNLOG_CATEGORY_NAME        "r_runlog.r_screen"
/**@brief �¼���־appender���� */
#define EVENTLOG_APPENDER_NAME      "appender_eventlog"
/**@brief ������־appender���� */
#define RUNLOG_APPENDER_NAME        "appender_runlog"

#define UPLOAD_HOUR      (23)        //ÿ�춨ʱ�ϴ�ʱ��(Сʱ)
#define UPLOAD_MIN       (00)        //ÿ�춨ʱ�ϴ�ʱ��(����)


/**@brief ��־����ṹ�� */
typedef struct
{
	pthread_mutex_t stMutex;
	int iEnFlg;
	log4c_category_t *pstEventLogger;
	log4c_category_t *pstRunLogger;
} LOG_MNG_ST;
extern LOG_MNG_ST g_stLogMng;


/**@brief ���¼���־д��һ��������Ϊ�ָ���ʹ�� */
#define LOG_CMD_DIVIDING_LENES "CMD_LINE_FEED"
#define SECOND_PREFIX          "0000000000000000"                       //��־���ļ�������������ʹ�õ����豸ID��Ĭ�ϷǷ�ID
#define TERMINAL_ID_LEN        (32)                                     //�豸IDmax����


//pthread_mutex_lock(&(g_stLogMng.stMutex));
//pthread_mutex_unlock(&(g_stLogMng.stMutex));
//20120730���������ദ�����ϴ����ܣ��궨�������������ӣ����Ϊ��������
#define LOG_EVENT(prio, fmt, args...) \
	{ \
		if( g_stLogMng.iEnFlg == 1 ) \
		{ \
			pthread_mutex_lock(&(g_stLogMng.stMutex)); \
			log4c_location_info_t locinfo = LOG4C_LOCATION_INFO_INITIALIZER(NULL); \
			log4c_category_log_locinfo(g_stLogMng.pstEventLogger, &locinfo, prio, fmt, ##args); \
			pthread_mutex_unlock(&(g_stLogMng.stMutex)); \
		} \
		else \
		{ \
			printf("[stdout] "fmt"\n", ##args); \
		} \
	}
#define LOG_RUN(prio, fmt,args...) \
	{ \
		if( g_stLogMng.iEnFlg == 1 ) \
		{ \
			pthread_mutex_lock(&(g_stLogMng.stMutex)); \
			log4c_location_info_t locinfo = LOG4C_LOCATION_INFO_INITIALIZER(NULL); \
			log4c_category_log_locinfo(g_stLogMng.pstRunLogger, &locinfo, prio, fmt, ##args);  \
			pthread_mutex_unlock(&(g_stLogMng.stMutex)); \
		} \
		else \
		{ \
			printf("[stdout] "fmt"\n", ##args); \
		} \
	}





//ʹ��__log4c_category_vlogʱvprintf�����δ���
//#define LOG_EVENT(prio, fmt, args...) \
//	{ \
//		const log4c_location_info_t locinfo = LOG4C_LOCATION_INFO_INITIALIZER(NULL); \
//		__log4c_category_vlog(g_stLogMng.pstEventLogger, &locinfo, prio, fmt, ##args);  \
//	}
//#define LOG_RUN(prio, fmt,args...) \
//	{ \
//		const log4c_location_info_t locinfo = LOG4C_LOCATION_INFO_INITIALIZER(NULL); \
//		__log4c_category_vlog(g_stLogMng.pstRunLogger, &locinfo, prio, fmt, ##args);  \
//	}


///**@brief �¼���־�б��� */

//#define EVT_OUT_DOOR_CALL_START "out door call start"                                         //�ݿڻ����п�ʼ
//#define EVT_OUT_DOOR_CALL_TIMEOUT "out door call timeout"                                     //�ݿڻ����г�ʱ(ָ���ݿں������ڻ�δ�����ĺ��г�ʱ)
//#define EVT_OUT_DOOR_CALL_END "out door call end"                                             //�ݿڻ����н���

//#define EVT_INDOOR_RINGING "indoor ringing"                                                   //���ڻ�����
//#define EVT_INDOOR_PICK_UP "indoor pick up"                                                   //���ڻ�����
//#define EVT_INDOOR_OPEN_DOOR "indoor open door"                                               //���ڻ�����
//#define EVT_INDOOR_HAND_UP "indoor hand up"                                                   //���ڻ��һ�

//#define EVT_INDOOR_MONITOR_REQUEST "indoor monitor request"                                   //���ڻ��������
//#define EVT_INDOOR_MONITOR_END "indoor monitor end"                                           //���ڻ���ؽ���

//#define EVT_SIP_CALL_START "sip call start"                                                   //SIP���п�ʼ
//#define EVT_SIP_CALL_RINGING "sip call ringing"                                               //SIP��������
//#define EVT_SIP_CALL_FAIL "sip call fail"                                                     //SIP����ʧ��
//#define EVT_USERS_PHONE_PICK_UP "user's phone pick up"                                        //�ֻ���͑��˽���
//#define EVT_USERS_PHONE_OPEN_DOOR "user's phone open door"                                    //�ֻ���͑��˿���
//#define EVT_USERS_PHONE_HAND_UP "user's phone hand up"                                        //�ֻ���͑��˹һ�����ͨ��
//#define EVT_USERS_PHONE_COMM_TIMEOUT "user's phone comm timeout"                              //�ֻ���͑���ͨ����ʱ

//#define EVT_CLIENT_MONITOR_START "client monitor start"                                       //�͑��˼�ؿ�ʼ
//#define EVT_CLIENT_MONITOR_END "client monitor end"                                           //�͑��˼�ؽ���
//#define EVT_CLIENT_MONITOR_TIMEOUT "client monitor timeout"                                   //�͑��˼�س�ʱ

//#define EVT_CARD_SWIPING "card swiping"                                                       //ˢ�������¼�
//#define EVT_ONEKEY_OPEN_DOOR "onekey open door"                                               //һ�������¼�

//#define EVT_DEMO_CALL "demo call"                                                             //�ÿ�����

//#define EVT_IMS_REGISTER "ims register"                                                       //IMSע���¼�(�ɹ�/ʧ��)
//#define EVT_CONFIG_RELOAD "reload config"                                                     //��������(�ɹ�/ʧ��)
//#define EVT_SYSTEM_REBOOT "system reboot"                                                     //ϵͳ����(ϵͳÿ���������������������¼һ��)
//#define EVT_PLATFORM_REBOOT "platform reboot"                                                 //ƽ̨��������(ƽ̨�·�)
//#define EVT_APP_REBOOT "app reboot"                                                           //Ӧ�ó�����������(�����е���reboot����ϵͳ����)
//#define EVT_POWER_DOWN "power down"                                                           //ϵͳ����
//#define EVT_FTP_DOWNLOAD "ftp download"                                                       //FTP����(�ɹ�/ʧ��)
//#define EVT_FTP_UPLOAD "ftp upload"                                                           //FTP�ϴ�(�ɹ�/ʧ��)
//#define EVT_APP_UPDATE "app update"                                                           //�汾����(�ɹ�/ʧ��)
//#define EVT_NET_CONNECT "net connect"                                                         //�������(�ɹ�/ʧ��)
//#define EVT_NET_DEVICE_REBOOT "net device reboot"                                             //�ⲿ�����豸����



/**@brief �¼���־�����û�������ö�ٶ��� */
typedef enum
{
	LOG_EVT_OUT_DOOR_CALL_START = 1,    //1 //�ݿڻ����п�ʼ
	LOG_EVT_OUT_DOOR_CALL_TIMEOUT,      //2 //�ݿڻ����г�ʱ(ָ���ݿں������ڻ�δ�����ĺ��г�ʱ)
	LOG_EVT_OUT_DOOR_CALL_END,          //3 //�ݿڻ����н���

	LOG_EVT_INDOOR_RINGING,             //4 //���ڻ�����
	LOG_EVT_INDOOR_PICK_UP,             //5 //���ڻ�����
	LOG_EVT_INDOOR_OPEN_DOOR,           //6 //���ڻ�����
	LOG_EVT_INDOOR_HAND_UP,             //7 //���ڻ��һ�

	LOG_EVT_INDOOR_MONITOR_REQUEST,     //8 //���ڻ��������
	LOG_EVT_INDOOR_MONITOR_END,         //9 //���ڻ���ؽ���

	LOG_EVT_SIP_CALL_START,             //10 //SIP���п�ʼ
	LOG_EVT_SIP_CALL_RINGING,           //11 //SIP��������
	LOG_EVT_SIP_CALL_FAIL,              //12 //SIP����ʧ��
	LOG_EVT_USERS_PHONE_PICK_UP,        //13 //�ֻ���͑��˽���
	LOG_EVT_USERS_PHONE_OPEN_DOOR,      //14 //�ֻ���͑��˿���
	LOG_EVT_USERS_PHONE_HAND_UP,        //15 //�ֻ���͑��˹һ�����ͨ��
	LOG_EVT_USERS_PHONE_COMM_TIMEOUT,   //16 //�ֻ���͑���ͨ����ʱ

	LOG_EVT_CLIENT_MONITOR_START,       //17  //�͑��˼�ؿ�ʼ
	LOG_EVT_CLIENT_MONITOR_END,         //18 //�͑��˼�ؽ���
	LOG_EVT_CLIENT_MONITOR_TIMEOUT,     //19 //�͑��˼�س�ʱ

	LOG_EVT_CARD_SWIPING,               //20 //ˢ�������¼�
	LOG_EVT_ONEKEY_OPEN_DOOR,           //21 //һ�������¼�
	LOG_EVT_DEMO_CALL,                  //22 //�ÿ�����

}LOG_EVT_USER_EN;

/**@brief �¼���־����ϵͳ������ö�ٶ��� */
typedef enum
{
	LOG_EVT_IMS_REGISTER = 51,         //51 //IMSע���¼�(�ɹ�/ʧ��)
	LOG_EVT_RELOAD_CONFIG,             //52 //��������(�ɹ�/ʧ��)
	LOG_EVT_SYSTEM_REBOOT,             //53 //ϵͳ����(ϵͳÿ���������������������¼һ��)
	LOG_EVT_PLATFORM_REBOOT,           //54 //ƽ̨��������(ƽ̨�·�)
	LOG_EVT_APP_REBOOT,                //55 //Ӧ�ó�����������(ϵͳ�������ɻָ��龰ʱ����reboot����ϵͳ����)
	LOG_EVT_POWER_DOWN,                //56 //ϵͳ����
	LOG_EVT_FTP_DOWNLOAD,              //57 //FTP����(�ɹ�/ʧ��)
	LOG_EVT_FTP_UPLOAD,                //58 //FTP�ϴ�(�ɹ�/ʧ��)
	LOG_EVT_APP_UPDATE,                //59 //�汾����(�ɹ�/ʧ��)
	LOG_EVT_NET_CONNECT,               //60 //�������(�ɹ�/ʧ��)
	LOG_EVT_NET_DEVICE_REBOOT,         //61 //�ⲿ�����豸����
	LOG_EVT_SYSTEM_ERROR               //62 //ϵͳ�쳣����
}LOG_EVT_SYS_EN;

///**@brief ��־���� */
//typedef enum
//{
//		ALL_LOG = 0,      //����������־����
//		EVENT_LOG,        //����������־����
//		SYS_LOG           //����������־����
//}LOG_OBJ_TYPE_EN;


/**
 * @brief                   ��־ģ���ʼ��
 *
 * ʹ��ǰ�Ƚ�����FTP�ϴ�����־�ļ����Ŀ¼���������ļ������úã�
 * ��������־�ļ���ŵ���ʱĿ¼(��������Ϊ�ϴ�Ŀ¼��"Temp")
 * ���ϴ���־�Ĵ��Ŀ¼Ϊ"Log"������־ģ����ʱ�����־Ŀ¼Ϊ "LogTemp"
 *
 * @param[in]
 * @return  	             0/<0  0-�ɹ� <0-ʧ��
 */
int LOGIntfInit(void);

/**
 * @brief                 ��־ģ����Դ����
 *
 * @param[in]      ��
 * @return             ��
 */
void LOGIntfFini(void);

/**
 * @brief              �ϴ���־ǰ����Դ׼��
 *
 * @param[in]    ��
 * @return           0�ɹ� <0ʧ��
 */
int LOGUploadResPrepross(void);



//brief ϵͳ��־�ӿ�
#if 1
	#define LOG_RUNLOG_DEBUG(fmt, args...) LOG_RUN(LOG4C_PRIORITY_DEBUG, fmt, ## args)
	#define LOG_RUNLOG_INFO(fmt, args...) LOG_RUN(LOG4C_PRIORITY_INFO, fmt, ## args)
	#define LOG_RUNLOG_WARN(fmt, args...) LOG_RUN(LOG4C_PRIORITY_WARN, fmt, ## args)
	#define LOG_RUNLOG_ERROR(fmt, args...) LOG_RUN(LOG4C_PRIORITY_ERROR, fmt, ## args)
#else
	#define LOG_RUNLOG_DEBUG(fmt, args...) 
	#define LOG_RUNLOG_INFO(fmt, args...) LOG_RUN(LOG4C_PRIORITY_INFO, fmt, ## args)
	#define LOG_RUNLOG_WARN(fmt, args...) LOG_RUN(LOG4C_PRIORITY_WARN, fmt, ## args)
	#define LOG_RUNLOG_ERROR(fmt, args...) LOG_RUN(LOG4C_PRIORITY_ERROR, fmt, ## args)
#endif
//#define LOG_RUNLOG_DEBUG(fmt, args...) __android_log_print(LOG4C_PRIORITY_DEBUG, "com.fsti.ladder", fmt, ## args)
//#define LOG_RUNLOG_INFO(fmt, args...) __android_log_print(LOG4C_PRIORITY_INFO, "com.fsti.ladder", fmt, ## args)
//#define LOG_RUNLOG_WARN(fmt, args...) __android_log_print(LOG4C_PRIORITY_WARN, "com.fsti.ladder", fmt, ## args)
//#define LOG_RUNLOG_ERROR(fmt, args...) __android_log_print(LOG4C_PRIORITY_ERROR, "com.fsti.ladder", fmt, ## args)




//�¼���־��ʽ��
//��ʱ�䡿���ָ���������Ԫ�š����ָ��������¼����롿���ָ��������¼���������ָ���������ע��
/*
			NO   ����	���ȣ��ַ���	��ע
			1    ʱ��	19          ����2011-12-18 17:07:21
			2    �ָ���	1           �ո�
			3    ��Ԫ��	4           ���磺1105  ������1105��Ԫ�������¼�
			4    �ָ���	1           �ո�
			5    �¼�����	2           ���������1
			6    �ָ���	1           �ո�
			7    �¼����	1           0������ɹ� / 1������ʧ�� /�޳ɹ���ʧ�ܸ���ʱ����0���
			8	�ָ���	1	        �ո�
			9	��ע	    n           1.sip����ʧ�ܵ�ʧ�ܴ���
			                        2.ˢ�����ŵĿ���
			                        3.FTP�ϴ����ص��ļ���
			                        4.һ�����ŵĿ��ź���
			                        5.�汾�����ľɰ汾�š��°汾��(old_version,new_version)
			                        6.���ø��µ��ļ���
			                        7.SIP���еĵ绰����
 ע��1��ÿ����־һ�С�
	2����ע�����Ŀǰֻ��7���������Ҫ��д�����ϱ��
*/
//�¼���־�ӿ� �¼���־��¼ʹ��LOG_EVENTLOG_INFO����ȼ�����
//1���¼���־(��־��¼):       LOG_EVENTLOG_INFO(EVENTLOG_FORMAT, "��Ԫ��", �¼�����, �¼����, "��ע����");
//                          ��ע�����ֻ��7�������д�����ϱ�
//2:�¼���־(����һ��ͨ������): LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);

//�¼���־��ʽ������, ��LOG_EVENTLOG_INFOʹ�� (��Ԫ��λ��12λ(�����󲹿�))���޷���� 00000000000
#define LOG_EVENTLOG_FORMAT "%12s %02d %d %s"
#define LOG_EVENTLOG_NO_ROOM "000000000000"


//�¼���־�ӿ�
//#define LOG_EVENTLOG_DEBUG(fmt, args...) LOG_EVENT(LOG4C_PRIORITY_DEBUG, fmt, ## args)
#define LOG_EVENTLOG_INFO(fmt, args...) LOG_EVENT(LOG4C_PRIORITY_INFO, fmt, ## args)
//#define LOG_EVENTLOG_WARN(fmt, args...) LOG_EVENT(LOG4C_PRIORITY_WARN, fmt, ## args)
//#define LOG_EVENTLOG_ERROR(fmt, args...) LOG_EVENT(LOG4C_PRIORITY_ERROR, fmt, ## args)



#endif /* LOG_INTF_LAYER_H_ */
