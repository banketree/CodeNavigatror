//add by xuqd
extern "C"{
#ifdef __cplusplus
	#define __STDC_CONSTANT_MACROS
	#ifdef _STDINT_H
	#undef _STDINT_H
	#endif
	#include <stdint.h>
#endif

#include "libavcodec/avcodec.h"
#include <msvideo.h>
}
//add end

#include "CFimc.h"
#include "jpeglib.h"
#include "IDBTCfg.h"

int fimc_v4l2_querycap(int fp)
{
    struct v4l2_capability cap;
    int ret = 0;

    ret = ioctl(fp, VIDIOC_QUERYCAP, &cap);

    if (ret < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):VIDIOC_QUERYCAP failed\n", __func__);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("&&&&&& CFIMC ERR(%s):no capture devices\n", __func__);
        return -1;
    }

    return ret;
}

const __u8* fimc_v4l2_enuminput(int fp, int index)
{
    static struct v4l2_input input;

    input.index = index;
    if (ioctl(fp, VIDIOC_ENUMINPUT, &input) != 0) {
        LOGE("&&&&&& CFIMC ERR(%s):No matching index found\n", __func__);
        return NULL;
    }
    LOGD("CFIMC Name of input channel[%d] is %s\n", input.index, input.name);

    return input.name;
}

int fimc_v4l2_s_input(int fp, int index)
{
    struct v4l2_input input;
    int ret;

    input.index = index;

    ret = ioctl(fp, VIDIOC_S_INPUT, &input);
    if (ret < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):VIDIOC_S_INPUT failed\n", __func__);
        return ret;
    }

    return ret;
}

int fimc_v4l2_enum_fmt(int fp, unsigned int fmt)
{
    struct v4l2_fmtdesc fmtdesc;
    int found = 0;

    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;

    while (ioctl(fp, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        if (fmtdesc.pixelformat == fmt) {
            LOGD("&&&&&& CFIMC passed fmt = %#x found pixel format[%d]: %s\n", fmt, fmtdesc.index, fmtdesc.description);
            found = 1;
            break;
        }

        fmtdesc.index++;
    }

    if (!found) {
        LOGE("&&&&&& CFIMC unsupported pixel format\n");
        return -1;
    }

    return 0;
}

int fimc_v4l2_s_fmt(int fp, int width, int height, unsigned int fmt, int flag_capture)
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

    pixfmt.sizeimage = (width * height * 12) / 8;

    if (flag_capture == 1)
        pixfmt.field = V4L2_FIELD_NONE;

    v4l2_fmt.fmt.pix = pixfmt;

    /* Set up for capture */
    if (ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt) < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):VIDIOC_S_FMT failed\n", __func__);
        return -1;
    }

    return 0;
}

int fimc_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs)
{
    struct v4l2_requestbuffers req;
    int ret;

    req.count = nr_bufs;
    req.type = type;
    req.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fp, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):VIDIOC_REQBUFS failed\n", __func__);
        return -1;
    }

    return req.count;
}

int fimc_v4l2_qbuf(int fp, int index)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;

    ret = ioctl(fp, VIDIOC_QBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):VIDIOC_QBUF failed\n", __func__);
        return ret;
    }

    return 0;
}
int fimc_v4l2_streamon(int fp)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(fp, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):VIDIOC_STREAMON failed\n", __func__);
        return ret;
    }

    return ret;
}

int fimc_v4l2_streamoff(int fp)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(fp, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):VIDIOC_STREAMOFF failed, %s\n", __func__, strerror);
        return ret;
    }

    return ret;
}
int CFIMC::fimc_poll(struct pollfd *events)
{
    int ret;

    /* 10 second delay is because sensor can take a long time
     * to do auto focus and capture in dark settings
     */
    ret = poll(events, 1, 10000);
    if (ret < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):poll error\n", __func__);
        return ret;
    }

    if (ret == 0) {
    	LOGE("&&&&&& CFIMC ERR(%s):No data in 10 secs..\n", __func__);
        return ret;
    }

    return ret;
}

int fimc_v4l2_dqbuf(int fp)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fp, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):VIDIOC_DQBUF failed, dropped frame\n", __func__);
        return ret;
    }

    return v4l2_buf.index;
}

 int fimc_v4l2_s_ctrl(int fp, unsigned int id, unsigned int value)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;
    ctrl.value = value;

    ret = ioctl(fp, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("&&&&&& CFIMC ERR(%s):VIDIOC_S_CTRL(id = %#x (%d), value = %d) failed ret = %d\n",
             __func__, id, id-V4L2_CID_PRIVATE_BASE, value, ret);

        return ret;
    }

    return ctrl.value;
}

 int fime_v4l2_set_rotation(int fd,unsigned int value)
 {
	 if(fd< 0 )
		 return -1;
	return fimc_v4l2_s_ctrl(fd,V4L2_CID_ROTATION,value);
 }
 int CFIMC::setFrameRate(int frame_rate)
 {
	 if(m_cam_fd < 0 )
		 return -1;

	 if (frame_rate < 0 || 10 < frame_rate )
		 LOGE("&&&&&& CFIMC ERR(%s):Invalid frame_rate(%d)", __func__, frame_rate);
 
	 if (m_frame_rate != (unsigned)frame_rate) {
		 m_frame_rate =(unsigned)frame_rate;

		 if (m_flag_stat) {
			 if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FRAME_RATE, frame_rate) < 0) {
				 LOGE("&&&&&& CFIMC ERR(%s):Fail on V4L2_CID_CAMERA_FRAME_RATE", __func__);
				 return -1;
			 }
		 }
	 }
	 return 0;
 }
int CFIMC::getRecordFrame(void)
{
	int ret,index;
	if(m_cam_fd < 0 || m_flag_stat==0)
		return -1;

    if( fimc_poll(&m_events_c2)<=0 )
    {
		return -1;
	}
	index =	fimc_v4l2_dqbuf(m_cam_fd);
    if(index<0 || index >n_buffer)
    {
		LOGE("CFIMC ERR(%s):Fail on getRecordFrame", __func__);
		return -1;
    }
	addr[index].addr_y = getRecPhyAddrY(index);
	addr[index].addr_cbcr = getRecPhyAddrC(index);
	addr[index].buf_idx = index;

	return index;
}

int CFIMC::getRecordFrame(void **pRecordFrame, int *FrameLen)
{
	int index;
	int len;
	int i;

	if(m_cam_fd<0)
	{
		return -1;
	}

   if( fimc_poll(&m_events_c2)<=0 )
    {
		return -1;
	}

	index = fimc_v4l2_dqbuf(m_cam_fd);
	if(index<0||index>MAX_BUFFERS)
	{
		printf("index=%d, error:index<0||index>n_buffer \n", index);
		return -1;
	}

//	printf("getRecordFrame index = %d\n", index);

//	printf("getRecordFrame 1\n");
//	printf("buffers[index].length = %d\n", buffers[index].length);
	*FrameLen = buffers[index].length;

	*pRecordFrame = buffers[index].start;

//	memcpy(pRecordFrame, buffers[index].start, len);
//	printf("getRecordFrame 2\n");

	addr[index].addr_y = getRecPhyAddrY(index);
	addr[index].addr_cbcr = getRecPhyAddrC(index);
	addr[index].buf_idx = index;

//	if(releaseRecordFrame(index) < 0)
//	{
//		printf("release index failed!\n");
//	}
	return index;
}

mblk_t *CFIMC::getqueue()
{
	return getq(&captureQ);
}

int CFIMC::releaseRecordFrame(int index)
{
	if(m_cam_fd < 0 || m_flag_stat==0)
	{
		LOGE("&&&&&& error:%s,m_cam_fd=%d, m_flag_stat=%d\n", __func__,m_cam_fd,m_flag_stat);
		printf("&&&&&& error:%s,m_cam_fd=%d, m_flag_stat=%d\n", __func__,m_cam_fd,m_flag_stat);
		return -1;
	}
	addr[index].addr_cbcr=0;
	addr[index].addr_y=0;
	addr[index].buf_idx=-1;

	return fimc_v4l2_qbuf(m_cam_fd, index);
}

unsigned int CFIMC::getRecPhyAddrY(int index)
{
    unsigned int addr_y;

    addr_y = fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_PADDR_Y, index);
//    CHECK((int)addr_y);
    return addr_y;
}

unsigned int CFIMC::getRecPhyAddrC(int index)
{
    unsigned int addr_c;

    addr_c = fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_PADDR_CBCR, index);
 //   CHECK((int)addr_c);
    return addr_c;
}

unsigned int CFIMC::getAddrY(int index)
{
	if( addr[index].buf_idx!=index)
		return 0;
    return addr[index].addr_y;
}

unsigned int CFIMC::getAddrC(int index)
{
	if( addr[index].buf_idx!=index)
		return 0;
    return addr[index].addr_cbcr;
}

CFIMC::CFIMC()
{

	//initFormat = 1; //1:cif 2:d1
	auto_mode_width = 704;
	auto_mode_height = 576;

	m_cam_fd = -1;
	m_flag_stat=0;
	m_frame_rate=0;
	
	qinit(&captureQ);
//    ms_mutex_init(&s->mutex,NULL);

	LOGD("CFIMC CFIMC()");
}
CFIMC::~CFIMC()
{
	if(m_cam_fd>0)
	{
		close(m_cam_fd);
	}
	LOGD("CFIMC ~CFIMC()");
}
void CFIMC::_flushq()
{
	mblk_t *mp;
	while ((mp=getq(&captureQ))!=NULL)
	{
	
		int index=*( (int *)mp->b_rptr);
		releaseRecordFrame(index);
		freemsg(mp);
	}
}
int CFIMC::init_pic(int camera_id)
{
	LOGD("CFIMC init_pic open fd begin");

	if(m_cam_fd < 0)
	{
		m_cam_fd = open(CAMERA_DEV_NAME2, O_RDWR|O_NONBLOCK);
		if (m_cam_fd < 0) {
			LOGE("CFIMC ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME2, strerror(errno));
			goto initCamera_done;
		}
		LOGD("CFIMC init_pic open fd :%d", m_cam_fd);
	}
	n_buffer = 8;
	if (fimc_v4l2_querycap(m_cam_fd) < 0) {
		 LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		 goto initCamera_done;
	 }
	 if (!fimc_v4l2_enuminput(m_cam_fd, camera_id)) {
		 LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		 goto initCamera_done;
	 }
	 if (fimc_v4l2_s_input(m_cam_fd, camera_id)) {
		 LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		 goto initCamera_done;
	 }
	if(fimc_v4l2_enum_fmt(m_cam_fd,V4L2_PIX_FMT_YUYV)<0) {
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}
	if(fimc_v4l2_s_fmt(m_cam_fd, 352, 288,V4L2_PIX_FMT_YUYV,0)<0) {
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}
	if(fimc_v4l2_reqbufs(m_cam_fd,V4L2_BUF_TYPE_VIDEO_CAPTURE,n_buffer)<0){
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}
	memset(buffers,0,sizeof(buffers));
    /* start with all buffers in queue */
    for (int i = 0; i < n_buffer; i++)
    {
    	struct v4l2_buffer buf;
    	memset(&buf,0,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (ioctl (m_cam_fd, VIDIOC_QUERYBUF, &buf)<0) //映射用户空间
			printf ("VIDIOC_QUERYBUF error,%s\n",strerror(errno));

		buffers[i].length = buf.length;
		buffers[i].start=mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,m_cam_fd,buf.m.offset);
		if(NULL == buffers[i].start)
				LOGE ("CFIMC mmap failed, %s\n",strerror(errno));
		else
				LOGD ("CFIMC mmap success,index=%d, length=%d addr=0x%X\n",i,buf.length, buffers[i].start);
		if(fimc_v4l2_qbuf(m_cam_fd, i)<0){
				LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
				goto initCamera_done;
		}
    }

	return m_cam_fd;
initCamera_done:
	if(m_cam_fd>0)
	{
		close(m_cam_fd);
		LOGD("CFIMC init_pic close fd end");
	}

	m_cam_fd=-1;
	return m_cam_fd;

}


void CFIMC::uninit_video()
{
	if(m_cam_fd > 0)
	{
		LOGD("CFIMC will close m_cam_fd:%d", m_cam_fd);
	    for (int i = 0; i < n_buffer; i++)
	    {
	    	munmap(buffers[i].start, buffers[i].length);
	    }

		LOGD("CFIMC will close m_cam_fd:%d", m_cam_fd);
		close(m_cam_fd);
		m_cam_fd = -1;
	}
}

int CFIMC::change_format(int width, int height)
{
	if(m_cam_fd <= 0)
	{
		LOGE("CFIMC %s::%d fail. errno:m_cam_fd <= 0\n");
		return -1;
	}

	LOGD("CFIMC will close m_cam_fd:%d", m_cam_fd);
	close(m_cam_fd);


	//352, 288
    if(fimc_v4l2_s_fmt(m_cam_fd, width, height, V4L2_PIX_FMT_NV12T,0)<0) {
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
	}
}


int CFIMC::init_video(int camera_id, int width, int height){
	//return -1;
	LOGD("CFIMC init_video open fd begin");
	m_cam_fd = open(CAMERA_DEV_NAME, O_RDWR|O_NONBLOCK);
	LOGD("CFIMC init_video open fd :%d", m_cam_fd);
	n_buffer = MAX_BUFFERS;
	if (m_cam_fd < 0) {
		LOGE("CFIMC ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME, strerror(errno));
		goto initCamera_done;
	}
	if (fimc_v4l2_querycap(m_cam_fd) < 0) {
		 LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		 goto initCamera_done;
	 }
	 if (!fimc_v4l2_enuminput(m_cam_fd, camera_id)) {
		 LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		 goto initCamera_done;
	 }
	 if (fimc_v4l2_s_input(m_cam_fd, camera_id)) {
		 LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		 goto initCamera_done;
	 }

	if(fimc_v4l2_enum_fmt(m_cam_fd,V4L2_PIX_FMT_NV12)<0) {
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

    if(fimc_v4l2_s_fmt(m_cam_fd, width, height, V4L2_PIX_FMT_NV12T,0)<0) {
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if(fimc_v4l2_reqbufs(m_cam_fd,V4L2_BUF_TYPE_VIDEO_CAPTURE,MAX_BUFFERS)<0){
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}
    /* start with all buffers in queue */
    for (int i = 0; i < n_buffer; i++) {
        if(fimc_v4l2_qbuf(m_cam_fd, i)<0){
			LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
			goto initCamera_done;
		}
    }
	return m_cam_fd;
initCamera_done:
	if(m_cam_fd>0)
	{
		close(m_cam_fd);
		LOGD("CFIMC init_video close fd end");
	}

	m_cam_fd=-1;
	return m_cam_fd;
}

int CFIMC::init_mjpeg_video(int camera_id, unsigned int width, unsigned int height)
{
	int index = 0;

	image_height = height;
	image_width = width;

	if(camera_id == 0)
	{
		m_cam_fd = open(CAMERA_DEV_NAME, O_RDWR|O_NONBLOCK);
		if (m_cam_fd < 0)
		{
			LOGE("&&&&&& CFIMC ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME, strerror(errno));
			goto initCamera_done;
		}
	}
	else if(camera_id == 2)
	{
		m_cam_fd = open(CAMERA_DEV_NAME2, O_RDWR|O_NONBLOCK);
		if (m_cam_fd < 0)
		{
			LOGE("&&&&&& CFIMC ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME2, strerror(errno));
			goto initCamera_done;
		}
	}
	else
	{
		m_cam_fd = -1;
		goto initCamera_done;
	}

	if (fimc_v4l2_querycap(m_cam_fd) < 0)
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if (!fimc_v4l2_enuminput(m_cam_fd, 0))
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if (fimc_v4l2_s_input(m_cam_fd, 0))
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if(fimc_v4l2_enum_fmt(m_cam_fd,V4L2_PIX_FMT_YUYV)<0)
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if(fimc_v4l2_s_fmt(m_cam_fd, image_width, image_height, V4L2_PIX_FMT_YUYV,0)<0)
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if(fimc_v4l2_reqbufs(m_cam_fd,V4L2_BUF_TYPE_VIDEO_CAPTURE,MAX_BUFFERS)<0)
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
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

		if (ioctl (m_cam_fd, VIDIOC_QUERYBUF, &buf)<0) //映射用户空间
			LOGE("&&&&&& VIDIOC_QUERYBUF error,%s\n",strerror(errno));

		buffers[i].length = buf.length;
		buffers[i].start=mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,m_cam_fd,buf.m.offset);
		if(NULL == buffers[i].start)
				LOGE ("&&&&&& CFIMC mmap failed, %s\n",strerror(errno));
		else
				LOGD ("&&&&&& CFIMC mmap success,index=%d, length=%d addr=0x%X\n",i,buf.length, buffers[i].start);
		if(fimc_v4l2_qbuf(m_cam_fd, i)<0){
				LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
				goto initCamera_done;
		}
	}

	memset(&m_events_c2, 0, sizeof(m_events_c2));
	m_events_c2.fd = m_cam_fd;
	m_events_c2.events = POLLIN | POLLERR;
	if(fimc_poll(&m_events_c2)<0)
	{
		LOGE("&&&&&& test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	setFrameRate(1);
	m_flag_stat = 1;
#if 0
	if( fime_v4l2_set_rotation(m_cam_fd,180) < 0)
	{
		LOGE("test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}
#endif
	if(fimc_v4l2_streamon(m_cam_fd)<0){
		LOGE("&&&&&& test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if( fimc_poll(&m_events_c2)<=0 )
	{
		goto initCamera_done;
	}

	//去掉前面20张照片
	for(int i=0; i<20; i++)
	{
		if( fimc_poll(&m_events_c2)<=0 )
		{
			LOGE("&&&&&& fimc_poll error!\n");
			return -1;
		}

		index =	fimc_v4l2_dqbuf(m_cam_fd);
		if(index<0 || index >8)
		{
			LOGE("&&&&&& test ERR(%s):Fail on getRecordFrame", __func__);
			return -1;
		}
		LOGD("&&&&&& fimc_v4l2_dqbuf:index=%d\n",index);

		releaseRecordFrame(index);
	}

	return m_cam_fd;

initCamera_done:
	if(m_cam_fd>0)
	{
		close(m_cam_fd);
		LOGD("&&&&&& CFIMC init_video close fd end");
	}

	m_cam_fd=-1;
	return m_cam_fd;
}

int CFIMC::init_mpeg4_video(int camera_id, unsigned int width, unsigned int height)
{
	int index = 0;

	image_height = height;
	image_width = width;

	if(camera_id == 0)
	{
		m_cam_fd = open(CAMERA_DEV_NAME, O_RDWR|O_NONBLOCK);
		if (m_cam_fd < 0)
		{
			LOGE("&&&&&& CFIMC ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME, strerror(errno));
			goto initCamera_done;
		}
	}
	else if(camera_id == 2)
	{
		m_cam_fd = open(CAMERA_DEV_NAME2, O_RDWR|O_NONBLOCK);
		if (m_cam_fd < 0)
		{
			LOGE("&&&&&& CFIMC ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME2, strerror(errno));
			goto initCamera_done;
		}
	}
	else
	{
		m_cam_fd = -1;
		goto initCamera_done;
	}

	if (fimc_v4l2_querycap(m_cam_fd) < 0)
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if (!fimc_v4l2_enuminput(m_cam_fd, 0))
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if (fimc_v4l2_s_input(m_cam_fd, 0))
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if(fimc_v4l2_enum_fmt(m_cam_fd,V4L2_PIX_FMT_NV12T)<0)
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if(fimc_v4l2_s_fmt(m_cam_fd, image_width, image_height, V4L2_PIX_FMT_NV12T,0)<0)
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if(fimc_v4l2_reqbufs(m_cam_fd,V4L2_BUF_TYPE_VIDEO_CAPTURE,MAX_BUFFERS)<0)
	{
		LOGE("&&&&&& CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
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

		if (ioctl (m_cam_fd, VIDIOC_QUERYBUF, &buf)<0) //映射用户空间
			LOGE("&&&&&& VIDIOC_QUERYBUF error,%s\n",strerror(errno));

		buffers[i].length = buf.length;
//		printf("************buffers[%d].length = %d*******************\n", i, buf.length);
		buffers[i].start=mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,m_cam_fd,buf.m.offset);
		if(NULL == buffers[i].start)
				LOGE ("&&&&&& CFIMC mmap failed, %s\n",strerror(errno));
		else
				LOGD ("&&&&&& CFIMC mmap success,index=%d, length=%d addr=0x%X\n",i,buf.length, buffers[i].start);
		if(fimc_v4l2_qbuf(m_cam_fd, i)<0){
				LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
				goto initCamera_done;
		}
	}

	memset(&m_events_c2, 0, sizeof(m_events_c2));
	m_events_c2.fd = m_cam_fd;
	m_events_c2.events = POLLIN | POLLERR;
	if(fimc_poll(&m_events_c2)<0)
	{
		LOGE("&&&&&& test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	setFrameRate(1);
	m_flag_stat = 1;
#if 0
	if( fime_v4l2_set_rotation(m_cam_fd,180) < 0)
	{
		LOGE("test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}
#endif
	if(fimc_v4l2_streamon(m_cam_fd)<0){
		LOGE("&&&&&& test %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		goto initCamera_done;
	}

	if( fimc_poll(&m_events_c2)<=0 )
	{
		goto initCamera_done;
	}

	//去掉前面20张照片
	for(int i=0; i<20; i++)
	{
		if( fimc_poll(&m_events_c2)<=0 )
		{
			LOGE("&&&&&& fimc_poll error!\n");
			return -1;
		}

		index =	fimc_v4l2_dqbuf(m_cam_fd);
		if(index<0 || index >8)
		{
			LOGE("&&&&&& test ERR(%s):Fail on getRecordFrame", __func__);
			return -1;
		}
//		printf("&&&&&& fimc_v4l2_dqbuf:index=%d\n",index);

		releaseRecordFrame(index);
	}

	return m_cam_fd;

initCamera_done:
	if(m_cam_fd>0)
	{
		close(m_cam_fd);
		LOGD("&&&&&& CFIMC init_video close fd end");
	}

	m_cam_fd=-1;
	return m_cam_fd;
}

void* CFIMC::capture_thread(void *data)
{
	CFIMC *fimc = (CFIMC *)data;
	while(fimc->m_flag_stat)
	{
		fimc->capture();
	}
}
unsigned char *CFIMC::jpg_compress(void *yuv_buf,int *jpgsize)
{
#if 0
	exif_attribute_t * ptrExifInfo = NULL;
	JpegEncoder jpgEnc;
	if (!jpgEnc.create())
	{
		LOGE("CFIMC JpegEncoder.create() Error\n");
		return NULL;
	}

	int inFormat = JPG_MODESEL_YCBCR;
	int outFormat = JPG_422;


	if (jpgEnc.setConfig(JPEG_SET_ENCODE_IN_FORMAT, inFormat) != JPG_SUCCESS)
		LOGE("CFIMC [JPEG_SET_ENCODE_IN_FORMAT] Error\n");

	if (jpgEnc.setConfig(JPEG_SET_SAMPING_MODE, outFormat) != JPG_SUCCESS)
		LOGE("CFIMC [JPEG_SET_SAMPING_MODE] Error\n");
	image_quality_type_t jpegQuality;
	jpegQuality = JPG_QUALITY_LEVEL_2;

	if (jpgEnc.setConfig(JPEG_SET_ENCODE_QUALITY, jpegQuality) != JPG_SUCCESS)
		LOGE("CFIMC [JPEG_SET_ENCODE_QUALITY] Error\n");
	if (jpgEnc.setConfig(JPEG_SET_ENCODE_WIDTH, 352) != JPG_SUCCESS)
		LOGE("CFIMC [JPEG_SET_ENCODE_WIDTH] Error\n");

	if (jpgEnc.setConfig(JPEG_SET_ENCODE_HEIGHT, 288) != JPG_SUCCESS)
		LOGE("CFIMC [JPEG_SET_ENCODE_HEIGHT] Error\n");

//	int thumbWidth = 352, thumbHeight=288, thumbSrcSize = 352*288*2;
//
//	if (   jpgEnc.setConfig(JPEG_SET_THUMBNAIL_WIDTH, thumbWidth) != JPG_SUCCESS
//		|| jpgEnc.setConfig(JPEG_SET_THUMBNAIL_HEIGHT, thumbHeight) != JPG_SUCCESS) {
//		LOGE("CFIMC JPEG_SET_THUMBNAIL_WIDTH or  JPEG_SET_THUMBNAIL_HEIGHT Error\n");
//		ptrExifInfo = NULL;
//	}

	// In this function, Exif doesn't required..
	// This is very important.
	// Please, rewrite about jpeg encoding scheme with SecCameraHWInterface.cpp
	ptrExifInfo = NULL;

	unsigned int snapshot_size = 352 * 288 * 2;
	unsigned char *pInBuf = (unsigned char *)jpgEnc.getInBuf(snapshot_size);

	if (pInBuf == NULL) {
		LOGE("CFIMC JPEG input buffer is NULL!!\n");
		return NULL;
	}
	memcpy(pInBuf, yuv_buf, snapshot_size);
	uint64_t outbuf_size;
	unsigned int output_size =0;
	jpgEnc.encode(&output_size, ptrExifInfo);

	unsigned char *pOutBuf = (unsigned char *)jpgEnc.getOutBuf(&outbuf_size);

	if (pOutBuf == NULL) {
		LOGE("CFIMC JPEG output buffer is NULL!!\n");
		return NULL;
	}
//	printf("outbuf_size=%u\n", outbuf_size);
	//fwrite(pOutBuf, outbuf_size, 1, fd);
	unsigned char *jpeg_buf = (unsigned char *)malloc(outbuf_size);
	if(jpeg_buf == NULL) return NULL;
	memcpy(jpeg_buf, pOutBuf, outbuf_size);
	*jpgsize = outbuf_size;
	jpgEnc.destroy();

	return jpeg_buf;
#endif
	return NULL;
}

unsigned int CFIMC::mjpeg_compress(void *yuv_buf, int frame_rate, int mjpeg_width, int mjpeg_height, void *mjpeg_buf, int *mjpeg_buf_size)
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

unsigned int CFIMC::libjpeg_compress(unsigned char *image, int width, int height, int quality, unsigned char **jpeg_buf)
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

	unsigned long jpeg_out_len = 0;
	jpeg_mem_dest(&cinfo, jpeg_buf, &jpeg_out_len);
	jpeg_start_compress(&cinfo, TRUE);

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

	return (unsigned int)jpeg_out_len;
}

unsigned char *CFIMC::getjpg(int *len)
{
	int ret,index;
    struct v4l2_buffer v4l2_buf;
    unsigned char *jpgbuf = NULL;
	if(m_cam_fd<0)
		return NULL;

	setFrameRate(1);
#if 0
	if( fime_v4l2_set_rotation(m_cam_fd,180) < 0)
	{
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		return NULL;
	}
#endif

	LOGE("CFIMC iRotation: %d\n", g_stIdbtCfg.iRotation);
	if(g_stIdbtCfg.iRotation)
	{
		if( fime_v4l2_set_rotation(m_cam_fd,180) < 0)
		{
			LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
			return NULL;
		}
	}
	else
	{
		if( fime_v4l2_set_rotation(m_cam_fd,0) < 0)
		{
			LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
			return NULL;
		}
	}

	if(fimc_v4l2_streamon(m_cam_fd)<0){
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		return NULL;
	}

	memset(&m_events_c2, 0, sizeof(m_events_c2));
	m_events_c2.fd = m_cam_fd;
	m_events_c2.events = POLLIN | POLLERR;
	//因为图片效果太差，丢掉前８０张照片
#if 1
	for(int i=0;i<10;i++)
	{
		if( fimc_poll(&m_events_c2)<0) {
			LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
			continue;
		}
		index = fimc_v4l2_dqbuf(m_cam_fd);
		LOGD("CFIMC drop index = %d\n",index);
		releaseRecordFrame(index);
	}
#endif
	if( fimc_poll(&m_events_c2)<0) {
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		return NULL;
	}
	else
	{
		index = fimc_v4l2_dqbuf(m_cam_fd);
		LOGD("CFIMC index = %d\n",index);
		if(index<0||index>n_buffer)
			return NULL;

		//		FILE *fpJpeg = fopen("/data/data/testjpeg.yuv", "w");
		//		if(fpJpeg == NULL)
		//		{
		//			LOGE("open jpeg yuv file failed!\n");
		//		}
		//		fwrite(buffers[index].start, buffers[index].length, 1, fpJpeg);

		jpgbuf= jpg_compress(buffers[index].start,len);

		FILE *fd = fopen("test.yuv","w");
		if(fd > 0)
		{
			LOGD("CFIMC index = %d addr=0x%X FILE=0x%X",index, buffers[index].start, fd);
			fwrite((char *)(buffers[index].start), 352 * 288 * 2, 1, fd);
			fclose(fd);
		}
		else
		{
			LOGD("CFIMC fd=0x%X", fd);
		}

		releaseRecordFrame(index);
	}
	if(fimc_v4l2_streamoff(m_cam_fd)<0)
	{
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
	}
	return jpgbuf;
}

unsigned int CFIMC::getLibjpeg(unsigned char *libjpeg_buf)
{
	int index;

	index =	fimc_v4l2_dqbuf(m_cam_fd);
	if(index<0 || index >8)
	{
		LOGE("&&&&&& test ERR(%s):Fail on getRecordFrame", __func__);
		return 0;
	}
#if 0
	int yuv422len = buffers[index].length;
	int pixlen = image_width*image_height;
	int uvlen = pixlen / 2;
	unsigned char *yuv422 = (unsigned char *)malloc(yuv422len);
	if(yuv422 == NULL)
	{
		LOGE("&&&&&& %s, errno:%s\n", __func__, strerror(errno));
		return 0;
	}
	unsigned char *u422 = yuv422 + pixlen;
	unsigned char *v422 = yuv422 + pixlen + pixlen / 2;
	for(int i=0,j=0,k=0; i<yuv422len/4; i++,j++,k++)
	{
		*(yuv422 + j * 2) = *((unsigned char *)buffers[index].start + i * 4);
		*(yuv422 + j * 2 + 1) = *((unsigned char *)buffers[index].start + i * 4 + 2);
		*(u422 + k) = *((unsigned char *)buffers[index].start + i * 4 + 1);
		*(v422 + k) = *((unsigned char *)buffers[index].start + i * 4 + 3);
	}
#endif
#if 0
	FILE *fp = fopen("/data/data/yuv422p.yuv", "w");
	if(fp == NULL)
	{
		LOGE("open file failed!\n");
	}
	fwrite(yuv422, yuv422len, 1, fp);
	fclose(fp);
#endif
	unsigned char *pLibJpegBuf = NULL;
	unsigned int LibJpegLen = 0;
//	LibJpegLen = libjpeg_compress(yuv422, image_width, image_height, 60, &pLibJpegBuf);
//	LibJpegLen = libjpeg_compress(buffers[index].start, image_width, image_height, 60, &pLibJpegBuf);
	LibJpegLen = libjpeg_compress((unsigned char *)buffers[index].start, image_width, image_height, 60, &libjpeg_buf);
//	if(LibJpegLen != 0)
//	{
//		memcpy(libjpeg_buf, pLibJpegBuf, LibJpegLen);
//	}

//	if(NULL != pLibJpegBuf)
//	{
//		free(pLibJpegBuf);
//	}

	releaseRecordFrame(index);

	return LibJpegLen;
}

int CFIMC::uninit_mjpeg_video(void)
{
	if(fimc_v4l2_streamoff(m_cam_fd)<0)
	{
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	return 0;
}

int CFIMC::uninit_mpeg4_video(void)
{
	if(fimc_v4l2_streamoff(m_cam_fd)<0)
	{
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	return 0;
}

void CFIMC::capture()
{
	int ret,index;
	if(m_cam_fd < 0 || m_flag_stat==0)
		return ;

    if( fimc_poll(&m_events_c2)<=0 )
    {
		return ;
	}
	index =	fimc_v4l2_dqbuf(m_cam_fd);
    if(index<0 || index >n_buffer)
    {
		LOGE("CFIMC ERR(%s):Fail on getRecordFrame", __func__);
		return ;
    }
	addr[index].addr_y = getRecPhyAddrY(index);
	addr[index].addr_cbcr = getRecPhyAddrC(index);
	addr[index].buf_idx = index;

	mblk_t *yuv_block=allocb(sizeof(int),0);
	if(yuv_block)
	{
		memcpy(yuv_block->b_wptr,&index,sizeof(int));
		yuv_block->b_wptr+=sizeof(int);
		putq(&captureQ,yuv_block);

	}
	else
	{
		releaseRecordFrame(index);
	}
}

int CFIMC::start(){

	if(m_cam_fd<0)
		return -1;

	setFrameRate(1);
	LOGE("CFIMC iRotation: %d\n", g_stIdbtCfg.iRotation);
	if(g_stIdbtCfg.iRotation)
	{
		if( fime_v4l2_set_rotation(m_cam_fd,180) < 0)
		{
			LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
			return -1;
		}
	}
	else
	{
		if( fime_v4l2_set_rotation(m_cam_fd,0) < 0)
		{
			LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
			return -1;
		}
	}
	if(fimc_v4l2_streamon(m_cam_fd)<0){
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}	

	memset(&m_events_c2, 0, sizeof(m_events_c2));
	m_events_c2.fd = m_cam_fd;
	m_events_c2.events = POLLIN | POLLERR;
	if( fimc_poll(&m_events_c2)<0) {
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	m_flag_stat=1;
	for(int i=0;i<n_buffer;i++){
		addr[i].addr_cbcr=0;
		addr[i].addr_y=0;
		addr[i].buf_idx=-1;
	}
	ms_thread_create(&threadid,NULL,CFIMC::capture_thread,this);
}
	
int CFIMC::stop(){
	if(m_cam_fd<0)
		return -1;
	m_flag_stat=0;
	ms_thread_join(threadid,NULL);

	if(fimc_v4l2_streamoff(m_cam_fd)<0)
	{
		LOGE("CFIMC %s::%d fail. errno: %s\n", __func__, __LINE__, strerror(errno));
	}

}

bool CFIMC::getstatus(){
	return m_flag_stat;
}


CFIMC g_cam_pic;
int capture_pic()
{
	LOGD("CFIMC capture_pic begin");
	int jpgsize = 0;
	unsigned char *jpg_buf = NULL;

	FILE *fd = fopen("/data/data/com.fsti.ladder/capture.jpeg","w");
	if(fd == NULL)
	{
		LOGE("CFIMC open jpeg file faild");
		return -1;
	}

	LOGD("CFIMC init camera");
	if( g_cam_pic.init_pic(0) < 0)
	{
		LOGE("CFIMC init camera for picture faild");
		return -1;
	}
	else
	{
		LOGD("CFIMC init camera for picture ok");
	}
	LOGD("CFIMC getjpg");
	g_cam_pic.m_flag_stat = 1;
	jpg_buf = g_cam_pic.getjpg(&jpgsize);
	if(jpg_buf == NULL)
	{
		LOGE("CFIMC get picture faild");
		return -1;
	}
	else
	{
		LOGD("CFIMC get picture ok");
	}

	//to-do:.............
	fwrite(jpg_buf, jpgsize, 1, fd);
	free(jpg_buf);
	fclose(fd);

	LOGD("CFIMC capture_pic end:%d close %d", jpgsize, g_cam_pic.m_cam_fd);

	return 0;


}


#if 0  //Get PIC demo
int main(int argc,char **argv)
{
	int jpgsize = 0;
	FILE *fd = fopen("test.jpg","w");
	unsigned char *jpg_buf = NULL;
	CFIMC cam;
	cam.m_flag_stat = 1;
	if( cam.init_pic(0) < 0)
	{
		LOGE("CFIMC init camera for picture faild");
		return -1;
	}
	else
	{
		LOGD("CFIMC init camera for picture ok");
	}
	jpg_buf = cam.getjpg(&jpgsize);
	if(jpg_buf == NULL)
	{
		LOGE("CFIMC get picture faild");
		return -1;
	}
	else
	{
		LOGD("CFIMC get picture ok");
	}
	//to-do:.............
	fwrite(jpg_buf, jpgsize, 1, fd);
	free(jpg_buf);
}
#endif
