#include "CFimc.h"
#include "CFimcMpeg4.h"
#include <android/log.h>

CFIMC cam;
int videofd = -1;

int CamInit(int videoid, int width, int height)
{
	//videoid = 0 »ò videoid = 2
	videofd = cam.init_mpeg4_video(videoid, width, height);

	return videofd;
}

int CamGetFrame(void **pFrameBuf, int *FrameLen)
{
	int index = 0;

	index = cam.getRecordFrame(pFrameBuf, FrameLen);

	return index;
}

int CamClose(void)
{
	int ret;

	if(cam.uninit_mpeg4_video() == -1)
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

	return ret;
}

int CamGetYAddr(int index)
{
	return cam.getRecPhyAddrY(index);
}

int CamGetCAddr(int index)
{
	return cam.getRecPhyAddrC(index);
}

int CamReleaseFrame(int index)
{
	return cam.releaseRecordFrame(index);
}
