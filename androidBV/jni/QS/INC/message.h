
#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "type.h"

#define SIZE_WND_MAILBOX		1024

typedef struct _MSG {     // msg 
	HWND	hWnd;     
	UINT	message; 
	WPARAM	wParam; 
	LPARAM	lParam; 
	void*	pData;
} MSG;
typedef MSG* PMSG;


typedef struct _NtfMsgLink
{
	MSG		msg;
	struct _NtfMsgLink* pNext;
} NtfMsgLink;
typedef NtfMsgLink* PNtfMsgLink;


typedef struct _SyncMsgLink
{
	MSG		msg;
	int		iRetValue;
	sem_t	sem;
	struct _SyncMsgLink* pNext;
} SyncMsgLink;

typedef SyncMsgLink* PSyncMsgLink;


typedef struct _WndMailBox
{
	MSG			msg[SIZE_WND_MAILBOX];
	int			iReadPos;
	int			iWritePos;
} WndMailBox;
typedef WndMailBox* PWndMailBox;



typedef struct _MsgQueue
{
	pthread_mutex_t 	mutex;       
	sem_t			sem;  

	unsigned long			dwState;              
         
	PSyncMsgLink		pHeadSyncMsg;
	PSyncMsgLink		pTailSyncMsg;

	PNtfMsgLink		pHeadNtfMsg;
	PNtfMsgLink		pTailNtfMsg;

	PNtfMsgLink		pHeadPntMsg;
	PNtfMsgLink		pTailPntMsg;

	WndMailBox		wndMailBox;

} 	MsgQueue;
typedef MsgQueue* 	PMsgQueue;


BOOL InitMsgQueue(void);

void DestroyMsgQueue(void);


BOOL GetMessage(PMSG pMsg);


BOOL PostMessage(int iMsg, WPARAM wParam, LPARAM lParam);

BOOL PostQuitMessage(void);

int SendMessage(int iMsg, WPARAM wParam, LPARAM lParam);


BOOL SendNotifyMessage(int iMsg, WPARAM wParam, LPARAM lParam);

BOOL SendPaintMessage(void);

BOOL TranslateMessage(PMSG pMsg);

int PostSyncMessage(int msg, WPARAM wParam, LPARAM lParam);
int DispatchMessage(PMSG pMsg);
#endif



