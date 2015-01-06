#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "AudioPlay.h"
#include "g711common.h"


struct pcm *play_open(int SampleRate, int ChannelNumber)
{
	struct pcm *pcm_play;
	unsigned flags = PCM_OUT;

	if(ChannelNumber == 1)
	{
		flags |= PCM_MONO;
	}
	else if(ChannelNumber == 2)
	{
		flags |= PCM_STEREO;
	}

	pcm_play = pcm_open(SampleRate, flags);

	if (!pcm_ready(pcm_play))
	{
		pcm_close(pcm_play);
		return NULL;
	}

	return pcm_play;
}

//返回0表示成功
//返回-1表示失败
int play_close(struct pcm *pcm_play)
{
	if(NULL != pcm_play)
	{
		return pcm_close(pcm_play);
	}

	return 0;
}

int play_pcm(struct pcm *pcm_play, char *pPcmBuf, int PcmBufLen)
{
	if (0 != pcm_write(pcm_play, (void *)pPcmBuf, PcmBufLen))
	{
		printf("pcm write failed!\n");
		goto fail;
	}

	return PcmBufLen;

fail:
	pcm_close(pcm_play);

	return -1;
}

//输入音频类型为g711
//播放音频类型为PCM
int AudioPlay(struct pcm *pcm_play, char *pAudioBuf, int AudioLen)
{
	int *pPcmBuf = NULL;
	int PcmLen,i;

	PcmLen = AudioLen;
	pPcmBuf = (unsigned int *)malloc(PcmLen);
	if(pPcmBuf)
	{
		printf("malloc pPcmBuf failed!\n");
		goto fail;
	}

	for(i=0; i<AudioLen; i++)
	{
		*pPcmBuf = ulaw_to_s16(*pAudioBuf);
		pPcmBuf ++;
		pAudioBuf ++;
	}

	if(play_pcm(pcm_play, (char *)pPcmBuf, PcmLen*2) == -1)
	{
		printf("play pcm failed!\n");
		goto fail;
	}

	if(NULL != pPcmBuf)
	{
		free(pPcmBuf);
	}

	return 0;

fail:

	if(NULL != pPcmBuf)
	{
		free(pPcmBuf);
	}

	return -1;
}
