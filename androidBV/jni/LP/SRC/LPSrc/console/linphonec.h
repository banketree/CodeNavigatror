/****************************************************************************
 *
 *  $Id: linphonec.h,v 1.3 2006/01/20 14:12:34 strk Exp $
 *
 *  Copyright (C) 2005  Sandro Santilli <strk@keybit.net>
 *
 ****************************************************************************
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#ifndef LINPHONEC_H
#define LINPHONEC_H 1

#ifdef HAVE_CONFIG_H
#include "linphone-config.h"
#endif

#ifdef HAVE_READLINE_H
#include <readline.h>
#define HAVE_READLINE
#else
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#define HAVE_READLINE
#endif
#endif
#ifdef HAVE_HISTORY_H
#include <history.h>
#else
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif
#endif

#include <sal.h>
#include <linphonecore.h>


#include "eXosip2/eXosip.h"


#undef PARAMS
/**************************************************************************
 *
 * Compile-time defines
 *
 **************************************************************************/

#define HISTSIZE 500		/* how many lines of input history */
#define PROMPT_MAX_LEN 256	/* max len of prompt string */
#define LINE_MAX_LEN 256	/* really needed ? */

/*
 * Define this to have your primary contact
 * as the prompt string
 */
/* #define IDENTITY_AS_PROMPT 1 */

/*
 * Time between calls to linphonec_idle_call during main
 * input read loop in microseconds.
 */
#define LPC_READLINE_TIMEOUT 1000000

/*
 * Filename of linphonec history
 */
#define LPC_HIST_FILE "~/.linphonec_history"

/*
 * Maximum number of pending authentications
 */
#define MAX_PENDING_AUTH 8

/**************************************************************************
 *
 *  Types
 *
 **************************************************************************/

/*
 * A structure which contains information on the commands this program
 * can understand.
 */
typedef int (*lpc_cmd_handler)(LinphoneCore *, char *);
typedef struct {
	char *name;		/* User printable name of the function. */
	lpc_cmd_handler func;	/* Function to call to do the job. */
	char *help;		/* Short help for this function.  */
	char *doc;		/* Long description.  */
} LPC_COMMAND;

typedef struct {
	int x,y,w,h;
	unsigned long wid;
	bool_t show;
	bool_t refresh;
} VideoParams;


extern VideoParams lpc_video_params;
extern VideoParams lpc_preview_params;
//extern LinphoneCoreVTable linphonec_vtable;

/***************************************************************************
 *
 *  Forward declarations
 *
 ***************************************************************************/

extern int linphonec_parse_command_line(LinphoneCore *lc, char *cl);
extern char *linphonec_command_generator(const char *text, int state);
void linphonec_main_loop_exit();
extern void linphonec_finish(int exit_status);
extern char *linphonec_readline(char *prompt);
void linphonec_set_autoanswer(bool_t enabled);
bool_t linphonec_get_autoanswer();
void linphonec_command_finished(void);
void linphonec_set_caller(const char *caller);
LinphoneCall *linphonec_get_call(long id);
void linphonec_call_identify(LinphoneCall* call);

extern bool_t linphonec_camera_enabled;




//----------------------------------------------------------------------------------------------------
//add by chensq 20120520
typedef void   (*CB_CALL_STATE_CHANGE)(LinphoneCore *pstLpCore, LinphoneCall *pstLpCall, LinphoneCallState enCallState);
typedef void   (*CB_REG_STATE_CHANGE)(struct _LinphoneCore *pstLpCore, LinphoneProxyConfig *pstLpProxyConfig,LinphoneRegistrationState enRegState, const char *pcMsg);
typedef void   (*CB_REG_OK_FAILED_NOTIFY)(LinphoneCore *pstLpCore, int iStatus, const char *pcCode);
typedef void   (*CB_DTMF_RECV)(LinphoneCore *pstLpCore, LinphoneCall *pstLpCall, int iDtmf);
typedef void   (*CB_HINT_MUSIC_DONE)(LinphoneCore *lc, LinphoneCall *call);
typedef void   (*CB_CALL_FAILED)(LinphoneCore *pstLpCore, LinphoneCall *pstLpCall,SalError enError, SalReason enReason, int iCode);
typedef char*  (*CB_MSG_HANDLE_SUPPORT)(void);
typedef char*  (*CB_DEFAULT_REG)(void);
typedef void   (*CB_MESSAGE_RECV)(LinphoneCore *lc, const LinphoneAddress *from, const char *pcMsg); //add by chensq 20121022 for gyy
typedef void   (*CB_EVENT_PROCESS)(Sal *sal, eXosip_event_t *ev);
extern CB_EVENT_PROCESS pfEventProcess;

//add by chensq 20120724
int linphonec_is_registered(void);
//add by chensq 20120724
void linphonec_set_registered(void);
//add by chensq 20120727
void linphonec_send_dtmf(char dtmf);

void linphonec_set_call_state_changed_cb(CB_CALL_STATE_CHANGE pfCb);
void linphonec_set_reg_state_changed_cb(CB_REG_STATE_CHANGE pfCb);
void linphonec_set_reg_okfailed_notify_cb(CB_REG_OK_FAILED_NOTIFY pfCb);
void linphonec_set_dtmf_recv_cb(CB_DTMF_RECV pfCb);
void linphonec_set_hint_music_done_cb(CB_HINT_MUSIC_DONE cb);
void linphonec_set_call_failure_cb(CB_CALL_FAILED pfCb);
void linphonec_set_msg_handle_support(CB_MSG_HANDLE_SUPPORT pfCb);
void linphonec_set_default_reg(CB_DEFAULT_REG pfCb);

LinphoneCore* linphonec_startup(void);
void linphonec_shutdown(void);

int linphonec_push_proxy(const char* username, const char* password,
        const char* proxy, const char* route, int expires, int enable_register);
//----------------------------------------------------------------------------------------------------



#endif /* def LINPHONEC_H */

/****************************************************************************
 *
 * $Log: linphonec.h,v $
 * Revision 1.3  2006/01/20 14:12:34  strk
 * Added linphonec_init() and linphonec_finish() functions.
 * Handled SIGINT and SIGTERM to invoke linphonec_finish().
 * Handling of auto-termination (-t) moved to linphonec_finish().
 * Reworked main (input read) loop to not rely on 'terminate'
 * and 'run' variable (dropped). configfile_name allocated on stack
 * using PATH_MAX limit. Changed print_usage signature to allow
 * for an exit_status specification.
 *
 * Revision 1.2  2006/01/14 13:29:32  strk
 * Reworked commands interface to use a table structure,
 * used by command line parser and help function.
 * Implemented first level of completion (commands).
 * Added notification of invalid "answer" and "terminate"
 * commands (no incoming call, no active call).
 * Forbidden "call" intialization when a call is already active.
 * Cleaned up all commands, adding more feedback and error checks.
 *
 * Revision 1.1  2006/01/13 13:00:29  strk
 * Added linphonec.h. Code layout change (added comments, forward decl,
 * globals on top, copyright notices and Logs). Handled out-of-memory
 * condition on history management. Removed assumption on sizeof(char).
 * Fixed bug in authentication prompt (introduced by readline).
 * Added support for multiple authentication requests (up to MAX_PENDING_AUTH).
 *
 *
 ****************************************************************************/
