#include "mediastreamer2/msfilter.h"



#ifdef CODEC_AUDIO_GSM
extern MSFilterDesc ms_gsm_enc_desc;
extern MSFilterDesc ms_gsm_dec_desc;
#endif
extern MSFilterDesc ms_alaw_dec_desc;
extern MSFilterDesc ms_alaw_enc_desc;
extern MSFilterDesc ms_ulaw_dec_desc;
extern MSFilterDesc ms_ulaw_enc_desc;
extern MSFilterDesc ms_rtp_send_desc;
extern MSFilterDesc ms_rtp_recv_desc;
extern MSFilterDesc ms_dtmf_gen_desc;
extern MSFilterDesc ms_ice_desc;
extern MSFilterDesc ms_tee_desc;
extern MSFilterDesc ms_conf_desc;
extern MSFilterDesc ms_join_desc;
extern MSFilterDesc ms_volume_desc;
extern MSFilterDesc ms_void_sink_desc;
extern MSFilterDesc ms_equalizer_desc;
extern MSFilterDesc ms_channel_adapter_desc;
extern MSFilterDesc ms_audio_mixer_desc;
extern MSFilterDesc ms_itc_source_desc;
extern MSFilterDesc ms_itc_sink_desc;
extern MSFilterDesc ms_tone_detector_desc;
//extern MSFilterDesc ms_speex_dec_desc;
//extern MSFilterDesc ms_speex_enc_desc;
//extern MSFilterDesc ms_speex_ec_desc;
extern MSFilterDesc ms_file_player_desc;
extern MSFilterDesc ms_file_rec_desc;

extern MSFilterDesc ms_resample_desc;

//extern MSFilterDesc alsa_write_desc;
//extern MSFilterDesc alsa_read_desc;

#ifdef _YK_XT8126_
extern MSFilterDesc YK8126AudioReadDesc;
extern MSFilterDesc YK8126AudioWriteDesc;
//#elif _YK_IMX27_AV_
//extern MSFilterDesc Imx27AudioReadDesc;
//extern MSFilterDesc Imx27AudioWriteDesc;
#else
//extern MSFilterDesc oss_read_desc;
//extern MSFilterDesc oss_write_desc;
#endif


//extern MSFilterDesc ms_v4l2_desc;
extern MSFilterDesc ms_mire_desc;
extern MSFilterDesc ms_ext_display_desc;


#ifdef _YK_S5PV210_
#ifdef _HAS_X264_
	extern MSFilterDesc x264_enc_desc;
	extern MSFilterDesc x264_dec_desc;
#else if _MFC210_
	extern MSFilterDesc mfc264_enc_desc;
	extern MSFilterDesc mfc264_dec_desc;
#endif
#endif


#ifdef _YK_XT8126_
extern MSFilterDesc ms_V8126_desc;			//Xingtel 8126 Video Capture

#ifdef _CODEC_MPEG4_
extern MSFilterDesc ms_V8126Mpeg4Enc_Desc;	//Xingtel 8126 MPEG4 Encoder
extern MSFilterDesc ms_V8126Mpeg4Dec_Desc;	//Xingtel 8126 MPEG4 Decoder
#endif

#ifdef _CODEC_H264_
extern MSFilterDesc ms_V8126H264Enc_Desc;	//Xingtel 8126 H264 Encoder
extern MSFilterDesc ms_V8126H264Dec_Desc;	//Xingtel 8126 H264 Decoder
#endif

#endif

#ifdef _YK_IMX27_AV_
extern MSFilterDesc ms_VIMX27_desc;			//Freescale IMX27 Video Capture

#ifdef _CODEC_MPEG4_
extern MSFilterDesc ms_VIMX27Mpeg4Enc_Desc;	//Freescale IMX27 MPEG4 Encoder
extern MSFilterDesc ms_VIMX27Mpeg4Dec_Desc;	//Freescale IMX27 MPEG4 Decoder
#endif

#ifdef _CODEC_H264_
extern MSFilterDesc ms_VIMX27H264Enc_Desc;	//Freescale IMX27 H264 Encoder
extern MSFilterDesc ms_VIMX27H264Dec_Desc;	//Freescale IMX27 H264 Decoder
#endif

#endif

extern MSFilterDesc ms_dtmf_inband_desc;    //20120525 dtmf inband 


MSFilterDesc * ms_filter_descs[]={
#ifdef CODEC_AUDIO_GSM
&ms_gsm_enc_desc,
&ms_gsm_dec_desc,
#endif
&ms_alaw_dec_desc,
&ms_alaw_enc_desc,
&ms_ulaw_dec_desc,
&ms_ulaw_enc_desc,
&ms_rtp_send_desc,
&ms_rtp_recv_desc,
&ms_dtmf_gen_desc,
&ms_ice_desc,
&ms_tee_desc,
&ms_conf_desc,
&ms_join_desc,
&ms_volume_desc,
&ms_void_sink_desc,
&ms_equalizer_desc,
&ms_channel_adapter_desc,
&ms_audio_mixer_desc,
&ms_itc_source_desc,
&ms_itc_sink_desc,
&ms_tone_detector_desc,
//&ms_speex_dec_desc,
//&ms_speex_enc_desc,
//&ms_speex_ec_desc,
&ms_file_player_desc,
&ms_file_rec_desc,

&ms_resample_desc,

//annonate by chensq
//&alsa_write_desc,
//&alsa_read_desc,


#ifdef _YK_XT8126_
&YK8126AudioReadDesc,
&YK8126AudioWriteDesc,
//#elif _YK_IMX27_AV_
//&Imx27AudioReadDesc,
//&Imx27AudioWriteDesc,
#else
//&oss_read_desc,
//&oss_write_desc,
#endif

//&ms_v4l2_desc,
&ms_mire_desc,
&ms_ext_display_desc,

#ifdef _YK_XT8126_
&ms_V8126_desc,							//Xingtel 8126 Video Capture

#if defined(_CODEC_MPEG4_)
&ms_V8126Mpeg4Enc_Desc,					//Xingtel 8126 MPEG4 Encoder
&ms_V8126Mpeg4Dec_Desc,					//Xingtel 8126 MPEG4 Decoder
#endif

#if defined(_CODEC_H264_)
&ms_V8126H264Enc_Desc,					//Xingtel 8126 H264  Encoder
&ms_V8126H264Dec_Desc,				    //Xingtel 8126 H264  Decoder
#endif

#endif

#ifdef _YK_S5PV210_
#ifdef _HAS_X264_
	&x264_enc_desc,
	&x264_dec_desc,
#else if _MFC210_
	&mfc264_enc_desc,
	&mfc264_dec_desc,
#endif
#endif


#ifdef _YK_IMX27_AV_
&ms_VIMX27_desc,						//Freescale IMX27 Video Capture

#if defined(_CODEC_MPEG4_)
&ms_VIMX27Mpeg4Enc_Desc,			#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>	//Freescale IMX27 MPEG4 Encoder
&ms_VIMX27Mpeg4Dec_Desc,				//Freescale IMX27 MPEG4 Decoder
#endif

#if defined(_CODEC_H264_)
&ms_VIMX27H264Enc_Desc,					//Freescale IMX27 H264  Encoder
&ms_VIMX27H264Dec_Desc,				    //Freescale IMX27 H264  Decoder
#endif

#endif

&ms_dtmf_inband_desc,                  //20120525 dtmf inband 
NULL
};

