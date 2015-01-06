#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "android/log.h"

#include "linuxaudio.h"

	char readBuf[5000] = {0};
	char writeBuf[5000] = {0};

int main(void)
{

	int readAduioLen = 0;
	int readLen = 0;
	int readTotalLen = 0;
	int ret = 0;

	if(linux_snd_card_detect() < 0)
	{
		printf("linux_snd_card_detect failed!\n");
		return -1;
	}

	if(linux_snd_read_init() < 0)
	{
		printf("linux_snd_read_init failed!\n");
		return -1;
	}

	if(linux_snd_write_init() < 0)
	{
		ret = -1;
		printf("linux_snd_write_init \n");
		linux_snd_read_uninit();
		return -1;
	}

	FILE *pfAudioRecord = fopen("/data/data/linuxaudiorecord", "w+");
	if(pfAudioRecord == NULL)
	{
		printf("open file failed!\n");
		return -1;
	}

	for(int i=0; i<2000; i++)
	{
//		readAduioLen = linux_snd_read(readBuf);

		readAduioLen = AudioRecord(readBuf, 128);

		if(readAduioLen <= 0)
		{
			printf("i=%d, readAduioLen = 0\n", i);
			usleep(1000);
			continue;
		}

		printf("i=%d, readAduioLen = %d\n", i, readAduioLen);
//		if(readAduioLen > 128)
//		{
//			readAduioLen = 128;
//		}
		fwrite(readBuf, 1, readAduioLen, pfAudioRecord);

//		memset(readBuf, 0, 5000);
		readTotalLen += readAduioLen;

		usleep(10000);
	}

	fflush(pfAudioRecord);

	fseek(pfAudioRecord, 0, SEEK_SET);

	printf("readTotalLen = %d\n", readTotalLen);


	while(!feof(pfAudioRecord))
	{
		readLen = fread(writeBuf, 1, 160, pfAudioRecord);

//		printf("*******************linux_snd_write = %d\n", readLen);

		if(readLen > 0)
		{
			AudioPlay(writeBuf, readLen);
		}

		memset(writeBuf, 0, 5000);

		usleep(10000);
	}

	printf("write end!\n");

	while(linux_snd_is_write_end())
	{
		usleep(5000);
	}

	fclose(pfAudioRecord);

err1:
	linux_snd_write_uninit();

err0:
	linux_snd_read_uninit();

	return ret;
}
