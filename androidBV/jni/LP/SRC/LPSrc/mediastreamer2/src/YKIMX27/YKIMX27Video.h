///*********************************************************************
//*  Copyright (C), 2001 - 2012, 福建邮科通信技术有限公司.
//*  File			:
//*  Author		:李程达
//*  Version 		:
//*  Date			:
//*  Description	:
//*  History		: 更新记录 .
//*            	  <修改作者>  <修改时间>  <修改描述>
//**********************************************************************/
//
#ifdef _YK_IMX27_AV_

#ifndef _YK_IMX27_VIDEO_H_
#define _YK_IMX27_VIDEO_H_

//头文件包含
//与系统相关
//与工程相关
#ifdef HAVE_CONFIG_H
#include "../../mediastreamer-config.h"
#endif

#ifdef VIDEO_ENABLED
#include <mediastreamer2/mswebcam.h>
#include <mediastreamer2/msvideo.h>
#include "mediastreamer2/rfc3984.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "vpu_voip_test.h"

extern MSWebCamDesc VIMX27WebCamDesc;


#endif//!VIDEO_ENABLED

//宏定义
#define CAPTURE_DATA_BUF_SIZE    4096
#define CAPTURE_DATA_LMT_TIMES   5

#define BEGIN_FLAG_NOT_FOUND     0x01
#define BEGIN_FLAG_FOUND         0x02
#define NALU_END_FLAG1_FOUND     0x03
#define NALU_END_FLAG2_FOUND     0x04
#define NALU_END_FLAG_NOT_FOUND  0x05
//全局变量声明
typedef struct
{
    unsigned char  buf[CAPTURE_DATA_BUF_SIZE]	;
    unsigned int   len;            //the length of valid data in the ring buf
    unsigned char *head;          //the point to the valid data
    unsigned char  counter;
    unsigned char  state;
}CAPTURE_DATA_CTX_ST;

//VIMX27私有数据结构

typedef struct VIMX27State{
	ms_thread_t     		thread; 				// thread
	ms_mutex_t      		mutex;  				// mutex
	queue_t         		captureQ;       		// queque

	char           		   *devname;   				// device name
	int						video_fd;				//
	bool_t          		bCaptureStart;  		//
	CAPTURE_DATA_CTX_ST     ctx;

	struct 	codec_config 	codec_param;
}VIMX27State;
////函数声明
////本地函数声明
////外部函数声明
extern void *VIMX27Processthread(void *data);
extern int vpu_getcfg(int *codec_num,struct codec_config *usr_config, FILE * cfg_file);
#endif //_YK_IMX27_VIDEO_H_

#endif //_YK_IMX27_AV_
