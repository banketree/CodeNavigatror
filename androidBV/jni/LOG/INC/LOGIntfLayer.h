 /**
	*@file   LOGIntLayer.h
	*@brief log4c日志接口
	*
	*		该文件主要用于:
	*		初始化log4c扩展日志输出格式，日志文件循环策略，日志启动资源初始化与清理
	*
	*      使用简要说明(日志文件分事件日志与运行日志)：
	*
	*      1.使用前需在log4crc文件内配置日志的存放目录(logdir的值)，供FTP上传:
	*        <appender name="appender_eventlog"  logdir="/idbt/log" ...
    *        <appender name="appender_runlog"      logdir="/idbt/log" ...
    *
    *      2.使用LOG_EVENTLOG_INFO记录事件日志:
    *        见LOGIntfLayer.h第xx行说明
    *        使用LOG_RUNLOG_DEBUG/LOG_RUNLOG_INFO/LOG_RUNLOG_WARN/LOG_RUNLOG_ERROR记录运行日志:
    *        见LOGIntfLayer.h第xx行说明
    *
    *      3.通过配置<category name="r_runlog" priority="error"的priority属性可以控制什么等级的日志可以记录到运行日志文件或打印到屏幕
    *        备注：priority可以配置为debug/info/warn/error(其他选项暂时不用), 低指定等级不触发category进行相应的日志打印或记录。
	*        只有大于等于category的priority定义的等级时候才会触发相应appender进行日志记录。
	*
	*      4.日志等级说明：
    *		DEBUG    Level : 指出细粒度信息事件对调试应用程序是非常有帮助的。
    *		INFO     Level : 表明消息在粗粒度级别上突出强调应用程序的运行过程。
    *		WARN     Level : 表明会出现潜在错误的情形，但不影响系统的继续运行。
    *		ERROR    Level : 指出发生错误事件，但影响系统的继续运行。
    *
    *
    *
    *
    备注：配置文件注释说明：
    <!-- 事件日志输出方式定义 根据配置事件日志记录会触发运行日志记录以及屏幕打印-->
	<!-- 无需修改配置 程序中使用LOG_EVENTLOG_INFO宏进行日志记录，会依次输出到运行日志文件/事件日志文件/屏幕-->
   	<category name="e_runlog" priority="info" appender="appender_runlog" /> <!-- 定义运行日志输出类型 -->
   	<category name="e_runlog.e_eventlog" priority="info" appender="appender_eventlog" /><!-- 定义事件日志输出类型 -->
  	<category name="e_runlog.e_eventlog.e_screen" priority="info" appender="stdout" /> <!-- 定义屏幕输出类型-->


  	<!--
		扩展策略使用定义：datednamelog(文件名生成策略)
	-->
     <rollingpolicy name="rollingpolicy_eventlog" type="datednamelog" />
     <rollingpolicy name="rollingpolicy_runlog" type="datednamelog"  />

   	<!--
		扩展输出目标策略使用定义：datednamelog(文件名生成策略)
	-->
    <appender name="appender_eventlog" type="rollingfile" logdir="./LogDir/Log" prefix="eventlog"  layout="ext_eventlog" rollingpolicy="rollingpolicy_eventlog" />
    <appender name="appender_runlog" type="rollingfile" logdir="./LogDir/Log" prefix="runlog"  layout="ext_runlog" rollingpolicy="rollingpolicy_runlog"  />

  	<!-- 系统默认输出目标定义-->
  	<appender name="stdout" type="stream" layout="basic"/>

	 <!--
		扩展输出布局使用定义：ext_eventlog(事件日志输出布局)/ext_runlog(运行日志输出布局)
	-->
	<layout name="ext_eventlog" type="ext_eventlog"/>
	<layout name="ext_runlog" type="ext_runlog"/>

	<!-- 系统默认输出布局定义-->
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

/**@brief log4c 配置文件路径 */
//#define LOG4C_RCPATH "LOG/LOGConfig"
/**@brief 事件日志category命名 */
#define EVENTLOG_CATEGORY_NAME      "e_runlog.e_eventlog.e_screen"
/**@brief 运行日志category命名 */
#define RUNLOG_CATEGORY_NAME        "r_runlog.r_screen"
/**@brief 事件日志appender命名 */
#define EVENTLOG_APPENDER_NAME      "appender_eventlog"
/**@brief 运行日志appender命名 */
#define RUNLOG_APPENDER_NAME        "appender_runlog"

#define UPLOAD_HOUR      (23)        //每天定时上传时间(小时)
#define UPLOAD_MIN       (00)        //每天定时上传时间(分钟)


/**@brief 日志管理结构体 */
typedef struct
{
	pthread_mutex_t stMutex;
	int iEnFlg;
	log4c_category_t *pstEventLogger;
	log4c_category_t *pstRunLogger;
} LOG_MNG_ST;
extern LOG_MNG_ST g_stLogMng;


/**@brief 供事件日志写入一串空行作为分割行使用 */
#define LOG_CMD_DIVIDING_LENES "CMD_LINE_FEED"
#define SECOND_PREFIX          "0000000000000000"                       //日志中文件名滚动策略中使用到的设备ID的默认非法ID
#define TERMINAL_ID_LEN        (32)                                     //设备IDmax长度


//pthread_mutex_lock(&(g_stLogMng.stMutex));
//pthread_mutex_unlock(&(g_stLogMng.stMutex));
//20120730因后续需求多处增加上传功能，宏定义代表的内容增加，需改为函数调用
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





//使用__log4c_category_vlog时vprintf发生段错误
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


///**@brief 事件日志列表项 */

//#define EVT_OUT_DOOR_CALL_START "out door call start"                                         //梯口机呼叫开始
//#define EVT_OUT_DOOR_CALL_TIMEOUT "out door call timeout"                                     //梯口机呼叫超时(指的梯口呼叫室内机未接听的呼叫超时)
//#define EVT_OUT_DOOR_CALL_END "out door call end"                                             //梯口机呼叫结束

//#define EVT_INDOOR_RINGING "indoor ringing"                                                   //室内机振铃
//#define EVT_INDOOR_PICK_UP "indoor pick up"                                                   //室内机接听
//#define EVT_INDOOR_OPEN_DOOR "indoor open door"                                               //室内机开门
//#define EVT_INDOOR_HAND_UP "indoor hand up"                                                   //室内机挂机

//#define EVT_INDOOR_MONITOR_REQUEST "indoor monitor request"                                   //室内机监控请求
//#define EVT_INDOOR_MONITOR_END "indoor monitor end"                                           //室内机监控结束

//#define EVT_SIP_CALL_START "sip call start"                                                   //SIP呼叫开始
//#define EVT_SIP_CALL_RINGING "sip call ringing"                                               //SIP呼叫振铃
//#define EVT_SIP_CALL_FAIL "sip call fail"                                                     //SIP呼叫失败
//#define EVT_USERS_PHONE_PICK_UP "user's phone pick up"                                        //手机或客戶端接听
//#define EVT_USERS_PHONE_OPEN_DOOR "user's phone open door"                                    //手机或客戶端开门
//#define EVT_USERS_PHONE_HAND_UP "user's phone hand up"                                        //手机或客戶端挂机结束通话
//#define EVT_USERS_PHONE_COMM_TIMEOUT "user's phone comm timeout"                              //手机或客戶端通话超时

//#define EVT_CLIENT_MONITOR_START "client monitor start"                                       //客戶端监控开始
//#define EVT_CLIENT_MONITOR_END "client monitor end"                                           //客戶端监控结束
//#define EVT_CLIENT_MONITOR_TIMEOUT "client monitor timeout"                                   //客戶端监控超时

//#define EVT_CARD_SWIPING "card swiping"                                                       //刷卡开门事件
//#define EVT_ONEKEY_OPEN_DOOR "onekey open door"                                               //一键开门事件

//#define EVT_DEMO_CALL "demo call"                                                             //访客体验

//#define EVT_IMS_REGISTER "ims register"                                                       //IMS注册事件(成功/失败)
//#define EVT_CONFIG_RELOAD "reload config"                                                     //更新配置(成功/失败)
//#define EVT_SYSTEM_REBOOT "system reboot"                                                     //系统重启(系统每次正常与非正常重启都记录一次)
//#define EVT_PLATFORM_REBOOT "platform reboot"                                                 //平台重启命令(平台下发)
//#define EVT_APP_REBOOT "app reboot"                                                           //应用程序重启命令(程序中调用reboot进行系统重启)
//#define EVT_POWER_DOWN "power down"                                                           //系统掉电
//#define EVT_FTP_DOWNLOAD "ftp download"                                                       //FTP下载(成功/失败)
//#define EVT_FTP_UPLOAD "ftp upload"                                                           //FTP上传(成功/失败)
//#define EVT_APP_UPDATE "app update"                                                           //版本升级(成功/失败)
//#define EVT_NET_CONNECT "net connect"                                                         //网络接入(成功/失败)
//#define EVT_NET_DEVICE_REBOOT "net device reboot"                                             //外部网络设备重启



/**@brief 事件日志关于用户动作的枚举定义 */
typedef enum
{
	LOG_EVT_OUT_DOOR_CALL_START = 1,    //1 //梯口机呼叫开始
	LOG_EVT_OUT_DOOR_CALL_TIMEOUT,      //2 //梯口机呼叫超时(指的梯口呼叫室内机未接听的呼叫超时)
	LOG_EVT_OUT_DOOR_CALL_END,          //3 //梯口机呼叫结束

	LOG_EVT_INDOOR_RINGING,             //4 //室内机振铃
	LOG_EVT_INDOOR_PICK_UP,             //5 //室内机接听
	LOG_EVT_INDOOR_OPEN_DOOR,           //6 //室内机开门
	LOG_EVT_INDOOR_HAND_UP,             //7 //室内机挂机

	LOG_EVT_INDOOR_MONITOR_REQUEST,     //8 //室内机监控请求
	LOG_EVT_INDOOR_MONITOR_END,         //9 //室内机监控结束

	LOG_EVT_SIP_CALL_START,             //10 //SIP呼叫开始
	LOG_EVT_SIP_CALL_RINGING,           //11 //SIP呼叫振铃
	LOG_EVT_SIP_CALL_FAIL,              //12 //SIP呼叫失败
	LOG_EVT_USERS_PHONE_PICK_UP,        //13 //手机或客戶端接听
	LOG_EVT_USERS_PHONE_OPEN_DOOR,      //14 //手机或客戶端开门
	LOG_EVT_USERS_PHONE_HAND_UP,        //15 //手机或客戶端挂机结束通话
	LOG_EVT_USERS_PHONE_COMM_TIMEOUT,   //16 //手机或客戶端通话超时

	LOG_EVT_CLIENT_MONITOR_START,       //17  //客戶端监控开始
	LOG_EVT_CLIENT_MONITOR_END,         //18 //客戶端监控结束
	LOG_EVT_CLIENT_MONITOR_TIMEOUT,     //19 //客戶端监控超时

	LOG_EVT_CARD_SWIPING,               //20 //刷卡开门事件
	LOG_EVT_ONEKEY_OPEN_DOOR,           //21 //一键开门事件
	LOG_EVT_DEMO_CALL,                  //22 //访客体验

}LOG_EVT_USER_EN;

/**@brief 事件日志关于系统动作的枚举定义 */
typedef enum
{
	LOG_EVT_IMS_REGISTER = 51,         //51 //IMS注册事件(成功/失败)
	LOG_EVT_RELOAD_CONFIG,             //52 //更新配置(成功/失败)
	LOG_EVT_SYSTEM_REBOOT,             //53 //系统重启(系统每次正常与非正常重启都记录一次)
	LOG_EVT_PLATFORM_REBOOT,           //54 //平台重启命令(平台下发)
	LOG_EVT_APP_REBOOT,                //55 //应用程序重启命令(系统发生不可恢复情景时调用reboot进行系统重启)
	LOG_EVT_POWER_DOWN,                //56 //系统掉电
	LOG_EVT_FTP_DOWNLOAD,              //57 //FTP下载(成功/失败)
	LOG_EVT_FTP_UPLOAD,                //58 //FTP上传(成功/失败)
	LOG_EVT_APP_UPDATE,                //59 //版本升级(成功/失败)
	LOG_EVT_NET_CONNECT,               //60 //网络接入(成功/失败)
	LOG_EVT_NET_DEVICE_REBOOT,         //61 //外部网络设备重启
	LOG_EVT_SYSTEM_ERROR               //62 //系统异常报错
}LOG_EVT_SYS_EN;

///**@brief 日志类型 */
//typedef enum
//{
//		ALL_LOG = 0,      //代表所有日志类型
//		EVENT_LOG,        //代表所有日志类型
//		SYS_LOG           //代表所有日志类型
//}LOG_OBJ_TYPE_EN;


/**
 * @brief                   日志模块初始化
 *
 * 使用前先建立供FTP上传的日志文件存放目录并在配置文件内设置好，
 * 并建立日志文件存放的临时目录(命名规则为上传目录加"Temp")
 * 如上传日志的存放目录为"Log"，则日志模块临时存放日志目录为 "LogTemp"
 *
 * @param[in]
 * @return  	             0/<0  0-成功 <0-失败
 */
int LOGIntfInit(void);

/**
 * @brief                 日志模块资源清理
 *
 * @param[in]      无
 * @return             无
 */
void LOGIntfFini(void);

/**
 * @brief              上传日志前的资源准备
 *
 * @param[in]    无
 * @return           0成功 <0失败
 */
int LOGUploadResPrepross(void);



//brief 系统日志接口
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




//事件日志格式：
//【时间】【分隔符】【单元号】【分隔符】【事件代码】【分隔符】【事件结果】【分隔符】【备注】
/*
			NO   内容	长度（字符）	备注
			1    时间	19          例如2011-12-18 17:07:21
			2    分隔符	1           空格
			3    单元号	4           例如：1105  代表房号1105单元发生的事件
			4    分隔符	1           空格
			5    事件代码	2           具体见附表1
			6    分隔符	1           空格
			7    事件结果	1           0：代表成功 / 1：代表失败 /无成功与失败概念时，用0替代
			8	分隔符	1	        空格
			9	备注	    n           1.sip呼叫失败的失败代码
			                        2.刷卡开门的卡号
			                        3.FTP上传下载的文件名
			                        4.一键开门的开门号码
			                        5.版本升级的旧版本号、新版本号(old_version,new_version)
			                        6.配置更新的文件名
			                        7.SIP呼叫的电话号码
 注：1、每条日志一行。
	2、备注代码项，目前只有7种情况下需要填写。见上表格。
*/
//事件日志接口 事件日志记录使用LOG_EVENTLOG_INFO这个等级即可
//1：事件日志(日志记录):       LOG_EVENTLOG_INFO(EVENTLOG_FORMAT, "单元号", 事件代码, 事件结果, "备注内容");
//                          备注代码项，只有7种情况填写。见上表。
//2:事件日志(用于一次通话换行): LOG_EVENTLOG_INFO("%s", LOG_CMD_DIVIDING_LENES);

//事件日志格式化定义, 供LOG_EVENTLOG_INFO使用 (单元号位数12位(不足左补空))，无房间号 00000000000
#define LOG_EVENTLOG_FORMAT "%12s %02d %d %s"
#define LOG_EVENTLOG_NO_ROOM "000000000000"


//事件日志接口
//#define LOG_EVENTLOG_DEBUG(fmt, args...) LOG_EVENT(LOG4C_PRIORITY_DEBUG, fmt, ## args)
#define LOG_EVENTLOG_INFO(fmt, args...) LOG_EVENT(LOG4C_PRIORITY_INFO, fmt, ## args)
//#define LOG_EVENTLOG_WARN(fmt, args...) LOG_EVENT(LOG4C_PRIORITY_WARN, fmt, ## args)
//#define LOG_EVENTLOG_ERROR(fmt, args...) LOG_EVENT(LOG4C_PRIORITY_ERROR, fmt, ## args)



#endif /* LOG_INTF_LAYER_H_ */
