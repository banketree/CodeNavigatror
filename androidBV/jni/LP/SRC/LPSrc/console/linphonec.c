/****************************************************************************
 *
 *  $Id: linphonec.c,v 1.57 2007/11/14 13:40:27 smorlat Exp $
 *
 *  Copyright (C) 2006  Sandro Santilli <strk@keybit.net>
 *  Copyright (C) 2002  Florian Winterstein <flox@gmx.net>
 *  Copyright (C) 2000  Simon MORLAT <simon.morlat@free.fr>
 *
****************************************************************************
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ****************************************************************************/


#ifdef _YK_IMX27_AV_
#include "Imx27WdTask.h"
#endif
#include <string.h>
#ifndef _WIN32_WCE
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "private.h" /*coreapi/private.h, needed for LINPHONE_VERSION */
#endif /*_WIN32_WCE*/
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>

#include <linphonecore.h>

#include "linphonec.h"

#ifdef WIN32
#include <ws2tcpip.h>
#include <ctype.h>
#ifndef _WIN32_WCE
#include <conio.h>
#endif /*_WIN32_WCE*/
#else
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/stat.h>
#endif

#if defined(_WIN32_WCE)

#if !defined(PATH_MAX)
#define PATH_MAX 256
#endif /*PATH_MAX*/

#if !defined(strdup)
#define strdup _strdup
#endif /*strdup*/

#endif /*_WIN32_WCE*/

#ifdef HAVE_GETTEXT
#include <libintl.h>
#ifndef _
#define _(String) gettext(String)
#endif
#else
#define _(something)	(something)
#endif

#ifndef PACKAGE_DIR
#define PACKAGE_DIR ""
#endif

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

// modify by xuqd
#ifdef ANDROID
#include <android/log.h>
#endif

//add by chensq 20120531
#include "LOGIntfLayer.h"
#include "LPIntfLayer.h"


/***************************************************************************
 *
 *  Types
 *
 ***************************************************************************/

typedef struct {
	LinphoneAuthInfo *elem[MAX_PENDING_AUTH];
	int nitems;
} LPC_AUTH_STACK;

/***************************************************************************
 *
 *  Forward declarations
 *
 ***************************************************************************/

char *lpc_strip_blanks(char *input);

static int handle_configfile_migration(void);
#if !defined(_WIN32_WCE)
static int copy_file(const char *from, const char *to);
#endif /*_WIN32_WCE*/
static int linphonec_parse_cmdline(int argc, char **argv);
static int linphonec_init(int argc, char **argv);
static int linphonec_main_loop (LinphoneCore * opm, char * sipAddr);
static int linphonec_idle_call (void);
#ifdef HAVE_READLINE
static int linphonec_initialize_readline(void);
static int linphonec_finish_readline();
static char **linephonec_readline_completion(const char *text,
	int start, int end);
#endif

/* These are callback for linphone core */
static void linphonec_prompt_for_auth(LinphoneCore *lc, const char *realm,
	const char *username);
static void linphonec_display_refer (LinphoneCore * lc, const char *refer_to);
static void linphonec_display_something (LinphoneCore * lc, const char *something);
static void linphonec_display_url (LinphoneCore * lc, const char *something, const char *url);
static void linphonec_display_warning (LinphoneCore * lc, const char *something);
static void linphonec_notify_received(LinphoneCore *lc, LinphoneCall *call, const char *from,const char *event);

static void linphonec_notify_presence_received(LinphoneCore *lc,LinphoneFriend *fid);
static void linphonec_new_unknown_subscriber(LinphoneCore *lc,
		LinphoneFriend *lf, const char *url);

static void linphonec_text_received(LinphoneCore *lc, LinphoneChatRoom *cr,
		const LinphoneAddress *from, const char *msg);
static void linphonec_display_status (LinphoneCore * lc, const char *something);
static void linphonec_dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf);
static void print_prompt(LinphoneCore *opm);
void linphonec_out(const char *fmt,...);

/***************************************************************************
 *
 * Global variables
 *
 ***************************************************************************/

LinphoneCore *linphonec;


//--------------------------------------------------------------------------------
//add by chensq 20120520
static pthread_t linphone_thr;
static pthread_mutex_t start_lp_mutex;
static pthread_cond_t start_lp_cond;
static pthread_mutex_t config_file_mutex;

static CB_CALL_STATE_CHANGE             pfCallStateChange    = NULL;
static CB_REG_STATE_CHANGE              pfRegStateChange     = NULL;
static CB_REG_OK_FAILED_NOTIFY          pfRegOkFailedNotify  = NULL;
static CB_DTMF_RECV                     pfDtmfRecv           = NULL;
static CB_CALL_FAILED                   pfCallFailed         = NULL;
static CB_MSG_HANDLE_SUPPORT            pfMsgHandleSupport   = NULL;
static CB_DEFAULT_REG                   pfDefaultReg         = NULL;
static CB_MESSAGE_RECV                  pfMessageRecv        = NULL; //add by chensq 20121022 for gyy
CB_EVENT_PROCESS                        pfEventProcess       = NULL; //add by chensq 20121112

#ifdef HINT_MUSIC
static CB_HINT_MUSIC_DONE               pfHintMusicDone      = NULL;
#endif
//--------------------------------------------------------------------------------



FILE *mylogfile;
#ifdef HAVE_READLINE
static char *histfile_name=NULL;
static char last_in_history[256];
#endif
//auto answer (-a) option
static bool_t auto_answer=FALSE;
static bool_t answer_call=FALSE;
static bool_t vcap_enabled=FALSE;
static bool_t display_enabled=FALSE;
static bool_t network_monitor_enabled=TRUE;
static bool_t preview_enabled=FALSE;
static bool_t show_general_state=FALSE;
static bool_t unix_socket=FALSE;
static bool_t linphonec_running=TRUE;
LPC_AUTH_STACK auth_stack;
static int trace_level = 0;
static char *logfile_name = NULL;
static char configfile_name[PATH_MAX];
static const char *factory_configfile_name=NULL;
static char *sipAddr = NULL; /* for autocall */
static int window_id = 0; /* 0=standalone window, or window id for embedding video */
#if !defined(_WIN32_WCE)
static ortp_pipe_t client_sock=ORTP_PIPE_INVALID;
#endif /*_WIN32_WCE*/
char prompt[PROMPT_MAX_LEN];
#if !defined(_WIN32_WCE)
static ortp_thread_t pipe_reader_th;
static bool_t pipe_reader_run=FALSE;
#endif /*_WIN32_WCE*/
#if !defined(_WIN32_WCE)
static ortp_pipe_t server_sock;
#endif /*_WIN32_WCE*/

bool_t linphonec_camera_enabled=TRUE;


//--------------------------------------------------------------------------------
//add by chensq 20120520
#ifdef HINT_MUSIC
static void linphonec_outgoing_hint_music_done(LinphoneCore *lc, LinphoneCall *call)
{
	if (pfHintMusicDone)
	{
		pfHintMusicDone(lc, call);
	}
}
#endif
//--------------------------------------------------------------------------------


void linphonec_send_dtmf2(char value)
{
	if(linphonec == NULL)
	{
		LOG_RUNLOG_WARN("LP send dtmf error because of linphonec is null");
		return;
	}
	LinphoneCall *call=linphone_core_get_current_call (linphonec);
	if(call == NULL)
	{
		LOG_RUNLOG_WARN("LP send dtmf error because of call is null");
		return;
	}
	LinphoneCallState call_state=LinphoneCallIdle;
	call_state=linphone_call_get_state(call);
	LOG_RUNLOG_DEBUG("LP call_state %d", call_state);

//	if(LinphoneCallConnected == call_state || LinphoneCallStreamsRunning == call_state)
//	{
		LOG_RUNLOG_DEBUG("LP send dtmf %c", value);
		linphonec_out("linphonec_send_dtmf:args:%c\n", value);
		linphone_core_send_dtmf(linphonec, value);
//	}
//	else
//	{
//		LOG_RUNLOG_WARN("LP send dtmf error because of call state is not LinphoneCallConnected");
//	}
}


//add by chensq 20120724 0:unregisted 1:registed
int linphonec_is_registered(void)
{
	LinphoneProxyConfig *cfg=NULL;
	linphone_core_get_default_proxy(linphonec,&cfg);
	if (cfg)
	{
		if(!linphone_proxy_config_is_registered(cfg))
		{
			//linphonec_out("default proxy didn't registered\n");
			return 0;
		}else{
			//linphonec_out("default proxy already registered\n");
			return 1;
		}
	}
	else
	{
		//linphonec_out("we do not have a default proxy\n");
		return 0;
	}
}



//add by chensq 20120724
void linphonec_set_registered(void)
{
	LinphoneProxyConfig *cfg=NULL;
	linphone_core_get_default_proxy(linphonec,&cfg);
	if (cfg)
	{
//		if(!linphone_proxy_config_is_registered(cfg))
//		{
			linphone_proxy_config_enable_register(cfg,TRUE);
			cfg->registered=FALSE;
			//linphone_proxy_config_done(cfg);
			LOG_RUNLOG_DEBUG("LP linphonec_set_registered to false success");
//		}
	}
}


//add by chensq 20120727
void linphonec_send_dtmf(char dtmf)
{
	linphone_core_send_dtmf(linphonec, dtmf);
}



void linphonec_call_identify(LinphoneCall* call){
	static long callid=1;
	linphone_call_set_user_pointer (call,(void*)callid);
	callid++;
}

LinphoneCall *linphonec_get_call(long id){
	const MSList *elem=linphone_core_get_calls(linphonec);
	for (;elem!=NULL;elem=elem->next){
		LinphoneCall *call=(LinphoneCall*)elem->data;
		if (linphone_call_get_user_pointer (call)==(void*)id){
			return call;
		}
	}
	linphonec_out("Sorry, no call with id %i exists at this time.\n",id);
	return NULL;
}

/***************************************************************************
 *
 * Linphone core callbacks
 *
 ***************************************************************************/

/*
 * Linphone core callback
 */
static void
linphonec_display_refer (LinphoneCore * lc, const char *refer_to)
{
	linphonec_out("Receiving out of call refer to %s\n", refer_to);
}

/*
 * Linphone core callback
 */
static void
linphonec_display_something (LinphoneCore * lc, const char *something)
{
	fprintf (stdout, "%s\n%s", something,prompt);
	fflush(stdout);
}

/*
 * Linphone core callback
 */
static void
linphonec_display_status (LinphoneCore * lc, const char *something)
{
	fprintf (stdout, "%s\n%s", something,prompt);
	fflush(stdout);
}

/*
 * Linphone core callback
 */
static void
linphonec_display_warning (LinphoneCore * lc, const char *something)
{
	fprintf (stdout, "Warning: %s\n%s", something,prompt);
	fflush(stdout);
}

/*
 * Linphone core callback
 */
static void
linphonec_display_url (LinphoneCore * lc, const char *something, const char *url)
{
	fprintf (stdout, "%s : %s\n", something, url);
}

/*
 * Linphone core callback
 */
static void
linphonec_prompt_for_auth(LinphoneCore *lc, const char *realm, const char *username)
{
	/* no prompt possible when using pipes or tcp mode*/
	if (unix_socket){
		linphone_core_abort_authentication(lc,NULL);
	}else{
		LinphoneAuthInfo *pending_auth;

		if ( auth_stack.nitems+1 > MAX_PENDING_AUTH )
		{
			fprintf(stderr,
				"Can't accept another authentication request.\n"
				"Consider incrementing MAX_PENDING_AUTH macro.\n");
			return;
		}

		pending_auth=linphone_auth_info_new(username,NULL,NULL,NULL,realm);
		auth_stack.elem[auth_stack.nitems++]=pending_auth;
	}
}

/*
 * Linphone core callback
 */
static void
linphonec_notify_received(LinphoneCore *lc, LinphoneCall *call, const char *from,const char *event)
{
	if(!strcmp(event,"refer"))
	{
		linphonec_out("The distand endpoint %s of call %li has been transfered, you can safely close the call.\n",
		              from,(long)linphone_call_get_user_pointer (call));
	}
}


/*
 * Linphone core callback
 */
static void
linphonec_notify_presence_received(LinphoneCore *lc,LinphoneFriend *fid)
{
	char *tmp=linphone_address_as_string(linphone_friend_get_address(fid));
	printf("Friend %s is %s\n", tmp, linphone_online_status_to_string(linphone_friend_get_status(fid)));
	ms_free(tmp);
	// todo: update Friend list state (unimplemented)
}

/*
 * Linphone core callback
 */
static void
linphonec_new_unknown_subscriber(LinphoneCore *lc, LinphoneFriend *lf,
		const char *url)
{
	printf("Friend %s requested subscription "
		"(accept/deny is not implemented yet)\n", url);
	// This means that this person wishes to be notified
	// of your presence information (online, busy, away...).

}

static void linphonec_call_updated(LinphoneCall *call){
	const LinphoneCallParams *cp=linphone_call_get_current_params(call);
	if (!linphone_call_camera_enabled (call) && linphone_call_params_video_enabled (cp)){
		linphonec_out("Far end requests to share video.\nType 'camera on' if you agree.\n");
	}
}


//--------------------------------------------------------------------------------
//add by chensq 20120518
static void linphonec_reg_state_changed(struct _LinphoneCore *lc,
        LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate,
        const char *message)
{
    switch (cstate)
    {
    case LinphoneRegistrationNone:
    {
        break;
    }
    case LinphoneRegistrationProgress:
    {
        break;
    }
    case LinphoneRegistrationOk:
    {
        break;
    }
    case LinphoneRegistrationCleared:
    	break;
    case LinphoneRegistrationFailed:
    {
    	//add by wzk
	    linphone_proxy_config_enable_register(cfg,TRUE);
	    cfg->registered=FALSE;
        break;
    }
    default:
        break;
    }

    if (pfRegStateChange)
    {
        pfRegStateChange(lc, cfg, cstate, message);
    }
}
//--------------------------------------------------------------------------------



//modify by chensq 20120518
static void linphonec_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState st, const char *msg){
	char *from=NULL;
	//char *from=linphone_call_get_remote_address_as_string(call);

	switch(st){
		case LinphoneCallEnd:
			//linphonec_out("Call %i with %s ended.\n", id, from);
		break;
		case LinphoneCallResuming:
			//linphonec_out("Resuming call %i with %s.\n", id, from);
		break;
		case LinphoneCallStreamsRunning:
			//linphonec_out("Media streams established with %s for call %i.\n", from,id);
		break;
		case LinphoneCallPausing:
			//linphonec_out("Pausing call %i with %s.\n", id, from);
		break;
		case LinphoneCallPaused:
			//linphonec_out("Call %i with %s is now paused.\n", id, from);
		break;
		case LinphoneCallPausedByRemote:
			//linphonec_out("Call %i has been paused by %s.\n",id,from);
		break;
		case LinphoneCallIncomingReceived:
			from=linphone_call_get_remote_address_as_string(call);
			linphonec_call_identify(call);
			linphone_call_enable_camera (call,linphonec_camera_enabled);
			long id=(long)linphone_call_get_user_pointer (call);
			linphonec_set_caller(from);
			if ( auto_answer)  {
				answer_call=TRUE;
			}
			linphonec_out("Receiving new incoming call from %s, assigned id %i\n", from,id);
			ms_free(from);
		break;
		case LinphoneCallOutgoingInit:
			linphonec_call_identify(call);
			//id=(long)linphone_call_get_user_pointer (call);
			//from=linphone_call_get_remote_address_as_string(call);
			//linphonec_out("Establishing call id to %s, assigned id %i\n", from,id);
		break;
		case LinphoneCallUpdatedByRemote:
			linphonec_call_updated(call);
		break;
		case LinphoneCallOutgoingProgress:
			//linphonec_out("Call %i to %s in progress.\n", id, from);
			break;
		case LinphoneCallOutgoingRinging:
			//linphonec_out("Call %i to %s ringing.\n", id, from);
			break;
		case LinphoneCallConnected:
			//linphonec_out("Call %i with %s connected.\n", id, from);
			break;
		case LinphoneCallOutgoingEarlyMedia:
			//linphonec_out("Call %i with %s early media.\n", id, from);
			break;
		case LinphoneCallError:
			//linphonec_out("Call %i with %s error.\n", id, from);
			break;
		default:
		break;
	}
	//ms_free(from);

	//--------------------------------------------------------------------------------
	//add by chensq 20120518
    if (pfCallStateChange)
    {
        pfCallStateChange(lc, call, st);
    }
    //--------------------------------------------------------------------------------
}

//--------------------------------------------------------------------------------
//add by chensq 20120518
static void linphonec_notify_call_failure(LinphoneCore *lc, LinphoneCall *call,
        SalError salerror, SalReason salreason, int code)
{
    long id = (long) linphone_call_get_user_pointer(call);

    if (pfCallFailed)
    {
        pfCallFailed(lc, call, salerror, salreason, code);
    }
}
//--------------------------------------------------------------------------------



/*
 * Linphone core callback
 */
static void
linphonec_text_received(LinphoneCore *lc, LinphoneChatRoom *cr,
		const LinphoneAddress *from, const char *msg)
{
	//linphonec_out("Message received from %s: %s\n", linphone_address_as_string(from), msg);
	// TODO: provide mechanism for answering.. ('say' command?)

	//--------------------------------------------------------------------------------
	//add by chensq 20121022 for gyy
	if (pfMessageRecv)
	{
		pfMessageRecv(lc, from, msg);
	}
	//--------------------------------------------------------------------------------


}


//modify by chensq 20120520
static void linphonec_dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf){
	char *from=linphone_call_get_remote_address_as_string(call);
	fprintf(stdout,"Receiving tone %c from %s\n",dtmf,from);
	fflush(stdout);
	ms_free(from);

	//--------------------------------------------------------------------------------
	//add by chensq 20120521
	if (pfDtmfRecv)
	{
		pfDtmfRecv(lc, call, dtmf);
	}
	//--------------------------------------------------------------------------------

}

static char received_prompt[PROMPT_MAX_LEN];
static ms_mutex_t prompt_mutex;
static bool_t have_prompt=FALSE;

static void *prompt_reader_thread(void *arg){
	char *ret;
	char tmp[PROMPT_MAX_LEN];
	while ((ret=fgets(tmp,sizeof(tmp),stdin))!=NULL){
		ms_mutex_lock(&prompt_mutex);
		strcpy(received_prompt,ret);
		have_prompt=TRUE;
		ms_mutex_unlock(&prompt_mutex);
	}
	return NULL;
}

static void start_prompt_reader(void){
	ortp_thread_t th;
	ms_mutex_init(&prompt_mutex,NULL);
	ortp_thread_create(&th,NULL,prompt_reader_thread,NULL);
}
#if !defined(_WIN32_WCE)
static ortp_pipe_t create_server_socket(void){
	char path[128];
#ifndef WIN32
	snprintf(path,sizeof(path)-1,"linphonec-%i",getuid());
	LOG_RUNLOG_DEBUG("LP create_server_socket:%s\n", path);
#else
	{
		TCHAR username[128];
		DWORD size=sizeof(username)-1;
		GetUserName(username,&size);
		snprintf(path,sizeof(path)-1,"linphonec-%s",username);
	}
#endif
	return ortp_server_pipe_create(path);
}


static void *pipe_thread(void*p){
	char tmp[250];
	server_sock=create_server_socket();
	LOG_RUNLOG_DEBUG("LP server_sock=ORTP_PIPE_INVALID\n");
	if (server_sock==ORTP_PIPE_INVALID) return NULL;

	LOG_RUNLOG_DEBUG("LP pipe_thread:pipe_thread begin\n");

	while(pipe_reader_run){
		while(client_sock!=ORTP_PIPE_INVALID){ /*sleep until the last command is finished*/
#ifndef WIN32
			usleep(20000);
#else
			Sleep(20);
#endif
		}
		LOG_RUNLOG_DEBUG("LP pipe_thread:ortp_server_pipe_accept_client\n");
		client_sock=ortp_server_pipe_accept_client(server_sock);
		if (client_sock!=ORTP_PIPE_INVALID){
			int len;
			LOG_RUNLOG_DEBUG("LP client_sock=%d\n", client_sock);
			/*now read from the client */
			if ((len=ortp_pipe_read(client_sock,(uint8_t*)tmp,sizeof(tmp)-1))>0){
				ortp_mutex_lock(&prompt_mutex);
				tmp[len]='\0';
				strcpy(received_prompt,tmp);
				LOG_RUNLOG_DEBUG("LP Receiving command '%s'\n",received_prompt);
				printf("Receiving command '%s'\n",received_prompt);fflush(stdout);
				have_prompt=TRUE;
				ortp_mutex_unlock(&prompt_mutex);
			}else{
				LOG_RUNLOG_DEBUG("LP read nothing\n");
				printf("read nothing\n");fflush(stdout);
				ortp_server_pipe_close_client(client_sock);
				client_sock=ORTP_PIPE_INVALID;
			}

		}else{
			if (pipe_reader_run) fprintf(stderr,"accept() failed: %s\n",strerror(errno));
		}
	}
	ms_message("Exiting pipe_reader_thread.");
	fflush(stdout);
	return NULL;
}

static void start_pipe_reader(void){
	ms_mutex_init(&prompt_mutex,NULL);
	pipe_reader_run=TRUE;
	ortp_thread_create(&pipe_reader_th,NULL,pipe_thread,NULL);
}

static void stop_pipe_reader(void){
	pipe_reader_run=FALSE;
	linphonec_out("stop_pipe_reader begin\n");
	linphonec_command_finished();
	linphonec_out("linphonec_command_finished over\n");
	ortp_server_pipe_close(server_sock);
	linphonec_out("ortp_server_pipe_close over\n");
	return; //临时添加代码
	ortp_thread_join(pipe_reader_th,NULL);
	linphonec_out("ortp_thread_join pipe_reader_th over\n");
}
#endif /*_WIN32_WCE*/

#ifdef HAVE_READLINE
#define BOOL_HAVE_READLINE 1
#else
#define BOOL_HAVE_READLINE 0
#endif

//modify by chensq 20120520
char *linphonec_readline(char *prompt){
	if (unix_socket || !BOOL_HAVE_READLINE ){
		static bool_t prompt_reader_started=FALSE;
		static bool_t pipe_reader_started=FALSE;

//modify by 20120528
//#define CONSOLE_LINPHONE
#ifdef CONSOLE_LINPHONE
		if (!prompt_reader_started){
			start_prompt_reader();
			prompt_reader_started=TRUE;
		}
#endif

		if (unix_socket && !pipe_reader_started){
#if !defined(_WIN32_WCE)
			LOG_RUNLOG_DEBUG("LP start_pipe_reader\n");
			start_pipe_reader();
			pipe_reader_started=TRUE;
#endif /*_WIN32_WCE*/
		}

//modify by 20120528
#ifdef CONSOLE_LINPHONE
		fprintf(stdout,"%s",prompt);
		fflush(stdout);
#endif

		while(1){
			ms_mutex_lock(&prompt_mutex);

			if (have_prompt){
				char *ret=strdup(received_prompt);
				have_prompt=FALSE;

				LOG_RUNLOG_DEBUG("LP received_prompt:%s", received_prompt);
				ms_mutex_unlock(&prompt_mutex);
				return ret;
			}

#ifdef _YK_IMX27_AV_
			static int iTempCount = 0;
			if(iTempCount++ > 3)
			{
				iTempCount = 0;
				NotifyWDThreadFromLP();
			}
#endif


//			//--------------------------------------------------------------------------------
//			//add by chensq 20120520
//			char *pcMsg = NULL;
//			if( pfMsgHandleSupport )
//			{
//			    pcMsg = pfMsgHandleSupport();
//	            if ( pcMsg )
//	            {
//	                ms_mutex_unlock(&prompt_mutex);
//	                fprintf(stdout, "%s\r\n", pcMsg);
//	                fflush(stdout);
//	                return pcMsg;
//	            }
//			}
//            //--------------------------------------------------------------------------------

			ms_mutex_unlock(&prompt_mutex);
			linphonec_idle_call();
#ifdef WIN32
			Sleep(20);
			/* Following is to get the video window going as it
				 should. Maybe should we only have this on when the option -V
				 or -D is on? */
			MSG msg;
	
			if (PeekMessage(&msg, NULL, 0, 0,1)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
#else
			usleep(20000);
#endif
		}
	}else{
#ifdef HAVE_READLINE
		return readline(prompt);
#endif
	}
}
//modify by xuqd

void linphonec_out(const char *fmt,...){
	char *res;
	va_list args;
	va_start (args, fmt);
	res=ortp_strdup_vprintf(fmt,args);
	va_end (args);
	printf("%s",res);
//#define LINPHONC_LOG_DOMAIN "linphonec_out"
//	__android_log_print(ANDROID_LOG_DEBUG, LINPHONC_LOG_DOMAIN, "%s",res);
//#ifdef ANDROID
//#define LINPHONC_LOG_DOMAIN "linphonec_out"
//	__android_log_print(ANDROID_LOG_DEBUG, LINPHONC_LOG_DOMAIN, "%s",res);
//#endif
	fflush(stdout);

//	//annonate by chensq 设备运行异常
//#if !defined(_WIN32_WCE)
//	if (client_sock!=ORTP_PIPE_INVALID){
//		if (ortp_pipe_write(client_sock,(uint8_t*)res,strlen(res))==-1){
//			fprintf(stderr,"Fail to send output via pipe: %s",strerror(errno));
//		}
//	}
//#endif /*_WIN32_WCE*/

	ortp_free(res);
}

void linphonec_command_finished(void){
#if !defined(_WIN32_WCE)
	if (client_sock!=ORTP_PIPE_INVALID){
		ortp_server_pipe_close_client(client_sock);
		client_sock=ORTP_PIPE_INVALID;
	}
#endif /*_WIN32_WCE*/
}

void linphonec_set_autoanswer(bool_t enabled){
	auto_answer=enabled;
}

bool_t linphonec_get_autoanswer(){
	return auto_answer;
}

LinphoneCoreVTable linphonec_vtable={0};

/***************************************************************************/
/*
 * Main
 *
 * Use globals:
 *
 *	- char *histfile_name
 *	- FILE *mylogfile
 */
#if defined (_WIN32_WCE)

char **convert_args_to_ascii(int argc, _TCHAR **wargv){
	int i;
	char **result=malloc(argc*sizeof(char*));
	char argtmp[128];
	for(i=0;i<argc;++i){
		wcstombs(argtmp,wargv[i],sizeof(argtmp));
		result[i]=strdup(argtmp);
	}
	return result;
}

int _tmain(int argc, _TCHAR* wargv[]) {
	char **argv=convert_args_to_ascii(argc,wargv);
	trace_level=6;

#else

//modify by chensq : main-->linphonec_main
int
linphonec_main (int argc, char *argv[]) {
#endif
	linphonec_vtable.call_state_changed=linphonec_call_state_changed;
	linphonec_vtable.notify_presence_recv = linphonec_notify_presence_received;
	linphonec_vtable.new_subscription_request = linphonec_new_unknown_subscriber;
	linphonec_vtable.auth_info_requested = linphonec_prompt_for_auth;
	linphonec_vtable.display_status = linphonec_display_status;
	linphonec_vtable.display_message=linphonec_display_something;
	linphonec_vtable.display_warning=linphonec_display_warning;
	linphonec_vtable.display_url=linphonec_display_url;
	linphonec_vtable.text_received=linphonec_text_received;
	linphonec_vtable.dtmf_received=linphonec_dtmf_received;
	linphonec_vtable.refer_received=linphonec_display_refer;
	linphonec_vtable.notify_recv=linphonec_notify_received;
	
	if (! linphonec_init(argc, argv) ) exit(EXIT_FAILURE);

	linphonec_main_loop (linphonec, sipAddr);

	linphonec_finish(EXIT_SUCCESS);

	exit(EXIT_SUCCESS); /* should never reach here */
}


//add by chensq 20120520
void *linphonec_main2(void *arg)
{
    char* argv[5];
    memset(&linphonec_vtable, 0, sizeof(LinphoneCoreVTable));
    linphonec_vtable.call_state_changed = linphonec_call_state_changed;
    linphonec_vtable.registration_state_changed = linphonec_reg_state_changed;
    linphonec_vtable.notify_presence_recv = linphonec_notify_presence_received;
    linphonec_vtable.new_subscription_request =  linphonec_new_unknown_subscriber;
    linphonec_vtable.dtmf_received = linphonec_dtmf_received;
    linphonec_vtable.notify_recv = linphonec_notify_received;
    linphonec_vtable.notify_call_failure = linphonec_notify_call_failure;
    //linphonec_vtable.notify_reg_status = pfRegOkFailedNotify;
#ifdef HINT_MUSIC
    linphonec_vtable.outgoing_hint_music_done = linphonec_outgoing_hint_music_done;
#endif


	//add by chensq 20121022 for gyy
    linphonec_vtable.text_received = linphonec_text_received;


    argv[0] = "linphone";
    argv[1] = "-c";
    argv[2] = getenv("LP_CONFIG_PATH") ? getenv("LP_CONFIG_PATH") : "~/.linphonerc";
    argv[3] = "-C";
    argv[4] = "--pipe";

    if (!linphonec_init(5, argv))
    {
        pthread_cond_signal(&start_lp_cond);
        pthread_exit((void *) 1);
    }

//    pthread_cond_signal(&start_lp_cond);

    if(pfDefaultReg)
    {
    	pfDefaultReg();
    	sleep(3);
    }
    else
    {
    	LOG_RUNLOG_DEBUG("LP has no default reg info");
    }

    pthread_cond_signal(&start_lp_cond);

    /* startup done */
    linphonec_main_loop(linphonec, NULL);

    linphonec_finish(EXIT_SUCCESS);

    //pthread_exit((void *) 0); //annonate by chensq

    // never reached
    return NULL;
}


//add by chensq 20120520
LinphoneCore* linphonec_startup(void)
{
    int rst = 0;

    rst = pthread_create(&linphone_thr, NULL, linphonec_main2, NULL);
    if (0 != rst)
    {
        return NULL;
    }
    else
    {
        /* wait for main_loop start! */
        int thr_status = 0;
        pthread_mutex_init(&start_lp_mutex, NULL);
        pthread_mutex_init(&config_file_mutex, NULL);
        pthread_cond_init(&start_lp_cond, NULL);
        pthread_mutex_lock(&start_lp_mutex);
        pthread_cond_wait(&start_lp_cond, &start_lp_mutex);
        /* 向一个线程发送信号测试线程是否存活*/
        thr_status = pthread_kill(linphone_thr, 0);
        pthread_mutex_destroy(&start_lp_mutex);
        pthread_cond_destroy(&start_lp_cond);

        if (ESRCH == thr_status)
        {
            return NULL;
        }
        else
        {
            /* ok */
            return linphonec;
        }
    }
}

//add by chensq 20120520
void linphonec_shutdown(void)
{
	void *return_code;

    LOG_RUNLOG_DEBUG("LP linphonec_shutdown begin:%d", linphonec_running);
    if (linphone_thr)
    {
    	LOG_RUNLOG_DEBUG("LP wait linphone_thr thread end......\n");
    	//LPProcessCommand("quit");

		LOG_RUNLOG_DEBUG("LP LPProcessCommand:%s", "quit");
	    char path[512]={0x0};
	    ortp_pipe_t handle=NULL;
	    snprintf(path,sizeof(path)-1,"linphonec-%i",getuid());
	    handle = ortp_client_pipe_connect(path);
	    ortp_pipe_write(handle, "quit", strlen("quit"));
	    ortp_client_pipe_close(handle);

    	//sleep(4);
    	pthread_join(linphone_thr, &return_code);
    }
    linphone_thr = NULL;
    pthread_mutex_destroy(&config_file_mutex);
    LOG_RUNLOG_DEBUG("LP linphonec_shutdown end :%d", linphonec_running);
}


/*
 * Initialize linphonec
 */
static int
linphonec_init(int argc, char **argv)
{

	//g_mem_set_vtable(&dbgtable);

	/*
	 * Set initial values for global variables
	 */
	mylogfile = NULL;
	
	
#ifndef _WIN32
	snprintf(configfile_name, PATH_MAX, "%s/.linphonerc",
			getenv("HOME"));
#elif defined(_WIN32_WCE)
	strncpy(configfile_name,PACKAGE_DIR "\\linphonerc",PATH_MAX);
	mylogfile=fopen(PACKAGE_DIR "\\" "linphonec.log","w");
	printf("Logs are redirected in" PACKAGE_DIR "\\linphonec.log");
#else
	snprintf(configfile_name, PATH_MAX, "%s/Linphone/linphonerc",
			getenv("APPDATA"));
#endif
	/* Handle configuration filename changes */
	switch (handle_configfile_migration())
	{
		case -1: /* error during file copies */
			fprintf(stderr,
				"Error in configuration file migration\n");
			break;

		case 0: /* nothing done */
		case 1: /* migrated */
		default:
			break;
	}

#ifdef ENABLE_NLS
	if (NULL == bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR))
		perror ("bindtextdomain failed");
#ifndef __ARM__
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
	textdomain (GETTEXT_PACKAGE);
#else
	printf ("NLS disabled.\n");
#endif

	linphonec_parse_cmdline(argc, argv);

	if (trace_level > 0)
	{
		if (logfile_name != NULL)
			mylogfile = fopen (logfile_name, "w+");

		if (mylogfile == NULL)
		{
			mylogfile = stdout;
			fprintf (stderr,
				 "INFO: no logfile, logging to stdout\n");
		}
		linphone_core_enable_logs(mylogfile);
	}
	else
	{
		linphone_core_disable_logs();
	}
	/*
	 * Initialize auth stack
	 */
	auth_stack.nitems=0;

	/*
	 * Initialize linphone core
	 */
	linphonec=linphone_core_new (&linphonec_vtable, configfile_name, factory_configfile_name, NULL);
	//LOG_RUNLOG_DEBUG("LP video config C=%d, D=%d, P=%d", vcap_enabled,display_enabled, preview_enabled);
	linphone_core_enable_video(linphonec,vcap_enabled,display_enabled);

	//使能网络监测
	linphone_core_isable_network_monitor(linphonec, network_monitor_enabled);

	if (display_enabled && window_id != 0) 
	{
		printf ("Setting window_id: 0x%x\n", window_id);
		linphone_core_set_native_video_window_id(linphonec,window_id);
	}

	linphone_core_enable_video_preview(linphonec,preview_enabled);
	if (!(vcap_enabled || display_enabled)) printf("Warning: video is disabled in linphonec, use -V or -C or -D to enable.\n");
#ifdef HAVE_READLINE
	/*
	 * Initialize readline
	 */
	linphonec_initialize_readline();
#endif
#if !defined(_WIN32_WCE)
	/*
	 * Initialize signal handlers
	 */
	//modify by chensq
	signal(SIGTERM, linphonec_finish);
	signal(SIGINT, linphonec_finish);
//	signal(SIGTERM, LPFini);
//	signal(SIGINT, LPFini);

#endif /*_WIN32_WCE*/
	return 1;
}


void linphonec_main_loop_exit(void){
	linphonec_running=FALSE;
}

/*
 * Close linphonec, cleanly terminating
 * any pending call
 */
void
linphonec_finish(int exit_status)
{

	// Do not allow concurrent destroying to prevent glibc errors
	static bool_t terminating=FALSE;
	if (terminating) return; 
	terminating=TRUE;
	linphonec_out("Terminating...\n");

	/* Terminate any pending call */
	linphone_core_terminate_all_calls(linphonec);
	linphonec_out("Terminating over\n");
#ifdef HAVE_READLINE
	linphonec_finish_readline();
#endif
#if !defined(_WIN32_WCE)
	if (pipe_reader_run)
		stop_pipe_reader();
#endif /*_WIN32_WCE*/

	linphone_core_destroy (linphonec);

	if (mylogfile != NULL && mylogfile != stdout)
	{
		fclose (mylogfile);
	}
	printf("\n");
	//exit(exit_status); //check

}

/*
 * This is called from idle_call() whenever
 * pending_auth != NULL.
 *
 * It prompts user for a password.
 * Hitting ^D (EOF) would make this function
 * return 0 (Cancel).
 * Any other input would try to set linphone core
 * auth_password for the pending_auth, add the auth_info
 * and return 1.
 */
int
linphonec_prompt_for_auth_final(LinphoneCore *lc)
{
	char *input, *iptr;
	char auth_prompt[256];
#ifdef HAVE_READLINE
	rl_hook_func_t *old_event_hook;
#endif
	LinphoneAuthInfo *pending_auth=auth_stack.elem[auth_stack.nitems-1];

	snprintf(auth_prompt, 256, "Password for %s on %s: ",
		pending_auth->username, pending_auth->realm);

	printf("\n");
#ifdef HAVE_READLINE
	/*
	 * Disable event hook to avoid entering an
	 * infinite loop. This would prevent idle_call
	 * from being called during authentication reads.
	 * Note that it might be undesiderable...
	 */
	old_event_hook=rl_event_hook;
	rl_event_hook=NULL;
#endif

	while (1)
	{
		input=linphonec_readline(auth_prompt);

		/*
		 * If EOF (^D) is sent you probably don't want
		 * to provide an auth password... should give up
		 * the operation, but there's no mechanism to
		 * send this info back to caller currently...
		 */
		if ( ! input )
		{
			printf("Cancel requested, but not implemented.\n");
			continue;
		}

		/* Strip blanks */
		iptr=lpc_strip_blanks(input);

		/*
		 * Only blanks, continue asking
		 */
		if ( ! *iptr )
		{
			free(input);
			continue;
		}

		/* Something typed, let's try */
		break;
	}

	/*
	 * No check is done here to ensure password is correct.
	 * I guess password will be asked again later.
	 */
	linphone_auth_info_set_passwd(pending_auth, input);
	linphone_core_add_auth_info(lc, pending_auth);
	linphone_auth_info_destroy(pending_auth);
	auth_stack.elem[auth_stack.nitems-1]=0;
	--(auth_stack.nitems);
#ifdef HAVE_READLINE
	/*
	 * Reset line_buffer, to avoid the password
	 * to be used again from outer readline
	 */
	rl_line_buffer[0]='\0';
	rl_event_hook=old_event_hook;
#endif
	return 1;
}

void
print_usage (int exit_status)
{
	fprintf (stdout, "\n\
usage: linphonec [-c file] [-s sipaddr] [-a] [-V] [-d level ] [-l logfile]\n\
       linphonec -v\n\
\n\
  -b  file             specify path of readonly factory configuration file.\n\
  -c  file             specify path of configuration file.\n\
  -d  level            be verbose. 0 is no output. 6 is all output\n\
  -l  logfile          specify the log file for your SIP phone\n\
  -s  sipaddress       specify the sip call to do at startup\n\
  -a                   enable auto answering for incoming calls\n\
  -V                   enable video features globally (disabled by default)\n\
  -C                   enable video capture only (disabled by default)\n\
  -D                   enable video display only (disabled by default)\n\
  -S                   show general state messages (disabled by default)\n\
  --wid  windowid      force embedding of video window into provided windowid (disabled by default)\n\
  -v or --version      display version and exits.\n");

  	exit(exit_status);
}

#ifdef VIDEO_ENABLED

#ifdef HAVE_X11_XLIB_H
static void x11_apply_video_params(VideoParams *params, Window window){
	XWindowChanges wc;
	unsigned int flags=0;
	static Display *display = NULL;
	const char *dname=getenv("DISPLAY");

	if (display==NULL && dname!=NULL){
		display=XOpenDisplay(dname);
	}

	if (display==NULL){
		ms_error("Could not open display %s",dname);
		return;
	}
	memset(&wc,0,sizeof(wc));
	wc.x=params->x;
	wc.y=params->y;
	wc.width=params->w;
	wc.height=params->h;
	if (params->x!=-1 ){
		flags|=CWX|CWY;
	}
	if (params->w!=-1){
		flags|=CWWidth|CWHeight;
	}
	/*printf("XConfigureWindow x=%i,y=%i,w=%i,h=%i\n",
	       wc.x, wc.y ,wc.width, wc.height);*/
	XConfigureWindow(display,window,flags,&wc);
	if (params->show)
		XMapWindow(display,window);
	else
		XUnmapWindow(display,window);
	XSync(display,FALSE);
}
#endif


static void lpc_apply_video_params(){
	static unsigned long old_wid=0,old_pwid=0;
	unsigned long wid=linphone_core_get_native_video_window_id (linphonec);
	unsigned long pwid=linphone_core_get_native_preview_window_id (linphonec);

	if (wid!=0 && (lpc_video_params.refresh || old_wid!=wid)){
		lpc_video_params.refresh=FALSE;
#ifdef HAVE_X11_XLIB_H
		if (lpc_video_params.wid==0){  // do not manage window if embedded
			x11_apply_video_params(&lpc_video_params,wid);
		}
#endif
	}
	old_wid=wid;
	if (pwid!=0 && (lpc_preview_params.refresh || old_pwid!=pwid)){
		lpc_preview_params.refresh=FALSE;
#ifdef HAVE_X11_XLIB_H
		/*printf("wid=%lu pwid=%lu\n",wid,pwid);*/
		if (lpc_preview_params.wid==0){  // do not manage window if embedded
			printf("Refreshing\n");
			x11_apply_video_params(&lpc_preview_params,pwid);
		}
#endif
	}
	old_pwid=pwid;
}

#endif


/*
 *
 * Called every second from main read loop.
 *
 * Will use the following globals:
 *
 *  - LinphoneCore linphonec
 *  - LPC_AUTH_STACK auth_stack;
 *
 */
static int
linphonec_idle_call ()
{
//	static int iCount = 0;
//	if(iCount++ > 50000)//20 000 * 4000 = 20ms*50000 = 1000s =16m
//	{
//		iCount = 0;
//		system("date");
//		LOG_RUNLOG_INFO("LP LP Thread linphonec_idle_call alive");
//	}


	LinphoneCore *opm=linphonec;

	/* Uncomment the following to verify being called */
	/* printf(".\n"); */

	linphone_core_iterate(opm);
	if (answer_call){
		fprintf (stdout, "-------auto answering to call-------\n" );
		linphone_core_accept_call(opm,NULL);
		answer_call=FALSE;
	}

	if ( auth_stack.nitems )
	{
		/*
		 * Inhibit command completion
		 * during password prompts
		 */
#ifdef HAVE_READLINE
		rl_inhibit_completion=1;
#endif
		linphonec_prompt_for_auth_final(opm);
#ifdef HAVE_READLINE
		rl_inhibit_completion=0;
#endif
	}

#ifdef VIDEO_ENABLED
	lpc_apply_video_params();
#endif

	return 0;
}

#ifdef HAVE_READLINE
/*
 * Use globals:
 *
 *	- char *histfile_name (also sets this)
 *      - char *last_in_history (allocates it)
 */
static int
linphonec_initialize_readline()
{
	/*rl_bind_key('\t', rl_insert);*/

	/* Allow conditional parsing of ~/.inputrc */
	rl_readline_name = "linphonec";

	/* Call idle_call() every second */
	rl_set_keyboard_input_timeout(LPC_READLINE_TIMEOUT);
	rl_event_hook=linphonec_idle_call;

	/* Set history file and read it */
	histfile_name = ms_strdup_printf ("%s/.linphonec_history",
		getenv("HOME"));
	read_history(histfile_name);

	/* Initialized last_in_history cache*/
	last_in_history[0] = '\0';

	/* Register a completion function */
	rl_attempted_completion_function = linephonec_readline_completion;

	/* printf("Readline initialized.\n"); */
        setlinebuf(stdout);
	return 0;
}

/*
 * Uses globals:
 *
 *	- char *histfile_name (writes history to file and frees it)
 *	- char *last_in_history (frees it)
 *
 */
static int
linphonec_finish_readline()
{

	stifle_history(HISTSIZE);
	write_history(histfile_name);
	free(histfile_name);
	histfile_name=NULL;
	return 0;
}

#endif

static void print_prompt(LinphoneCore *opm){
#ifdef IDENTITY_AS_PROMPT
	snprintf(prompt, PROMPT_MAX_LEN, "%s> ",
		linphone_core_get_primary_contact(opm));
#else
	snprintf(prompt, PROMPT_MAX_LEN, "linphonec> ");
#endif
}

static int
linphonec_main_loop (LinphoneCore * opm, char * sipAddr)
{
	LOG_RUNLOG_DEBUG("LP linphonec_main_loop begin :%d", linphonec_running);

	char buf[LINE_MAX_LEN]; /* auto call handling */
	char *input;

	print_prompt(opm);


	/* auto call handling */
	if (sipAddr != NULL )
	{
		snprintf (buf, sizeof(buf),"call %s", sipAddr);
		linphonec_parse_command_line(linphonec, buf);
	}

	while (linphonec_running && (input=linphonec_readline(prompt)))
	{


		char *iptr; /* input and input pointer */
		size_t input_len;

		/* Strip blanks */
		iptr=lpc_strip_blanks(input);

		input_len = strlen(iptr);

		/*
		 * Do nothing but release memory
		 * if only blanks are read
		 */
		if ( ! input_len )
		{
			free(input);
			continue;
		}

#ifdef HAVE_READLINE
		/*
		 * Only add to history if not already
		 * last item in it, and only if the command
		 * doesn't start with a space (to allow for
		 * hiding passwords)
		 */
		if ( iptr == input && strcmp(last_in_history, iptr) )
		{
			strncpy(last_in_history,iptr,sizeof(last_in_history));
			last_in_history[sizeof(last_in_history)-1]='\0';
			add_history(iptr);
		}
#endif

		LOG_RUNLOG_DEBUG("LP linphonec_parse_command_line:%s", iptr);
		linphonec_parse_command_line(linphonec, iptr);
		linphonec_command_finished();
		free(input);
	}
	LOG_RUNLOG_DEBUG("LP linphonec_main_loop end :%d", linphonec_running);
	return 0;
}


////20120518
//static int linphonec_main_loop_autoanswer(LinphoneCore * opm, char * sipAddr)
//{
//    char buf[LINE_MAX_LEN]; /* auto call handling */
//    char *input;
//
//    print_prompt(opm);
//
//    linphonec_set_autoanswer(TRUE);
//
//    /* auto call handling */
//    if (sipAddr != NULL)
//    {
//        snprintf(buf, sizeof(buf), "call %s", sipAddr);
//        linphonec_parse_command_line(linphonec, buf);
//    }
//
//    while (linphonec_running && (input = linphonec_readline(prompt)))
//    {
//        char *iptr; /* input and input pointer */
//        size_t input_len;
//
//        /* Strip blanks */
//        iptr = lpc_strip_blanks(input);
//
//        input_len = strlen(iptr);
//
//        /*
//         * Do nothing but release memory
//         * if only blanks are read
//         */
//        if (!input_len)
//        {
//            free(input);
//            continue;
//        }
//
//#ifdef HAVE_READLINE
//        /*
//         * Only add to history if not already
//         * last item in it, and only if the command
//         * doesn't start with a space (to allow for
//         * hiding passwords)
//         */
//        if ( iptr == input && strcmp(last_in_history, iptr) )
//        {
//            strncpy(last_in_history,iptr,sizeof(last_in_history));
//            last_in_history[sizeof(last_in_history)-1]='\0';
//            add_history(iptr);
//        }
//#endif
//
//        linphonec_parse_command_line(linphonec, iptr);
//        linphonec_command_finished();
//        free(input);
//    }
//
//    return 0;
//}



/*
 *  Parse command line switches
 *
 *  Use globals:
 *
 *	- int trace_level
 *	- char *logfile_name
 *	- char *configfile_name
 *	- char *sipAddr
 */
static int
linphonec_parse_cmdline(int argc, char **argv)
{
	int arg_num=1;

	while (arg_num < argc)
	{
		int old_arg_num = arg_num;
		if (strncmp ("-d", argv[arg_num], 2) == 0)
		{
			arg_num++;
			if (arg_num < argc)
				trace_level = atoi (argv[arg_num]);
			else
				trace_level = 1;
		}
		else if (strncmp ("-l", argv[arg_num], 2) == 0)
		{
			arg_num++;
			if (arg_num < argc)
				logfile_name = argv[arg_num];
		}
		else if (strncmp ("-c", argv[arg_num], 2) == 0)
		{
			if ( ++arg_num >= argc ) print_usage(EXIT_FAILURE);
#if !defined(_WIN32_WCE)
			if (access(argv[arg_num],F_OK)!=0 )
			{
				fprintf (stderr,
					"Cannot open config file %s.\n",
					 argv[arg_num]);
				exit(EXIT_FAILURE);
			}
#endif /*_WIN32_WCE*/
			snprintf(configfile_name, PATH_MAX, "%s", argv[arg_num]);
		}
		else if (strncmp ("-b", argv[arg_num], 2) == 0)
		{
			if ( ++arg_num >= argc ) print_usage(EXIT_FAILURE);
#if !defined(_WIN32_WCE)
			if (access(argv[arg_num],F_OK)!=0 )
			{
				fprintf (stderr,
					"Cannot open config file %s.\n",
					 argv[arg_num]);
				exit(EXIT_FAILURE);
			}
#endif /*_WIN32_WCE*/
			factory_configfile_name = argv[arg_num];
		}
		else if (strncmp ("-s", argv[arg_num], 2) == 0)
		{
			arg_num++;
			if (arg_num < argc)
				sipAddr = argv[arg_num];
		}
                else if (strncmp ("-a", argv[arg_num], 2) == 0)
                {
                        auto_answer = TRUE;
                }
		else if (strncmp ("-C", argv[arg_num], 2) == 0)
                {
                        vcap_enabled = TRUE;
                        LOG_RUNLOG_DEBUG("LP start config vcap_enabled => TRUE");
                }
		else if (strncmp ("-D", argv[arg_num], 2) == 0)
                {
                        display_enabled = TRUE;
                        LOG_RUNLOG_DEBUG("LP start config display_enabled => TRUE");
                }
		else if (strncmp ("-V", argv[arg_num], 2) == 0)
                {
                        display_enabled = TRUE;
			vcap_enabled = TRUE;
			preview_enabled=TRUE;
			LOG_RUNLOG_DEBUG("LP start config vcap_enabled => TRUE display_enabled => TRUE preview_enabled => TRUE");
                }
		else if ((strncmp ("-v", argv[arg_num], 2) == 0)
			 ||
			 (strncmp
			  ("--version", argv[arg_num],
			   strlen ("--version")) == 0))
		{
#if !defined(_WIN32_WCE)
			printf ("version: " LINPHONE_VERSION "\n");
#endif
			exit (EXIT_SUCCESS);
		}
		else if (strncmp ("-S", argv[arg_num], 2) == 0)
		{
			show_general_state = TRUE;
		}
		else if (strncmp ("--pipe", argv[arg_num], 6) == 0)
		{
			unix_socket=1;
		}
		else if (strncmp ("--wid", argv[arg_num], 5) == 0)
		{
			arg_num++;
			if (arg_num < argc) {
				char *tmp;
				window_id = strtol( argv[arg_num], &tmp, 0 );
			}
		}
		else if (old_arg_num == arg_num)
		{
			fprintf (stderr, "ERROR: bad arguments\n");
			print_usage (EXIT_FAILURE);
		}
		arg_num++;
	}

	return 1;
}

/*
 * Up to version 1.2.1 linphone used ~/.linphonec for
 * CLI and ~/.gnome2/linphone for GUI as configuration file.
 * In newer version both interfaces will use ~/.linphonerc.
 *
 * This function helps transparently migrating from one
 * to the other layout using the following heuristic:
 *
 *	IF new_config EXISTS => do nothing
 *	ELSE IF old_cli_config EXISTS => copy to new_config
 *	ELSE IF old_gui_config EXISTS => copy to new_config
 *
 * Returns:
 *	 0 if it did nothing
 *	 1 if it migrated successfully
 *	-1 on error
 */
static int
handle_configfile_migration()
{
#if !defined(_WIN32_WCE)
	char *old_cfg_gui;
	char *old_cfg_cli;
	char *new_cfg;
#if !defined(_WIN32_WCE)
	const char *home = getenv("HOME");
#else
	const char *home = ".";
#endif /*_WIN32_WCE*/
	new_cfg = ms_strdup_printf("%s/.linphonerc", home);

	/*
	 * If the *NEW* configuration already exists
	 * do nothing.
	 */
	if (access(new_cfg,F_OK)==0)
	{
		free(new_cfg);
		return 0;
	}

	old_cfg_cli = ms_strdup_printf("%s/.linphonec", home);

	/*
	 * If the *OLD* CLI configurations exist copy it to
	 * the new file and make it a symlink.
	 */
	if (access(old_cfg_cli, F_OK)==0)
	{
		if ( ! copy_file(old_cfg_cli, new_cfg) )
		{
			free(old_cfg_cli);
			free(new_cfg);
			return -1;
		}
		printf("%s copied to %s\n", old_cfg_cli, new_cfg);
		free(old_cfg_cli);
		free(new_cfg);
		return 1;
	}

	free(old_cfg_cli);
	old_cfg_gui = ms_strdup_printf("%s/.gnome2/linphone", home);

	/*
	 * If the *OLD* GUI configurations exist copy it to
	 * the new file and make it a symlink.
	 */
	if (access(old_cfg_gui, F_OK)==0)
	{
		if ( ! copy_file(old_cfg_gui, new_cfg) )
		{
			exit(EXIT_FAILURE);
			free(old_cfg_gui);
			free(new_cfg);
			return -1;
		}
		printf("%s copied to %s\n", old_cfg_gui, new_cfg);
		free(old_cfg_gui);
		free(new_cfg);
		return 1;
	}

	free(old_cfg_gui);
	free(new_cfg);
#endif /*_WIN32_WCE*/
	return 0;
}
#if !defined(_WIN32_WCE)
/*
 * Copy file "from" to file "to".
 * Destination file is truncated if existing.
 * Return 1 on success, 0 on error (printing an error).
 */
static int
copy_file(const char *from, const char *to)
{
	char message[256];
	FILE *in, *out;
	char buf[256];
	size_t n;

	/* Open "from" file for reading */
	in=fopen(from, "r");
	if ( in == NULL )
	{
		snprintf(message, 255, "Can't open %s for reading: %s\n",
			from, strerror(errno));
		fprintf(stderr, "%s", message);
		return 0;
	}

	/* Open "to" file for writing (will truncate existing files) */
	out=fopen(to, "w");
	if ( out == NULL )
	{
		snprintf(message, 255, "Can't open %s for writing: %s\n",
			to, strerror(errno));
		fprintf(stderr, "%s", message);
		return 0;
	}

	/* Copy data from "in" to "out" */
	while ( (n=fread(buf, 1, sizeof buf, in)) > 0 )
	{
		if ( ! fwrite(buf, 1, n, out) )
		{
			return 0;
		}
	}

	fclose(in);
	fclose(out);

	return 1;
}
#endif /*_WIN32_WCE*/

#ifdef HAVE_READLINE
static char **
linephonec_readline_completion(const char *text, int start, int end)
{
	char **matches = NULL;

	/*
	 * Prevent readline from falling
	 * back to filename-completion
	 */
	rl_attempted_completion_over=1;

	/*
	 * If this is the start of line we complete with commands
	 */
	if ( ! start )
	{
		return rl_completion_matches(text, linphonec_command_generator);
	}

	/*
	 * Otherwise, we should peek at command name
	 * or context to implement a smart completion.
	 * For example: "call .." could return
	 * friends' sip-uri as matches
	 */

	return matches;
}

#endif

/*
 * Strip blanks from a string.
 * Return a pointer into the provided string.
 * Modifies input adding a NULL at first
 * of trailing blanks.
 */
char *
lpc_strip_blanks(char *input)
{
	char *iptr;

	/* Find first non-blank */
	while(*input && isspace(*input)) ++input;

	/* Find last non-blank */
	iptr=input+strlen(input);
	if (iptr > input) {
		while(isspace(*--iptr));
		*(iptr+1)='\0';
	}

	return input;
}





//------------------------------------------
//add by chensq

//返回值：0成功 <0失败
int linphonec_push_proxy(const char* username, const char* password,
        const char* proxy, const char* route, int expires, int enable_register)
{
	int iRet;
	pthread_mutex_lock (&config_file_mutex);
	iRet = linphonec_proxy_add2(linphonec, username, password, proxy, route, expires, enable_register) ;
   pthread_mutex_unlock (&config_file_mutex);
   return iRet;
}

void linphonec_set_call_state_changed_cb(CB_CALL_STATE_CHANGE pstCb)
{
    pfCallStateChange = pstCb;
}

void linphonec_set_reg_state_changed_cb(CB_REG_STATE_CHANGE pstCb)
{
    pfRegStateChange = pstCb;
}

void linphonec_set_reg_okfailed_notify_cb(CB_REG_OK_FAILED_NOTIFY pstCb)
{
    pfRegOkFailedNotify = pstCb;
}

void linphonec_set_dtmf_recv_cb(CB_DTMF_RECV pstCb)
{
    pfDtmfRecv = pstCb;
}

void linphonec_set_call_failure_cb(CB_CALL_FAILED pstCb)
{
    pfCallFailed = pstCb;
}

void linphonec_set_msg_handle_support(CB_MSG_HANDLE_SUPPORT pstCb)
{
    pfMsgHandleSupport = pstCb;
}

void linphonec_set_default_reg(CB_DEFAULT_REG pstCb)
{
    pfDefaultReg = pstCb;
}


//add by chensq 20121022 for gyy
void linphonec_set_message_recv_cb(CB_MESSAGE_RECV pstCb)
{
    pfMessageRecv = pstCb;
}

//add by chensq 20121112
void linphonec_set_event_process_cb(CB_EVENT_PROCESS pstCb)
{
    pfEventProcess = pstCb;
}


#ifdef HINT_MUSIC
void linphonec_set_hint_music_done_cb(CB_HINT_MUSIC_DONE cb)
{
	pfHintMusicDone = cb;
}
#endif

//------------------------------------------

/****************************************************************************
 *
 * $Log: linphonec.c,v $
 * Revision 1.57  2007/11/14 13:40:27  smorlat
 * fix --disable-video build.
 *
 * Revision 1.56  2007/09/26 14:07:27  fixkowalski
 * - ANSI/C++ compilation issues with non-GCC compilers
 * - Faster epm-based packaging
 * - Ability to build & run on FC6's eXosip/osip
 *
 * Revision 1.55  2007/09/24 16:01:58  smorlat
 * fix bugs.
 *
 * Revision 1.54  2007/08/22 14:06:11  smorlat
 * authentication bugs fixed.
 *
 * Revision 1.53  2007/02/13 21:31:01  smorlat
 * added patch for general state.
 * new doxygen for oRTP
 * gtk-doc removed.
 *
 * Revision 1.52  2007/01/10 14:11:24  smorlat
 * add --video to linphonec.
 *
 * Revision 1.51  2006/08/21 12:49:59  smorlat
 * merged several little patches.
 *
 * Revision 1.50  2006/07/26 08:17:28  smorlat
 * fix bugs.
 *
 * Revision 1.49  2006/07/17 18:45:00  smorlat
 * support for several event queues in ortp.
 * glib dependency removed from coreapi/ and console/
 *
 * Revision 1.48  2006/04/09 12:45:32  smorlat
 * linphonec improvements.
 *
 * Revision 1.47  2006/04/04 08:04:34  smorlat
 * switched to mediastreamer2, most bugs fixed.
 *
 * Revision 1.46  2006/03/16 17:17:40  smorlat
 * fix various bugs.
 *
 * Revision 1.45  2006/03/12 21:48:31  smorlat
 * gcc-2.95 compile error fixed.
 * mediastreamer2 in progress
 *
 * Revision 1.44  2006/03/04 11:17:10  smorlat
 * mediastreamer2 in progress.
 *
 * Revision 1.43  2006/02/13 09:50:50  strk
 * fixed unused variable warning.
 *
 * Revision 1.42  2006/02/02 15:39:18  strk
 * - Added 'friend list' and 'friend call' commands
 * - Allowed for multiple DTFM send in a single line
 * - Added status-specific callback (bare version)
 *
 * Revision 1.41  2006/02/02 13:30:05  strk
 * - Padded vtable with missing callbacks
 *   (fixing a segfault on friends subscription)
 * - Handled friends notify (bare version)
 * - Handled text messages receive (bare version)
 * - Printed message on subscription request (bare version)
 *
 * Revision 1.40  2006/01/26 09:48:05  strk
 * Added limits.h include
 *
 * Revision 1.39  2006/01/26 02:11:01  strk
 * Removed unused variables, fixed copyright date
 *
 * Revision 1.38  2006/01/25 18:33:02  strk
 * Removed the -t swich, terminate_on_close made the default behaviour
 *
 * Revision 1.37  2006/01/20 14:12:34  strk
 * Added linphonec_init() and linphonec_finish() functions.
 * Handled SIGINT and SIGTERM to invoke linphonec_finish().
 * Handling of auto-termination (-t) moved to linphonec_finish().
 * Reworked main (input read) loop to not rely on 'terminate'
 * and 'run' variable (dropped). configfile_name allocated on stack
 * using PATH_MAX limit. Changed print_usage signature to allow
 * for an exit_status specification.
 *
 * Revision 1.36  2006/01/18 09:25:32  strk
 * Command completion inhibited in proxy addition and auth request prompts.
 * Avoided use of readline's internal filename completion.
 *
 * Revision 1.35  2006/01/14 13:29:32  strk
 * Reworked commands interface to use a table structure,
 * used by command line parser and help function.
 * Implemented first level of completion (commands).
 * Added notification of invalid "answer" and "terminate"
 * commands (no incoming call, no active call).
 * Forbidden "call" intialization when a call is already active.
 * Cleaned up all commands, adding more feedback and error checks.
 *
 * Revision 1.34  2006/01/13 13:00:29  strk
 * Added linphonec.h. Code layout change (added comments, forward decl,
 * globals on top, copyright notices and Logs). Handled out-of-memory
 * condition on history management. Removed assumption on sizeof(char).
 * Fixed bug in authentication prompt (introduced by readline).
 * Added support for multiple authentication requests (up to MAX_PENDING_AUTH).
 *
 *
 ****************************************************************************/

