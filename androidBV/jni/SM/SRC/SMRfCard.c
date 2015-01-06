#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include <YKSystem.h>
#include "../INC/SMRfCard.h"
#include "../INC/SMUtil.h"
#include "../INC/SMSerial.h"
#include "../INC/SMEventHndl.h"
#include "TMInterface.h"
#include "LOGIntfLayer.h"
#include "IDBTIntfLayer.h"
//#include "SMXt8126SndTool.h"

#define CARD_LEN     8
#define STIRNGS_CR_LN_SNED_OK               "\r\n> "   //���ͳɹ��ַ���
typedef enum
{
    CHARS_CR_CNSNED_OK_FOUND = 0,  //�ҵ�"/r/n> " ����
    CHARS_CR_LN_FOUND = 1,      //�ҵ� "/r/n" ����
    CHARS_NO_FOUND          //δ�ҵ� ����
}CHAR_TYPE_EN;



#define MAX_RECV_LEN 100
#define MAX_CARD_RECV_CNT 20
#define CHAR_CR_LN               "\r\n"   //�س��ͻ��з�

#define CMD_EQPMNT_OPEN 			("\x02\x75\xFE\x10\x10\x00\x11\x22\x33\x00\x11\x22\x33\x00\x11\x22\x33\x00\x11\x22\x33\x9B\x10\x03")	//���豸
#define CMD_EQPMNT_VERSION			("\x02\x76\x4F\x00\x39\x10\x03")	                    //��ȡӲ���汾������
#define CMD_EQPMNT_CLOSE			("")	                                                //�ر��豸
#define CMD_SIGNAL_DETECTION_REQ   ("\x02\x0B\x41\x01\x01\x4A\x10\x03")	                //Ѱ������
#define CMD_ANITICOLLISION_REQ		("\x02\x0C\x42\x05\x00\x07\xF8\x89\x23\x1E\x10\x03")	//����ͻ����



//���ڽ������ݽṹ��
typedef struct
{
    char acBuf[COMM_DATA_RCV_SIZE];
    char *pcHead;
    unsigned char ucCounter;
    unsigned char ucState;
    int iLen;
} COM_CARD_DATA_RCV_ST;



static int g_iComFd = -1;                // modify by cs 2012-11-09
static pthread_t g_uimIdHndl;
static BOOL g_bRunCardReadFlg = TRUE;
static COM_CARD_DATA_RCV_ST   pstComDataRcv;

static CHAR_TYPE_EN FindValidChars(char * pAddr,int iLength, char * cpRecvCmd, int *iCnt);
static void CardComRcvData(COM_CARD_DATA_RCV_ST *pstComDataRcv);
static void ChangeStringsSeq(char * pcStrings);
static unsigned char SMRFCardProcess(unsigned char *num);

int SMCardReaderInit(void)
{
	int iLen = 0;
	g_iComFd = SMOpenComPort(0, CARD_READER_COM_BAUD, CARD_READER_COM_DATABIT, CARD_READER_COM_STOPBIT, CARD_READER_COM_PARITYBIT);
	if(g_iComFd < 0)
	{
		LOG_RUNLOG_ERROR("SM open com port failed");
		return FAILURE;
	}

#ifndef NEW_CARD_READER
	//�򿪶������豸
	unsigned char acComRcv[20] = {0};

	iLen = write(g_iComFd, CMD_EQPMNT_OPEN, sizeof(CMD_EQPMNT_OPEN));
	if(iLen<=0)
	{
		LOG_RUNLOG_ERROR("SM g_iComFd write failed!");
		return FAILURE;
	}

	usleep(100*1000);

	iLen = read(g_iComFd, acComRcv, 20);
	//��֤
	if(iLen<=0)
	{
		LOG_RUNLOG_ERROR("SM card reader equipment open failed!");
		return FAILURE;
	}
	if(acComRcv[2]==0x00 && acComRcv[3]==0x02 && acComRcv[4]==0xAA && acComRcv[5]==0x55)
	{
		LOG_RUNLOG_DEBUG("SM card reader equipment open success!");
		return SUCCESS;
	}
	else
	{
		LOG_RUNLOG_ERROR("SM rf equipment open failed!");
		return FAILURE;
	}
#endif
}

static int SMMsgSendSMABRfidcardValidate(RFIDCARD_VALIDATE_EN enStatus)
{
	int iRet = -1;
	if(NULL == g_pstABMsgQ)
	{
		LOG_RUNLOG_WARN("SM g_pstABMsgQ is null");
		return -1;
	}

	SMAB_RFIDCARD_VALIDATE_ST *pstMsg= (SMAB_RFIDCARD_VALIDATE_ST*)malloc(sizeof(SMAB_RFIDCARD_VALIDATE_ST));
	if(pstMsg == NULL)
	{
		LOG_RUNLOG_ERROR("SM malloc SMMsgSendSMABRfidcardValidate failed");
		return -1;
	}
	memset(pstMsg, 0x00, sizeof(SMAB_RFIDCARD_VALIDATE_ST));

	pstMsg->uiValidateStatus = (unsigned int)enStatus;

	pstMsg->uiPrmvType = SMAB_RFIDCARD_VALIDATE_STATUS;

	iRet = YKWriteQue( g_pstABMsgQ,  pstMsg,  LP_QUE_WRITE_TIMEOUT);

	LOG_RUNLOG_DEBUG("SM SMMsgQ send msg:\nuiPrmvType:SMAB_RFIDCARD_VALIDATE_STATUS | 0x%04x\n", SMAB_RFIDCARD_VALIDATE_STATUS);

	return iRet;
}
#ifdef NEW_CARD_READER
void *SMCardReaderCore(void *pvPara)
{

    LOG_RUNLOG_DEBUG("SM rf card card reader is ready!g_bRunCardReadFlg:%d",g_bRunCardReadFlg);
    memset(pstComDataRcv.acBuf, 0x0, COMM_DATA_RCV_SIZE * sizeof(unsigned char));
    pstComDataRcv.iLen = 0;
    pstComDataRcv.pcHead = pstComDataRcv.acBuf;
    pstComDataRcv.ucCounter = 0;
    pstComDataRcv.ucState = BEGIN_FLAG_NOT_FOUND;
    while (g_bRunCardReadFlg)
    {
        CardComRcvData(&pstComDataRcv);
        usleep(100 * 1000);
    }


	return NULL;
}

void CardComRcvData(COM_CARD_DATA_RCV_ST *pstComDataRcv)
{

    int iCount = 0;
    BOOL iRfCardExist = FALSE;
    int iRcvDataLen = 0;
    unsigned char uacTmpBuf[MAX_RECV_LEN / 2] ={0};
    int iIndex = 0xFFFF;
    unsigned char uacRecvCmd[128] = {0} ;//���Ŵ��滺����
    static int iNetRecvEn = 0;


    //���õĴ����͵�������ʱ����������յ����ݵ���Ч��
    if(pstComDataRcv->ucCounter++ >= MAX_CARD_RECV_CNT)
    {
        pstComDataRcv->iLen = 0;
        pstComDataRcv->pcHead = pstComDataRcv->acBuf;
        pstComDataRcv->ucCounter = 0;
        pstComDataRcv->ucState = BEGIN_FLAG_NOT_FOUND;
        iNetRecvEn = 0;
        return ;
    }

    iRcvDataLen = read(g_iComFd, uacTmpBuf, sizeof(uacTmpBuf));
    if(iRcvDataLen <= 0)
    {
        return;
    }

    if(&pstComDataRcv->acBuf[COMM_DATA_RCV_SIZE] < pstComDataRcv->pcHead + pstComDataRcv->iLen + iRcvDataLen)
    {
        //�������ݶ���գ��ָ���ʼ״̬
        pstComDataRcv->iLen = 0;
        pstComDataRcv->pcHead = pstComDataRcv->acBuf;
        pstComDataRcv->ucState = BEGIN_FLAG_NOT_FOUND;
        iNetRecvEn = 0;
    }
    memcpy(pstComDataRcv->pcHead + pstComDataRcv->iLen, uacTmpBuf, iRcvDataLen);
    pstComDataRcv->iLen += iRcvDataLen;

    while((pstComDataRcv->iLen > 0) && (iIndex != 0))
    {
            if (CHARS_CR_LN_FOUND == FindValidChars(pstComDataRcv->pcHead, pstComDataRcv->iLen, uacRecvCmd, &iIndex))  //�س����з���
            {
                pstComDataRcv->ucCounter = 0; //�յ�������־��counter����
                ChangeStringsSeq(uacRecvCmd);     //����˳��ת��

                LOG_RUNLOG_DEBUG("SM Card id is --- %s ---", uacRecvCmd);
                iCount = 0;
                SMRFCardProcess(uacRecvCmd);


           //��ʱ���ã��������ٶȿ���
#if 0
                if(TMSMIsICOpenDoorEnabled() == TRUE)
                {
                    iRfCardExist = TMSMIsValidICIDCard((char *)uacRecvCmd);
                    if(iRfCardExist == FALSE)
                    {
                        iRfCardExist = TMSMIsValidPropertyCard((char *)uacRecvCmd);
                    }
                    if(iRfCardExist == FALSE)
                    {
                        SMSMSendOpenDoor(OPEN_DOOR_EVENT_CARD, OPEN_DOOR_ERROR);  //��Ч��
        #ifdef _YK_XT8126_AV_
                        //SMPlayWavFile("/usr/wav/card_fail->wav", 0);
                        SMPlayWavFile("/usr/wav/card_fail->wav", 0, 0); //for call_preview
        #endif
                        LOG_RUNLOG_WARN("SM rf card id is invalid");
                        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_CARD_SWIPING, 1, uacRecvCmd);

                    }
                    else
                    {
                        SMSMSendOpenDoor(OPEN_DOOR_EVENT_CARD, OPEN_DOOR_OK);  //����Ч
        //#ifndef _YK_PC_
        //                              //SMOpenDoor();
        //                              //SMPlayWavFile("/usr/wav/dooropen->wav", 0);
        //#ifdef _YK_XT8126_AV_
        //                              SMPlayWavFile("/usr/wav/dooropen->wav", 0, 2); //for call_preview
        //#endif
        //#endif
                        //==============ADD BY HB==================
                        TMSetICIDOpenDoor(uacRecvCmd);
                        //==============ADD BY HB==================
                        LOG_RUNLOG_DEBUG("SM rf card id is on the list,open the door");
                        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_CARD_SWIPING, 0, uacRecvCmd);
                    }
                }
                else
                {
                    LOG_RUNLOG_DEBUG("SM IC open door is disable!");
                }
#endif

                pstComDataRcv->pcHead += iIndex;
                pstComDataRcv->iLen -= iIndex;

            }
            else
            {
                iIndex = 0;
//              pstComDataRcv->ucCounter++;

            }
        }



    //�����ݵ�����buf��ǰ��
    if(pstComDataRcv->pcHead != pstComDataRcv->acBuf)
    {
        memcpy(pstComDataRcv->acBuf, pstComDataRcv->pcHead, pstComDataRcv->iLen);
        pstComDataRcv->pcHead = pstComDataRcv->acBuf;
    }

}



unsigned char SMRFCardProcess(unsigned char *num)
{
    unsigned char buffTempCardNum[16];
    BOOL iRfCardExist = FALSE;
    static unsigned char ucAuthorizeCardStatus = 0;
    memcpy(buffTempCardNum, num, 8);

//    SMSplitToBytes(buffTempCardNum, 4);
    buffTempCardNum[8] = '\0';
///    LOG_RUNLOG_DEBUG("SM Card id is --- %s ---", buffTempCardNum);

#if 0   //��Ȩ��
    if(AUTH_CARD_RSQ == GetCardRsq())  //��Ȩ������
   {
         if(ucAuthorizeCardStatus == 1)  //�Ѿ�������Ȩ��ģʽ
           {

               if(TMSMIsValidAuthorizeCard((char *)buffTempCardNum) == 1)
        //     if(strcmp(acCardNum, "5B201F0A") == 0)
               {
                   SMSMSendOpenDoor(OPEN_DOOR_EVENT_CARD, OPEN_DOOR_REPEAT);  //���ظ�
                   LOG_RUNLOG_WARN("SM this is AuthorizeCard, no need to add!");
               }
               else
               {
                   LOG_RUNLOG_WARN("XT add card, card num is %s", (char *)buffTempCardNum);
                   if(SMLocalCardAdd((char *)buffTempCardNum) == -1)
                   {
                       SMSMSendOpenDoor(OPEN_DOOR_EVENT_CARD, OPEN_DOOR_REPEAT);  //���ظ�
                       LOG_RUNLOG_DEBUG("SM rf card id is on the list");

                   }
                   else
                   {
                       SMSMSendOpenDoor(OPEN_DOOR_EVENT_CARD, OPEN_DOOR_USER);//���ס����
                   }

               }
               return NULL;
           }
           if(TMSMIsValidAuthorizeCard((char *)buffTempCardNum) == 1)   //�Ƿ�����Ȩ��
        //   else if(strcmp(acCardNum, "5B201F0A") == 0)
           {
               ucAuthorizeCardStatus = 1;
               SMSMSendOpenDoor(OPEN_DOOR_EVENT_CARD, OPEN_DOOR_AUTH);//��Ȩ��
                return NULL;
           }
           else
           {
               SMSMSendOpenDoor(OPEN_DOOR_EVENT_CARD, OPEN_DOOR_REPEAT);  //���ظ�
               LOG_RUNLOG_DEBUG("SM rf card id not AuthorizeCardt");
           }
   }
    else
    {
        ucAuthorizeCardStatus = 0;
    }
#endif

    if(TMSMIsICOpenDoorEnabled() == TRUE)
    {
//        if((TMSMIsValidICIDCard((char *)buffTempCardNum) == 1) || (SMLocalCardIsExist((char *)buffTempCardNum) == 1)
//                || TMSMIsValidAuthorizeCard((char *)buffTempCardNum) == 1)
    	if((TMSMIsValidICIDCard((char *)buffTempCardNum) == 1))
        {
            //==============ADD BY HB==================
//            TMSetICIDOpenDoor(buffTempCardNum);
            //==============ADD BY HB==================
        	SMMsgSendSMABRfidcardValidate(RFIDCARD_VALID);
            LOG_RUNLOG_DEBUG("SM rf card id is on the list,open the door");
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_CARD_SWIPING, 0, buffTempCardNum);
        }
        else
        {
        	SMMsgSendSMABRfidcardValidate(RFIDCARD_INVALID);
            LOG_RUNLOG_WARN("SM rf card id is invalid");
            LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_CARD_SWIPING, 1, buffTempCardNum);

        }

    }
    else
    {
        LOG_RUNLOG_DEBUG("SM IC open door is disable!");
    }

}
//�ı��ַ���˳��
static void ChangeStringsSeq(char * pcStrings)
{
    char acBuf[CARD_LEN+1] = {0};
    int i  = 0;
    if(NULL == pcStrings || CARD_LEN != strlen(pcStrings) )
    {
        return ;
    }
    memcpy(acBuf, pcStrings, CARD_LEN);
    for(i = 0; i < CARD_LEN; i += 2)
    {
        pcStrings[i] = acBuf[CARD_LEN - 1 - i - 1];
        pcStrings[i + 1] = acBuf[CARD_LEN - 1 -i];

    }

}

/*
 * ���� Ѱ���ַ���.
 * ���룺ҪѰ�ҵĵ�ַ�� ���� �� ��Ѱ�ҵ��ַ���
 * ������ɹ�������Ѱ�ҵ�λ�ã�ʧ�ܷ���0
 */
int FindStings(unsigned char *pucStr, int iLen, unsigned char *ucpStings)
{
    int iCnt = 0;
    int i = 0;
    int iPos = 0;       //λ��
    int iFindStringsLen = 0;
    if (NULL == pucStr && NULL == ucpStings && 0 == iLen && iLen <= strlen(ucpStings))  //û���ҵ���Ӧ�ַ�
    {
        return 0;
    }

    iFindStringsLen = strlen(ucpStings);

    for (i = 0, iCnt = 0; i < iLen; i++)
    {
        if(pucStr[i] == ucpStings[iCnt])
        {
            if(++iCnt == iFindStringsLen)
            {
                iPos = i - iFindStringsLen + 2;  //�ҵ��ַ���
                break;
            }
        }
    }

    return iPos;

}

/*
 * Ѱ����Ч����
 * �����ַ�ʹ�С
 * ���أ�      CHARS_CR_CNSNED_OK_FOUND = 0,  //�ҵ� "/r/n> "
            CHARS_CR_LN_FOUND = 1,      //�ҵ� �س����з���λ��
            CHARS_NO_FOUND          //δ�ҵ� ����
 */
static CHAR_TYPE_EN FindValidChars(char * pAddr,int iLength, char * cpRecvCmd, int *iCnt)
{
    *iCnt = 0;
    int iCharCrCnAddr   = 0;    //�س����з���λ��
    int iStatus = CHARS_NO_FOUND;

    if (NULL == pAddr && 0 == iLength )
    {
        return CHARS_NO_FOUND;
    }


    iCharCrCnAddr = FindStings(pAddr, iLength, CHAR_CR_LN);// �һس����з���λ��

    if (iCharCrCnAddr == 0)  //��û���ҵ��ַ�
    {
        iStatus = CHARS_NO_FOUND;
    }
    else
    {
        *iCnt += iCharCrCnAddr + 1;
        memcpy(cpRecvCmd, pAddr  , iCharCrCnAddr -1);
        cpRecvCmd[iCharCrCnAddr - 1] = '\0';
        iStatus = CHARS_CR_LN_FOUND ;
    }


return iStatus;

}
#else
void *SMCardReaderCore(void *pvPara)
{
	int iPos = 0;
	int iLen = 0;
	int iCount = 0;
	unsigned char acCardNum[32] = { 0 };		//���Ŵ��滺����
	unsigned char acComRcv[100] = { 0 };
	unsigned char acTmp[100] = {0};
	unsigned char ucVerifyCode = 0;
	BOOL iRfCardExist = FALSE;

	LOG_RUNLOG_DEBUG("SM rf card card reader is ready!");
	while (g_bRunCardReadFlg)
	{
		memset(acComRcv, 0x00, 100);
		usleep(TM_READ_INTERVAL * 1000);
		iLen = write(g_iComFd, CMD_SIGNAL_DETECTION_REQ, sizeof(CMD_SIGNAL_DETECTION_REQ));
		if (iLen <= 0)
		{
			LOG_RUNLOG_ERROR("SM g_iComFd write failed!");
			continue;
		}
		usleep(220 * 1000);
		if(read(g_iComFd, acComRcv, 100)>0)
		{
			if (acComRcv[2] == 0x00)
			{
				LOG_RUNLOG_DEBUG("SM card pcType: %X", acComRcv[4]);
				memset(acComRcv, 0x00, 100);
				iLen = write(g_iComFd, CMD_ANITICOLLISION_REQ, sizeof(CMD_ANITICOLLISION_REQ));
				if (iLen <= 0)
				{
					LOG_RUNLOG_ERROR("SM g_iComFd write failed!");
					continue;
				}
				usleep(220 * 1000);
				if((iLen=read(g_iComFd, acComRcv, 100))>0)
				{
					memset(acTmp, 0x00, 100);
					memcpy(acTmp, acComRcv, iLen-3);
					//BCC��֤
					ucVerifyCode = SMCalcuBCC((char *)acTmp,8);

					if(acComRcv[2] == 0x00 && acComRcv[iLen-3] == ucVerifyCode)
					{
						for(iPos=iLen-4; iPos>=4; iPos--)
						{
							acCardNum[iCount] = acComRcv[iPos];
							iCount++;
						}
						acCardNum[iCount] = '\0';
						unsigned char *buffTempCardNum = acCardNum;
						SMSplitToBytes(buffTempCardNum, 4);
						LOG_RUNLOG_DEBUG("SM Card id is --- %s ---", buffTempCardNum);
						iCount = 0;
						if(TMSMIsICOpenDoorEnabled() == TRUE)
						{
							iRfCardExist = TMSMIsValidICIDCard((char *)acCardNum);
							if(iRfCardExist == FALSE)
							{
								iRfCardExist = TMSMIsValidPropertyCard((char *)acCardNum);
							}
							if(iRfCardExist == FALSE)
							{
								//SMPlayWavFile("/usr/wav/card_fail.wav", 0);
								SMMsgSendSMABRfidcardValidate(RFIDCARD_INVALID);
								LOG_RUNLOG_WARN("SM rf card id is invalid");
							}
							else
							{
#ifndef _YK_PC_
								//SMOpenDoor();
								//SMPlayWavFile("/usr/wav/dooropen.wav", 0);
#endif
								SMMsgSendSMABRfidcardValidate(RFIDCARD_VALID);
								SM210OpenDoor();

								//==============ADD BY HB==================
								//TMSetICIDOpenDoor(acCardNum);
								//==============ADD BY HB==================
								LOG_RUNLOG_DEBUG("SM rf card id is on the list,open the door");
								LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_CARD_SWIPING, 0, buffTempCardNum);
							}
						}
						else
						{
							SMMsgSendSMABRfidcardValidate(RFIDCARD_INVALID);
							LOG_RUNLOG_DEBUG("SM IC open door is disable!");
						}
					}
				}
			}
		}
	}

	return NULL;
}
#endif

int SMCardReaderClose(void)
{
    if (g_iComFd < 0)                       // modify by cs 2012-11-09
    {
	    write(g_iComFd, CMD_EQPMNT_CLOSE, sizeof(CMD_EQPMNT_CLOSE));
    }

	return 0;
}

int SMRfThreadCreate(void)
{
	if(pthread_create(&g_uimIdHndl, NULL, SMCardReaderCore, NULL) < 0)
	{
		LOG_RUNLOG_ERROR("SM create rf card event hndl thread failed!");
		return FAILURE;
	}

	return SUCCESS;
}

int SMRfInit(void)
{
	int iRet = SUCCESS;

	LOG_RUNLOG_DEBUG("SM SMCardReaderInit begin!");
	iRet = SMCardReaderInit();
	if(FAILURE == iRet)
	{
		LOG_RUNLOG_ERROR("SM card reader init failed!");
		return iRet;
	}

	iRet = SMRfThreadCreate();
	if(FAILURE == iRet)
	{
		LOG_RUNLOG_ERROR("SM rf card thread create failed!");
		return iRet;
	}

	LOG_RUNLOG_DEBUG("SM SMCardReaderInit end!");
	return iRet;

}

void SMRfFini(void)
{
	SMCardReaderClose();
	g_bRunCardReadFlg = FALSE;
    if (g_uimIdHndl != NULL)                    // modify by cs 2012-11-09
    {
    	YKJoinThread((void *)g_uimIdHndl);
    	YKDestroyThread(g_uimIdHndl);
    }
}
