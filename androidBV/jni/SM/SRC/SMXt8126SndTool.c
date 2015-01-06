#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <semaphore.h>
#include <dirent.h>
#include <errno.h>
#include <linux/soundcard.h>

#include "IDBT.h"
#include "YKSystem.h"
#include "SMXt8126SndTool.h"
#include "LOGIntfLayer.h"



WAV_FILE_HEADER_ST stWavFileHeader;				//音频文件文件头

WAV_BUF_ST stWavBuf;							//音频文件缓冲区

pthread_t g_audioPlayThread;                    //音频播放线程

static int g_iSndDevFd = -1;		    		//声卡设备文件描述符

static volatile int g_iAudioPlayFlg = 0;		//音频播放标志(0-停止  1-播放)

pthread_mutex_t snd_card_lock;                  // 声卡操作的锁add by cs 2012-09-29


void SMXt8126AudioInit(void)
{
	char acTmp[128]={0x0};

	//set a IPGA volume, from -27 ~ +36 dB
	system("echo 0 > /proc/asound/adda300_RIV"); //6
	system("echo 0 > /proc/asound/adda300_LIV");

	//set a digital input volume, from -50 ~ +30 dB
	sprintf(acTmp,"echo %d > /proc/asound/adda300_LADV",g_iInputVol);
	system(acTmp);

	//system("echo 7 > /proc/asound/adda300_LADV");
	system("echo 7 > /proc/asound/adda300_RADV");

	//set digital output volume, from 0 ~ -40 dB
	system("echo -30 > /proc/asound/adda300_LDAV");
	system("echo -30 > /proc/asound/adda300_RDAV");

	//set speaker analog output gain, from -40 ~ +6 dB, this affect the final frontend output
	sprintf(acTmp,"echo %d > /proc/asound/adda300_SPV",g_iOutputVol);
	system(acTmp);
	system("echo 0 > /proc/asound/adda300_input_mode input_mode");
	system("echo -6 > /proc/asound/adda300_ALCMIN");
	system("echo 1 > /proc/asound/adda300_BTL_mode");
}

int SMOpenSndCard(void)
{
	g_iSndDevFd = open("/dev/dsp2", O_WRONLY, 0);
	if(g_iSndDevFd < 0)
	{
		LOG_RUNLOG_ERROR("SM open snd card failed!");
		return FAILURE;
	}
	else
	{
		LOG_RUNLOG_DEBUG("SM open snd card successed!");
		return SUCCESS;
	}
}

void SMCloseSndCard(void)
{
	if(0 == close(g_iSndDevFd))
	{
		LOG_RUNLOG_DEBUG("SM close snd card successed!");
	}
	else
	{
		LOG_RUNLOG_WARN("SM close snd card failed!");
	}
}


int SMSetFormat(int iBits, int iHz, int iChn)
{
	int iArg;    	//用于ioctl调用的参数
	int iRet; 	    //系统调用的返回值
	int iBufSize;

	// 设置采样时的量化位数       FIC8120只支持16位
	LOG_RUNLOG_DEBUG("iBits = %d, iHz = %d, iChn = %d, g_iSndDevFd = %d", iBits, iHz, iChn, g_iSndDevFd);
	if (g_iSndDevFd <= 0)
		return -1;

	iRet = ioctl(g_iSndDevFd, SOUND_PCM_READ_BITS, &iArg);
	if (iArg != iBits)
	{
		iArg = iBits;
		iRet = ioctl(g_iSndDevFd, SOUND_PCM_WRITE_BITS, &iArg);
		if (iRet == -1)
		{
			LOG_RUNLOG_WARN("SM SOUND_PCM_WRITE_BITS ioctl failed");
		}
		if (iArg != iBits)
		{
			LOG_RUNLOG_WARN("SM unable to set sample size");
		}
	}

	// 设置采样时的采样频率
	iRet = ioctl(g_iSndDevFd, SOUND_PCM_READ_RATE, &iArg);
	if (iArg != iHz)
	{
		iArg = iHz;
		iRet = ioctl(g_iSndDevFd, SOUND_PCM_WRITE_RATE, &iArg);
		if (iRet == -1)
		{
			LOG_RUNLOG_WARN("SM SOUND_PCM_WRITE_RATE ioctl failed");
		}
	}

	// 设置采样时的声道数目
	iRet = ioctl(g_iSndDevFd, SOUND_PCM_READ_CHANNELS, &iArg);
	if (iArg != iChn)
	{
		iArg = iChn;
		iRet = ioctl(g_iSndDevFd, SOUND_PCM_WRITE_CHANNELS, &iArg);
		if (iRet == -1)
		{
			LOG_RUNLOG_WARN("SM SOUND_PCM_WRITE_CHANNELS ioctl failed");
		}
		if (iArg != iChn)
		{
			LOG_RUNLOG_WARN("SM unable to set number of channels");
		}
	}

	iBufSize = AUDIOBLK;
	ioctl(g_iSndDevFd, SNDCTL_DSP_GETBLKSIZE, &iBufSize);

	if (iBufSize < 4 || iBufSize > 65536)
	{
		return FAILURE;
	}

	return SUCCESS;
}

void SMSyncPlay(void)
{
	int iRet;
	if (g_iSndDevFd > 0)
	{
		iRet = ioctl(g_iSndDevFd, SOUND_PCM_SYNC, 0);
		if (iRet == -1)
		{
			LOG_RUNLOG_WARN("SM SOUND_PCM_SYNC ioctl failed");
		}
	}
}

void SMStopPlayWavFile(void)
{
	g_iAudioPlayFlg = 0;
}

int SMWriteDataToSndDev(char *pcBuf, int iLen)
{
	int iCount;
	iCount = 0;

	if (g_iSndDevFd > 0)
	{
		iCount = write(g_iSndDevFd, pcBuf, iLen);
	}

	return iCount;
}

void *SMPlayWavThreadFunc(int iPlayFlg)
{
	int iCount;
	int iAudioLen;

	while(g_iAudioPlayFlg == 1)
	{
		if((stWavBuf.iGet + AUDIOPLAYBLK) < stWavBuf.iAudioLen)
		{
			iAudioLen = AUDIOPLAYBLK;
		}
		else
		{
			iAudioLen = stWavBuf.iAudioLen - stWavBuf.iGet;
		}
		iCount = SMWriteDataToSndDev((char *) (stWavBuf.pucBuf + stWavBuf.iGet), iAudioLen);

		if (iCount != AUDIOPLAYBLK)
		{
			LOG_RUNLOG_DEBUG("SM SMPlayWavThreadFunc::iCount = %d, AUDIOPLAYBLK = %d", iCount, AUDIOPLAYBLK);
		}
		stWavBuf.iGet += iAudioLen;

		if ((stWavBuf.iGet + AUDIOPLAYBLK) >= stWavBuf.iAudioLen)
		{
			if (iPlayFlg == 0) //单次播放
			{
				usleep(1000);
				SMStopPlayWavFile();
			}
			else //循环播放
			{
				stWavBuf.iGet = 0;
			}
		}
	}

	SMCloseSndCard();

	//等待回放结束
//	SMSyncPlay();

	if (stWavBuf.pucBuf != NULL)
	{
		free(stWavBuf.pucBuf);
		stWavBuf.pucBuf = NULL;
	}

	return NULL;
}

void SMPlayWavFile(char *pcFilePath, int iPlayFlg)
{
	pthread_attr_t attr;
	FILE *pFd;
	int iReadCount;

    pthread_mutex_lock(&snd_card_lock);

    if (g_iAudioPlayFlg == 1)
    {
        LOG_RUNLOG_DEBUG("SMPlayWavFile: already play !!!");
        goto _ERR_PRO_;
    }

	if ((pFd = fopen(pcFilePath, "rb")) == NULL)
	{
		LOG_RUNLOG_DEBUG("SM Open Error:%s", pcFilePath);
		goto _ERR_PRO_;
	}

	iReadCount = fread(stWavFileHeader.chRIFF, sizeof(stWavFileHeader.chRIFF), 1, pFd);
	iReadCount = fread(&stWavFileHeader.dwRIFFLen, sizeof(stWavFileHeader.dwRIFFLen), 1, pFd);
	iReadCount = fread(stWavFileHeader.chWAVE, sizeof(stWavFileHeader.chWAVE), 1, pFd);
	iReadCount = fread(stWavFileHeader.chFMT, sizeof(stWavFileHeader.chFMT), 1, pFd);
	iReadCount = fread(&stWavFileHeader.dwFMTLen, sizeof(stWavFileHeader.dwFMTLen), 1, pFd);
	iReadCount = fread(&stWavFileHeader.wFormatTag, sizeof(stWavFileHeader.wFormatTag), 1, pFd);
	iReadCount = fread(&stWavFileHeader.nChannels, sizeof(stWavFileHeader.nChannels), 1, pFd);
	iReadCount = fread(&stWavFileHeader.nSamplesPerSec, sizeof(stWavFileHeader.nSamplesPerSec), 1, pFd);
	iReadCount = fread(&stWavFileHeader.nAvgBytesPerSec, sizeof(stWavFileHeader.nAvgBytesPerSec), 1, pFd);
	iReadCount = fread(&stWavFileHeader.nBlockAlign, sizeof(stWavFileHeader.nBlockAlign), 1, pFd);
	iReadCount = fread(&stWavFileHeader.wBitsPerSample, sizeof(stWavFileHeader.wBitsPerSample), 1, pFd);

	fseek(pFd,sizeof(stWavFileHeader.chRIFF) + sizeof(stWavFileHeader.dwRIFFLen)
					+ sizeof(stWavFileHeader.chWAVE) + sizeof(stWavFileHeader.chFMT)
					+ sizeof(stWavFileHeader.dwFMTLen) + stWavFileHeader.dwFMTLen, SEEK_SET);
	iReadCount = fread(stWavFileHeader.chDATA, sizeof(stWavFileHeader.chDATA), 1, pFd);

	if ((stWavFileHeader.chDATA[0] == 'f') && (stWavFileHeader.chDATA[1] == 'a')
			&& (stWavFileHeader.chDATA[2] == 'c') && (stWavFileHeader.chDATA[3] == 't'))
	{
		iReadCount = fread(&stWavFileHeader.dwFACTLen, sizeof(stWavFileHeader.dwFACTLen), 1, pFd);
		fseek(pFd, stWavFileHeader.dwFACTLen, SEEK_CUR);
		iReadCount = fread(stWavFileHeader.chDATA, sizeof(stWavFileHeader.chDATA), 1, pFd);
	}
	if ((stWavFileHeader.chDATA[0] == 'd') && (stWavFileHeader.chDATA[1] == 'a')
			&& (stWavFileHeader.chDATA[2] == 't') && (stWavFileHeader.chDATA[3] == 'a'))
	{
		iReadCount = fread(&stWavFileHeader.dwDATALen, sizeof(stWavFileHeader.dwDATALen), 1, pFd);
	}

	if ((stWavFileHeader.chRIFF[0] != 'R') || (stWavFileHeader.chRIFF[1] != 'I')
			|| (stWavFileHeader.chRIFF[2] != 'F') || (stWavFileHeader.chRIFF[3] != 'F')
			|| (stWavFileHeader.chWAVE[0] != 'W') || (stWavFileHeader.chWAVE[1] != 'A')
			|| (stWavFileHeader.chWAVE[2] != 'V') || (stWavFileHeader.chWAVE[3] != 'E')
			|| (stWavFileHeader.chFMT[0] != 'f') || (stWavFileHeader.chFMT[1] != 'm')
			|| (stWavFileHeader.chFMT[2] != 't') || (stWavFileHeader.chFMT[3] != ' '))
	{
		LOG_RUNLOG_DEBUG("SM wav file format error");
		fclose(pFd);
		goto _ERR_PRO_;
	}

	stWavBuf.iPut = 0;
	stWavBuf.iGet = 0;

	stWavBuf.pucBuf = (unsigned char *) malloc(stWavFileHeader.dwDATALen);
	iReadCount = fread(stWavBuf.pucBuf, stWavFileHeader.dwDATALen, 1, pFd);
	if (iReadCount != 1)
	{
		LOG_RUNLOG_DEBUG("SM Read Error:%s", pcFilePath);
		if (stWavBuf.pucBuf != NULL)
		{
			free(stWavBuf.pucBuf);
			stWavBuf.pucBuf = NULL;
		}
		fclose(pFd);
		goto _ERR_PRO_;
	}
	stWavBuf.iAudioLen = stWavFileHeader.dwDATALen;
	fclose(pFd);

	//音频
	if (FAILURE == SMOpenSndCard())
	{
		goto _ERR_PRO_;
	}

	SMSetFormat(stWavFileHeader.wBitsPerSample,
			stWavFileHeader.nSamplesPerSec, stWavFileHeader.nChannels);

	g_iAudioPlayFlg = 1;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&g_audioPlayThread, &attr, (void *) SMPlayWavThreadFunc, (void *)iPlayFlg);
	pthread_attr_destroy(&attr);
	if (g_audioPlayThread == 0)
	{
		LOG_RUNLOG_WARN("SM can't create sound play thread!");
		goto _ERR_PRO_;
	}

_ERR_PRO_:
    pthread_mutex_unlock(&snd_card_lock);
    return;
}
