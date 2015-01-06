#ifndef __CFIMC__
#define __CFIMC__


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


#include <utils/Errors.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <mediastreamer2/mscommon.h>
#include <mediastreamer2/msqueue.h>

#include <linux/videodev2.h>
#include <videodev2_samsung.h>
#include <Jpeg.h>

#ifdef __cplusplus
extern "C" {
#endif

int capture_pic();

#ifdef __cplusplus
}
#endif

#define SIP_SCREEN_WIDTH 	352
#define SIP_SCREEN_HEIGHT	288
#define IDU_SCREEN_WIDTH	352
#define IDU_SCREEN_HEIGHT	240

using namespace android;

#define MAX_BUFFERS     8

struct buffer {
        void *                  start;
        size_t                  length;
};

#define CAMERA_DEV_NAME   "/dev/video0"
#define CAMERA_DEV_NAME2  "/dev/video2"

struct ADDRS {
    unsigned int addr_y;
    unsigned int addr_cbcr;
    unsigned int buf_idx;
};
/*
	查询接口是否具备捕获图片能力
	return:0 成功，-1失败
*/
int fimc_v4l2_querycap(int fp);
/*
	查询接口指定的ｉｄ的摄像头是否可以正常打开
	return:0 成功，-1失败
*/

const __u8* fimc_v4l2_enuminput(int fp, int index);

/*
	指定打开的摄像头id
	eturn:0 成功，-1失败
*/
int fimc_v4l2_s_input(int fp, int index);

/*
	查询接口是否支持指定的格式的输出。如yuv420,rgb565 等。
	eturn:0 成功，-1失败
*/
int fimc_v4l2_enum_fmt(int fp, unsigned int fmt);

/*
	指定打开的输出图片格式
	eturn:0 成功，-1失败
*/
int fimc_v4l2_s_fmt(int fp, int width, int height, unsigned int fmt, int flag_capture);

/*
	请求内核图片捕获缓冲区。
	eturn:0 成功，-1失败
*/
int fimc_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs);

/*
	将缓冲区放进驱动队列。
	eturn:0 成功，-1失败
*/
int fimc_v4l2_qbuf(int fp, int index);

/*
	开始捕获
	eturn:0 成功，-1失败
*/

int fimc_v4l2_streamon(int fp);

/*
	v4l2参数设置封装函数
	eturn:0 成功，-1失败
*/
int fimc_v4l2_s_ctrl(int fp, unsigned int id, unsigned int value);

class CFIMC
{
public:
	int m_cam_fd;
	//int initFormat; //1:cif 2:d1
	unsigned int image_height;
	unsigned int image_width;

	unsigned int auto_mode_height;
	unsigned int auto_mode_width;

	CFIMC();
	~CFIMC();
	//初始化视频流捕获得功能
	int init_video(int camera_id,int width, int height);
	void uninit_video();
	////初始化视频流格式以及准备相应的空间
	int init_mjpeg_video(int camera_id, unsigned int width, unsigned int height);
	int uninit_mjpeg_video(void);
	int init_mpeg4_video(int camera_id, unsigned int width, unsigned int height);
	int uninit_mpeg4_video(void);
	//初始化图片捕抓功能
	int init_pic(int camera_id);
	int change_format(int width, int height);

	//输出jpg格式图片数据缓冲，缓冲区由使用者负责free
	unsigned char *getjpg(int *len);

	unsigned int getLibjpeg(unsigned char *libjpeg_buf);

	//开始视频捕获
	int start();
	//停止视频捕获
	int stop();
	//获得队列中的图片地址
	mblk_t *getqueue();
	bool getstatus();

	//io轮询接口
	int fimc_poll(struct pollfd *events);
	//释放处理完毕的图片缓冲区，重新放入捕获队列
	int releaseRecordFrame(int index);

	//获得Y分量缓冲区物理地址
	unsigned int getAddrY(int index);
	//获得cbcr分量缓冲区物理地址
	unsigned int getAddrC(int index);
	//从驱动中获得捕获成功的图片缓冲区痛列号
	int getRecordFrame(void);

	//从驱动中获取一帧视频数据
	int getRecordFrame(void **pRecordFram, int *FrameLen);

	//设置帧率，内核未实现该功能，所以该函数不生效。
	int setFrameRate(int frame_rate);
	//捕获线程处理函数，静态函数
	static void* capture_thread(void *arg);
	int m_flag_stat;

//private:
	//获得Y分量缓冲区物理地址
	unsigned int getRecPhyAddrY(int index);
	//获得cbcr分量缓冲区物理地址
	unsigned int getRecPhyAddrC(int index);

private:
	//yuv　->jpg压缩函数。
	unsigned char *jpg_compress(void *yuv_buf,int *jpgsize);
	unsigned int libjpeg_compress(unsigned char *image, int width, int height, int quality, unsigned char **jpeg_buf);
	//yuv->mjpeg压缩函数
	//yuv_buf:输入的YUV数据
	//frame_rate:设置帧速
	//mjpeg_width：设置输出帧宽度
	//mjpeg_height：设置输出帧高度
	//mjpeg_buf：事先分配的输出缓冲
	//mjpeg_buf_size：实现分配的输出缓冲的配置长度
	//返回：返回mjpeg数据的实际长度
	unsigned int mjpeg_compress(void *yuv_buf, int frame_rate, int mjpeg_width, int mjpeg_height, void *mjpeg_buf, int *mjpeg_buf_size);

	//从驱动中获得捕获成功的图片缓冲区物理地址，并置入内核线程队列
	void capture(void);
	void _flushq();

	int m_frame_rate;
	int n_buffer;
	struct buffer buffers[MAX_BUFFERS];
	struct pollfd	m_events_c2;
	struct ADDRS addr[MAX_BUFFERS];
    ms_thread_t     threadid; 		//
 //   ms_mutex_t      mutex;  		//
    queue_t         captureQ;       /**<! add capture video data to this message queue. */
};

#endif

