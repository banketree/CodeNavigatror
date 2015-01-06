/**
 *@file 	SMUtil.h
 *@brief 	南向接口组件函数模块
 *@author 	cx
 *@version 	1.0
 *@date 	2012-05-10
 */

#ifndef SM_UTIL_H_
#define SM_UTIL_H_


/**
 *@brief 		计算输入字节数据的CRC校验码
 *@param[in] 	pcData 			待计算的数据
 *@param[in] 	ulDataLen		待计算的数据长度
 *@param[in] 	usCrcInitVal	CRC初始值
 *@return 		二字节的CRC校验码
 */
unsigned short SMCalcuCRC(const char* pcData, unsigned long ulDataLen, unsigned short usCrcInitVal);

/**
 *@brief 		计算输入字节数据的BCC校验码
 *@param[in] 	pcData 			待计算的数据
 *@param[in] 	ulDataLen		待计算的数据长度
 *@return 		单字节的BCC校验码
 */
unsigned char SMCalcuBCC(const char *acData, int iDataLen);

/**
 *@brief 		找出输入字符串某字符第一次出现的位置
 *@param[in] 	pucStr 		输入的字符串
 *@param[in] 	iLen		字符串长度
 *@param[in] 	ucChar		需要找出的字符
 *@return 		该字符在字符串中的偏移数（左起）
 */
int SMFindChar(unsigned char *pucStr, int iLen, unsigned char ucChar);

/**
 *@brief 		十六进制字符串转十进制数字
 *@param[in] 	pcHex 	输入的十六进制字符串
 *@return 		十进制数字
 */
int SMHexAsciiToDecimal(char* pcHex);

/**
 *@brief 		验证是否是十六进制字符串
 *@param[in] 	pcHexStr 	输入的十六进制字符串
 *@param[in] 	iSize		输入的字符串长度
 *@return 		TRUE-是	FALSE-否
 */
int SMVerifyHexAscii(const char* pcHexStr, int iSize);

/**
 *@brief 		字符串分离
 *@param[out] 	pucData 	待分离的字符串
 *@param[in] 	uiDataLen	字符串长度
 *@return 		无
 */
void SMSplitToBytes(unsigned char *pucData, unsigned int uiDataLen);

/**
 *@brief 		统计某字符在字符串中出现的个数
 *@param[in] 	pcStr 	待统计的字符串
 *@param[in] 	pcStr 	待统计的字符串长度
 *@param[in] 	cChr    待统计的字符
 *@return 		无
 */
int SMStrChrCnt(char *pcStr, int iStrLen, char cChr);

#endif /* SM_UTIL_H_ */
