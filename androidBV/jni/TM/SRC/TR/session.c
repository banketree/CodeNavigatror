// $Id: session.c 12 2011-02-18 04:05:43Z cedric.shih@gmail.com $
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <evhttp.h>

#include "log.h"
#include "util.h"
#include "msg_xml.h"
#include "method.h"

#include "session.h"
#include <LOGIntfLayer.h>
#include <TMInterface.h>

#define PRINTLOC printf("%s(%d)\n", __func__, __LINE__)

static void evcpe_session_http_close_cb(struct evhttp_connection *conn, void *arg);
static void evcpe_session_http_cb(struct evhttp_request *req, void *arg);
static int evcpe_session_send_do(struct evcpe_session *session,
		struct evcpe_msg *msg);

extern int displayFlag ;

//Tue, 26 Feb 2013 02:19:51 GMT
//Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
void evcpe_set_datetime(const char *pcDatatime)
{
	char acDatatime[128];
	char acTime[64];
	char acMonth[64];
	char acYear[64];
	char *tmp;
	int iMonth;
	int iData;
	int iHour;
	int iMin;
	int iYear;
	static int iUpdateFirstFlg = 0;

	time_t   timep;
	struct   tm     *timenow;
	//ʱ���Ƿ���Ҫ����
	time   (&timep);
	timenow   =   localtime(&timep);
	LOG_RUNLOG_DEBUG("LG %d", timenow->tm_year +1900);
	if(timenow->tm_year +1900 > 2012)
	{
		return;
	}

	strcpy(acDatatime,pcDatatime);
	tmp = strtok(acDatatime," ");
	tmp = strtok(NULL," ");
	iData = atoi(tmp);
	tmp = strtok(NULL," ");
	strcpy(acMonth,tmp);
	if(strcmp(acMonth,"Jan") == 0)
	{
		iMonth = 1;
	}
	else if(strcmp(acMonth,"Feb") == 0)
	{
		iMonth = 2;
	}
	else if(strcmp(acMonth,"Mar") == 0)
	{
		iMonth = 3;
	}
	else if(strcmp(acMonth,"Apr") == 0)
	{
		iMonth = 4;
	}
	else if(strcmp(acMonth,"May") == 0)
	{
		iMonth = 5;
	}
	else if(strcmp(acMonth,"Jun") == 0)
	{
		iMonth = 6;
	}
	else if(strcmp(acMonth,"Jul") == 0)
	{
		iMonth = 7;
	}
	else if(strcmp(acMonth,"Aug") == 0)
	{
		iMonth = 8;
	}
	else if(strcmp(acMonth,"Sep") == 0)
	{
		iMonth = 9;
	}
	else if(strcmp(acMonth,"Oct") == 0)
	{
		iMonth = 10;
	}
	else if(strcmp(acMonth,"Nov") == 0)
	{
		iMonth = 11;
	}
	else if(strcmp(acMonth,"Dec") == 0)
	{
		iMonth = 12;
	}
	tmp = strtok(NULL," ");
	strcpy(acYear,tmp);
	tmp = strtok(NULL," ");
	strcpy(acTime,tmp);
	tmp = strtok(acTime,":");
	iHour = atoi(tmp);
	tmp = strtok(NULL,":");
	iMin = atoi(tmp);
	sprintf(acDatatime,"busybox date %02d%02d%02d%02d%s",iMonth,iData,iHour,iMin,acYear);
	setenv("TZ", "CST", 1);
//	system("date");
	system(acDatatime);
	setenv("TZ", "CST-8", 1);
	//�޸���־ʱ��
	if(iUpdateFirstFlg == 0)
	{
		iUpdateFirstFlg++;
		LOG_RUNLOG_DEBUG("LG evcpe_set_datetime");
		LOGUploadResPrepross();
		LOGUploadExec(0);
	}
//	system("date");
}

/*�Ự�ر�*/
void evcpe_session_close(struct evcpe_session *session, int code)
{
	evcpe_info(__func__, "closing CWMP session");
	evhttp_connection_set_closecb(session->conn, NULL, NULL);
	if (session->close_cb)
		(*session->close_cb)(session, code, session->close_cbarg);
}

void evcpe_session_http_close_cb(struct evhttp_connection *conn, void *arg)
{
	evcpe_error(__func__, "HTTP connection closed");
	//--------------------del by zlin
	//evcpe_session_close(arg, ECONNRESET);
	//--------------------
}

/*�Ự��Ӧ���Ľ�������*/
int evcpe_session_handle_incoming(struct evcpe_session *session,
		struct evbuffer *buffer)
{
	int rc;
	struct evcpe_msg *msg, *req;
       /*�����µ���Ϣ�ṹ*/
	if (!(msg = evcpe_msg_new())) {
		evcpe_error(__func__, "failed to create evcpe_msg");
		rc = ENOMEM;
		goto finally;
	}
       /*��xml��ʽ�Ļ�Ӧ�����н������ݲ����msg�ṹ*/
	if ((rc = evcpe_msg_from_xml(msg, buffer))) {
		evcpe_error(__func__, "failed to unmarshal response");
		goto finally;
	}
       /*holder_requests״̬��ֵ*/
	session->hold_requests = msg->hold_requests;
       /*������Ϣ���ͽ��д���*/
	switch (msg->type) {
	case EVCPE_MSG_REQUEST:/*�������ͱ���*/
		TAILQ_INSERT_TAIL(&session->req_in, msg, entry);
		(*session->cb)(session, msg->type, msg->method_type,
				msg->data, NULL, session->cbarg);
		break;
	case EVCPE_MSG_RESPONSE:/*��Ӧ�ʹ������ͱ���*/
		if (!(req = TAILQ_FIRST(&session->req_out))) {
			evcpe_error(__func__, "no pending CPE request");
			rc = EPROTO;
			goto finally;
		}
		if (msg->type == EVCPE_MSG_RESPONSE
				&& msg->method_type != req->method_type) {
			evcpe_error(__func__, "method of request/response doesn't match: "
					"%d != %d", req->method_type, msg->method_type);
			rc = EPROTO;
			goto finally;
		}
		/*cb����Ϊevcpe_session_message_cb*/
		(*session->cb)(session, msg->type, msg->method_type,
				req, msg->data, session->cbarg);
		TAILQ_REMOVE(&session->req_out, req, entry);
		evcpe_msg_free(req);
		evcpe_msg_free(msg);
		break;
	case EVCPE_MSG_FAULT:
		if (!(req = TAILQ_FIRST(&session->req_out))) {
			evcpe_error(__func__, "no pending CPE request");
			rc = EPROTO;
			goto finally;
		}
		if (req->method_type == EVCPE_INFORM)
		{
			/*cb����Ϊevcpe_session_message_cb*/
			TAILQ_REMOVE(&session->req_out, req, entry);
		}
		else
		{
			/*cb����Ϊevcpe_session_message_cb*/
			(*session->cb)(session, msg->type, msg->method_type,
					req, msg->data, session->cbarg);
			/*cb����Ϊevcpe_session_message_cb*/
			TAILQ_REMOVE(&session->req_out, req, entry);
			evcpe_msg_free(req);
			evcpe_msg_free(msg);
		}
		break;
	default:
		evcpe_error(__func__, "unexpected message type: %d", msg->type);
		rc = EINVAL;
		goto finally;
	}
	rc = 0;

finally:
	return rc;
}

/*http���մ�����*/
void evcpe_session_http_cb(struct evhttp_request *http_req, void *arg)
{
	int rc;
	const char *cookies;
	const char *server_time;
	struct evcpe_msg *msg;
	struct evcpe_session *session = arg;
	//---------------add by zlin
	struct evcpe *cpe = session->cbarg;
	//evhttp_connection_fail
	if (0 == http_req) {
		evcpe_info(__func__, "HTTP connection fail");
		rc = ETIMEDOUT;
		LOG_RUNLOG_DEBUG("TM HTTP connection fail");
#if _TM_TYPE_ == _TR069_
		TMSetLinkStatus(LINK_STATUS_OFFLINE);
#endif
		goto close;
	}
	//---------------
	/*http�������ж�*/
	if (0 == http_req->response_code) {
		evcpe_info(__func__, "session timed out");
		cpe->timeoutNum ++;	//add by zlin
		rc = ETIMEDOUT;
		LOG_RUNLOG_DEBUG("TM session timed out");
#if _TM_TYPE_ == _TR069_
		TMSetLinkStatus(LINK_STATUS_OFFLINE);
#endif
		goto close;
	}

	evcpe_info(__func__, "HTTP response code: %d", http_req->response_code);
	evcpe_debug(__func__, "HTTP response content: %.*s",
			EVBUFFER_LENGTH(http_req->input_buffer),
			EVBUFFER_DATA(http_req->input_buffer));

	//add by hb
	//����ʱ��
	server_time = evhttp_find_header(http_req->input_headers, "Date");
	if(server_time)
	{
		//ʱ����������� Tue, 26 Feb 2013 02:19:51 GMT
		evcpe_set_datetime(server_time);
	}
	//===================================

	/*����cookieͷ������*/
	if ((cookies = evhttp_find_header(http_req->input_headers, "Set-Cookie"))
			&& (rc = evcpe_cookies_set_from_header(
					&session->cookies, cookies))) {
		evcpe_error(__func__, "failed to set cookies: %s", cookies);
		goto close;
	}
	//-----------------add by zlin
	if (cpe->timeoutNum > 5 && !(rc = evcpe_repo_get_event(cpe->repo, "1 BOOT"))) {
		cpe->boot_flag = 1;
#if _TM_TYPE_ == _TR069_
		TMSetLinkStatus(LINK_STATUS_OFFLINE);
#endif
	}

	if(http_req->input_buffer->totallen > 0)
	{
		cpe->timeoutNum = 0;
		LOG_RUNLOG_DEBUG("TM LINK_STATUS_ONLINE");
#if _TM_TYPE_ == _TR069_
		TMSetLinkStatus(LINK_STATUS_ONLINE);
#endif
	}

	if (displayFlag == TRUE)
	{
		printf("================================================\n");
		LOG_RUNLOG_DEBUG("[TR069_MESSAGE] -Recv[%d] = %s \n", http_req->input_buffer->totallen, http_req->input_buffer->buffer);
		printf("================================================\n");
	}
	//-----------------

	/*�յ����Ļ����������жϲ�����http������Ϣ*/
	if (EVBUFFER_LENGTH(http_req->input_buffer) &&
			(rc = evcpe_session_handle_incoming(
					session, http_req->input_buffer))) {
		evcpe_error(__func__, "failed to handle incoming data");
		goto close;
	}
	/*evcpe_session_handle_incoming�����ĺ󣬻�ȡ���ķ�����Ϣ���е�msg��Ϣ׼������*/
	if ((msg = TAILQ_FIRST(&session->res_pending))) {
		/*�ӻ�Ӧ�����Ͷ����з���msg��Ϣ*/
		if ((rc = evcpe_session_send_do(session, msg))) {
			evcpe_error(__func__, "failed to response ACS request");
			goto close;
		}
		TAILQ_REMOVE(&session->res_pending, msg, entry);
		evcpe_msg_free(msg);
	} else if (session->hold_requests) {
		/*����hold_requests������msg��Ϣ*/
		if ((rc = evcpe_session_send_do(session, NULL))) {
			evcpe_error(__func__, "failed to send empty HTTP request");
			goto close;
		}
	} else if ((msg = TAILQ_FIRST(&session->req_pending))) {
		/*����������Ͷ����з���msg��Ϣ*/
		if ((rc = evcpe_session_send_do(session, msg))) {
			evcpe_error(__func__, "failed to send CPE request");
			goto close;
		}
		TAILQ_REMOVE(&session->req_pending, msg, entry);
		TAILQ_INSERT_TAIL(&session->req_out, msg, entry);
	} else if (!EVBUFFER_LENGTH(http_req->input_buffer)) {
		/*���ܵ��Ļ���������Ϊ��*/
		evcpe_info(__func__, "session termination criteria are met");
		goto close;
	} else {
		/*���ʹ����Ӧ��Ϣ*/
		if ((rc = evcpe_session_send_do(session, NULL))) {
			evcpe_error(__func__, "failed to send empty HTTP request");
			goto close;
		}
	}
	return;
/*�쳣��Ҫ�رջỰ*/
close:
	evcpe_session_close(session, rc);
}

struct evcpe_session *evcpe_session_new(struct evhttp_connection *conn,
		struct evcpe_url *acs, evcpe_session_cb cb, void *cbarg)
{
	struct evcpe_session *session;

	evcpe_debug(__func__, "constructing evcpe_session");

	if (!(session = calloc(1, sizeof(struct evcpe_session)))) {
		evcpe_error(__func__, "failed to calloc evcpe_session");
		return NULL;
	}
	RB_INIT(&session->cookies);
	TAILQ_INIT(&session->req_in);
	TAILQ_INIT(&session->req_out);
	TAILQ_INIT(&session->req_pending);
	TAILQ_INIT(&session->res_pending);
	session->conn = conn;
	session->acs = acs;
	session->cb = cb;
	session->cbarg = cbarg;

	return session;
}

void evcpe_session_free(struct evcpe_session *session)
{
	if (!session) return;

	evcpe_debug(__func__, "destructing evcpe_session");

	evcpe_cookies_clear(&session->cookies);
	evcpe_msg_queue_clear(&session->req_in);
	evcpe_msg_queue_clear(&session->req_out);
	evcpe_msg_queue_clear(&session->req_pending);
	evcpe_msg_queue_clear(&session->res_pending);
	if (session->conn) {
		evhttp_connection_set_closecb(session->conn, NULL, NULL);
	}
	free(session);
}

void evcpe_session_set_close_cb(struct evcpe_session *session,
		evcpe_session_close_cb close_cb, void *cbarg)
{
	session->close_cb = close_cb;
	session->close_cbarg = cbarg;
}

/*inform֪ͨ�Ự����*/
int evcpe_session_start(struct evcpe_session *session)
{
	int rc;
	struct evcpe_msg *req;
	/*�ȴ���Ϣ���д������򷵻�*/
	if (!(req = TAILQ_FIRST(&session->req_pending))) {
		evcpe_error(__func__, "no pending CPE request");
		rc = EINVAL;
		goto finally;
	}
	/*��֪ͨ�¼�����*/
	if (req->method_type != EVCPE_INFORM) {
		evcpe_error(__func__, "first CPE request must be an inform");
		rc = EINVAL;
		goto finally;
	}
	req = TAILQ_FIRST(&session->req_pending);
	/*����evcpe_session_send_do ������װ���ģ�׼������inform����*/
	if ((rc = evcpe_session_send_do(session, req))) {
		evcpe_error(__func__, "failed to send first request");
		goto finally;
	}
	TAILQ_REMOVE(&session->req_pending, req, entry);
	TAILQ_INSERT_TAIL(&session->req_out, req, entry);
	rc = 0;
	return rc;		//add by zlin

finally:
	TAILQ_REMOVE(&session->req_pending, req, entry);//add by zlin
	return rc;
}

int evcpe_session_send(struct evcpe_session *session, struct evcpe_msg *msg)
{
	int rc;
	struct evcpe_msg *req;

	if (!session || !msg) return EINVAL;

	switch (msg->type) {
	case EVCPE_MSG_REQUEST:
		TAILQ_INSERT_TAIL(&session->req_pending, msg, entry);
		break;
	case EVCPE_MSG_RESPONSE:
	case EVCPE_MSG_FAULT:
		if (!(req = TAILQ_FIRST(&session->req_in))) {
			evcpe_error(__func__, "no pending ACS request");
			rc = -1;
			goto finally;
		} else if (req->method_type != msg->method_type) {
			evcpe_error(__func__, "method type mismatch: %s != %s",
					evcpe_method_type_to_str(req->method_type),
					evcpe_method_type_to_str(msg->method_type));
			rc = -1;
			goto finally;
		}
		if (!(msg->session = strdup(req->session))) {
			evcpe_error(__func__, "failed to duplicate session ID");
			rc = ENOMEM;
			goto finally;
		}
		TAILQ_INSERT_TAIL(&session->res_pending, msg, entry);
		TAILQ_REMOVE(&session->req_in, req, entry);
		evcpe_msg_free(req);
		break;
	default:
		evcpe_error(__func__, "unexpected message type: %d", msg->type);
		rc = EINVAL;
		goto finally;
	}
	rc = 0;

finally:
	return rc;
}

static int evcpe_session_add_header(struct evkeyvalq *keyvalq,
		const char *key, const char *value)
{
	evcpe_debug(__func__, "adding HTTP header: %s => %s", key, value);
	return evhttp_add_header(keyvalq, key, value);
}

/*�Ự������װ�ͷ��ͺ���*/
int evcpe_session_send_do(struct evcpe_session *session, struct evcpe_msg *msg)
{
	int rc, len;
	char buffer[513];
	struct evhttp_request *req;
	struct evcpe_cookie *cookie;

//#ifndef _YK_PC_
//	//�����Ƿ����
//	if(NMNetTest() != 0)
//	{
////		printf("net isn't ok, tr session not to do , %s ,%d\n",__FILE__,__LINE__);
//		rc = ENOMEM;
//		goto finally;
//	}
////	printf("net is ok\n");
//#endif

	if (msg)
		evcpe_info(__func__, "sending CWMP %s message in HTTP request",
				evcpe_msg_type_to_str(msg->type));
	else
		evcpe_info(__func__, "sending empty HTTP request");
	/*����һ���µ�http������󣬲�����http�ص�������evcpe_session_http_cb*/
	if (!(req = evhttp_request_new(
			evcpe_session_http_cb, session))) {
		evcpe_error(__func__, "failed to create evhttp_connection");
		rc = ENOMEM;
		goto finally;
	}
	/*����Ϣ���ݽṹת����xml��ʽ���ݱ���*/
	if (msg && msg->data && (rc = evcpe_msg_to_xml(msg,
			req->output_buffer))) {
		evcpe_error(__func__, "failed to create SOAP message");
		//evhttp_request_free(req);//del by zlin
		goto finally;
	}
	if (displayFlag == TRUE)
	{
		printf("================================================\n");
		LOG_RUNLOG_DEBUG("[TR069_MESSAGE] -Send = %s \n", req->output_buffer->buffer);
		printf("================================================\n");
	}
	req->major = 1;
	req->minor = 1;
	snprintf(buffer, sizeof(buffer), "%s:%d",
			session->acs->host, session->acs->port);
	/*����host��httpͷ*/
	if ((rc = evcpe_session_add_header(req->output_headers,
			"Host", buffer))) {

		evcpe_error(__func__, "failed to add header: Host=\"%s\"", buffer);
		//evhttp_request_free(req);//del by zlin
		goto finally;
	}

	/*�ж�rb_root���лỰcookie�Ƿ�Ϊ��*/
	if (!RB_EMPTY(&session->cookies)) {
		len = 0;
              /*��������cookie*/
		RB_FOREACH(cookie, evcpe_cookies, &session->cookies) {
			len += snprintf(buffer + len, sizeof(buffer) - len, "%s=%s; ",
					cookie->name, cookie->value);
		}
		if (len - 2 < sizeof(buffer))
			buffer[len - 2] = '\0';
		/*����cookie��httpͷ*/
		if ((rc = evcpe_session_add_header(req->output_headers,
				"Cookie", buffer))) {
			evcpe_error(__func__, "failed to add header: Cookie=\"%s\"", buffer);
			//evhttp_request_free(req);//del by zlin
			goto finally;
		}
	}

	/*����SOAPAction��httpͷ*/
	if (msg && msg->data && (rc = evcpe_session_add_header(req->output_headers,
			"SOAPAction", ""))) {
		evcpe_error(__func__, "failed to add header: SOAPAction=\"\"");
		//evhttp_request_free(req);//del by zlin
		goto finally;
	}

	if ((rc = evcpe_session_add_header(req->output_headers,
			"User-Agent", "evcpe-"EVCPE_VERSION))) {
		evcpe_error(__func__, "failed to add header: "
				"User-Agent=\"evcpe-"EVCPE_VERSION"\"");
		//evhttp_request_free(req);//del by zlin
		goto finally;
	}

	if ((rc = evcpe_session_add_header(req->output_headers,
			"Content-Type", "text/xml"))) {
		evcpe_error(__func__, "failed to add header: Content-Type=text/xml");
		//evhttp_request_free(req);//del by zlin
		goto finally;
	}

	evcpe_debug(__func__, "HTTP request content: %.*s",
			EVBUFFER_LENGTH(req->output_buffer),
			EVBUFFER_DATA(req->output_buffer));
	evcpe_info(__func__, "making HTTP request");
	/*����post����*/
	if ((rc = evhttp_make_request(session->conn, req,
			EVHTTP_REQ_POST, session->acs->uri))) {
		evcpe_error(__func__, "failed to make request");
		LOG_RUNLOG_DEBUG("TM failed to make request");
#if _TM_TYPE_ == _TR069_
		TMSetLinkStatus(LINK_STATUS_OFFLINE);
#endif
		//evhttp_request_free(req);//del by zlin
		goto finally;
	}
	//���ùرջص���������ʱ����Զ�����Ӧ����ת��evcpe_session_http_cb����response*/
	evhttp_connection_set_closecb(session->conn, evcpe_session_http_close_cb,
			session);
	rc = 0;

finally:
	return rc;
}
