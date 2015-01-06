 /**
	*@file   LOGIntLayer.c
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


#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifndef _YK_S5PV210_
	#include <sys/io.h>
	#include <libio.h>
	#include <sys/msg.h>
#endif
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mman.h>
#include <pthread.h>
#include "appender.h"
#include "LOGIntfLayer.h"
#include <FPInterface.h>
#include "IDBT.h"
#include "IDBTCfg.h"


typedef struct
{
	YK_TIMER_ST     *pstTimer;
	unsigned long	 ulMagicNum;
}LOG_TIMER_ST;

LOG_MNG_ST g_stLogMng;
YKHandle   g_LGThreadId;
LOG_TIMER_ST g_stLogUploadTimer;


extern int LOGExtLayoutTypeInit();
extern int LOGExtRollingPolicyTypeInit();



/**
 * @brief        调用TM与FP接口上传文件
 *
 * @param[in]    无
 * @return       无
 */
void LOGUploadExec(FP_TRANSFER_TYPE_EN enTransferType)
{
	LOG_RUNLOG_DEBUG("LG LGUploadExec Upload begin");

	char acHost[128] = {0x00};
	int iPort;
	char acUsername[128] = {0x00};
	char acPassword[128] = {0x00};

	//==========add by hb============
#ifdef IDBT_AUTO_TEST
	if(ATCheckLogDefault())
	{
		ATQueryFtpInfo(acHost, &iPort, acUsername, acPassword);
	}
	else
#endif
	{
	//========判断是否默认FTP==========
		TMQueryFtpInfo(acHost, &iPort, acUsername, acPassword);
	}
	// modify by xuqd
//	strcpy(acHost, "222.76.124.9");
//	iPort = 21;
//	strcpy(acUsername, "test");
//	strcpy(acPassword, "test");

	LOG_RUNLOG_DEBUG("LG LOGTask ftp info:[%s:%d:%s:%s]", acHost, iPort, acUsername, acPassword);

	if(strlen(acHost) == 0 || strlen(acUsername) == 0 || strlen(acPassword) == 0)
	{
		LOG_RUNLOG_DEBUG("LG LOGIntfFini TMQueryFtpInfo input param error");
	}
	else
	{
		//LOGUploadResPrepross();
		//==========add by hb============
#ifdef IDBT_AUTO_TEST
		if(ATCheckLogTest())
		{
			FPPutLog(acHost, iPort, acUsername, acPassword,LG_LOCAL_PATH,LG_REMOTE_PATH,NULL,NULL,FP_TRANSFER_NOWAIT);
		}
		else
#endif
		{
			FPPutLog(acHost, iPort, acUsername, acPassword,LG_LOCAL_PATH,LG_REMOTE_PATH,NULL,NULL,enTransferType);
		}
	}

	LOG_RUNLOG_DEBUG("LG LGUploadExec Upload end");
}




//add by chensq 20121107 因8126Flash有出现检测值不准
//90% 1M*1024*90%>>921K
int LOGCheckSpace2(const char *path, int maxSpace)
{
	int maxValue = 1024*maxSpace/100;
	//printf("maxValue:%d\n", maxValue);
	int err = 0;
	FILE *file;
	char line[300];
	char tmp[100];
	char *token;
	char value[10] = {0x00};
	int i = 0;
	sprintf(tmp, "du %s -k", path);
	file = popen(tmp, "r");
	if (file != NULL) {
		while (fgets(line, 300, file) != NULL)
		{
		}

		//check tab
		while( *(line + i) != 9)
		{
			//printf("%d:[%c]\n", i, *(line + i));
			i++;
		}

		memcpy(value, line, i);
		//printf("value:%s\n", value);
		//printf("unit:%x\n", *(line + i));

		//if is tab
		if(*(line + i) == 9)
		{
			if(atoi(value) >= maxValue)
			{
				LOG_RUNLOG_WARN("LG Not Enough Disk Space[%dK|max:%dK]\n", atoi(value), maxValue);
				err = -2;
			}
			else
			{
				//printf("debug: Space Enough[%dK|max:%dK]\n", atoi(value), maxValue);
				err = 0;
			}
		}
		else
		{
			//8126 max space is 1M
			LOG_RUNLOG_WARN("LG check space error\n");
			err = 0;
		}
		pclose(file);
	}
	return err;
}

///**
// * @brief        检查磁盘空间是否够用
// *
// * @param[in]    path 路径
// * @param[in]    maxSpace 最大空间(百分比,范围:>0 且 <100
// * @return       0成功 <0失败
// */
//#define _LINE_LENGTH        300
//int LOGCheckSpace(const char *path, int maxSpace)
//{
//	int err=-1;
//	FILE *file;
//	char line[_LINE_LENGTH];
//	char tmp[100];
////	char acMaxSpace[5] = {0x00};
//	char *token;
//
////	if(maxSpace <= 0 || maxSpace > 100)
////	{
////		snprintf(acMaxSpace, sizeof(acMaxSpace), "99%");
////	}
////	else
////	{
////		snprintf(acMaxSpace, sizeof(acMaxSpace), "%d%", maxSpace);
////
////	}
//	//printf("debug: LOGCheckSpace maxSpace:%s\n", acMaxSpace);
//
//	sprintf(tmp, "df %s", path);
//	file = popen(tmp, "r");
//	if (file != NULL) {
//		if (fgets(line, _LINE_LENGTH, file) != NULL)
//		{
//			if (fgets(line, _LINE_LENGTH, file) != NULL)
//			{
//				token = strtok(line, " ");
//				if (token != NULL)
//				{
//					//printf("Filesystem = %s\n", token);
//				}
//				token = strtok(NULL, " ");
//				if (token != NULL)
//				{
//					//printf("1K-blocks =%s\n", token);
//				}
//				token = strtok(NULL, " ");
//				if (token != NULL)
//				{
//					//printf("Used = %s\n", token);
//				}
//				token = strtok(NULL, " ");
//				if (token != NULL)
//				{
//					//printf("Available = %s\n", token);
//				}
//				token = strtok(NULL, " ");
//				if (token != NULL)
//				{
////					printf("Use% = %s\n", token);
//
////					if( strlen(token) == 3 && strncmp(token, acMaxSpace, 3) > 0 )
////					{
////						printf("warn:Not Enough Disk Space[%s|max:%s%]\n", token, acMaxSpace);
////						LOG_RUNLOG_WARN("LG: Not Enough Disk Space[%s|max:%s%]\n", token, acMaxSpace);
////					}
////					else if( strlen(token) == 3 && strncmp(token, acMaxSpace, 3) <= 0 )
////					{
////						//printf("debug: Space Enough[%s|max:%s%]\n", token, acMaxSpace);
////						err = 0;
////					}
////					else if( strlen(token) == 4 && strncmp(token, "100%", 4) == 0)
////					{
////						LOG_RUNLOG_WARN("LG: Not Enough Disk Space[%s]\n", token);
////						printf("warn:Not Enough Disk Space[%s]\n", token);
////					}
////					int temp = atoi(token);
////					printf("temp:%d\n", temp);
//
//					if(atoi(token) > maxSpace)
//					{
//						LOG_RUNLOG_WARN("LG: Not Enough Disk Space[%s|max:%d%]\n", token, maxSpace);
//					}
//					else
//					{
//						//printf("debug: Space Enough[%s|max:%d%]\n", token, maxSpace);
//						err = 0;
//					}
//				}
//			}
//		}
//		pclose(file);
//	}
//	return err;
//}



/**
 * @brief        上传日志前的资源准备
 *
 * @param[in]    this:log4c_appender_t*
 * @return       0成功 <0失败
 */
static int LOGAppendResPrepross(log4c_appender_t* this)
{
	int rc = 0;
	rollingfile_udata_t* rfup = (rollingfile_udata_t*) log4c_appender_get_udata(this);
	sd_debug("LOGAppendResPrepross[");
	//锁粒度加粗可取消该锁
	pthread_mutex_lock(&(rfup->rfu_mutex));
	if (rfup->rfu_conf.rfc_policy != NULL)
	{
		if ((rc = log4c_rollingpolicy_rollover(rfup->rfu_conf.rfc_policy,
				&rfup->rfu_current_fp)) <= ROLLINGPOLICY_ROLLOVER_ERR_CAN_LOG)
		{
			rfup->rfu_current_file_size = 0;
		}
	}
	sd_debug("]");
	pthread_mutex_unlock(&rfup->rfu_mutex);
	return (rc);
}

/**
 * @brief        上传日志前的资源准备
 *
 * @param[in]    无
 * @return       0成功 <0失败
 */
int LOGUploadResPrepross(void)
{
	system("busybox mkdir " LG_LOCAL_PATH " -p");

	int iRet = -2;
	log4c_appender_t *pstAppender =  NULL;

	printf("LG LOGUploadResPrepross begin");

	if(g_iRunLGTaskFlag == 0 )
	{
		printf("LG g_stLogMng.g_iRunLGTaskFlag==0 will not exec LOGUploadResPrepross");
		return -3;
	}

	pthread_mutex_lock(&(g_stLogMng.stMutex));

	pstAppender = log4c_appender_get("appender_eventlog");
	if( NULL == pstAppender )
	{
		printf("LG log4c_appender_get appender_eventlog error\n");
		//sleep(5); //测试用，便于在窗口观察
		return -1;
	}
	if(LOGAppendResPrepross(pstAppender)  == 0)
	{
		iRet++;
	}
	pstAppender = log4c_appender_get("appender_runlog");
	if( NULL == pstAppender )
	{
		printf("LG log4c_appender_get appender_runlog error\n");
		//sleep(5);  //测试用，便于在窗口观察
		return -1;
	}
	if(LOGAppendResPrepross(pstAppender)  == 0)
	{
		iRet++;
	}

	pthread_mutex_unlock(&(g_stLogMng.stMutex));

	return iRet;
}


void LOGUploadCallBack(void* pvPara)
{
	LOGUploadResPrepross();
	LOGUploadExec(FP_TRANSFER_NOWAIT);
}


/**
 * @brief      日志定时上传
 *
 * @param[in]
 * @return
 */
void *LOGTask(void* arg)
{
	//printf("debug:LOGTask begin\n");
	time_t stTimep;
	struct tm *pstTm;
	int iUploadFlg = 0;

	int iMode = log4c_get_upload_mode();				//日志上传模式
	int iUploadHour = log4c_get_upload_hour();			//上传时间(时)
	int iUploadMinute = log4c_get_upload_minute();		//上传时间（分）
	int iMaxspace = log4c_get_upload_maxspace();
	int iCycleTime = log4c_get_upload_cycle_time();		//周期时间（小时）

	LOG_RUNLOG_DEBUG("LG %d:%d:%d:%d:%d", iUploadHour, iUploadMinute, iMaxspace, g_stIdbtCfg.iLogUploadMode, g_stIdbtCfg.iLogUploadCycleTime);
	if(iUploadHour == 0 && iUploadMinute == 0 && iMaxspace == 0)
	{
		iUploadHour = 23;
		iUploadMinute = 0;
		iMaxspace = 90;
		iCycleTime = 6;
		LOG_RUNLOG_DEBUG("LG %d:%d:%d:%d", iMode, iUploadHour, iUploadMinute, iMaxspace);
	}
	if(g_stIdbtCfg.iLogUploadMode == 2)
	{
		g_stLogUploadTimer.pstTimer = YKCreateTimer(LOGUploadCallBack, NULL, g_stIdbtCfg.iLogUploadCycleTime*60*60*1000,
					YK_TT_PERIODIC, (unsigned long *)&g_stLogUploadTimer.ulMagicNum);
		if(NULL == g_stLogUploadTimer.pstTimer)
		{
			LOG_RUNLOG_ERROR("SM create log upload timer failed!");
		}
	}

	/*
	 *  1900 + pstTm->tm_year,
		1 + pstTm->tm_mon,
		pstTm->tm_mday,
		pstTm->tm_hour,
		pstTm->tm_min,
		pstTm->tm_sec
	 */
    while(g_iRunLGTaskFlag)
    {
#ifdef _YK_IMX27_AV_
    	NotifyWDThreadFromLOG();
#endif
    	sleep(5);

//add by chensq 20120731 BV增加Flash空间检測
#if (defined(_YK_XT8126_AV_)) || (defined(_YK_XT8126_BV_))
    	if(LOGCheckSpace2(LOG_FLASH_PATH, iMaxspace) != 0)
    	{
    		LOG_RUNLOG_INFO("LG check not enough disk space will upload all log (1)");
    		//空间滿则尝试上传
    		LOGUploadResPrepross();
#ifndef LP_ONLY
    		LOGUploadExec(FP_TRANSFER_NOWAIT);
#endif
    		//再检測,如果空间还滿,删除旧日志运行文件
    		if(LOGCheckSpace2(LOG_FLASH_PATH, iMaxspace) != 0)
			{
    			char rmBuff[128] = {0x00};
    			snprintf(rmBuff, sizeof(rmBuff), "busybox rm %s* -rf", LG_LOCAL_PATH);
    			LOG_RUNLOG_WARN("LG check not enough disk space will rm all log (%s) (2)", rmBuff);
    			system(rmBuff);
    			//system("mkdir " LG_LOCAL_PATH " -p");

    			//再检測,如果空间还滿,日志记录禁能
        		if(LOGCheckSpace2(LOG_FLASH_PATH, iMaxspace) != 0)
    			{
        			LOG_RUNLOG_WARN("LG check not enough disk space will unable logflag (3)");
        			g_stLogMng.iEnFlg = 0;
    			}
			}
    	}
#endif

    	//if(pstTm->tm_hour == UPLOAD_HOUR && pstTm->tm_min == UPLOAD_MIN)
    	//printf("debug:LOGTask upload param:[%d:%d maxspace:%d%]\n", log4c_get_upload_hour(), log4c_get_upload_minute(), log4c_get_upload_maxspace());

    	//modified by lcx
    	if(iMode == 1)
    	{
			time(&stTimep);
			pstTm = localtime(&stTimep);
			if(pstTm->tm_hour == iUploadHour && pstTm->tm_min == iUploadMinute)
			{

				LOG_RUNLOG_DEBUG("LG LOGTask checktime:[%d:%d:%d]", pstTm->tm_hour, pstTm->tm_min, iUploadFlg);
				iUploadFlg++;         //第一次时间符合指定小时与分钟才才上传日志
				if(iUploadFlg > 1)    //非第一次不上传日志
				{
					continue;
				}

				LOGUploadResPrepross();
#ifndef LP_ONLY
				LOGUploadExec(FP_TRANSFER_NOWAIT);
#endif
			}
			else
			{
				iUploadFlg = 0;
			}
    	}

    }
    //printf("debug:LOGTask end\n");
}


/**
 * @brief          创建日志配置文件log4crc
 *
 * @param[in]      无
 * @return         无
 */
void TMCreateLogCfg(void)
{
	FILE *fp;
	char acLog4cCfgPath[100] = { 0 };

	char *pclog4crc = "\
<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<!DOCTYPE log4c SYSTEM \"\">\n\
\n\
<log4c version=\"1.2.1\">\n\
\n\
	<config>\n\
		<bufsize>0</bufsize>\n\
		<debug level=\"2\"/>\n\
		<nocleanup>0</nocleanup>\n\
		<reread>1</reread>\n\
		<file_buffer>0</file_buffer>\n\
	</config>\n\
\n\
	<upload>\n\
		<mode>2</mode>\n\
		<hour>23</hour>\n\
		<minute>00</minute>\n\
		<maxspace>96</maxspace>\n\
		<cycle_time>5</cycle_time>\n\
	</upload>\n\
\n\
	<category name=\"root\" priority=\"trace\"/>\n\
\n\
	<category name=\"e_runlog\" priority=\"info\" appender=\"appender_runlog\" />\n\
	<category name=\"e_runlog.e_eventlog\" priority=\"info\" appender=\"appender_eventlog\" />\n\
	<category name=\"e_runlog.e_eventlog.e_screen\" priority=\"info\" appender=\"stdout\" />\n\
\n\
	<category name=\"r_runlog\" priority=\"info\" appender=\"appender_runlog\" />\n\
	<category name=\"r_runlog.r_screen\" priority=\"debug\" appender=\"stdout\" />\n\
\n\
	 <rollingpolicy name=\"rollingpolicy_eventlog\" type=\"datednamelog\" />\n\
	 <rollingpolicy name=\"rollingpolicy_runlog\" type=\"datednamelog\"  />\n\
\n\
	<appender name=\"appender_eventlog\" type=\"rollingfile\" logdir=\""LG_LOCAL_PATH_1"\" prefix=\"eventlog\"  layout=\"ext_eventlog\" rollingpolicy=\"rollingpolicy_eventlog\" />\n\
	<appender name=\"appender_runlog\" type=\"rollingfile\" logdir=\""LG_LOCAL_PATH_1"\" prefix=\"runlog\"  layout=\"ext_runlog\" rollingpolicy=\"rollingpolicy_runlog\"  />\n\
	<appender name=\"stdout\" type=\"stream\" layout=\"basic\"/>\n\
\n\
	<layout name=\"ext_eventlog\" type=\"ext_eventlog\"/>\n\
	<layout name=\"ext_runlog\" type=\"ext_runlog\"/>\n\
	<layout name=\"basic\" type=\"basic\"/>\n\
\n\
</log4c>\n\
";


	snprintf(acLog4cCfgPath, 100-1, "%s/log4crc", LOG4C_RCPATH);
	//log4crc生成
	fp = fopen(acLog4cCfgPath, "w+");
	fwrite(pclog4crc, sizeof(char), strlen(pclog4crc), fp);
	fclose(fp);
}


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
int LOGIntfInit(void)
{

	char acId[TERMINAL_ID_LEN+1] = {0x00};
	char acPath[128] = {0x00};


//	sprintf(acPath, "busybox mkdir %s -p", LG_LOCAL_PATH);
//	//printf("debug:LOGIntfInit %s\n", acPath);
//	system(acPath);
//
//	memset(acPath, 0x00, sizeof(acPath));
//	sprintf(acPath, "busybox mkdir %s", LG_LOCAL_PATH);
//	sprintf(acPath+strlen(acPath)-1, "%s", "Temp/ -p");
//	//printf("debug:LOGIntfInit %s\n", acPath);
//	system(acPath);
	system("busybox mkdir /data/data/com.fsti.ladder/LogDir");
	system("busybox mkdir /data/data/com.fsti.ladder/LogDir/Log");
	system("busybox mkdir /data/data/com.fsti.ladder/LogDir/LogTemp");


#ifdef LOG_DEBUG
	setenv("SD_DEBUG", "", 1);                                                                                 //log4c与xml相关信息调试开关控制
#endif

	setenv("LOG4C_RCPATH", LOG4C_RCPATH, 1);                                          //设置log4c使用的配置文件绝对路径

	TMCreateLogCfg();

#ifdef LP_ONLY
		setenv("SECOND_PREFIX",  "1234567890123456", 1);
#else
	//需要修改为从TM获得当前设备的设备ID
	TMQueryDeviceID(acId);
	if( strlen(acId) < 16 )
	{
		__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG", "LG LOGIntfInit TMQueryDeviceID id too short[%s]\n", acId);
		return -1;
	}

	else
	{
		__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG", "LG LOGIntfInit TMQueryDeviceID [%s]\n", acId);
		setenv("SECOND_PREFIX",  acId, 1);
	}
#endif

	LOGExtLayoutTypeInit();                                                                                     //初始化自定义布局
	LOGExtRollingPolicyTypeInit();                                                                          //初始化自定义文件滚动策略

	if (log4c_init())
	{

		__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG log4c_init() failed\n");
		return -1;
	}
	else
	{
		__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG log4c_init() success\n");

	}

	if ((g_stLogMng.pstEventLogger = log4c_category_get(EVENTLOG_CATEGORY_NAME)) == NULL)
	{
		__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG log4c_category_get failed[g_pstEventLogger]\n");
		return -1;
	}

	if ((g_stLogMng.pstRunLogger = log4c_category_get(RUNLOG_CATEGORY_NAME)) == NULL)
	{
		__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG log4c_category_get failed[g_pstRunLogger]\n");
		return -1;
	}

	pthread_mutex_init(&g_stLogMng.stMutex, NULL);

	g_stLogMng.iEnFlg = 1;
	g_iRunLGTaskFlag = TRUE;

	//创建日志管理任务
	g_LGThreadId = YKCreateThread(LOGTask, NULL);
	if(NULL == g_LGThreadId)
	{
		__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG YKCreateThread LOGTask FAILURE\n");
		return FAILURE;
	}
	__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG YKCreateThread LOGTask SUCCESS\n");
	return 0;
}

/**
 * @brief          日志模块资源清理
 *
 * @param[in]      无
 * @return         无
 */
void LOGIntfFini(void)
{
	__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG LOGIntfFini begin");
	g_stLogMng.iEnFlg = 0;
	LOGUploadResPrepross();
#ifndef LP_ONLY
	LOGUploadExec(FP_TRANSFER_WAIT);
#endif

	YKRemoveTimer(g_stLogUploadTimer.pstTimer, g_stLogUploadTimer.ulMagicNum);

	g_iRunLGTaskFlag = FALSE;
    if(NULL != g_LGThreadId)
    {
        YKJoinThread(g_LGThreadId);
        YKDestroyThread(g_LGThreadId);
    }
    __android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG LOGIntfFini YKJoinThread g_LGThreadId success\n");

	pthread_mutex_destroy(&(g_stLogMng.stMutex));

	if (log4c_fini())
	{
		__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG log4c_fini() failed\n");
	}
	__android_log_print(LOG4C_PRIORITY_DEBUG, "LOG",  "LG LOGIntfFini success\n");


}

////add by chensq 20120730
//void log_event( int iPriority, const char* pcFormat, ... )
//{
//	if( g_stLogMng.iEnFlg == 1 )
//	{
//		pthread_mutex_lock(&(g_stLogMng.stMutex));
//		const log4c_location_info_t locinfo = LOG4C_LOCATION_INFO_INITIALIZER(NULL);
//		va_list va;
//		va_start(va, pcFormat);
//		log4c_category_log_locinfo(g_stLogMng.pstEventLogger, &locinfo, iPriority, pcFormat, va);
//		va_end(va);
//		pthread_mutex_unlock(&(g_stLogMng.stMutex));
//	}
//}
////add by chensq 20120730
//void log_run( int iPriority, const char* pcFormat, ... )
//{
//	if( g_stLogMng.iEnFlg == 1 )
//	{
//		pthread_mutex_lock(&(g_stLogMng.stMutex));
//		log4c_location_info_t locinfo = LOG4C_LOCATION_INFO_INITIALIZER(NULL);
//		va_list va;
//		va_start(va, pcFormat);
//		log4c_category_log_locinfo(g_stLogMng.pstEventLogger, &locinfo, iPriority, pcFormat, va);
//		va_end(va);
//		pthread_mutex_unlock(&(g_stLogMng.stMutex));
//	}
//}
