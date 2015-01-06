/*
 * SMDtmfSerial.h
 *
 *  Created on: 2012-10-11
 *      Author: Zhengwenxiang
 */
#ifndef SM_DTMF_SERIAL_H_
#define SM_DTMF_SERIAL_H_
/**
 *@brief 		DTMF模块初始化
 *@param[in] 	无
 *@return 		SUCCESS-成功	FAILURE-失败
 */
int SMDtmfInit(void);

/**
 *@brief 		释放DTMF模块占用资源
 *@param[in] 	无
 *@return 		无
 */
void SMDtmfFini(void);

#endif
