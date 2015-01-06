/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file mxc_v4l2_capture.c
 * 
 * @brief Mxc Video For Linux 2 driver test application
 * 
 */

#ifdef _YK_IMX27_AV_

#ifdef __cplusplus
extern "C"{
#endif


/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <errno.h>
    
/* Verification Test Environment Include Files */
#include <sys/types.h>	
#include <sys/stat.h>	
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>    
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <asm/types.h>
#include <linux/compiler.h>
#include <linux/videodev.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>
//#include <asm/arch/mxc_v4l2.h>
#include "TMInterface.h"
#include <FPInterface.h>

#define TRUE 0
#define FALSE -1

#define CP_REMOTE_PATH			"./CapturePic/"

extern int Y2J(const char *yuv_file_name, const char *jpeg_file_name);

struct v4l2_mxc_offset {
	uint32_t u_offset;
	uint32_t v_offset;
};

#define TEST_BUFFER_NUM 3

struct testbuffer
{
        unsigned char *start;
        size_t offset;
        unsigned int length;
};

struct testbuffer buffers[TEST_BUFFER_NUM];
int g_width = 176;
int g_height = 144;
int g_capture_count = 100;
int g_rotate = 0;
int g_cap_fmt = V4L2_PIX_FMT_YUV420;
int g_camera_framerate = 30;
int g_extra_pixel = 0;

char CurrFileName[128] = {0};
char RemoteFileName[128] = {0};

static int start_capturing(int fd_v4l)
{
        unsigned int i;
        struct v4l2_buffer buf;
        enum v4l2_buf_type type;

        for (i = 0; i < TEST_BUFFER_NUM; i++)
        {
                memset(&buf, 0, sizeof (buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.index = i;
                if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &buf) < 0)
                {
                        printf("VIDIOC_QUERYBUF error\n");
                        return -1;
                }

                buffers[i].length = buf.length;
                buffers[i].offset = (size_t) buf.m.offset;
                buffers[i].start = mmap (NULL, buffers[i].length, 
                    PROT_READ | PROT_WRITE, MAP_SHARED, 
                    fd_v4l, buffers[i].offset);
				memset(buffers[i].start, 0xFF, buffers[i].length);
        }

        for (i = 0; i < TEST_BUFFER_NUM; i++)
        {
                memset(&buf, 0, sizeof (buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
				buf.m.offset = buffers[i].offset; 
				if (g_extra_pixel){
	                buf.m.offset += g_extra_pixel * 
	                	(g_width + 2 * g_extra_pixel) + g_extra_pixel;
				}

                if (ioctl (fd_v4l, VIDIOC_QBUF, &buf) < 0) {
                        printf("VIDIOC_QBUF error\n");
                        return -1;
                }
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl (fd_v4l, VIDIOC_STREAMON, &type) < 0) {
                printf("VIDIOC_STREAMON error\n");
                return -1;
        }
        return 0;
}

static int stop_capturing(int fd_v4l)
{
        enum v4l2_buf_type type;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        return ioctl (fd_v4l, VIDIOC_STREAMOFF, &type);
}

static int v4l_capture_setup(void)
{
        char v4l_device[100] = "/dev/v4l/video0";
        struct v4l2_format fmt;
        struct v4l2_control ctrl;
        struct v4l2_streamparm parm;
        int fd_v4l = 0;
		struct v4l2_mxc_offset off;

        if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0)
        {
                printf("Unable to open %s\n", v4l_device);
                return 0;
        }

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
        fmt.fmt.pix.width = g_width;
        fmt.fmt.pix.height = g_height;
        if (g_extra_pixel){
			off.u_offset = (2 * g_extra_pixel + g_width) * (g_height + g_extra_pixel)
				 - g_extra_pixel + (g_extra_pixel / 2) * ((g_width / 2) 
				 + g_extra_pixel) + g_extra_pixel / 2;  
			off.v_offset = off.u_offset + (g_extra_pixel + g_width / 2) * 
				((g_height / 2) + g_extra_pixel);  
        	fmt.fmt.pix.bytesperline = g_width + g_extra_pixel * 2;
			fmt.fmt.pix.priv = (uint32_t) &off;
        	fmt.fmt.pix.sizeimage = (g_width + g_extra_pixel * 2 ) 
        		* (g_height + g_extra_pixel * 2) * 3 / 2;
		} else {
	        fmt.fmt.pix.bytesperline = g_width;
			fmt.fmt.pix.priv = 0;
        	fmt.fmt.pix.sizeimage = 0;
		}

        if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0)
        {
                printf("set format failed\n");
                return 0;
        } 

        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.timeperframe.numerator = 1;
        parm.parm.capture.timeperframe.denominator = g_camera_framerate;
        parm.parm.capture.capturemode = 0;
         
        if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0)
        {
                printf("VIDIOC_S_PARM failed\n");
                return -1;
        } 

        // Set rotation
        ctrl.id = V4L2_CID_PRIVATE_BASE + 0;
        ctrl.value = g_rotate;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
        {
                printf("set ctrl failed\n");
                return 0;
        }

        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof (req));
        req.count = TEST_BUFFER_NUM;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd_v4l, VIDIOC_REQBUFS, &req) < 0)
        {
                printf("v4l_capture_setup: VIDIOC_REQBUFS failed\n");
                return 0;
        }

        return fd_v4l;
}

static int v4l_capture_test(int fd_v4l, const char * file)
{
        struct v4l2_buffer buf;
#if TEST_OUTSYNC_ENQUE
        struct v4l2_buffer temp_buf;
#endif
        struct v4l2_format fmt;
        FILE * fd_y_file = 0;
        int count = g_capture_count;
   
        if ((fd_y_file = fopen(file, "wb")) < 0)
        {
                printf("Unable to create y frame recording file\n");
                return FALSE;
        }

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd_v4l, VIDIOC_G_FMT, &fmt) < 0)
        {
                printf("get format failed\n");
                return FALSE;
        } 
        else
        {
                printf("\t Width = %d", fmt.fmt.pix.width);
                printf("\t Height = %d", fmt.fmt.pix.height);
                printf("\t Image size = %d\n", fmt.fmt.pix.sizeimage);
                printf("\t pixelformat = %d\n", fmt.fmt.pix.pixelformat);
        }

        if (start_capturing(fd_v4l) < 0)
        {
                printf("start_capturing failed\n");
                return FALSE;
        } 

        while (count-- > 0) {
                memset(&buf, 0, sizeof (buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                if (ioctl (fd_v4l, VIDIOC_DQBUF, &buf) < 0)	{
                        printf("VIDIOC_DQBUF failed.\n");
                }

                fwrite(buffers[buf.index].start, fmt.fmt.pix.sizeimage, 1, fd_y_file); 

#if TEST_OUTSYNC_ENQUE
				/* Testing out of order enque */
				if (count == 25) {
					temp_buf = buf;
                    printf("buf.index %d\n", buf.index);
					continue;
				}

				if (count == 15) {
                        if (ioctl (fd_v4l, VIDIOC_QBUF, &temp_buf) < 0) {
                                printf("VIDIOC_QBUF failed\n");
                        		break;
                        }
				}
#endif
                if (count >= TEST_BUFFER_NUM) { 
                        if (ioctl (fd_v4l, VIDIOC_QBUF, &buf) < 0) {
                                printf("VIDIOC_QBUF failed\n");
                        		break;
                        }
                }
                else
                        printf("buf.index %d\n", buf.index);
        }

        if (stop_capturing(fd_v4l) < 0)
        {
                printf("stop_capturing failed\n");
                return FALSE;
        } 

        fclose(fd_y_file);

        close(fd_v4l);
        return TRUE;
}


static int process_cmdline(int argc, char **argv)
{
	int i;

	if(argc = 1)
	{
		g_width = 352;
		g_height = 288;
		g_capture_count = 2;
		g_camera_framerate = 30;
	}
	else
	{
		for (i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-w") == 0)
			{
				g_width = atoi(argv[++i]);
			}
			else if (strcmp(argv[i], "-h") == 0)
			{
				g_height = atoi(argv[++i]);
			}
			else if (strcmp(argv[i], "-r") == 0)
			{
				g_rotate = atoi(argv[++i]);
			}
			else if (strcmp(argv[i], "-c") == 0)
			{
				g_capture_count = atoi(argv[++i]);
			}
			else if (strcmp(argv[i], "-fr") == 0)
			{
				g_camera_framerate = atoi(argv[++i]);
			}
			else if (strcmp(argv[i], "-e") == 0)
			{
				g_extra_pixel = atoi(argv[++i]);
			}
			else if (strcmp(argv[i], "-f") == 0)
			{
				i++;
				g_cap_fmt = v4l2_fourcc(argv[i][0], argv[i][1],argv[i][2],argv[i][3]);

				if ( (g_cap_fmt != V4L2_PIX_FMT_BGR24) &&
					 (g_cap_fmt != V4L2_PIX_FMT_BGR32) &&
					 (g_cap_fmt != V4L2_PIX_FMT_RGB565) &&
					 (g_cap_fmt != V4L2_PIX_FMT_YUV420) )
				{
					return -1;
				}
			}
			else if (strcmp(argv[i], "-help") == 0)
			{
				printf("MXC Video4Linux capture Device Test\n\n" \
					   "Syntax: mxc_v4l2_capture.out -w <capture width>\n" \
					   " -h <capture height>\n" \
					   " -r <rotation> -c <capture counter> \n"
					   " -e <destination cropping: extra pixels> \n" \
					   " -f <format> -fr <frame rate, 0-auto> \n"); \
					   " -default or none argv:set with=352,heigh=288,c=2,fr=30";
				return -1;
		   }
		}

		printf("g_width = %d, g_height = %d\n", g_width, g_height);

		if ((g_width == 0) || (g_height == 0)) {
				return -1;
		}
	}
	return 0;
}

int CPCapturePicInit(void)
{
    g_width = 352;
	g_height = 288;
	g_capture_count = 2;
	g_camera_framerate = 30;
    g_rotate = 0;
    g_extra_pixel = 0;
    g_cap_fmt = V4L2_PIX_FMT_YUV420;

    return TRUE;
}


//int main(int argc, char **argv)
int CPCapturePic(const char *filename)
{
	int fd_v4l;
	time_t stTimep;
	struct tm *pstTm;
	char yuv_name[128] = {0};
	char jpeg_name[128] = {0};

	printf("CPCapturePic : filename:%s\n", filename);
/*
    if (filename == NULL)
    {
        printf("CPCapturePic : input file name error\n");
        return FALSE;
    }
*/
/*
	if (process_cmdline(argc, argv) < 0)
	{
		return FALSE;
	}
*/
    CPCapturePicInit();

	fd_v4l = v4l_capture_setup();
#if 0
	time(&stTimep);
	pstTm = localtime(&stTimep);

	snprintf(yuv_name, sizeof(yuv_name), "%04d%02d%02d%02d%02d%02d%s",
													1900 + pstTm->tm_year,
													1 + pstTm->tm_mon,
													pstTm->tm_mday,
													pstTm->tm_hour,
													pstTm->tm_min,
													pstTm->tm_sec,
													".yuv");

	snprintf(jpeg_name, sizeof(jpeg_name), "%04d%02d%02d%02d%02d%02d%s",
														1900 + pstTm->tm_year,
														1 + pstTm->tm_mon,
														pstTm->tm_mday,
														pstTm->tm_hour,
														pstTm->tm_min,
														pstTm->tm_sec,
														".jpeg");

    printf("jpeg file name:%s\n", jpeg_name);
    //strcpy(CurrFileName, jpeg_name);
    sprintf(CurrFileName, "%s%s", "/home/YK-IDBT/", jpeg_name);

    sprintf(RemoteFileName, "%s%s", CP_REMOTE_PATH, jpeg_name);
    printf("jpeg file name:%s\n", CurrFileName);
#else
    //sprintf(yuv_name, "%s", CP_REMOTE_PATH, jpeg_name);
    printf("CPCapturePic : rm /home/temp.yuv");
    system("busybox rm /home/temp.yuv");
    strcpy(yuv_name, "/home/temp.yuv");
    strcpy(jpeg_name, filename);
#endif
	if(v4l_capture_test(fd_v4l, yuv_name) == TRUE)
	{
		Y2J(yuv_name, jpeg_name);
	}

	time(&stTimep);
    pstTm = localtime(&stTimep);
    printf("capture end time %02d%02d%02d\n", pstTm->tm_hour, pstTm->tm_min, pstTm->tm_sec);

	return TRUE;
}

static void CPTranaferProcessFunc(FP_TRANSFER_INFO_ST* pstInfo)
{
	time_t stTimep;
	struct tm *pstTm;
	time(&stTimep);
	printf("upload time = %d\n", pstInfo->lTotalTime);
	pstTm = localtime(&stTimep);
	printf("CPTranaferProcessFunc time %02d%02d%02d\n", pstTm->tm_hour, pstTm->tm_min, pstTm->tm_sec);
	return;
}

int CPUploadPic(char *local, char *remote)
{
    char acUserName[256], acPassWord[256], pacHost[256]; int iPort=21;

    printf("CPUploadPic : local:%s, remote:%s\n", local, remote);

    TMQueryFtpInfo(pacHost, &iPort, acUserName, acPassWord);

    FPPut(pacHost, iPort, acUserName, acPassWord, local, remote, CPTranaferProcessFunc, NULL, FP_TRANSFER_NOWAIT);

    return TRUE;
}

#endif

