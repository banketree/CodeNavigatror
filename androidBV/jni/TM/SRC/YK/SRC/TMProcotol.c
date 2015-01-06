#include "../INC/TMProcotol.h"
#include "../INC/TMFilters.h"
#include <YKApi.h>
#include <IDBT.h>

#if _TM_TYPE_ == _YK069_

static unsigned short TMCalcCRC(const unsigned char* pucData, unsigned long ulDataLen, unsigned short usCrcInitValue)
{
    unsigned long i = 0;
    unsigned short usCrc = usCrcInitValue;
    while(ulDataLen-- != 0)
    {
        for(i = 0x80; i != 0; i/=2) {
            if((usCrc & 0x8000) != 0)
            {
                usCrc *= 2;
                usCrc ^= 0x1021; //CRC
            }else{
                usCrc *= 2;
            }
            if((*pucData & i) != 0)
            {
                usCrc ^= 0x1021; //CRC
            }
        }
        pucData++;
    }
    return usCrc;
}

static BOOL TMStringIsDigit(const char* pcSrc, int iLen)
{
    int i=0;
    for(i=0; i<iLen; i++)
    {
        if(!isdigit(pcSrc[i]))return FALSE;
    }
    return TRUE;
}

static int  TMUserServiceCountAllUsers(TM_USER_SERVICE_INFO_ST* pstInfo)
{
    char acUserCount[sizeof(pstInfo->acUserCount)+1]={0x0};
    memcpy(acUserCount, pstInfo->acUserCount, sizeof(acUserCount)-1);
    return TMHexStringToDecimal(acUserCount);
}

static TM_HOUSE_INFO_ST*  TMUserServiceQueryHouseInfo(TM_USER_SERVICE_INFO_ST* pstInfo, int iIndex)
{
    int nFirstHouseOffset=sizeof(pstInfo->acUserCount);
    int iOffset = nFirstHouseOffset+iIndex*sizeof(TM_HOUSE_INFO_ST);
    return (TM_HOUSE_INFO_ST*)((char*)pstInfo+iOffset);
}

static int TMUserServiceCountPhones(TM_HOUSE_INFO_ST* pstInfo)
{
    char acPhoneCount[sizeof(pstInfo->cPhoneCount)+1]={0x0};
    memcpy(acPhoneCount, pstInfo->cPhoneCount, sizeof(pstInfo->cPhoneCount)-1);
    return TMHexStringToDecimal(acPhoneCount);
}

static TM_PHONE_INFO_ST* TMUserServiceQueryPhoneInfo(TM_HOUSE_INFO_ST* pstInfo, int iIndex)
{
    int nFirstPhoneOffset=sizeof(pstInfo->acHouseCode)+sizeof(pstInfo->acRingTime)+\
        sizeof(pstInfo->cInCommingcCallNotice)+sizeof(pstInfo->acSIPCallDelay)+\
        sizeof(pstInfo->cPhoneCount);
    int iOffset = nFirstPhoneOffset+iIndex*sizeof(TM_PHONE_INFO_ST);
    return (TM_PHONE_INFO_ST*)((char*)pstInfo+iOffset);
}

static int  TMUserServiceCountDistrubTime(TM_PHONE_INFO_ST* pstInfo)
{
    char acDisCount[sizeof(pstInfo->cNotDisturbCount)+1]={0x0};
    memcpy(acDisCount, pstInfo->cNotDisturbCount, sizeof(pstInfo->cNotDisturbCount)-1);
    return TMHexStringToDecimal(acDisCount);
}

static TM_DISTURB_ST* TMUserServiceQueryDisturbInfo(TM_PHONE_INFO_ST* pstInfo, int iIndex)
{
    int nDisturbOffset=sizeof(pstInfo->cVoipEnable)+sizeof(pstInfo->acCsPhoneNumber)+\
        sizeof(pstInfo->acSipPhoneNumber)+sizeof(pstInfo->cNotDisturbCount);
    int iOffset = nDisturbOffset+iIndex*sizeof(TM_DISTURB_ST);
    return (TM_DISTURB_ST*)((char*)pstInfo+iOffset);
}

static int  TMUserServiceCountCards(TM_HOUSE_INFO_ST* pstInfo)
{
    char cardCount[sizeof(pstInfo->cCardCount)+1]={0x0};
    memcpy(cardCount, pstInfo->cCardCount, sizeof(pstInfo->cCardCount)-1);
    return TMHexStringToDecimal(cardCount);
}

static TM_CARD_INFO_ST* TMUserServiceQueryCardInfo(TM_HOUSE_INFO_ST* pstInfo, int iIndex)
{
    int nFirstCardOffset=sizeof(pstInfo->acHouseCode)+sizeof(pstInfo->acRingTime)+\
        sizeof(pstInfo->cInCommingcCallNotice)+sizeof(pstInfo->acSIPCallDelay)+\
        sizeof(pstInfo->cPhoneCount)*sizeof(TM_PHONE_INFO_ST)+sizeof(pstInfo->cCardCount);
    int iOffset = nFirstCardOffset+iIndex*sizeof(TM_CARD_INFO_ST);
    return (TM_CARD_INFO_ST*)((char*)pstInfo+iOffset);
}

int TMHexStringToDecimal(const char* pcHex)
{
    char* pcTmp = NULL;
    return strtol(pcHex, &pcTmp, 16);
}

BOOL TMCompareNoCase(const char* pcSrc, const char* pcDest, int iLen)
{
    int i = 0;
    for(i = 0; i< iLen; i++)
    {
        if(pcSrc[i] == pcDest[i])
        {
            continue;
        }
        if(isalpha(pcSrc[i]) && isalpha(pcDest[i])) {
            if(pcSrc[i] - 32 == pcDest[i] ||pcSrc[i] + 32 == pcDest[i])
                continue;
        }
        return FALSE;
    }
    return TRUE;  
}

BOOL TMIsPhoneNumber(const char* phone, int len)
{
    int i=0;
    int start=0;
    while(phone[i++]==' '&& i<len)
    {
    	continue;
    }
    if(i>=len)
    {
    	return TRUE;
    }

    start=i-1;

    for(start; start< len; start++)
    {
        if(isdigit(phone[start]))
        {
            continue;
        }
        if(start==i-1 && phone[start]=='+')
        {
            continue;
        }
        return FALSE;
    }
    return TRUE;
}

TM_CONTEXT_STATE_EN TMDeviceInfoCheck(TM_SET_DEVICE_INFO_ST* pstInfo, int iSize)
{
    int iIndex=0;
    if(iSize<sizeof(TM_SET_DEVICE_INFO_ST))
    {
        return TM_CONTEXT_ERROR;
    }
    for(iIndex=0; iIndex<sizeof(pstInfo->acOui); iIndex++)
    {
        if(pstInfo->acOui[iIndex]<'0' || pstInfo->acOui[iIndex]>'9')
        {
            return TM_CONTEXT_ERROR;
        }
    }
    if(!strncmp(pstInfo->acProductClass + sizeof(pstInfo->acProductClass) -strlen("iDBT"), "iDBT", strlen("iDBT")))
    {
        return TM_CONTEXT_ERROR;
    }
    return TM_CONTEXT_PASS;
}

TM_CONTEXT_STATE_EN TMCommunicateInfoCheck(TM_SET_COMMUNICATE_INFO_ST* pstInfo, int iSize)
{
    if(iSize<sizeof(TM_SET_COMMUNICATE_INFO_ST))
        return TM_CONTEXT_ERROR;
    return TM_CONTEXT_PASS;
}

TM_CONTEXT_STATE_EN TMWorkInfoCheck(TM_SET_WORK_INFO_ST* pstInfo, int iSize)
{
    if(iSize< sizeof(TM_SET_WORK_INFO_ST))
        return TM_CONTEXT_ERROR;
    if((pstInfo->cIcOpenDoorEnable[0] != '0' &&pstInfo->cIcOpenDoorEnable[0]!='1')||
        (pstInfo->cImsCallEnable[0]!='0' && pstInfo->cImsCallEnable[0]!='1') ||
        (pstInfo->cMonitorEnable[0]!='0' && pstInfo->cMonitorEnable[0]!='1') ||
        (pstInfo->cWisdomOpenDoorEnable[0]!='0' && pstInfo->cWisdomOpenDoorEnable[0]!='1'))
    {
            return TM_CONTEXT_ERROR;
    }
    return TM_CONTEXT_PASS;
}

TM_CONTEXT_STATE_EN TMAlarmCheck(TM_SET_ALARM_ENABLE_ST*pstInfo, int iSize)
{
    if(iSize< sizeof(TM_SET_ALARM_ENABLE_ST))
        return TM_CONTEXT_ERROR;
    if((pstInfo->cFileTransferFail[0] != '0' &&pstInfo->cFileTransferFail[0]!='1')||
        (pstInfo->cIfBoardComFail[0]!='0' && pstInfo->cIfBoardComFail[0]!='1') ||
        (pstInfo->cImsRegisterFail[0]!='0' && pstInfo->cImsRegisterFail[0]!='1') ||
        (pstInfo->cSipCallFail[0]!='0' && pstInfo->cSipCallFail[0]!='1'))
    {
            return TM_CONTEXT_ERROR;
    }
    return TM_CONTEXT_PASS;
}

TM_CONTEXT_STATE_EN TMSysRebootChekc(TM_SET_SYS_REBOOT_ST*pstInfo, int iSize)
{
    if(iSize< sizeof(TM_SET_SYS_REBOOT_ST))
        return TM_CONTEXT_ERROR;
    return TM_CONTEXT_PASS;
}

TM_CONTEXT_STATE_EN TMSetDownloadCheck(TM_SET_DOWNLOAD_ST*pstInfo, int iSize)
{
    if(iSize< sizeof(TM_SET_DOWNLOAD_ST))
        return TM_CONTEXT_ERROR;
    if(pstInfo->cType[0] != '0' && pstInfo->cType[0] != '1' && pstInfo->cType[0] != '2')
        return TM_CONTEXT_ERROR;
    return TM_CONTEXT_PASS;
}

TM_CONTEXT_STATE_EN TMSetUploadCheck(TM_SET_UPLOAD_ST*pstInfo, int iSize)
{
    if(iSize< sizeof(TM_SET_UPLOAD_ST))
        return TM_CONTEXT_ERROR;
    if(pstInfo->cType[0] != '0' && pstInfo->cType[0] != '1' && pstInfo->cType[0] != '2')
        return TM_CONTEXT_ERROR;
    return TM_CONTEXT_PASS;
}

TM_CONTEXT_STATE_EN TMSetCallOutGoingCheck(TM_OUT_GOING_CALL_INFO*pstInfo, int iSize)
{
	if(iSize< sizeof(TM_OUT_GOING_CALL_INFO) ||pstInfo->acPhoneNumber[sizeof(pstInfo->acPhoneNumber)-1]==' ')
	{
		return TM_CONTEXT_ERROR;	//phone number is null. or mesage size is error.
	}
	if(!TMIsPhoneNumber(pstInfo->acPhoneNumber, sizeof(pstInfo->acPhoneNumber)))
	{
		return TM_CONTEXT_ERROR;// Invalid telephone number.
	}
	return TM_CONTEXT_PASS;
}


TM_CONTEXT_STATE_EN TMSetTransferCheck(TM_SET_TRANSFER_ST*pstInfo, int iSize)
{
    if(iSize< sizeof(TM_SET_TRANSFER_ST))
        return TM_CONTEXT_ERROR;
    if(pstInfo->cType[0] != '0' && pstInfo->cType[0] != '1')
        return TM_CONTEXT_ERROR;
    return TM_CONTEXT_PASS;
}

TM_CONTEXT_STATE_EN TMUserServiceCheck(TM_USER_SERVICE_INFO_ST*pstInfo, int iSize)
{
    int userCount, iPhoneCount, cardCount, disturbCount;
    TM_HOUSE_INFO_ST* pstHouse=NULL;
    TM_PHONE_INFO_ST* pstPhone=NULL;
    TM_DISTURB_ST*   disturb=NULL;
    TM_CARD_INFO_ST*  card =NULL;
    int userIndex, iPhoneIndex, cardIndex, disturbIndex;
    userCount=TMUserServiceCountAllUsers(pstInfo);
    if(iSize<userCount*sizeof(TM_HOUSE_INFO_ST)+sizeof(pstInfo->acUserCount))
    {
        return TM_CONTEXT_ERROR;
    }
    for(userIndex=0; userIndex<userCount; userIndex++)
    {
        //check pstHouse pstInfo.
        pstHouse=TMUserServiceQueryHouseInfo(pstInfo, userIndex);
        if((pstHouse->cInCommingcCallNotice[0]!='0'&& pstHouse->cInCommingcCallNotice[0]!='1'))
        {
            return TM_CONTEXT_ERROR;
        }
         // check pstPhone pstInfo.
        iPhoneCount=TMUserServiceCountPhones(pstHouse);
        for(iPhoneIndex=0; iPhoneIndex<iPhoneCount; iPhoneIndex++)
        {
            pstPhone=TMUserServiceQueryPhoneInfo(pstHouse, iPhoneIndex);
            if(!TMIsPhoneNumber(pstPhone->acCsPhoneNumber, sizeof(pstPhone->acCsPhoneNumber))||
               !TMIsPhoneNumber(pstPhone->acSipPhoneNumber, sizeof(pstPhone->acSipPhoneNumber)) ||
                (pstPhone->cVoipEnable[0]!='0' && pstPhone->cVoipEnable[0]!='1'))
            {
                return TM_CONTEXT_ERROR;
            }
            //check disturb time.
            disturbCount=TMUserServiceCountDistrubTime(pstPhone);
            for(disturbIndex=0; disturbIndex<disturbCount; disturbCount++)
            {
                disturb=TMUserServiceQueryDisturbInfo(pstPhone, disturbIndex);
                if(!TMStringIsDigit(disturb->acTime, sizeof(disturb->acTime)))
                {
                    return TM_CONTEXT_ERROR;
                }
            }
        }
        cardCount=TMUserServiceCountCards(pstHouse);
        for(cardIndex=0; cardIndex<cardCount; cardCount++)
        {

        }
    }
    return TM_CONTEXT_PASS;
}

BOOL TMDecodePacket(TM_MSG_DATA_ST* pstIn, TM_MSG_DATA_ST**ppstOut, TM_CONTEXT_STATE_EN*penError)
{
    if(NULL == pstIn || NULL==pstIn->pvBuffer)
    {
        return FALSE;
    }
    do{
        int iIndex = 0, cmdSize = 0, iCMD = -1, contentStart = 0, contentSize=0;
        TM_PROCOTOL_PACKET_ST protoObj;
        char* pcBuffer=(char*)pstIn->pvBuffer;                
        char acTmp[SIZE_32] = {0x0};
        unsigned short CrcDest, CrcSrc;
        char acID[sizeof(protoObj.acDeviceID)+1]={0x0};
        TMQueryDeviceID(acID);
        *ppstOut = NULL;

        if(pcBuffer[iIndex] != TM_PROCOTOL_START_FLAG || TM_PROCOTOL_END_FLAG!=pcBuffer[pstIn->iSize - 1])
        {
            *penError=TM_CONTEXT_ERROR;break;
        }
        iIndex += sizeof(protoObj.acBegin);
        memcpy(acTmp, &pcBuffer[iIndex], sizeof(protoObj.acLength));
        cmdSize = TMHexStringToDecimal(acTmp);
        iIndex += sizeof(protoObj.acLength);
        CrcSrc = TMCalcCRC((const unsigned char*)pcBuffer, cmdSize + sizeof(protoObj.acLength) + sizeof(protoObj.acBegin), 0);
        memset(acTmp, 0x0, sizeof(acTmp));
        memcpy(acTmp, pcBuffer + sizeof(protoObj.acLength) + sizeof(protoObj.acBegin) + cmdSize, sizeof(protoObj.acCrc));
        CrcDest = (unsigned short)TMHexStringToDecimal(acTmp);
        if(CrcSrc != CrcDest)
        {
            *penError=TM_CONTEXT_CRC;break;
        }
        if(!TMCompareNoCase(acID, &pcBuffer[iIndex], sizeof(protoObj.acDeviceID)))
        {
            *penError=TM_CONTEXT_ERROR;break;
        }
        iIndex += sizeof(protoObj.acDeviceID);
        memset(acTmp, 0x0, sizeof(acTmp));
        memcpy(acTmp, &pcBuffer[iIndex], sizeof(protoObj.acCommandType));
        iCMD = TMHexStringToDecimal(acTmp);
        if(iCMD < 0 || iCMD >= TM_COMMAND_UNKNOW)
        {
            *penError=TM_CONTEXT_ERROR;break;
        }
        iIndex += sizeof(protoObj.acCommandType);
        contentStart = iIndex;
        contentSize = cmdSize - sizeof(protoObj.acCommandType) - sizeof(protoObj.acDeviceID);
        iIndex += contentSize;
        *ppstOut = YKNew0(TM_MSG_DATA_ST, 1);
        (*ppstOut)->pvBuffer = (void*)YKNew0(char, contentSize);
        if(NULL == (*ppstOut)->pvBuffer)
        {
            YKFree(*ppstOut);
            *ppstOut = NULL;
            *penError=TM_CONTEXT_ERROR;
            break;
        }
        memcpy((*ppstOut)->pvBuffer, &pcBuffer[contentStart], contentSize);
        (*ppstOut)->enCommandType = (TM_COMMAND_TYPE_EN)iCMD;
        (*ppstOut)->iSize = contentSize;
        (*ppstOut)->enMessageType = MESSAGE_FOR_DECODE;
    }while(FALSE);
    return ((*ppstOut)&&(*ppstOut)->pvBuffer)?TRUE:FALSE;
}

BOOL TMEncodePacket(TM_MSG_DATA_ST*pstIn, TM_MSG_DATA_ST**ppstOut)
{
    char* encode=NULL;
    if((int)pstIn->enCommandType < 0 || (int)pstIn->enCommandType > TM_COMMAND_UNKNOW)
    {
        return FALSE;// iCMD type error
    }
    *ppstOut=YKNew0(TM_MSG_DATA_ST, 1);
    (*ppstOut)->enCommandType = pstIn->enCommandType;
    (*ppstOut)->enMessageType = pstIn->enMessageType;
    (*ppstOut)->pvBuffer =NULL;
    (*ppstOut)->iSize = 0;
    do{
        TM_PROCOTOL_PACKET_ST protoObj;
        int protocrlSize = sizeof(protoObj) + pstIn->iSize;
        int iIndex = 0;
        int cmdSize=0;
        unsigned short crcValue=0; 
        int iCMD = (int)pstIn->enCommandType;
        char acID[sizeof(protoObj.acDeviceID)+1]={0x0};
        TMQueryDeviceID(acID);
        encode = YKNew0(char, protocrlSize);
        if(NULL == encode)
        {
            break;
        }
        encode[iIndex] = TM_PROCOTOL_START_FLAG;
        iIndex+=sizeof(protoObj.acBegin);
        cmdSize = pstIn->iSize + sizeof(protoObj.acCommandType) + sizeof(protoObj.acDeviceID);
        if(cmdSize >= 0xFFFF)
        {
            break;
        }
        sprintf(&encode[iIndex], "%04X", cmdSize);
        iIndex += sizeof(protoObj.acLength);
        memcpy(&encode[iIndex], acID, sizeof(protoObj.acDeviceID));
        iIndex += sizeof(protoObj.acDeviceID);
        sprintf(&encode[iIndex], "%02X", iCMD);
        iIndex += sizeof(protoObj.acCommandType);
        if(NULL != pstIn->pvBuffer && pstIn->iSize != 0)
        {
            memcpy(&encode[iIndex], pstIn->pvBuffer, pstIn->iSize);
            iIndex += pstIn->iSize;
        }
        crcValue = TMCalcCRC((const unsigned char*)encode, protocrlSize - sizeof(protoObj.acCrc) - sizeof(protoObj.acEnd), 0);
        sprintf( &encode[iIndex], "%04X", crcValue);
        iIndex += sizeof(protoObj.acCrc);
        encode[iIndex] = TM_PROCOTOL_END_FLAG;
        (*ppstOut)->iSize = protocrlSize;
        (*ppstOut)->pvBuffer = encode;
        (*ppstOut)->enCommandType=pstIn->enCommandType;
    }while (FALSE);

    if((*ppstOut)->iSize == 0 && encode != NULL)
    {
        YKFree(encode);
        encode = NULL;
    }
    return (*ppstOut)->iSize>0?TRUE:FALSE;
}

#endif
