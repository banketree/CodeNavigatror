/*
 * SMDtmfSerial.h
 *
 *  Created on: 2012-10-11
 *      Author: Zhengwenxiang
 */
#ifndef SM_DTMF_SERIAL_H_
#define SM_DTMF_SERIAL_H_
/**
 *@brief 		DTMFģ���ʼ��
 *@param[in] 	��
 *@return 		SUCCESS-�ɹ�	FAILURE-ʧ��
 */
int SMDtmfInit(void);

/**
 *@brief 		�ͷ�DTMFģ��ռ����Դ
 *@param[in] 	��
 *@return 		��
 */
void SMDtmfFini(void);

#endif
