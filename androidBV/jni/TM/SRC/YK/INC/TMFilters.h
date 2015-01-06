/**
 *@addtogroup TManage
 *@{
 */

/**
 *@file TMFilters.h
 *@brief TM filters defines.
 *@author Amos
 *@version 1.0
 *@date 2012-04-27
 */

#ifndef __TM_FILTER_H__
#define __TM_FILTER_H__

#include "TMProcotol.h"
#include "TerminalManage.h"
#include <YKApi.h>
#include <YKMsgQue.h>

#if _TM_TYPE_ == _YK069_

typedef enum{
    MESSAGE_FOR_DECODE=0, 
    MESSAGE_FOR_ENCODE
}TM_MESSAGE_FOR_EN;

typedef struct __MsgData{
    void*               pvBuffer;
    int                 iSize;
    TM_MESSAGE_FOR_EN   enMessageType;
    TM_COMMAND_TYPE_EN  enCommandType;
    void*               pvResolve;
}TM_MSG_DATA_ST;

typedef enum{
    TM_FILTER_ENCODE_ID=0, 
    TM_FILTER_DECODE_ID, 
    TM_FILTER_RECV_ID, 
    TM_FILTER_SEND_ID, 
    TM_FILTER_CONFIG_READ_ID,
    TM_FILTER_CONFIG_WRITE_ID,
    TM_FILTER_CONTEXT_CHECK_ID,
}TM_FILTER_DESC_ID_EN;

typedef struct _PIFilter{
    YK_MSG_QUE_ST*  pstRecv;
    YK_MSG_QUE_ST*  pstSend;
    void*           pvData;
    TM_CORE_VTABLE_ST* pstCallBack;
}TM_FILTER_ST;

typedef void (*TMFilterFunc)(TM_FILTER_ST*);
typedef struct _FilterDesc{
    TMFilterFunc            Init;
    TMFilterFunc            PreProcess;
    TMFilterFunc            Process;		/* called every tick to do the filter's job*/
    TMFilterFunc            PostProcess;	/*called once after processing */
    TM_FILTER_ST*           pstFilter;
    TM_FILTER_DESC_ID_EN    enID;
}TM_FILTER_DESC_ST;

int     TMQueryHouseCount();
BOOL    TMQueryHouseInfo(TM_HOUSE_INFO_ST* pstHouse, int iHouseIndex);
BOOL    TMQueryHouseByHouseCode(TM_HOUSE_INFO_ST* pstHouse, const char* pcHouseCode);
int     TMQueryPhoneCount(TM_HOUSE_INFO_ST* pstHouse);
TM_PHONE_INFO_ST* TMQueryPhoneInfo(TM_HOUSE_INFO_ST* pstHouse, int iPhoneIndex);
BOOL    TMIsNotDisturbTime(TM_PHONE_INFO_ST* pstPhone);
int     TMQueryICIDCardCount(TM_HOUSE_INFO_ST* pstHouse);
BOOL    TMQueryICIDCardInfo(char* pcSerialNumber, TM_HOUSE_INFO_ST* pstHouse, int iIndex);
void    TMQuerySipInfo(char* pcUserName, char* pcPassWord, char* pcProxy, int* piPort, char* pcDomain);
void    TMQueryWanInfo(char* pcUserName, char* pcPassWord);

#endif

#endif//!__TM_FILTER_H__

/**@}*///TManage
