/**
*@file   LOGExtLayoutTypes.c
*@brief 扩展输出布局类型
*@author chensq
*@version 1.0
*@date 2012-4-23
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <log4c.h>
#include "LOGIntfLayer.h"
#include "LOGExtLayoutTypes.h"

/**
 *@brief              实现"eventlog_format"的扩展layout的格式化
 *@param[in]
 *@return             格式化后的缓存地址
 */
static const char* LOGEventlogFormat(const log4c_layout_t* pstLayout,
		const log4c_logging_event_t*pstEvent)
{
	static char buffer[1024];
	time_t stTimep;
	struct tm *pstTm;

	if ( strncmp(pstEvent->evt_msg, LOG_CMD_DIVIDING_LENES, strlen(LOG_CMD_DIVIDING_LENES))  == 0 )
	{
		//事件日志分割行处理
		sd_debug("%s", "cmd LOG_CMD_DIVIDING_LENES");
		snprintf(buffer, sizeof(buffer), "%s\n", "          ");
	}
	else
	{
		time(&stTimep);
		pstTm = localtime(&stTimep);

		snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d %s\n",
                                                                        1900 + pstTm->tm_year,
                                                                        1 + pstTm->tm_mon,
                                                                        pstTm->tm_mday,
                                                                        pstTm->tm_hour,
                                                                        pstTm->tm_min,
                                                                        pstTm->tm_sec,
                                                                        pstEvent->evt_msg);
	}
	return buffer;
}

//定义扩展layout:log4c_layout_ext_type_eventlog
const log4c_layout_type_t stLog4cLayoutExtTypeForEventLog =
{
		"ext_eventlog",
		LOGEventlogFormat
};

/**
 *@brief           实现none_format的扩展layout的格式化
 *@param[in]       无
 *@return          格式化后的缓存地址
 */
static const char* LOGRunLogFormat(const log4c_layout_t* pstLayout,
		const log4c_logging_event_t*pstEvent)
{
	static char buffer[4096];
	time_t stTimep;
	struct tm *pstTm;


	if (strncmp(pstEvent->evt_msg, LOG_CMD_DIVIDING_LENES,
			strlen(LOG_CMD_DIVIDING_LENES)) == 0)
	{
		sd_debug("%s", "cmd LOG_CMD_DIVIDING_LENES");
		snprintf(buffer, sizeof(buffer), "%s\n", "          ");
	}
	else
	{
		//运行日志格式
		time(&stTimep);
		pstTm = localtime(&stTimep);
//		snprintf(buffer, sizeof(buffer), "[%04d-%02d-%02d %02d:%02d:%02d][%-8s][%s][%s:%s:%d]\n",
//                                                                          1900 + pstTm->tm_year,
//                                                                          1 + pstTm->tm_mon,
//                                                                          pstTm->tm_mday,
//                                                                          pstTm->tm_hour,
//                                                                          pstTm->tm_min,
//                                                                          pstTm->tm_sec,
//                                                                          log4c_priority_to_string(pstEvent->evt_priority),
//                                                                          pstEvent->evt_msg,
//                                                                          pstEvent->evt_loc->loc_file,
//                                                                          pstEvent->evt_loc->loc_function,
//                                                                          pstEvent->evt_loc->loc_line);

		snprintf(buffer, sizeof(buffer), "[%04d-%02d-%02d %02d:%02d:%02d][%-8s][%s][%s:%d]\n",
                                                                          1900 + pstTm->tm_year,
                                                                          1 + pstTm->tm_mon,
                                                                          pstTm->tm_mday,
                                                                          pstTm->tm_hour,
                                                                          pstTm->tm_min,
                                                                          pstTm->tm_sec,
                                                                          log4c_priority_to_string(pstEvent->evt_priority),
                                                                          pstEvent->evt_msg,
                                                                          pstEvent->evt_loc->loc_function,
                                                                          pstEvent->evt_loc->loc_line);

	}

	return buffer;
}

//定义扩展layout
const log4c_layout_type_t stLog4cLayoutExtTypeRunlog =
{
		"ext_runlog",
		LOGRunLogFormat,
};

//定义扩展layout列表
static const log4c_layout_type_t * const apstExtLayoutTypes[] =
{
		&stLog4cLayoutExtTypeForEventLog,
		&stLog4cLayoutExtTypeRunlog
};

static int ilayout_types = (int) (sizeof(apstExtLayoutTypes) / sizeof(apstExtLayoutTypes[0]));


/**
 *@brief         初始化扩展layout
 *@param[in]     无
 *@return        0
 */
int LOGExtLayoutTypeInit(void)
{

	int rc = 0;
	int i = 0;

	for (i = 0; i < ilayout_types; i++)
	{
//		printf("----------------nlayout_types:%d(set layout:%s)\n", nlayout_types, ext_layout_types[i]->name);
		log4c_layout_type_set(apstExtLayoutTypes[i]);
	}
	return (rc);
}

