// $Id: evcpe.c 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
/*
 * Copyright (C) 2010 AXIM Communications Inc.
 * Copyright (C) 2010 Cedric Shih <cedric.shih@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <signal.h>
//#include <mcheck.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <evcpe.h>
#include <evdns.h>

#ifndef XNMP_ENV
#include <IDBT.h>
#include <LOGIntfLayer.h>
#endif

TR_EVCPE_ST g_tr_evcpe_st;
extern int g_iRunSystemFlag;
extern struct TR_CALLBACK_ST evcpe_callback;

static void sig_handler(int signum);

static void error_cb(struct evcpe *cpe,
		enum evcpe_error_type type, int code, const char *reason, void *cbarg);

static int load_file(const char *filename, struct evbuffer *buffer);

int evcpe_shutdown(int code);

static void help(FILE *stream)
{
	fprintf(stream, "evcpe, version "EVCPE_VERSION"\n\n"
			"Usage: evcpe [options]\n\n"
			" Options are:\n"
			"  -m REPO_MODEL\tmodel of TR-069 repository\n"
			"  -d REPO_DATA\tdata of TR-069 repository\n"
			"  -v\t\tdisplay more logs\n"
			"  -s\t\tsuppress logs\n"
			"\n");
}

void error_cb(struct evcpe *cpe,
		enum evcpe_error_type type, int code, const char *reason, void *cbarg)
{
	LOG_RUNLOG_ERROR("TM type: %d, code: %d, reason: %s", type, code, reason);
	evcpe_shutdown(code);
}

void evcpe_update_sn()
{
	char acSn[128];
	TMNMGetSN(acSn);

	TMUpdateMac(acSn);
}

void *evcpe_startup(void *arg)
{
	int c, rc, bootstrap,dev_boot = 0;
	enum evcpe_log_level level;
	struct sigaction action;
	const char *repo_model, *repo_data;
	struct evbuffer *buffer = NULL;
	char acCommandLine[512]={0x0};
	char bootflag=0;

	char acDataName[128];
	char* pDataName;
	char **pcPath;

	repo_model = repo_data = NULL;
	pcPath = (char *)arg;
	repo_model = pcPath[0];
	repo_data = pcPath[1];

	if(strcmp(repo_data,TM_DATA_PATH) == 0)
	{
		dev_boot = 1;
	}
    
#ifdef XNMP_ENV
	level = EVCPE_LOG_INFO;
#else
	level = EVCPE_LOG_FATAL;
#endif
	/*需要模型和数据*/
	if (!repo_model || !repo_data) {
		help(stderr);
		rc = EINVAL;
		goto finally;
	}
	/*日志控制*/
	if (level <= EVCPE_LOG_FATAL)
		evcpe_add_logger("stderr", level, EVCPE_LOG_FATAL,
				NULL, evcpe_file_logger, stdout);

	evcpe_info(__func__, "initializing evcpe");

	//mtrace();
	/*分配并返回一个空的evbuffer*/
	if (!(buffer = evbuffer_new())) {
		LOG_RUNLOG_ERROR("TM failed to create evbuffer");
		rc = ENOMEM;
		goto finally;
	}

	bzero(&g_tr_evcpe_st, sizeof(TR_EVCPE_ST));
	/*加载model配置文件*/
	if ((rc = load_file(repo_model, buffer))) {
		LOG_RUNLOG_ERROR("TM failed to load model: %s", repo_model);
		goto finally;
	}
	/*初始化class数据结构*/
	if (!((g_tr_evcpe_st).cls = evcpe_class_new(NULL))) {
		LOG_RUNLOG_ERROR("TM failed to create evcpe_class");
		rc = ENOMEM;
		goto finally;
	}
	/*解析填充model文件中的内容并填充xml结构*/
	if ((rc = evcpe_class_from_xml((g_tr_evcpe_st).cls, buffer))) {
		LOG_RUNLOG_ERROR("TM failed to parse model: %s", repo_model);
		goto finally;
	}
	/*新建Model object*/
	if (!((g_tr_evcpe_st).obj = evcpe_obj_new((g_tr_evcpe_st).cls, NULL))) {
		LOG_RUNLOG_ERROR("TM failed to create root object");
		rc = ENOMEM;
		goto finally;
	}
	/*初始化并解析查找xml树生成object*/
	if ((rc = evcpe_obj_init((g_tr_evcpe_st).obj))) {
		LOG_RUNLOG_ERROR("TM failed to init root object");
		goto finally;
	}
	/*加载data配置文件*/
	if ((rc = load_file(repo_data, buffer))) {
		LOG_RUNLOG_ERROR("TM failed to load data: %s", repo_data);
		goto finally;
	}
	//--------------add by zlin
#ifndef XNMP_ENV
	/*重新加载data备份配置文件*/
	if (buffer->off <= 1000) {
		strcpy(acDataName,repo_data);
		pDataName = strtok(acDataName,".");
		sprintf(acCommandLine, "cp -rf %s_bak.xml %s\n", pDataName, repo_data);
		//sprintf(acCommandLine, "cp -rf %s %s\n", TM_DATA_BACKUP_PATH, TM_DATA_PATH);
		system(acCommandLine);
		bootflag = 1;
		if ((rc = load_file(repo_data, buffer))) {
			LOG_RUNLOG_ERROR("TM failed to load data: %s", repo_data);
			goto finally;
		}
	}
#endif
	//--------------
	/*查找tr098_data.xml 对应的xml树并生成data 中的object*/
	if ((rc = evcpe_obj_from_xml((g_tr_evcpe_st).obj, buffer))) {
		LOG_RUNLOG_ERROR("TM failed to parse data: %s", repo_data);
		goto finally;
	}
	/*新建repo结构，并将obj中得数据填充到repo结构*/
	if (!((g_tr_evcpe_st).repo = evcpe_repo_new((g_tr_evcpe_st).obj))) {
		LOG_RUNLOG_ERROR("TM failed to create repo");
		rc = ENOMEM;
		goto finally;
	}
	/*初始化libevent库*/
	if (((g_tr_evcpe_st).evbase = event_init()) == NULL)
	{
		LOG_RUNLOG_ERROR("TM failed to init event");
		rc = ENOMEM;
		goto finally;
	}
	/*分配持续内存*/
	if (!((g_tr_evcpe_st).persist = evcpe_persister_new((g_tr_evcpe_st).evbase))) {
		LOG_RUNLOG_ERROR("TM failed to create persister");
		rc = ENOMEM;
		goto finally;
	}
	/*结构设入内存*/
	if ((rc = evcpe_persister_set((g_tr_evcpe_st).persist, (g_tr_evcpe_st).repo, repo_data))) {
		LOG_RUNLOG_ERROR("TM failed to set persister");
		goto finally;
	}
	//------------------del by zlin
	/*初始化dns解析器*/
	/*if ((rc = evdns_init())) {
		LOG_RUNLOG_ERROR("failed to initialize DNS");
		goto finally;
	}*/
	//------------------
	/*新建并初始化cpe结构，为后续填充参数做准备*/
	if (((g_tr_evcpe_st).cpe = evcpe_new((g_tr_evcpe_st).evbase,
			NULL, error_cb, NULL)) == NULL) {
		LOG_RUNLOG_ERROR("TM failed to create evcpe");
		rc = ENOMEM;
		goto finally;
	}
	/*从repo中读取相关参数填充cpe结构*/
	if ((rc = evcpe_set((g_tr_evcpe_st).cpe, (g_tr_evcpe_st).repo))) {
		LOG_RUNLOG_ERROR("TM failed to set evcpe");
		goto finally;
	}
	//-----------------add by zlin
	/*读取bootstrap*/
	if ((rc = evcpe_boot((g_tr_evcpe_st).cpe, (g_tr_evcpe_st).repo, bootflag))) {
		LOG_RUNLOG_ERROR("TM failed to set evcpe");
		goto finally;
	}
	//-----------------
	evcpe_info(__func__, "configuring signal action");
	action.sa_handler = sig_handler;
	sigemptyset (&action.sa_mask);
	action.sa_flags = 0;
	if ((rc = sigaction(SIGINT, &action, NULL)) != 0) {
		LOG_RUNLOG_ERROR("TM failed to configure signal action");
		goto finally;
	}
	//------------------add by zlin
	/*读取版本号填充cpe结构*/
#ifndef TM_CONFIG_DOWNLOAD
	evcpe_info(__func__, "set version");
	if ((rc = TMSetParamVersion((g_tr_evcpe_st).cpe))) {
		LOG_RUNLOG_ERROR("TM failed to set version");
		goto finally;
	}
#endif
	/*删除事件*/
	evcpe_info(__func__, "delete evcpe event");
	if ((rc = evcpe_delete_event((g_tr_evcpe_st).cpe))) {
		LOG_RUNLOG_ERROR("TM failed to delete evcpe");
		goto finally;
	}
	//------------------
	if(dev_boot)
	{
		//设备启动了
		(g_tr_evcpe_st).cpe->deviceboot = 1;
	}

	/*启动evcpe,进入主流程*/
	//printf("i am here20!!!!!!!!!!!\n");
	evcpe_info(__func__, "starting evcpe");
	if ((rc = evcpe_start((g_tr_evcpe_st).cpe))) {
		LOG_RUNLOG_ERROR("TM failed to start evcpe");
		goto finally;
	}

	//序列号更新到idbt.cfg 并通知ladder
	evcpe_update_sn();

	//初始化完成，通知linphone注册
	if (evcpe_callback.nodifysip_cb)
				(*evcpe_callback.nodifysip_cb)();

	evcpe_info(__func__, "dispatching event base");
	//printf("i am here21!!!!!!!!!!!\n");
	/*循环接受事件处理，仅仅出错时才返回*/
	if ((rc = event_dispatch()) != 0) {
//		LOG_RUNLOG_ERROR("TM failed to dispatch event base");
		goto finally;
	}
//	LOG_RUNLOG_DEBUG("TM &&%s&&&&evcpe bye ",repo_data);
	if (buffer) evbuffer_free(buffer);
	return rc;
	//printf("i am here22!!!!!!!!!!!\n");
finally:
//    LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_SYSTEM_ERROR, 0, "108");
//	LOG_RUNLOG_DEBUG("TM &&%s&&&&evcpe bye ",repo_data);
	if (buffer) evbuffer_free(buffer);
	return rc;
}

int load_file(const char *filename, struct evbuffer *buffer)
{
	FILE *file;
	int rc, fd, len;

	if(!(file = fopen(filename, "r"))) {
		rc = errno;
		goto finally;
	}
	fd = fileno(file);
	evbuffer_drain(buffer, EVBUFFER_LENGTH(buffer));
	do {
		len = evbuffer_read(buffer, fd, -1);
	} while(len > 0);

	rc = len < 0 ? -1 : 0;
	fclose(file);

finally:
	return rc;
}

void sig_handler(int signal)
{
	evcpe_info(__func__, "signal caught: %d", signal);
	if (signal == SIGINT)
		evcpe_shutdown(0);
}

extern int g_bConfigUpdateFlag;
void evcpe_quit()
{
//	g_iRunSystemFlag = 0;
//	TMStopTerminateMange();
//	g_iRunTMTaskFlag = 0;
//	g_bConfigUpdateFlag = 0;
#if _TM_TYPE_== _TR069_
	IDBTMsgSendABReboot(CONFIG_REBOOT);
#else
	LOG_RUNLOG_DEBUG("\n!!!!!!!!!!evcpe_quit!!!!!!!!!!!\n\n");
	event_loopbreak();
	g_bConfigUpdateFlag = 0;
#endif
}

int evcpe_shutdown(int code)
{
	int flag = FALSE;
#ifndef XNMP_ENV
	flag = g_iUpdateConfigFlag;
#endif
	evcpe_info(__func__, "shuting down with code: %d (%s)", code, strerror(code));
	event_base_loopbreak(g_tr_evcpe_st.evbase);
	evcpe_free(g_tr_evcpe_st.cpe);
	evdns_shutdown(0);
	evcpe_persister_free(g_tr_evcpe_st.persist, flag);
	event_base_free(g_tr_evcpe_st.evbase);
	evcpe_repo_free(g_tr_evcpe_st.repo);
	evcpe_obj_free(g_tr_evcpe_st.obj);
	evcpe_class_free(g_tr_evcpe_st.cls);
	g_tr_evcpe_st.repo = NULL;
	//muntrace();
	//exit(code);
}

#if _TM_TYPE_ == _TR069_
void TMUpdateAlarmState(TM_ALARM_ID_EN enModule, const char* cState)
{
#ifndef XNMP_ENV
	if(!g_tr_evcpe_st.repo)
	{
		return;
	}
	switch(enModule)
	{
	case TM_ALARM_IMS_REGISTER:
		strncpy(g_tr_evcpe_st.cpe->alarm_state.ImsRegisterFail, cState, sizeof(g_tr_evcpe_st.cpe->alarm_state.ImsRegisterFail));
		if (TMCCIsAlarmEnabled(enModule)) {
			g_tr_evcpe_st.cpe->alarm_flag.ImsRegisterFail = 1;
		}
		break;
	case TM_ALARM_SIP_CALL:
		strncpy(g_tr_evcpe_st.cpe->alarm_state.SipCallFail, cState, sizeof(g_tr_evcpe_st.cpe->alarm_state.SipCallFail));
		if (TMCCIsAlarmEnabled(enModule)) {
			g_tr_evcpe_st.cpe->alarm_flag.SipCallFail = 1;
		}
		break;
	case TM_ALARM_IF_BOARD:
		strncpy(g_tr_evcpe_st.cpe->alarm_state.IfBoardComFail, cState, sizeof(g_tr_evcpe_st.cpe->alarm_state.IfBoardComFail));
		if (TMCCIsAlarmEnabled(enModule)) {
			g_tr_evcpe_st.cpe->alarm_flag.IfBoardComFail = 1;
		}
		break;
	case TM_ALARM_TRANSFER:
		strncpy(g_tr_evcpe_st.cpe->alarm_state.FileTransFail, cState, sizeof(g_tr_evcpe_st.cpe->alarm_state.FileTransFail));
		if (TMCCIsAlarmEnabled(enModule)) {
			g_tr_evcpe_st.cpe->alarm_flag.FileTransFail = 1;
		}
		break;
	case TM_ALARM_UPDATERESULT:
		strncpy(g_tr_evcpe_st.cpe->alarm_state.UpdateResult, cState, sizeof(g_tr_evcpe_st.cpe->alarm_state.UpdateResult));
		if (TMCCIsAlarmEnabled(enModule)) {
			g_tr_evcpe_st.cpe->alarm_flag.UpdateResult = 1;
		}
		break;
	case TM_ALARM_SOUTH_INTERFACE:
		strncpy(g_tr_evcpe_st.cpe->alarm_state.SouthInterface, cState, sizeof(g_tr_evcpe_st.cpe->alarm_state.SouthInterface));
		if (TMCCIsAlarmEnabled(enModule)) {
			g_tr_evcpe_st.cpe->alarm_flag.SouthInterface = 1;
		}
		break;
	case TM_ALARM_CALL_STATE:
		strncpy(g_tr_evcpe_st.cpe->alarm_state.SipCallState, cState, sizeof(g_tr_evcpe_st.cpe->alarm_state.SipCallState));
		g_tr_evcpe_st.cpe->alarm_flag.SipCallState = 1;
		break;
	default:
		break;
    }
#endif
}
#endif

void evcpe_event_change(enum evcpe_list_event code)
{
	switch(code)
	{
	case EVCPE_EVENT_LOGOUT:
		g_tr_evcpe_st.cpe->logout = 1;
		break;
	case EVCPE_EVENT_PARAMETER:
		g_tr_evcpe_st.cpe->parameter_change = 1;
		break;
	case EVCPE_EVENT_UPDATE:
		g_tr_evcpe_st.cpe->update_ask = 1;
		break;
	}
}

void evcpe_event_transfer(int result, struct tm *starttime, struct tm *completetime)
{
#ifdef _GYY_I5_
//	evcpe_repo_add_event(g_tr_evcpe_st.cpe->repo, "TRANSFERCOMPLETE", "");
#endif
	g_tr_evcpe_st.cpe->transfer.flag = 1;
	g_tr_evcpe_st.cpe->transfer.result = result;
	g_tr_evcpe_st.cpe->transfer.starttime = starttime;
	g_tr_evcpe_st.cpe->transfer.completetime = completetime;
}
