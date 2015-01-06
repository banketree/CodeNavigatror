/*********************************************************************
*  Copyright (C), 2001 - 2012, 福建邮科通信技术有限公司.
*  File			:
*  Author		zlin
*  Version 		:
*  Date			:
*  Description	:
*  History		: 更新记录 .
*            	  <修改作者>  <修改时间>  <修改描述>
**********************************************************************/
#ifndef WDTASK_H_
#define WDTASK_H_
//头文件包含


//CC 内部定时器消息ID

//全局变量声明
extern YK_MSG_QUE_ST*  g_pstWDMsgQ;

//函数声明
//本地函数声明
int WDTaskInit(void);
void WDTaskFini(void);

int WDFeedDogFormLP(void);
//外部函数声明
#endif
