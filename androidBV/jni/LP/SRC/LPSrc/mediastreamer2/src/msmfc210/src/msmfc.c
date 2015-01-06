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




#ifdef _MSC_VER
#include <stdint.h>
#endif

#include "SsbSipMfcApi.h"
#include "mfc_interface.h"
#include <android/log.h>

#ifndef VERSION
#define VERSION "1.4.1"
#endif


#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "ProjectName", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "ProjectName", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , "ProjectName", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , "ProjectName", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "ProjectName", __VA_ARGS__)


#define RC_MARGIN 10000 /*bits per sec*/


#define SPECIAL_HIGHRES_BUILD_CRF 28



typedef struct _mfc_EncData{
//	x264_t *enc;
	void *encHandle;
	MSVideoSize vsize;
	int bitrate;
	float fps;
	int mode;
	Rfc3984Context *packer;
	SSBSIP_MFC_ENC_INPUT_INFO input_info;
	SSBSIP_MFC_ENC_OUTPUT_INFO output_info;
	SSBSIP_MFC_ENC_H264_PARAM param;
}MFCEncData;

typedef struct _mfc_Enc_MPEG4_Data{
//	x264_t *enc;
	void *encHandle;
	MSVideoSize vsize;
	int bitrate;
	float fps;
	int mode;
	Rfc3984Context *packer;
	SSBSIP_MFC_ENC_INPUT_INFO input_info;
	SSBSIP_MFC_ENC_OUTPUT_INFO output_info;
	SSBSIP_MFC_ENC_MPEG4_PARAM param;
}MFCEncMpeg4Data;


static void enc_init(MSFilter *f){
		printf("mfc264_enc_desc init\n");
	MFCEncData *d=ms_new(MFCEncData,1);
	memset(d,0,sizeof(MFCEncData));
	d->encHandle=NULL;
    MS_VIDEO_SIZE_ASSIGN(d->vsize,CIF);
	d->bitrate=384000;
	d->fps=15;
//	d->keyframe_int=10; /*10 seconds */
	d->mode=0;
//	d->framenum=0;
//	d->generate_keyframe=FALSE;
	f->data=d;
printf("mfc264_enc_desc init\n");

}

static void enc_uninit(MSFilter *f){
	MFCEncData *d=(MFCEncData*)f->data;
	ms_free(d);
}

static int setparam(SSBSIP_MFC_ENC_H264_PARAM *param)
{

		/* common parameters  */
		param->codecType=H264_ENC;			/* [IN] codec type */
		param->SourceWidth=352;				/* [IN] width of video to be encoded */
		param->SourceHeight=288;			/* [IN] height of video to be encoded */
		param->IDRPeriod=30;				/* [IN] GOP number(interval of I-frame) */
		param->SliceMode=0;					/* [IN] Multi slice mode */
		param->RandomIntraMBRefresh=0;		/* [IN] cyclic intra refresh */
		param->Bitrate=12800;				/* [IN] rate control parameter(bit rate) */

		param->QSCodeMax=30;					   /* [IN] Maximum Quantization value */
		param->QSCodeMin=10;					   /* [IN] Minimum Quantization value */
		param->PadControlOn=0;				   /* [IN] Enable padding control */
		param->LumaPadVal=0;					   /* [IN] Luma pel value used to fill padding area */
		param->CbPadVal=0;					   /* [IN] CB pel value used to fill padding area */
		param->CrPadVal=0;					   /* [IN] CR pel value used to fill padding area */

		param->ProfileIDC=0; 					/* [IN] profile 0-main 1-high 2-base */
		param->LevelIDC=40;						/* [IN] level */
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


		param->FrameQp=20;					   /* [IN] The quantization parameter of the frame */
		param->FrameQp_P=20;					   /* [IN] The quantization parameter of the P frame */
		param->FrameQp_B=20;					   /* [IN] The quantization parameter of the B frame */
		param->FrameRate=15;					   /* [IN] rate control parameter(frame rate) */


		param->EnableFRMRateControl=0;		   /* [IN] frame based rate control enable */
		param->EnableMBRateControl=0; 		   /* [IN] Enable macroblock-level rate control */
		param->CBRPeriodRf=100; 				   /* [IN] Reaction coefficient parameter for rate control */
}

static void enc_preprocess(MSFilter *f){
	void *hopen;
	MFCEncData *d=(MFCEncData*)f->data;
//	x264_param_t params;

	float bitrate;

	LOGV("enc_preprocess\n");

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
	printf("mfc enc init success\n");
}


static void mfc_nals_to_msgb(MSQueue *q,void *pnals,int len){
	int i;
	mblk_t *m;
	m=allocb(len-4,0);
	if(m==NULL) return ;
	
	memcpy(m->b_wptr,pnals+4,len-4);
	m->b_wptr+=len-4;
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
	int size = 352*288;
	ms_queue_init(&nalus);
	MFC_ENC_ADDR_INFO addrinfo;

	
	printf("mfc enc_process\n");
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		if (ms_yuv_buf_init_from_mblk(&pic,im)==0){

			
			d->input_info.YPhyAddr = pic.planes[0];
			d->input_info.CPhyAddr = pic.planes[1];
			d->input_info.YSize = size;
			d->input_info.CSize=size/2;
			
			ret = SsbSipMfcEncSetInBuf(d->encHandle,&d->input_info);
			if(ret != MFC_RET_OK)
			{
				printf("SsbSipMfcEncSetInBuf fail\n");
				continue;
			}

			ret = SsbSipMfcEncExe(d->encHandle);
			if(ret != MFC_RET_OK)
			{
				printf("SsbSipMFCEncExe fail\n");
				continue;
				
			}

			ret = SsbSipMfcEncGetOutBuf(d->encHandle,&d->output_info);

			start_code_len=0;
			if(memcmp(d->output_info.StrmVirAddr,NALU_BEGIN_FLAG1,4) == 0)
				start_code_len = 4;

			printf("start_code_len = %d\n",start_code_len);
			
			if(ret == MFC_RET_OK)
			{
				printf("datasize=%d\n",d->output_info.dataSize);
				printf("frametype=%d\n",d->output_info.frameType);
				
				mfc_nals_to_msgb(&nalus,d->output_info.StrmPhyAddr,d->output_info.dataSize);
				//rfc3984_pack(d->packer,&d->output_info.StrmVirAddr,f->outputs[0],ts);
				rfc3984_pack(d->packer,&nalus,f->outputs[0],ts);
			}
		}
		freemsg(im);
	}
	printf("mfc enc_process out\n");
	
}

static void enc_postprocess(MSFilter *f){
	MFCEncData *d=(MFCEncData*)f->data;
	rfc3984_destroy(d->packer);
	d->packer=NULL;
	if (d->encHandle!=NULL){
		SsbSipMfcEncClose(d->encHandle);
		d->encHandle=NULL;
	}
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

#ifndef _MSC_VER

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
	MS_FILTER_DECODER
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

