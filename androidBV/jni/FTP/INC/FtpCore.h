/**
 *@addtogroup FTPCore
 *@{
 */

/**
 *@file ftp.h
 *@brief Out going other module's API 
 *
 *All APIs which needed by current module's go here
 *@author Amos
 *@version 1.0
 *@date 2012-04-27
 */

#ifndef __FTP_H__
#define __FTP_H__

#include <YKSystem.h>
#include <YKApi.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef enum {
    FP_TRANSFER_OP_COVER=0, 
    FP_TRANSFER_OP_RESUME, 
    FP_TRANSFER_OP_SKIP
}FP_TRANSFER_OP_EN;

typedef struct _TransferInfo{
    long     lTotalSize;
    long     lTransferredSize;
    long     lRemoteFileSize;
    long     lLocalFileSize;
    long     lTotalTime;
    double   dMaxSpeed;
    double   dAvlSpeed;
    double   dMinSpeed;
}FP_TRANSFER_INFO_ST;
typedef void (*FPTranaferSpeedFunc)(FP_TRANSFER_INFO_ST* pstInfo);

typedef enum _FPCommand{
    FP_COMMAND_HOST=0,
    FP_COMMAND_USER,
    FP_COMMAND_PASS,
    FP_COMMAND_TYPE,
    FP_COMMAND_PASV,
    FP_COMMAND_SIZE,
    FP_COMMAND_RETR,
    FP_COMMAND_STOR,
    FP_COMMAND_APPE,
    FP_COMMAND_ABOR,
    FP_COMMAND_RNFR,
    FP_COMMAND_RNTO,
}FP_COMMAND_EN;
typedef void (*FPCommandStateFunc)(FP_COMMAND_EN enCode, const char* pcMessage);

typedef struct __FPCallbackVtable{
    FPCommandStateFunc    CommandState;
    FPTranaferSpeedFunc   TransSpeed;
}FP_CALLBACK_VTABLE_ST;

YKHandle FPOpen(
    const char* pacHost, 
    int         iPort, 
    const char* pcUserName, 
    const char* pcPassWord, 
    FP_CALLBACK_VTABLE_ST* pstVtable);

typedef enum _FPTransState{
    FP_TRANSFER_START=0,
    FP_TRANSFER_CONTINUE,
    FP_TRANSFER_FINISH,
    FP_TRANSFER_ERROR,
}FP_TRANSFER_EN;

typedef enum _FPTransMode{
    FP_TRANSFER_MODE_BINARY=0,
    FP_TRANSFER_MODE_ASCII,
}FP_TRANS_MODE_EN;

FP_TRANSFER_EN FPDownload(
    YKHandle    hHandle, 
    const char* pcLocal, 
    const char* pcRemote, 
    FP_TRANS_MODE_EN enMode,
    BOOL    bPause, 
    int     iPauseTime);

FP_TRANSFER_EN FPContinueDownload(YKHandle hHandle);

FP_TRANSFER_EN FPUpload(
    YKHandle    hHandle, 
    const char* pcLocal, 
    const char* pcRemote, 
    FP_TRANS_MODE_EN enMode,
    BOOL    bPause, 
    int     iPauseTime);
FP_TRANSFER_EN FPContinueUpload(YKHandle hHandle);
FP_TRANSFER_EN FPCreateDir(const char* pcFolder);
FP_TRANSFER_EN FPRename(YKHandle handle, const char* src, const char* dest);
void FPClose(YKHandle hHandle);

typedef int (*FPSendFunc)(YKHandle hHandle, void* pvBuffer, int iSize);
typedef int (*FPRecvFunc)(YKHandle hHandle, void* pvBuffer, int iSize);

typedef struct _FPCore{
    YKHandle            LinkCommand;
    YKHandle            LinkData;
    FP_TRANSFER_INFO_ST TransInfo;
    FP_TRANSFER_EN      TransState;
    char                Server[64];
    int                 PortCommand;
    int                 PortData;
    BOOL                Pause;
    int                 PauseTime;
    YK_TIME_VAL_ST           PrePauseVal;
    YK_TIME_VAL_ST           PreCalcSpeed;
    FP_CALLBACK_VTABLE_ST    Callback;
    FP_TRANSFER_OP_EN        Op;
    FILE*               LocalFile;
    FPSendFunc          Send;
    FPRecvFunc          Recv;
    char				BufferCommand[1024];
}FP_CORE_ST;

typedef int (*FPCProcess)(FP_CORE_ST* hHandle, void* pvData, int iSize, int iReturnCode);

typedef struct __FPCCommand{
    FP_COMMAND_EN   enType;
    char*           Context;
    int             ReturnCode;
    FPCProcess      Process;
}FP_COMMAND_ST;

#ifdef __cplusplus
};
#endif

#endif//!__FTP_H__

/**@}*///FTPCore
