#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "CFimcMpeg4.h"
#include "MfcMpeg4Enc.h"

#define VIDEO0 0
#define VIDEO2	2


//  应用程序名   视频图像高度  视频图像宽度  视频帧率  视频质量  视频码率 视频帧数
// mpeg4test 352 288 15 30 64000 100
int main(int argc, char *argv[])
{
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;
	struct timezone tz1;
	struct timezone tz2;
	struct timezone tz3;
	int BufLen = 50000;
	int Mpeg4Len = -1;
	int VideoLen = -1;
	int TotalMpeg4Len = 0;
	int i=0;
	int j=0;
	int VideoFd;
	int index;
	unsigned char *pMpeg4Buf = NULL;
	void *pVideoBuf = NULL;
	VIDEO_MPEG4ENC_ST *pVideo = NULL;
	char *pMpeg4 = NULL;
	char VideoBuffer[500000] = {0};
	void *Y;
	void *UV;

	if(argc != 7)
	{
		printf("please check input formats!\n");
		return -1;
	}

	int Width = atoi(argv[1]);
	int Height = atoi(argv[2]);
	int FrameRate = atoi(argv[3]);
	int Quality = atoi(argv[4]);
	int Bitrate = atoi(argv[5]);
	int TotalFrames = atoi(argv[6]);

	printf("cam init begin!\n");
	VideoFd = CamInit(VIDEO2, Width, Height);
	if(VideoFd < 0)
	{
		printf("cam init failed!\n");
		return -1;
	}
	printf("cam init finish!\n");


	pVideo = (VIDEO_MPEG4ENC_ST *)malloc(sizeof(VIDEO_MPEG4ENC_ST));
	if(pVideo == NULL)
	{
		printf("malloc for pVideo failed!\n");
		return -1;
	}
//	printf("malloc for pVideo, the size is %d \n", sizeof(VIDEO_MPEG4ENC_ST));

	pVideo->VideoPlaner.Height = Height;
	pVideo->VideoPlaner.Width = Width;
	pVideo->VideoPlaner.YSize = ALIGN_TO_8KB(ALIGN_TO_128B(Width) * ALIGN_TO_32B(Height));
	pVideo->VideoPlaner.CSize = ALIGN_TO_8KB(ALIGN_TO_128B(Width) * ALIGN_TO_32B(Height/2));
//	pVideo->VideoPlaner.YSize = Height * Width;
//	pVideo->VideoPlaner.CSize = Height * Width / 2;
	pVideo->Frames = FrameRate;
	pVideo->Quality = Quality;
	pVideo->Bitrate = Bitrate;

	printf("pVideo->VideoPlaner.YSize = %d, pVideo->VideoPlaner.CSize = %d\n", pVideo->VideoPlaner.YSize, pVideo->VideoPlaner.CSize);

	//进行编码器初始化
	if(MfcEncMpeg4Init(pVideo) != OMX_ErrorNone)
	{
		printf("mpeg4enc init failed!\n");
		goto EXIT;
	}


	FILE *fp_nv12t = fopen("/data/data/test_nv12t.yuv", "w+");
	if(fp_nv12t == NULL)
	{
		printf("CamCapture open yuv file faild! \n");
		goto EXIT;
	}


#if 0
	FILE *fp_yuv420p = fopen("/data/data/test_yuv420p.yuv", "w+");
	if(fp_yuv420p == NULL)
	{
		printf("CamCapture open yuv file faild! \n");
		goto EXIT;
	}
#endif

	FILE *fp_mpeg4 = fopen("/data/data/test.mpeg4", "w+");
	if(fp_mpeg4 == NULL)
	{
		printf("CamCapture open mpeg4 file faild! \n");
		goto EXIT;
	}

	fwrite(pVideo->header, 1, pVideo->headerSize, fp_mpeg4);
	//	printf("headerSize = %d \n", pVideo->headerSize);

	TotalMpeg4Len += pVideo->headerSize;

	MfcGetInputBuf(pVideo, &Y, &UV);
//	printf("MfcGetInputBuf():: Y(0x%08x) UV(0x%08x)\n", Y, UV);

//	//计算起始时间
//	gettimeofday(&tv1, &tz1);

	printf("TotalFrames = %d \n", TotalFrames);
//	for(i=0; i< TotalFrames; i++)
//	{
//		index = CamGetFrame(&pVideoBuf, &VideoLen);
//		printf("index = %d, VideoLen = %d\n", index, VideoLen);
//		fwrite(pVideoBuf, 1, VideoLen, fp_nv12t);
//
//		if(CamReleaseFrame(index) < 0)
//		{
//			printf("release frame failed!\n");
//		}
//		//延时25ms，再获取下一帧，相当于每秒40帧
//		usleep(25000);
//	}
//
//	fflush(fp_nv12t);
//
//	fseek(fp_nv12t, 0, SEEK_SET);

	printf("begin to mpeg4 enc!\n");

//	for(i=0; i< TotalFrames; i++)
//	{
//		if(feof(fp_nv12t))
//		{
//			if(fread(pVideoBuf, 1, VideoLen, fp_nv12t) != VideoLen)
//			{
//				printf("fread len error!\n");
//				break;
//			}
//		}
//		memcpy(Y, pVideoBuf, pVideo->VideoPlaner.YSize);
//		memcpy(UV, pVideoBuf + pVideo->VideoPlaner.YSize, pVideo->VideoPlaner.CSize);
//		//调用编码器对这一帧数据进行编码
//		if(MfcMpeg4EncProcess(pVideo, &pMpeg4, &Mpeg4Len) != OMX_ErrorNone)
//		{
//			printf("mpeg4 enc failed!\n");
//			goto EXIT2;
//		}
//		printf("i = %d, Mpeg4Len = %d\n", i, Mpeg4Len);
//		fwrite(pMpeg4, 1, Mpeg4Len, fp_mpeg4);
//	}
////end
//	goto WRITE_END;

	for(i=0; i< TotalFrames; i++)
	{

		printf("Frames = %d \n", i);
		//获取一帧原始视频数据
		index = CamGetFrame(&pVideoBuf, &VideoLen);
//		printf("index = %d, VideoLen = %d \n",index, VideoLen);
		if(VideoLen < 0)
		{
			printf("error:Mpeg4Len < 0 \n");
			goto EXIT2;
		}

		if(CamReleaseFrame(index) < 0)
		{
			printf("release frame failed!\n");
		}
//		printf("index = %d \n", index);
//	goto EXIT2;
		//获取的视频流长度与设置的效果不一致，则警告退出
//		if(VideoLen != pVideo->VideoPlaner.YSize + pVideo->VideoPlaner.CSize)
//		{
//			printf("VideoLen != pVideo->VideoPlaner.YSize + pVideo->VideoPlaner.CSize \n");
//			goto EXIT2;
//		}

//		int pixlen = Height * Width;

//************************以下测试通过**************************
//		//记录yuv文件，此时文件为NV12格式
//		fwrite(pVideoBuf, 1, VideoLen, fp_nv12);
//
//		char *pY = VideoBuffer;
//		char *pCbCr = pVideoBuf + pixlen;
//		char *pU = VideoBuffer + pixlen;
//		char *pV = VideoBuffer + pixlen + pixlen / 4;
//
//		memcpy(pY, pVideoBuf, pixlen);
//		for(j=0; j< pixlen/4; j++)
//		{
//			*pU++ = *pCbCr++;
//			*pV++ = *pCbCr++;
//		}
//
//		//记录yuv文件，此时文件为yuv420p格式
//		fwrite(pY, 1, VideoLen, fp_yuv420p);
//************************************************************

//		//将YUV420的视频数据进行内存位置调整成NV12
//		char *p_y = pVideoBuf;
//		char *p_cb = pVideoBuf + pixlen;
//		char *p_cr = pVideoBuf + pixlen + pixlen / 2;
//		char *p_cbcr = VideoBuffer + pixlen;
//		int i;
//		//先放入Y数据
//		memset(VideoBuffer, 0, 500000);
//		memcpy(VideoBuffer, p_y, pixlen);
//
//		//再放入U V数据
//		for(i=0; i< pixlen/2; i++)
//		{
//			*p_cbcr++ = *p_cb;
//			*p_cbcr++ = *p_cr;
//		}

//		int YAddr = CamGetYAddr(index);
//		int CAddr = CamGetCAddr(index);

//		printf("YAddr=0x%08x, CAddr=0x%08x\n", YAddr, CAddr);
//		printf("YLEN = CAddr-YAddr = %d\n", CAddr-YAddr);
//		memcpy(Y, &YAddr, (CAddr-YAddr));
//		memcpy(UV, &CAddr, VideoLen-(CAddr-YAddr));

#if 1
		memcpy(Y, pVideoBuf, pVideo->VideoPlaner.YSize);
		memcpy(UV, pVideoBuf + pVideo->VideoPlaner.YSize, pVideo->VideoPlaner.CSize);
#endif

//		printf("MfcMpeg4EncProcess(pVideo) begin\n");
		//调用编码器对这一帧数据进行编码
		if(MfcMpeg4EncProcess(pVideo, &pMpeg4, &Mpeg4Len) != OMX_ErrorNone)
		{
			printf("mpeg4 enc failed!\n");
			goto EXIT2;
		}
//		printf("MfcMpeg4EncProcess(pVideo) finish\n");
	//	//计算结束时间
	//	gettimeofday(&tv2, &tz2);
	//	printf("begin time is:time_sec=%d, time_usec=%d\n", tv1.tv_sec, tv1.tv_usec);
	//	printf("finish time is:time_sec=%d, time_usec=%d\n", tv2.tv_sec, tv2.tv_usec);

		printf("Mpeg4Len=%d\n", Mpeg4Len);

		TotalMpeg4Len += Mpeg4Len;

//		fwrite(pVideo->header, 1, pVideo->headerSize, fp_mpeg4);

		fwrite(pMpeg4, 1, Mpeg4Len, fp_mpeg4);

//		if(CamReleaseFrame(index) < 0);
//		{
//			printf("release frame failed!\n");
//		}

		//延时25ms，再获取下一帧，相当于每秒40帧
		usleep(25000);
	}

	printf("TotalMpeg4Len=%d\n", TotalMpeg4Len);

WRITE_END:

	if(fp_nv12t != NULL)
	{
		fclose(fp_nv12t);
	}

	if(fp_mpeg4 != NULL)
	{
		fclose(fp_mpeg4);
	}

#if 0
	fclose(fp_nv12t);
	fclose(fp_yuv420p);
#endif


	printf("write end!\n");

	if(CamClose() < 0)
	{
		printf("CamClose faild! \n");
		return -1;
	}
	printf("encode finish!\n");

	MfcMpeg4EncExit(pVideo);

	if(pVideo != NULL)
	{
		free(pVideo);
	}


	return 0;

EXIT2:
	MfcMpeg4EncExit(pVideo);
	if(pVideo != NULL)
	{
		free(pVideo);
	}

EXIT:

	if(VideoFd > 0)
	{
		if(CamClose() < 0)
		{
			printf("CamClose faild! \n");
			return -1;
		}
	}

	if(fp_mpeg4 != NULL)
	{
		fclose(fp_mpeg4);
	}
	return -1;
}

