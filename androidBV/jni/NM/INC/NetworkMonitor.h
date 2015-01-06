/**
 *@addtogroup NMonitor
 *@{
 */

/**
 *@file NetorkMonitor.h
 *@brief NM module's protect APIs defines.
 *
 *All APIs which needed by TM module's go here
 *@author hb
 *@version 1.0
 *@date 2012-05-7
 */

#ifndef NETWORK_MONITOR_H_
#define NETWORK_MONITOR_H_

#include <YKApi.h>

#if _YK_OS_==_YK_OS_WIN32_
#pragma comment(lib, "../bin/libTerminalManage.lib")
#pragma comment(lib, "../bin/libykcrt.lib")
#pragma comment(lib,"Ws2_32.lib")
#endif

/**
 *@brief 返回值成功失败定义.
 */
#ifndef RST_OK
#define RST_OK  0
#endif

#ifndef RST_ERR
#define RST_ERR  -1
#endif


/**
 *@brief	网络监控模块进入.
 *@return	RST_OK 成功  RST_ERR 失败
 */
int NMInit(void);

/**
 *@brief	网络监控模块退出.
 */
void NMFini(void);		

/**
 *@brief	通知更新数据.
 */
void NMNotifyInfo(void);

/**
 *@brief		命令行支持.
 *@param[in]:	const char *cmd 命令
 *@return		RST_OK 成功  RST_ERR 失败
 */
int NMProcessCommand(const char *cmd);

int NMGetPPPoeIpAddress(char *pcIP);

int NMGetIpAddress(char *pcIP);

int NMGetLocalMacByName(char *pcMac,char *pcName);

void NMUpdateInfo(void);

#endif /* NETLINK_H_ */
