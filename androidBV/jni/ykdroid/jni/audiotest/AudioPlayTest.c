#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

//#include "AudioPlay.h"


int main(int arc, char **argv)
{
	struct pcm *pcm_play;
	int PcmSampleRate = 0;
	int MoNo = 0;
	unsigned bufsize = 0;
	char namebuf[50] = {0};

	PcmSampleRate = atoi(argv[2]);
	MoNo = atoi(argv[3]);

	printf("the sample parament is : PcmSampleRate=%d, MoNo=%d\n", PcmSampleRate, MoNo);
	pcm_play = play_open(PcmSampleRate, MoNo);
	if(pcm_play == NULL)
	{
		printf("open pcm failed!\n");
		goto fail;
	}

	strcpy(namebuf, "/data/data/");
	strcat(namebuf, argv[1]);

	FILE *fp = fopen(namebuf, "r");
	if(NULL == fp)
	{
		printf("open file failed!\n");
		goto fail;
	}

	bufsize = pcm_buffer_size(pcm_play);
	char *AudioBuf = (char *)malloc(bufsize);
	if(AudioBuf == NULL)
	{
		printf("malloc for audio buf failed!\n");
		goto fail;
	}

	while (fread(AudioBuf, 1, bufsize, fp) == bufsize)
	{
//		printf("bufsize=%d\n", bufsize);
		if(play_pcm(pcm_play, AudioBuf, bufsize) == -1)
		{
			printf("paly pcm failed!\n");
			break;
		}
//		usleep(10000);
	}

	printf("paly pcm finish!\n");

	if(NULL != AudioBuf)
	{
		free(AudioBuf);
	}

	pcm_close(pcm_play);

	return 0;

fail:

	if(pcm_play != NULL)
	{
		pcm_close(pcm_play);
	}

	if(NULL != AudioBuf)
	{
		free(AudioBuf);
	}
	return -1;
}
