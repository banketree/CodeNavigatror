/**
 *@file    LPUtils.h
 *@brief   工具
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
 * @brief        重建文件
 * @param[in]    pcFileName 文件名
 * @return       NULL代表创建失败，否则返回的是FILE*文件指针
 */
char* LPUtilsReCreateFile(const char* pcFileName);


int  LPUtilsLoadWav(char *pcFileName,  char *pcBuff, int pcBuffSize);

#endif /* LPUTILS_H_ */
