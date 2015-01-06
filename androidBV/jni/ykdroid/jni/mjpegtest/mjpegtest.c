#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "CFimcMjpeg.h"
#define VIDEO0 0
#define VIDEO2	2


int main(void)
{
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;
	struct timezone tz1;
	struct timezone tz2;
	struct timezone tz3;
	int JpegLen = 50000;
	int LibJpegLen = -1;
	unsigned char *pLibJpegBuf = NULL;


	printf("cam init begin!\n");

	if(CamInit(VIDEO2, 640, 480) < 0)
	{
		printf("cam init failed!\n");
		return -1;
	}
	printf("cam init finish!\n");

	printf("cam 2!\n");
	pLibJpegBuf = (unsigned char *)malloc(JpegLen);
	if(pLibJpegBuf == NULL)
	{
		printf("malloc pJpegBuf failed! \n");
		return -1;
	}

	//计算起始时间
	gettimeofday(&tv1, &tz1);

	LibJpegLen = CamCapture(pLibJpegBuf);
	printf("cam 4! LibJpegLen = %d \n",LibJpegLen);
	if(LibJpegLen < 0)
	{
		printf("error:LibJpegLen < 0 \n");
		return -1;
	}

	//计算结束时间
	gettimeofday(&tv2, &tz2);
	printf("begin time is:time_sec=%d, time_usec=%d\n", tv1.tv_sec, tv1.tv_usec);
	printf("finish time is:time_sec=%d, time_usec=%d\n", tv2.tv_sec, tv2.tv_usec);
	printf("LibJpegLen=%d\n", LibJpegLen);


	FILE *fp = fopen("/data/data/test_libjpeg.jpeg", "w");
	if(fp == NULL)
	{
		printf("CamCapture open file faild! \n");
		return -1;
	}
	fwrite(pLibJpegBuf, 1, LibJpegLen, fp);
	fclose(fp);
	printf("write end!\n");

	if(NULL != pLibJpegBuf)
	{
		printf("cam 5!\n");
		free(pLibJpegBuf);
	}
	printf("cam 6!\n");

	if(CamClose() < 0)
	{
		printf("CamClose faild! \n");
		return -1;
	}
	printf("cam 7!\n");
	return 0;
}

