// $Id: cpe.c 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
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

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <evdns.h>
#include <IDBT.h>

#include "log.h"
#include "util.h"
#include "msg.h"
#include "data.h"
#include "fault.h"
#include "inform.h"
#include "get_rpc_methods.h"
#include "get_param_names.h"
#include "get_param_values.h"
#include "get_param_attrs.h"
#include "add_object.h"
#include "set_param_values.h"
//------------------add by zlin
#include "delete_object.h"
#include "download.h"
#include "upload.h"
#include "reboot.h"
#include "transfercomplete.h"
//-------------------

#include "cpe.h"
#include "LOGIntfLayer.h"


#define EVCPE_USERINFO_STR   "UserInfo"
#define EVCPE_CARDINFO_STR	 "CardInfo"
#define EVCPE_PHONEINFO_STR   "PhoneInfo"

extern int g_bUpdateFlag;
extern char g_acTRFirmVersion[64];
int g_bConfigUpdateFlag = 0;

EVCPE_TEMP_ST g_stTemp ;
struct TR_CALLBACK_ST evcpe_callback;//add by zlin
static char g_acCRC[10];

static void evcpe_send_error(struct evcpe *cpe, enum evcpe_error_type type,
		int code, const char *reason);

static void evcpe_creq_cb(struct evhttp_request *req, void *arg);

static void evcpe_session_message_cb(struct evcpe_session *session,
		enum evcpe_msg_type type, enum evcpe_method_type method_type,
				void *request, void *response, void *arg);

static void evcpe_session_terminate_cb(struct evcpe_session *session,
		int code, void *arg);

static void evcpe_dns_cb(int result, char type, int count, int ttl,
	    void *addresses, void *arg);

static inline void evcpe_dns_timer_cb(int fd, short event, void *arg);

static int evcpe_dns_entry_resolve(struct evcpe_dns_entry *entry,
		const char *hostname);

static int evcpe_dns_add(struct evcpe *cpe, const char *hostname);

static inline int evcpe_handle_request(struct evcpe *cpe,
		struct evcpe_session *session,
		enum evcpe_method_type method_type, void *request);

static inline int evcpe_handle_response(struct evcpe *cpe,
		enum evcpe_method_type method_type, void *request, void *response);

static inline int evcpe_handle_get_rpc_methods(struct evcpe *cpe,
		struct evcpe_get_rpc_methods *req, struct evcpe_msg *msg);

static inline int evcpe_handle_get_param_names(struct evcpe *cpe,
		struct evcpe_get_param_names *req,
		struct evcpe_msg *msg);

static inline int evcpe_handle_get_param_values(struct evcpe *cpe,
		struct evcpe_get_param_values *req,
		struct evcpe_msg *msg);

static inline int evcpe_handle_get_param_attrs(struct evcpe *cpe,
		struct evcpe_get_param_attrs *req,
		struct evcpe_msg *msg);

static inline int evcpe_handle_add_object(struct evcpe *cpe,
		struct evcpe_add_object *req,
		struct evcpe_msg *msg);

//--------------------add by zlin
static inline int evcpe_handle_delete_object(struct evcpe *cpe,
		struct evcpe_delete_object *req,
		struct evcpe_msg *msg);
//---------------------

static inline int evcpe_handle_set_param_values(struct evcpe *cpe,
		struct evcpe_set_param_values *req,
		struct evcpe_msg *msg);

//---------------------add by zlin
static inline int evcpe_handle_transfercomplete(struct evcpe *cpe,
		struct evcpe_session *session);
    
static inline int evcpe_handle_upload(struct evcpe *cpe,
		struct evcpe_upload *req,
		struct evcpe_msg *msg);

static inline int evcpe_handle_download(struct evcpe *cpe,
		struct evcpe_download *req,
		struct evcpe_msg *msg);

static inline int evcpe_handle_reboot(struct evcpe *cpe,
		struct evcpe_reboot *req,
		struct evcpe_msg *msg);
//----------------------

static inline int evcpe_handle_inform_response(struct evcpe *cpe,
		struct evcpe_inform *req, struct evcpe_inform_response *resp);

static int evcpe_retry_session(struct evcpe *cpe);

static void evcpe_start_session_cb(int fd, short event, void *arg);

static int evcpe_start_session(struct evcpe *cpe);

//----------------------add by zlin
void evcpe_callback_void_cb(evcpe_void_cb reboot_cb);

void evcpe_callback_load_cb(evcpe_load_cb load_cb);

//----------------------add by hb
void evcpe_callback_config_cb(evcpe_config_cb config_cb);
//----------------------

//----------------------add by hb
void evcpe_index_save()
{
	FILE *hCfgFd;
 	hCfgFd=fopen(TR_INDEX_TMP,"w+");
	if(hCfgFd == NULL)
	{
		return;
	}

	fprintf(hCfgFd,"IndexNum=%d\n",g_stTemp.iIndex);
	fprintf(hCfgFd,"ICIDOpenLog=%s\n",g_stTemp.acICIDLog);

	fclose(hCfgFd);
}

void evcpe_index_load()
{
	FILE *hCfgFd;
	//int iIndex = 0;
	memset(&g_stTemp,0,sizeof(EVCPE_TEMP_ST));
	hCfgFd=fopen(TR_INDEX_TMP,"r");
	if(hCfgFd == NULL)
	{
		return;
	}

	//获取类型
	//索引号
	fscanf(hCfgFd,"IndexNum=%d\n",&g_stTemp.iIndex);
	//刷卡记录
	fscanf(hCfgFd,"ICIDOpenLog=%s\n",g_stTemp.acICIDLog);

	fclose(hCfgFd);

}

void evcpe_index_del()
{
	char buf[128];
	sprintf(buf,"busybox rm -rf %s",TR_INDEX_TMP);
	system(buf);
}

unsigned short TMAlgoCRC(const unsigned char* pData, unsigned long ulSize, unsigned short usInitValue)
{
	unsigned long i = 0;
	unsigned short usCrc = usInitValue;
	while(ulSize-- != 0)
	{
		for(i = 0x80; i != 0; i/=2)
		{
			if((usCrc & 0x8000) != 0)
			{
				usCrc *= 2;
				usCrc ^= 0x1021; //CRC
			}else{
				usCrc *= 2;
			}
			if((*pData & i) != 0)
			{
				usCrc ^= 0x1021; //CRC
			}
		}
		pData++;
	}
	return usCrc;
}

BOOL TMCheckCRC(char* pcFileName,char* pcCRCFile)
{
	unsigned int size = 0;
	unsigned char* buffer=NULL;
	char value[512] = {0x0};
	const char* pcFileCrc=NULL;
	char command[512] = {0x0};
	FILE* pf = fopen(pcFileName, "rb");
	if(NULL != pf)
	{

		fseek(pf, 0, SEEK_END);
		size = ftell(pf);
		fseek(pf, 0, SEEK_SET);
		buffer = (unsigned char*)malloc(size);
		if(size != fread(buffer, sizeof(char), size, pf))
		{
			fclose(pf);
			free(buffer);
			return FALSE;
		}

		pcFileCrc = strrchr(pcCRCFile, '.');
		sprintf(value, "%d", TMAlgoCRC(buffer, size, 0));
		free(buffer);
		fclose(pf);

		if(strcmp(pcFileCrc + 1, value) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL TMGetCRC(char* pcCRC)
{
    unsigned int size = 0;
    unsigned char* buffer=NULL;
    char value[512] = {0x0};
    const char* imagecrc=NULL;
    char command[512] = {0x0};
    //需要添加访客机判断
    sprintf(command,"busybox cp -rf %s /data/data/com.fsti.ladder/TMCRC.xml",TM_DATA_PATH);
    system(command);
//#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
//    system("busybox bunzip2 TMData.xml.bz2");
//#endif
    FILE* pf = fopen("/data/data/com.fsti.ladder/TMCRC.xml", "rb");
    if(NULL != pf)
    {
        fseek(pf, 0, SEEK_END);
        size = ftell(pf);
        fseek(pf, 0, SEEK_SET);
        buffer = (unsigned char*)malloc(size);
        if(size != fread(buffer, sizeof(char), size, pf))
        {
            fclose(pf);
            free(buffer);
            return FALSE;
        }
    }

    sprintf(pcCRC, "%u",TMAlgoCRC(buffer, size, 0));
    return TRUE;
}
//----------------------add by hb

struct evcpe *evcpe_new(struct event_base *evbase,
		evcpe_request_cb cb, evcpe_error_cb error_cb, void *cbarg)
{
	struct evcpe *cpe;

	evcpe_debug(__func__, "constructing evcpe");

	if ((cpe = calloc(1, sizeof(struct evcpe))) == NULL) {
		evcpe_error(__func__, "failed to calloc evcpe");
		return NULL;
	}
	RB_INIT(&cpe->dns_cache);
	cpe->evbase = evbase;
	cpe->cb = cb;
	cpe->error_cb = error_cb;
	cpe->cbarg = cbarg;

	return cpe;
}

void evcpe_free(struct evcpe *cpe)
{
	if (cpe == NULL) return;

	evcpe_debug(__func__, "destructing evcpe");

	if (event_initialized(&cpe->retry_ev) &&
			evtimer_pending(&cpe->retry_ev, NULL)) {
		event_del(&cpe->retry_ev);
	}
	if (event_initialized(&cpe->periodic_ev) &&
			evtimer_pending(&cpe->periodic_ev, NULL)) {
		event_del(&cpe->periodic_ev);
	}
	if (cpe->session) evcpe_session_free(cpe->session);
	if (cpe->http) evhttp_free(cpe->http);
	if (cpe->acs_url) evcpe_url_free(cpe->acs_url);
	if (cpe->proxy_url) evcpe_url_free(cpe->proxy_url);
	if (cpe->creq_url) evcpe_url_free(cpe->creq_url);

	evcpe_dns_cache_clear(&cpe->dns_cache);
	free(cpe);
}

int evcpe_set(struct evcpe *cpe,
		struct evcpe_repo *repo)
{
	int rc, number;
	const char *value;
	unsigned int len;

	if ((rc = evcpe_repo_get(repo,
			".ManagementServer.Authentication", &value, &len))) {
		evcpe_error(__func__, "failed to get ACS authentication");
		goto finally;
	}
	if (!strcmp("NONE", value))
		cpe->acs_auth = EVCPE_AUTH_NONE;
	else if (!strcmp("BASIC", value))
		cpe->acs_auth = EVCPE_AUTH_NONE;
	else if (!strcmp("DIGEST", value))
		cpe->acs_auth = EVCPE_AUTH_NONE;
	else {
		evcpe_error(__func__, "invalid authentication value: %s", value);
		rc = EINVAL;
		goto finally;
	}

	if ((rc = evcpe_repo_get(repo,
			".ManagementServer.URL", &value, &len))) {
		evcpe_error(__func__, "failed to get ACS URL");
		goto finally;
	}
	if (!(cpe->acs_url = evcpe_url_new())) {
		evcpe_error(__func__, "failed to create evcpe_url");
		rc = ENOMEM;
		goto finally;
	}
	if ((rc = evcpe_url_from_str(cpe->acs_url, value))) {
		evcpe_error(__func__, "failed to parse ACS URL: %s", value);
		goto finally;
	}
	if ((rc = evcpe_dns_add(cpe, cpe->acs_url->host))) {
		evcpe_error(__func__, "failed to resolve ACS hostname");
		goto finally;
	}
	//--------------------del by zlin
	//cpe->id.device_no= evcpe_repo_find(repo,
	//		".DeviceInfo.DeviceNo");
	//---------------------
    
	cpe->acs_username = evcpe_repo_find(repo,
			".ManagementServer.Username");
	cpe->acs_password = evcpe_repo_find(repo,
			".ManagementServer.Password");
	if ((value = evcpe_repo_find(repo,
			".ManagementServer.Timeout"))) {
		if ((number = atoi(value)) < 0) {
			evcpe_error(__func__, "invalid ACS timeout: %d", number);
			rc = EINVAL;
			goto finally;
		}
		cpe->acs_timeout = number;
	} else {
		cpe->acs_timeout = EVCPE_ACS_TIMEOUT;
	}

	if ((value = evcpe_repo_find(repo,
			".ManagementServer.ProxyURL"))) {
		if (!(cpe->proxy_url = evcpe_url_new())) {
			evcpe_error(__func__, "failed to create evcpe_url");
			rc = ENOMEM;
			goto finally;
		}
		if ((rc = evcpe_url_from_str(cpe->proxy_url, value))) {
			evcpe_error(__func__, "failed to parse proxy URL: %s", value);
			goto finally;
		}
		if ((rc = evcpe_dns_add(cpe, cpe->proxy_url->host))) {
			evcpe_error(__func__, "failed to resolve HTTP proxy hostname");
			goto finally;
		}
		cpe->proxy_username = evcpe_repo_find(repo,
				".ManagementServer.ProxyUsername");
		cpe->proxy_password = evcpe_repo_find(repo,
				".ManagementServer.ProxyPassword");
	}

	if ((value = evcpe_repo_find(repo,
			".ManagementServer.ConnectionRequestURL"))) {
		if (!(cpe->creq_url = evcpe_url_new())) {
			evcpe_error(__func__, "failed to create evcpe_url");
			rc = ENOMEM;
			goto finally;
		}
		if ((rc = evcpe_url_from_str(cpe->creq_url, value))) {
			evcpe_error(__func__, "failed to parse ACS URL");
			goto finally;
		}
		cpe->creq_username = evcpe_repo_find(repo,
				".ManagementServer.ConnectionRequestUsername");
		cpe->creq_password = evcpe_repo_find(repo,
				".ManagementServer.ConnectionRequestPassword");
		if ((value = evcpe_repo_find(repo,
				".ManagementServer.ConnectionRequestInterval"))) {
			if ((number = atoi(value)) < 0) {
				evcpe_error(__func__, "invalid connection request interval: %d",
						number);
				rc = EINVAL;
				goto finally;
			}
			cpe->creq_interval = number;
		} else {
			cpe->creq_interval = EVCPE_CREQ_INTERVAL;
		}
	}

	if ((value = evcpe_repo_find(repo,
			".ManagementServer.PeriodicInformEnable")) &&
			!strcmp("1", value)) {
		if (!(value = evcpe_repo_find(repo,
				".ManagementServer.PeriodicInformInterval"))) {
			evcpe_error(__func__, "periodic inform interval was not set");
			rc = EINVAL;
			goto finally;
		}
		if ((number = atoi(value)) < 0) {
			evcpe_error(__func__, "invalid periodic inform interval: %d",
					number);
			rc = EINVAL;
			goto finally;
		}
		evtimer_set(&cpe->periodic_ev, evcpe_start_session_cb, cpe);
		if ((rc = event_base_set(cpe->evbase, &cpe->periodic_ev))) {
			evcpe_error(__func__, "failed to set event base");
			goto finally;
		}
		evutil_timerclear(&cpe->periodic_tv);
		cpe->periodic_tv.tv_sec = number;
		evcpe_info(__func__, "scheduling periodic inform in %ld second(s)",
				cpe->periodic_tv.tv_sec);
		if ((rc = event_add(&cpe->periodic_ev, &cpe->periodic_tv))) {
			evcpe_error(__func__, "failed to schedule periodic inform");
			goto finally;
		}
	}
	cpe->repo = repo;

finally:
	return rc;
}

//-------------------------add by zlin
int evcpe_boot(struct evcpe *cpe,
		struct evcpe_repo *repo, char bootflag)
{
	int rc;
	const char *value;
	value = evcpe_repo_find(repo, ".ManagementServer.BootStrap");
	if (value == NULL || !strcmp(value, "0") || bootflag == 1) {
		cpe->bootstrap= 1;
	}
	/*else {
		cpe->bootstrap= 0;
	}*/
	return 0;
}
//--------------------------

void evcpe_creq_cb(struct evhttp_request *req, void *arg)
{
	time_t curtime;
	struct evcpe *cpe = arg;

	if (req->type != EVHTTP_REQ_GET || (cpe->creq_url->uri &&
			strncmp(cpe->creq_url->uri,
			evhttp_request_uri(req), strlen(cpe->creq_url->uri)))) {
		evhttp_send_reply(req, 404, "Not Found", NULL);
		return;
	} else {
		curtime = time(NULL);
		if (cpe->creq_last && difftime(curtime, cpe->creq_last) <
				cpe->creq_interval) {
			evhttp_send_reply(req, 503, "Service Unavailable", NULL);
			return;
		} else {
			if (cpe->session) {
				evhttp_send_reply(req, 503, "Session in Progress", NULL);
				return;
			}
			// TODO: handle auth
			if (evcpe_repo_add_event(cpe->repo, "6 CONNECTION REQUEST", "")) {
				evcpe_error(__func__, "failed to add connection request event");
				evhttp_send_reply(req, 501, "Internal Server Error", NULL);
				return;
			}
			if (evcpe_start_session(cpe)) {
				evcpe_error(__func__, "failed to start session");
				evhttp_send_reply(req, 501, "Internal Server Error", NULL);
				return;
			} else {
				cpe->creq_last = curtime;
				evhttp_send_reply(req, 200, "OK", NULL);
				return;
			}
		}
	}
}

int evcpe_bind(struct evcpe *cpe)
{
	int rc;
	struct evhttp *http;

	evcpe_info(__func__, "binding %s on port: %d",
			cpe->creq_url->protocol, cpe->creq_url->port);

	// TODO: SSL

	if (!(http = evhttp_new(cpe->evbase))) {
		evcpe_error(__func__, "failed to create evhttp");
		rc = ENOMEM;
		goto finally;
	}
	if ((rc = evhttp_bind_socket(http, "0.0.0.0", cpe->creq_url->port)) != 0) {
		evcpe_error(__func__, "failed to bind evhttp on port:%d",
				cpe->creq_url->port);
		evhttp_free(http);
		goto finally;
	}

	if (cpe->http) evhttp_free(cpe->http);
	cpe->http = http;
	evcpe_info(__func__, "accepting connection request: %s", cpe->creq_url->uri);
	evhttp_set_gencb(cpe->http, evcpe_creq_cb, cpe);

finally:
	return rc;
}

void evcpe_send_error(struct evcpe *cpe, enum evcpe_error_type type,
		int code, const char *reason)
{
	evcpe_error(__func__, "%d type error: %d %s", type, code, reason);
	if (cpe->error_cb)
		(*cpe->error_cb)(cpe, type, code, reason, cpe->cbarg);
}

int evcpe_retry_session(struct evcpe *cpe)
{
	int rc, base, secs;

	if (evtimer_pending(&cpe->retry_ev, NULL)) {
		evcpe_error(__func__, "another session retry has been scheduled");
		rc = EINVAL;
		goto finally;
	}
	LOG_RUNLOG_DEBUG("TM ***** evcpe_retry_session  ******");

    if (cpe->retry_count == 0) {
		secs = 0;
	} else {
		switch (cpe->retry_count) {
		case 1:
			base = 5;
			break;
		case 2:
			base = 10;
			break;
		case 3:
			base = 20;
			break;
		case 4:
			base = 40;
			break;
		case 5:
			base = 80;
			break;
		case 6:
			base = 160;
			break;
		case 7:
			base = 320;
			break;
		case 8:
			base = 640;
			break;
		case 9:
			base = 1280;
			break;
		default:
			base = 2560;
			break;
		}
		secs = base + rand() % base;
	}

	evcpe_info(__func__, "scheduling session retry in %d second(s)", secs);

	evutil_timerclear(&cpe->retry_tv);
	cpe->retry_tv.tv_sec = secs;
	if ((rc = event_add(&cpe->retry_ev, &cpe->retry_tv)))
		evcpe_error(__func__, "failed to add timer event");

finally:
	return rc;
}

//---------------add by zlin
int evcpe_delete_event(struct evcpe *cpe)
{
	int rc;

	if (!cpe->repo) {
		evcpe_error(__func__, "evcpe is not initialized");
		rc = EINVAL;
		goto finally;
	}

	evcpe_info(__func__, "delete evcpe event");

	evcpe_repo_del_event(cpe->repo, "0 BOOTSTRAP");
	evcpe_repo_del_event(cpe->repo, "1 BOOT");
	evcpe_repo_del_event(cpe->repo, "X 000001 LOGOUT");
	evcpe_repo_del_event(cpe->repo, "X 000001 PARAMETER_CHANGE");
	evcpe_repo_del_event(cpe->repo, "X 000001 UPDATE_ASK");
	evcpe_repo_del_event(cpe->repo, "4 VALUE CHANGE");
	rc = 0;

finally:
	return rc;
}
//------------------

int evcpe_start(struct evcpe *cpe)
{
	int rc;
	const char *value = NULL;
	unsigned int len = 0;
	int iIndex = 0;
	int iUserCount = 0;
	unsigned int index;

	if (!cpe->repo) {
		evcpe_error(__func__, "evcpe is not initialized");
		rc = EINVAL;
		goto finally;
	}

	evcpe_info(__func__, "starting evcpe");
	//--------------- edit by zlin
	if (cpe->bootstrap == 0) {
		if ((rc = evcpe_repo_add_event(cpe->repo, "1 BOOT", ""))) {
			evcpe_error(__func__, "failed to add boot event");
			goto finally;
		}
		evcpe_index_load();
#ifdef TM_CONFIG_DOWNLOAD
		if(strlen(g_stTemp.acICIDLog) > 0)
		{
			cpe->icidflag = 1;
		}
#else
		evcpe_repo_get(cpe->repo, ".Config.ICIDOpenLog", &value, &len);
		if(value && len>0)
		{
			cpe->icidflag = 1;
		}
		evcpe_repo_get(cpe->repo, ".Business.UserCount", &value, &len);
		if(value && len>0)
		{
			iUserCount = atoi(value);
			iIndex = g_stTemp.iIndex;
			while(iUserCount < iIndex)
			{
				//添加索引
				if ((rc = evcpe_repo_add_obj(cpe->repo,".Business.UserInfo.",&index)))
				{
					evcpe_error(__func__, "failed to add object: %s", ".Business.UserInfo.");
				}
				iUserCount ++;
				LOG_RUNLOG_DEBUG("TM evcpe_start add object %d",index);
			}
		}
#endif
	}
	else {
		if ((rc = evcpe_repo_add_event(cpe->repo, "0 BOOTSTRAP", ""))) {
			evcpe_error(__func__, "failed to add boot event");
			goto finally;
		}
		evcpe_index_del();
		evcpe_index_load();
		evcpe_repo_get(cpe->repo, ".Business.UserCount", &value, &len);
		if(value && len>0)
		{
			if ((rc = evcpe_repo_del_object_info(cpe->repo, ".Business.",EVCPE_USERINFO_STR)))
			{
				evcpe_error(__func__, "failed to delete object: %s", ".Business");
				goto finally;
			}
		}
		evcpe_repo_get(cpe->repo, ".PropertyCard.", &value, &len);
		if(value && len>0)
		{
			if ((rc = evcpe_repo_del_object_info(cpe->repo, ".PropertyCard.",EVCPE_CARDINFO_STR))) {
				evcpe_error(__func__, "failed to delete object: %s", ".PropertyCard");
				goto finally;
			}
		}
		evcpe_repo_get(cpe->repo, ".Demo.PhoneCount", &value, &len);
		if(value && len>0)
		{
			if ((rc = evcpe_repo_del_object_info(cpe->repo, ".Demo.",EVCPE_PHONEINFO_STR))) {
				evcpe_error(__func__, "failed to delete object: %s", ".Demo");
				goto finally;
			}
		}
		evcpe_repo_get(cpe->repo, ".Config.ICIDOpenLog", &value, &len);
		if(value && len>0)
		{
			if (evcpe_repo_set(cpe->repo, ".Config.ICIDOpenLog", "", 0))
			{
				evcpe_error(__func__, "failed to set param: %s", "icidflag");
				return EPROTO;
			}
		}
	}
	//----------------
//	if (cpe->creq_url && (rc = evcpe_bind(cpe))) {
//		evcpe_error(__func__, "failed to bind for connection request");
//		goto finally;
//	}
	evtimer_set(&cpe->retry_ev, evcpe_start_session_cb, cpe);
	event_base_set(cpe->evbase, &cpe->retry_ev);
	if ((rc = evcpe_start_session(cpe))) {
		evcpe_error(__func__, "failed to start session");
		//goto finally;		//del by zlin
	}
	rc = 0;

finally:
	return rc;
}

void evcpe_start_session_cb(int fd, short ev, void *arg)
{
	int rc;
	struct evcpe *cpe = arg;


    //------------------------add by zlin
	if(cpe->boot_flag == 1)
	{
		cpe->boot_flag = 0;
		if ((rc = evcpe_repo_add_event(cpe->repo, "1 BOOT", ""))) {
			evcpe_error(__func__, "failed to add boot event");
		}
	}
	else if(cpe->logout == 1)
	{
		cpe->logout = 0;
		if ((rc = evcpe_repo_add_event(cpe->repo, "X 000001 LOGOUT", ""))) {
			evcpe_error(__func__, "failed to add boot event");
		}
	}
	else if(cpe->parameter_change == 1)
	{
		cpe->parameter_change = 0;
		if ((rc = evcpe_repo_add_event(cpe->repo, "X 000001 PARAMETER_CHANGE", ""))) {
			evcpe_error(__func__, "failed to add boot event");
		}
	}
	else if(cpe->update_ask== 1)
	{
		cpe->update_ask = 0;
		if ((rc = evcpe_repo_add_event(cpe->repo, "X 000001 UPDATE_ASK", ""))) {
			evcpe_error(__func__, "failed to add boot event");
		}
	}
	//--------------------------
	if (!cpe->session && (rc = evcpe_start_session(cpe))) {
		evcpe_error(__func__, "failed to start session");
	}
	if (event_initialized(&cpe->periodic_ev) &&
			!evtimer_pending(&cpe->periodic_ev, NULL)) {
		evcpe_info(__func__, "scheduling periodic inform in %ld second(s)",
				cpe->periodic_tv.tv_sec);
		if ((rc = event_add(&cpe->periodic_ev, &cpe->periodic_tv)))
			evcpe_error(__func__, "failed to schedule periodic inform");
	}
}

int evcpe_start_session(struct evcpe *cpe)
{
	int rc = 0;
	struct evcpe_inform *inform;
	struct evcpe_msg *msg;
	const char *hostname, *address;
	u_short port;
	struct evhttp_connection *conn;

	if(g_bConfigUpdateFlag == 2)
	{
		goto finally;
	}

	/*会话正忙*/
	if (cpe->session) {
		evcpe_error(__func__, "session in progress");
		rc = EINVAL;
		goto finally;
	}

	evcpe_info(__func__, "starting session");

	msg = NULL;
	conn = NULL;

	hostname = cpe->proxy_url ? cpe->proxy_url->host : cpe->acs_url->host;
	port = cpe->proxy_url ? cpe->proxy_url->port : cpe->acs_url->port;
	/*查dns获取url域名的ip信息*/
	if (!(address = evcpe_dns_cache_get(&cpe->dns_cache, hostname))) {
		evcpe_info(__func__, "hostname not resolved: %s", hostname);
		cpe->retry_count ++;
		/*重试*/
		if ((rc = evcpe_retry_session(cpe)))
			evcpe_error(__func__, "failed to schedule session retry");
		goto finally;
	}
	/*分配消息内存*/
	if (!(msg = evcpe_msg_new())) {
		evcpe_error(__func__, "failed to create evcpe_msg");
		rc = ENOMEM;
		goto exception;
	}
	/*分配会话随机数*/
	if (!(msg->session = evcpe_ltoa(random()))) {
		evcpe_error(__func__, "failed to create session string");
		rc = ENOMEM;
		goto exception;
	}
	msg->major = 1;
	msg->minor = 0;
	/*新建通知会话，初始化事件和参数队列*/
	if (!(msg->data = inform = evcpe_inform_new())) {
		evcpe_error(__func__, "failed to create inform request");
		rc = ENOMEM;
		goto exception;
	}
	/*设置消息类型和请求方法类型*/
	msg->type = EVCPE_MSG_REQUEST;
	msg->method_type = EVCPE_INFORM;
	/*将data.xml对应的repo结构中得数据填充到inform结构中*/
	if ((rc = evcpe_repo_to_inform(cpe->repo, inform, cpe))) {
		evcpe_error(__func__, "failed to prepare inform message");
		goto exception;
	}
	inform->retry_count = cpe->retry_count;
	/*新建http连接*/
	if (!(conn = evhttp_connection_new(address, port))) {
		evcpe_error(__func__, "failed to create evhttp_connection");
		rc = ENOMEM;
		goto exception;
	}
	/*关联事件和设置超时*/
	evhttp_connection_set_base(conn, cpe->evbase);
	evhttp_connection_set_timeout(conn, cpe->acs_timeout);
	/*新建连接会话，置一些会话处理的回调函数*/
	if (!(cpe->session = evcpe_session_new(conn, cpe->acs_url,
			evcpe_session_message_cb, cpe))) {
		evcpe_error(__func__, "failed to create evcpe_session");
		rc = ENOMEM;
		goto exception;
	}
	/*置一些关闭会话的回调函数*/
	evcpe_session_set_close_cb(cpe->session, evcpe_session_terminate_cb, cpe);
	/*检索并处理消息待发送队列，准备发送会话请求*/
	if ((rc = evcpe_session_send(cpe->session, msg))) {
		evcpe_error(__func__, "failed to send inform message");
		goto exception;
	}
	/*开始发起会话请求*/
	if ((rc = evcpe_session_start(cpe->session))) {
		evcpe_error(__func__, "failed to start session");
		//----------------edit by zlin
		//goto finally;
		goto exception;
		//----------------
	}
	rc = 0;
	goto finally;

exception:
	cpe->timeoutNum = 10;//add by zlin
	if (msg) evcpe_msg_free(msg);
	if (conn) evhttp_connection_free(conn);
	if (cpe->session) {
		evcpe_session_free(cpe->session);
		cpe->session = NULL;
	}
finally:
	return rc;
}

/*关闭会话的回调函数*/
void evcpe_session_terminate_cb(struct evcpe_session *session, int rc,
		void *arg)
{
	struct evcpe *cpe = arg;
	evhttp_connection_free(session->conn);
	evcpe_session_free(cpe->session);
	cpe->session = NULL;
	if (rc) {
		evcpe_error(__func__, "session failed: %d - %s", rc, strerror(rc));
		cpe->retry_count ++;
		if (evcpe_retry_session(cpe)) {
			evcpe_error(__func__, "failed to schedule session retry");
		}
	} else {
		cpe->retry_count = 0;
	}
}

/*请求会话报文处理消息回调函数，发送回应报文函数*/
void evcpe_session_message_cb(struct evcpe_session *session,
		enum evcpe_msg_type type, enum evcpe_method_type method_type,
				void *request, void *response, void *arg)
{
	int rc;
	struct evcpe *cpe = arg;

	evcpe_info(__func__, "handling %s %s message from ACS",
			evcpe_method_type_to_str(method_type), evcpe_msg_type_to_str(type));

	switch(type) {
	case EVCPE_MSG_REQUEST:/*处理请求报文*/
		if ((rc = evcpe_handle_request(cpe, session, method_type, request))) {
			evcpe_error(__func__, "failed to handle request");
			goto close_session;
		}
		break;
	case EVCPE_MSG_RESPONSE:/*处理回应报文*/
		if ((rc = evcpe_handle_response(cpe, method_type, request, response))) {
			evcpe_error(__func__, "failed to handle response");
			goto close_session;
		}
		break;
	case EVCPE_MSG_FAULT:/*处理错误报文*/
		evcpe_error(__func__, "CWMP fault encountered: %d - %s",
				((struct evcpe_fault *)request)->code,
				((struct evcpe_fault *)request)->string);
		// TODO: notifies
		goto close_session;
	default:
		evcpe_error(__func__, "unexpected message type: %d", type);
		rc = EINVAL;
		goto close_session;
	}
	return;
/*关闭连接会话*/
close_session:
	evcpe_session_close(cpe->session, rc);
}

/*处理请求报文*/
int evcpe_handle_request(struct evcpe *cpe, struct evcpe_session *session,
		enum evcpe_method_type method_type, void *request)
{
	int rc;
	struct evcpe_msg *msg;
	struct evcpe_fault *fault;

	if (!(msg = evcpe_msg_new())) {
		evcpe_error(__func__, "failed to create evcpe_msg");
		rc = ENOMEM;
		goto finally;
	}
	msg->method_type = method_type;
	switch(method_type) {
	case EVCPE_GET_RPC_METHODS:/*获取支持方法集*/
		rc = evcpe_handle_get_rpc_methods(cpe, request, msg);
		break;
	case EVCPE_GET_PARAMETER_NAMES:/*获取参数名称*/
		rc = evcpe_handle_get_param_names(cpe, request, msg);
		break;
	case EVCPE_GET_PARAMETER_ATTRIBUTES:/*获取参数属性*/
		rc = evcpe_handle_get_param_attrs(cpe, request, msg);
		break;
	case EVCPE_GET_PARAMETER_VALUES:/*获取参数值*/
		rc = evcpe_handle_get_param_values(cpe, request, msg);
		break;
	case EVCPE_ADD_OBJECT:/*增加新实例*/
		rc = evcpe_handle_add_object(cpe, request, msg);
		break;
	//---------------add by zlin
	case EVCPE_DELETE_OBJECT:/*删除实例*/
		rc = evcpe_handle_delete_object(cpe, request, msg);
		break;
	//---------------
	case EVCPE_SET_PARAMETER_VALUES:/*设置参数值*/
		rc = evcpe_handle_set_param_values(cpe, request, msg);
		break;
	//---------------add by zlin
	case EVCPE_REBOOT:/*重启*/
		evcpe_info(__func__, "evcpe reboot");
		rc = evcpe_handle_reboot(cpe, request, msg);
		if (evcpe_callback.reboot_cb)
			(*evcpe_callback.reboot_cb)();
		break;
	case EVCPE_DOWNLOAD:/*下载*/
		evcpe_info(__func__, "evcpe download");
		rc = evcpe_handle_download(cpe, request, msg);
		break;
	case EVCPE_UPLOAD:/*上载*/
		evcpe_info(__func__, "evcpe upload");
		rc = evcpe_handle_upload(cpe, request, msg);
		break;
	//------------------
	default:
		evcpe_error(__func__, "unexpected method type: %s",
				evcpe_method_type_to_str(method_type));
		rc = EVCPE_CPE_METHOD_NOT_SUPPORTED;
		break;
	}
	if (rc) {
		if (!(msg->data = fault = evcpe_fault_new())) {
			rc = ENOMEM;
			goto finally;
		}
		msg->type = EVCPE_MSG_FAULT;
		if (rc >= EVCPE_CPE_FAULT_MIN && rc <= EVCPE_CPE_FAULT_MAX)
			fault->code = rc;
//		else if (rc == ENOMEM)
//			fault->code = EVCPE_CPE_RESOUCES_EXCEEDS;
		else
			fault->code = EVCPE_CPE_INTERNAL_ERROR;
	}
	if ((rc = evcpe_session_send(session, msg))) {
		evcpe_error(__func__, "failed to send CWMP %s message",
				evcpe_method_type_to_str(msg->method_type));
		evcpe_msg_free(msg);
		goto finally;
	}
	rc = 0;

finally:
	return rc;
}

/*ACS响应报文处理函数*/
int evcpe_handle_response(struct evcpe *cpe,
		enum evcpe_method_type method_type, void *request, void *response)
{
	int rc;

	switch(method_type) {
	case EVCPE_INFORM:/*处理acs的inform的回应报文*/
		if ((rc = evcpe_handle_inform_response(cpe, request, response)))
			goto finally;
		//---------------add by zlin
		if(cpe->transfer.flag == 1)
		{      
			evcpe_handle_transfercomplete(cpe, cpe->session);
			cpe->transfer.flag = 0;
		}
		//----------------
		break;
	//--------------add by zlin
	case EVCPE_TRANSFER_COMPLETE:/*处理acs的transfercomplete的回应报文*/
		//如果是配置文件成功下载则重启
		if(g_bConfigUpdateFlag == 1)
		{
			//退出
			g_bConfigUpdateFlag = 2;
//			LOG_RUNLOG_DEBUG("ID ConfigUpdate OK!!!!!!!!");
			evcpe_quit();
		}
		break;
	//--------------
	default:
		evcpe_error(__func__, "unexpected method type: %d", method_type);
		rc = EINVAL;
		goto finally;
	}
	rc = 0;

finally:
	return rc;
}

/*rpc获取支持的远程方法名*/
int evcpe_handle_get_rpc_methods(struct evcpe *cpe,
		struct evcpe_get_rpc_methods *req,
		struct evcpe_msg *msg)
{
	int rc;
	struct evcpe_get_rpc_methods_response *method;
       /*设置消息类型*/
	msg->type = EVCPE_MSG_RESPONSE;
       /*分配数据结构的内存，并初始化列表*/
	if (!(msg->data = method = evcpe_get_rpc_methods_response_new())) {
		evcpe_error(__func__, "failed to create "
				"evcpe_get_rpc_methods_response");
		rc = ENOMEM;
		goto finally;
	}
       /*以下将支持的方法加入到列表中*/
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"GetRPCMethods"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"GetParameterNames"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"GetParameterValues"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"SetParameterValues"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"GetParameterAttributes"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"SetParameterAttributes"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"AddObject"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"DeleteObject"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"Download"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"Upload"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"Reboot"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"FactoryReset"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"GetQueuedTransfers"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	if ((rc = evcpe_method_list_add_method(&method->method_list,
			"ScheduleInform"))) {
		evcpe_error(__func__, "failed to add method");
		goto finally;
	}
	rc = 0;

finally:
	return rc;
}

/*rpc获取参数名*/
int evcpe_handle_get_param_names(struct evcpe *cpe,
		struct evcpe_get_param_names *req,
		struct evcpe_msg *msg)
{
	int rc;
	struct evcpe_get_param_names_response *resp;
       /*设置消息类型*/
	msg->type = EVCPE_MSG_RESPONSE;
       /*分配回应信息数据结构内存并初始化*/
	if (!(msg->data = resp = evcpe_get_param_names_response_new())) {
		evcpe_error(__func__, "failed to create "
				"evcpe_get_param_names_response");
		rc = ENOMEM;
		goto finally;
	}
       /*从内存树结构中获取节点参数名并加到列表中，内存树中信息来源于tr098_model.xml*/
	if ((rc = evcpe_repo_to_param_info_list(cpe->repo,
			req->parameter_path, &resp->parameter_list, req->next_level))) {
		evcpe_error(__func__, "failed to get param names: %s",
				req->parameter_path);
		evcpe_get_param_names_response_free(resp);
		goto finally;
	}
	rc = 0;

finally:
	return rc;
}

/*获取支持的参数节点值*/
int evcpe_handle_get_param_values(struct evcpe *cpe,
		struct evcpe_get_param_values *req,
		struct evcpe_msg *msg)
{
	int rc;
	struct evcpe_param_name *param;
	struct evcpe_get_param_values_response *resp;

       /*设置消息类型*/
	msg->type = EVCPE_MSG_RESPONSE;
       /*分配存储回应参数节点值的数据结构，并初始化队列*/
	if (!(msg->data = resp = evcpe_get_param_values_response_new())) {
		evcpe_error(__func__, "failed to create "
				"evcpe_get_param_values_response");
		rc = ENOMEM;
		goto finally;
	}
       /*遍历请求队列中ACS 传进来的获取参数值的节点参数名*/
	TAILQ_FOREACH(param, &req->parameter_names.head, entry)
	{
#ifdef TM_CONFIG_DOWNLOAD
		//如果是查询CRC
		if(strcmp(param->name,"InternetGatewayDevice.Config.CRC") == 0)
		{
			memset(g_acCRC,0,10);
			//获取CRC
			TMGetCRC(g_acCRC);
			//设入CRC值
			evcpe_value_to_param_value_list(&resp->parameter_list, "InternetGatewayDevice.Config.CRC", strlen("InternetGatewayDevice.Config.CRC"),
					g_acCRC, strlen(g_acCRC), EVCPE_TYPE_STRING);
		}
		else if(strcmp(param->name,"InternetGatewayDevice.DeviceInfo.FirmwareVersion") == 0)
		{
			evcpe_value_to_param_value_list(&resp->parameter_list, "InternetGatewayDevice.DeviceInfo.FirmwareVersion", strlen("InternetGatewayDevice.DeviceInfo.FirmwareVersion"),
					YK_APP_VERSION, strlen(YK_APP_VERSION), EVCPE_TYPE_STRING);
		}
		else if(strcmp(param->name,"InternetGatewayDevice.DeviceInfo.KernelVersion") == 0)
		{
			evcpe_value_to_param_value_list(&resp->parameter_list, "InternetGatewayDevice.DeviceInfo.KernelVersion", strlen("InternetGatewayDevice.DeviceInfo.KernelVersion"),
					YK_KERNEL_VERSION, strlen(YK_KERNEL_VERSION), EVCPE_TYPE_STRING);
		}
		else if(strcmp(param->name,"InternetGatewayDevice.DeviceInfo.HardwareVersion") == 0)
		{
			evcpe_value_to_param_value_list(&resp->parameter_list, "InternetGatewayDevice.DeviceInfo.HardwareVersion", strlen("InternetGatewayDevice.DeviceInfo.HardwareVersion"),
					YK_HARDWARE_VERSION, strlen(YK_HARDWARE_VERSION), EVCPE_TYPE_STRING);
		}
		else
#endif
		/*将repo结构中的节点的参数值获取并放入链表中*/
		if ((rc = evcpe_repo_to_param_value_list(cpe->repo,
				param->name, &resp->parameter_list))) {
			evcpe_error(__func__, "failed to get param values: %s",
					param->name);
                     /*获取出错，释放预分配的内存，返回错误*/
			evcpe_get_param_values_response_free(resp);
			goto finally;
		}
	}
	rc = 0;

finally:
	return rc;
}

int evcpe_handle_get_param_attrs(struct evcpe *cpe,
		struct evcpe_get_param_attrs *req,
		struct evcpe_msg *msg)
{
	int rc;
	struct evcpe_param_name *param;
	struct evcpe_get_param_attrs_response *resp;

	msg->type = EVCPE_MSG_RESPONSE;
	if (!(msg->data = resp = evcpe_get_param_attrs_response_new())) {
		evcpe_error(__func__, "failed to create "
				"evcpe_get_param_attrs_response");
		rc = ENOMEM;
		goto finally;
	}
	TAILQ_FOREACH(param, &req->parameter_names.head, entry) {
		if ((rc = evcpe_repo_to_param_attr_list(cpe->repo,
				param->name, &resp->parameter_list))) {
			evcpe_error(__func__, "failed to get param values: %s",
					param->name);
			evcpe_get_param_attrs_response_free(resp);
			goto finally;
		}
	}
	rc = 0;

finally:
	return rc;
}

int evcpe_handle_add_object(struct evcpe *cpe,
		struct evcpe_add_object *req,
		struct evcpe_msg *msg)
{
	int rc;
	unsigned int index;
	struct evcpe_add_object_response *resp;
	const char *value;
	unsigned int len;

	if ((rc = evcpe_repo_add_obj(cpe->repo, req->object_name, &index))) {
		evcpe_error(__func__, "failed to add object: %s", req->object_name);
		goto finally;
	}

	//--------------------------add by hb
	evcpe_repo_get(cpe->repo, ".Business.UserCount", &value, &len);
	if(value && len > 0)
	{
		g_stTemp.iIndex = atoi(value);
		evcpe_index_save();
	}
	//--------------------------add by hb

	// TODO: set ParameterKey
	msg->type = EVCPE_MSG_RESPONSE;
	if (!(msg->data = resp = evcpe_add_object_response_new())) {
		evcpe_error(__func__, "failed to create add_object_response");
		rc = ENOMEM;
		goto finally;
	}
	resp->instance_number = index;
	resp->status = 0;
	rc = 0;

finally:
	return rc;
}

//-------------------add by zlin
int evcpe_handle_delete_object(struct evcpe *cpe,
		struct evcpe_delete_object *req,
		struct evcpe_msg *msg)
{
	int rc;
	unsigned int index;
	struct evcpe_delete_object_response *resp;
	const char *value;
	unsigned int len;
	int iUserCount = 0;
	int iPhoneCount = 0;
	char acCMD[128] = {0x0};
	char acLogInfo[256] = {0x0};

	memset(acLogInfo,0,256);
	if(strstr(req->object_name,"InternetGatewayDevice.Business.UserInfo"))
	{
		sscanf(req->object_name,"InternetGatewayDevice.Business.UserInfo.%d.PhoneInfo.%d",&iUserCount,&iPhoneCount);
		if(iUserCount != 0 && iPhoneCount != 0)
		{
			sprintf(acCMD,"InternetGatewayDevice.Business.UserInfo.%d.RoomNumber",iUserCount);
			evcpe_repo_get(g_tr_evcpe_st.repo, acCMD, &value, &len);
			if(value && len != 0)
			{
				sprintf(acLogInfo,", RoomNumber : %s",value);
			}
			sprintf(acCMD,"InternetGatewayDevice.Business.UserInfo.%d.PhoneInfo.%d.CsPhoneNum",iUserCount, iPhoneCount);
			value = NULL;
			evcpe_repo_get(g_tr_evcpe_st.repo, acCMD, &value, &len);
			if(value && len != 0)
			{
				strcat(acLogInfo," , CsPhone : ");
				strcat(acLogInfo,value);
			}
			sprintf(acCMD,"InternetGatewayDevice.Business.UserInfo.%d.PhoneInfo.%d.SipAccount",iUserCount, iPhoneCount);
			value = NULL;
			evcpe_repo_get(g_tr_evcpe_st.repo, acCMD, &value, &len);
			if(value && len != 0)
			{
				strcat(acLogInfo," , SipPhone : ");
				strcat(acLogInfo,value);
				strcat(acLogInfo," .");
			}
		}
		else if(iUserCount != 0 && iPhoneCount == 0)
		{
			sprintf(acCMD,"InternetGatewayDevice.Business.UserInfo.%d.RoomNumber",iUserCount);
			evcpe_repo_get(g_tr_evcpe_st.repo, acCMD, &value, &len);
			if(value && len != 0)
			{
				sprintf(acLogInfo,", RoomNumber : %s .",value);
			}
		}
		else if(iUserCount == 0 && iPhoneCount == 0)
		{
			sprintf(acLogInfo,".");
		}
	}
	LOG_RUNLOG_INFO("TR Object Name : %s %s",req->object_name,acLogInfo);

	if ((rc = evcpe_repo_del_obj(cpe->repo, req->object_name))) {
		evcpe_error(__func__, "failed to delete object: %s", req->object_name);
		//goto finally;//del by zlin order to acs
	}
	// TODO: set ParameterKey
	msg->type = EVCPE_MSG_RESPONSE;
	if (!(msg->data = resp = evcpe_delete_object_response_new())) {
		evcpe_error(__func__, "failed to create delete_object_response");
		rc = ENOMEM;
		goto finally;
	}
	resp->status = 0;
	rc = 0;

finally:
	return rc;
}
//--------------------
    
int evcpe_handle_set_param_values(struct evcpe *cpe,
		struct evcpe_set_param_values *req,
		struct evcpe_msg *msg)
{
	int rc;
	char notifyflag = 0;
	char demoflag = 0;
	char wanflag = 0;
	char phonenum[64]={0x0};
	char roomnum[64]={0x0};
	char voipen[2]={0x0};
	struct evcpe_set_param_value *param;
	struct evcpe_set_param_values_response *resp;

	TAILQ_FOREACH(param, &req->parameter_list.head, entry)
	{
#ifdef TM_CONFIG_DOWNLOAD
		if (strstr(param->name, "Demo.PhoneNum") != NULL) {
			strncpy(phonenum, param->data, param->len);
			demoflag = 1;
			continue;
		}
		else if (strstr(param->name, "Demo.VoipEn") != NULL) {
			strncpy(voipen, param->data, param->len);
			//demoflag = 1;
			continue;
		}
		else if (strstr(param->name, "Demo.RoomNum") != NULL) {
			strncpy(roomnum, param->data, param->len);
			demoflag = 2;
			continue;
		}
#endif
		if ((rc = evcpe_repo_set(cpe->repo, param->name,
				param->data, param->len)))
		{
			evcpe_error(__func__, "failed to set param: %s", param->name);
			// TODO: error codes
			goto finally;
		}
		//-------------------add by zlin
		if (strstr(param->name, "Communication.SipAccount") != NULL) {
			notifyflag = 1;
		}
		else if (strstr(param->name, "Communication.SipPassword") != NULL) {
			notifyflag = 1;
		}
		else if (strstr(param->name, "Communication.ImsDomainName") != NULL) {
			notifyflag = 1;
		}
		else if (strstr(param->name, "Communication.ImsProxyIP") != NULL) {
			notifyflag = 1;
		}
#ifndef TM_CONFIG_DOWNLOAD
		if (strstr(param->name, "Demo.PhoneNum") != NULL) {
			strncpy(phonenum, param->data, param->len);
			demoflag = 1;
		}
		else if (strstr(param->name, "Demo.VoipEn") != NULL) {
			strncpy(voipen, param->data, param->len);
			//demoflag = 1;
		}
		else if (strstr(param->name, "Demo.RoomNum") != NULL) {
			strncpy(roomnum, param->data, param->len);
			demoflag = 2;
		}
#endif
		if (strstr(param->name, "Communication.WanUserName") != NULL) {
			wanflag = 1;
		}
		else if (strstr(param->name, "Communication.WanPassword") != NULL) {
			wanflag = 1;
		}

		//--------------------
	}
	// TODO: set ParameterKey
	msg->type = EVCPE_MSG_RESPONSE;
	if (!(msg->data = resp = evcpe_set_param_values_response_new())) {
		evcpe_error(__func__, "failed to create "
				"evcpe_get_param_values_response");
		rc = ENOMEM;
		goto finally;
	}
	//-------------------add by zlin
	if (evcpe_callback.nodifyinfo_cb)
		(*evcpe_callback.nodifyinfo_cb)();
	if (notifyflag == 1) {
		if (evcpe_callback.nodifysip_cb)
			(*evcpe_callback.nodifysip_cb)();
	}
	if (demoflag == 1) {
		if (evcpe_callback.demo_cb)
			(*evcpe_callback.demo_cb)(phonenum, voipen);
	}
	if (demoflag == 2) {
		if (evcpe_callback.demo_room_cb)
			(*evcpe_callback.demo_room_cb)(roomnum);
	}
	if (wanflag == 1) {
		if (evcpe_callback.nodifywan_cb)
			(*evcpe_callback.nodifywan_cb)();
	}
	//-------------------
	resp->status = 0;
	rc = 0;

finally:
	return rc;
}

//----------------------add by zlin
int evcpe_handle_transfercomplete(struct evcpe *cpe, struct evcpe_session *session)
{
	int rc;
	struct evcpe_transfercomplete *resp;

	struct evcpe_msg *msg;

	if (!(msg = evcpe_msg_new())) {
		evcpe_error(__func__, "failed to create evcpe_msg");
		rc = ENOMEM;
		goto finally;
	}
    
       /*分配会话随机数*/
	if (!(msg->session = evcpe_ltoa(random()))) {
		evcpe_error(__func__, "failed to create session string");
		rc = ENOMEM;
		goto finally;
	}
	msg->major = 1;
	msg->minor = 0;
    
	msg->method_type = EVCPE_TRANSFER_COMPLETE;
	msg->type = EVCPE_MSG_REQUEST;
	if (!(msg->data = resp = evcpe_transfercomplete_new())) {
		evcpe_error(__func__, "failed to create "
				"evcpe_get_param_values_response");
		rc = ENOMEM;
		goto finally;
	}
    
	//resp->commandkey = cpe->id.device_no;
	resp->fault.code = cpe->transfer.result;
	resp->starttime = cpe->transfer.starttime;
	resp->completetime = cpe->transfer.completetime;
	
	if ((rc = evcpe_session_send(session, msg))) {
		evcpe_error(__func__, "failed to send CWMP %s message",
				evcpe_method_type_to_str(msg->method_type));
		evcpe_msg_free(msg);
		goto finally;
	}
	rc = 0;

finally:
	return rc;

}

int evcpe_handle_download(struct evcpe *cpe,
		struct evcpe_download *req,
		struct evcpe_msg *msg)
{
	int rc;
	//struct evcpe_download *param;
	struct evcpe_download_response *resp;
	enum TM_TRANSFER_TYPE_EN enType;
	enum TM_TRANSFER_FILE_TYPE_EN enFileType;
	char pcLocalUrl[256]={0x0};
	char pcRemoteUrl[256]={0x0};
	char buf[256]={0x0};

	memset(pcLocalUrl, 0x0, sizeof(pcLocalUrl));
	memset(pcRemoteUrl, 0x0, sizeof(pcRemoteUrl));
	LOG_RUNLOG_DEBUG("TM [%s] req->file_type = %s", __func__, req->file_type);

	//-------------add by hb
	if (!strcmp(req->file_type, "3 Vendor Configuration File"))
	{
		if (evcpe_callback.config_cb)
		{
			LOG_RUNLOG_DEBUG("TM start to 3 Vendor Configuration File");
			(*evcpe_callback.config_cb)(req->url,NULL,NULL);
		}
	}
	else if(!strcmp(req->file_type,"1 Firmware Upgrade Image"))
	{
		if (evcpe_callback.upgrade_cb)
		{
			LOG_RUNLOG_DEBUG("TM start to 1 Firmware Upgrade Image");
			(*evcpe_callback.upgrade_cb)(req->url,req->target_filename);
		}
	}
#ifdef TM_CONFIG_DOWNLOAD
	if (!strcmp(req->file_type, "3 TR069 TMData File"))
	{
		if (evcpe_callback.tr_config_cb)
		{
			LOG_RUNLOG_DEBUG("TM start to download TRConfig.xml.bz2");
			(*evcpe_callback.tr_config_cb)(req->url,req->target_filename);
		}
	}
#endif
	//-------------add by hb
	else
	{
		if (!strcmp(req->file_type, "X 000001 Devsetinfo File")) {
			enFileType = TM_TYPE_CONFIG;
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
		//解压配置文件到/tmp
			system("busybox rm /tmp/TMData.xml*");
//			sprintf(buf,"cp -rf %s %s.bz2",TM_DATA_PATH,TM_DATA_UPLOAD);
//			system(buf);
//			system("bunzip2 /tmp/TMData.xml.bz2");
			strcpy(pcLocalUrl, TM_DATA_UPLOAD);
#else
			strcpy(pcLocalUrl, TM_DATA_PATH);
#endif
		}
		else if (!strcmp(req->file_type, "X 000001 TRModel File")) {
			enFileType=TM_TYPE_TR_MODEL;
			strcpy(pcLocalUrl, TM_MODEL_PATH);
		}
		else if (!strcmp(req->file_type, "X 000001 Ringsound File")) {
			enFileType = TM_TYPE_RINGING;
			//strcpy(pcLocalUrl, "/data/data/com.fsti.ladder/Ringing");
			strcpy(pcLocalUrl, "/data/data/com.fsti.ladder/adv.zip");
			LOG_RUNLOG_DEBUG("TM [%s] pcLocalUrl = %s", __func__, pcLocalUrl);
		}
		else if (!strcmp(req->file_type, "X 000001 Fireversion File")) {
			enFileType = TM_TYPE_UPDATE;
			strcpy(pcLocalUrl, "/data/data/com.fsti.ladder/update/AndroidLadder.apk");
		}
		else if (!strcmp(req->file_type, "X 000001 Advert File")) {
			enFileType = TM_TYPE_ADVERT;
			strcpy(pcLocalUrl, "/data/data/com.fsti.ladder/adv.zip");
		}
		else {
			enFileType = TM_TYPE_OTHER;
			strcpy(pcLocalUrl, "");
		}
		enType = TM_REMOTE_DOWNLOAD;
		strncpy(pcRemoteUrl, req->url, strlen(req->url));
		LOG_RUNLOG_DEBUG("TM pcLocalUrl =  %s pcRemoteUrl = %s", pcLocalUrl, pcRemoteUrl);

		if (evcpe_callback.load_cb) {
			(*evcpe_callback.load_cb)(enType, enFileType, pcLocalUrl, pcRemoteUrl);
		}
	}

	// TODO: set ParameterKey
	msg->type = EVCPE_MSG_RESPONSE;
	if (!(msg->data = resp = evcpe_download_response_new())) {
		evcpe_error(__func__, "failed to create "
				"evcpe_download_response");
		rc = ENOMEM;
		goto finally;
	}
	resp->status = 0;
	rc = 0;

finally:
	return rc;
}

int evcpe_handle_upload(struct evcpe *cpe,
		struct evcpe_upload *req,
		struct evcpe_msg *msg)
{
	int rc;
	//struct evcpe_upload *param;
	struct evcpe_upload_response *resp;
	enum TM_TRANSFER_TYPE_EN enType;
	enum TM_TRANSFER_FILE_TYPE_EN enFileType;
	char pcLocalUrl[256]={0x0};
	char pcRemoteUrl[256]={0x0};
	char buf[256] = {0x0};

	memset(pcLocalUrl, 0x0, sizeof(pcLocalUrl));
	memset(pcRemoteUrl, 0x0, sizeof(pcRemoteUrl));
	LOG_RUNLOG_DEBUG("TM [%s] req->file_type = %s ", __func__, req->file_type);
	if (!strcmp(req->file_type, "X 000001 Configuration File")) {
		enFileType = TM_TYPE_CONFIG;
#if defined(DATA_BZIP) && defined(_YK_XT8126_BV_)
		//解压配置文件到/tmp
		system("busybox rm /tmp/TMData.xml*");
		sprintf(buf,"busybox cp -rf %s %s.bz2",TM_DATA_PATH,TM_DATA_UPLOAD);
		system(buf);
		system("busybox bunzip2 /tmp/TMData.xml.bz2");
		strcpy(pcLocalUrl, TM_DATA_UPLOAD);
#else
		strcpy(pcLocalUrl, TM_DATA_PATH);
#endif
	}
	else if (!strcmp(req->file_type, "X 000001 Runlog File")) {
        enFileType=TM_TYPE_EVENT_LOG;
        strcpy(pcLocalUrl, LG_LOCAL_PATH);
	}
	else {
        enFileType=TM_TYPE_OTHER;
        strcpy(pcLocalUrl, "");
	}
	enType =  TM_REMOTE_UPLOAD;
	strncpy(pcRemoteUrl, req->url, strlen(req->url));

	if (evcpe_callback.load_cb) {
		(*evcpe_callback.load_cb)(enType, enFileType, pcLocalUrl, pcRemoteUrl);
	}
	// TODO: set ParameterKey
	msg->type = EVCPE_MSG_RESPONSE;
	if (!(msg->data = resp = evcpe_upload_response_new())) {
		evcpe_error(__func__, "failed to create "
				"evcpe_upload_response");
		rc = ENOMEM;
		goto finally;
	}
	resp->status = 0;
	rc = 0;

finally:
	return rc;
}

int evcpe_handle_reboot(struct evcpe *cpe,
		struct evcpe_reboot *req,
		struct evcpe_msg *msg)
{
	int rc;
	//struct evcpe_reboot *param;
	struct evcpe_reboot_response *resp;

	// TODO: set ParameterKey
	msg->type = EVCPE_MSG_RESPONSE;
	if (!(msg->data = resp = evcpe_reboot_response_new())) {
		evcpe_error(__func__, "failed to create "
				"evcpe_reboot_response");
		rc = ENOMEM;
		goto finally;
	}
	rc = 0;

finally:
	return rc;
}
//-------------------------

int evcpe_handle_inform_response(struct evcpe *cpe,
		struct evcpe_inform *req, struct evcpe_inform_response *resp)
{
	char *value;
    
	//------------------del by zlin order by gyy
	/*if (resp->max_envelopes != 1) {
		evcpe_error(__func__, "invalid max envelopes: %d", resp->max_envelopes);
		return EPROTO;
	}*/
        
	evcpe_repo_del_event(cpe->repo, "0 BOOTSTRAP");
	evcpe_repo_del_event(cpe->repo, "1 BOOT");
	//---------------------add by zlin
	evcpe_repo_del_event(cpe->repo, "X 000001 LOGOUT");
	evcpe_repo_del_event(cpe->repo, "X 000001 PARAMETER_CHANGE");
	evcpe_repo_del_event(cpe->repo, "X 000001 UPDATE_ASK");
	evcpe_repo_del_event(cpe->repo, "4 VALUE CHANGE");
    
	if (cpe->bootstrap == 1) {
		value = evcpe_ltoa(1);
		if (evcpe_repo_set(cpe->repo, ".ManagementServer.BootStrap", value, strlen(value))) {
			evcpe_error(__func__, "failed to set param: %s", "BootStrap");
			return EPROTO;
		}
	}

	//--------------------add by hb
	if(cpe->icidflag == 1)
	{
		cpe->icidflag = 0;
#ifdef TM_CONFIG_DOWNLOAD
		//写入结构体
		memset(g_stTemp.acICIDLog,0,sizeof(g_stTemp.acICIDLog));
		//写入文件
		evcpe_index_save();
#else
		if (evcpe_repo_set(cpe->repo, ".Config.ICIDOpenLog", "", 0)) {
			evcpe_error(__func__, "failed to set param: %s", "icidflag");
			return EPROTO;
		}
#endif
	}
	//--------------------add by hb
	if (cpe->alarm_flag.FileTransFail == 1)
	{
		cpe->alarm_flag.FileTransFail = 0;
	}
	if(cpe->alarm_flag.SipCallState == 1)
	{
		cpe->alarm_flag.SipCallState = 0;
	}
	if(cpe->alarm_flag.UpdateResult == 1)
	{
		cpe->alarm_flag.UpdateResult = 0;
		g_bUpdateFlag = 2;
	}
	if(cpe->deviceboot == 1)
	{
		cpe->deviceboot = 0;
	}
	//---------------------add by hb
	return 0;
}

int evcpe_dns_entry_resolve(struct evcpe_dns_entry *entry, const char *hostname)
{
	int rc;
	evcpe_debug(__func__, "resolving DNS: %s", hostname);
	if ((rc = evdns_resolve_ipv4(hostname, 0, evcpe_dns_cb, entry)))
		evcpe_error(__func__, "failed to resolve IPv4 address: %s",
				entry->name);
	return rc;
}

int evcpe_dns_add(struct evcpe *cpe, const char *hostname)
{
	int rc;
	struct evcpe_dns_entry *entry;
	struct in_addr addr;

	evcpe_debug(__func__, "adding hostname: %s", hostname);

	if ((rc = evcpe_dns_cache_add(&cpe->dns_cache, hostname, &entry))) {
		evcpe_error(__func__, "failed to create DNS entry: %s", hostname);
		goto finally;
	}
	if (inet_aton(hostname, &addr)) {
		if (!(entry->address = strdup(hostname))) {
			evcpe_error(__func__, "failed to duplicate address");
			rc = ENOMEM;
			evcpe_dns_cache_remove(&cpe->dns_cache, entry);
			goto finally;
		}
	} else {
		if ((rc = evcpe_dns_entry_resolve(entry, entry->name))) {
			evcpe_error(__func__, "failed to resolve entry: %s", hostname);
			goto finally;
		}
		evtimer_set(&entry->ev, evcpe_dns_timer_cb, entry);
		event_base_set(cpe->evbase, &entry->ev);
	}
	rc = 0;

finally:
	return rc;
}

void evcpe_dns_timer_cb(int fd, short event, void *arg)
{
	if (evcpe_dns_entry_resolve(arg, ((struct evcpe_dns_entry *)arg)->name))
		evcpe_error(__func__, "failed to start DNS resolution: %s",
				((struct evcpe_dns_entry *)arg)->name);
}

void evcpe_dns_cb(int result, char type, int count, int ttl,
    void *addresses, void *arg)
{
	struct evcpe_dns_entry *entry = arg;
	const char *address;

	evcpe_debug(__func__, "starting DNS callback");

	switch(result) {
	case DNS_ERR_NONE:
		break;
	case DNS_ERR_SERVERFAILED:
	case DNS_ERR_FORMAT:
	case DNS_ERR_TRUNCATED:
	case DNS_ERR_NOTEXIST:
	case DNS_ERR_NOTIMPL:
	case DNS_ERR_REFUSED:
	case DNS_ERR_TIMEOUT:
	default:
		evcpe_error(__func__, "DNS resolution failed: %d", result);
		goto exception;
	}

	evcpe_debug(__func__, "type: %d, count: %d, ttl: %d: ", type, count, ttl);
	switch (type) {
	case DNS_IPv6_AAAA: {
		// TODO
		break;
	}
	case DNS_IPv4_A: {
		struct in_addr *in_addrs = addresses;
		/* a resolution that's not valid does not help */
		if (ttl < 0) {
			evcpe_error(__func__, "invalid DNS TTL: %d", ttl);
			goto exception;
		}
		if (count == 0) {
			evcpe_error(__func__, "zero DNS address count");
			goto exception;
		} else if (count == 1) {
			address = inet_ntoa(in_addrs[0]);
		} else {
			address = inet_ntoa(in_addrs[rand() % count]);
		}
		if (!(entry->address = strdup(address))) {
			evcpe_error(__func__, "failed to duplicate address string");
			goto exception;
		}
		evcpe_debug(__func__, "address resolved for entry \"%s\": %s",
				entry->name, address);
		entry->tv.tv_sec = ttl;
		break;
	}
	case DNS_PTR:
		/* may get at most one PTR */
		if (count != 1) {
			evcpe_error(__func__, "invalid PTR count: %d", count);
			goto exception;
		}
		address = *(char **)addresses;
		if (evcpe_dns_entry_resolve(entry, address)) {
			evcpe_error(__func__, "failed to start DNS resolve: %s", address);
			goto exception;
		}
		break;
	default:
		evcpe_error(__func__, "unexpected type: %d", type);
		goto exception;
	}
	goto finally;

exception:
	entry->tv.tv_sec = 0;

finally:
	evcpe_info(__func__, "next DNS resolution in %ld seconds: %s",
			entry->tv.tv_sec, entry->name);
	if (event_add(&entry->ev, &entry->tv)) {
		evcpe_error(__func__, "failed to schedule DNS resolution");
	}
}

//------------------add by zlin
void evcpe_callback_reboot(evcpe_void_cb void_cb)
{
	evcpe_callback.reboot_cb = void_cb;
}

void evcpe_callback_load(evcpe_load_cb load_cb)
{
	evcpe_callback.load_cb = load_cb;
}

void evcpe_callback_config(evcpe_config_cb config_cb)
{
	evcpe_callback.config_cb = config_cb;
}

void evcpe_callback_notifyinfo(evcpe_void_cb void_cb)
{
	evcpe_callback.nodifyinfo_cb = void_cb;
}

void evcpe_callback_notifysip(evcpe_void_cb void_cb)
{
	evcpe_callback.nodifysip_cb = void_cb;
}

void evcpe_callback_notifywan(evcpe_void_cb void_cb)
{
	evcpe_callback.nodifywan_cb = void_cb;
}

void evcpe_callback_demo(evcpe_demo_cb demo_cb)
{
	evcpe_callback.demo_cb = demo_cb;
}

void evcpe_callback_demo_room(evcpe_demo_room_cb demo_room_cb)
{
	evcpe_callback.demo_room_cb = demo_room_cb;
}

void evcpe_callback_tr_config(evcpe_tr_config_cb tr_config_cb)
{
	evcpe_callback.tr_config_cb = tr_config_cb;
}
//--------------------
