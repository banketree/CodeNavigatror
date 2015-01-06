// $Id: evcpe.h 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
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

#ifndef EVCPE_H_
#define EVCPE_H_

#ifdef __GNUC__
#define EVCPE_CHKFMT(a,b) __attribute__((format(printf, a, b)))
#else
#define EVCPE_CHKFMT(a,b)
#endif

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <evhttp.h>

#include "cpe.h"
#include "repo.h"
#include "class_xml.h"
#include "obj_xml.h"
#include "persister.h"
#include "evcpe-config.h"
#include "TRManage.h"


#include "INC/TMProcotol.h"

#define EVCPE_IANA_CWMP_PORT 7547

#ifdef XNMP_ENV
#define TM_DATA_PATH			"../../TMData.xml"
#define TM_MODEL_PATH			"../../TMModel.xml"
#define LG_LOCAL_PATH			"../../../LogDir/Log/"
#endif

enum evcpe_list_event{
	EVCPE_EVENT_LOGOUT,
	EVCPE_EVENT_ALARM,
	EVCPE_EVENT_PARAMETER,
	EVCPE_EVENT_UPDATE
};

enum evcpe_log_level {
	EVCPE_LOG_TRACE,
	EVCPE_LOG_DEBUG,
	EVCPE_LOG_INFO,
	EVCPE_LOG_WARN,
	EVCPE_LOG_ERROR,
	EVCPE_LOG_FATAL
};

typedef struct {
	struct event_base *evbase;
	struct evcpe *cpe;
	struct evcpe_class *cls;
	struct evcpe_obj *obj;
	struct evcpe_persister *persist;
	struct evcpe_repo *repo;
} TR_EVCPE_ST;

extern TR_EVCPE_ST g_tr_evcpe_st;

typedef void (*evcpe_logger)(enum evcpe_log_level level, const char *func,
		const char *fmt, va_list ap, void *cbarg);

void evcpe_file_logger(enum evcpe_log_level level, const char *func,
		const char *fmt, va_list ap, void *cbarg);

int evcpe_add_logger(const char *name,
		enum evcpe_log_level min, enum evcpe_log_level max,
		const char *prefix, evcpe_logger logger, void *cbarg);

int evcpe_remove_logger(const char *name);

int evcpe_set_log_level(enum evcpe_log_level min, enum evcpe_log_level max);

void evcpe_set_log_out(FILE *out, FILE *err);

void evcpe_trace(const char *func, const char *fmt, ...) EVCPE_CHKFMT(2,3);

void evcpe_debug(const char *func, const char *fmt, ...) EVCPE_CHKFMT(2,3);

void evcpe_info(const char *func, const char *fmt, ...) EVCPE_CHKFMT(2,3);

void evcpe_warn(const char *func, const char *fmt, ...) EVCPE_CHKFMT(2,3);

void evcpe_error(const char *func, const char *fmt, ...) EVCPE_CHKFMT(2,3);

void evcpe_fatal(const char *func, const char *fmt, ...) EVCPE_CHKFMT(2,3);

void *evcpe_startup(void *arg);

void evcpe_event_change(enum evcpe_list_event code);

void evcpe_event_transfer(int result, struct tm *starttime, struct tm *completetime);

#if _TM_TYPE_ == _TR069_
void TMUpdateAlarmState(TM_ALARM_ID_EN enModule, const char* cState);
#endif
    
#endif /* EVCPE_H_ */
