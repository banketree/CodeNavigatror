/*
 * SMCapturePicture.h
 *
 *  Created on: 2013-1-17
 *      Author: zhengwx
 */


#ifndef SM_CAPTURE_PICTURE_H_
#define SM_CAPTURE_PICTURE_H_
extern YK_MSG_QUE_ST*   g_pstSSMsgQ ;







//外部函数



// 内部函数
void SSProcessMessage(SS_MSG_DATA_ST* msg);
int UploadFile(char *local, char *remote);




#endif
