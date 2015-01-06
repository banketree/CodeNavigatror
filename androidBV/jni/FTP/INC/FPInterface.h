/**
 *@addtogroup FTPCore
 *@{
 */

/**
 *@file FPInterface.h
 *@brief Out going other module's API 
 *
 *All APIs which needed by other module's go here
 *@author Amos
 *@version 1.0
 *@date 2012-04-27
 */

#ifndef __FP_INTERFACE_H__
#define __FP_INTERFACE_H__

#include <YKSystem.h>
#include "FtpCore.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef enum {
	FP_TRANSFER_NOWAIT=0,
	FP_TRANSFER_WAIT
}FP_TRANSFER_TYPE_EN;

/**
 *@name FP APIs.
 *@{
 */

/**@brief Transfer process callback func.*/
typedef void (*FPTransferProcessFunc)(FP_TRANSFER_INFO_ST* pstInfo);
/**@brief Transfer done callback func.*/
typedef void (*FPTransferDoneFunc)(YKHandle hHandle, BOOL bState, const char* pcLocal, const char* pcRemote);

/**
 *@brief Download application from remote.
 *@param[in] pcHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] pcLocal Local file path.
 *@param[in] remote Remote file path.
 *@param[in] func Transfer process. If not used set it NULL
 *@param[in] done Transfer done. If not used set it NULL
 */
void FPUpdateApplication(
    const char* pcHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Func, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType);

/**
 *@brief Download config from remote.
 *@param[in] pcHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] pcLocal Local file path.
 *@param[in] remote Remote file path.
 *@param[in] func Transfer process. If not used set it NULL
 *@param[in] done Transfer done. If not used set it NULL
 */
void FPUpdateConfig(
    const char* pcHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Func, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType);

/**
 *@brief Download ringing file from remote.
 *@param[in] pcHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] local Local file path.
 *@param[in] remote Remote file path.
 *@param[in] func Transfer process. If not used set it NULL
 *@param[in] done Transfer done. If not used set it NULL
 */
void FPUpdateRinging(
    const char* pcHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Func, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType);

void FPUpdateAdvert(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType);

/**
 *@brief Upload logs to remote.
 *@param[in] pcHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] local Local file path.
 *@param[in] remote Remote file path.
 *@param[in] func Transfer process. If not used set it NULL
 */
void FPPutLog(
    const char* pcHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Func, FPTransferDoneFunc done, FP_TRANSFER_TYPE_EN enTransferType);

/**
 *@brief Upload call capture Picture to pcRemote.
 *@param[in] pacHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] pcLocal Local file path.
 *@param[in] pcRemote Remote file path.
 *add by cs for capture picture 2012-11-16
 */
void FPPutPicture(
    const char* pacHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Process, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType);


/**
 *@brief Upload file/folders to remote.
 *@param[in] pcHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] local Local file path.
 *@param[in] remote Remote file path.
 *@param[in] func Transfer process. If not used set it NULL
 *@param[in] done Transfer done. If not used set it NULL
 */
void FPPut(
    const char* pcHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Func, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType);

/**
 *@brief Download file from remote.
 *@param[in] pcHost Server address.
 *@param[in] iPort Server iPort.
 *@param[in] pcUserName FTP acUserName.
 *@param[in] pcPassWord FTP acPassword.
 *@param[in] local Local file path.
 *@param[in] remote Remote file path.
 *@param[in] func Transfer process. If not used set it NULL
 *@param[in] done Transfer done. If not used set it NULL
 */
void FPGet(
    const char* pcHost, int iPort, 
    const char* pcUserName, const char* pcPassWord,
    const char* pcLocal, const char* pcRemote, 
    FPTransferProcessFunc Func, FPTransferDoneFunc done,FP_TRANSFER_TYPE_EN enTransferType);

typedef enum _FPCheckState{
    FP_CHECK_NORMAL=0,    /**<!For normal operation.*/
    FP_CHECK_UPGRADE,     /**<!Eligible for upgrade.*/
    FP_CHECK_ERROR,       /**<!Version error.*/
}FP_VERSION_CHECK_STATE_EN;

/**
 *@brief To detect the current version
 *
 *If is Upgrade version it will Comparison of old and new version 
 *number to determine whether you need to upgrade
 *@param[in] mark Upgraded version of the tag
 *@param[in] oldVersion Old version number
 *@return @see FP_VERSION_CHECK_STATE_EN 
 */
FP_VERSION_CHECK_STATE_EN FPCheckApplication(const char* pcMark, const char* pcOldVersion);

BOOL FPCRCForAppImage(char* image);

BOOL  FPTryToRunApplication(const char* acApp);

int FTPProcessCmdFunc(const char *cmd);
/**@}*///FP APIs.
#ifdef __cplusplus
};
#endif

#endif//!__FP_INTERFACE_H__

/**@}*///FTPCore
