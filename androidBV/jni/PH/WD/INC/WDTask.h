/*********************************************************************
*  Copyright (C), 2001 - 2012, �����ʿ�ͨ�ż������޹�˾.
*  File			:
*  Author		zlin
*  Version 		:
*  Date			:
*  Description	:
*  History		: ���¼�¼ .
*            	  <�޸�����>  <�޸�ʱ��>  <�޸�����>
**********************************************************************/
#ifndef WDTASK_H_
#define WDTASK_H_
//ͷ�ļ�����


//CC �ڲ���ʱ����ϢID

//ȫ�ֱ�������
extern YK_MSG_QUE_ST*  g_pstWDMsgQ;

//��������
//���غ�������
int WDTaskInit(void);
void WDTaskFini(void);

int WDFeedDogFormLP(void);
//�ⲿ��������
#endif
