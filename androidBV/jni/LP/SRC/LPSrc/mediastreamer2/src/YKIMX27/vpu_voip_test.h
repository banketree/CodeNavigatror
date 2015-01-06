/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Copyright (c) 2006, Chips & Media.  All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file vpu_voip_test.h
 *
 * @brief This file is codec test header.
 *
 * @ingroup VPU
 */
#ifdef _YK_IMX27_AV_
#ifndef __VPU__VOIP__TEST__H
#define __VPU__VOIP__TEST__H

#include "vpu_io.h"
#include "vpu_lib.h"


#define PAL_MODE		0
#define NTSC_MODE		1

//#define	STREAM_BUF_SIZE		0x40000
#define	STREAM_BUF_SIZE		0xA0000

#if 0
#ifdef PAL_MODE
#define MAX_WIDTH		720
#define MAX_HEIGHT		576
#else
#define MAX_WIDTH		720
#define MAX_HEIGHT		480
#endif
#endif

//#ifdef VGA_PANEL
#define SCREEN_MAX_WIDTH	640
#define SCREEN_MAX_HEIGHT	480
//#else
//#define SCREEN_MAX_WIDTH	240
//#define SCREEN_MAX_HEIGHT	320
//#endif

#define MAX_WIDTH		720
// >>> alex
#define MAX_HEIGHT		480
//#define MAX_HEIGHT		576
// <<< alex

#define STRIDE			MAX_WIDTH

#define STREAM_FILL_SIZE	0x8000
#define	STREAM_END_SIZE		512	// 0

enum {
	COMMAND_NONE = 0,
	COMMAND_DECODE,
	COMMAND_ENCODE,
	COMMAND_MULTI_DEC,
	COMMAND_MULTI_CODEC
};

typedef struct {
	int Index;
	int AddrY;
	int AddrCb;
	int AddrCr;
	int StrideY;
	int StrideC;		/* Stride Cb/Cr */

	int DispY;		/* DispY is the page aligned AddrY */
	int DispCb;		/* DispCb is the page aligned AddrCb */
	int DispCr;
	vpu_mem_desc CurrImage;	/* Current memory descriptor for user space */
} FRAME_BUF;

struct codec_file {
	char name[50];
	FILE *fd;
};

//#define MAX_NUM_INSTANCE	4

/*display1/reference16/reconstruction1/loopfilter1/rot1 for 1 video, 20 in total*/
/*display4/reference16/reconstruction1/loopfilter1/rot1 for multi video, 23 in total */
/*why 22 here? */
#define NUM_FRAME_BUF	(1+17+2)
#define MAX_FRAME	(16+2+4)

struct codec_config {
	Uint32 	index;
	Uint32 	src;
	char 	src_name[80];
	Uint32 	dst;
	char 	dst_name[80];
	Uint32 	enc_flag;
	Uint32 	fps;
	int 	bps;
	Uint32 	mode;
	Uint32 	width;
	Uint32 	height;
	Uint32 	gop;
	Uint32 	frame_count;
	Uint32 	rot_angle;
	Uint32 	out_ratio;
	Uint32 	mirror_angle;
};

void *DecodeTest(void *param);
void *EncodeTest(void *param);
void *sig_thread(void *arg);	/* the thread is used to monitor signal */

int FillBsBufMulti(int src, char *src_name, int targetAddr,
		   int bsBufStartAddr, int bsBufEndAddr, int size,
		   int index, int checkeof, int *streameof, int read_flag,void *data);
/* Read/Write one frame raw data each time, for Encoder/Decoder respectively */
int FillYuvImageMulti(int src, char *src_name, int buf, void *emma_buf,
		      int inwidth, int inheight, int index, int read_flag,
		      int rot_en, int output_ratio, int frame_num);

int vpu_SystemInit(void);
int vpu_SystemShutdown(void);

#define TEST_BUFFER_NUM		3

#define CODEC_READING		0
#define CODEC_WRITING		1

#define PATH_FILE		0
#define PATH_EMMA		1
#define PATH_NET		2

pthread_t codec_thread[MAX_NUM_INSTANCE];

#ifdef DEBUG
#define DPRINTF(fmt, args...) printf("%s: " fmt , __FUNCTION__, ## args)
#else
#define DPRINTF(fmt, args...)
#endif

#endif
#endif
