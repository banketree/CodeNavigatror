/**
 *@addtogroup TManage
 *@{
 */

/**
 *@file TerminalManage.h
 *@brief TM module's protect APIs defines.
 *
 *All APIs which needed by TM module's go here
 *@author Amos
 *@version 1.0
 *@date 2012-04-27
 */

#ifndef __TERMINATE_MANAGE_H__
#define __TERMINATE_MANAGE_H__

#include <YKSystem.h>
#include "TMProcotol.h"

#if _TM_TYPE_ == _YK069_

#ifdef __cplusplus
extern "C"{
#endif

/**
 *@name Callback function table.
 *@{
 */

/**@brief Connect remote server state.*/
typedef enum{
    TM_START_TO_CONNECT_SERVER=0,   /**<! Start to connect server. */
    TM_CONNECT_SERVER_FAILED,      /**<! Connect server failed. */
    TM_CONNECT_SERVER_SUCCESS,     /**<! Connect server success. */
    TM_RECONNECT_SERVER_START,     /**<! Reconnect server. */
    TM_DISCONNECT_BY_LOCAL,        /**<! Disconnect by local. */
    TM_DISCONNECT_BY_REMOTE,       /**<! Disconnect by remote. */
    TM_DISCONNECT_BY_NO_OPTION,     /**<! Disconnect by no heart. */
}TM_CONNECT_STATE_EN;

/**
 *@brief Out put current server connect state.
 *
 *This function will called when TM request/received connect or disconnect messages. 
 *@param[out] state The state of connect server.
 *@param[out] message some messages.
 *@return void
 */
typedef void(*TMConnectFunc)(TM_CONNECT_STATE_EN enState,  const char* pcMessage);

/**@brief Out put state when config changed. */
typedef enum{
    TM_CONFIG_CREAT=0, /**<! Create config. */ 
    TM_CONFIG_READ,   /**<! Read information from config. */ 
    TM_CONFIG_WRITE,  /**<! Write information to config. */ 
    TM_CONFIG_CLOSE,  /**<! Close and sync data to storage. */
}TM_CONFIG_STATE_EN;

/**
 *@brief Out put current state of config operate.
 *
 *This function will called when config changed. 
 *@param[out] state The state of config operate.
 *@param[out] message Some messages.
 *@return void
 */
typedef void(*TMConfigFunc)(TM_CONFIG_STATE_EN enState, const char* pcMessage);

/**@brief Remote transfer type. */
typedef enum{
    TM_REMOTE_DOWNLOAD=0, /**<!Download type.*/ 
    TM_REMOTE_UPLOAD      /**<!Upload type.*/
}TM_TRANSFER_TYPE_EN;

/**@brief Transfer file type.*/
typedef enum{
    TM_TYPE_EVENT_LOG=0,
    TM_TYPE_SYSTEM_LOG,
    TM_TYPE_CONFIG,
    TM_TYPE_RINGING,
    TM_TYPE_UPDATE,
    TM_TYPE_OTHER
}TM_TRANSFER_FILE_TYPE_EN;

/**
 *@brief Notify remote transfer start.
 *
 *This function will called when received remote transfer message.
 *@param[out] type Transfer type.
 *@param[out] localUrl Terminate file path.
 *@param[out] remoteUrl Remote server file path.
 *@return void
 */
typedef void(*TMRemoteTransferFunc)(
    TM_TRANSFER_TYPE_EN enType,TM_TRANSFER_FILE_TYPE_EN enFileType, 
    const char* pcLocalUrl, const char* pcRemoteUrl);

/**@brief Register to terminate manage platform state. */
typedef enum{
    TM_REGISTER_ACCEPT=0, /**<!Accept by TMP*/
    TM_REGISTER_PROCESS,  /**<!Register process. */
    TM_REGISTER_ERROR,    /**<!Register error.*/ 
    TM_REGISTER_REJECT,   /**<!Reject by TMP.*/ 
    TM_REGISTER_UNREGISTER,   /**<!Unknow State.*/
    TM_REGISTER_QUIT,
}TM_REGISTER_STATE_EN;

/**
 *@brief Out put register state.
 *
 *This function will called when PM register to TMP happened.
 *@param[out] state Register state.
 *@param[out] message Some message.
 *@return void.
 */
typedef void(*TMRegisterFunc)(TM_REGISTER_STATE_EN enState, const char* pcMessage);

/**
 *@brief Out put message which received from remote.
 *@param[out] command Command type.*
 *@param[out] message Command context.
 *@return void.
 */
typedef void(*TMCommandFunc)(int iType, int iCommand, const char* pcMessage);

/**@brief Notify when SIP\FTP\WAN information changed.*/
typedef enum{
    TM_CONFIG_SIP_CHANGED=0, /**<! Sip changed. */ 
    TM_CONFIG_FTP_CHANGED, /**<! Ftp changed. */ 
    TM_CONFIG_WAN_CHANGED  /**<! Wan changed. */
}TM_COMMUNICATE_CHANGED_EN;

/**
 *@brief Notify communicate changed.
 *@param[out] id Changed id.
 *@return void.
 */
typedef void(*TMCommunicateChangedFunc)(TM_COMMUNICATE_CHANGED_EN enID);

/**@brief Reboot of cause.*/
typedef enum{
    TM_REBOOT_BY_REMOTE=0,   /**<! Reboot by Remote. */ 
    TM_REBOOT_BY_UNKNOW      /**<! Unknow. */
}TM_REBOOT_CODE_EN;

/**@brief Out put cause of reboot*/
typedef void(*TMRebootSysFunc)(TM_REBOOT_CODE_EN enCode, const char* pcMessage);

/**@brief This function called when TM exit. */
typedef void(*TMQuitModuleFunc)(int iCode, const char* pcMessage);

typedef struct __CoreVtable{
    TMConnectFunc               ConnectState;
    TMConfigFunc                ConfigState;
    TMCommandFunc               CommandState;
    TMRemoteTransferFunc        RemoteTransfer;
    TMCommunicateChangedFunc    CommunicateChanged;
    TMRegisterFunc              RegisterState;
    TMRebootSysFunc             Reboot;
    TMQuitModuleFunc            Quit;
}TM_CORE_VTABLE_ST;

/**@}*///Callback function table

/**
 *@name Protected function table
 *@{
 */
/**@brief Init terminate manage. */
BOOL TMInitTerminateManage(const char* pcVersion, const char* pcConfig);
void TMFiltersInit(TM_CORE_VTABLE_ST* pstCoreVtable);
void TMFiltersPreProcess();
void TMFiltersProcess();
void TMFiltersPostProcess();

typedef enum{
    TM_WARNING=0, 
    TM_MESSAGE, 
    TM_DEBUG, 
    TM_ERROR
}TM_LOG_LEVEL_EN;
void TMSystemLogOut(TM_LOG_LEVEL_EN enLevel, const char* pcMessage, ...);

void TMSendAlarm(TM_ALARM_ID_EN enModule, char cValue);

/**@}*///Protected function table

#ifdef __cplusplus
};
#endif

#endif

#endif//!__TERMINATE_MANAGE_H__


/**@}*///TManage
