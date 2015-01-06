/**
 *@file 	SMUimCard.h
 *@brief 	������ģ��
 *@author 	cx
 *@version 	1.0
 *@date 	2012-05-10
 */

#ifndef SM_RF_CARD_H_
#define SM_RF_CARD_H_

#include <IDBT.h>
#include "SMUtil.h"

//#define _DEBUG

//�����������������
#ifdef NEW_CARD_READER
#define CARD_READER_COM_BAUD							9600		//������
#else
#define CARD_READER_COM_BAUD							115200		//������
#endif
#define CARD_READER_COM_DATABIT						    8			//����λ
#define CARD_READER_COM_STOPBIT						    "1"			//ֹͣλ
#define CARD_READER_COM_PARITYBIT						'N'			//��żУ��λ

#define TM_READ_INTERVAL								220			//������ѭ����ȡ���ʱ�䣨���룩


typedef enum
{
	RFIDCARD_VALID,
	RFIDCARD_INVALID
}RFIDCARD_VALIDATE_EN;

/**
 *@brief 		������ģ���ʼ��
 *@param[in] 	��
 *@return 		RST_OK - �ɹ� RST_ERR-ʧ��
 */
int SMRfInit(void);

/**
 *@brief 		������ģ�������Դ�ͷ�
 *@param[in] 	��
 *@return 		��
 */
void SMRfFini(void);


#endif //SM_RF_CARD_H_
