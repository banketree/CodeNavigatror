/**
 *@file 	SMUtil.h
 *@brief 	����ӿ��������ģ��
 *@author 	cx
 *@version 	1.0
 *@date 	2012-05-10
 */

#ifndef SM_UTIL_H_
#define SM_UTIL_H_


/**
 *@brief 		���������ֽ����ݵ�CRCУ����
 *@param[in] 	pcData 			�����������
 *@param[in] 	ulDataLen		����������ݳ���
 *@param[in] 	usCrcInitVal	CRC��ʼֵ
 *@return 		���ֽڵ�CRCУ����
 */
unsigned short SMCalcuCRC(const char* pcData, unsigned long ulDataLen, unsigned short usCrcInitVal);

/**
 *@brief 		���������ֽ����ݵ�BCCУ����
 *@param[in] 	pcData 			�����������
 *@param[in] 	ulDataLen		����������ݳ���
 *@return 		���ֽڵ�BCCУ����
 */
unsigned char SMCalcuBCC(const char *acData, int iDataLen);

/**
 *@brief 		�ҳ������ַ���ĳ�ַ���һ�γ��ֵ�λ��
 *@param[in] 	pucStr 		������ַ���
 *@param[in] 	iLen		�ַ�������
 *@param[in] 	ucChar		��Ҫ�ҳ����ַ�
 *@return 		���ַ����ַ����е�ƫ����������
 */
int SMFindChar(unsigned char *pucStr, int iLen, unsigned char ucChar);

/**
 *@brief 		ʮ�������ַ���תʮ��������
 *@param[in] 	pcHex 	�����ʮ�������ַ���
 *@return 		ʮ��������
 */
int SMHexAsciiToDecimal(char* pcHex);

/**
 *@brief 		��֤�Ƿ���ʮ�������ַ���
 *@param[in] 	pcHexStr 	�����ʮ�������ַ���
 *@param[in] 	iSize		������ַ�������
 *@return 		TRUE-��	FALSE-��
 */
int SMVerifyHexAscii(const char* pcHexStr, int iSize);

/**
 *@brief 		�ַ�������
 *@param[out] 	pucData 	��������ַ���
 *@param[in] 	uiDataLen	�ַ�������
 *@return 		��
 */
void SMSplitToBytes(unsigned char *pucData, unsigned int uiDataLen);

/**
 *@brief 		ͳ��ĳ�ַ����ַ����г��ֵĸ���
 *@param[in] 	pcStr 	��ͳ�Ƶ��ַ���
 *@param[in] 	pcStr 	��ͳ�Ƶ��ַ�������
 *@param[in] 	cChr    ��ͳ�Ƶ��ַ�
 *@return 		��
 */
int SMStrChrCnt(char *pcStr, int iStrLen, char cChr);

#endif /* SM_UTIL_H_ */
