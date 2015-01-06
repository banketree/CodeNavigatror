/**
 *@file    LPUtils.h
 *@brief   ����
 *@author  chensq
 *@version 1.0
 *@date    2012-5-17
 */

#ifndef LPUTILS_H_
#define LPUTILS_H_

#include "sd/list.h"
#include "sd/domnode.h"



#define LP_CHECK_NET_IP1 "115.239.210.26"
#define LP_CHECK_NET_IP2 "220.162.97.165"
#define LP_CHECK_NET_IP3 "196.196.196.1"

/**
 * @brief        �ؽ��ļ�
 * @param[in]    pcFileName �ļ���
 * @return       NULL������ʧ�ܣ����򷵻ص���FILE*�ļ�ָ��
 */
char* LPUtilsReCreateFile(const char* pcFileName);


int  LPUtilsLoadWav(char *pcFileName,  char *pcBuff, int pcBuffSize);

#endif /* LPUTILS_H_ */
