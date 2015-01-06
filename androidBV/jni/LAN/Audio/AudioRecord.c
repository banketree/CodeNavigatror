#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
//#include <utils/Errors.h>

#include "alsa_audio.h"
#include "g711common.h"
#include "common.h"
#include "LOGIntfLayer.h"

#include <utils/Log.h>

#define PCM_RECORD_LEN	4096
#define MAKEWORD(byte1,byte2)		((unsigned short)byte2 << 8 | byte1)
#define LOWORD(dword)				((unsigned short)dword)
#define LOBYTE(word)					((unsigned char)word)
#define HIBYTE(word)					((unsigned char)(word >> 8))

struct pcm *pcm_record=NULL;
int frame_size = 0;

int frame_size_get(int SampleRate, int ChannelNumber)
{
	int size = -1;

	switch(SampleRate)
	{
	case 8000:
		size = 320;
		break;
	case 16000:
		size = 640;
		break;
	case 24000:
		size = 960;
		break;
	case 32000:
		size = 1280;
		break;
	case 48000:
		size = 1920;
		break;
	case 44100:
		size = 441*4;
		break;
	case 22050:
		size = 441*2;
		break;
	case 11025:
		size = 441;
		break;
	default:
		break;
	}
	return ChannelNumber*size;

}

//杩斿洖>1鐨勬暟琛ㄧず鎴愬姛锛屽叾鍊艰〃绀烘瘡涓�抚PCM鏁版嵁鐨勯暱搴�//杩斿洖-1琛ㄧず澶辫触
int record_open(int SampleRate, int ChannelNumber)
{

	if(ChannelNumber == 1)
	{
		pcm_record = pcm_open(SampleRate, PCM_IN|PCM_MONO);
	}
	else if(ChannelNumber == 2)
	{
		pcm_record = pcm_open(SampleRate, PCM_IN|PCM_STEREO);
	}

   if (!pcm_ready(pcm_record))

	{
		pcm_close(pcm_record);
		return -1;
	}

	frame_size = frame_size_get(SampleRate, ChannelNumber);

	return frame_size;
}

//杩斿洖0琛ㄧず鎴愬姛
//杩斿洖-1琛ㄧず澶辫触
int record_close(void)
{
	if(NULL != pcm_record)
	{
		return pcm_close(pcm_record);
	}

	return 0;
}

int RaiseVolum(char *buf, unsigned int buf_size, unsigned uRepeat, double AmplifyRatio)
{
	int i = 0;
	int j = 0;

	if(!buf_size)
	{
		return -1;
	}

	for(i=0; i<buf_size;)
	{
		signed long minData = -0x8000;//濡傛灉鏄�bit锛岃繖閲屽彉鎴�0x80
		signed long maxData = 0x7FFF;//濡傛灉鏄�bit缂栫爜杩欓噷鍙樻垚0xFF
		signed short wData = buf[i+1];
		wData = MAKEWORD(buf[i], buf[i+1]);
		signed long dwData = wData;

		for(j=0; j<uRepeat; j++)
		{
			dwData=dwData * AmplifyRatio;
			if(dwData < -0x8000)
			{
				dwData = -0x4000;
			}
			else if(dwData > 0x7FFF)
			{
				dwData = 0x4000;
			}
		}
		wData = LOWORD(dwData);
		buf[i] = LOBYTE(wData);
		buf[i+1] = HIBYTE(wData);
		i+=2;
	}
}

int pcm_volume_control(char *PcmBuf, int PcmBufLen, double AmplifyRatio)
{
	return RaiseVolum(PcmBuf, PcmBufLen, 1, AmplifyRatio);
}


int record_pcm(char *pPcmBuf, int PcmBufLen, double AmplifyRatio)
{
   if(0 != pcm_read(pcm_record, pPcmBuf, PcmBufLen))
    {
        LOGE("con't pcm read %d bytes\n", PcmBufLen);
        return -1;
    }
   //for debug

#if 0
   FILE *fp=fopen("/data/data/recordoriginalpcmfile", "w");

   if(fp == NULL)
   {
	   LOGE("&&&&&& open record original pcm file failed!\n");
	   return -1;
   }
   fwrite(pPcmBuf, PcmBufLen, 1, fp);
   if(fp != NULL)
   {
	   fclose(fp);
   }
#endif

   //瀵筆CM杩涜鏀惧ぇ澶勭悊
   if(AmplifyRatio != 1)
   {
	   pcm_volume_control(pPcmBuf, PcmBufLen, AmplifyRatio);
   }

   return PcmBufLen;
}

int AudioRecord(char *pAudioBuf, int FrameSize, int nFrame, double AmplifyRatio)
{
	char *pPcmBuf = NULL;
	unsigned short *pPcm = NULL;
	int PcmBufLen, AudioLen, i;

	if((FrameSize == 0) || (nFrame == 0) || (pAudioBuf == NULL))
	{
		goto fail;
	}

	//姣忔璇诲彇鎸囧畾闀垮害鐨凱CM鏁版嵁锛屼互瀛楄妭涓哄崟浣嶈繘琛岃鎿嶄綔
	AudioLen = FrameSize * nFrame;
	PcmBufLen = AudioLen * 2;
	pPcmBuf = (unsigned char *)malloc(PcmBufLen);
	if (!pPcmBuf) {
		printf("could not allocate %d bytes\n", PcmBufLen);
		return -1;
	}

	if(record_pcm(pPcmBuf, PcmBufLen, AmplifyRatio) == -1)
	{
		printf("record pcm failed!\n");
		goto fail;
	}

	//for debug
#if 0
	FILE *fp = fopen("/data/data/recordpcmfile", "w");
	if(NULL == fp)
	{
		printf("open pcm record file failed\n");
		goto fail;
	}
	fwrite(pPcmBuf, PcmBufLen, 1, fp);
	if(fp != NULL)
	{
		fclose(fp);
	}
#endif

	pPcm = (unsigned short *)pPcmBuf;

	for(i=0; i<AudioLen; i++)
	{
		*pAudioBuf = s16_to_ulaw(*pPcm);
		pAudioBuf ++;
		pPcm ++;
	}

	if(pPcmBuf != NULL)
	{
		free(pPcmBuf);
	}

    return AudioLen;

fail:
	if(pPcmBuf != NULL)
	{
		free(pPcmBuf);
	}
	return -1;
}
