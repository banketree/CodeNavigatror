/**
 *@file 	SMUimCard.h
 *@brief 	读卡器模块
 *@author 	cx
 *@version 	1.0
 *@date 	2012-05-10
 */

#ifndef SM_RF_CARD_H_
#define SM_RF_CARD_H_

#include <IDBT.h>
#include "SMUtil.h"

//#define _DEBUG

//读卡器串口相关设置
#ifdef NEW_CARD_READER
#define CARD_READER_COM_BAUD							9600		//波特率
#else
#define CARD_READER_COM_BAUD							115200		//波特率
#endif
#define CARD_READER_COM_DATABIT						    8			//数据位
#define CARD_READER_COM_STOPBIT						    "1"			//停止位
#define CARD_READER_COM_PARITYBIT						'N'			//奇偶校验位

#define TM_READ_INTERVAL								220			//读卡器循环读取间隔时间（毫秒）


typedef enum
{
	RFIDCARD_VALID,
	RFIDCARD_INVALID
}RFIDCARD_VALIDATE_EN;

/**
 *@brief 		读卡器模块初始化
 *@param[in] 	无
 *@return 		RST_OK - 成功 RST_ERR-失败
 */
int SMRfInit(void);

/**
 *@brief 		读卡器模块相关资源释放
 *@param[in] 	无
 *@return 		无
 */
void SMRfFini(void);


#endif //SM_RF_CARD_H_
