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
 * @file vpu_codec.h
 *
 * @brief This file implement codec application.
 *
 * @ingroup VPU
 */
#ifdef _YK_IMX27_AV_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <linux/videodev.h>
#include "YKIMX27Video.h"
#include "vpu_voip_test.h"
#include "vpu_display.h"
#include "vpu_capture.h"

#include "LOGIntfLayer.h"

extern struct codec_file multi_yuv[MAX_NUM_INSTANCE];
/******  Timing Stuff Begin******/
typedef enum evType_t {
	DEC_READ = 1,
	DEC_RD_OVER,
	DEC_START,
	DEC_STOP,
	DEC_OUT,
	DEC_OUT_OVER,
	ENC_READ,
	ENC_RD_OVER,
	ENC_START,
	ENC_STOP,
	ENC_OUT,
	ENC_OUT_OVER,
	TEST_BEGIN,
	TEST_END,

} evType;

#define MEASURE_COUNT 3000

typedef struct time_rec {
	evType tag;
	struct timeval tval;
} time_rec_t;

time_rec_t time_rec_vector[MEASURE_COUNT + 1];

static unsigned long iter;

unsigned int enc_core_time;
unsigned int enc_core_time_max;
unsigned int enc_core_time_min = 100000;
unsigned int enc_idle_time;
unsigned int enc_idle_time_max;
unsigned int enc_idle_time_min = 100000;

unsigned int dec_core_time;
unsigned int dec_core_time_max;
unsigned int dec_core_time_min = 100000;
unsigned int dec_idle_time;
unsigned int dec_idle_time_max;
unsigned int dec_idle_time_min = 100000;

unsigned int ttl_time = 0;

#define	DIFF_TIME_TV_SEC(i, j)	\
	(time_rec_vector[i].tval.tv_sec - time_rec_vector[j].tval.tv_sec)
#define	DIFF_TIME_TV_USEC(i, j)	\
	(time_rec_vector[i].tval.tv_usec - time_rec_vector[j].tval.tv_usec)
#define DIFF_TIME_US(i, j) (DIFF_TIME_TV_SEC(i, j) * 1000000 + DIFF_TIME_TV_USEC(i, j))

void timer(evType tag)
{
	if(iter >= MEASURE_COUNT)
		return;

	struct timeval tval;
	gettimeofday(&tval, NULL);

	time_rec_vector[iter].tag 	= tag;
	time_rec_vector[iter].tval 	= tval;
	iter++;
}

void PrintTimingData(void)
{
#ifdef DEBUG
	unsigned int i = 0;
	unsigned int j = 0, k = 0, m = 0, n = 0;
	unsigned int s1 = 0;
	unsigned int first_enc_start, last_enc_start;
	unsigned int first_dec_start, last_dec_start;
	unsigned int num_enc = 0, num_dec = 0;
	FILE *fp;

//      printf("iter=%d\n", iter);
	if ((fp = fopen("/tmp/vpu.txt", "w")) == NULL)
		printf("Unable to open log file\n");

	while (i < iter) {
		switch (time_rec_vector[i].tag) {
		case ENC_START:
			j = i;
			if (k > 0) {
				enc_idle_time += DIFF_TIME_US(i, k);
				if (enc_idle_time_max < DIFF_TIME_US(i, k))
					enc_idle_time_max = DIFF_TIME_US(i, k);
				if (enc_idle_time_min > DIFF_TIME_US(i, k))
					enc_idle_time_min = DIFF_TIME_US(i, k);

				printf("%8u\n", DIFF_TIME_US(i, k));
				fprintf(fp, "%8u\n", DIFF_TIME_US(i, k));
			} else {
				first_enc_start = i;
			}
			last_enc_start = i;

			printf("Encoding %4u : ", num_enc);
			fprintf(fp, "Encoding %4u : ", num_enc);

			break;
		case ENC_STOP:
			enc_core_time += DIFF_TIME_US(i, j);
			if (enc_core_time_max < DIFF_TIME_US(i, j))
				enc_core_time_max = DIFF_TIME_US(i, j);
			if (enc_core_time_min > DIFF_TIME_US(i, j))
				enc_core_time_min = DIFF_TIME_US(i, j);
			printf("%8u  ", DIFF_TIME_US(i, j));
			fprintf(fp, "%8u  ", DIFF_TIME_US(i, j));
			k = i;
			ttl_time = DIFF_TIME_US(i, 0);
			num_enc++;
			break;
		case DEC_START:
			m = i;
			if (n > 0) {
				dec_idle_time += DIFF_TIME_US(i, n);
				if (dec_idle_time_max < DIFF_TIME_US(i, n))
					dec_idle_time_max = DIFF_TIME_US(i, n);
				if (dec_idle_time_min > DIFF_TIME_US(i, n))
					dec_idle_time_min = DIFF_TIME_US(i, n);

				printf("%8u\n", DIFF_TIME_US(i, n));
				fprintf(fp, "%8u\n", DIFF_TIME_US(i, n));
			} else {
				first_dec_start = i;
			}
			last_dec_start = i;

			printf("Decoding %4u : ", num_dec);
			fprintf(fp, "Decoding %4u : ", num_dec);

			break;
		case DEC_STOP:
			dec_core_time += DIFF_TIME_US(i, m);
			if (dec_core_time_max < DIFF_TIME_US(i, m))
				dec_core_time_max = DIFF_TIME_US(i, m);
			if (dec_core_time_min > DIFF_TIME_US(i, m))
				dec_core_time_min = DIFF_TIME_US(i, m);
			printf("%8u  ", DIFF_TIME_US(i, m));
			fprintf(fp, "%8u  ", DIFF_TIME_US(i, m));
			n = i;
			ttl_time = DIFF_TIME_US(i, 0);
			num_dec++;
			break;
		case TEST_BEGIN:
			s1 = i;

			printf("<Test %4u :", num_dec);
			fprintf(fp, "<Test %4u :", num_dec);

			break;
		case TEST_END:
			printf("%5u >", DIFF_TIME_US(i, s1));
			fprintf(fp, "%5u >", DIFF_TIME_US(i, s1));
			break;
		}

		i++;
	}

	printf
	    ("\n\nSummary of Testing Result(also can see \"/tmp/vpu.txt\"): \n");
	fprintf(fp, "\n\nSummary of Testing Result: \n");
	if (num_enc > 0) {
		printf("Max Encoding time %6u %9u\n", enc_core_time_max,
		       enc_idle_time_max);
		fprintf(fp, "Max Encoding time %6u %9u\n", enc_core_time_max,
			enc_idle_time_max);
		printf("Min Encoding time %6u %9u\n", enc_core_time_min,
		       enc_idle_time_min);
		fprintf(fp, "Min Encoding time %6u %9u\n", enc_core_time_min,
			enc_idle_time_min);
		printf("Enc average time  %6u %9u\n", enc_core_time / num_enc,
		       enc_idle_time / num_enc);
		fprintf(fp, "Enc average time  %6u %9u\n",
			enc_core_time / num_enc, enc_idle_time / num_enc);
		printf("Enc fps = %.1f\n",
		       (float)num_enc * 1000000 / DIFF_TIME_US(last_enc_start,
							       first_enc_start));
		fprintf(fp, "Enc fps = %.1f\n",
			(float)num_enc * 1000000 / DIFF_TIME_US(last_enc_start,
								first_enc_start));
	}
	if (num_dec > 0) {
		printf("Max Decoding time %6u %9u\n", dec_core_time_max,
		       dec_idle_time_max);
		fprintf(fp, "Max Decoding time %6u %9u\n", dec_core_time_max,
			dec_idle_time_max);
		printf("Min Decoding time %6u %9u\n", dec_core_time_min,
		       dec_idle_time_min);
		fprintf(fp, "Min Decoding time %6u %9u\n", dec_core_time_min,
			dec_idle_time_min);
		printf("Dec average time  %6u %9u\n", dec_core_time / num_dec,
		       dec_idle_time / num_dec);
		fprintf(fp, "Dec average time  %6u %9u\n",
			dec_core_time / num_dec, dec_idle_time / num_dec);
		printf("Dec fps = %.1f\n",
		       (float)num_dec * 1000000 / DIFF_TIME_US(last_dec_start,
							       first_dec_start));
		fprintf(fp, "Dec fps = %.1f\n",
			(float)num_dec * 1000000 / DIFF_TIME_US(last_dec_start,
								first_dec_start));
	}

	if (num_enc > 0 && num_dec > 0) {
		printf("Average Frame Rate: %.1f\n",
		       (float)(num_enc + num_dec) * 1000 * 1000 / ttl_time / 2);
		fprintf(fp, "Average Frame Rate: %.1f\n",
			(float)(num_enc +
				num_dec) * 1000 * 1000 / ttl_time / 2);
	}

	printf("\n");
	fprintf(fp, "\n");

	fclose(fp);
#endif
	return;
}
/******  Timing Stuff End******/

extern struct codec_file multi_bitstream[MAX_NUM_INSTANCE];
#define EOSCHECK_APISCHEME

int quitflag = 0;

typedef struct codec_info {
	CodecInst handle;
	FRAME_BUF *FbPool;
	int FbNumber;
	vpu_mem_desc bitstream_buf;
} codec_info;

static struct codec_info enc_info;
static struct codec_info dec_info;

static pthread_mutex_t codec_mutex;
#ifdef INT_CALLBACK
static pthread_mutex_t vpu_busy_mutex;
static pthread_cond_t current_job_cond;
#endif

extern sigset_t mask;

static RetCode ReadBsBufHelper(EncHandle        handle,
							   int 		        src,
							   char            *src_name,
				               PhysicalAddress  paBsBufStart,
				               int              bsBufStartAddr,
				               int              bsBufEndAddr,
				               int              defaultsize,
				               int              index,
				               int 				read_flag,
				               void            *data)
{
	RetCode 	    ret 	 = RETCODE_SUCCESS;
	int 		    loadSize = 0;
	PhysicalAddress paRdPtr, paWrPtr;
	Uint32          size     = 0;

	ret = vpu_EncGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size);

	if (ret != RETCODE_SUCCESS)
	{
		printf("vpu_EncGetBitstreamBuffer failed. Error code is 0x%x \n", ret);
		goto LOAD_BS_ERROR;
	}

	DPRINTF("ReadRingBs wrsize %d | bssize %d | rdPtr 0x%x | WrPtr 0x%x\n", defaultsize, size, paRdPtr, paWrPtr);

	if (size > 0)
	{
		if (defaultsize > 0)
		{
			if (size < defaultsize)
				return RETCODE_SUCCESS;
			loadSize = defaultsize;
		}
		else
		{
			loadSize = size;
		}

		if (loadSize > 0)
		{
			ret = FillBsBufMulti(src,
				                 src_name,
					             bsBufStartAddr + paRdPtr -	paBsBufStart,
					             bsBufStartAddr,
					             bsBufEndAddr,
					             loadSize,
					             ((EncInst *) handle)->instIndex,
					             0,
					             0,
					             CODEC_WRITING,
					             data);

			ret = vpu_EncUpdateBitstreamBuffer(handle, loadSize);

			if (ret != RETCODE_SUCCESS)
			{
				printf("vpu_EncUpdateBitstreamBuffer failed. Error code is 0x%x \n", ret);
				goto LOAD_BS_ERROR;
			}
		}
	}

LOAD_BS_ERROR:

	return ret;
}

/* FrameBuffer is a round-robin buffer for Current buffer and reference */
int FrameBufferInit(int strideY, int height,
		    FRAME_BUF * FrameBuf, int FrameNumber)
{
	int i;

	for (i = 0; i < FrameNumber; i++) {
		memset(&(FrameBuf[i].CurrImage), 0, sizeof(vpu_mem_desc));
		FrameBuf[i].CurrImage.size = (strideY * height * 3 / 2);
		IOGetPhyMem(&(FrameBuf[i].CurrImage));
		if (FrameBuf[i].CurrImage.phy_addr == 0) {
			int j;
			for (j = 0; j < i; j++) {
				IOFreeVirtMem(&(FrameBuf[j].CurrImage));
				IOFreePhyMem(&(FrameBuf[j].CurrImage));
			}
			printf("No enough mem for framebuffer!\n");
			return -1;
		}
		FrameBuf[i].Index = i;
		FrameBuf[i].AddrY = FrameBuf[i].CurrImage.phy_addr;
		FrameBuf[i].AddrCb = FrameBuf[i].AddrY + strideY * height;
		FrameBuf[i].AddrCr =
		    FrameBuf[i].AddrCb + strideY / 2 * height / 2;
		FrameBuf[i].StrideY = strideY;
		FrameBuf[i].StrideC = strideY / 2;
		FrameBuf[i].DispY = FrameBuf[i].AddrY;
		FrameBuf[i].DispCb = FrameBuf[i].AddrCb;
		FrameBuf[i].DispCr = FrameBuf[i].AddrCr;
		FrameBuf[i].CurrImage.virt_uaddr =
		    IOGetVirtMem(&(FrameBuf[i].CurrImage));
	}
	return 0;
}

void FrameBufferFree(FRAME_BUF * FrameBuf, int FrameNumber)
{
	int i;

	for (i = 0; i < FrameNumber; i++)
	{
		IOFreeVirtMem(&(FrameBuf[i].CurrImage));
		IOFreePhyMem (&(FrameBuf[i].CurrImage));
	}

	return;
}

int vpu_BitstreamPad(DecHandle handle, char *wrPtr, int size)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	int i;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->openParam.bitstreamFormat == STD_AVC) {
		((unsigned int *)wrPtr)[i] = 0x0;
		i += 4;
		((unsigned int *)wrPtr)[i] = STD_AVC_ENDBUFDATA;
		i += 4;
		while (i < size) {
			((unsigned int *)wrPtr)[i] = 0x0;
			i += 4;
		}
	} else {
		if (pDecInfo->initialInfo.mp4_shortVideoHeader == 1) {
			((unsigned int *)wrPtr)[i] = 0x0;
			i += 4;
			((unsigned int *)wrPtr)[i] = STD_H263_ENDBUFDATA;
			i += 4;
			while (i < size) {
				((unsigned int *)wrPtr)[i] = 0x0;
				i += 4;
			}
		} else {
			((unsigned int *)wrPtr)[i] = 0x0;
			i += 4;
			((unsigned int *)wrPtr)[i] = STD_MP4_ENDBUFDATA;
			i += 4;
			while (i < size) {
				((unsigned int *)wrPtr)[i] = 0x0;
				i += 4;
			}
		}
	}

	return size;
}

FRAME_BUF *GetFrameBuffer(int index, FRAME_BUF * FrameBuf)
{
	return &FrameBuf[index];
}

int GetFrameBufIndex(FRAME_BUF ** bufPool, int poolSize, FrameBuffer * frame)
{
	int i;

	for (i = 0; i < poolSize; ++i) {
		if ((*bufPool)->AddrY == frame->bufY) {
			return i;
		}
		bufPool++;
	}
	return i;
}

#ifdef INT_CALLBACK
void vpu_test_callback(int status)
{
#if 1
	if (status & IRQ_PIC_RUN) {
#else
	if ((status & IRQ_PIC_RUN) || (status & IRQ_DEC_BUF_EMPTY)) {
#endif
		pthread_mutex_lock(&vpu_busy_mutex);
		pthread_cond_signal(&current_job_cond);
		pthread_mutex_unlock(&vpu_busy_mutex);
	}

	return;
}
#endif

int vpu_SystemInit(void)
{
	int ret = -1;
	pthread_mutex_init(&codec_mutex, NULL);
#ifdef INT_CALLBACK
	pthread_mutex_init(&vpu_busy_mutex, NULL);
	pthread_cond_init(&current_job_cond, NULL);

	ret = IOSystemInit((void *)(vpu_test_callback));
#else
	ret = IOSystemInit(NULL);
#endif
	if (ret < 0) {
		//printf("IO system init failed!\n");
		LOG_RUNLOG_ERROR("LP IMX27 IO system init failed");
		LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_REBOOT, 0, "");
		LOGUploadResPrepross();
		system("reboot");
		return -1;
	}
	return ret;
}

int vpu_SystemShutdown(void)
{
	if (enc_info.FbNumber) {
		vpu_EncClose(&(enc_info.handle));
		FrameBufferFree(enc_info.FbPool, enc_info.FbNumber);
		IOFreeVirtMem(&(enc_info.bitstream_buf));
		IOFreePhyMem(&(enc_info.bitstream_buf));
		memset(&enc_info, 0, sizeof(struct codec_info));
	}
	if (dec_info.FbNumber) {
		vpu_DecClose(&(dec_info.handle));
		FrameBufferFree(dec_info.FbPool, dec_info.FbNumber);
		IOFreeVirtMem(&(dec_info.bitstream_buf));
		IOFreePhyMem(&(dec_info.bitstream_buf));
		memset(&dec_info, 0, sizeof(struct codec_info));
	}
	IOSystemShutdown();
	PrintTimingData();
	DPRINTF("file close end\n");

	return 0;
}

/* To supporte multi-instance, Rd & Wr Ptr should be assigned to different regs.*/
/* This Decode Test doesn't do the rotation
 * and no reconstruction buffer is needed*/
/*
void *DecodeTest(void *param)
{
	const int extra_fb = 1;
	struct codec_config *usr_config = (struct codec_config *)param;
	DecHandle handle = { 0 };
	DecOpenParam decOP = { 0 };
	DecInitialInfo initialInfo = { 0 };
	DecOutputInfo outputInfo = { 0 };
	DecParam decParam = { 0 };
	RetCode ret;
	PhysicalAddress paWrPtr;
	PhysicalAddress paRdPtr;
	Uint32 vaBsBufStart;
	Uint32 vaBsBufEnd;
	int frameIdx = 0, totalNumofErrMbs = 0, fillendBs = 0, decodefinish = 0;
	int streameof = 0;
	int checkeof = 0;
	Uint32 size = 0;
	Uint32 fRateInfo = 0;
	Uint32 framebufWidth = 0, framebufHeight = 0;
	int needFrameBufCount = 0;
	FRAME_BUF FrameBufPool[MAX_FRAME];

	FRAME_BUF *pFrame[NUM_FRAME_BUF];
	FrameBuffer frameBuf[NUM_FRAME_BUF];

	checkeof = 0;

	memset(&dec_info, 0, sizeof(struct codec_info));
	int i;
	struct v4l2_buffer pp_buffer;
	int output_ratio;
	int image_size;
	int rot_en = 0;
	int rot_angle;
	int mirror_dir;
	int rotStride;
	int storeImage;

	switch (usr_config->mode) {
	case 0:
		decOP.bitstreamFormat = STD_MPEG4;
		DPRINTF("Dec mode: MPEG4\n");
		break;
	case 1:
		decOP.bitstreamFormat = STD_H263;
		DPRINTF("Dec mode: H.263\n");
		break;
	case 2:
		decOP.bitstreamFormat = STD_AVC;
		DPRINTF("Dec mode: H.264\n");
		break;
	default:
		printf("Unknown Dec mode\n");
		return NULL;
	}
	rot_angle = usr_config->rot_angle;
	mirror_dir = usr_config->mirror_angle;

	pthread_mutex_lock(&codec_mutex);

	vpu_mem_desc bit_stream_buf;
	memset(&bit_stream_buf, 0, sizeof(vpu_mem_desc));
	bit_stream_buf.size = STREAM_BUF_SIZE;
	IOGetPhyMem(&bit_stream_buf);
	int virt_bit_stream_buf = IOGetVirtMem(&bit_stream_buf);
	decOP.bitstreamBuffer = bit_stream_buf.phy_addr;
	decOP.bitstreamBufferSize = STREAM_BUF_SIZE;
	vaBsBufStart = virt_bit_stream_buf;
	vaBsBufEnd = vaBsBufStart + decOP.bitstreamBufferSize;
	decOP.qpReport = 0;

	decOP.reorderEnable = 0;

	// Get buffer position and Fill Buffer
	ret = vpu_DecOpen(&handle, &decOP);
	if (ret != RETCODE_SUCCESS) {
		printf("vpu_DecOpen failed. Error code is %d \n", ret);
		goto ERR_DEC_INIT;
	}

	ret = vpu_DecGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size);
	if (ret != RETCODE_SUCCESS) {
		printf("vpu_DecGetBitstreamBuffer failed. Error code is %d \n",
		       ret);
		goto ERR_DEC_OPEN;
	}

	pthread_mutex_unlock(&codec_mutex);
	ret = FillBsBufMulti(usr_config->src, usr_config->src_name,
			     virt_bit_stream_buf + paWrPtr -
			     bit_stream_buf.phy_addr, vaBsBufStart,
			     vaBsBufEnd, STREAM_FILL_SIZE,
			     ((DecInst *) handle)->instIndex, checkeof,
			     &streameof, CODEC_READING);
	if (ret < 0) {
		printf("FillBsBufMulti failed. Error code is %d\n", ret);
		vpu_SystemShutdown();
		return &codec_thread[usr_config->index];
	}
	pthread_mutex_lock(&codec_mutex);

	if (usr_config->src != PATH_NET) {
		ret = vpu_DecUpdateBitstreamBuffer(handle, STREAM_FILL_SIZE);
	} else {
		ret = vpu_DecUpdateBitstreamBuffer(handle, ret);
	}

	if (ret != RETCODE_SUCCESS) {
		printf
		    ("vpu_DecUpdateBitstreamBuffer failed. Error code is %d \n",
		     ret);
		goto ERR_DEC_OPEN;
	}

	vpu_DecSetEscSeqInit(handle, 1);

	// Parse bitstream and get width/height/framerate/h264/mp4 info etc.
	ret = vpu_DecGetInitialInfo(handle, &initialInfo);
	if (ret != RETCODE_SUCCESS) {
		printf
		    ("vpu_DecGetInitialInfo failed. Error code is %d \n",
		     ret);
		goto ERR_DEC_OPEN;
	}

	vpu_DecSetEscSeqInit(handle, 0);

	DPRINTF("Dec: min buffer count= %d\n", initialInfo.minFrameBufferCount);
	fRateInfo = initialInfo.frameRateInfo;

	DPRINTF
	    ("Dec InitialInfo =>\npicWidth: %u, picHeight: %u, frameRate: %u\n",
	     initialInfo.picWidth, initialInfo.picHeight,
	     (unsigned int)initialInfo.frameRateInfo);

	framebufWidth = ((initialInfo.picWidth + 15) & ~15);
	framebufHeight = ((initialInfo.picHeight + 15) & ~15);

	if ((usr_config->dst != PATH_EMMA) || (rot_angle != 0) || (mirror_dir != 0)) {
		// Frame Buffer Pool needed is the minFrameBufferCount +1 loopfilter,
		//  if rotation yes, should also add 1
		ret = FrameBufferInit(initialInfo.picWidth, initialInfo.picHeight,
				      FrameBufPool,
				      initialInfo.minFrameBufferCount + extra_fb);
		if (ret < 0) {
			printf("Mem system allocation failed!\n");
			return &codec_thread[usr_config->index];
		}
	}

	dec_info.FbPool = FrameBufPool;
	dec_info.FbNumber = initialInfo.minFrameBufferCount + extra_fb;
	needFrameBufCount = dec_info.FbNumber;
	memcpy(&(dec_info.bitstream_buf), &bit_stream_buf,
	       sizeof(vpu_mem_desc));
	memcpy(&(dec_info.handle), &handle, sizeof(DecHandle));

	if ((usr_config->dst != PATH_EMMA) ||
	    ((usr_config->dst == PATH_EMMA) && ((rot_angle != 0) || (mirror_dir != 0)))) {
		// Get the image buffer
		for (i = 0; i < needFrameBufCount; ++i) {
			pFrame[i] = GetFrameBuffer(i, FrameBufPool);
			frameBuf[i].bufY = pFrame[i]->AddrY;
			frameBuf[i].bufCb = pFrame[i]->AddrCb;
			frameBuf[i].bufCr = pFrame[i]->AddrCr;
		}
	}

	memset(&pp_buffer, 0, sizeof(struct v4l2_buffer));
	image_size = initialInfo.picWidth * initialInfo.picHeight;

	output_ratio = usr_config->out_ratio;

	if ((usr_config->dst == PATH_EMMA) && (rot_angle == 0) && (mirror_dir == 0)) {
		if (!multi_yuv[((DecInst *) handle)->instIndex].fd) {
			multi_yuv[((DecInst *) handle)->instIndex].fd = (FILE *)
			    emma_dev_open(initialInfo.picWidth, initialInfo.picHeight, CODEC_WRITING,
					  output_ratio, initialInfo.minFrameBufferCount);

		}

		for (i = 0; i < initialInfo.minFrameBufferCount; ++i) {
			frameBuf[i].bufY = buffers[i].offset;
			frameBuf[i].bufCb = frameBuf[i].bufY + image_size;
			frameBuf[i].bufCr =
			    frameBuf[i].bufCb + (image_size >> 2);
		}
	}

	// Set the frame buffer array to the structure
	ret = vpu_DecRegisterFrameBuffer(handle, frameBuf,
					 initialInfo.minFrameBufferCount,
					 initialInfo.picWidth);
	if (ret != RETCODE_SUCCESS) {
		printf("vpu_DecRegisterFrameBuffer failed. Error code is %d \n",
		       ret);
		goto ERR_DEC_OPEN;
	}
#ifdef EOSCHECK_APISCHEME
	decParam.dispReorderBuf = 0;
	decParam.prescanEnable = 1;
	decParam.prescanMode = 0;
#else
	decParam.dispReorderBuf = 0;
	decParam.prescanEnable = 0;
	decParam.prescanMode = 0;
#endif
	decParam.skipframeMode = 0;
	decParam.skipframeNum = 0;
	decParam.iframeSearchEnable = 0;


	pthread_mutex_unlock(&codec_mutex);

	FRAME_BUF *pDispFrame;
	FrameBuffer *emma_buffer;

	if ((usr_config->dst == PATH_EMMA) && ((rot_angle != 0) || (mirror_dir != 0))) {
		emma_buffer = &frameBuf[initialInfo.minFrameBufferCount + 1];
	}

	rot_en = (rot_angle == 90 || rot_angle == 270) ? 1 : 0;
	if ((usr_config->dst != PATH_EMMA) ||
	    ((usr_config->dst == PATH_EMMA) && ((rot_angle != 0) || (mirror_dir != 0)))) {
		vpu_DecGiveCommand(handle, SET_ROTATION_ANGLE, &rot_angle);
	}
	//vpu_DecGiveCommand(handle, SET_MIRROR_DIRECTION, &mirror_dir);
	storeImage = 1;
	rotStride = (rot_angle == 90 || rot_angle == 270) ?
	    framebufHeight : framebufWidth;
	rotStride = storeImage ? rotStride : STRIDE;
	if ((usr_config->dst != PATH_EMMA) ||
	    ((usr_config->dst == PATH_EMMA) && ((rot_angle != 0) || (mirror_dir != 0)))) {
		vpu_DecGiveCommand(handle, SET_ROTATOR_STRIDE, &rotStride);
	}

	while (1) {
		if (frameIdx % 100 == 0)
			DPRINTF(" Inst %d, No. %d\n",
				((DecInst *) handle)->instIndex, frameIdx);

#if 0
		ret = vpu_DecBitBufferFlush(handle);
		if( ret != RETCODE_SUCCESS )
		{
			printf("vpu_DecGetBitstreamBuffer failed Error code is 0x%x \n", ret);
			goto ERR_DEC_OPEN;
		}
#endif

//		printf("frameIdx = %d\n", frameIdx);
#ifdef EOSCHECK_APISCHEME
		if (decOP.reorderEnable == 1 &&
		    decOP.bitstreamFormat == STD_AVC) {
			static int prescanEnable;
			if (frameIdx == 0) {
				prescanEnable = decParam.prescanEnable;
				decParam.prescanEnable = 0;
			} else if (frameIdx == 1)
				decParam.prescanEnable = prescanEnable;
		}
#endif

		pthread_mutex_lock(&codec_mutex);

		if ((usr_config->dst == PATH_EMMA) && ((rot_angle != 0) || (mirror_dir != 0))) {
			vpu_DecGiveCommand(handle, SET_ROTATOR_OUTPUT,
					   (void *)emma_buffer);
			if (frameIdx == 0) {
				vpu_DecGiveCommand(handle, ENABLE_ROTATION, 0);
				//vpu_DecGiveCommand(handle, ENABLE_MIRRORING, 0);
			}
		}

		if ((usr_config->dst == PATH_EMMA) && (rot_angle == 0) && (mirror_dir == 0)) {
			FillYuvImageMulti(usr_config->dst, usr_config->dst_name,
						0,
					  (void *)&pp_buffer, initialInfo.picWidth,
					  initialInfo.picHeight,
					  ((DecInst *) handle)->instIndex,
					  CODEC_WRITING, rot_en, output_ratio, 0);
		}

		// To fix the MPEG4 issue on MX27 TO2
		CodStd codStd = ((DecInst *) handle)->CodecInfo.decInfo.openParam.bitstreamFormat;
		DPRINTF("CodStd = %d\n", codStd);
		if (codStd == STD_MPEG4) {
			vpu_ESDMISC_LHD(1);
		}
		// Start decoding a frame.
		ret = vpu_DecStartOneFrame(handle, &decParam);
		if (ret != RETCODE_SUCCESS) {
			printf
			    ("vpu_DecStartOneFrame failed. Error code is %d \n",
			     ret);
			pthread_mutex_unlock(&codec_mutex);
			goto ERR_DEC_OPEN;
		}

		// fill the bitstream buffer while the system is busy
		timer(DEC_START);
#ifdef INT_CALLBACK
		pthread_mutex_lock(&vpu_busy_mutex);
		pthread_cond_wait(&current_job_cond, &vpu_busy_mutex);
		pthread_mutex_unlock(&vpu_busy_mutex);
#else
		vpu_WaitForInt(200);
#endif
		timer(DEC_STOP);

		if (quitflag) {
			pthread_mutex_unlock(&codec_mutex);
			break;
		}

		//
		// vpu_DecGetOutputInfo() should match vpu_DecStartOneFrame() with
		// the same handle. No other API functions can intervene between these two
		// functions, except for vpu_IsBusy(), vpu_DecGetBistreamBuffer(),
		// and vpu_DecUpdateBitstreamBuffer().
		//
		ret = vpu_DecGetOutputInfo(handle, &outputInfo);
		if (ret != RETCODE_SUCCESS) {
			printf
			    ("vpu_DecGetOutputInfo failed. Error code is %d \n",
			     ret);
		}
//              printf("output index = %d\n", outputInfo.indexDecoded);
		if (outputInfo.indexDecoded == -1 || outputInfo.indexDecoded > needFrameBufCount)
			decodefinish = 1;

		ret =
		    vpu_DecGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr,
					      &size);

		// To fix the MPEG4 issue on MX27 TO2
		if (codStd == STD_MPEG4) {
			vpu_ESDMISC_LHD(0);
		}
		pthread_mutex_unlock(&codec_mutex);

		if (size >= STREAM_FILL_SIZE) {
			// will the speed match with the bitprocessor?
			ret =
			    FillBsBufMulti(usr_config->src,
					   usr_config->src_name,
					   virt_bit_stream_buf + paWrPtr -
					   bit_stream_buf.phy_addr,
					   vaBsBufStart, vaBsBufEnd,
					   STREAM_FILL_SIZE,
					   ((DecInst *) handle)->instIndex,
					   checkeof, &streameof, CODEC_READING);
			if (ret < 0) {
				if (ferror
				    (multi_bitstream
				     [((DecInst *) handle)->instIndex].fd)) {
					printf
					    ("FillBsBufMulti failed. Error code is %d \n",
					     ret);
					goto ERR_DEC_OPEN;
				}
			}

			if (usr_config->src == PATH_NET) {
				ret = vpu_DecUpdateBitstreamBuffer(handle,
								   ret);
				if (ret != RETCODE_SUCCESS) {
					printf
					    ("vpu_DecUpdateBitstreamBuffer failed. Error code is %d \n",
					     ret);
					goto ERR_DEC_OPEN;
				}
				goto NET_JUMP;
			}

#ifdef EOSCHECK_APISCHEME
			if (streameof == 0) {
				ret = vpu_DecUpdateBitstreamBuffer(handle,
								   STREAM_FILL_SIZE);
				if (ret != RETCODE_SUCCESS) {
					printf
					    ("vpu_DecUpdateBitstreamBuffer failed. Error code is %d \n",
					     ret);
					goto ERR_DEC_OPEN;
				}

			} else {
				if (!fillendBs) {
					vpu_BitstreamPad(handle,
							 (char *)(virt_bit_stream_buf +
							 paWrPtr -
							 bit_stream_buf.
							 phy_addr),
							 STREAM_END_SIZE);
					ret =
					    vpu_DecUpdateBitstreamBuffer(handle,
									 STREAM_END_SIZE);
					if (ret != RETCODE_SUCCESS) {
						printf
						    ("vpu_DecUpdateBitstreamBuffer failed. Error code is %d \n",
						     ret);
						goto ERR_DEC_OPEN;
					}
					fillendBs = 1;
				}
			}
#else
			if (streameof == 0) {
				ret = vpu_DecUpdateBitstreamBuffer(handle,
								   STREAM_FILL_SIZE);
				if (ret != RETCODE_SUCCESS) {
					printf
					    ("vpu_DecUpdateBitstreamBuffer failed. Error code is %d \n",
					     ret);
					goto ERR_DEC_OPEN;
				}
			}
#endif
		}

NET_JUMP:
#ifdef EOSCHECK_APISCHEME
		if (outputInfo.prescanresult == 0 &&
		    decParam.dispReorderBuf == 0) {
			if (streameof) {
				if (decOP.reorderEnable == 1 &&
				    decOP.bitstreamFormat == STD_AVC &&
				    (outputInfo.indexDecoded != -1)) {
					decParam.dispReorderBuf = 1;
					continue;
				} else {
					decodefinish = 1;
				}
			} else {	// not enough bitstream data
				printf("PreScan Result: No enough bitstream data.\n");
				continue;
			}
		}
#endif

		if (decodefinish)
			break;

		if (outputInfo.indexDecoded == -2) // BIT don't have picture to be displayed
			continue;

		pDispFrame =
		    GetFrameBuffer(outputInfo.indexDecoded, FrameBufPool);

		if ((usr_config->dst != PATH_EMMA) ||
		    ((usr_config->dst == PATH_EMMA) && ((rot_angle != 0) || (mirror_dir != 0)))) {
			FillYuvImageMulti(usr_config->dst, usr_config->dst_name,
					  pDispFrame->AddrY +
					  pDispFrame->CurrImage.virt_uaddr -
					  pDispFrame->CurrImage.phy_addr,
					  (void *)&pp_buffer, initialInfo.picWidth,
					  initialInfo.picHeight,
					  ((DecInst *) handle)->instIndex,
					  CODEC_WRITING, rot_en, output_ratio, initialInfo.minFrameBufferCount - 1);
		}

		if ((usr_config->dst == PATH_EMMA) && ((rot_angle != 0) || (mirror_dir != 0))) {
			emma_buffer->bufY = buffers[pp_buffer.index].offset;
			emma_buffer->bufCb = emma_buffer->bufY + image_size;
			emma_buffer->bufCr =
			    emma_buffer->bufCb + (image_size >> 2);
		}

		if (decOP.qpReport == 1)
			SaveQpReport(outputInfo.qpInfo, initialInfo.picWidth,
				initialInfo.picHeight, frameIdx, "decqpreport.dat" );

		if (outputInfo.numOfErrMBs)
		{
			totalNumofErrMbs += outputInfo.numOfErrMBs;
			printf("Num of Error Mbs : %d, in Frame : %d \n", outputInfo.numOfErrMBs, frameIdx);
		}

		frameIdx++;
	}

	if (totalNumofErrMbs) {
		printf("Total Num of Error MBs : %d\n", totalNumofErrMbs);
	}

      ERR_DEC_OPEN:
	pthread_mutex_lock(&codec_mutex);
	vpu_DecClose(handle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE) {
		vpu_DecGetOutputInfo(handle, &outputInfo);
		vpu_DecClose(handle);
	}
	pthread_mutex_unlock(&codec_mutex);

      ERR_DEC_INIT:
	FrameBufferFree(FrameBufPool,
			initialInfo.minFrameBufferCount + extra_fb);
	IOFreeVirtMem(&bit_stream_buf);
	IOFreePhyMem(&bit_stream_buf);
	memset(&dec_info, 0, sizeof(struct codec_info));
	DPRINTF("Dec closed\n");

	return &codec_thread[usr_config->index];
}
*/
/* for this test, only 1 reference*/
/* no rotation*/

extern unsigned int PS_Send_Count;

void *VIMX27Processthread(void *data)
{
	VIMX27State  *s    	= (VIMX27State *)data;

	PS_Send_Count = 0;

	EncHandle 		handle 			= { 0 };
	EncOpenParam 	encOP 			= { 0 };
	EncParam 		encParam 		= { 0 };
	EncInitialInfo 	initialInfo 	= { 0 };
	EncOutputInfo 	outputInfo 		= { 0 };
	SearchRamParam 	searchPa 		= { 0 };
	RetCode 		ret 			= RETCODE_SUCCESS;
	EncHeaderParam 	encHeaderParam 	= { 0 };
	int 			saveEncHeader 	= 1;

#if STREAM_ENC_PIC_RESET == 0
	PhysicalAddress 	paBsBufStart, paBsBufEnd;
	Uint32 				vaBsBufStart, vaBsBufEnd;
#else
	PhysicalAddress 	bsBuf0;
	Uint32 				size0;
#endif

	int 	srcFrameIdx;
	int 	extra_fb 		= 1;
	int 	stride 			= 0;
	Uint32 	framebufWidth 	= 0, framebufHeight = 0;

	//FMO(Flexible macroblock ordering) support
	int 	fmoEnable 		= 0;
	int 	fmoSliceNum 	= 2;
	int 	fmoType 		= 0;	//0 - interleaved, or 1 - dispersed

	int 	mode;
	struct 	codec_config 	*usr_config = &s->codec_param;
	int 	picWidth 		= usr_config->width;
	int 	picHeight 		= usr_config->height;
	int     fps             = usr_config->fps;       //M by lcd
	int 	bitRate 		= usr_config->bps;
	int     gopsize         = usr_config->gop;       //M by lcd
	int     mbcount         = usr_config->frame_count;//M by lcd

	switch (usr_config->mode)
	{
		case 0:
			mode = STD_MPEG4;
			printf("Enc mode: MPEG4\n");
			break;
		case 1:
			mode = STD_H263;
			printf("Enc mode: H.263\n");
			break;
		case 2:
			mode = STD_AVC;
			printf("Enc mode: H.264\n");
			break;
		default:
			printf("Unknown Enc mode\n");
			return NULL;
	}

	printf("Width = %d, Height = %d, Bitrate = %d Kbps\n", picWidth, picHeight, bitRate);

	memset(&enc_info, 0, sizeof(struct codec_info));

	FRAME_BUF 	*pFrame[NUM_FRAME_BUF];
	FrameBuffer  frameBuf[NUM_FRAME_BUF];
	FRAME_BUF 	 FrameBufPool[MAX_FRAME];
	int 		 i;
	int 		 frameIdx;
	int 		 rot_en = 0;
	int 		 output_ratio = 4;
	struct v4l2_buffer prp_buffer;
	int 		 image_size;
	//allocate the bit stream buffer

	vpu_mem_desc bit_stream_buf;

	memset(&bit_stream_buf, 0, sizeof(vpu_mem_desc));

	bit_stream_buf.size = STREAM_BUF_SIZE;

	pthread_mutex_lock(&codec_mutex);

	IOGetPhyMem(&bit_stream_buf);

	int virt_bit_stream_buf = IOGetVirtMem(&bit_stream_buf);

	pthread_mutex_unlock(&codec_mutex);


	encOP.bitstreamBuffer 		= bit_stream_buf.phy_addr;
	encOP.bitstreamBufferSize 	= STREAM_BUF_SIZE;

#if STREAM_ENC_PIC_RESET == 0
	paBsBufStart 	= encOP.bitstreamBuffer;
	paBsBufEnd 		= encOP.bitstreamBuffer + encOP.bitstreamBufferSize;
	vaBsBufStart 	= virt_bit_stream_buf;
	vaBsBufEnd 		= vaBsBufStart + encOP.bitstreamBufferSize;
#endif

	//framebufWidth and framebufHeight must be a multiple of 16
	framebufWidth 	= ((picWidth + 15) & ~15);
	framebufHeight 	= ((picHeight + 15) & ~15);

	encOP.bitstreamFormat 	= mode;
	encOP.picWidth 			= picWidth;
	encOP.picHeight 		= picHeight;
	//encOP.frameRateInfo 	= 30;        //the origin
	encOP.frameRateInfo 	= fps;       //M by lcd

	encOP.bitRate 		    = bitRate;   //the origin
	//encOP.bitRate 		= 0;         //M by lcd

	encOP.initialDelay 		= 0;
	encOP.vbvBufferSize 	= 0;		 //0 = ignore 8
	encOP.enableAutoSkip 	= 0;

	encOP.gopSize 			= gopsize;	 //
	//encOP.gopSize 		= 1;		 //
	//encOP.gopSize 		= 2;

	//encOP.sliceMode 		= 0;		 //1 slice per picture
	encOP.sliceMode 		= 1;         //M by lcd
	//encOP.sliceSizeMode 	= 0;
	encOP.sliceSizeMode 	= 1;         //M by lcd

	//encOP.sliceSize 		= 4000;		 //not used if sliceMode is 0
	encOP.sliceSize 		= mbcount;   //M by lcd

	encOP.intraRefresh 		= 0;
	encOP.sliceReport 		= 0;
	encOP.mbReport 			= 0;
	encOP.mbQpReport 		= 0;

	if (mode == STD_MPEG4)
	{
		encOP.EncStdParam.mp4Param.mp4_dataPartitionEnable 	= 0;
		encOP.EncStdParam.mp4Param.mp4_reversibleVlcEnable 	= 0;
		encOP.EncStdParam.mp4Param.mp4_intraDcVlcThr 		= 0;
	}
	else if (mode == STD_H263)
	{
		encOP.EncStdParam.h263Param.h263_annexJEnable 		= 0;
		encOP.EncStdParam.h263Param.h263_annexKEnable 		= 0;
		encOP.EncStdParam.h263Param.h263_annexTEnable 		= 0;

		if (encOP.EncStdParam.h263Param.h263_annexJEnable == 0 &&
		    encOP.EncStdParam.h263Param.h263_annexKEnable == 0 &&
			encOP.EncStdParam.h263Param.h263_annexTEnable == 0 )
		{
			encOP.frameRateInfo = 0x3E87530;
		}
	}
	else if (mode == STD_AVC)
	{
		encOP.EncStdParam.avcParam.avc_constrainedIntraPredFlag = 0;
		encOP.EncStdParam.avcParam.avc_disableDeblk 			= 0;
		encOP.EncStdParam.avcParam.avc_deblkFilterOffsetAlpha 	= 0;
		encOP.EncStdParam.avcParam.avc_deblkFilterOffsetBeta 	= 0;
		encOP.EncStdParam.avcParam.avc_chromaQpOffset 			= 0;
		encOP.EncStdParam.avcParam.avc_audEnable 				= 0;
		encOP.EncStdParam.avcParam.avc_fmoEnable 				= fmoEnable;
		encOP.EncStdParam.avcParam.avc_fmoType 					= fmoType;
		encOP.EncStdParam.avcParam.avc_fmoSliceNum 				= fmoSliceNum;
	}
	else
	{
		printf("Encoder: Invalid codec standard mode \n");
		return NULL;
	}

	pthread_mutex_lock(&codec_mutex);

	ret = vpu_EncOpen(&handle, &encOP);

	if (ret != RETCODE_SUCCESS)
	{
		printf("vpu_EncOpen failed. Error code is %d \n", ret);

		goto ERR_ENC_INIT;
	}

	searchPa.searchRamAddr = DEFAULT_SEARCHRAM_ADDR;

	ret = vpu_EncGiveCommand(handle, ENC_SET_SEARCHRAM_PARAM, &searchPa);

	if (ret != RETCODE_SUCCESS)
	{
		printf("vpu_EncGiveCommand ( ENC_SET_SEARCHRAM_PARAM ) failed. Error code is %d \n",ret);
		goto ERR_ENC_OPEN;
	}

	ret = vpu_EncGetInitialInfo(handle, &initialInfo);

	if (ret != RETCODE_SUCCESS)
	{
		printf("vpu_EncGetInitialInfo failed. Error code is %d \n",ret);

		goto ERR_ENC_OPEN;
	}

	DPRINTF("Enc: min buffer count= %d\n", initialInfo.minFrameBufferCount);

	if (saveEncHeader == 1)
	{
		if (mode == STD_MPEG4)
		{
			SaveGetEncodeHeader(handle, ENC_GET_VOS_HEADER,  "mp4_vos_header.dat");
			SaveGetEncodeHeader(handle, ENC_GET_VO_HEADER,   "mp4_vo_header.dat");
			SaveGetEncodeHeader(handle, ENC_GET_VOL_HEADER,  "mp4_vol_header.dat");
		}
		else if (mode == STD_AVC)
		{
			SaveGetEncodeHeader(handle, ENC_GET_SPS_RBSP, "avc_sps_rbsp_header.dat");
			SaveGetEncodeHeader(handle, ENC_GET_PPS_RBSP, "avc_pps_rbsp_header.dat");
		}
	}

	//allocate the image buffer, rec buffer/ref buffer plus src buffer
	FrameBufferInit(picWidth, picHeight,FrameBufPool,initialInfo.minFrameBufferCount + extra_fb);

	enc_info.FbPool 	= FrameBufPool;
	enc_info.FbNumber 	= initialInfo.minFrameBufferCount + extra_fb;

	memcpy(&(enc_info.bitstream_buf), &bit_stream_buf,sizeof(struct vpu_mem_desc));
	memcpy(&(enc_info.handle), &handle, sizeof(DecHandle));

	srcFrameIdx = initialInfo.minFrameBufferCount;

	for (i = 0; i < initialInfo.minFrameBufferCount + extra_fb; ++i)
	{
		pFrame[i] 			= GetFrameBuffer(i, FrameBufPool);
		frameBuf[i].bufY 	= pFrame[i]->AddrY;
		frameBuf[i].bufCb 	= pFrame[i]->AddrCb;
		frameBuf[i].bufCr 	= pFrame[i]->AddrCr;
	}

	stride 	= framebufWidth;

	ret 	= vpu_EncRegisterFrameBuffer(handle, frameBuf,initialInfo.minFrameBufferCount,stride);

	if (ret != RETCODE_SUCCESS)
	{
		printf("vpu_EncRegisterFrameBuffer failed.Error code is %d \n",ret);

		goto ERR_ENC_OPEN;
	}

	pthread_mutex_unlock(&codec_mutex);

	printf("Disp %x, %x, %x,\n\tStore buf %x, %x, %x\n",
			pFrame[srcFrameIdx]->DispY,
			pFrame[srcFrameIdx]->DispCb,
			pFrame[srcFrameIdx]->DispCr,
		    (unsigned int)frameBuf[srcFrameIdx].bufY,
			(unsigned int)frameBuf[srcFrameIdx].bufCb,
			(unsigned int)frameBuf[srcFrameIdx].bufCr);
	frameIdx 				= 0;
	encParam.sourceFrame 	= &frameBuf[srcFrameIdx];
	encParam.quantParam 	= 30;
	encParam.forceIPicture 	= 0;
	encParam.skipPicture 	= 0;

	image_size = picWidth * picHeight;
	memset(&prp_buffer, 0, sizeof(struct v4l2_buffer));

	//Must put encode header before first picture encoding.
	if (mode == STD_MPEG4)
	{/*
		encHeaderParam.headerType = VOS_HEADER;
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &encHeaderParam);
#if STREAM_ENC_PIC_RESET == 1
		//ReadBsResetBufHelper(bsFp, encHeaderParam.buf, encHeaderParam.size);
		bsBuf0 	= encHeaderParam.buf;
		size0 	= encHeaderParam.size;
		DPRINTF("MPEG4 encHeaderParam(VOS_HEADER) addr:%p len:%d\n", bsBuf0, size0);
		ret = FillBsBufMulti(usr_config->dst, usr_config->dst_name,
				     virt_bit_stream_buf + bsBuf0 -
				     bit_stream_buf.phy_addr, 0,
				     0, size0,
				     ((EncInst *) handle)->instIndex,
				     0, 0, CODEC_WRITING);
#endif
		encHeaderParam.headerType = VIS_HEADER;
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &encHeaderParam);
#if STREAM_ENC_PIC_RESET == 1
		//ReadBsResetBufHelper(bsFp, encHeaderParam.buf, encHeaderParam.size);
		bsBuf0 = encHeaderParam.buf;
		size0 = encHeaderParam.size;
		DPRINTF("MPEG4 encHeaderParam(VIS_HEADER) addr:%p len:%d\n", bsBuf0, size0);
		ret = FillBsBufMulti(usr_config->dst, usr_config->dst_name,
				     virt_bit_stream_buf + bsBuf0 -
				     bit_stream_buf.phy_addr, 0,
				     0, size0,
				     ((EncInst *) handle)->instIndex,
				     0, 0, CODEC_WRITING);
#endif
		encHeaderParam.headerType = VOL_HEADER;
		vpu_EncGiveCommand(handle, ENC_PUT_MP4_HEADER, &encHeaderParam);
#if STREAM_ENC_PIC_RESET == 1
		//ReadBsResetBufHelper(bsFp, encHeaderParam.buf, encHeaderParam.size);
		bsBuf0 = encHeaderParam.buf;
		size0 = encHeaderParam.size;
		DPRINTF("MPEG4 encHeaderParam(VOL_HEADER) addr:%p len:%d\n", bsBuf0, size0);
		ret = FillBsBufMulti(usr_config->dst, usr_config->dst_name,
				     virt_bit_stream_buf + bsBuf0 -
				     bit_stream_buf.phy_addr, 0,
				     0, size0,
				     ((EncInst *) handle)->instIndex,
				     0, 0, CODEC_WRITING);
#endif
*/
	}
	else if (mode == STD_AVC)
	{
		encHeaderParam.headerType = SPS_RBSP;

		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &encHeaderParam);

#if STREAM_ENC_PIC_RESET == 1
		//ReadBsResetBufHelper(bsFp, encHeaderParam.buf, encHeaderParam.size);

		bsBuf0 = encHeaderParam.buf;
		size0  = encHeaderParam.size;
		DPRINTF("AVC encHeaderParam(SPS_RBSP) addr:%p len:%d\n", bsBuf0, size0);

		ret = FillBsBufMulti(usr_config->dst,
			                 usr_config->dst_name,
				             virt_bit_stream_buf + bsBuf0 - bit_stream_buf.phy_addr,
				             0,
				             0,
				             size0,
				             ((EncInst *) handle)->instIndex,
				             0,
				             0,
				             CODEC_WRITING,
				             data);
#endif
		encHeaderParam.headerType = PPS_RBSP;
		vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &encHeaderParam);

#if STREAM_ENC_PIC_RESET == 1
		//ReadBsResetBufHelper(bsFp, encHeaderParam.buf, encHeaderParam.size);
		bsBuf0 = encHeaderParam.buf;
		size0 = encHeaderParam.size;
		DPRINTF("AVC encHeaderParam(PPS_RBSP) addr:%p len:%d\n", bsBuf0, size0);
		ret = FillBsBufMulti(usr_config->dst,
			                 usr_config->dst_name,
				             virt_bit_stream_buf + bsBuf0 - bit_stream_buf.phy_addr, 0,
				             0,
				             size0,
				             ((EncInst *) handle)->instIndex,
				             0,
				             0,
				             CODEC_WRITING,
				             data);
#endif
	}


	while (s->bCaptureStart)
	{
		if (frameIdx % 100 == 0)
		{
			//printf(" Inst %d, No. %d\n",((EncInst *) handle)->instIndex, frameIdx);
		}

		if (usr_config->dst == PATH_NET)
		{
			if (frameIdx % 5 == 0)
			{
				encParam.forceIPicture = 1;
			}
			else
			{
				encParam.forceIPicture = 0;
			}
		}

		ret = FillYuvImageMulti(usr_config->src,
			                    usr_config->src_name,
				      			frameBuf[srcFrameIdx].bufY + FrameBufPool[srcFrameIdx].CurrImage.virt_uaddr - FrameBufPool[srcFrameIdx].CurrImage.phy_addr,
				      			(void *)&prp_buffer,
				      			picWidth,
				      			picHeight,
				      			((EncInst *) handle)->instIndex,
				      			CODEC_READING,
				      			rot_en,
				      			output_ratio,
				      			0);

		if (usr_config->src == PATH_EMMA)
		{
			frameBuf[srcFrameIdx].bufY 	= cap_buffers[prp_buffer.index].offset;
			frameBuf[srcFrameIdx].bufCb = frameBuf[srcFrameIdx].bufY 	+ image_size;
			frameBuf[srcFrameIdx].bufCr = frameBuf[srcFrameIdx].bufCb 	+ (image_size >> 2);
		}

		if (ret < 0)
		{
			printf("Encoder finished\n");
			break;
		}

		pthread_mutex_lock(&codec_mutex);

		//To fix the MPEG4 issue on MX27 TO2
		CodStd codStd = ((EncInst *) handle)->CodecInfo.encInfo.openParam.bitstreamFormat;

		//printf("CodStd = %d\n", codStd);

		if (codStd == STD_MPEG4)
		{
			vpu_ESDMISC_LHD(1);
		}

		ret = vpu_EncStartOneFrame(handle, &encParam);

		if (ret != RETCODE_SUCCESS)
		{
			printf("vpu_EncStartOneFrame failed. Error code is %d \n",ret);
			pthread_mutex_unlock(&codec_mutex);
			goto ERR_ENC_OPEN;
		}

		timer(ENC_START);

#ifdef INT_CALLBACK
		pthread_mutex_lock(&vpu_busy_mutex);
		pthread_cond_wait(&current_job_cond, &vpu_busy_mutex);
		pthread_mutex_unlock(&vpu_busy_mutex);
#else
		vpu_WaitForInt(200);
#endif
		timer(ENC_STOP);

		ret = vpu_EncGetOutputInfo(handle, &outputInfo);

		if (ret != RETCODE_SUCCESS)
		{
			printf("vpu_EncGetOutputInfo failed. Error code is %d \n",ret);
			goto ERR_ENC_OPEN;
		}

		// To fix the MPEG4 issue on MX27 TO2
		if (codStd == STD_MPEG4)
		{
			vpu_ESDMISC_LHD(0);
		}
		pthread_mutex_unlock(&codec_mutex);

		//if (quitflag)
		//	break;

#if STREAM_ENC_PIC_RESET == 1
		bsBuf0 	= outputInfo.bitstreamBuffer;
		size0 	= outputInfo.bitstreamSize;
		ret 	= FillBsBufMulti(usr_config->dst,
			                     usr_config->dst_name,
				     	         virt_bit_stream_buf + bsBuf0 -	bit_stream_buf.phy_addr,
				     	         0,
				     	         0,
				     	         size0,
				     	         ((EncInst *) handle)->instIndex,
				     	         0,
				     	         0,
				     	         CODEC_WRITING,
				     	         data);
#else
		ReadBsBufHelper(handle,
		                usr_config->dst,
		                usr_config->dst_name,
						paBsBufStart,
						vaBsBufStart,
						vaBsBufEnd,
						512 * 4,
						((EncInst *) handle)->instIndex,
						CODEC_WRITING);

#endif

		if (encOP.mbQpReport == 1)
		{
			SaveQpReport(outputInfo.mbQpInfo,
						 encOP.picWidth,
						 encOP.picHeight,
						 frameIdx,
						 "encqpreport.dat" );
		}

		frameIdx++;
	}

#if STREAM_ENC_PIC_RESET == 0
	ReadBsBufHelper(handle,
	                usr_config->dst,
	                usr_config->dst_name,
			        paBsBufStart,
			        vaBsBufStart,
			        vaBsBufEnd,
			        0,
			        ((EncInst *) handle)->instIndex,
			        CODEC_WRITING);
#endif

ERR_ENC_OPEN:
	pthread_mutex_lock(&codec_mutex);

	ret = vpu_EncClose(handle);

	if (ret == RETCODE_FRAME_NOT_COMPLETE)
	{
		vpu_EncGetOutputInfo(handle, &outputInfo);
		vpu_EncClose(handle);
	}

	pthread_mutex_unlock(&codec_mutex);

ERR_ENC_INIT:
	FrameBufferFree(FrameBufPool,initialInfo.minFrameBufferCount + extra_fb);

	IOFreeVirtMem(&bit_stream_buf);

	IOFreePhyMem (&bit_stream_buf);

	DPRINTF("Enc closed\n");

	memset(&enc_info, 0, sizeof(struct codec_info));

	return &codec_thread[usr_config->index];
}

void *sig_thread(void *arg)
{
	DPRINTF("Enter signal monitor thread.\n");

	int signo;

	if (sigwait(&mask, &signo) != 0)
	{
		printf("sigwait error\n");
	}

	switch (signo)
	{
	    case SIGINT:
		    DPRINTF("interrupt: SIGINT.\n");

		    quitflag = 1;

#ifdef	INT_CALLBACK
		    pthread_cond_broadcast(&current_job_cond);
#endif
		    break;
	    default:
		    printf("unexpected signal %d\n", signo);
	}

	return NULL;
}
#endif
