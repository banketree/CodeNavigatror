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

#ifdef _MSC_VER
#include <stdint.h>
#endif

#include <x264.h>

#ifndef VERSION
#define VERSION "1.4.1"
#endif

#include "log4c.h"
#define RC_MARGIN 10000 /*bits per sec*/

#define SPECIAL_HIGHRES_BUILD_CRF 28


/* the goal of this small object is to tell when to send I frames at startup:
at 2 and 4 seconds*/
typedef struct VideoStarter{
	uint64_t next_time;
	int i_frame_count;
}VideoStarter;

static void x264_init();
//void msx264_init();


static void video_starter_init(VideoStarter *vs){
	vs->next_time=0;
	vs->i_frame_count=0;
}

static void video_starter_first_frame(VideoStarter *vs, uint64_t curtime){
	vs->next_time=curtime+2000;
}

static bool_t video_starter_need_i_frame(VideoStarter *vs, uint64_t curtime){
	if (vs->next_time==0) return FALSE;
	if (curtime>=vs->next_time){
		vs->i_frame_count++;
		if (vs->i_frame_count==1){
			vs->next_time+=2000;
		}else{
			vs->next_time=0;
		}
		return TRUE;
	}
	return FALSE;
}

typedef struct _EncData{
	x264_t *enc;
	MSVideoSize vsize;
	int bitrate;
	float fps;
	int mode;
	uint64_t framenum;
	Rfc3984Context *packer;
	int keyframe_int;
	VideoStarter starter;
	bool_t generate_keyframe;
}EncData;


EncData *g_enc = NULL;

void msx264_init()
{
	x264_param_t params;

	if(!g_enc){
		g_enc=ms_new(EncData,1);
		if(!g_enc) return;

	}
	LOGD("msx264_init success\n");
	memset(g_enc,0,sizeof(EncData));
	g_enc->enc=NULL;
    MS_VIDEO_SIZE_ASSIGN(g_enc->vsize,CIF);
	g_enc->bitrate=128000;
	g_enc->fps=5;
	g_enc->keyframe_int=10; /*10 seconds */
	g_enc->mode=0;
	g_enc->framenum=0;
	g_enc->generate_keyframe=FALSE;
	g_enc->packer=NULL;

	x264_init();
}
static void msx264_uninit()
{
	
}

static void enc_init(MSFilter *f){

	f->data = g_enc;
	if(!g_enc->enc)
	{
		msx264_init();
	}
	return;
#if 0
	EncData *d=ms_new(EncData,1);
	memset(d,0,sizeof(EncData));
	d->enc=NULL;
    MS_VIDEO_SIZE_ASSIGN(d->vsize,CIF);
	d->bitrate=128000;
	d->fps=5;
	d->keyframe_int=10; /*10 seconds */
	d->mode=0;
	d->framenum=0;
	d->generate_keyframe=FALSE;
	d->packer=NULL;
	f->data=d;
#endif
}

static void enc_uninit(MSFilter *f){
	LOGD("x264 enc_uninit\n");
	EncData *d=(EncData*)f->data;
	if(!d) return;
	rfc3984_destroy(d->packer);
	d->packer=NULL;
	if (d->enc!=NULL){
		x264_encoder_close(d->enc);
		d->enc=NULL;
	}
//	ms_free(d);
	msx264_init();
	f->data = NULL;
	
	LOGD("x264  enc_uninit out\n");

}
static void enc_preprocess(MSFilter *f){
	
}
static void __x264_log( void *p_unused, int i_level, const char *psz_fmt, va_list arg )
{
    char *psz_prefix;
    switch( i_level )
    {
        case X264_LOG_ERROR:
            psz_prefix = "error";
            break;
        case X264_LOG_WARNING:
            psz_prefix = "warning";
            break;
        case X264_LOG_INFO:
            psz_prefix = "info";
            break;
        case X264_LOG_DEBUG:
            psz_prefix = "debug";
            break;
        default:
            psz_prefix = "unknown";
            break;
    }
	LOGD("x264 [%s]: ", psz_prefix );
    LOGD(psz_fmt, arg );
}

static void x264_init(){
	EncData *d=g_enc;
	x264_param_t params;
	float bitrate;

	if(d==NULL) return;
	
	d->packer=rfc3984_new();

	rfc3984_set_mode(d->packer,d->mode);
	rfc3984_enable_stap_a(d->packer,FALSE);
#ifdef __arm__	
	//if (x264_param_default_preset(&params,"superfast"/*"ultrafast"*/,"zerolatency")) { 
	if (x264_param_default_preset(&params,"superfast"/*"ultrafast"*/,"zerolatency")) { 

#else
		x264_param_default(&params); {
#endif
		printf("Cannot apply default x264 configuration");
	};
	params.pf_log = __x264_log;
#if 1
	params.i_threads=1;
	params.i_sync_lookahead=0;
	params.i_width=d->vsize.width;
	params.i_height=d->vsize.height;
	params.i_fps_num=(int)d->fps;
	params.i_fps_den=1;
	params.i_slice_max_size=ms_get_payload_max_size()-100; /*-100 security margin*/
	params.i_level_idc=13;
	
	bitrate=(float)d->bitrate*0.92;
	if (bitrate>RC_MARGIN)
		bitrate-=RC_MARGIN;
	
#ifndef __arm__	
	printf("mode=__arm__\n");
	params.rc.i_rc_method = X264_RC_CRF;
	params.rc.i_bitrate=(int)(bitrate/1000);
	params.rc.f_rate_tolerance=0.1;
	params.rc.i_vbv_max_bitrate=(int) ((bitrate+RC_MARGIN/2)/1000);
	params.rc.i_vbv_buffer_size=params.rc.i_vbv_max_bitrate;
	params.rc.f_vbv_buffer_init=0.5;
	params.rc.i_qp_constant = 50;
#endif
	params.rc.i_lookahead=0;
	/*enable this by config ?*/
	/*
	 params.i_keyint_max = (int)d->fps*d->keyframe_int;
	 params.i_keyint_min = (int)d->fps;
	 */
	params.b_repeat_headers=1;
	params.b_annexb=0;
	
	//these parameters must be set so that our stream is baseline
	params.analyse.b_transform_8x8 = 0;
	params.b_cabac = 0;
	params.i_cqm_preset = X264_CQM_FLAT;
	params.i_bframe = 0;
	params.analyse.i_weighted_pred = X264_WEIGHTP_NONE;
#endif	
	d->enc=x264_encoder_open(&params);
	if (d->enc==NULL) ms_error("Fail to create x264 encoder.");
	d->framenum=0;
	video_starter_init(&d->starter);
}

static void x264_nals_to_msgb(x264_nal_t *xnals, int num_nals, MSQueue * nalus){
	int i;
	mblk_t *m;
	/*int bytes;*/
	for (i=0;i<num_nals;++i){
		m=allocb(xnals[i].i_payload+10,0);
		
		memcpy(m->b_wptr,xnals[i].p_payload+4,xnals[i].i_payload-4);
		m->b_wptr+=xnals[i].i_payload-4;
		if (xnals[i].i_type==7) {
			ms_message("A SPS is being sent.");
		}else if (xnals[i].i_type==8) {
			ms_message("A PPS is being sent.");
		}
		ms_queue_put(nalus,m);
	}
}

static void enc_process(MSFilter *f){
	EncData *d=(EncData*)f->data;
	if(!d)
	{
		LOGD("init faile return\n");
	}
	uint32_t ts=f->ticker->time*90LL;
	mblk_t *im;
	MSPicture pic;
	MSQueue nalus;
	ms_queue_init(&nalus);
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		if (ms_yuv_buf_init_from_mblk(&pic,im)==0){
			x264_picture_t xpic;
			x264_picture_t oxpic;
			x264_nal_t *xnals=NULL;
			int num_nals=0;

			memset(&xpic, 0, sizeof(xpic));
			memset(&oxpic, 0, sizeof(oxpic));
			LOGD("w=%d h=%d\n",pic.w,pic.h);
			/*send I frame 2 seconds and 4 seconds after the beginning */
			if (video_starter_need_i_frame(&d->starter,f->ticker->time))
				d->generate_keyframe=TRUE;
//			d->generate_keyframe=TRUE;

			if (d->generate_keyframe){
				xpic.i_type=X264_TYPE_IDR;
				d->generate_keyframe=FALSE;
			}else xpic.i_type=X264_TYPE_AUTO;

			xpic.i_qpplus1=0;
			xpic.i_pts=d->framenum;
			xpic.param=NULL;
			xpic.img.i_csp=X264_CSP_I420;
			xpic.img.i_plane=2;
			xpic.img.i_stride[0]=pic.strides[0];
			xpic.img.i_stride[1]=pic.strides[1];
			xpic.img.i_stride[2]=pic.strides[2];
			xpic.img.i_stride[3]=0;

			xpic.img.plane[0]=pic.planes[0];
			xpic.img.plane[1]=pic.planes[1];
			xpic.img.plane[2]=pic.planes[2];
			xpic.img.plane[3]=0;
            
			if (x264_encoder_encode(d->enc,&xnals,&num_nals,&xpic,&oxpic)>=0){
				x264_nals_to_msgb(xnals,num_nals,&nalus);
				/*if (num_nals == 0)	ms_message("Delayed frames info: current=%d max=%d\n", 
					x264_encoder_delayed_frames(d->enc),
					x264_encoder_maximum_delayed_frames(d->enc));
				*/
				rfc3984_pack(d->packer,&nalus,f->outputs[0],ts);
				d->framenum++;
				if (d->framenum==0)
					video_starter_first_frame(&d->starter,f->ticker->time);
			}else{
				ms_error("x264_encoder_encode() error.");
			}
		}
		freemsg(im);
	}
}

static void enc_postprocess(MSFilter *f){
}

static int enc_set_br(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
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
	EncData *d=(EncData*)f->data;
	d->fps=*(float*)arg;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	*(float*)arg=d->fps;
	return 0;
}

static int enc_get_vsize(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	*(MSVideoSize*)arg=d->vsize;
	return 0;
}

static int enc_set_vsize(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;

	d->vsize=*(MSVideoSize*)arg;

	return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	const char *fmtp=(const char *)arg;
	char value[12];
	if (fmtp_get_value(fmtp,"packetization-mode",value,sizeof(value))){
		d->mode=atoi(value);
		ms_message("packetization-mode set to %i",d->mode);
	}
	return 0;
}

static int enc_req_vfu(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	d->generate_keyframe=TRUE;
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


static void dec_init(MSFilter *f){};
static void dec_preprocess(MSFilter * f){};
static void dec_postprocess(MSFilter * f){};
static void dec_process(MSFilter * f){};
static void dec_uninit(MSFilter *f){};


	

#ifndef _MSC_VER
  MSFilterDesc x264_dec_desc={
	 .id=MS_S5PV210_VIDEO_H264DEC_ID,
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

 MSFilterDesc x264_enc_desc={
	.id=MS_S5PV210_VIDEO_H264ENC_ID,
	.name="MSX264Enc",
	.text="A H264 encoder based on x264 project",
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

  MSFilterDesc x264_dec_desc={
	 MS_S5PV210_VIDEO_H264DEC_ID,
	 "MSX264Dec",
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

 MSFilterDesc x264_enc_desc={
	MS_S5PV210_VIDEO_H264ENC_ID,
	"MSX264Enc",
	"A H264 encoder based on x264 project",
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

#endif

MS2_PUBLIC void libmsx264_init(void){
	ms_filter_register(&x264_enc_desc);
	ms_message("ms264-" VERSION " plugin registered.");
}

