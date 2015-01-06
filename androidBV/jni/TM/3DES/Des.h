/*
 * Des.h
 *
 *  Created on: 2012-11-8
 *      Author: root
 */

#ifndef DES_H_
#define DES_H_

#define ENCRYPT	0
#define DECRYPT	1
#define ECB	0
#define CBC	1
#define PAD_ISO_1	0
#define PAD_ISO_2	1
#define PAD_PKCS_7	2
#define MAXBLOCK	1024
typedef char (*PSubKey)[16][48];

extern char* pcDesKey ;

extern char acDesIv[8];

extern int iPadMode;

int CovertKey(char *iKey, char *oKey);
void XPRsm(char *Text);
int XPPad(int nType,const char* In,unsigned in_len,char* Out,int* padlen);
int XP3Des(int bType, int bMode, const char *In, unsigned int in_len, const char *Key, unsigned int key_len, char* Out, unsigned int out_len, const char cvecstr[8],int padMode);
int XP1Des(int bType, int bMode, const char *In, unsigned int in_len, const char *Key, unsigned int key_len, char* Out, unsigned int out_len, const char cvecstr[8],int padMode);
void DES(char Out[8], const char In[8], const PSubKey pSubKey, int Type);
char* Base64Encode(char *src,int srclen);
char* Base64Decode(char *src);
char *Base64FromHex(char *pcBase64,int iLen);

#endif /* DES_H_ */
