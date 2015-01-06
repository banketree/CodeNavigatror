#include "CFimc.h"
#include <LANIntfLayer.h>

#include <jni.h>
#include <android/log.h>
#include "CFimcMjpeg.h"

//#define _READ_JPEG_FILE

#ifdef _READ_JPEG_FILE
FILE *readjpeg_fd;
#endif

CFIMC cam;
int videofd=0;

//����-1��ʾ��ʼ��ʧ��
//����>0����ֵ��ʾ��ʼ���ɹ�
int CamInit(int videoid, int width, int height)
{
#ifdef _READ_JPEG_FILE
	readjpeg_fd = fopen("/data/data/com.fsti.ladder/test_jpeg.jpeg", "r");
	return 0;
#else
	if(videofd <= 0)
		videofd = cam.init_mjpeg_video(videoid, width, height);
	return videofd;
#endif
}

//����-1����ʾʧ��
//���ش���1��ֵ����ʾ��ȡ��image����
unsigned int CamCapture(unsigned char *pLibJpegBuf)
{
	int jpegLen = 0;

#ifdef _READ_JPEG_FILE
	fseek(readjpeg_fd, 0, SEEK_END);
	jpegLen = ftell(readjpeg_fd);
	fseek(readjpeg_fd, 0, SEEK_SET);
	fread(pLibJpegBuf, sizeof(char), jpegLen, readjpeg_fd);
#else
	jpegLen = cam.getLibjpeg(pLibJpegBuf);
#endif

	return jpegLen;

}

//����-1����ʾʧ��
//����0��ʾ�ɹ�
int CamClose(void)
{
	int ret = 0;

#ifdef _READ_JPEG_FILE
	fclose(readjpeg_fd);
#else
	if(cam.uninit_mjpeg_video() == -1)
	{
		LOGE("CFIMC uninit camera for picture faild");
		ret = -1;
	}

	if(cam.m_cam_fd > 0)
	{
		if(close(cam.m_cam_fd) != 0)
		{
			LOGE("CFIMC close cam faild,try again!");
			return -1;
		}
		cam.m_cam_fd = -1;
	}
#endif

	return ret;
}

