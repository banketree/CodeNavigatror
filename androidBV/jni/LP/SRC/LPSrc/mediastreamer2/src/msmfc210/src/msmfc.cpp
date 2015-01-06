/*
mediastreamer2 x264 plugin
Copyright (C) 2006-2010 Belledonne Communications SARL (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/rfc3984.h"
#include "android/log.h"

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

#include <linux/videodev2.h>
#include <videodev2_samsung.h>

#ifdef _MSC_VER
#include <stdint.h>
#endif

#include "SsbSipMfcApi.h"
#include "mfc_interface.h"
#include <android/log.h>
#include "CFimc.h"
#include <IDBTCfg.h>

#ifndef VERSION
#define VERSION "1.4.1"
#endif


#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "MFC", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "MFC", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , "MFC", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , "MFC", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "MFC", __VA_ARGS__)

#define RC_MARGIN 10000 /*bits per sec*/


#define SPECIAL_HIGHRES_BUILD_CRF 28
extern CFIMC g_cam;

typedef struct _mfc_EncData{
//	x264_t *enc;
	int isbegin;;
	void *encHandle;
	MSVideoSize vsize;
	int bitrate;
	float fps;
	int mode;
	mblk_t *sps,*pps;
	Rfc3984Context *packer;
	SSBSIP_MFC_ENC_INPUT_INFO input_info;
	SSBSIP_MFC_ENC_OUTPUT_INFO output_info;
	SSBSIP_MFC_ENC_H264_PARAM param;
}MFCEncData;


static void enc_init(MSFilter *f){
	printf("mfc264_enc_desc init in\n");
	MFCEncData *d=ms_new(MFCEncData,1);
	memset(d,0,sizeof(MFCEncData));
	d->encHandle=NULL;
    MS_VIDEO_SIZE_ASSIGN(d->vsize,CIF);
	d->bitrate=128000;
	d->fps=3;
	d->mode=1;
	d->isbegin = 0;
	f->data=d;
	printf("mfc264_enc_desc init out\n");
}

static void enc_uninit(MSFilter *f){

	MFCEncData *d=(MFCEncData*)f->data;
//	ms_free(d);
//	return;
	if(d->sps)
		freemsg(d->sps);
	if(d->pps)
		freemsg(d->pps);
	ms_free(d);

	if(g_stIdbtCfg.iNegotiateMode == 1 && g_cam.auto_mode_width != 704)
	{
		g_cam.auto_mode_width = 704;
		g_cam.auto_mode_height = 576;

		g_cam.uninit_video();
		LOGE("CFIMC iNegotiateMode == 1 enc_uninit will change videomode --> D1\n");
		//d1
		if(g_cam.init_video(0, 704, 576) < 0){
			LOGE("CFIMC camera init faild\n");
		}
	}
	else if(g_stIdbtCfg.iNegotiateMode == 1 && g_cam.auto_mode_width == 704)
	{
		LOGE("CFIMC iNegotiateMode == 1 enc_uninit current mode is D1\n");
	}
}

static int setparam(SSBSIP_MFC_ENC_H264_PARAM *param)
{

//		if( g_stIdbtCfg.iVedioQuality  == 1 ) //cif 30
//		{
//			g_cam.initFormat = 1;
//			g_cam.change_format(MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H);
//			g_stIdbtCfg.iVedioQuality = 30;
//		}
//		else if( g_stIdbtCfg.iVedioQuality == 2 ) //d1 30
//		{
//			g_cam.initFormat = 2;
//			g_cam.change_format(MS_VIDEO_SIZE_4CIF_W, MS_VIDEO_SIZE_4CIF_H);
//			g_stIdbtCfg.iVedioQuality = 30;
//		}

		/* common parameters  */
		param->codecType=H264_ENC;			/* [IN] codec type */

		if(g_stIdbtCfg.iNegotiateMode == 0 && g_stIdbtCfg.iVedioFmt == 0)
		{
			param->SourceWidth=MS_VIDEO_SIZE_CIF_W; //352;				/* [IN] width of video to be encoded */
			param->SourceHeight=MS_VIDEO_SIZE_CIF_H; //288;				/* [IN] height of video to be encoded */
		}
		else if(g_stIdbtCfg.iNegotiateMode == 0 && g_stIdbtCfg.iVedioFmt == 1)
		{
			param->SourceWidth=MS_VIDEO_SIZE_4CIF_W; //352;				/* [IN] width of video to be encoded */
			param->SourceHeight=MS_VIDEO_SIZE_4CIF_H; //288;			/* [IN] height of video to be encoded */
		}
		else if(g_stIdbtCfg.iNegotiateMode == 0 && g_stIdbtCfg.iVedioFmt == 2)
		{
			param->SourceWidth=MS_VIDEO_SIZE_720P_W; //352;				/* [IN] width of video to be encoded */
			param->SourceHeight=MS_VIDEO_SIZE_720P_H; //288;			/* [IN] height of video to be encoded */
		}
		else if(g_stIdbtCfg.iNegotiateMode == 1)
		{
			param->SourceWidth  = g_cam.auto_mode_width;
			param->SourceHeight = g_cam.auto_mode_height;
		}

//		param->SourceWidth=MS_G_WIDTH;				/* [IN] width of video to be encoded */
//		param->SourceHeight=MS_G_HEIGHT;			/* [IN] height of video to be encoded */
		param->IDRPeriod=20;				/* [IN] GOP number(interval of I-frame) */
		param->SliceMode=0;					/* [IN] Multi slice mode */
		param->RandomIntraMBRefresh=0;		/* [IN] cyclic intra refresh */
		param->Bitrate=128000;				/* [IN] rate control parameter(bit rate) */

		param->QSCodeMax=30;					   /* [IN] Maximum Quantization value */
		param->QSCodeMin=10;					   /* [IN] Minimum Quantization value */
		param->PadControlOn=0;				   /* [IN] Enable padding control */
		param->LumaPadVal=0;					   /* [IN] Luma pel value used to fill padding area */
		param->CbPadVal=0;					   /* [IN] CB pel value used to fill padding area */
		param->CrPadVal=0;					   /* [IN] CR pel value used to fill padding area */

		param->ProfileIDC=0; 					/* [IN] profile 0-main 1-high 2-base */
		param->LevelIDC=13;						/* [IN] level */
		param->FrameMap=0;						/* [IN] Encoding input mode(tile mode or linear mode) */

		param->SliceArgument=0;					/* [IN] MB number or byte number */
		param->NumberBFrames=0;					/* [IN] The number of consecutive B frame inserted */
		param->NumberReferenceFrames=1;			/* [IN] The number of reference pictures used */
		param->NumberRefForPframes=1;			/* [IN] The number of reference pictures used for encoding P pictures */
		param->LoopFilterDisable=1;				/* [IN] disable the loop filter */
		param->LoopFilterAlphaC0Offset=0;		/* [IN] Alpha & C0 offset for H.264 loop filter */
		param->LoopFilterBetaOffset=0;			/* [IN] Beta offset for H.264 loop filter */
		param->SymbolMode=0; 					/* [IN] The mode of entropy coding(CABAC, CAVLC) */
		param->PictureInterlace=0;				/* [IN] Enables the interlace mode */
		param->Transform8x8Mode=0;				/* [IN] Allow 8x8 transform(This is allowed only for high profile) */
		param->DarkDisable=1;					/* [IN] Disable adaptive rate control on dark region */
		param->SmoothDisable=1;				/* [IN] Disable adaptive rate control on smooth region */
		param->StaticDisable=1;					/* [IN] Disable adaptive rate control on static region */
		param->ActivityDisable=1;				/* [IN] Disable adaptive rate control on high activity region */


		//edit by hb
		//param->FrameQp;  param->FrameQp_P;  param->FrameQp_B;
		param->FrameQp=g_stIdbtCfg.iVedioQuality;					/* [IN] The quantization parameter of the frame */
		param->FrameQp_P=g_stIdbtCfg.iVedioQuality;				  	/* [IN] The quantization parameter of the P frame */
		param->FrameQp_B=g_stIdbtCfg.iVedioQuality;					/* [IN] The quantization parameter of the B frame */
		//---------------
//		param->FrameQp=20;					   /* [IN] The quantization parameter of the frame */
//		param->FrameQp_P=20;				   /* [IN] The quantization parameter of the P frame */
//		param->FrameQp_B=20;				   /* [IN] The quantization parameter of the B frame */
		param->FrameRate=3;					   /* [IN] rate control parameter(frame rate) */

#if 0		//CBR
		param->EnableFRMRateControl=1;		   /* [IN] frame based rate control enable */
		param->EnableMBRateControl=1; 		   /* [IN] Enable macroblock-level rate control */
		param->CBRPeriodRf=10; 				   /* [IN] Reaction coefficient parameter for rate control */
#else		//VBR
		param->EnableFRMRateControl=0;		   /* [IN] frame based rate control enable */
		param->EnableMBRateControl=0;		   /* [IN] Enable macroblock-level rate control */
		param->CBRPeriodRf=100;				   /* [IN] Reaction coefficient parameter for rate control */
#endif
}
unsigned char* FindDelimiter(unsigned char* pBuffer, unsigned int size)
{
    int i;

    for (i = 0; i < size - 3; i++) {
        if ((pBuffer[i] == 0x00)   &&
            (pBuffer[i+1] == 0x00) &&
            (pBuffer[i+2] == 0x00) &&
            (pBuffer[i+3] == 0x01))
            return (pBuffer + i);
    }

    return NULL;
}


static void enc_preprocess(MSFilter *f){
	void *hopen;
	MFCEncData *d=(MFCEncData*)f->data;

	float bitrate;

	printf("enc_preprocess in\n");

	d->packer=rfc3984_new();
	rfc3984_set_mode(d->packer,d->mode);
	rfc3984_enable_stap_a(d->packer,FALSE);

	setparam(&d->param);
	
	hopen = SsbSipMfcEncOpen();
	if(NULL == hopen)
	{
		printf("SsbSipMfcEncOpen faild\n");
		return;
	}
	d->encHandle = hopen;
	if(SsbSipMfcEncInit(hopen,&d->param) != MFC_RET_OK)
	{
		printf("SsbSipMfcEncInit faild\n");
		SsbSipMfcEncClose(d->encHandle);
		d->encHandle=NULL;
		return;
	}
	

	if( SsbSipMfcEncGetInBuf(hopen,&d->input_info) !=MFC_RET_OK)
	{
		printf("SsbSipMfcEncGetInBuf faild\n");
		SsbSipMfcEncClose(d->encHandle);
		d->encHandle=NULL;
		return;
	}
	//create sps pps frame;
	
	if( SsbSipMfcEncGetOutBuf(hopen, &d->output_info) !=MFC_RET_OK)
	{
		printf("SsbSipMfcEncGetOutBuf create sps pps frame fail\n");
		SsbSipMfcEncClose(d->encHandle);
		d->encHandle=NULL;
		return;
	}
	else
	{
		unsigned char *p = NULL;
		int iSpsSize = 0;
		int iPpsSize = 0;
		
		#if 0
		d->sps = allocb(d->output_info.headerSize,0);
		if(d->sps)
		{
			memcpy(d->sps->b_wptr,d->output_info.StrmVirAddr+4,d->output_info.headerSize-4);
			d->sps->b_wptr+=d->output_info.headerSize-4;
		}
		#else
		p = FindDelimiter((unsigned char *)(d->output_info.StrmVirAddr) + 4, d->output_info.headerSize - 4);
		if(p==NULL)
		{
			printf("sps pps delimiter is null\n");
			return;
		}
		iSpsSize = p - (unsigned char *)d->output_info.StrmVirAddr;
		iPpsSize = d->output_info.headerSize - iSpsSize;
		printf("headersize=%d ppssize=%d,spssize=%d\n",d->output_info.headerSize,iPpsSize,iSpsSize);
		d->sps = allocb(iSpsSize-4,0);
		if(d->sps)
		{
			memcpy(d->sps->b_wptr,d->output_info.StrmVirAddr+4,iSpsSize-4);
			d->sps->b_wptr+=iSpsSize-4;
		}
		d->pps = allocb(iPpsSize-4,0);
		if(d->pps)
		{
			memcpy(d->pps->b_wptr,d->output_info.StrmVirAddr+iSpsSize+4,iPpsSize-4);
			d->pps->b_wptr+=iPpsSize-4;
		}
		#endif
	}
	
	printf("mfc enc init success\n");
}


static void mfc_nals_to_msgb(MSQueue *q,void *pnals,unsigned int len){
	int i;
	mblk_t *m;
	int _size = len;
	m=allocb(_size,0);
	if(m==NULL) return ;
	memcpy(m->b_wptr,pnals+4,_size-4);
	m->b_wptr+=_size-4;
	ms_queue_put(q,m);
}

typedef struct
{
	void *pAddrY;
	void *pAddrC;
}MFC_ENC_ADDR_INFO;

unsigned char		 NALU_BEGIN_FLAG1[4]={0x00,0x00,0x00,0x01};


static void enc_process(MSFilter *f){
	
	int ret;
	int start_code_len = 0;
	MFCEncData *d=(MFCEncData*)f->data;
	uint32_t ts=f->ticker->time*90LL;
	mblk_t *im;
	MSPicture pic;
	MSQueue nalus;
	ms_queue_init(&nalus);
	MFC_ENC_ADDR_INFO addrInfo;
	mblk_t * sps,*pps;
	unsigned long width=MS_VIDEO_SIZE_CIF_W;
	unsigned long height=MS_VIDEO_SIZE_CIF_H;

	if(g_stIdbtCfg.iNegotiateMode == 0 && g_stIdbtCfg.iVedioFmt == 0)
	{
		width = MS_VIDEO_SIZE_CIF_W;
		height = MS_VIDEO_SIZE_CIF_H;
	}
	else if(g_stIdbtCfg.iNegotiateMode == 0 && g_stIdbtCfg.iVedioFmt == 1)
	{
		width = MS_VIDEO_SIZE_4CIF_W;
		height = MS_VIDEO_SIZE_4CIF_H;
	}
	else if(g_stIdbtCfg.iNegotiateMode == 0 && g_stIdbtCfg.iVedioFmt == 2)
	{
		width = MS_VIDEO_SIZE_720P_W;
		height = MS_VIDEO_SIZE_720P_H;
	}
	else if(g_stIdbtCfg.iNegotiateMode == 1)
	{
		width = g_cam.auto_mode_width;
		height = g_cam.auto_mode_height;
	}

	unsigned long y_size = width * height;
	unsigned long c_size = (width * height) >> 1;

	unsigned long aligned_y_size = ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height));
    unsigned long aligned_c_size = ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height/2));

	d->input_info.YSize = aligned_y_size;
	d->input_info.CSize=aligned_c_size;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		int index=*( (int *)im->b_rptr);
		if(d->isbegin < 6)
		{
			if(d->sps && d->pps)
			{
				sps = copyb(d->sps);
				if(sps) ms_queue_put(&nalus,sps);
				pps = copyb(d->pps);
				if(pps) ms_queue_put(&nalus,pps);

			}
			freemsg(im);
			g_cam.releaseRecordFrame(index);
			rfc3984_pack(d->packer,&nalus,f->outputs[0],ts);
			d->isbegin++;
			LOGV("MSG(%s):send sps pps counts = %d", __func__, d->isbegin);
			if((im=ms_queue_get(f->inputs[0]))!=NULL){
				index=*( (int *)im->b_rptr);
				freemsg(im);
				g_cam.releaseRecordFrame(index);
			}
	//				while((im=ms_queue_get(f->inputs[0]))!=NULL){
	//					index=*( (int *)im->b_rptr);
	//					freemsg(im);
	//					g_cam.releaseRecordFrame(index);
	//				}
			return;
		}

		unsigned int addry=g_cam.getAddrY(index);
		unsigned int addrc=g_cam.getAddrC(index);

		if(addry==0 || addrc ==0)
			continue;
		memcpy(&addrInfo.pAddrY, &addry, sizeof(addrInfo.pAddrY));
		memcpy(&addrInfo.pAddrC, &addrc, sizeof(addrInfo.pAddrC));

		d->input_info.YPhyAddr= addrInfo.pAddrY;
		d->input_info.CPhyAddr = addrInfo.pAddrC;


		ret = SsbSipMfcEncSetInBuf(d->encHandle,&d->input_info);
		if(ret != MFC_RET_OK)
		{
			continue;
		}
		ret = SsbSipMfcEncExe(d->encHandle);
		if(ret != MFC_RET_OK)
		{
			continue;
			
		}

		ret = SsbSipMfcEncGetOutBuf(d->encHandle,&d->output_info);

		if(ret == MFC_RET_OK)
		{
			printf("datasize=%d\n",d->output_info.dataSize);
			printf("frametype=%d\n",d->output_info.frameType);
			if(d->output_info.frameType == MFC_FRAME_TYPE_I_FRAME)
			{
				if(d->sps && d->pps)
				{
					sps = copyb(d->sps);
					if(sps) ms_queue_put(&nalus,sps);
					pps = copyb(d->pps);
					if(pps) ms_queue_put(&nalus,pps);
				}
			}
			mfc_nals_to_msgb(&nalus,d->output_info.StrmVirAddr,d->output_info.dataSize);
			rfc3984_pack(d->packer,&nalus,f->outputs[0],ts);
		}
		freemsg(im);
		g_cam.releaseRecordFrame(index);
	}
}

static void enc_postprocess(MSFilter *f){
	LOGE("msmfc postprocess in\n");

	MFCEncData *d=(MFCEncData*)f->data;

	rfc3984_destroy(d->packer);
	d->packer=NULL;
	if (d->encHandle!=NULL){
		SsbSipMfcEncClose(d->encHandle);
		d->encHandle=NULL;
	}
	LOGE("msmfc postprocess out\n");
}

static int enc_set_br(MSFilter *f, void *arg){
	MFCEncData *d=(MFCEncData*)f->data;
	d->bitrate=*(int*)arg;

	if (d->bitrate>=1024000){
		d->vsize.width = MS_VIDEO_SIZE_SVGA_W;
		d->vsize.height = MS_VIDEO_SIZE_SVGA_H;
		d->fps=25;
	}else if (d->bitrate>=512000){
		d->vsize.width = MS_VIDEO_SIZE_VGA_W;
		d->vsize.height = MS_VIDEO_SIZE_VGA_H;
		d->fps=25;
	} else if (d->bitrate>=256000){
		d->vsize.width = MS_VIDEO_SIZE_VGA_W;
		d->vsize.height = MS_VIDEO_SIZE_VGA_H;
		d->fps=15;
	}else if (d->bitrate>=170000){
		d->vsize.width=MS_VIDEO_SIZE_QVGA_W;
		d->vsize.height=MS_VIDEO_SIZE_QVGA_H;
		d->fps=15;
	}else if (d->bitrate>=128000){
		d->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		d->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		d->fps=10;
	}else if (d->bitrate>=64000){
		d->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		d->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		d->fps=7;
	}else{
		d->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		d->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		d->fps=5;
	}

#if defined (ANDROID) || TARGET_OS_IPHONE==1
	d->vsize.width=MS_VIDEO_SIZE_QVGA_W;
	d->vsize.height=MS_VIDEO_SIZE_QVGA_H;
	d->fps=10;

#endif
	
	ms_message("bitrate requested...: %d (%d x %d)\n", d->bitrate, d->vsize.width, d->vsize.height);
	return 0;
}

static int enc_set_fps(MSFilter *f, void *arg){
	MFCEncData *d=(MFCEncData*)f->data;
	d->fps=*(float*)arg;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
	MFCEncData *d=(MFCEncData*)f->data;
	*(float*)arg=d->fps;
	return 0;
}

static int enc_get_vsize(MSFilter *f, void *arg){
	MFCEncData *d=(MFCEncData*)f->data;
	*(MSVideoSize*)arg=d->vsize;
	return 0;
}

static int enc_set_vsize(MSFilter *f, void *arg){
	MFCEncData *d=(MFCEncData*)f->data;

	d->vsize=*(MSVideoSize*)arg;

	return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	MFCEncData *d=(MFCEncData*)f->data;
	const char *fmtp=(const char *)arg;
	char value[12];
	if (fmtp_get_value(fmtp,"packetization-mode",value,sizeof(value))){
		d->mode=atoi(value);
		ms_message("packetization-mode set to %i",d->mode);
	}
	return 0;
}

static int enc_req_vfu(MSFilter *f, void *arg){
	MFCEncData *d=(MFCEncData*)f->data;
//	d->generate_keyframe=TRUE;
	return 0;
}


static MSFilterMethod enc_methods[]={
	{	MS_FILTER_SET_FPS	,	enc_set_fps	},
	{	MS_FILTER_SET_BITRATE	,	enc_set_br	},
	{	MS_FILTER_GET_FPS	,	enc_get_fps	},
	{	MS_FILTER_GET_VIDEO_SIZE,	enc_get_vsize	},
	{	MS_FILTER_SET_VIDEO_SIZE,	enc_set_vsize	},
	{	MS_FILTER_ADD_FMTP	,	enc_add_fmtp	},
	{	MS_FILTER_REQ_VFU	,	enc_req_vfu	},
	{	0	,			NULL		}
};

static void dec_init(MSFilter *f){
	printf("mfc264_dec_desc init\n");
};
static void dec_preprocess(MSFilter * f){};
static void dec_postprocess(MSFilter * f){};
static void dec_process(MSFilter * f){};
static void dec_uninit(MSFilter *f){};

//#ifndef _MSC_VER
#if 0
  MSFilterDesc mfc264_dec_desc={
	 .id=MS_FILTER_PLUGIN_ID,
	 .name="MSX264Enc",
	 .text="A H264 encoder based on x264 project",
	 .category=MS_FILTER_DECODER,
	 .enc_fmt="H264",
	 .ninputs=1,
	 .noutputs=1,
	 .init=dec_init,
	 .preprocess=dec_preprocess,
	 .process=dec_process,
	 .postprocess=dec_postprocess,
	 .uninit=dec_uninit,
	 .methods=NULL
 };

 MSFilterDesc mfc264_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSMFC264Enc",
	.text="A H264 encoder based on samsung s5pv210 mfc",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="H264",
	.ninputs=1,
	.noutputs=1,
	.init=enc_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=enc_methods
};

#else

 MSFilterDesc mfc264_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSMFC264Enc",
	"A H264 encoder based on samsung s5pv20 mfc",
	MS_FILTER_ENCODER,
	"H264",
	1,
	1,
	enc_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	enc_methods
};

 MSFilterDesc mfc264_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSX264Dnc",
	"A H264 encoder based on x264 project",
	MS_FILTER_DECODER,
	"H264",
	1,
	1,
	dec_init,
	dec_preprocess,
	dec_process,
	dec_postprocess,
	dec_uninit,
	NULL
};

#endif

MS2_PUBLIC void libmsmfc264_init(void){
	ms_filter_register(&mfc264_enc_desc);
	ms_message("msmfc264-" VERSION " plugin registered.");
}

