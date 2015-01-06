/*
mediastreamer2 library - modular sound and video processing and streaming
This is the video capture filter for Android.
It uses one of the JNI wrappers to access Android video capture API.
See:
	org.linphone.mediastream.video.capture.AndroidVideoApi9JniWrapper
	org.linphone.mediastream.video.capture.AndroidVideoApi8JniWrapper
	org.linphone.mediastream.video.capture.AndroidVideoApi5JniWrapper

 * Copyright (C) 2010  Belledonne Communications, Grenoble, France

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

extern "C" {
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/mswebcam.h"
#include "mediastreamer2/msticker.h"
#include "SMEventHndl.h"

}

#include <IDBTCfg.h>
#include "CFimc.h"
CFIMC g_cam;

int g_capture=0;



extern "C" int CFimcReInitVideo(int width, int height)
{
	g_cam.auto_mode_width = width;
	g_cam.auto_mode_height = height;
	g_cam.uninit_video();
	if(g_cam.init_video(0, width, height) < 0){
		LOGE("CFIMC camera init faild\n");
	}
}

extern "C" int getCFimcAutoModeWidth()
{
	return g_cam.auto_mode_width;
}
extern "C" int getCFimcAudoModeHeight()
{
	return g_cam.auto_mode_height;
}
//extern "C" void setCFimcAutoModeWidth(int width)
//{
//	g_cam.audo_mode_width = width;
//}
//extern "C" void setCFimcAudoModeHeight(int height)
//{
//	g_cam.audo_mode_height = height;
//}


static void video_capture_preprocess(MSFilter *f){
	if(SMGetSenseState())
	{
		SMCtlLightLed(1);	//打开补光灯
	}
	if(g_cam.start()<0){
		LOGE("fimc camera start faild\n");
	}
	g_capture=0;
	return;
}

static void video_capture_process(MSFilter *f){
	mblk_t *im = g_cam.getqueue();
	if(im){
//		if(g_capture==0){
//			LOGV("MSG(%s):capture a frame", __func__);
			ms_queue_put(f->outputs[0],im);
//			g_capture++;
//		}
//		else {
//			int index=*( (int *)im->b_rptr);
//			freemsg(im);
//			g_cam.releaseRecordFrame(index);
//			g_capture++;
//			if(g_capture==2)
//				g_capture=0;
//		}
	}
	return;		
}

static void video_capture_postprocess(MSFilter *f){
	LOGE("android fimc capture video_capture_postprocess in\n");

	if(g_cam.stop()<0){
		LOGE("fimc camera start faild\n");
	}
	SMCtlLightLed(0);	//关闭补光灯

	LOGE("android fimc capture video_capture_postprocess out\n");
	return;
}

static void video_capture_init(MSFilter *f) {
	LOGE("init of Android Fimc VIDEO capture filter\n");
}

static void video_capture_uninit(MSFilter *f) {
	printf("Postprocessing of Android Fimc VIDEO capture filter\n");
}

MSFilterDesc ms_fimc_video_capture_desc={
		MS_S5PV210_VIDEO_CAPTURE_ID,
		"MSAndroidVideoCapture",
		N_("A filter that captures Android video."),
		MS_FILTER_OTHER,
		NULL,
		0,
		1,
		video_capture_init,
		video_capture_preprocess,
		video_capture_process,
		video_capture_postprocess,
		video_capture_uninit,
		NULL
};

MS_FILTER_DESC_EXPORT(ms_fimc_video_capture_desc)

/* Webcam methods */
static void video_capture_detect(MSWebCamManager *obj);
static void video_capture_cam_init(MSWebCam *cam);

static void video_capture_cam_init(MSWebCam *cam){
	printf("Android VIDEO capture filter cam init\n");
}

static MSFilter *video_capture_create_reader(MSWebCam *obj){
	printf("Instanciating Android VIDEO capture MS filter\n");

	MSFilter* lFilter = ms_filter_new_from_desc(&ms_fimc_video_capture_desc);
	
	return lFilter;
}

MSWebCamDesc ms_android_fimc_video_capture_desc={
		"AndroidVideoCapture",
		&video_capture_detect,
		NULL,
		&video_capture_create_reader,
		NULL
};


static void video_capture_detect(MSWebCamManager *obj){
	
	MSWebCam *cam =NULL;
	char* idstring;

	if(g_stIdbtCfg.iNegotiateMode == 0 && g_stIdbtCfg.iVedioFmt == 2)
	{
		LOGE("CFIMC video_capture_detect 720P\n");
		//720p
		if(g_cam.init_video(0, MS_VIDEO_SIZE_720P_W, MS_VIDEO_SIZE_720P_H) < 0){
			printf("CFIMC camera init faild\n");
			return;
		}
	}
	else if(g_stIdbtCfg.iNegotiateMode == 0 && g_stIdbtCfg.iVedioFmt == 1)
	{
		LOGE("CFIMC video_capture_detect D1\n");
		//d1
		if(g_cam.init_video(0, MS_VIDEO_SIZE_4CIF_W, MS_VIDEO_SIZE_4CIF_H) < 0){
			printf("CFIMC camera init faild\n");
			return;
		}
	}
	else if(g_stIdbtCfg.iNegotiateMode == 0 && g_stIdbtCfg.iVedioFmt == 0)
	{
		LOGE("CFIMC video_capture_detect CIF\n");
		//cif
		if(g_cam.init_video(0, MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H) < 0){
			printf("CFIMC camera init faild\n");
			return;
		}
	}
	else if(g_stIdbtCfg.iNegotiateMode == 1)
	{
		LOGE("CFIMC iNegotiateMode == 1 will use D1\n");
		//d1
		if(g_cam.init_video(0, MS_VIDEO_SIZE_4CIF_W, MS_VIDEO_SIZE_4CIF_H) < 0){
			printf("CFIMC camera init faild\n");
			return;
		}
	}

	cam = ms_web_cam_new(&ms_android_fimc_video_capture_desc);
	cam->name = ms_strdup("Android video name");
	idstring = (char*) malloc(15);
	snprintf(idstring, 15, "Android%d", 0);
	cam->id = idstring;
	ms_web_cam_manager_add_cam(obj,cam);
	LOGD("CFIMC Detection of Android VIDEO cards done\n");
//	set_thread_priority(10);
	return;
}
