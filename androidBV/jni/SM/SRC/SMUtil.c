#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
//#include <libio.h>
#include <dirent.h>
#include <sys/mman.h>

#include <YKSystem.h>
#include "../INC/SMUtil.h"


unsigned short SMCalcuCRC(const char* pcData, unsigned long ulDataLen, unsigned short usCrcInitVal)
{
	unsigned long i = 0;
	unsigned short usCrc = usCrcInitVal;

	while (ulDataLen-- != 0)
	{
		for (i = 0x80; i != 0; i /= 2)
		{
			if ((usCrc & 0x8000) != 0)
			{
				usCrc *= 2;
				usCrc ^= 0x1021; //余式CRC乘以2再求CRC
			}
			else
			{
				usCrc *= 2;
			}
			if ((*pcData & i) != 0)
			{
				usCrc ^= 0x1021; //再加上本位的CRC
			}
		}
		pcData++;
	}
	return usCrc;
}

unsigned char SMCalcuBCC(const char *acData, int iDataLen)
{
	int iPos;
    unsigned char iBccValue = 0;

    if(iDataLen <= 0 )
    {
    	return -1;
    }

    for(iPos=1; iPos <=iDataLen; iPos++)
    {
    	iBccValue ^= acData[iPos];         //XOR
    }

    return iBccValue;
}

int SMFindChar(unsigned char *pucStr, int iLen, unsigned char ucChar)
{
	int iPos = 1;
	while ((iPos <= iLen) && (iLen > 0))
	{
		if (*(pucStr + iPos - 1) == ucChar)
		{
			return iPos;
		}
		else
		{
			iPos++;
		}
	}
	return 0;
}

int SMVerifyHexAscii(const char* pcHexStr, int iSize)
{
	int iPos = 0;

	for (iPos = 0; iPos < iSize; iPos++)
	{
		if ((pcHexStr[iPos] >= 48 && pcHexStr[iPos] <= 57)
				|| (pcHexStr[iPos] >= 65 && pcHexStr[iPos] <= 70)
				|| (pcHexStr[iPos] >= 65 + 32 && pcHexStr[iPos] <= 70 + 32)
				|| pcHexStr[iPos] == 80 || pcHexStr[iPos] == 80 + 32)
		{
			continue;
		}
		return FALSE;
	}
	return TRUE;
}

int SMHexAsciiToDecimal(char* pcHex)
{
	char* pcTmp = NULL;
	return strtol(pcHex, &pcTmp, 16);
}

void SMSplitToBytes(unsigned char *pucStr, unsigned int uiDataLen)
{
	int  iPos;
	unsigned char ucTemp;

	for(iPos = uiDataLen-1; iPos >= 0; iPos--)
	{

		ucTemp = (unsigned char)(pucStr[iPos] & 0x0F);
		if(ucTemp <= 9)
		{
			pucStr[(iPos << 1) + 1] = ucTemp + 0x30;
		}
		else
		{
			pucStr[(iPos << 1) + 1] = ucTemp + 0x37;
		}


		ucTemp = (unsigned char)((pucStr[iPos] & 0xF0) >> 4);
		if(ucTemp <= 9)
		{
			pucStr[iPos << 1] = ucTemp + 0x30;
		}
		else
		{
			pucStr[iPos << 1] = ucTemp + 0x37;
		}
	}
}

int SMStrChrCnt(char *pcStr, int iStrLen, char cChr)
{
	int iCount = 0;
	int iPos = 0;

	if(pcStr == NULL || iStrLen == 0)
	{
		return -1;
	}

	for(iPos=0; iPos<iStrLen; iPos++)
	{
		if(pcStr[iPos] == cChr)
		{
			iCount++;
		}
	}

	return iCount;
}
