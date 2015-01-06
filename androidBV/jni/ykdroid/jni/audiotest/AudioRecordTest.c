#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "AudioRecord.h"
int PcmFrameSize;

//命令格式 audiorec 8000 1 10 1.5
int main(int arc, char **argv)
{
	int AudioLen = 0;
	char *pAudioBuf = NULL;
	int PcmSampleRate = 0;
	int MoNo = 0;
	int nFrame = 0;
	double AmplifyRadio;

	PcmSampleRate = atoi(argv[1]);
	MoNo = atoi(argv[2]);
	nFrame = atoi(argv[3]);
	AmplifyRadio = atof(argv[4]);		//表示放大倍数，例如输入1.2表示新的pcm是的音强是原来的1.2倍

	printf("the sample parament is : PcmSampleRate=%d, MoNo=%d, nFrame=%d, AmplifyRadio=%.1f\n", PcmSampleRate, MoNo, nFrame, AmplifyRadio);

	//采样率为8K，单声道
	PcmFrameSize = record_open(PcmSampleRate, MoNo);
	if(PcmFrameSize == -1)
	{
		printf("open pcm failed!\n");
		return -1;
	}

	pAudioBuf = (char *)malloc(PcmFrameSize * nFrame);
	if(NULL == pAudioBuf)
	{
		printf("malloc for record audio failed!\n");
		return -1;
	}

	AudioLen = AudioRecord(pAudioBuf, PcmFrameSize, nFrame, AmplifyRadio);
	if(AudioLen == -1)
	{
		printf("error:pPcmBuf is NULL!\n");
		return -1;
	}

	FILE *fp = fopen("/data/data/recordg711file", "w");
	if(NULL == fp)
	{
		printf("open file failed!\n");
		return -1;
	}

	fwrite(pAudioBuf, 1, AudioLen, fp);
	printf("wirte end!\n");

	if(NULL != pAudioBuf)
	{
		free(pAudioBuf);
	}

	if(NULL != fp)
	{
		fclose(fp);
	}

	record_close();

	return 0;
}
