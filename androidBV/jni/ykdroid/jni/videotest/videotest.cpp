extern "C"{
#ifdef __cplusplus
	#define __STDC_CONSTANT_MACROS
	#ifdef _STDINT_H
	#undef _STDINT_H
	#endif
	#include <stdint.h>
#endif

#include "libavcodec/avcodec.h"
}

#include <math.h>

#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <surfaceflinger/Surface.h>  
#include <binder/IMemory.h> 
#include <android/log.h>

#include <surfaceflinger/ISurface.h>
//#include <ui/GraphicBuffer.h>
#include <camera/ICamera.h>
#include <camera/ICameraClient.h>
#include <camera/ICameraService.h>
//#include <ui/Overlay.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/KeyedVector.h>
#include <utils/Log.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#include <binder/IPCThreadState.h>
#include <media/stagefright/AudioSource.h>
#include <media/stagefright/CameraSource.h>
#include <media/stagefright/MPEG2TSWriter.h>
#include <media/stagefright/MPEG4Writer.h>
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/MediaProfiles.h>
#include <utils/Errors.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
//#include <CameraPreviewSource.h>

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

#include <videodev2.h>
#include <videodev2_samsung.h>
#include <Jpeg.h>
#include "jpeglib.h"


#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "CFIMC", __VA_ARGS__)


#define _CHECK_OP(x,y,suffix,op)                                         \
    do {                                                                \
		if(x!=y)	\
			printf("check fail in FUNCTION=%s line=%d\n",__FUNCTION__,__LINE__);                                           \
    } while (false)

#define _CHECK(x)                                         \
			do {																\
				if(!x)	\
					printf("check fail in FUNCTION=%s line=%d\n",__FUNCTION__,__LINE__);										   \
			} while (false)

#define _CHECK_EQ(x,y)   _CHECK_OP(x,y,EQ,==)


using namespace android;
int m_cam_fd = -1;


static sp<Camera> mCamera=NULL;


static void* bind_loop(void* arg)
{
	ProcessState::self()->startThreadPool();
}
#define ALIGN_TO_16B(x)   ((((x) + (1 <<  4) - 1) >>  4) <<  4)
#define ALIGN_TO_32B(x)   ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_64B(x)   ((((x) + (1 <<  6) - 1) >>  6) <<  6)
#define ALIGN_TO_128B(x)  ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_2KB(x)   ((((x) + (1 << 11) - 1) >> 11) << 11)
#define ALIGN_TO_4KB(x)   ((((x) + (1 << 12) - 1) >> 12) << 12)
#define ALIGN_TO_8KB(x)   ((((x) + (1 << 13) - 1) >> 13) << 13)
#define ALIGN_TO_64KB(x)  ((((x) + (1 << 16) - 1) >> 16) << 16)
#define ALIGN_TO_128KB(x) ((((x) + (1 << 17) - 1) >> 17) << 17)

#define CAMERA_DEV_NAME   "/dev/video0"
#define CAMERA_DEV_NAME2  "/dev/video2"

#define MAX_BUFFERS     8


static int fimc_v4l2_querycap(int fp)
{
    struct v4l2_capability cap;
    int ret = 0;

    ret = ioctl(fp, VIDIOC_QUERYCAP, &cap);

    if (ret < 0) {
        printf("ERR(%s):VIDIOC_QUERYCAP failed\n", __func__);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        printf("ERR(%s):no capture devices\n", __func__);
        return -1;
    }

    return ret;
}

static const __u8* fimc_v4l2_enuminput(int fp, int index)
{
    static struct v4l2_input input;

    input.index = index;
    if (ioctl(fp, VIDIOC_ENUMINPUT, &input) != 0) {
        printf("ERR(%s):No matching index found\n", __func__);
        return NULL;
    }
    printf("Name of input channel[%d] is %s\n", input.index, input.name);

    return input.name;
}

static int fimc_v4l2_s_input(int fp, int index)
{
    struct v4l2_input input;
    int ret;

    input.index = index;

    ret = ioctl(fp, VIDIOC_S_INPUT, &input);
    if (ret < 0) {
        printf("ERR(%s):VIDIOC_S_INPUT failed\n", __func__);
        return ret;
    }

    return ret;
}
static int fimc_v4l2_enum_fmt(int fp, unsigned int fmt)
{
    struct v4l2_fmtdesc fmtdesc;
    int found = 0;

    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;

    while (ioctl(fp, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        if (fmtdesc.pixelformat == fmt) {
            printf("passed fmt = %#x found pixel format[%d]: %s\n", fmt, fmtdesc.index, fmtdesc.description);
            found = 1;
            break;
        }

        fmtdesc.index++;
    }

    if (!found) {
        printf("unsupported pixel format\n");
        return -1;
    }

    return 0;
}

static int get_pixel_depth(unsigned int fmt)
{
    int depth = 0;

    switch (fmt) {
    case V4L2_PIX_FMT_NV12:
        depth = 12;
        break;
    case V4L2_PIX_FMT_NV12T:
        depth = 12;
        break;
    case V4L2_PIX_FMT_NV21:
        depth = 12;
        break;
    case V4L2_PIX_FMT_YUV420:
        depth = 12;
        break;

    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_YUV422P:
        depth = 16;
        break;

    case V4L2_PIX_FMT_RGB32:
        depth = 32;
        break;
    }

    return depth;
}

static int fimc_v4l2_s_fmt(int fp, int width, int height, unsigned int fmt, int flag_capture)
{
    struct v4l2_format v4l2_fmt;
    struct v4l2_pix_format pixfmt;

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    memset(&pixfmt, 0, sizeof(pixfmt));

    pixfmt.width = width;
    pixfmt.height = height;
    pixfmt.pixelformat = fmt;
    if (fmt == V4L2_PIX_FMT_JPEG) {
        pixfmt.colorspace = V4L2_COLORSPACE_JPEG;
    }
    printf("fmt is 0x%X, pixel_depth=%d\n",fmt,get_pixel_depth(fmt));
    pixfmt.sizeimage = (width * height * get_pixel_depth(fmt)) / 8;
    printf("pixfmt.sizeimage is %d\n",pixfmt.sizeimage);
    if (flag_capture == 1)
        pixfmt.field = V4L2_FIELD_NONE;

    v4l2_fmt.fmt.pix = pixfmt;

    /* Set up for capture */
    if (ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt) < 0) {
        printf("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
        return -1;
    }

    return 0;
}

static int fimc_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs)
{
    struct v4l2_requestbuffers req;
    int ret;

    req.count = nr_bufs;
    req.type = type;
    req.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fp, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        printf("ERR(%s):VIDIOC_REQBUFS failed\n", __func__);
        return -1;
    }

    return req.count;
}

static int fimc_v4l2_qbuf(int fp, int index)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;

    ret = ioctl(fp, VIDIOC_QBUF, &v4l2_buf);
    if (ret < 0) {
        printf("ERR(%s):VIDIOC_QBUF failed\n", __func__);
        return ret;
    }

    return 0;
}
static int fimc_v4l2_streamon(int fp)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(fp, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        printf("ERR(%s):VIDIOC_STREAMON failed\n", __func__);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_streamoff(int fp)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(fp, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        printf("ERR(%s):VIDIOC_STREAMOFF failed\n", __func__);
        return ret;
    }

    return ret;
}
static int fimc_poll(struct pollfd *events)
{
    int ret;

    /* 10 second delay is because sensor can take a long time
     * to do auto focus and capture in dark settings
     */
    ret = poll(events, 1, 10000);
    if (ret < 0) {
        printf("ERR(%s):poll error\n", __func__);
        return ret;
    }

    if (ret == 0) {
        printf("ERR(%s):No data in 10 secs..\n", __func__);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_dqbuf(int fp)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fp, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) {
        printf("ERR(%s):VIDIOC_DQBUF failed, dropped frame\n", __func__);
        return ret;
    }

    return v4l2_buf.index;
}
static int fimc_v4l2_s_ctrl(int fp, unsigned int id, unsigned int value)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;
    ctrl.value = value;

    ret = ioctl(fp, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        printf("ERR(%s):VIDIOC_S_CTRL(id = %#x (%d), value = %d) failed ret = %d\n",
             __func__, id, id-V4L2_CID_PRIVATE_BASE, value, ret);

        return ret;
    }

    return ctrl.value;
}

static int fime_v4l2_set_rotation(int fd,unsigned int value)
{
	 if(fd< 0 )
		 return -1;
	return fimc_v4l2_s_ctrl(fd,V4L2_CID_ROTATION,value);
}

struct pollfd	m_events_c2;

int setFrameRate(int frame_rate)
{
	 if (frame_rate < 0 || 10 < frame_rate ){
		 printf("CFIMC ERR(%s):Invalid frame_rate(%d)", __func__, frame_rate);
	 }
	 else {
		 if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FRAME_RATE, frame_rate) < 0) {
			 printf("CFIMC ERR(%s):Fail on V4L2_CID_CAMERA_FRAME_RATE", __func__);
			 return -1;
		 }
	 }
	 return 0;
}

static int getRecordFrame(void)
{

    fimc_poll(&m_events_c2);
    return fimc_v4l2_dqbuf(m_cam_fd);
}

int releaseRecordFrame(int index)
{

    return fimc_v4l2_qbuf(m_cam_fd, index);
}

unsigned int getRecPhyAddrY(int index)
{
    unsigned int addr_y;

    addr_y = fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_PADDR_Y, index);
    CHECK((int)addr_y);
    return addr_y;
}

unsigned int getRecPhyAddrC(int index)
{
    unsigned int addr_c;

    addr_c = fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_PADDR_CBCR, index);
    CHECK((int)addr_c);
    return addr_c;
}
void getThumbnailConfig(int *width, int *height, int *size)
{
    *width  = 352;
    *height = 288;
    *size   = (*width) * (*height) * 2;
}
struct buffer {
        void *                  start;
        size_t                  length;
};
struct buffer buffers[MAX_BUFFERS];
#if 0
int getSnapshotAndJpeg()
{
    int index;
	static int n = 1;
	char filename[20];
	
	memset(filename,0,sizeof(filename));
	sprintf(filename,"test%d.jpg",n);
    FILE *fd = fopen(filename,"w");
	n++;

    fimc_poll(&m_events_c2);
    index = fimc_v4l2_dqbuf(m_cam_fd);
	printf("index = %d\n",index);
    //fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_STREAM_PAUSE, 0);

    //memcpy(yuv_buf, buffers[index].start, 352 * 288 * 2);

    //fimc_v4l2_streamoff(m_cam_fd);

    /* JPEG encoding */
   exif_attribute_t * ptrExifInfo = NULL;

    JpegEncoder jpgEnc;
    if (!jpgEnc.create())
    {
        printf("JpegEncoder.create() Error\n");
        return -1;
    }

    int inFormat = JPG_MODESEL_YCBCR;
    int outFormat = JPG_422;


    if (jpgEnc.setConfig(JPEG_SET_ENCODE_IN_FORMAT, inFormat) != JPG_SUCCESS)
        printf("[JPEG_SET_ENCODE_IN_FORMAT] Error\n");

    if (jpgEnc.setConfig(JPEG_SET_SAMPING_MODE, outFormat) != JPG_SUCCESS)
        printf("[JPEG_SET_SAMPING_MODE] Error\n");

    image_quality_type_t jpegQuality;
    jpegQuality = JPG_QUALITY_LEVEL_2;

    if (jpgEnc.setConfig(JPEG_SET_ENCODE_QUALITY, jpegQuality) != JPG_SUCCESS)
        printf("[JPEG_SET_ENCODE_QUALITY] Error\n");
    if (jpgEnc.setConfig(JPEG_SET_ENCODE_WIDTH, 352) != JPG_SUCCESS)
        printf("[JPEG_SET_ENCODE_WIDTH] Error\n");

    if (jpgEnc.setConfig(JPEG_SET_ENCODE_HEIGHT, 288) != JPG_SUCCESS)
        printf("[JPEG_SET_ENCODE_HEIGHT] Error\n");

    int thumbWidth, thumbHeight, thumbSrcSize;
    getThumbnailConfig(&thumbWidth, &thumbHeight, &thumbSrcSize);

    if (   jpgEnc.setConfig(JPEG_SET_THUMBNAIL_WIDTH, thumbWidth) != JPG_SUCCESS
        || jpgEnc.setConfig(JPEG_SET_THUMBNAIL_HEIGHT, thumbHeight) != JPG_SUCCESS) {
        printf("JPEG_SET_THUMBNAIL_WIDTH or  JPEG_SET_THUMBNAIL_HEIGHT Error\n");
        ptrExifInfo = NULL;
    }

    // In this function, Exif doesn't required..
    // This is very important.
    // Please, rewrite about jpeg encoding scheme with SecCameraHWInterface.cpp
    ptrExifInfo = NULL;

    unsigned int snapshot_size = 352 * 288 * 2;
    unsigned char *pInBuf = (unsigned char *)jpgEnc.getInBuf(snapshot_size);

    if (pInBuf == NULL) {
        printf("JPEG input buffer is NULL!!\n");
        return -1;
    }
    memcpy(pInBuf, buffers[index].start, snapshot_size);
    uint64_t outbuf_size;
    unsigned int output_size =0;

    //setExifChangedAttribute();
    jpgEnc.encode(&output_size, ptrExifInfo);

    unsigned char *pOutBuf = (unsigned char *)jpgEnc.getOutBuf(&outbuf_size);

    if (pOutBuf == NULL) {
        printf("JPEG output buffer is NULL!!\n");
        return -1;
    }
    fwrite(pOutBuf, outbuf_size, 1, fd); 

    //memcpy(jpeg_buf, pOutBuf, outbuf_size);

    jpgEnc.destroy();

    return 0;
}
#endif
#if 0
int main(int argc,char **argv)
{
	int ret;	
	int index;
    	unsigned int phyYAddr;
    	unsigned int phyCAddr;
	system("rm test*.jpg");

	m_cam_fd = open(CAMERA_DEV_NAME2, O_RDWR|O_NONBLOCK);
	if (m_cam_fd < 0) {
		printf("ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME, strerror(errno));
		goto initCamera_done;
	}
	
	if (fimc_v4l2_querycap(m_cam_fd) < 0) {
		 printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		 goto initCamera_done;
	 }
	 if (!fimc_v4l2_enuminput(m_cam_fd, 0)) {
		 printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		 goto initCamera_done;
	 }
	 if (fimc_v4l2_s_input(m_cam_fd, 0)) {
		 printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		 goto initCamera_done;
	 }

	if(fimc_v4l2_enum_fmt(m_cam_fd,V4L2_PIX_FMT_YUYV)<0) {
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

    	if(fimc_v4l2_s_fmt(m_cam_fd, 352,288,V4L2_PIX_FMT_YUYV, 0)<0) {
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if(fimc_v4l2_reqbufs(m_cam_fd,V4L2_BUF_TYPE_VIDEO_CAPTURE,MAX_BUFFERS)<0){
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}	

    	/* start with all buffers in queue */
	memset(buffers,0,sizeof(buffers));
	for (int i = 0; i < MAX_BUFFERS; i++) {
	struct v4l2_buffer buf;
	memset(&buf,0,sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = i;

	if (ioctl (m_cam_fd, VIDIOC_QUERYBUF, &buf)<0)
		printf ("VIDIOC_QUERYBUF error,%s\n",strerror(errno));

	buffers[i].length = buf.length;
	buffers[i].start=mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,m_cam_fd,buf.m.offset);
	if(NULL == buffers[i].start)
			printf ("mmap failed, %s\n",strerror(errno));
	else
			printf ("mmap success,index=%d, length=%d\n",i,buf.length);
		if(fimc_v4l2_qbuf(m_cam_fd, i)<0){
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
		}
	}

	if(fimc_v4l2_streamon(m_cam_fd)<0){
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}	

	
    memset(&m_events_c2, 0, sizeof(m_events_c2));
    m_events_c2.fd = m_cam_fd;
    m_events_c2.events = POLLIN | POLLERR;
#if 1
    if( fimc_poll(&m_events_c2)<0) {
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}
#endif
	printf("success start record\n");
	for(int i=0;i<5;i++)
	{
		printf("before record i=%d\n",i);
		getSnapshotAndJpeg();
		releaseRecordFrame(index);
		
		continue;
		index=getRecordFrame();
		
		printf("after record index=%d\n",index);
		if (index < 0) {
			printf("ERR(%s):Fail on SecCamera->getPreview()\n", __func__);
			continue;
		}
		printf("index=%d,length=%d\n",index,buffers[index].length);

		return 0;
		//phyYAddr = getRecPhyAddrY(index);
		//phyCAddr = getRecPhyAddrC(index);
		//printf("index = %d Yaddr = %0x  Caddr = %0x\n", index,phyYAddr, phyCAddr);
	}
		
initCamera_done:
	if(m_cam_fd>0)
		close(m_cam_fd);
	return ret;
}
#endif

//static AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height)
//{
//    AVFrame *picture;
//    uint8_t *picture_buf;
//    int size;
//
//    picture = avcodec_alloc_frame();
//    if (!picture)
//        return NULL;
//    size = avpicture_get_size(pix_fmt, width, height);
//    picture_buf = av_malloc(size);
//    if (!picture_buf) {
//        av_free(picture);
//        return NULL;
//    }
//    avpicture_fill((AVPicture *)picture, picture_buf,
//                   pix_fmt, width, height);
//    return picture;
//}

unsigned char *jpg_compress(void *yuv_buf,unsigned int *jpgsize,int height, int width)
{
	exif_attribute_t * ptrExifInfo = NULL;
	JpegEncoder jpgEnc;
	if (!jpgEnc.create())
	{
		printf("CFIMC JpegEncoder.create() Error\n");
		return NULL;
	}

	int inFormat = JPG_MODESEL_YCBCR;
	int outFormat = JPG_422;


	if (jpgEnc.setConfig(JPEG_SET_ENCODE_IN_FORMAT, inFormat) != JPG_SUCCESS)
		printf("CFIMC [JPEG_SET_ENCODE_IN_FORMAT] Error\n");

	if (jpgEnc.setConfig(JPEG_SET_SAMPING_MODE, outFormat) != JPG_SUCCESS)
		printf("CFIMC [JPEG_SET_SAMPING_MODE] Error\n");
	image_quality_type_t jpegQuality;
	jpegQuality = JPG_QUALITY_LEVEL_2;

	if (jpgEnc.setConfig(JPEG_SET_ENCODE_QUALITY, jpegQuality) != JPG_SUCCESS)
		printf("CFIMC [JPEG_SET_ENCODE_QUALITY] Error\n");
	if (jpgEnc.setConfig(JPEG_SET_ENCODE_WIDTH, height) != JPG_SUCCESS)
		printf("CFIMC [JPEG_SET_ENCODE_WIDTH] Error\n");

	if (jpgEnc.setConfig(JPEG_SET_ENCODE_HEIGHT, width) != JPG_SUCCESS)
		printf("CFIMC [JPEG_SET_ENCODE_HEIGHT] Error\n");

//	int thumbWidth = 352, thumbHeight=288, thumbSrcSize = 352*288*2;
//
//	if (   jpgEnc.setConfig(JPEG_SET_THUMBNAIL_WIDTH, thumbWidth) != JPG_SUCCESS
//		|| jpgEnc.setConfig(JPEG_SET_THUMBNAIL_HEIGHT, thumbHeight) != JPG_SUCCESS) {
//		printf("CFIMC JPEG_SET_THUMBNAIL_WIDTH or  JPEG_SET_THUMBNAIL_HEIGHT Error\n");
//		ptrExifInfo = NULL;
//	}

	// In this function, Exif doesn't required..
	// This is very important.
	// Please, rewrite about jpeg encoding scheme with SecCameraHWInterface.cpp
	ptrExifInfo = NULL;

	unsigned int snapshot_size = height * width * 2;
	unsigned char *pInBuf = (unsigned char *)jpgEnc.getInBuf(snapshot_size);

	if (pInBuf == NULL) {
		printf("CFIMC JPEG input buffer is NULL!!\n");
		return NULL;
	}
	memcpy(pInBuf, yuv_buf, snapshot_size);
	uint64_t outbuf_size = 0;
	unsigned int output_size =0;
	printf("begin to encode with jpeg!\n");
	if(jpgEnc.encode(&output_size, ptrExifInfo) == JPG_SUCCESS)
	{
		printf("JPG_SUCCESS!\n");
	}
	else
	{
		printf("JPG_ERROR!\n");
	}

	unsigned char *pOutBuf = (unsigned char *)jpgEnc.getOutBuf(&outbuf_size);
	if (pOutBuf == NULL) {
		printf("CFIMC JPEG output buffer is NULL!!\n");
		return NULL;
	}

	//fwrite(pOutBuf, outbuf_size, 1, fd);
	unsigned int jpg_output_len = (unsigned int)outbuf_size;
	*jpgsize = jpg_output_len;
	printf("jpg_output_len=%u\n",jpg_output_len);
	unsigned char *jpg_out_buf = (unsigned char *)malloc(jpg_output_len);
	if(jpg_out_buf == NULL)
	{
		return NULL;
	}

	memcpy(jpg_out_buf, pOutBuf, jpg_output_len);

	jpgEnc.destroy();

	return jpg_out_buf;
}

unsigned int mjpeg_compress(void *yuv_buf, int frame_rate, int mjpeg_width, int mjpeg_height, void *mjpeg_buf, int *mjpeg_buf_size)
{
    AVCodec *codec;
    AVCodecContext *contex= NULL;
    AVFrame *picture;
    unsigned int picturesize,mjpeg_size;

    printf("Mjpeg Video encoding for 8130\n");

    /* must be called before using avcodec lib */
    avcodec_init();

	/* register all the codecs */
	avcodec_register_all();

	printf("begin to find encoder!\n");

    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(CODEC_ID_MJPEG);
    if (!codec) {
    	printf("codec not found\n");
        exit(1);
    }

    printf("alloc frame!\n");
    contex= avcodec_alloc_context();
    picture= avcodec_alloc_frame();

    /* put sample parameters */
    contex->bit_rate = 400000;
    /* resolution must be a multiple of two */
    contex->width = mjpeg_width;
    contex->height = mjpeg_height;
    /* frames per second */
    contex->time_base= (AVRational){1,25};
    contex->gop_size = 0; /* emit one intra frame every ten frames */
    contex->max_b_frames= 0;
    contex->pix_fmt = PIX_FMT_YUV422P;
    contex->strict_std_compliance = FF_COMPLIANCE_UNOFFICIAL;

    printf("open codec!\n");
    /* open it */
    if (avcodec_open(contex, codec) < 0) {
    	printf("could not open codec\n");
        exit(1);
    }

//    picture = alloc_picture(contex->pix_fmt, contex->width, contex->height);

    picturesize = (contex->width * contex->height); /* size for YUV 422P */
    picture->data[0] = (uint8_t *)yuv_buf;
    picture->data[1] = picture->data[0] + picturesize;
    picture->data[2] = picture->data[1] + picturesize / 2;
    picture->linesize[0] = contex->width;
    picture->linesize[1] = contex->width / 2;
    picture->linesize[2] = contex->width / 2;

    printf("begin eccodec!\n");
    mjpeg_size = avcodec_encode_video(contex, (unsigned char *)mjpeg_buf, *mjpeg_buf_size, picture);

    avcodec_close(contex);
    av_free(contex);
    av_free(picture);
    printf("mjpeg compress end\n");

    return mjpeg_size;
}
#if 0
void jpeg_compress(void *yuv_buf, void *jpeg_buf, unsigned long jpeg_len)
{

	 /* This struct contains the JPEG compression parameters and pointers to
	   * working space (which is allocated as needed by the JPEG library).
	   * It is possible to have several such structures, representing multiple
	   * compression/decompression processes, in existence at once.  We refer
	   * to any one struct (and its associated working data) as a "JPEG object".
	   */
	  struct jpeg_compress_struct cinfo;
	  /* This struct represents a JPEG error handler.  It is declared separately
	   * because applications often want to supply a specialized error handler
	   * (see the second half of this file for an example).  But here we just
	   * take the easy way out and use the standard error handler, which will
	   * print a message on stderr and call exit() if compression fails.
	   * Note that this struct must live as long as the main JPEG parameter
	   * struct, to avoid dangling-pointer problems.
	   */
	  struct jpeg_error_mgr jerr;
	  /* More stuff */
	  FILE * outfile;		/* target file */
	  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	  JSAMPLE * image_buffer = (JSAMPLE *)yuv_buf;
	  unsigned char *jpeg_ptr = (unsigned char *)jpeg_buf;

	  int row_stride;		/* physical row width in image buffer */

	  /* Step 1: allocate and initialize JPEG compression object */

	  /* We have to set up the error handler first, in case the initialization
	   * step fails.  (Unlikely, but it could happen if you are out of memory.)
	   * This routine fills in the contents of struct jerr, and returns jerr's
	   * address which we place into the link field in cinfo.
	   */
	  cinfo.err = jpeg_std_error(&jerr);
	  /* Now we can initialize the JPEG compression object. */
	  jpeg_create_compress(&cinfo);

	  /* Step 2: specify data destination (eg, a file) */
	  /* Note: steps 2 and 3 can be done in either order. */

	  /* Here we use the library-supplied code to send compressed data to a
	   * stdio stream.  You can also write your own code to do something else.
	   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	   * requires it in order to write binary files.
	   */
//	  if ((outfile = fopen(filename, "wb")) == NULL) {
//	    fprintf(stderr, "can't open %s\n", filename);
//	    exit(1);
//	  }
//	  jpeg_stdio_dest(&cinfo, outfile);
	  jpeg_mem_dest(&cinfo, &jpeg_ptr, &jpeg_len);

	  /* Step 3: set parameters for compression */

	  /* First we supply a description of the input image.
	   * Four fields of the cinfo struct must be filled in:
	   */
	  cinfo.image_width = 352; 	/* image width and height, in pixels */
	  cinfo.image_height = 288;
	  cinfo.input_components = 3;		/* # of color components per pixel */
	  cinfo.in_color_space = JCS_YCbCr; 	/* colorspace of input image */
	  /* Now use the library's routine to set default compression parameters.
	   * (You must set at least cinfo.in_color_space before calling this,
	   * since the defaults depend on the source color space.)
	   */
	  jpeg_set_defaults(&cinfo);
	  /* Now you can set any non-default parameters you wish to.
	   * Here we just illustrate the use of quality (quantization table) scaling:
	   */
	  jpeg_set_quality(&cinfo, 20, TRUE /* limit to baseline-JPEG values */);

	  /* Step 4: Start compressor */

	  /* TRUE ensures that we will write a complete interchange-JPEG file.
	   * Pass TRUE unless you are very sure of what you're doing.
	   */
	  jpeg_start_compress(&cinfo, TRUE);

	  /* Step 5: while (scan lines remain to be written) */
	  /*           jpeg_write_scanlines(...); */

	  /* Here we use the library's state variable cinfo.next_scanline as the
	   * loop counter, so that we don't have to keep track ourselves.
	   * To keep things simple, we pass one scanline per call; you can pass
	   * more if you wish, though.
	   */
//	  row_stride = image_width * 3;	/* JSAMPLEs per row in image_buffer */
	  row_stride = 288 * 3;

	  while (cinfo.next_scanline < cinfo.image_height) {
	    /* jpeg_write_scanlines expects an array of pointers to scanlines.
	     * Here the array is only one element long, but you could pass
	     * more than one scanline at a time if that's more convenient.
	     */
	    row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
	    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	  }

	  /* Step 6: Finish compression */

	  jpeg_finish_compress(&cinfo);
	  /* After finish_compress, we can close the output file. */
	  fclose(outfile);

	  /* Step 7: release JPEG compression object */

	  /* This is an important step since it will release a good deal of memory. */
	  jpeg_destroy_compress(&cinfo);
	  pInBuf
	  /* And we're done! */

}
#endif

//static int jpeg_compress(char *filename, unsigned char *image, int width, int height, int quality)
static unsigned int jpeg_compress(unsigned char *image, int width, int height, int quality, unsigned char **jpeg_buf)
{
	int i, j;

	JSAMPROW y[16],cb[16],cr[16]; // y[2][5] = color sample of row 2 and pixel column 5; (one plane)
	JSAMPARRAY data[3]; // t[0][2][5] = color sample 0 of row 2 and column 5

	FILE *fp;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	data[0] = y;
	data[1] = cb;
	data[2] = cr;

	cinfo.err = jpeg_std_error(&jerr);  // Errors get written to stderr

	jpeg_create_compress(&cinfo);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);

	jpeg_set_colorspace(&cinfo, JCS_YCbCr);

	cinfo.raw_data_in = TRUE; // Supply downsampled data
#if JPEG_LIB_VERSION >= 70
#warning using JPEG_LIB_VERSION >= 70
	cinfo.do_fancy_downsampling = FALSE;  // Fix segfault with v7
#endif
	cinfo.comp_info[0].h_samp_factor = 2;
	cinfo.comp_info[0].v_samp_factor = 2;
	cinfo.comp_info[1].h_samp_factor = 1;
	cinfo.comp_info[1].v_samp_factor = 1;
	cinfo.comp_info[2].h_samp_factor = 1;
	cinfo.comp_info[2].v_samp_factor = 1;

	jpeg_set_quality(&cinfo, quality, TRUE);
	cinfo.dct_method = JDCT_FASTEST;

//	if ((fp = fopen(filename, "wb")) == NULL)
//	{
//		fprintf(stderr, "can't open %s\n", filename);
//		exit(1);
//	}
//
//	jpeg_stdio_dest(&cinfo, fp);        // Data written to file
	unsigned long jpeg_out_len = 0;
//	unsigned char *jpeg_out_buf;

	jpeg_mem_dest(&cinfo, jpeg_buf, &jpeg_out_len);

	jpeg_start_compress(&cinfo, TRUE);
#if 0
	//420P
	for (j = 0; j < height; j += 16) {
		for (i = 0; i < 16; i++) {
			y[i] = image + width * (i + j);
			if (i % 2 == 0) {
				cb[i / 2] = image + width * height + width / 2 * ((i + j) / 2);
				cr[i / 2] = image + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
			}
		}
		jpeg_write_raw_data(&cinfo, data, 16);
	}
#endif
	//422P
	for (j = 0; j < height; j += 16) {
		for (i = 0; i < 16; i++) {
			y[i] = image + width * (i + j);
			cb[i] = image + width * height + width / 2 * (i + j);
			cr[i] = image + width * height + width * height / 2 + width / 2 * (i + j);
		}
		jpeg_write_raw_data(&cinfo, data, 16);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

//	if(jpeg_buf == NULL)
//	{
//		jpeg_buf = (unsigned char *)malloc(jpeg_out_len);
//		memset(jpeg_buf, 0, jpeg_out_len);
//	}

//	printf("jpeg_out_len=%u\n", (unsigned int)jpeg_out_len);
//	memcpy(jpeg_buf, jpeg_out_buf, jpeg_out_len);
//	if(jpeg_out_buf != NULL)
//	{
//		free(jpeg_out_buf);
//	}

//	FILE *fJpegout = fopen("/data/data/test_jpegout.jpeg", "w");
//	if(fJpegout == NULL)
//	{
//		printf("error:%s\n", __func__);
//	}
//	fwrite(*jpeg_buf, jpeg_out_len, 1, fJpegout);
//	fclose(fJpegout);
	return (unsigned int)jpeg_out_len;
}

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

//  jmp_buf setjmp_buffer;	/* for return to caller */
};
typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
//  longjmp(myerr->setjmp_buffer, 1);
}

int jpeg_decompress(char * infilename, char * outfilename)
{
	char *rgbbuf = NULL;
	int rgblen = 0;
	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct my_error_mgr jerr;
	/* More stuff */
	FILE * infile;		/* source file */
	FILE * outfile;
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */

	/* In this example we want to open the input file before doing anything else,
	* so that the setjmp() error recovery below can assume the file is open.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to read binary files.
	*/

	if ((infile = fopen(infilename, "rb")) == NULL) {
	fprintf(stderr, "can't open %s\n", infilename);
	return 0;
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
//	if (setjmp(jerr.setjmp_buffer)) {
//	/* If we get here, the JPEG code has signaled an error.
//	 * We need to clean up the JPEG object, close the input file, and return.
//	 */
//	jpeg_destroy_decompress(&cinfo);
//	fclose(infile);
//	return 0;
//	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	*   (a) suspension is not possible with the stdio data source, and
	*   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	* See libjpeg.txt for more info.
	*/

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	* jpeg_read_header(), so we do nothing here.
	*/

	/* Step 5: Start decompressor */

	(void) jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	* In this example, we need to make an output work buffer of the right size.
	*/
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	rgbbuf = (char *)malloc(500000);
	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	printf("cinfo.output_height=%d\n", cinfo.output_height);
	while (cinfo.output_scanline < cinfo.output_height) {
	/* jpeg_read_scanlines expects an array of pointers to scanlines.
	 * Here the array is only one element long, but you could ask for
	 * more than one scanline at a time if that's more convenient.
	 */
	(void) jpeg_read_scanlines(&cinfo, buffer, 1);
	/* Assume put_scanline_someplace wants a pointer and sample count. */
//	put_scanline_someplace(buffer[0], row_stride);
	memcpy(rgbbuf, buffer[0], row_stride);
	rgbbuf += row_stride;
	rgblen += row_stride;
	}

	/* Step 7: Finish decompression */

	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	* Here we postpone it until after no more JPEG errors are possible,
	* so as to simplify the setjmp error logic above.  (Actually, I don't
	* think that jpeg_destroy can do an error exit, but why assume anything...)
	*/
	fclose(infile);

	if((outfile = fopen(outfilename, "w")) == NULL)
	{
	fprintf(stderr, "can't open %s\n", outfilename);
	return 0;
	}

	fwrite(rgbbuf, 1, rgblen, outfile);
	if(outfile != NULL)
	{
		fclose(outfile);
	}

	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/

	/* And we're done! */
	return 1;
}

int main(int argc,char **argv)
{
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;
	struct timeval tv4;
	struct timeval tv5;
	struct timeval tv6;
	struct timezone tz1;
	struct timezone tz2;
	struct timezone tz3;
	struct timezone tz4;
	struct timezone tz5;
	struct timezone tz6;

	int height = atoi(argv[1]);
	int width = atoi(argv[2]);

	m_cam_fd = open(CAMERA_DEV_NAME2, O_RDWR|O_NONBLOCK);
	if (m_cam_fd < 0)
	{
		printf("ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME, strerror(errno));
//		goto initCamera_done;
	}

	if (fimc_v4l2_querycap(m_cam_fd) < 0) {
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
//		goto initCamera_done;
	}
	if (!fimc_v4l2_enuminput(m_cam_fd, 0)) {
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
//		goto initCamera_done;
	}
	if (fimc_v4l2_s_input(m_cam_fd, 0)) {
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
//		goto initCamera_done;
	}

	if(fimc_v4l2_enum_fmt(m_cam_fd,V4L2_PIX_FMT_YUYV)<0) {
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
//		goto initCamera_done;
	}

	if(fimc_v4l2_s_fmt(m_cam_fd, height,width,V4L2_PIX_FMT_YUYV, 0)<0) {
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
//		goto initCamera_done;
	}

	if(fimc_v4l2_reqbufs(m_cam_fd,V4L2_BUF_TYPE_VIDEO_CAPTURE,MAX_BUFFERS)<0){
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
//		goto initCamera_done;
	}

	memset(buffers,0,sizeof(buffers));

	/* start with all buffers in queue */
	for (int i = 0; i < MAX_BUFFERS; i++)
	{
		struct v4l2_buffer buf;
		memset(&buf,0,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (ioctl (m_cam_fd, VIDIOC_QUERYBUF, &buf)<0)
		{
			printf("VIDIOC_QUERYBUF error,%s\n",strerror(errno));
		}
		buffers[i].length = buf.length;
		buffers[i].start=mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,m_cam_fd,buf.m.offset);
		if(NULL == buffers[i].start)
		{
			printf("mmap failed, %s\n",strerror(errno));
		}
		else
		{
			printf("test mmap success,index=%d, length=%d addr=0x%X\n",i,buf.length, buffers[i].start);
		}

		if(fimc_v4l2_qbuf(m_cam_fd, i)<0)
		{
			printf("test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
//			goto initCamera_done;
		}
	}

    memset(&m_events_c2, 0, sizeof(m_events_c2));
    m_events_c2.fd = m_cam_fd;
    m_events_c2.events = POLLIN | POLLERR;
    if( fimc_poll(&m_events_c2)<0) {
		printf("%s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
//		goto initCamera_done;
	}

	printf("success start record\n");

	setFrameRate(1);

	if( fime_v4l2_set_rotation(m_cam_fd,180) < 0)
	{
		printf("test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	//
	if(fimc_v4l2_streamon(m_cam_fd)<0){
		printf("test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}

	if( fimc_poll(&m_events_c2)<=0 )
	{
		return -1;
	}

	int index;
	for(int i=0; i<20; i++)
	{
		if( fimc_poll(&m_events_c2)<=0 )
		{
			printf("fimc_poll error!\n");
			return -1;
		}

		index =	fimc_v4l2_dqbuf(m_cam_fd);
		if(index<0 || index >8)
		{
			printf("test ERR(%s):Fail on getRecordFrame", __func__);
			return -1;
		}
		printf("fimc_v4l2_dqbuf:index=%d\n",index);

		releaseRecordFrame(index);
	}

	if( fimc_poll(&m_events_c2)<=0 )
	{
		printf("fimc_poll error!\n");
		return -1;
	}

	index =	fimc_v4l2_dqbuf(m_cam_fd);
	if(index<0 || index >8)
	{
		printf("test ERR(%s):Fail on getRecordFrame", __func__);
		return -1;
	}
	printf("fimc_v4l2_dqbuf:index=%d\n",index);

	printf("begin to save yuv422!\n");
	FILE *fpYuv = fopen("/data/data/test_yuv422.yuv", "w");
	if(fpYuv == NULL)
	{
		printf("open yuv file failed!\n");
	}
	fwrite(buffers[index].start, buffers[index].length, 1, fpYuv);
	fclose(fpYuv);
	printf("save yuv422 finish!\n");

	printf("begin to convert yuv422 to yuv422p!\n");
	int yuv422len = buffers[index].length;
	printf("yuv422len = %d\n", yuv422len);
	int pixlen = height*width;
	int uvlen = height*width / 2;
	unsigned char *yuv422 = (unsigned char *)malloc(yuv422len);
	if(yuv422 == NULL)
	{
		printf("%s, errno:%s\n", __func__, strerror(errno));
	}
	gettimeofday(&tv1, &tz1);
	unsigned char *u422 = yuv422 + pixlen;
	unsigned char *v422 = yuv422 + pixlen + pixlen / 2;
	for(int i=0,j=0,k=0; i<yuv422len/4; i++,j++,k++)
	{
		*(yuv422 + j * 2) = *((unsigned char *)buffers[index].start + i * 4);
		*(yuv422 + j * 2 + 1) = *((unsigned char *)buffers[index].start + i * 4 + 2);
		*(u422 + k) = *((unsigned char *)buffers[index].start + i * 4 + 1);
		*(v422 + k) = *((unsigned char *)buffers[index].start + i * 4 + 3);
	}
	gettimeofday(&tv2, &tz2);
	printf("convert yuv422 to yuv422p finish!\n");

	printf("begin to save yuv422p!\n");
	FILE *fpYuv422 = fopen("/data/data/test_yuv422p.yuv", "w");
	if(fpYuv == NULL)
	{
		printf("open yuv file failed!\n");
	}
	fwrite(yuv422, yuv422len, 1, fpYuv422);
	fclose(fpYuv);
	printf("save yuv422p finish!\n");

	printf("begin to compress jpeg with hardware method!\n");
	unsigned int jpg_len;
	unsigned char * jpg_buf = NULL;
	gettimeofday(&tv3, &tz3);
	jpg_buf= jpg_compress(buffers[index].start,&jpg_len,height, width);
	gettimeofday(&tv4, &tz4);
	printf("jpeglen = %d,compress jpeg with hardware method finish!\n",jpg_len);

	printf("begin to save jpeg!\n");
	FILE *fpJpg = fopen("/data/data/test_jpeg.jpeg","w");
	if(fpJpg == NULL)
	{
		printf("open jpeg file failed!\n");
	}
	fwrite(jpg_buf, jpg_len, 1, fpJpg);
	fclose(fpJpg);

	if(NULL != jpg_buf)
	{
		printf("free jpg_buf\n");
		free(jpg_buf);
	}
	printf("save jpeg finish!\n");

	//begin to compress jpeg with libjpeg method;
	unsigned int jpeg_len = 0;
	unsigned char *jpeg_buf = NULL;
	gettimeofday(&tv5, &tz5);
	jpeg_len = jpeg_compress(yuv422, height, width, 60, &jpeg_buf);
	gettimeofday(&tv6, &tz6);
	printf("jpeg_len=%u\n",jpeg_len);
	printf("compress jpeg with libjpeg finish!\n");

	printf("begin to save jpeg file which compress with libjpeg\n");
	FILE *fpLibjpeg = fopen("/data/data/test_libjpeg.jpeg", "w");
	if(fpLibjpeg == NULL)
	{
		printf("open libjpeg file failed!\n");
	}
	fwrite(jpeg_buf, jpeg_len, 1, fpLibjpeg);
	fclose(fpLibjpeg);

	printf("save libjpeg file finish\n");

	if(NULL != jpeg_buf)
	{
		printf("free jpeg_buf\n");
		free((void *)jpeg_buf);
	}

	printf("begin to compress mjpeg!\n");
	unsigned int mjpeg_size;
	int mjpeg_buf_len = 300000;
	unsigned char *mjpeg_buf = (unsigned char *)malloc(mjpeg_buf_len);
	if(mjpeg_buf == NULL)
	{
		printf("%s, errno:%s\n", __func__, strerror(errno));
	}
	mjpeg_size = mjpeg_compress(yuv422,1,height,width,mjpeg_buf,&mjpeg_buf_len);
	printf("mjpeg size which compress from yuv is: %d \n", mjpeg_size);
	printf("mjpeg compress finish!\n");

	printf("begin to save mjpeg!\n");
	FILE *fpMJpeg = fopen("/data/data/test.mjpeg","w");
	if(fpMJpeg == NULL)
	{
		printf("open mjpeg file failed!\n");
	}
	fwrite(mjpeg_buf, mjpeg_size, 1, fpMJpeg);
	fclose(fpMJpeg);
	printf("save mjpeg finish!\n");

//	printf("begin to save jpeg file which decompress with libjpeg\n");
//	if(jpeg_decompress("/data/data/test_libjpeg.jpeg", "/data/data/test_rgb.rgb") == 0)
//	{
//		printf("save rgb file which decompress with libjpeg failed!\n");
//	}
//	else
//	{
//		printf("save rgb file which decompress with libjpeg finish!\n");
//	}

	if(fimc_v4l2_streamoff(m_cam_fd)<0)
	{
		printf("test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
	}

	releaseRecordFrame(index);
	free(yuv422);
	free(mjpeg_buf);
	close(m_cam_fd);
	m_cam_fd=-1;
	printf("test succeed!\n");
	printf("change yuv422 to yuv422p, begin time is:time_sec=%d, time_usec=%d\n", tv1.tv_sec, tv1.tv_usec);
	printf("finish time is:time_sec=%d, time_usec=%d\n", tv2.tv_sec, tv2.tv_usec);
	printf("compress jpeg with hardware method,begin time is:time_sec=%d, time_usec=%d\n", tv3.tv_sec, tv3.tv_usec);
	printf("finish time is:time_sec=%d, time_usec=%d\n", tv4.tv_sec, tv4.tv_usec);
	printf("compress jpeg with libjpeg method,begin time is:time_sec=%d, time_usec=%d\n", tv5.tv_sec, tv5.tv_usec);
	printf("finish time is:time_sec=%d, time_usec=%d\n", tv6.tv_sec, tv6.tv_usec);
	return 0;

//initCamera_done:
//	printf("init camera failed!\n");
//	if(m_cam_fd>0)
//	{
//		close(m_cam_fd);
//		printf("test init_video close fd end");
//	}
//
//	printf("over!\n");
//	m_cam_fd=-1;
//
//	return 0;
}

