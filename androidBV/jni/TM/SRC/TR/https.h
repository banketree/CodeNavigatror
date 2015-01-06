/*
 * https.h
 *
 *  Created on: 2012-12-28
 *      Author: root
 */

#ifndef HTTPS_H_
#define HTTPS_H_

char* mystrstr1(const char *s1, const char *s2,int *outlen);

char* mystrstr(const char *s1, const char *s2,int *outlen);

int StringFindFirst(unsigned char* pvBuffer, int iSize, char c);

/********************************************
功能：搜索字符串右边起的第一个匹配字符
********************************************/
char *Rstrchr(char *s, char x);

/**************************************************************
功能：从字符串src中分析出网站地址和端口，并得到用户要下载的文件
***************************************************************/
int GetHost(char *src, char *web, char *file, int *port,int *flag);

int FindParamFromBuf(const char *buf_in, const char *pItemStr, char *buf_out);

#endif /* HTTPS_H_ */
