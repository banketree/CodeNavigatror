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
//#include "mediastreamer2/msjava.h"
#include "mediastreamer2/msticker.h"
}

#include <jni.h>
#include <math.h>

#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <surfaceflinger/Surface.h>  
#include <binder/IMemory.h> 
#include <android/log.h>

#include <surfaceflinger/ISurface.h>
#include <ui/GraphicBuffer.h>
#include <camera/ICamera.h>
#include <camera/ICameraClient.h>
#include <camera/ICameraService.h>
#include <ui/Overlay.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/KeyedVector.h>
#include <utils/Log.h>
#include <utils/Vector.h>
#include <utils/threads.h>



using namespace android;

class CameraReader : public CameraListener {
public:
	
	~CameraReader();
	CameraReader();
	CameraReader(MSFilter *f);
	void init(MSFilter *f);
	void release();
	
    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType, const sp<IMemory> &dataPtr);

    virtual void postDataTimestamp(
            nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);
	void setCallbackMode();

//	void getcallbackfunc(void(*p)(void* lpdata,int len) ){mcallbackfunc=p; }; 

	sp<Camera> getCamera() { return mCamera; }
//	void(*mcallbackfunc)(void* lpdata,int len) ;
//	void copyAndPost(const sp<IMemory>& dataPtr, int msgType); 

	MSFrameRateController fpsControl;
	MSAverageFPS averageFps;

	MSFilter *filter;
	MSWebCam *webcam;

	mblk_t *frame;
	float fps;
	MSVideoSize requestedSize, hwCapableSize;
	ms_mutex_t mutex;
	int rotation, rotationSavedDuringVSize;
	int useDownscaling;
	char fps_context[64];

	sp<Camera> mCamera;


};


sp<Camera> g_camera=NULL;
CameraReader g_reader;

static int android_sdk_version = 10;
//static const char* AndroidApi9WrapperPath = "java/lang/String";

static const char* AndroidApi9WrapperPath = "org/linphone/mediastream/video/capture/AndroidVideoApi9JniWrapper";
static const char* AndroidApi8WrapperPath = "org/linphone/mediastream/video/capture/AndroidVideoApi8JniWrapper";
static const char* AndroidApi5WrapperPath = "org/linphone/mediastream/video/capture/AndroidVideoApi5JniWrapper";

#define UNDEFINED_ROTATION -1

/************************ Data structures              ************************/
// Struct holding Android's cameras properties
struct AndroidWebcamConfig {
	int id;
	int frontFacing;
	int orientation;
};

struct AndroidReaderContext {
	AndroidReaderContext(MSFilter *f, MSWebCam *cam):filter(f), webcam(cam),frame(0),fps(5){
		ms_message("Creating AndroidReaderContext for Android VIDEO capture filter");
		ms_mutex_init(&mutex,NULL);
		androidCamera = 0;
		rotation = rotationSavedDuringVSize = UNDEFINED_ROTATION;
	};

	~AndroidReaderContext(){
		if (frame != 0) {
			freeb(frame);
		}
		ms_mutex_destroy(&mutex);
	};

	MSFrameRateController fpsControl;
	MSAverageFPS averageFps;

	MSFilter *filter;
	MSWebCam *webcam;

	mblk_t *frame;
	float fps;
	MSVideoSize requestedSize, hwCapableSize;
	ms_mutex_t mutex;
	int rotation, rotationSavedDuringVSize;
	int useDownscaling;
	char fps_context[64];

	sp<Camera> camera;
	jobject androidCamera;
	jobject previewWindow;
	jclass helperClass;
};

/************************ Private helper methods       ************************/
static jclass getHelperClassGlobalRef(JNIEnv *env);
static int compute_image_rotation_correction(AndroidReaderContext* d, int rotation);
static void compute_cropping_offsets(MSVideoSize hwSize, MSVideoSize outputSize, int* yoff, int* cbcroff);
static AndroidReaderContext *getContext(MSFilter *f);


/************************ MS2 filter methods           ************************/
static int video_capture_set_fps(MSFilter *f, void *arg){

	AndroidReaderContext* d = (AndroidReaderContext*) f->data;
	d->fps=*((float*)arg);
	return 0;
}

static int video_capture_set_autofocus(MSFilter *f, void* data){

//	JNIEnv *env = ms_get_jni_env();
	//AndroidReaderContext* d = (AndroidReaderContext*) f->data;
	//jmethodID method = env->GetStaticMethodID(d->helperClass,"activateAutoFocus", "(Ljava/lang/Object;)V");
	//env->CallStaticObjectMethod(d->helperClass, method, d->androidCamera);
	
	return 0;
}

static int video_capture_get_fps(MSFilter *f, void *arg){
	AndroidReaderContext* d = (AndroidReaderContext*) f->data;
	*((float*)arg) = d->fps;
	return 0;
}

static int video_capture_set_vsize(MSFilter *f, void* data){
	AndroidReaderContext* d = (AndroidReaderContext*) f->data;
	ms_mutex_lock(&d->mutex);

	d->requestedSize=*(MSVideoSize*)data;

	// always request landscape mode, orientation is handled later
	if (d->requestedSize.height > d->requestedSize.width) {
		int tmp = d->requestedSize.height;
		d->requestedSize.height = d->requestedSize.width;
		d->requestedSize.width = tmp;
	}

	JNIEnv *env = ms_get_jni_env();

	jmethodID method = env->GetStaticMethodID(d->helperClass,"selectNearestResolutionAvailable", "(III)[I");

	// find neareast hw-available resolution (using jni call);
	jobject resArray = env->CallStaticObjectMethod(d->helperClass, method, ((AndroidWebcamConfig*)d->webcam->data)->id, d->requestedSize.width, d->requestedSize.height);

	if (!resArray) {
		ms_error("Failed to retrieve camera '%d' supported resolutions\n", ((AndroidWebcamConfig*)d->webcam->data)->id);
		return -1;
	}

	// handle result :
	//   - 0 : width
    //   - 1 : height
    //   - 2 : useDownscaling
	jint res[3];
   env->GetIntArrayRegion((jintArray)resArray, 0, 3, res);
	ms_message("Camera selected resolution is: %dx%d (requested: %dx%d) with downscaling?%d\n", res[0], res[1], d->requestedSize.width, d->requestedSize.height, res[2]);
	d->hwCapableSize.width =  res[0];
	d->hwCapableSize.height = res[1];
	d->useDownscaling = res[2];

	int rqSize = d->requestedSize.width * d->requestedSize.height;
	int hwSize = d->hwCapableSize.width * d->hwCapableSize.height;
	double downscale = d->useDownscaling ? 0.5 : 1;

	// if hw supplies a smaller resolution, modify requested size accordingly
	if ((hwSize * downscale * downscale) < rqSize) {
		ms_message("Camera cannot produce requested resolution %dx%d, will supply smaller one: %dx%d\n",
			d->requestedSize.width, d->requestedSize.height, (int) (res[0] * downscale), (int) (res[1]*downscale));
		d->requestedSize.width = (int) (d->hwCapableSize.width * downscale);
		d->requestedSize.height = (int) (d->hwCapableSize.height * downscale);
	} else if ((hwSize * downscale * downscale) > rqSize) {
		ms_message("Camera cannot produce requested resolution %dx%d, will capture a bigger one (%dx%d) and crop it to match encoder requested resolution\n",
			d->requestedSize.width, d->requestedSize.height, (int)(res[0] * downscale), (int)(res[1] * downscale));
	}
	
	// is phone held |_ to cam orientation ?
	if (d->rotation == UNDEFINED_ROTATION || compute_image_rotation_correction(d, d->rotation) % 180 != 0) {
		if (d->rotation == UNDEFINED_ROTATION) {
			ms_error("To produce a correct image, Mediastreamer MUST be aware of device's orientation BEFORE calling 'configure_video_source'\n"); 
			ms_warning("Capture filter do not know yet about device's orientation.\n"
				"Current assumption: device is held perpendicular to its webcam (ie: portrait mode for a phone)\n");
			d->rotationSavedDuringVSize = 0;
		} else {
			d->rotationSavedDuringVSize = d->rotation;
		}
		bool camIsLandscape = d->hwCapableSize.width > d->hwCapableSize.height;
		bool reqIsLandscape = d->requestedSize.width > d->requestedSize.height;

		// if both are landscape or both portrait, swap
		if (camIsLandscape == reqIsLandscape) {
			int t = d->requestedSize.width;
			d->requestedSize.width = d->requestedSize.height;
			d->requestedSize.height = t;
			ms_message("Swapped resolution width and height to : %dx%d\n", d->requestedSize.width, d->requestedSize.height);
		}
	} else {
		d->rotationSavedDuringVSize = d->rotation;
	}

	ms_mutex_unlock(&d->mutex);
	return 0;
}

static int video_capture_get_vsize(MSFilter *f, void* data){
	AndroidReaderContext* d = (AndroidReaderContext*) f->data;
	*(MSVideoSize*)data=d->requestedSize;
	return 0;
}

static int video_capture_get_pix_fmt(MSFilter *f, void *data){
	*(MSPixFmt*)data=MS_YUV420P;
	return 0;
}

// Java will give us a pointer to capture preview surface.
static int video_set_native_preview_window(MSFilter *f, void *arg) {
	AndroidReaderContext* d = (AndroidReaderContext*) f->data;
	ms_mutex_lock(&d->mutex);

	jobject w = *((jobject*)arg);

	if (w == d->previewWindow) {
		ms_mutex_unlock(&d->mutex);
		return 0;
	}

	JNIEnv *env = ms_get_jni_env();

	jmethodID method = env->GetStaticMethodID(d->helperClass,"setPreviewDisplaySurface", "(Ljava/lang/Object;Ljava/lang/Object;)V");

	if (d->androidCamera) {
		if (d->previewWindow == 0) {
			ms_message("Preview capture window set for the 1st time (win: %p rotation:%d)\n", w, d->rotation);
		} else {
			ms_message("Preview capture window changed (oldwin: %p newwin: %p rotation:%d)\n", d->previewWindow, w, d->rotation);

			env->CallStaticVoidMethod(d->helperClass,
						env->GetStaticMethodID(d->helperClass,"stopRecording", "(Ljava/lang/Object;)V"),
						d->androidCamera);
			env->DeleteGlobalRef(d->androidCamera);
			d->androidCamera = env->NewGlobalRef(
			env->CallStaticObjectMethod(d->helperClass,
						env->GetStaticMethodID(d->helperClass,"startRecording", "(IIIIIJ)Ljava/lang/Object;"),
						((AndroidWebcamConfig*)d->webcam->data)->id,
						d->hwCapableSize.width,
						d->hwCapableSize.height,
						(jint)d->fps,
						(d->rotation != UNDEFINED_ROTATION) ? d->rotation:0,
						(jlong)d));

		}
		// if previewWindow AND camera are valid => set preview window
		if (w && d->androidCamera)
			env->CallStaticVoidMethod(d->helperClass, method, d->androidCamera, w);
	} else {
		ms_message("Preview capture window set but camera not created yet; remembering it for later use\n");
	}
	if (d->previewWindow) {
		ms_message("Deleting previous preview window %p", d->previewWindow);
		env->DeleteGlobalRef(d->previewWindow);
	}
	d->previewWindow = w;

	ms_mutex_unlock(&d->mutex);
	return 0;
}

static int video_get_native_preview_window(MSFilter *f, void *arg) {
    AndroidReaderContext* d = (AndroidReaderContext*) f->data;
    arg = &d->previewWindow;
    return 0;
}

static int video_set_device_rotation(MSFilter* f, void* arg) {
	AndroidReaderContext* d = (AndroidReaderContext*) f->data;
	d->rotation=*((int*)arg);
	ms_message("%s : %d\n", __FUNCTION__, d->rotation);
	return 0;
}

CameraReader::CameraReader()
{
	ms_mutex_init(&mutex,NULL);
	init(NULL);
}

CameraReader::CameraReader(MSFilter *f)
{
	ms_mutex_init(&mutex,NULL);
	init(f);
}
void CameraReader::init(MSFilter *f)
{
	mCamera = 0;
	filter = f;
	frame = 0;
	fps = 5;
	rotation = rotationSavedDuringVSize = UNDEFINED_ROTATION;

}
void CameraReader::release(){
	filter = 0;
	if (frame != 0) {
		freemsg(frame);
		frame = 0;
	}
}

CameraReader::~CameraReader(){
	release();
	ms_mutex_destroy(&mutex);

};

void CameraReader::notify(int32_t msgType, int32_t ext1, int32_t ext2) {
	LOGD("notify(%d, %d, %d)", msgType, ext1, ext2);
}
FILE *fp;
void CameraReader::postData(int32_t msgType, const sp<IMemory> &dataPtr) {
//	CameraReader* d = (CameraReader*) filter->data;
	ms_mutex_lock(&mutex);
	if(filter ==0)
	{
		ms_mutex_unlock(&mutex);
		return;
	}
	printf("postData\n");
	if(dataPtr!=NULL)
	{
		ssize_t offset;  
		size_t size;  
  		int width = 352;
		int height = 288;
        sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);  
		
		uint8_t* y_src = (uint8_t*)(heap->base()+offset);
		uint8_t* cbcr_src = (uint8_t*) (y_src + width * height);

		if(fp)
			fwrite(y_src,1,width*height*3/2,fp);
		/* Warning note: image_rotation_correction == 90 does not imply portrait mode !
		   (incorrect function naming).
		   It only implies one thing: image needs to rotated by that amount to be correctly
		   displayed.
		*/

		
		MSPicture pict;
		
		mblk_t *yuv_block = ms_yuv_buf_alloc(&pict, 352, 288);
		if(yuv_block){
			LOGD("size=%d\n",size);
			size = 352*288;
			memcpy(yuv_block->b_rptr,y_src,size);
			memset(yuv_block->b_rptr+size,0,size/2);
			if(frame)
			{
				LOGD("CameraReader::postData frame is not send free\n");
				freemsg(frame);
			}
			frame =	yuv_block; 
	}
	}
	ms_mutex_unlock(&mutex);
	LOGD("postdata out\n");
}
void CameraReader::postDataTimestamp(
        nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) {
    postData(msgType, dataPtr);
	return;
}

void CameraReader::setCallbackMode()
{
	mCamera->setPreviewCallbackFlags(FRAME_CALLBACK_FLAG_CAMERA);
}
void video_capture_preprocess(MSFilter *f){
	LOGD("Preprocessing of Android VIDEO capture filter\n");
	sp<Surface> sps = NULL;  

	CameraReader* d = (CameraReader*) f->data;;
	fp = fopen("test.yuv","wb+");
	if(!fp)
		LOGD("open test.yuv error");
	ms_mutex_lock(&d->mutex);

	snprintf(d->fps_context, sizeof(d->fps_context), "Captured mean fps=%%f, expected=%f", d->fps);
//	ms_video_init_framerate_controller(&d->fpsControl, d->fps);
//	ms_video_init_average_fps(&d->averageFps, d->fps_context);

	

	if(g_camera == NULL)
		return;
	
	g_camera->setListener(d);

	if (g_camera->setPreviewDisplay(sps) != NO_ERROR) 
	{
		LOGD("setPreviewDisplay error\n");
		return;
	}
	//printf("Starting Android camera '%d' (rotation:%d)\n", ((AndroidWebcamConfig*)d->webcam->data)->id, d->rotation);
	
	//ÉèÖÃ²ÎÊý
	CameraParameters param(g_camera->getParameters());

	param.setPreviewFormat("yuv420p");
	param.setPreviewSize(320,240);
	param.setPreviewFrameRate(5);
	param.set(CameraParameters::KEY_ANTIBANDING,"50hz");

	LOGD("format=%s fps_rang:%s,fps=%d\n",param.getPreviewFormat(),param.get(CameraParameters::KEY_PREVIEW_FPS_RANGE),param.getPreviewFrameRate());
	LOGD("KEY_SUPPORTED_ANTIBANDING=%s,%s\n",param.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING),param.get(CameraParameters::KEY_ANTIBANDING));


	g_camera->setParameters(param.flatten());

	g_camera->setPreviewCallbackFlags(FRAME_CALLBACK_FLAG_CAMERA); 



	if( (g_camera->startPreview()) !=NO_ERROR)
	{
		LOGD("startPreview error\n");
		return;
	}
	
	d->mCamera= g_camera;

	ms_mutex_unlock(&d->mutex);

	LOGD("video_capture_preprocess out\n");
}

static void video_capture_process(MSFilter *f){
	CameraReader* d = (CameraReader*) f->data;;
	ms_mutex_lock(&d->mutex);
	//LOGD("video_capture_process\n");
	// If frame not ready, return
	if (d->frame == 0) {
		//LOGD("video_capture_process not frame\n");
		
		ms_mutex_unlock(&d->mutex);
		return;
	}
	
	LOGD("video_capture_process have frame\n");
//	ms_video_update_average_fps(&d->averageFps, f->ticker->time);
	ms_queue_put(f->outputs[0],d->frame);
	d->frame = 0;
	ms_mutex_unlock(&d->mutex);
	LOGD("video_capture_process out\n");

}

static void video_capture_postprocess(MSFilter *f){
	LOGD("Postprocessing of Android VIDEO capture filter\n");
}

static void video_capture_init(MSFilter *f) {

	CameraReader* d = &g_reader;
	d->init(f);
	ms_mutex_lock(&d->mutex);

	LOGD("Init of Android VIDEO capture filter (%p)\n", d);
	f->data = d;
	ms_mutex_unlock(&d->mutex);
}

static void video_capture_uninit(MSFilter *f) {
	LOGD("Uninit of Android VIDEO capture filter\n");

	CameraReader* d = (CameraReader*) f->data;;

	ms_mutex_lock(&d->mutex);

	if (d->mCamera!=0) {

		d->mCamera->stopPreview();
		d->mCamera->setPreviewCallbackFlags(FRAME_CALLBACK_FLAG_NOOP);
		d->mCamera = 0;
	}
	ms_mutex_unlock(&d->mutex);
	d->release();
}

static MSFilterMethod video_capture_methods[]={
		{	MS_FILTER_SET_FPS,	&video_capture_set_fps},
		{	MS_FILTER_GET_FPS,	&video_capture_get_fps},
		{	MS_FILTER_SET_VIDEO_SIZE, &video_capture_set_vsize},
		{	MS_FILTER_GET_VIDEO_SIZE, &video_capture_get_vsize},
		{	MS_FILTER_GET_PIX_FMT, &video_capture_get_pix_fmt},
		{	MS_VIDEO_DISPLAY_SET_NATIVE_WINDOW_ID , &video_set_native_preview_window },//preview is managed by capture filter
		{	MS_VIDEO_DISPLAY_GET_NATIVE_WINDOW_ID , &video_get_native_preview_window },
		{   MS_VIDEO_CAPTURE_SET_DEVICE_ORIENTATION, &video_set_device_rotation },
		{   MS_VIDEO_CAPTURE_SET_AUTOFOCUS, &video_capture_set_autofocus },
		{	0,0 }
};

MSFilterDesc ms_video_capture_desc={
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

MS_FILTER_DESC_EXPORT(ms_video_capture_desc)

/* Webcam methods */
static void video_capture_detect(MSWebCamManager *obj);
static void video_capture_cam_init(MSWebCam *cam);

static void video_capture_cam_init(MSWebCam *cam){
	printf("Android VIDEO capture filter cam init\n");
}

static MSFilter *video_capture_create_reader(MSWebCam *obj){
	printf("Instanciating Android VIDEO capture MS filter\n");

	MSFilter* lFilter = ms_filter_new_from_desc(&ms_video_capture_desc);
//	getContext(lFilter)->webcam = obj;
	
	return lFilter;
}

MSWebCamDesc ms_android_video_capture_desc={
		"AndroidVideoCapture",
		&video_capture_detect,
		NULL,
		&video_capture_create_reader,
		NULL
};
static void* bind_loop(void* arg)
{
	ProcessState::self()->startThreadPool();
}

extern "C"{
extern void msx264_init();
}
static void video_capture_detect(MSWebCamManager *obj){
	LOGD("Detecting Android VIDEO cards camera \n");
	int count= Camera::getNumberOfCameras();
	LOGD("%d cards detected\n", count);
	
	if(count <1)
		return;
	
	g_camera=Camera::connect(0);
	if(g_camera==NULL)
	{
		LOGD("Camera open faild\n");
		return;
	}
	if(g_camera->getStatus()!=NO_ERROR)
	{
		LOGD("Camera open error\n");
		return;

	}
	
	
	MSWebCam *cam = ms_web_cam_new(&ms_android_video_capture_desc);
	cam->name = ms_strdup("Android video name");
	char* idstring = (char*) malloc(15);
	snprintf(idstring, 15, "Android%d", 0);
	cam->id = idstring;
	ms_web_cam_manager_add_cam(obj,cam);
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, bind_loop, NULL);
	//printf("camera created: id=%d frontFacing=%d orientation=%d [msid:%s]\n", cfg->id, cfg->frontFacing, cfg->orientation, idstring);
	printf("Detection of Android VIDEO cards done\n");
//	set_thread_priority(10);
	msx264_init();
}



/************************ JNI methods                  ************************/
#ifdef __cplusplus
extern "C" {
#endif
	
JNIEXPORT void JNICALL Java_org_linphone_mediastream_video_capture_AndroidVideoApi5JniWrapper_setAndroidSdkVersion
  (JNIEnv *env, jclass c, jint version) {
	android_sdk_version = version;
	ms_message("Android SDK version: %d\n", android_sdk_version);
}

JNIEXPORT void JNICALL Java_org_linphone_mediastream_video_capture_AndroidVideoApi5JniWrapper_putImage(JNIEnv*  env,
		jclass  thiz,jlong nativePtr,jbyteArray frame) {
	AndroidReaderContext* d = (AndroidReaderContext*) nativePtr;
	if (!d->androidCamera)
		return;
	ms_mutex_lock(&d->mutex);

	if (!ms_video_capture_new_frame(&d->fpsControl,d->filter->ticker->time)) {
		ms_mutex_unlock(&d->mutex);
		return;
	}

	if (d->rotation != UNDEFINED_ROTATION && d->rotationSavedDuringVSize != d->rotation) {
		ms_warning("Rotation has changed (new value: %d) since vsize was run (old value: %d)."
					"Will produce inverted images. Use set_device_orientation() then update call.\n",
			d->rotation, d->rotationSavedDuringVSize);
	}

	int image_rotation_correction = compute_image_rotation_correction(d, d->rotationSavedDuringVSize);

	jboolean isCopied;
	jbyte* jinternal_buff = env->GetByteArrayElements(frame, &isCopied);
	if (isCopied) {
		ms_warning("The video frame received from Java has been copied");
	}

	int y_cropping_offset=0, cbcr_cropping_offset=0;
	//compute_cropping_offsets(d->hwCapableSize, d->requestedSize, &y_cropping_offset, &cbcr_cropping_offset);

	int width = d->hwCapableSize.width;
	int height = d->hwCapableSize.height;

	uint8_t* y_src = (uint8_t*)(jinternal_buff + y_cropping_offset);
	uint8_t* cbcr_src = (uint8_t*) (jinternal_buff + width * height + cbcr_cropping_offset);

	/* Warning note: image_rotation_correction == 90 does not imply portrait mode !
	   (incorrect function naming).
	   It only implies one thing: image needs to rotated by that amount to be correctly
	   displayed.
	*/
 	mblk_t* yuv_block = copy_ycbcrbiplanar_to_true_yuv_with_rotation_and_down_scale_by_2(y_src
														, cbcr_src
														, image_rotation_correction
														, d->requestedSize.width
														, d->requestedSize.height
														, d->hwCapableSize.width
														, d->hwCapableSize.width,
														false,
														d->useDownscaling);
	if (yuv_block) {
		if (d->frame)
			freemsg(d->frame);
		d->frame = yuv_block;
	}
	ms_mutex_unlock(&d->mutex);

	// JNI_ABORT free the buffer without copying back the possible changes
	env->ReleaseByteArrayElements(frame, jinternal_buff, JNI_ABORT);
}

#ifdef __cplusplus
}
#endif

static int compute_image_rotation_correction(AndroidReaderContext* d, int rotation) {
	AndroidWebcamConfig* conf = (AndroidWebcamConfig*)(AndroidWebcamConfig*)d->webcam->data;

	int result;
	if (conf->frontFacing) {
		ms_debug("%s: %d + %d\n", __FUNCTION__, ((AndroidWebcamConfig*)d->webcam->data)->orientation, rotation);
	 	result = ((AndroidWebcamConfig*)d->webcam->data)->orientation + rotation;
	} else {
		ms_debug("%s: %d - %d\n", __FUNCTION__, ((AndroidWebcamConfig*)d->webcam->data)->orientation, rotation);
	 	result = ((AndroidWebcamConfig*)d->webcam->data)->orientation - rotation;
	}
	while(result < 0)
		result += 360;
	return result % 360;
}

static void compute_cropping_offsets(MSVideoSize hwSize, MSVideoSize outputSize, int* yoff, int* cbcroff) {
	// if hw <= out -> return
	if (hwSize.width * hwSize.height <= outputSize.width * outputSize.height) {
		*yoff = 0;
		*cbcroff = 0;
		return;
	}

	int halfDiffW = (hwSize.width - ((outputSize.width>outputSize.height)?outputSize.width:outputSize.height)) / 2;
	int halfDiffH = (hwSize.height - ((outputSize.width<outputSize.height)?outputSize.width:outputSize.height)) / 2;

	*yoff = hwSize.width * halfDiffH + halfDiffW;
	*cbcroff = hwSize.width * halfDiffH * 0.5 + halfDiffW;
}

static jclass getHelperClassGlobalRef(JNIEnv *env) {
	ms_message("getHelperClassGlobalRef (env: %p)", env);
	// FindClass only returns local references.
	if (android_sdk_version >= 9) {
		jclass c = env->FindClass(AndroidApi9WrapperPath);
		if (c == 0)
			ms_error("Could not load class '%s' (%d)", AndroidApi9WrapperPath, android_sdk_version);
		else
			ms_message("getHelperClassGlobalRef success", env);
		return reinterpret_cast<jclass>(env->NewGlobalRef(c));
	} else if (android_sdk_version >= 8) {
		jclass c = env->FindClass(AndroidApi8WrapperPath);
		if (c == 0)
			ms_error("Could not load class '%s' (%d)", AndroidApi8WrapperPath, android_sdk_version);
		return reinterpret_cast<jclass>(env->NewGlobalRef(c));
	} else {
		jclass c = env->FindClass(AndroidApi5WrapperPath);
		if (c == 0)
			ms_error("Could not load class '%s' (%d)", AndroidApi5WrapperPath, android_sdk_version);
		return reinterpret_cast<jclass>(env->NewGlobalRef(c));
	}
}

