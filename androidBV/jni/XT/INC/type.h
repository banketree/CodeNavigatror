//type define
#ifndef _TYPE_H_
#define _TYPE_H_

#ifndef FALSE
#define FALSE       0
#endif
#ifndef TRUE
#define TRUE        1
#endif

#ifndef false
#define false       0
#endif
#ifndef true
#define true        1
#endif

#ifndef NULL
#define NULL                ((void *)0)
#endif

#define LONG long
#define VOID        void

typedef int				BOOL;
//typedef unsigned char		BYTE;
typedef signed char		SBYTE;
//typedef unsigned short		WORD;
typedef signed short		SWORD;
//typedef unsigned long		DWORD;
typedef signed long		SDWORD;
typedef unsigned int		UINT;

typedef unsigned long		UINT32;
typedef unsigned short		UINT16;
typedef unsigned char		UINT08;

typedef char*			LPCSTR;
typedef UINT  			WPARAM;
typedef unsigned long			LPARAM;

typedef int				HCURSOR;
typedef int				HICON;
typedef int				HINSTANCE;
typedef int				HACCEL;
typedef int				HHOOK;
typedef void				*PVOID,*LPVOID;

typedef PVOID			HWND;
typedef PVOID			HANDLE;
typedef HWND			HDLG;
typedef PVOID			HMENU;


typedef PVOID			HGDIOBJ;
typedef HGDIOBJ			HPEN;
typedef HGDIOBJ			HBRUSH;
typedef HGDIOBJ			HFONT;
typedef HGDIOBJ			HBITMAP;

typedef LONG 			LRESULT;
typedef LRESULT			(*WNDPROC)(HWND,UINT,WPARAM,LPARAM);

#define LMSG_QUIT						0x5000
#define LMSG_PAINT						0x5001
#define LMSG_NCPAINT					0x5002
#define LMSG_SETFOCUS					0x5003
#define LMSG_KILLFOCUS					0x5004
#define LMSG_CREATE						0x5005
#define LMSG_DESTROY					0x5006
#define LMSG_COMMAND					0x5007
#define LMSG_SRVEXIT					0x5008
#define LMSG_ERASEBKGND				0x5009
#define LMSG_ACTIVEWINDOW				0x500A
#define LMSG_DISACTIVEWINDOW			0x500B
#define LMSG_ENABLE						0x500C
#define LMSG_TIMER						0x500D
#define LMSG_SHOWWINDOW				0x500E
#define LMSG_VSCROLL					0x500F
#define LMSG_HSCROLL					0x5010
#define LMSG_GETTEXT 					0x5011
#define LMSG_SETTEXT 					0x5012

#define LMSG_CLOSE						0x5014
#define LMSG_OK							0x5015

#define LMSG_GETTEXTLENGTH			0x5016
#define LMSG_RESETIME				0X5017

#define LMSG_RECVIDEO				0X5018            //对讲视频
#define CIF_WINDOW  1
#define D1_WINDOW  2
#define STOP_REC  3

#define LMSG_PLAYAUDIO				0X5019            //对讲音频
#define START_REC_AUDIO  1
#define STOP_REC_AUDIO  2
#define START_PLAY_AUDIO  3
#define STOP_PLAY_AUDIO  4

#define LMSG_SHOW_WINDOW				0X5020            //显示窗口
#endif
