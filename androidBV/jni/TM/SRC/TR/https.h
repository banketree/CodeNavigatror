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
���ܣ������ַ����ұ���ĵ�һ��ƥ���ַ�
********************************************/
char *Rstrchr(char *s, char x);

/**************************************************************
���ܣ����ַ���src�з�������վ��ַ�Ͷ˿ڣ����õ��û�Ҫ���ص��ļ�
***************************************************************/
int GetHost(char *src, char *web, char *file, int *port,int *flag);

int FindParamFromBuf(const char *buf_in, const char *pItemStr, char *buf_out);

#endif /* HTTPS_H_ */
