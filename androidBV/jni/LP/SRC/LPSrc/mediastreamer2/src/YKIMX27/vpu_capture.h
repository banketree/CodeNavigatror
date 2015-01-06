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
 * @file vpu_capture.h
 *
 * @brief This file includes capture definition for vpu test.
 *
 * @ingroup VPU
 */
#ifdef _YK_IMX27_AV_
#ifndef __VPU__CAPTURE__H
#define __VPU__CAPTURE__H

#define VOPS                    100
#define ASICRUNLENGTH           22
#define MEPG4_FILENAME             "./1.264"
#define YUV_FILENAME               "./1.yuv"

#define TEST_BUFFER_NUM 3

struct cap_testbuffer {
	unsigned char *start;
	size_t offset;
	unsigned int length;
};

struct cap_testbuffer cap_buffers[TEST_BUFFER_NUM];

int start_capturing(int fd_v4l);
void stop_capturing(int fd_v4l);
int v4l_capture_setup(int width, int height, int frame_rate);
int v4l_get_data(int fd, void *input_buf);

#endif				/* __VPU__CAPTURE__H */
#endif
