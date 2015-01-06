/*********************************************************************
*  Copyright (C), 2001 - 2012, 福建邮科通信技术有限公司.
*  File			:
*  Author		:李程达
*  Version 		:
*  Date			:
*  Description	:
*  History		: 更新记录 .
*            	  <修改作者>  <修改时间>  <修改描述>
**********************************************************************/
#ifdef _YK_IMX27_AV_

//头文件包含
//与系统相关
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/videodev.h>
#include <fcntl.h>
#include <unistd.h>
//与工程相关
#include "YKIMX27Video.h"
#include "rfc3984.h"
#include "msfilter.h"
#include "msvideo.h"
#include "msticker.h"
#include "mswebcam.h"
#include "mscommon.h"
#include "vpu_display.h"
#include "vpu_capture.h"
#include "vpu_voip_test.h"
#include "vpu_lib.h"
#include "LOGIntfLayer.h"
//全局变量定义

/*************************************************
*Name	:VIMX27Capturethread
*Input	:
*Output	:
*Desc	:采集线程
**************************************************/
/*
static void* VIMX27Capturethread(void * data)
{
	VIMX27State  *s    	= (VIMX27State *)data;
	int 		  ret 	= 0;


	//sleep(5);
	printf("The thread for video cap begin to run\n");

	ret = IMX27VideoInit();

	if(ret == -1)			//初始化出错，退出线程
	{
		printf("IMX27Video Init error,out the thread\n");
		ms_thread_exit(NULL);
	}

	while(s->bCaptureStart)
	{
		mblk_t *m;

		//m = VIMX27GrapImageMap(s);
		//printf("进入兴联采集过程\n");

		//加入兴联采集过程
		rec_fds.fd 		= s->fdEnc;
		rec_fds.revents = 0;
		rec_fds.events 	= POLLIN_MAIN_BS;

		poll(&rec_fds, 1, 500);

		if (!(rec_fds.revents & POLLIN_MAIN_BS))
			continue;

		ret = ioctl(s->fdEnc, DVR_ENC_QUEUE_GET, &encdata);
		if(ret < 0)
			continue;

		m = esballoc(s->mmapbuf + encdata.bs.offset,encdata.bs.length,0,NULL);
		m->b_wptr+=encdata.bs.length;
		ioctl(s->fdEnc, DVR_ENC_QUEUE_PUT, &encdata);

		//printf("本次采集结果:buf add = 0x%8X,size = %d\n",s->mmapbuf + encdata.bs.offset,encdata.bs.length);
		//printf("本次采集结果:db_base = 0x%8X,size = %d\n",m->b_datap->db_base,m->b_datap->db_lim-m->b_datap->db_base);



		if(m == NULL)
		{
			printf("Frame is NULL,cap error!!!\n");
			continue;
		}
		else if(m)
		{
			mblk_t	*dm = dupmsg(m);
			ms_mutex_lock(&s->mutex);
			putq(&s->captureQ,dm);
			ms_mutex_unlock(&s->mutex);
			//printf("帧数据采集成功\n");
		}
		else
		{

		}
	}
	printf("Out the loop for video cap,stop the video device\n");
	VIMX27DoMunmap(s);
	printf("Stop the thread for video cap,Out\n");
	ms_thread_exit(NULL);
	return NULL;

}*/
/*************************************************
*Name	:VIMX27StartCapture
*Input	:
*Output	:
*Desc	:创建采集线程
**************************************************/
static int VIMX27StartCapture(VIMX27State *s)
{
	int 		ret                = -1;
	int 		codec_num;
    char 		codec_cfg_name[80] = "modules/video/IMX27Codec.cfg";			//enc/dec config file name
	FILE 	   *codec_cfg_fd       = NULL;
	//struct 		codec_config 	   codec_param[MAX_NUM_INSTANCE];

	vpu_versioninfo 		vpu_ver;

	printf("Configuring the Codec from file = %s\n",codec_cfg_name);

	codec_cfg_fd = fopen(codec_cfg_name, "r");

	if (!codec_cfg_fd)
	{
		printf("Codec: open codec config file failed\n");
		return -1;
	}
	else
	{
		printf("Codec: open codec config file successful\n");
	}

	ret = vpu_getcfg(&codec_num, &s->codec_param, codec_cfg_fd);

	printf("**********************************************************\n\n");
	printf("* The Confguration:\n");
	printf("* codec_num:%d\n",codec_num);
	printf("* index        = %d\n", s->codec_param.index);
	printf("* src          = %d\n", s->codec_param.src);
	printf("* src_name     = %s\n", s->codec_param.src_name);
	printf("* dst          = %d\n", s->codec_param.dst);
	printf("* dst_name     = %s\n", s->codec_param.dst_name);
	printf("* enc_flag     = %ld\n",s->codec_param.enc_flag);
	printf("* fps          = %ld\n",s->codec_param.fps);
	printf("* bps          = %d\n", s->codec_param.bps);
	printf("* mode         = %ld\n",s->codec_param.mode);
	printf("* width        = %ld\n",s->codec_param.width);
	printf("* height       = %ld\n",s->codec_param.height);
	printf("* gop          = %ld\n",s->codec_param.gop);
	printf("* frame_count  = %ld\n",s->codec_param.frame_count);
	printf("* rot_angle    = %ld\n",s->codec_param.rot_angle);
	printf("* out_ratio    = %ld\n",s->codec_param.out_ratio);
	printf("* mirror_angle = %ld\n",s->codec_param.mirror_angle);
	printf("**********************************************************\n\n");

	fclose(codec_cfg_fd);

	if (ret < 0)
	{
		printf("Configuring the Codec from %s failed\n",codec_cfg_name);
	    return -1;
	}
	else
	{
		printf("Configuring the Codec from %s successful\n",codec_cfg_name);
	}

    ret = vpu_SystemInit();

	if (ret < 0)
	{
		//printf("VPU_SystemInit Failed\n");
		LOG_RUNLOG_ERROR("LP IMX27 VPU_SystemInit Failed");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_REBOOT, 0, "");
		LOGUploadResPrepross();
		system("reboot");
		return -1;
	}

	printf("codec_num %d\n", codec_num);

	ret = vpu_GetVersionInfo(&vpu_ver);

	printf("VPU Firmware Version: %d.%d.%d\n", vpu_ver.fw_major,  vpu_ver.fw_minor,  vpu_ver.fw_release );
	printf("VPU Library  Version: %d.%d.%d\n", vpu_ver.lib_major, vpu_ver.lib_minor, vpu_ver.lib_release);

	if (ret != 0)
	{
		printf("Can't get the firmware version info. Error code is %d\n", ret);
		return -1;
	}

	s->codec_param.index = 0;

	if(s->codec_param.enc_flag == 1) //Encoder
	{
		s->bCaptureStart = TRUE;

		//ret = ms_thread_create(&codec_thread[0],NULL, VIMX27Processthread,(void *)(&(codec_param[0])));
		ret = ms_thread_create(&s->thread,NULL, VIMX27Processthread,s);

	    if (ret < 0)
	    {
	    	printf("Create the thread for video capture failed\n");

	        vpu_SystemShutdown();

	        return -1;
	    }
	    else
	    {
	    	printf("Create the thread for video capture succeed\n");
	    }
	}

	return 0;
}
/*************************************************
*Name	:VIMX27StopCapture
*Input	:
*Output	:
*Desc	:关闭线程
**************************************************/
static void VIMX27StopCapture(VIMX27State *s)
{
    if(s->bCaptureStart)
    {
    	s->bCaptureStart = FALSE;

        ms_thread_join(s->thread,NULL);

        vpu_SystemShutdown();

		printf("VIMX27StopCapture:VIMX27 thread joined\n");

        flushq(&s->captureQ,0);
    }
}
/*************************************************
*Name	:IMX27CaptureFilterInit
*Input	:
*Output	:
*Desc	:init
**************************************************/
static void IMX27CaptureFilterInit(MSFilter *f)
{
	VIMX27State          *s    = ms_new0(VIMX27State,1);
	CAPTURE_DATA_CTX_ST  *ctx  = &(s->ctx);

	printf("Enter Webcam Filter Init\n");

	s->devname			= ms_strdup("/dev/v4l/Video0");
	s->video_fd			= -1;
	s->bCaptureStart    = FALSE;

	qinit(&s->captureQ);
    ms_mutex_init(&s->mutex,NULL);

    //init ctx
    memset(ctx->buf,0x0,CAPTURE_DATA_BUF_SIZE * sizeof(unsigned char));
    ctx->len     = 0;
    ctx->head    = ctx->buf;
    ctx->counter = 0;
    ctx->state   = BEGIN_FLAG_NOT_FOUND;


	f->data				= s;
	printf("Out Webcam Filter Init\n");

}
/*************************************************
*Name	:IMX27CaptureFilterPreprocess
*Input	:
*Output	:
*Desc	:preprocess
**************************************************/
static void IMX27CaptureFilterPreprocess(MSFilter *f)
{
	int ret;

	VIMX27State *s=(VIMX27State *)f->data;

	printf("Enter Webcam Filter Preprocess\n");

	ret = VIMX27StartCapture(s);						//创建视频采集线程（初始化、采集）

	if(ret == -1)
	{
        //printf("VIMXStartCapture Failed\n");
        LOG_RUNLOG_ERROR("LP IMX27 VIMXStartCapture Failed");
        LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_REBOOT, 0, "");
        LOGUploadResPrepross();
        system("reboot");
	}
	else
	{
        printf("VIMXStartCapture Succeed\n");
	}

	printf("Out Webcam Filter Preprocess\n");

}
/*************************************************
*Name	:IMX27CaptureFilterProcess
*Input	:
*Output	:
*Desc	:process
**************************************************/
#if 1
static void IMX27CaptureFilterProcess(MSFilter *f)
{
	VIMX27State *s = (VIMX27State *)f->data;
	uint32_t	timestamp;

   	mblk_t *om = NULL;

	//printf("Enter Webcam Filter Process\n");

	if(s->bCaptureStart == FALSE)
	{
		return;
	}

	ms_mutex_lock(&s->mutex);

    while((om=getq(&s->captureQ))!=0)
	{
		//printf("Filter收到数据帧:db_base = 0x%8X,size=%d\n",om->b_datap->db_base,om->b_datap->db_lim-om->b_datap->db_base);
		timestamp = f->ticker->time*90;

		mblk_set_timestamp_info(om,timestamp);
		mblk_set_marker_info(om,TRUE);
	 	ms_queue_put(f->outputs[0],om);
    }

    ms_mutex_unlock(&s->mutex);

	//printf("Out Webcam Filter Process\n");

}
#endif

#if 0
/****************************************************
 *  IMX27
 ****************************************************/
int FindByte(unsigned char *data, int len, unsigned char byte)
{
    int  i = 1;

    while( (i <= len) && (len > 0) )
    {
        if( *(data + i - 1) == byte )
        {
            return i;
        }
        else
        {
            i++;
        }
    }
    return 0;
}

int FindBytes(unsigned char *data, int len, unsigned char *bytes, int byteNum)
{
    int i = 1;
    int firstIndex = 0, index = 0;

    do
    {
        firstIndex = FindByte(data + index, len - index, bytes[0]);

        if(firstIndex == 0)
        {
            return 0;
        }

        index += firstIndex;

        while(len >= index)
        {
            if(i < byteNum)
            {
                if(data[index] == bytes[i])
                {
                    index++;
                    i++;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        if(i >= byteNum)
        {
            return index;
        }
    }while(index <= len);

    return 0;
}
static void IMX27CaptureFilterProcess(MSFilter *f)
{
	VIMX27State          *s    = (VIMX27State *)f->data;
	CAPTURE_DATA_CTX_ST  *ctx  = &(s->ctx);
	uint32_t	timestamp;

   	mblk_t *m = NULL;


   	unsigned char       *tmpbuf;
   	unsigned int         tmpbuflen;
   	unsigned int         offset;
   	unsigned char        naluend = NALU_END_FLAG_NOT_FOUND;
   	unsigned char       *NALUData = NULL;

   	unsigned char        NALU_BEGIN_FLAG1[4]={0x00,0x00,0x00,0x01};
   	unsigned char        NALU_END_FLAG1[4]  ={0x00,0x00,0x00,0x01};

   	unsigned char        NALU_BEGIN_FLAG2[4]={0x00,0x00,0x01};
   	unsigned char        NALU_END_FLAG2[4]  ={0x00,0x00,0x01};



	//printf("Enter Webcam Filter Process\n");

	if(s->bCaptureStart == FALSE)
	{
		return;
	}

	ms_mutex_lock(&s->mutex);

    while( ( m = getq(&s->captureQ) ) != 0 )
	//while( s->bCaptureStart )
	{
    	/*
    	timestamp = f->ticker->time*90;

    	mblk_set_timestamp_info(m,timestamp);
    	mblk_set_marker_info(m,TRUE);
    	ms_queue_put(f->outputs[0],m);
    	*/

		//printf("Filter收到数据帧:db_base = 0x%8X,size=%d\n",m->b_datap->db_base,m->b_datap->db_lim - m->b_datap->db_base);

    	if(ctx->counter >= CAPTURE_DATA_LMT_TIMES)
    	{
    	    ctx->len     = 0;
    	    ctx->head    = ctx->buf;
    	    ctx->counter = 0;
    	    ctx->state   = BEGIN_FLAG_NOT_FOUND;
    	}

    	//if( ( m = getq(&s->captureQ) ) == 0 )
    	//{
        //    break;
    	//}

    	printf("Filter收到数据帧:db_base = 0x%8X,size=%d\n", m->b_datap->db_base , m->b_datap->db_lim - m->b_datap->db_base);

    	tmpbuf     = m->b_datap->db_base;
    	tmpbuflen  = m->b_datap->db_lim - m->b_datap->db_base;

		if(tmpbuflen == 0)
		{
			continue;
		}

		if( &ctx->buf[CAPTURE_DATA_BUF_SIZE] < ctx->head + ctx->len + tmpbuflen)
		{
			printf("The Data exceed the buf\n");

            ctx->len     = 0;
            ctx->head    = ctx->buf;
            ctx->counter = 0;
            ctx->state   = BEGIN_FLAG_NOT_FOUND;
		}

		memcpy(ctx->head + ctx->len,tmpbuf,tmpbuflen);
		ctx->len = ctx->len + tmpbuflen;

		offset = 0xFFFF;

		while( (ctx->len > 0) && (offset != 0))
		{
            if(ctx->state == BEGIN_FLAG_NOT_FOUND)
            {
            	offset = FindBytes(ctx->head ,ctx->len,NALU_BEGIN_FLAG1,sizeof(NALU_BEGIN_FLAG1));    //寻找NALU 标志头 00 00 00 01

            	if(offset == 0)
            	{
            		printf("NOT Found NALU_BEGIN_FLAG1,Go to Found NALU_BEGIN_FLAG2\n");

                	offset = FindBytes(ctx->head ,ctx->len,NALU_BEGIN_FLAG2,sizeof(NALU_BEGIN_FLAG2));//寻找NALU 标志头 00 00 01
            	}

            	if(offset !=0 )  //找到标志头
            	{
            		printf("Found NALU Header\n");

                    ctx->head  = ctx->head + offset;
                    ctx->len   = ctx->len  - offset;
                    ctx->state = BEGIN_FLAG_FOUND;
            	}
            	else             //没有找到标志头，数据结构重置
            	{
            		printf("NOT Found NALU Header\n");

                    ctx->head  = ctx->buf;
                    ctx->len   = 0;
            	}
            }
            else
            {
            	offset = FindBytes(ctx->head ,ctx->len,NALU_END_FLAG1,sizeof(NALU_END_FLAG1));        //寻找NALU 标志尾 00 00 00 01

                if(offset != 0)
                {
                	naluend = NALU_END_FLAG1_FOUND;                                                 //找到NALU 标志尾 00 00 00 01

                	printf("Found NALU END FLAG1\n");
                }
                else
                {
                	offset = FindBytes(ctx->head ,ctx->len,NALU_END_FLAG2,sizeof(NALU_END_FLAG2));    //寻找NALU 标志尾 00 00 01

                	if(offset != 0)
                	{
                		naluend = NALU_END_FLAG2_FOUND;

                		printf("Found NALU END FLAG2\n");
                	}
                }

            	if(offset == 0)  //未检测到结束标志，counter开始计数
            	{
            		ctx->counter++;
            		naluend = NALU_END_FLAG_NOT_FOUND;

            		printf("NOT Found NALU END FLAG,ctx->counter = %d\n",ctx->counter);

            	}
            	else             //检测到结束标志
            	{
            		ctx->counter = 0; //counter清零

            		if( naluend == NALU_END_FLAG1_FOUND )
            		{
            			NALUData = (unsigned char *)malloc(offset-sizeof(NALU_END_FLAG1));
            		}
            		else if( naluend == NALU_END_FLAG2_FOUND)
            		{
            			NALUData = (unsigned char *)malloc(offset-sizeof(NALU_END_FLAG2));
            		}
            		else
            		{

            		}

            		if(NALUData != NULL) //内存申请成功
            		{
            			if( naluend == NALU_END_FLAG1_FOUND )
            			{
                            memcpy(NALUData,ctx->head,offset-sizeof(NALU_END_FLAG1));

                		    m = esballoc(NALUData , offset-sizeof(NALU_END_FLAG1) , 0 , NULL);

                		    m->b_wptr += ( offset - sizeof(NALU_END_FLAG1) );
            			}
            			else if( naluend == NALU_END_FLAG2_FOUND )
            			{
            				memcpy(NALUData,ctx->head,offset-sizeof(NALU_END_FLAG2));

            				m = esballoc(NALUData , offset-sizeof(NALU_END_FLAG2) , 0 , NULL);

            				m->b_wptr += ( offset - sizeof(NALU_END_FLAG2) );

            			}
            			else
            			{

            			}

            			printf("The valid NALU:db_base = 0x%8X,size = %d\n",m->b_datap->db_base,m->b_datap->db_lim-m->b_datap->db_base);

            			timestamp = f->ticker->time*90;

            			mblk_set_timestamp_info(m,timestamp);

            			mblk_set_marker_info(m,TRUE);

            			ms_queue_put(f->outputs[0],m);

            			printf("Send NALU to Encoder\n");

                		free(NALUData);
            		}

            		if( naluend == NALU_END_FLAG1_FOUND )
            		{
            			ctx->head = ctx->head + ( offset - sizeof(NALU_END_FLAG1) );

            			ctx->len  = ctx->len  - ( offset - sizeof(NALU_END_FLAG1) );
            		}
            		else if( naluend == NALU_END_FLAG2_FOUND )
            		{
            			ctx->head = ctx->head + ( offset - sizeof(NALU_END_FLAG2) );

            			ctx->len  = ctx->len  - ( offset - sizeof(NALU_END_FLAG2) );
            		}
            		else
            		{

            		}

            		ctx->state  = BEGIN_FLAG_NOT_FOUND;

            	}
            }
		}

        if(ctx->head != ctx->buf)
        {
        	memcpy(ctx->buf,ctx->head,ctx->len);
        	ctx->head = ctx->buf;
        	printf("Head go to buf\n");
        }


    }

    ms_mutex_unlock(&s->mutex);

	//printf("Out Webcam Filter Process\n");

}
#endif
/*************************************************
*Name	:IMX27CaptureFilterPostProcess
*Input	:
*Output	:
*Desc	:postprocess
**************************************************/
static void IMX27CaptureFilterPostProcess(MSFilter *f)
{
	VIMX27State *s = (VIMX27State *)f->data;

	printf("Enter Webcam Filter Postprocess\n");

	VIMX27StopCapture(s);		//关闭

	printf("Out Webcam Filter Postprocess\n");

}
/*************************************************
*Name	:IMX27CaptureFilterUninit
*Input	:
*Output	:
*Desc	:uninit
**************************************************/
static void IMX27CaptureFilterUninit(MSFilter *f)
{
	VIMX27State* s =(VIMX27State*)f->data;

	printf("Enter Webcam Filter:Uninit\n");

	if(NULL != s->devname)
		ms_free(s->devname);

    flushq(&s->captureQ,0);
    ms_mutex_destroy(&s->mutex);
    ms_free(s);

	printf("Out Webcam Filter:Uninit\n");

}
//视频采集设备对应的Filter
MSFilterDesc ms_VIMX27_desc = {
    MS_IMX27_VIDEO_CAPTURE_ID,
    "IMX27",
    N_("A filter that captures IMX27 video."),
    MS_FILTER_OTHER,
    NULL,
    0,
    1,
    IMX27CaptureFilterInit,
    IMX27CaptureFilterPreprocess,
    IMX27CaptureFilterProcess,
    IMX27CaptureFilterPostProcess,
    IMX27CaptureFilterUninit,
    NULL//ms_IMX27_capture_methods
};
MS_FILTER_DESC_EXPORT(ms_VIMX27_desc)
/*************************************************
*Name	:VIMX27_set_devfile
*Input	:
*Output	:
*Desc	:
**************************************************/
static int VIMX27_set_devfile(MSFilter *f,void *arg)
{
	VIMX27State *s = (VIMX27State*)f->data;

	if(s->devname)
		ms_free(s->devname);

	s->devname = ms_strdup((char*)arg);

	return 0;
}
/*************************************************
*Name	:VIMX27CaptureDetect
*Input	:
*Output	:
*Desc	:
**************************************************/
static void VIMX27CaptureDetect(MSWebCamManager *obj)
{
	const char *IMX27Videodevicename="/dev/v4l/Video0";

	//int video_fd = open(IMX27Videodevicename, O_RDWR);   	//open  video device
	int video_fd = 1;

	printf("Enter Webcam Detect: Open Video Device fd=%d\n",video_fd);

    if(video_fd >= 0)
    {
    	MSWebCam *cam	= ms_web_cam_new(&VIMX27WebCamDesc);
		cam->name		= ms_strdup(IMX27Videodevicename);
    	ms_web_cam_manager_add_cam(obj, cam);

    	//close(video_fd);

    	printf("Close Video Device fd\n");
    }

	printf("Out Webcam Detect: Close Video Device\n");
}
/*************************************************
*Name	:VIMX27CaptureCamInit
*Input	:
*Output	:
*Desc	:视频设备初始化
**************************************************/
static void VIMX27CaptureCamInit(MSWebCam *cam)
{
	printf("Enter Webcam Init:NULL\n");
	printf("Out   Webcam Init:NULL\n");
}
/*************************************************
*Name	:VIMX27CaptureCreateReader
*Input	:
*Output	:
*Desc	:创建相应的Filter
**************************************************/
static MSFilter *VIMX27CaptureCreateReader(MSWebCam *obj)
{
    MSFilter 	*f = ms_filter_new_from_desc(&ms_VIMX27_desc);
    VIMX27State	*s = (VIMX27State *)f->data;

	printf("Enter Webcam CreateReader\n");

	VIMX27_set_devfile(f,obj->name);

	printf("Out Webcam CreateReader\n");

    return f;

}
/*************************************************
*Name	:VIMX27CaptureCamUninit
*Input	:
*Output	:
*Desc	:uninit
**************************************************/
static void VIMX27CaptureCamUninit(MSWebCam *obj)
{
	printf("Enter Webcam uninit:NULL\n");
	printf("Out   Webcam uninit:NULL\n");
}
//定义视频采集设备描述
MSWebCamDesc VIMX27WebCamDesc={
    "VIMX27WebCam",
    &VIMX27CaptureDetect,
    &VIMX27CaptureCamInit,
    &VIMX27CaptureCreateReader,
    &VIMX27CaptureCamUninit
};
//===============================================================================================
/*************************************************
*Name	:enc_IMX27mpeg4_init
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27Mpeg4_init(MSFilter *f)
{
printf("Complete IMX27Mpeg4 encoder init\n");
}
/*************************************************
*Name	:enc_IMX27Mpeg4_preprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27Mpeg4_preprocess(MSFilter *f)
{
printf("Complete IMX27Mpeg4 encoder preprocess\n");
}
/*************************************************
*Name	:enc_IMX27Mpeg4_process
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27Mpeg4_process(MSFilter *f)
{
	mblk_t 		*inm;
	mblk_t 		*packet		=NULL;
	uint8_t 	*rptr;
	int 		len 		= 0;
	int 		mtu 		= ms_get_payload_max_size() - 2;
	uint32_t 	timestamp	= f->ticker->time*90LL;

	//printf("进入IMX27Mpeg4编码处理过程\n");

	ms_filter_lock(f);
	while((inm=ms_queue_get(f->inputs[0]))!=0)
	{
		for (rptr=inm->b_rptr;rptr<inm->b_wptr;)
		{
			len				= MIN(mtu,(inm->b_wptr-rptr));
			packet			= dupb(inm);
			packet->b_rptr	= rptr;
			packet->b_wptr	= rptr+len;
			mblk_set_timestamp_info(packet, timestamp);
			ms_queue_put(f->outputs[0],packet);
			rptr+=len;
		}
		mblk_set_marker_info(packet,TRUE);
		freemsg(inm);
		//printf("编码器向RTP发送消息\n");
		//printf("读取视频帧消息\n");
		//timestamp = f->ticker->time*90LL;
		//printf("盖时间戳=%d\n",timestamp);
		//mblk_set_timestamp_info(inm,timestamp);
		//ms_queue_put(f->outputs[0],inm);
		//printf("发送视频帧消息\n");
		//set marker bit on last packet
		//mblk_set_marker_info(inm,TRUE);
		//freemsg(inm);
	}
	ms_filter_unlock(f);
	//printf("退出IMX27Mpeg4编码处理过程\n");
}
/*************************************************
*Name	:enc_IMX27Mpeg4_postprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27Mpeg4_postprocess(MSFilter *f)
{
	printf("Complete IMX27Mpeg4 encoder postprocess\n");
}
/*************************************************
*Name	:enc_IMX27Mpeg4_uninit
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27Mpeg4_uninit(MSFilter  *f)
{
	printf("Complete IMX27Mpeg4 encoder uninit\n");
}
//========================================================================
/*************************************************
*Name	:dec_IMX27mpeg4_init
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27Mpeg4_init(MSFilter *f)
{
printf("Complete IMX27Mpeg4 decoder init\n");
}
/*************************************************
*Name	:dec_IMX27Mpeg4_preprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27Mpeg4_preprocess(MSFilter *f)
{
printf("Complete IMX27Mpeg4 decoder preprocess\n");
}

/*************************************************
*Name	:dec_IMX27Mpeg4_process
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27Mpeg4_process(MSFilter *f)
{
	printf("Complete IMX27Mpeg4 decoder process\n");
}
/*************************************************
*Name	:dec_IMX27Mpeg4_postprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27Mpeg4_postprocess(MSFilter *f)
{
	printf("Complete IMX27Mpeg4 decoder postprocess\n");
}
/*************************************************
*Name	:dec_IMX27Mpeg4_uninit
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27Mpeg4_uninit(MSFilter  *f)
{
	printf("Complete IMX27Mpeg4 decoder uninit\n");
}
//===============================================================================================
//
//===============================================================================================
//===============================================================================================
//									H 2 6 4
//===============================================================================================
/*************************************************
*Name	:enc_IMX27H264_init
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27H264_init(MSFilter *f)
{
	printf("Complete IMX27H264 encoder init\n");
}
/*************************************************
*Name	:enc_IMX27H264_preprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27H264_preprocess(MSFilter *f)
{
	printf("Complete IMX27H264 encoder preprocess\n");
}
/*************************************************
*Name	:enc_IMX27H264_process
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27H264_process(MSFilter *f)
{
    Rfc3984Context	*Context4H264;
	uint32_t 		timestamp	= f->ticker->time*90LL;

	Context4H264 = rfc3984_new();
	ms_filter_lock(f);
	//printf("Pack the NALU\n");
	rfc3984_pack(Context4H264, f->inputs[0], f->outputs[0], timestamp);
	ms_filter_unlock(f);

}
/*************************************************
*Name	:enc_IMX27H264_postprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27H264_postprocess(MSFilter *f)
{
	printf("Complete IMX27H264 encoder postprocess\n");
}
/*************************************************
*Name	:enc_IMX27H264_uninit
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_IMX27H264_uninit(MSFilter  *f)
{
	printf("Complete IMX27H264 encoder uninit\n");
}
//========================================================================
/*************************************************
*Name	:dec_IMX27H264_init
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27H264_init(MSFilter *f)
{
	printf("Complete IMX27H264 decoder init\n");
}
/*************************************************
*Name	:dec_IMX27H264_preprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27H264_preprocess(MSFilter *f)
{
	printf("Complete IMX27H264 decoder preprocess\n");
}
/*************************************************
*Name	:dec_IMX27H264_process
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27H264_process(MSFilter *f)
{
	printf("Complete IMX27H264 decoder process\n");
}
/*************************************************
*Name	:dec_IMX27H264_postprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27H264_postprocess(MSFilter *f)
{
	printf("Complete IMX27H264 decoder postprocess\n");
}
/*************************************************
*Name	:dec_IMX27H264_uninit
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_IMX27H264_uninit(MSFilter  *f)
{
	printf("Complete IMX27H264decoder uninit\n");
}

#ifdef _MSC_VER
//Freescale VIMX27 ms_VIMX27Mpeg4Enc_Desc  编码器
MSFilterDesc ms_VIMX27Mpeg4Enc_Desc={
	MS_IMX27_VIDEO_MPEG4ENC_ID,
	"MSMpeg4Enc",
	N_("A video MPEG4 encoder for Freescale IMX27"),
	MS_FILTER_ENCODER,
	"MP4V-ES",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_IMX27Mpeg4_init,
	enc_IMX27Mpeg4_preprocess,
	enc_IMX27Mpeg4_process,
	enc_IMX27Mpeg4_postprocess,
	enc_IMX27Mpeg4_uninit,
	NULL
};

//Freescale VIMX27 ms_VIMX27Mpeg4Dec_Desc 解码器
MSFilterDesc ms_VIMX27Mpeg4Dec_Desc={
	MS_IMX27_VIDEO_MPEG4DEC_ID,
	"MSMpeg4Dec",
	N_("A video MPEG4 decoder for Freescale IMX27"),
	MS_FILTER_DECODER,
	"MP4V-ES",
	1, /*MS_YUV420P is assumed on this input */
	1,
	dec_IMX27Mpeg4_init,
	dec_IMX27Mpeg4_preprocess,
	dec_IMX27Mpeg4_process,
	dec_IMX27Mpeg4_postprocess,
	dec_IMX27Mpeg4_uninit,
	NULL
};
MS_FILTER_DESC_EXPORT(ms_VIMX27Mpeg4Enc_Desc)
MS_FILTER_DESC_EXPORT(ms_VIMX27Mpeg4Dec_Desc)

//Freescale VIMX27 ms_VIMX27H264Enc_Desc
MSFilterDesc ms_VIMX27H264Enc_Desc={
	MS_IMX27_VIDEO_H264ENC_ID,
	"MSH264Enc",
	N_("A video H264 encoder for Freescale IMX27"),
	MS_FILTER_ENCODER,
	"H264",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_IMX27H264_init,
	enc_IMX27H264_preprocess,
	enc_IMX27H264_process,
	enc_IMX27H264_postprocess,
	enc_IMX27H264_uninit,
	NULL
};
//Freescale VIMX27 ms_VIMX27H264Dec_Desc
MSFilterDesc ms_VIMX27H264Dec_Desc={
	MS_IMX27_VIDEO_H264DEC_ID,
	"MSH264Dec",
	N_("A video H264 decoder for Freescale IMX27"),
	MS_FILTER_DECODER,
	"H264",
	1, /*MS_YUV420P is assumed on this input */
	1,
	dec_IMX27H264_init,
	dec_IMX27H264_preprocess,
	dec_IMX27H264_process,
	dec_IMX27H264_postprocess,
	dec_IMX27H264_uninit,
	NULL
};
MS_FILTER_DESC_EXPORT(ms_VIMX27H264Enc_Desc)
MS_FILTER_DESC_EXPORT(ms_VIMX27H264Dec_Desc)
#else
//Freescale VIMX27 ms_VIMX27Mpeg4Enc_Desc  编码器
MSFilterDesc ms_VIMX27Mpeg4Enc_Desc=
{
	.id			= MS_IMX27_VIDEO_MPEG4ENC_ID,
	.name		= "MSMpeg4Enc",
	.text		= N_("A video MPEG4 encoder for Freescale IMX27"),
	.category	= MS_FILTER_ENCODER,
	.enc_fmt	= "MP4V-ES",
	.ninputs	= 1, /*MS_YUV420P is assumed on this input */
	.noutputs	= 1,
	.init		= enc_IMX27Mpeg4_init,
	.preprocess	= enc_IMX27Mpeg4_preprocess,
	.process	= enc_IMX27Mpeg4_process,
	.postprocess= enc_IMX27Mpeg4_postprocess,
	.uninit		= enc_IMX27Mpeg4_uninit,

	.methods	= NULL
};
//Freescale VIMX27 ms_VIMX27Mpeg4Dec_Desc 解码器
MSFilterDesc ms_VIMX27Mpeg4Dec_Desc=
{
	.id			= MS_IMX27_VIDEO_MPEG4DEC_ID,
	.name		= "MSMpeg4Dec",
	.text		= N_("A video MPEG4 decoder for Freescale IMX27"),
	.category	= MS_FILTER_DECODER,
	.enc_fmt	= "MP4V-ES",
	.ninputs	= 1, /*MS_YUV420P is assumed on this input */
	.noutputs	= 1,
	.init		= dec_IMX27Mpeg4_init,
	.preprocess	= dec_IMX27Mpeg4_preprocess,
	.process	= dec_IMX27Mpeg4_process,
	.postprocess= dec_IMX27Mpeg4_postprocess,
	.uninit		= dec_IMX27Mpeg4_uninit,

	.methods	= NULL
};
MS_FILTER_DESC_EXPORT(ms_VIMX27Mpeg4Enc_Desc)
MS_FILTER_DESC_EXPORT(ms_VIMX27Mpeg4Dec_Desc)
//Freescale VIMX27 ms_VIMX27H264Enc_Desc
MSFilterDesc ms_VIMX27H264Enc_Desc=
{
	.id			= MS_IMX27_VIDEO_H264ENC_ID,
	.name		= "MSH264Enc",
	.text		= N_("A video H264 encoder for Freescale IMX27"),
	.category	= MS_FILTER_ENCODER,
	.enc_fmt	= "H264",
	.ninputs	= 1, /*MS_YUV420P is assumed on this input */
	.noutputs	= 1,
	.init		= enc_IMX27H264_init,
	.preprocess	= enc_IMX27H264_preprocess,
	.process	= enc_IMX27H264_process,
	.postprocess= enc_IMX27H264_postprocess,
	.uninit		= enc_IMX27H264_uninit,
	.methods	= NULL
};
//Freescale VIMX27 ms_VIMX27H264Dec_Desc
MSFilterDesc ms_VIMX27H264Dec_Desc=
{
	.id			= MS_IMX27_VIDEO_H264DEC_ID,
	.name		= "MSH264Dec",
	.text		= N_("A video H264 decoder for Freescale IMX27"),
	.category	= MS_FILTER_DECODER,
	.enc_fmt	= "H264",
	.ninputs	= 1, /*MS_YUV420P is assumed on this input */
	.noutputs	= 1,
	.init		= dec_IMX27H264_init,
	.preprocess	= dec_IMX27H264_preprocess,
	.process	= dec_IMX27H264_process,
	.postprocess= dec_IMX27H264_postprocess,
	.uninit		= dec_IMX27H264_uninit,
	.methods	= NULL
};
MS_FILTER_DESC_EXPORT(ms_VIMX27H264Enc_Desc)
MS_FILTER_DESC_EXPORT(ms_VIMX27H264Dec_Desc)
#endif

#endif
