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
	��ѯ�ӿ��Ƿ�߱�����ͼƬ����
	return:0 �ɹ���-1ʧ��
*/
int fimc_v4l2_querycap(int fp);
/*
	��ѯ�ӿ�ָ���ģ�������ͷ�Ƿ����������
	return:0 �ɹ���-1ʧ��
*/

const __u8* fimc_v4l2_enuminput(int fp, int index);

/*
	ָ���򿪵�����ͷid
	eturn:0 �ɹ���-1ʧ��
*/
int fimc_v4l2_s_input(int fp, int index);

/*
	��ѯ�ӿ��Ƿ�֧��ָ���ĸ�ʽ���������yuv420,rgb565 �ȡ�
	eturn:0 �ɹ���-1ʧ��
*/
int fimc_v4l2_enum_fmt(int fp, unsigned int fmt);

/*
	ָ���򿪵����ͼƬ��ʽ
	eturn:0 �ɹ���-1ʧ��
*/
int fimc_v4l2_s_fmt(int fp, int width, int height, unsigned int fmt, int flag_capture);

/*
	�����ں�ͼƬ���񻺳�����
	eturn:0 �ɹ���-1ʧ��
*/
int fimc_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs);

/*
	���������Ž��������С�
	eturn:0 �ɹ���-1ʧ��
*/
int fimc_v4l2_qbuf(int fp, int index);

/*
	��ʼ����
	eturn:0 �ɹ���-1ʧ��
*/

int fimc_v4l2_streamon(int fp);

/*
	v4l2�������÷�װ����
	eturn:0 �ɹ���-1ʧ��
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
	//��ʼ����Ƶ������ù���
	int init_video(int camera_id,int width, int height);
	void uninit_video();
	////��ʼ����Ƶ����ʽ�Լ�׼����Ӧ�Ŀռ�
	int init_mjpeg_video(int camera_id, unsigned int width, unsigned int height);
	int uninit_mjpeg_video(void);
	int init_mpeg4_video(int camera_id, unsigned int width, unsigned int height);
	int uninit_mpeg4_video(void);
	//��ʼ��ͼƬ��ץ����
	int init_pic(int camera_id);
	int change_format(int width, int height);

	//���jpg��ʽͼƬ���ݻ��壬��������ʹ���߸���free
	unsigned char *getjpg(int *len);

	unsigned int getLibjpeg(unsigned char *libjpeg_buf);

	//��ʼ��Ƶ����
	int start();
	//ֹͣ��Ƶ����
	int stop();
	//��ö����е�ͼƬ��ַ
	mblk_t *getqueue();
	bool getstatus();

	//io��ѯ�ӿ�
	int fimc_poll(struct pollfd *events);
	//�ͷŴ�����ϵ�ͼƬ�����������·��벶�����
	int releaseRecordFrame(int index);

	//���Y���������������ַ
	unsigned int getAddrY(int index);
	//���cbcr���������������ַ
	unsigned int getAddrC(int index);
	//�������л�ò���ɹ���ͼƬ������ʹ�к�
	int getRecordFrame(void);

	//�������л�ȡһ֡��Ƶ����
	int getRecordFrame(void **pRecordFram, int *FrameLen);

	//����֡�ʣ��ں�δʵ�ָù��ܣ����Ըú�������Ч��
	int setFrameRate(int frame_rate);
	//�����̴߳���������̬����
	static void* capture_thread(void *arg);
	int m_flag_stat;

//private:
	//���Y���������������ַ
	unsigned int getRecPhyAddrY(int index);
	//���cbcr���������������ַ
	unsigned int getRecPhyAddrC(int index);

private:
	//yuv��->jpgѹ��������
	unsigned char *jpg_compress(void *yuv_buf,int *jpgsize);
	unsigned int libjpeg_compress(unsigned char *image, int width, int height, int quality, unsigned char **jpeg_buf);
	//yuv->mjpegѹ������
	//yuv_buf:�����YUV����
	//frame_rate:����֡��
	//mjpeg_width���������֡���
	//mjpeg_height���������֡�߶�
	//mjpeg_buf�����ȷ�����������
	//mjpeg_buf_size��ʵ�ַ���������������ó���
	//���أ�����mjpeg���ݵ�ʵ�ʳ���
	unsigned int mjpeg_compress(void *yuv_buf, int frame_rate, int mjpeg_width, int mjpeg_height, void *mjpeg_buf, int *mjpeg_buf_size);

	//�������л�ò���ɹ���ͼƬ�����������ַ���������ں��̶߳���
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

