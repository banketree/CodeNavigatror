/**
 *@file 	SMSerial.h
 *@brief 	���ڲ�����غ���
 *@author 	cx
 *@version 	1.0
 *@date 	2012-05-10
 */

#ifndef SM_SERIAL_H_
#define SM_SERIAL_H_

/**
 *@brief 		�򿪴��ڲ������������
 *@param[in] 	ucComPort 		��򿪵Ķ˿ں�
 *@param[in] 	iBaudrate		������
 *@param[in] 	iDatabit		����λ
 *@param[in] 	pcStopbit		ֹͣλ
 *@param[in] 	cParity			У��λ
 *@return 		-1-��ʧ��	>0-�򿪵Ĵ����豸�ļ�������
 */
int SMOpenComPort (unsigned char ucComPort, int iBaudrate, int iDatabit, const char *pcStopbit, char cParity);

/**
 *@brief 		��USB�˿ڲ������������
 *@param[in] 	ucComPort 		��򿪵Ķ˿ں�
 *@param[in] 	iBaudrate		������
 *@param[in] 	iDatabit		����λ
 *@param[in] 	pcStopbit		ֹͣλ
 *@param[in] 	cParity			У��λ
 *@return 		-1-��ʧ��	>0-�򿪵�USB�˿��豸�ļ�������
 */
int SMOpenUsbPort (unsigned char ucComPort, int iBaudrate, int iDatabit, const char *pcStopbit, char cParity);

/**
 *@brief 		�رն˿�
 *@param[in] 	iFd 	Ҫ�رյĶ˿��豸�ļ�������
 *@return 		��
 */
void SMCloseComPort (int iFd);

/**
 *@brief 		��ȡ��������
 *@param[in] 	iFd 		�ļ�������
 *@param[out] 	pvData		��ȡ������
 *@param[in] 	iDataLen	Ҫ��ȡ���ݵĳ���
 *@return 		�������ݵ�ʵ�ʳ���
 */
int SMReadComPort(int iFd, void *pvData, int iDataLen);

/**
 *@brief 		д�봮������
 *@param[in] 	iFd 		�ļ�������
 *@param[out] 	pvData		Ҫд�������
 *@param[in] 	iDataLen	д�����ݵĳ���
 *@return 		д�������ʵ�ʳ���
 */
int SMWriteComPort(int iFd, char *pvData, int iDataLen);

#endif /* SM_SERIAL_H_ */
