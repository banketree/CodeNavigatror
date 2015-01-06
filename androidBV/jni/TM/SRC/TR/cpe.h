// $Id: cpe.h 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
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

#ifndef EVCPE_CPE_H_
#define EVCPE_CPE_H_

#include "handler.h"
#include "dns_cache.h"
#include "method.h"
#include "xml.h"
#include "repo.h"
#include "url.h"
#include "session.h"
#include "persister.h"
#include "transfercomplete.h"

#include "evcpe.h"

enum evcpe_auth_type {
	EVCPE_AUTH_NONE,
	EVCPE_AUTH_BASIC,
	EVCPE_AUTH_DIGEST
};

enum TM_TRANSFER_TYPE_EN{
	TM_REMOTE_DOWNLOAD=0, /**<!Download type.*/
	TM_REMOTE_UPLOAD      /**<!Upload type.*/
};

/**@brief Transfer file type.*/
enum TM_TRANSFER_FILE_TYPE_EN{
	TM_TYPE_EVENT_LOG=0,
	TM_TYPE_SYSTEM_LOG,
	TM_TYPE_CONFIG,
	TM_TYPE_TR_MODEL,
	TM_TYPE_RINGING,
	TM_TYPE_UPDATE,
	TM_TYPE_ADVERT,
	TM_TYPE_OTHER
};

typedef struct evcpe_alarm_state {
	char ImsRegisterFail[1];
	char SipCallFail[1];
	char IfBoardComFail[1];
	char FileTransFail[1];
	char UpdateResult[1];
	char SouthInterface[10];
	char SipCallState[1];
}EVCPE_ALARM_STATE_ST;

struct evcpe_alarm_flag {
	char ImsRegisterFail;
	char SipCallFail;
	char IfBoardComFail;
	char FileTransFail;
	char UpdateResult;
	char SouthInterface;
	char SipCallState;
};

typedef struct {
	int iIndex ;
	char acICIDLog[1024];
}EVCPE_TEMP_ST;

struct evcpe {
	struct event_base *evbase;

	evcpe_request_cb cb;
	evcpe_error_cb error_cb;
	void *cbarg;

	struct evcpe_dns_cache dns_cache;

	struct evcpe_repo *repo;
//	struct evcpe_persister *persist;

	struct evcpe_device_id id;
//	struct evcpe_event_list events;

	enum evcpe_auth_type acs_auth;
	struct evcpe_url *acs_url;
	const char *acs_username;
	const char *acs_password;
	unsigned int acs_timeout;

	struct evcpe_url *proxy_url;
	const char *proxy_username;
	const char *proxy_password;

	struct evcpe_url *creq_url;
	const char *creq_username;
	const char *creq_password;
	unsigned int creq_interval;
	time_t creq_last;

	struct evhttp *http;

	struct evcpe_session *session;

	unsigned int retry_count;
	struct event retry_ev;
	struct timeval retry_tv;

	struct event periodic_ev;
	struct timeval periodic_tv;

	struct evcpe_alarm_state alarm_state;
	struct evcpe_alarm_flag alarm_flag;
	char alarmflag;
	//add by hb
	unsigned int icidflag;
	unsigned int deviceboot;
	//====================
	char parameter_change;
	char update_ask;
	char logout;
	unsigned int timeoutNum;
	char boot_flag;
	struct evcpe_transfer transfer;
	char bootstrap;
};

typedef void (*evcpe_void_cb)(void);
typedef void (*evcpe_load_cb)(enum TM_TRANSFER_TYPE_EN enType, enum TM_TRANSFER_FILE_TYPE_EN enFileType, const char* pcLocalUrl, const char* pcRemoteUrl);
typedef void (*evcpe_demo_cb)(const char* acPhoneNumber, const char* cVoipEnable);
typedef void (*evcpe_config_cb)(const char *pcConfigUrl,const char* pcUserName, const char* pcPasswd);
typedef void (*evcpe_demo_room_cb)(const char* pcRoomNum);
typedef void (*evcpe_tr_config_cb)(const char *pcConfigUrl,const char *pcFileName);
typedef void (*evcpe_upgrade_cb)(const char *pcConfigUrl,const char *pcFileName);

struct TR_CALLBACK_ST
{
    evcpe_void_cb reboot_cb;
    evcpe_load_cb load_cb;
    evcpe_void_cb nodifyinfo_cb;
    evcpe_void_cb nodifysip_cb;
    evcpe_void_cb nodifywan_cb;
    evcpe_demo_cb demo_cb;
    evcpe_demo_room_cb demo_room_cb;
    evcpe_config_cb config_cb;
    evcpe_tr_config_cb tr_config_cb;
    evcpe_upgrade_cb upgrade_cb;
};

struct evcpe *evcpe_new(struct event_base *evbase,
		evcpe_request_cb cb, evcpe_error_cb error_cb, void *cbarg);

void evcpe_free(struct evcpe *cpe);

int evcpe_set(struct evcpe *cpe, struct evcpe_repo *repo);

int evcpe_bind_http(struct evcpe *cpe, const char *address, u_short port,
		const char *prefix);

int evcpe_is_attached(struct evcpe *cpe);

int evcpe_make_request(struct evcpe *cpe, const char *acs_url,
		struct evcpe_request *req);

int evcpe_set_acs(struct evcpe *cpe, const char *address, u_short port,
		const char *uri);

int evcpe_set_auth(struct evcpe *cpe, enum evcpe_auth_type type,
		const char *username, const char *password);

int evcpe_start(struct evcpe *cpe);

//----------------add by zlin
void evcpe_callback_reboot(evcpe_void_cb void_cb);

void evcpe_callback_load(evcpe_load_cb load_cb);

void evcpe_callback_notifyinfo(evcpe_void_cb void_cb);

void evcpe_callback_notifysip(evcpe_void_cb void_cb);

void evcpe_callback_notifywan(evcpe_void_cb void_cb);

void evcpe_callback_demo(evcpe_demo_cb demo_cb);

void evcpe_callback_config(evcpe_config_cb config_cb);

void evcpe_callback_tr_config(evcpe_tr_config_cb tr_config_cb);

int evcpe_boot(struct evcpe *cpe, struct evcpe_repo *repo, char bootflag);
//-----------------

//int evcpe_add_event(struct evcpe *cpe,
//		const char *event_code, const char *command_key);
//
//int evcpe_remove_event(struct evcpe *cpe,
//		const char *event_code);

#endif /* EVCPE_CPE_H_ */
