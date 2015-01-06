/**
*@file   LOGExtRollingPolicyType.c
*@brief ��չ���Ŀ�����ɲ���
*@author chensq
*@version 1.0
*@date 2012-4-23
*/


#ifdef HAVE_CONFIG_H
#include "log4c_config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include <log4c/appender.h>
#include <log4c/rollingpolicy.h>
#include <sd/malloc.h>
#include <sd/error.h>
#include <sd/sd_xplatform.h>
#include "appender_type_rollingfile.h"
#include "LOGExtRollingPolicyType.h"
#include "LOGIntfLayer.h"

//#define DNLC_SECOND_PREFIX_SIZE (17)                         //���峤��
#define TEMPDIR_POSTFIX                     "Temp"
#define FILE_BUFFER_MAX (4096)    //default max file buffer
#define FILE_BUFFER_DEAULT (1024)    //default file buffer
char g_acEventFileBuff[FILE_BUFFER_MAX] = {0x00};
char g_acRunFileBuff[FILE_BUFFER_MAX] = {0x00};

typedef struct
{
	char acDnlcSecondPrefix[TERMINAL_ID_LEN + 1];
}DATED_NAME_LOG_CONF_ST;

typedef struct
{
	DATED_NAME_LOG_CONF_ST stDnlConf;                         //�������Զ�������
	rollingfile_udata_t *pstDnlRfuData;
	const char *pcDnlLogdir;                                  //��־Ŀ¼
	const char *pcDnlFilesPrefix;                             //��־�ļ�ǰ׺
#define DNL_LAST_FOPEN_FAILED 0x0001
	int iDnlFlags;                                            //��־�ļ���ʧ�ܱ�־
}DATED_NAME_LOG_UDATA_ST;

static int LOGRollingPolicyDNLInit(log4c_rollingpolicy_t *pstThis, rollingfile_udata_t* pstRfup);
static int LOGRollingPolicyDNLRollover(log4c_rollingpolicy_t *pstThis, FILE **ppstCurrentFpp);
static int LOGRollingPolicyDNLIsTriggeringEvent(log4c_rollingpolicy_t *pstThis, const log4c_logging_event_t* pstEvent, long lCurrentFileSize);
static int LOGRollingPolicyDNLOpenZeroFile(char *pcFileName, FILE **ppstFpp);


/**
 * @brief        �������Դ��������ж�(��δʹ�ã�����δ����չ)
 *
 * @param[in]    this:log4c_rollingpolicy_t *�������Զ���
 * @param[in]    a_event:log4c_logging_event_t*��־�¼�����
 * @param[in]    current_file_size:��ǰ�ļ���С
 * @return
 */
static int LOGRollingPolicyDNLIsTriggeringEvent(log4c_rollingpolicy_t *pstThis,
		const log4c_logging_event_t* pstEvent, long lCurrentFileSize)
{
	int decision = 0;
	sd_debug("datednamelog_is_triggering_event[");
	sd_debug("not triggering event");
	sd_debug("]");
	return decision;
}


/**
 * @brief        ��������ִ��
 *
 * @param[in]    this:log4c_rollingpolicy_t *�������Զ���
 * @param[in]    current_fpp:FILE **�ļ�ָ��
 * @return
 */
static int LOGRollingPolicyDNLRollover(log4c_rollingpolicy_t *pstThis, FILE ** ppstCurrentFpp)
{
	int rc = 0;
	time_t stTimep;
	struct tm *pstTm;
	char acFileName[256] = { 0x00 };
	char acCmdBuff[512] = { 0x00 };
	int iFileLen = 0;

	DATED_NAME_LOG_UDATA_ST *pstDnlup = log4c_rollingpolicy_get_udata(pstThis);

	sd_debug("datednamelog_rollover[");

	//�ϴ��YԴ׼�����رյ�ǰ����־
	//���ļ��Ѵ򿪣��ر���
	if (!(pstDnlup->iDnlFlags & DNL_LAST_FOPEN_FAILED) && *ppstCurrentFpp)
	{
		fflush(*ppstCurrentFpp);
		if (fclose(*ppstCurrentFpp))
		{
			sd_error("failed to close current log file");
			rc = 1;
		}
	}
	else
	{
		if ((pstDnlup->iDnlFlags & DNL_LAST_FOPEN_FAILED))
		{
			sd_debug("Not closing log file...last open failed");
		}
		else
		{
			sd_debug("Not closing current log file...not sure why");
		}
	}

	//pthread_mutex_lock(&(g_stLogMng.stMutex));
	//�ƶ���־�����ϴ�Ŀ¼(��־Ŀ¼log��ftp�ϴ������أ���־Ŀ¼log_temp�ǹ������¼��־����ʱĿ¼)
	snprintf(acCmdBuff, sizeof(acCmdBuff), "busybox mv %s%s/%s*  %s/ ",
			pstDnlup->pcDnlLogdir, TEMPDIR_POSTFIX, pstDnlup->pcDnlFilesPrefix, pstDnlup->pcDnlLogdir);
	sd_debug(acCmdBuff);
	printf("\nLG %s\n", acCmdBuff);
#if defined(_YK_XT8126_AV_) || defined(_YK_XT8126_DOOR_)
	ClearWatchFlag();			//����ʱ��Ϊ1.6�룬��ֹ����
#endif
	system(acCmdBuff);
#if defined(_YK_XT8126_AV_) || defined(_YK_XT8126_DOOR_)
	ClearWatchFlag();			//����ʱ��Ϊ1.6�룬��ֹ����
#endif

	//pthread_mutex_unlock(&(g_stLogMng.stMutex));

	//�½�����־

	time(&stTimep);
	pstTm = localtime(&stTimep);

	//���´����ʽ���Ż� /xxxxxxxx/log+(_temp)+1(/)+prefix+ 1(_) + second(xxx) + 1(_) + (2012-05-23-19-30-22[19].[1]log[3])
	iFileLen = strlen(pstDnlup->pcDnlLogdir) + 5 + 1
			+ strlen(pstDnlup->pcDnlFilesPrefix) + 1
			+ strlen(pstDnlup->stDnlConf.acDnlcSecondPrefix) + 1 + 19 + 1 + 3;

	if (iFileLen > sizeof(acFileName))
	{
		printf("LG error:filename buffer size too small:size=%d/%d\n", iFileLen, sizeof(acFileName));
		return 1;
	}

	snprintf(acFileName, sizeof(acFileName),
			"%s%s/%s_%s_%04d-%02d-%02d-%02d-%02d-%02d.log",
			pstDnlup->pcDnlLogdir, TEMPDIR_POSTFIX, pstDnlup->pcDnlFilesPrefix,
			pstDnlup->stDnlConf.acDnlcSecondPrefix, 1900 + pstTm->tm_year,
			1 + pstTm->tm_mon, pstTm->tm_mday, pstTm->tm_hour, pstTm->tm_min,
			pstTm->tm_sec);
	sd_debug("filename:%s", acFileName);

	if (LOGRollingPolicyDNLOpenZeroFile(acFileName, ppstCurrentFpp))
	{
		pstDnlup->iDnlFlags |= DNL_LAST_FOPEN_FAILED;
		sd_error("open zero file failed");
		rc = 1;
	}
	else
	{
		pstDnlup->iDnlFlags &= !DNL_LAST_FOPEN_FAILED;
		rc = 0;
	}

	sd_debug("current file descriptor '%d'", fileno(*ppstCurrentFpp));

	sd_debug("]");

	return (rc);
}


/**
 *@brief         ����udata
 *@param[in]     ��
 *@return        DATED_NAME_LOG_UDATA_ST *
 */
LOG4C_API DATED_NAME_LOG_UDATA_ST *LOGRollingPolicyDNLMakeUdata(void)
{
	DATED_NAME_LOG_UDATA_ST *pstDnlup = NULL;
	pstDnlup = (DATED_NAME_LOG_UDATA_ST *) sd_calloc(1, sizeof(DATED_NAME_LOG_UDATA_ST));
	LOGRollingPolicyDNLUdataSetSecondPrefix(pstDnlup, getenv("SECOND_PREFIX") ? getenv("SECOND_PREFIX") : SECOND_PREFIX);
	return (pstDnlup);

}

/**
 *@brief         �������Ե���Դ��ʼ��
 *@param[in]     this:log4c_rollingpolicy_t *���Զ���
 *@param[in]     rfup:rollingfile_udata_t *
 *@return        0
 */
static int LOGRollingPolicyDNLInit(log4c_rollingpolicy_t *pstThis, rollingfile_udata_t *pstRfup)
{
	DATED_NAME_LOG_UDATA_ST *pstDnlup = NULL;

	sd_debug("datednamelog_init[");
	if (!pstThis)
	{
		goto datednamelog_init_exit;
	}

	pstDnlup = log4c_rollingpolicy_get_udata(pstThis);
	if (pstDnlup == NULL)
	{
		pstDnlup = LOGRollingPolicyDNLMakeUdata();
		log4c_rollingpolicy_set_udata(pstThis, pstDnlup);
	}

	pstDnlup->pcDnlLogdir = rollingfile_udata_get_logdir(pstRfup);
	pstDnlup->pcDnlFilesPrefix = rollingfile_udata_get_files_prefix(pstRfup);

	datednamelog_init_exit: sd_debug("]");

	return (0);
}

/**
 *@brief         �������Ե���Դ����
 *@param[in]     this:log4c_rollingpolicy_t *���Զ���
 *@return        0
 */
static int LOGRollingPolicyDNLFini(log4c_rollingpolicy_t *this)
{
	DATED_NAME_LOG_UDATA_ST *pstDnlup = NULL;
	int rc = 0;

	sd_debug("datednamelog_fini[ ");
	if (!this)
	{
		goto datednamelog_fini_exit;
	}

	pstDnlup = log4c_rollingpolicy_get_udata(this);
	if (!pstDnlup)
	{
		goto datednamelog_fini_exit;
	}

	sd_debug("freeing datednamelog udata from rollingpolicy instance");
	free(pstDnlup);
	log4c_rollingpolicy_set_udata(this, NULL);

	datednamelog_fini_exit: sd_debug("]");

	return (rc);
}

/**
 *@brief         ���ù������Ե��ļ������ɹ���֮һ
 *@param[in]     ��
 *@return        rollingpolicy_datednamelog_udata_t *
 */
LOG4C_API int LOGRollingPolicyDNLUdataSetSecondPrefix(DATED_NAME_LOG_UDATA_ST *pstDnlup, const char *pcSecondPrefix)
{
	snprintf(pstDnlup->stDnlConf.acDnlcSecondPrefix, sizeof(pstDnlup->stDnlConf.acDnlcSecondPrefix), "%s", pcSecondPrefix);
	return (0);
}



/**
 *@brief            ���ò��Զ����udata
 *@param[in]
 *@param[in]
 *@return           0
 */
LOG4C_API int LOGRollingPolicyDNLUdataSetRfudata(DATED_NAME_LOG_UDATA_ST *pstDnlu, rollingfile_udata_t *pstRfu)
{
	pstDnlu->pstDnlRfuData = pstRfu;
	return (0);
}

/**
 *@brief         ���������ļ�
 *@param[in]     filename:�ļ���
 *@param[in]     fpp:�ļ�ָ��
 *@return        0
 */
static int LOGRollingPolicyDNLOpenZeroFile(char *pcFileName, FILE **ppstFpp)
{
	int rc = 0;
	sd_debug("datednamelog_open_zero_file['%s'", pcFileName);

	if ((*ppstFpp = fopen(pcFileName, "w+")) == NULL)
	{
		sd_error(
				"failed to open zero file '%s'--defaulting to stderr--error='%s'",
				pcFileName, strerror(errno));
		*ppstFpp = stderr;
		rc = 1;
	}

	if(*ppstFpp != stderr)
	{
		int iFileBuffer = log4c_get_file_buffer();
		if(iFileBuffer > FILE_BUFFER_MAX || iFileBuffer == 0)
		{
			setbuf(*ppstFpp, NULL);
			printf("LG log file buffer size:%d\n", 0);
		}
		else if(iFileBuffer < 0)
		{
			if(strstr(pcFileName, "runlog") != NULL)
			{
				setvbuf(*ppstFpp, g_acRunFileBuff, _IOFBF, FILE_BUFFER_DEAULT);
				printf("LG runlog file buffer deault size:%d\n", FILE_BUFFER_DEAULT);
			}
			else if(strstr(pcFileName, "eventlog") != NULL)
			{
				setvbuf(*ppstFpp, g_acEventFileBuff, _IOFBF, FILE_BUFFER_DEAULT);
				printf("LG eventlog file buffer deault size:%d\n", FILE_BUFFER_DEAULT);
			}
		}
		else
		{
			if(strstr(pcFileName, "runlog") != NULL)
			{
				setvbuf(*ppstFpp, g_acRunFileBuff, _IOFBF, iFileBuffer);
				printf("LG runlog file buffer size:%d\n", iFileBuffer);
			}
			else if(strstr(pcFileName, "eventlog") != NULL)
			{
				setvbuf(*ppstFpp, g_acEventFileBuff, _IOFBF, iFileBuffer);
				printf("LG eventlog file buffer size:%d\n", iFileBuffer);
			}

		}
	}
	else
	{
		setbuf(*ppstFpp, NULL);
	}


	sd_debug("]");
	return (rc);
}


//�������ɲ��Զ�����
const log4c_rollingpolicy_type_t stLog4cRollingPolicyExtTypeDatednamelog =
{
		"datednamelog",
		LOGRollingPolicyDNLInit,
		LOGRollingPolicyDNLIsTriggeringEvent,
		LOGRollingPolicyDNLRollover,
		LOGRollingPolicyDNLFini
};

//������չrollingpolicy�б�
static const log4c_rollingpolicy_type_t * const apstExtRollingPolicyTypes[] =
{
		&stLog4cRollingPolicyExtTypeDatednamelog
};

static int iRollingpolicy_types = (int) (sizeof(apstExtRollingPolicyTypes) / sizeof(apstExtRollingPolicyTypes[0]));

/**
 *@brief  ��ʼ����չrollingpolicy
 *@param  ��
 *@return 0
 */
int LOGExtRollingPolicyTypeInit(void)
{

	int rc = 0;
	int i = 0;

	for (i = 0; i < iRollingpolicy_types; i++)
	{
//		printf("----------------nRollingpolicy_types:%d(set layout:%s)\n", nRollingpolicy_types, ext_rollingpolicy_types[i]->name);
		log4c_rollingpolicy_type_set(apstExtRollingPolicyTypes[i]);
	}

	return (rc);

}
