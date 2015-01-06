/**
 *@file 	SMSerial.h
 *@brief 	串口操作相关函数
 *@author 	cx
 *@version 	1.0
 *@date 	2012-05-10
 */

#ifndef SM_SERIAL_H_
#define SM_SERIAL_H_

/**
 *@brief 		打开串口并对其进行设置
 *@param[in] 	ucComPort 		需打开的端口号
 *@param[in] 	iBaudrate		波特率
 *@param[in] 	iDatabit		数据位
 *@param[in] 	pcStopbit		停止位
 *@param[in] 	cParity			校验位
 *@return 		-1-打开失败	>0-打开的串口设备文件描述符
 */
int SMOpenComPort (unsigned char ucComPort, int iBaudrate, int iDatabit, const char *pcStopbit, char cParity);

/**
 *@brief 		打开USB端口并对其进行设置
 *@param[in] 	ucComPort 		需打开的端口号
 *@param[in] 	iBaudrate		波特率
 *@param[in] 	iDatabit		数据位
 *@param[in] 	pcStopbit		停止位
 *@param[in] 	cParity			校验位
 *@return 		-1-打开失败	>0-打开的USB端口设备文件描述符
 */
int SMOpenUsbPort (unsigned char ucComPort, int iBaudrate, int iDatabit, const char *pcStopbit, char cParity);

/**
 *@brief 		关闭端口
 *@param[in] 	iFd 	要关闭的端口设备文件描述符
 *@return 		无
 */
void SMCloseComPort (int iFd);

/**
 *@brief 		读取串口数据
 *@param[in] 	iFd 		文件描述符
 *@param[out] 	pvData		读取的数据
 *@param[in] 	iDataLen	要读取数据的长度
 *@return 		读到数据的实际长度
 */
int SMReadComPort(int iFd, void *pvData, int iDataLen);

/**
 *@brief 		写入串口数据
 *@param[in] 	iFd 		文件描述符
 *@param[out] 	pvData		要写入的数据
 *@param[in] 	iDataLen	写入数据的长度
 *@return 		写入的数据实际长度
 */
int SMWriteComPort(int iFd, char *pvData, int iDataLen);

#endif /* SM_SERIAL_H_ */
