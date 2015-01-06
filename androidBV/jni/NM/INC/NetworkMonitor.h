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
 *@brief ����ֵ�ɹ�ʧ�ܶ���.
 */
#ifndef RST_OK
#define RST_OK  0
#endif

#ifndef RST_ERR
#define RST_ERR  -1
#endif


/**
 *@brief	������ģ�����.
 *@return	RST_OK �ɹ�  RST_ERR ʧ��
 */
int NMInit(void);

/**
 *@brief	������ģ���˳�.
 */
void NMFini(void);		

/**
 *@brief	֪ͨ��������.
 */
void NMNotifyInfo(void);

/**
 *@brief		������֧��.
 *@param[in]:	const char *cmd ����
 *@return		RST_OK �ɹ�  RST_ERR ʧ��
 */
int NMProcessCommand(const char *cmd);

int NMGetPPPoeIpAddress(char *pcIP);

int NMGetIpAddress(char *pcIP);

int NMGetLocalMacByName(char *pcMac,char *pcName);

void NMUpdateInfo(void);

#endif /* NETLINK_H_ */
