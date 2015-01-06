/* fmpeg4_avcodec.c
   It is sample code for ffmpeg application usage, to define AVCodec structure.
   Note it's not a part of fmpeg driver, but collect int the same folder
 */
#include <stdio.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <semaphore.h>       //sem_t
#include "CFimcMpeg4.h"
#include "MfcMpeg4Enc.h"

#define CommonH
#include "common.h"

//#define _SUB1_BITSTREAM_SUPPORT  //子码流1支持

#include "dvr_common_api.h"
#include "dvr_enc_api.h"
#include "gmavi_api.h"

int fmpeg4_enc_fd=0;
int dvr_fd = 0;
int enc_fd = 0;
unsigned char *bs_buf;
int enc_buf_size;
struct pollfd rec_fds;
#ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
 int sub1_bs_buf_offset;
 void sub1_bitstream_start(void);
 void sub1_bitstream_stop(void);
#endif

int video_rec_flag;
extern int audio_play_flag;
pthread_t video_rec_deal_thread;      //视频采集数据处理线程

void video_rec_deal_thread_func(void);

struct _SYNC sync_s;

struct timeval ref_time;  //基准时间,音频或视频第一帧

int curr_video_timestamp;       //当前已播放视频帧时间戳
int curr_audio_timestamp;       //当前已播放音频帧时间戳

void CheckRecVideoStatus(void);      //检查 视频录制状态，在gpio.c 里面检查

void StartRecVideo(int width, int height);
void StopRecVideo(void);
//为防止多次操作导致错误
int VideoRecIsStart=0;
int VideoRecInited = 0;
int VideoRecStarting = 0; //正在打开
int VideoRecStoping = 0; //正在关闭
//---------------------------------------------------------------------------
void CheckRecVideoStatus(void)      //检查 视频录制状态，在gpio.c 里面检查
{
  if(Local.Status == 0)
   if((VideoRecIsStart == 1)&&(VideoRecInited == 1))
    {
     printf("CheckRecVideoStatus...\n");
     StopRecVideo();
    }
}
//---------------------------------------------------------------------------
int mpeg4_encode_init(int enc_width, int enc_height, int framerate)
{
    int ret = 0, ch_num = 0;
    dvr_enc_channel_param   ch_param;
   #ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
    ReproduceBitStream sub1_param;
   #endif 
    EncParam_Ext3 enc_param_ext = {0};
    EncParam_Ext3 enc_param_ext1 = {0};    
    dvr_enc_control  enc_ctrl;
    dvr_enc_queue_get   data;
    unsigned char *buf;
    int buf_size;
    FuncTag tag;
    struct timeval t1,t2;  
    char tmp_str[128];

    dvr_fd = open("/dev/dvr_common", O_RDWR);   //open_dvr_common

    enc_fd = open("/dev/dvr_enc", O_RDWR);      //open_dvr_encode
    //set dvr encode source parameter
    ch_param.src.input_system = MCP_VIDEO_NTSC;
    ch_param.src.channel = ch_num;
    ch_param.src.enc_src_type = ENC_TYPE_FROM_CAPTURE;
    
    ch_param.src.dim.width = enc_width;//1280;
    ch_param.src.dim.height = enc_height;//720;
    
    ch_param.src.di_mode = LVFRAME_EVEN_ODD;
    ch_param.src.mode = LVFRAME_FRAME_MODE;
    ch_param.src.dma_order = DMAORDER_PACKET;
    ch_param.src.scale_indep = CAPSCALER_NOT_KEEP_RATIO;
    ch_param.src.input_system = MCP_VIDEO_NTSC;
    ch_param.src.color_mode = CAPCOLOR_YUV422;
    
    ch_param.src.vp_param.is_3DI = FALSE;
    ch_param.src.vp_param.is_denoise = FALSE;
    ch_param.src.vp_param.denoise_mode = GM3DI_FIELD;
    
    //set dvr encode main bitstream parameter
    ch_param.main_bs.enabled = DVR_ENC_EBST_ENABLE;
    ch_param.main_bs.out_bs = 0;
    ch_param.main_bs.enc_type = ENC_TYPE_MPEG;
    ch_param.main_bs.is_blocked = FALSE;
    ch_param.main_bs.en_snapshot = DVR_ENC_EBST_DISABLE;
    
    ch_param.main_bs.dim.width = enc_width;//1280;
    ch_param.main_bs.dim.height = enc_height;//720;
    
    //set main bitstream encode parameter
    ch_param.main_bs.enc.input_type = ENC_INPUT_H2642D;
    ch_param.main_bs.enc.frame_rate = framerate;//30;
    if(Local.RecPicSize == 2)       //D1
      ch_param.main_bs.enc.bit_rate = 1024*1024;
    else
      ch_param.main_bs.enc.bit_rate = 512*1024;
    ch_param.main_bs.enc.ip_interval = 20;//30;
    ch_param.main_bs.enc.init_quant = 25;
    ch_param.main_bs.enc.max_quant = 31;
    ch_param.main_bs.enc.min_quant = 1;
    ch_param.main_bs.enc.is_use_ROI = FALSE;
    ch_param.main_bs.enc.ROI_win.x = 0;
    ch_param.main_bs.enc.ROI_win.y = 0;
    ch_param.main_bs.enc.ROI_win.width = 320;
    ch_param.main_bs.enc.ROI_win.height = 240;
    
    //set main bitstream scalar parameter
    ch_param.main_bs.scl.src_fmt = SCALE_YUV422;
    ch_param.main_bs.scl.dst_fmt = SCALE_YUV422;
    ch_param.main_bs.scl.scale_mode = SCALE_LINEAR;
    ch_param.main_bs.scl.is_dither = FALSE;
    ch_param.main_bs.scl.is_correction = FALSE;
    ch_param.main_bs.scl.is_album = TRUE;
    ch_param.main_bs.scl.des_level = 0;

    //set main bitstream snapshot parameter
    ch_param.main_bs.snap.sample = JCS_yuv420;
    ch_param.main_bs.snap.RestartInterval = 0;
    ch_param.main_bs.snap.u82D = JENC_INPUT_MP42D;
    ch_param.main_bs.snap.quality = 70;

    //associate the ext. structure
    ch_param.main_bs.enc.ext_size = DVR_ENC_MAGIC_ADD_VAL(sizeof(enc_param_ext));
    ch_param.main_bs.enc.pext_data = &enc_param_ext;

    enc_param_ext.feature_enable = 0;      //CBR

  #ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
    //sub1 bitstream
    memset(&sub1_param, 0x0, sizeof(ReproduceBitStream));    
    sub1_param.enabled = DVR_ENC_EBST_ENABLE;
    sub1_param.out_bs = 1;
    sub1_param.enc_type = ENC_TYPE_MJPEG;
    sub1_param.is_blocked = FALSE;
    sub1_param.en_snapshot = DVR_ENC_EBST_DISABLE;
    
    sub1_param.dim.width = enc_width;
    sub1_param.dim.height = enc_height;
    
    sub1_param.enc.input_type = ENC_INPUT_H2642D;
    sub1_param.enc.frame_rate = 30;
    sub1_param.enc.bit_rate = 262144;
    sub1_param.enc.ip_interval = 15;
    sub1_param.enc.init_quant = 25;
    sub1_param.enc.max_quant = 51;
    sub1_param.enc.min_quant = 1;
    sub1_param.enc.is_use_ROI = FALSE;
    sub1_param.enc.ROI_win.x = 0;
    sub1_param.enc.ROI_win.y = 0;
    sub1_param.enc.ROI_win.width = 320;
    sub1_param.enc.ROI_win.height = 240;
    
    //set sub1 bitstream scalar parameter
    sub1_param.scl.src_fmt = SCALE_YUV422;
    sub1_param.scl.dst_fmt = SCALE_YUV422;
    sub1_param.scl.scale_mode = SCALE_LINEAR;
    sub1_param.scl.is_dither = FALSE;
    sub1_param.scl.is_correction = FALSE;
    sub1_param.scl.is_album = TRUE;
    sub1_param.scl.des_level = 0;
    
    //set sub1 bitstream snapshot parameter
    sub1_param.snap.sample = JCS_yuv420;   
    sub1_param.snap.RestartInterval = 0;   
    sub1_param.snap.u82D = JENC_INPUT_MP42D;   
    sub1_param.snap.quality = 70;   
    //associate the ext. structure                      
    sub1_param.enc.ext_size = DVR_ENC_MAGIC_ADD_VAL(sizeof(enc_param_ext1));
    sub1_param.enc.pext_data = &enc_param_ext1;

    enc_param_ext1.feature_enable &= ~DVR_ENC_MJPEG_FUNCTION;

    enc_param_ext1.MJ_quality = 70;//50;
    //end sub1 bitstream
  #endif

    ioctl(enc_fd, DVR_ENC_SET_CHANNEL_PARAM, &ch_param);

    ioctl(enc_fd, DVR_ENC_QUERY_OUTPUT_BUFFER_SIZE, &enc_buf_size);
  #ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
    ioctl(enc_fd, DVR_ENC_QUERY_OUTPUT_BUFFER_SUB1_BS_OFFSET, &sub1_bs_buf_offset);
  #endif

    printf("enc_buf_size = %d\n", enc_buf_size);
    bs_buf = (unsigned char*) mmap(NULL, enc_buf_size, PROT_READ|PROT_WRITE, 
                                          MAP_SHARED, enc_fd, 0);

  #ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
    ioctl(enc_fd, DVR_ENC_SET_SUB_BS_PARAM, &sub1_param);
  #endif

    /////////////////////////////////////////////////////////////////            
    //record start
    memset(&enc_ctrl, 0x0, sizeof(dvr_enc_control));
    enc_ctrl.command = ENC_START;
    enc_ctrl.stream = 0;
    ret = ioctl(enc_fd, DVR_ENC_CONTROL, &enc_ctrl);

    // set function tag paremeter to dvr graph level
    FN_RESET_TAG(&tag);
    FN_SET_REC_CH(&tag, ch_num);
    ioctl(dvr_fd, DVR_COMMON_APPLY, &tag);

   #ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
    //sub1_bitstream_start();
   #endif

    return 0;
}
//---------------------------------------------------------------------------
#ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
void sub1_bitstream_start(void)
{
    int ret = 0, ch_num = 0;
    FuncTag tag;        
    dvr_enc_control  enc_ctrl;
    //sub1 bitstream record start
    memset(&enc_ctrl, 0x0, sizeof(dvr_enc_control));
    enc_ctrl.command = ENC_START;
    enc_ctrl.stream = 1;
    ret = ioctl(enc_fd, DVR_ENC_CONTROL, &enc_ctrl);

    // set function tag paremeter to dvr graph level
    //printf("tag.func 1= %d, tag.cas_ch = %d, tag.lv_ch = %d, tag.rec_ch = %d, tag.pb_ch = %d\n", tag.func, tag.cas_ch, tag.lv_ch, tag.rec_ch, tag.pb_ch);
    FN_RESET_TAG(&tag);
    FN_SET_SUB1_REC_CH(&tag, ch_num);
    ioctl(dvr_fd, DVR_COMMON_APPLY, &tag);
}
//---------------------------------------------------------------------------
void sub1_bitstream_stop(void)
{
    int ret = 0, ch_num = 0;
    FuncTag tag;        
    dvr_enc_control  enc_ctrl;
    //sub1 bitstream record stop
    memset(&enc_ctrl, 0x0, sizeof(dvr_enc_control));
    enc_ctrl.command = ENC_STOP;
    enc_ctrl.stream = 1;
    ret = ioctl(enc_fd, DVR_ENC_CONTROL, &enc_ctrl);

    // set function tag paremeter to dvr graph level
    FN_RESET_TAG(&tag);
    FN_SET_SUB1_REC_CH(&tag, ch_num);
    ioctl(dvr_fd, DVR_COMMON_APPLY, &tag);
}
#endif
//---------------------------------------------------------------------------
int mpeg4_encode_uninit(void)
{
  int ch_num = 0;
  dvr_enc_control  enc_ctrl;
  FuncTag tag;       
  //record stop
  memset(&enc_ctrl, 0x0, sizeof(dvr_enc_control));
  enc_ctrl.stream = 0;
  enc_ctrl.command = ENC_STOP;
  ioctl(enc_fd, DVR_ENC_CONTROL, &enc_ctrl);

  FN_RESET_TAG(&tag);
  FN_SET_REC_CH(&tag, ch_num);
  ioctl(dvr_fd, DVR_COMMON_APPLY, &tag);
  
 #ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
    FN_RESET_TAG(&tag);
    FN_SET_SUB1_REC_CH(&tag, ch_num);
    ioctl(dvr_fd, DVR_COMMON_APPLY, &tag); 
  //sub1_bitstream_stop();
 #endif

  munmap((void*)bs_buf, enc_buf_size);

  printf("mpeg4_encode_uninit\n");

  close(enc_fd);      //close_dvr_encode
  close(dvr_fd);      //close_dvr_common
  enc_fd = 0;
  dvr_fd = 0;
}

void InitRecVideo(void)
{
#if 0 //by a20
	if(CamInit(2, 640, 480) < 0)
	//if(mpeg4_encode_init(width, height, framerate) !=0 )
	{
		P_Debug("mpeg4_encode_init error!\\n");
		VideoRecStarting = 0;
		return;
	}
#endif
return;

}
//---------------------------------------------------------------------------

static int VideoFd = -1;
VIDEO_MPEG4ENC_ST *pstVideo = NULL;
void *Y;
void *UV;

void StartRecVideo(int width, int height)
{
	pthread_attr_t attr;
	int i, j;
	int framerate;
	return;
#if 0	//by a20

	P_Debug("VideoRecStarting 0\n");
	//20110928 下面一行 在 多分机的情况下会产生错误，去掉为好
	//StopRecVideo();
//	sleep(3);
	if(VideoRecIsStart == 0)
	{
		if(VideoRecStarting == 1)
		{
			P_Debug("VideoRecStarting 1\n");
			return;
		}
		VideoRecStarting = 1; //正在打开
		VideoRecInited = 0;
		Local.nowvideoframeno = 1;
		if((width == D1_W)||(height == D1_H))
		{
			Local.RecPicSize = 2;      //720*240
			framerate = 20;
		}
		else
		{
			Local.RecPicSize = 1;      //352*288
			framerate = 20;
		}
//		usleep(1000*1000);

		P_Debug("VideoRecStarting 2\n");
		//开启补光函数
		//OpenFillLight_Func();
		/*usleep(400*1000);     //摄像头上电，等待视频稳定
		if((width == 720)&&(height == 480))
		{
			mpeg4_encode_init(1280, 720, framerate);
			mpeg4_encode_uninit();
		}
		P_Debug("VideoRecStarting 3\n");*/
		if(VideoFd == -1)
		{
			//视频采集及编码
			VideoFd = CamInit(2, width, height);
			if(VideoFd < 0)
			//if(mpeg4_encode_init(width, height, framerate) !=0 )
			{
				P_Debug("mpeg4_encode_init error!\\n");
				VideoRecStarting = 0;
				return;
			}
		}

		pstVideo = (VIDEO_MPEG4ENC_ST *)malloc(sizeof(VIDEO_MPEG4ENC_ST));
		if(pstVideo == NULL)
		{
			P_Debug("malloc for pstVideo failed!\n");
		}

		pstVideo->VideoPlaner.Height = height;
		pstVideo->VideoPlaner.Width = width;
		pstVideo->VideoPlaner.YSize = ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height));
		pstVideo->VideoPlaner.CSize = ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height/2));
	//	pstVideo->VideoPlaner.YSize = Height * Width;
	//	pstVideo->VideoPlaner.CSize = Height * Width / 2;
		pstVideo->Frames = framerate;
		pstVideo->Quality = 30;
		pstVideo->Bitrate = 1152000;

		//进行编码器初始化
		if(MfcEncMpeg4Init(pstVideo) != 0)
		{
			P_Debug("mpeg4enc init failed!\n");
		}

		P_Debug("VideoRecStarting 3\n");

		MfcGetInputBuf(pstVideo, &Y, &UV);

		pthread_mutex_init (&sync_s.video_rec_lock, NULL);

		video_rec_flag = 1;   //视频采集标志
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&video_rec_deal_thread,&attr,(void *)video_rec_deal_thread_func,NULL);
		if (video_rec_deal_thread == 0)
		{
			P_Debug("con't create video recv deal thread \n");
			pthread_attr_destroy(&attr);
			VideoRecStarting = 0;
			return;
		}

		P_Debug("VideoRecStarting 4\n");
		VideoRecStarting = 0; //正在打开
		VideoRecInited = 1;
		VideoRecIsStart = 1;
		Local.CaptureVideoStartFlag = 1;      //捕获视频开始标志     20101109  xu
		Local.CaptureVideoStartTime = 0;      //捕获视频开始计数
	}
	else
	{
		if(DebugMode == 1)
			P_Debug("repeat VideoRecStart\n");
	}
#endif
}
//---------------------------------------------------------------------------
void StopRecVideo(void)
{
	int i;
	int delaytime;
	delaytime=3;
#if 0 //by a20
	P_Debug("VideoRecStoping 0\n");
	if(VideoRecIsStart == 1)
	{
		if(VideoRecStoping == 1)
		{
			P_Debug("VideoRecStoping 1\n");
			return;
		}
		VideoRecStoping = 1; //正在关闭

		video_rec_flag = 0;   //视频采集标志
		//sleep(delaytime);
		//usleep(delaytime*1000);
		/*if(pthread_cancel(video_rec_deal_thread) ==0)
			P_Debug("video_rec_deal_thread cancel success\n");
		else
			P_Debug("video_rec_deal_thread cancel fail\n");
		usleep(delaytime*1000);*/
		pthread_mutex_destroy(&sync_s.video_rec_lock);

		//P_Debug("&&&&&& VideoRecStoping 4\n");
		//CamClose();
		//mpeg4_encode_uninit();

		//关闭补光函数
		//CloseFillLight_Func();

		VideoRecStoping = 0; //正在关闭
		VideoRecIsStart = 0;
	}
	else
	{
		if(DebugMode == 1)
			P_Debug("repeat VideoRecStop\n");
	}
#endif
}
//---------------------------------------------------------------------------
//MPEG4编码及发送线程
#if 0
void video_rec_deal_thread_func(void)
{
	int ret = 0;
	struct timeval tv;
	int i,j;
	int TotalPackage; //总包数
	int FrameNo = 0;
	unsigned char mpeg4_out[1600];
	int RemotePort;
	char RemoteHost[20];
	uint32_t nowtime,prevtime;
	//通话数据结构
	struct talkdata1 talkdata;
	dvr_enc_queue_get   data;
	unsigned char *buf;
	unsigned char cambuf[1024*500];
	int buf_size;
	int CanCapturePic;
	FILE *fd;
	unsigned char *pMpeg4Buf = NULL;
	void *pVideoBuf = NULL;
	char *pMpeg4 = NULL;
	char VideoBuffer[500000] = {0};
	int BufLen = 50000;
	int Mpeg4Len = -1;
	int VideoLen = -1;
	int index;
//	unsigned char isFirstSend = 0;
//	int iPos = 0;

//	FILE *fp_mpeg4 = fopen("/data/data/test.mpeg4", "w+");
//	if(fp_mpeg4 == NULL)
//	{
//		printf("CamCapture open mpeg4 file faild! \n");
//	}

//	fwrite(pstVideo->header, 1, pstVideo->headerSize, fp_mpeg4);

	if(DebugMode == 1)
		P_Debug("create video recv deal thread：\n" );
	while(video_rec_flag == 1)
	{
      //加锁
    //pthread_mutex_lock(&sync_s.video_rec_lock);
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
#ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
     CanCapturePic = 0;
     //printf("Local.RecordPic = %d  Local.RecPicSize = %d.........\n", Local.RecordPic, Local.RecPicSize);
     if((Local.RecordPic == 1)&&(Local.RecPicSize == 1))
      {
       //存储第75帧
       Local.IFrameCount ++;
       if(Local.IFrameCount > Local.IFrameNo)
        {
         CanCapturePic = 1;
         printf("sub1_bitstream_start.........\n");
         //fd = fopen ("dddd.jpg", "wb+" );
         sub1_bitstream_start();
        }
      }
#endif
#endif

		//头部
		memcpy(mpeg4_out, UdpPackageHead, 6);
		//命令
		if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
				||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))  //对讲
		{
			mpeg4_out[6] = VIDEOTALK;
		}
		if((Local.Status == 3)||(Local.Status == 4))  //监视
		{
			mpeg4_out[6] = VIDEOWATCH;
		}

		mpeg4_out[7] = 1;
		//子命令
		if((Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5)||(Local.Status == 7)||(Local.Status == 9))  //本机为主叫方
		{
			mpeg4_out[8] = CALLUP;
			memcpy(talkdata.HostAddr, LocalCfg.Addr, 20);
			memcpy(talkdata.HostIP, LocalCfg.IP, 4);
			memcpy(talkdata.AssiAddr, Remote.Addr[0], 20);
			if(Remote.DenNum == 1)
				memcpy(talkdata.AssiIP, Remote.IP[0], 4);
			else
				memcpy(talkdata.AssiIP, Remote.GroupIP, 4);
		}
		if((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6)||(Local.Status == 8)||(Local.Status == 10))  //本机为被叫方
		{
			mpeg4_out[8] = CALLDOWN;
			memcpy(talkdata.HostAddr, Remote.Addr[0], 20);
			if(Remote.DenNum == 1)
				memcpy(talkdata.HostIP, Remote.IP[0], 4);
			else
				memcpy(talkdata.HostIP, Remote.GroupIP, 4);
			memcpy(talkdata.AssiAddr, LocalCfg.Addr, 20);
			memcpy(talkdata.AssiIP, LocalCfg.IP, 4);
		}
		//时间戳  应该放在录制前，但7113复位会导致时间戳乱
		gettimeofday(&tv, NULL);
		//第一帧,设定起始时间戳
		if((ref_time.tv_sec ==0)&&(ref_time.tv_usec ==0))
		{
			ref_time.tv_sec = tv.tv_sec;
			ref_time.tv_usec = tv.tv_usec;
		}
		nowtime = (tv.tv_sec - ref_time.tv_sec) *1000 + (tv.tv_usec - ref_time.tv_usec)/1000;

		//时间戳
		talkdata.timestamp = nowtime;
		//帧序号
		talkdata.Frameno = FrameNo;
		FrameNo ++;

		// prepare to select(or poll)
//		rec_fds.fd = enc_fd;
//		rec_fds.revents = 0;
//#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
//		if(CanCapturePic == 0)
//			rec_fds.events = POLLIN_MAIN_BS;
//		else
//			rec_fds.events = POLLIN_MAIN_BS | POLLIN_SUB1_BS;
//#else
//		rec_fds.events = POLLIN_MAIN_BS;
//#endif
//
//		poll(&rec_fds, 1, 500);
//
//		if (!(rec_fds.revents & POLLIN_MAIN_BS))
//			continue;

#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
#ifdef _SUB1_BITSTREAM_SUPPORT  //子码流1支持
		   //printf("CanCapturePic = %d, rec_fds.revents = 0x%X, rec_fds.events = 0x%X\n", CanCapturePic, rec_fds.revents, rec_fds.events);
		   if(CanCapturePic == 1)
			if (rec_fds.revents & POLLIN_SUB1_BS)
			 {
					//printf("rec_fds.revents & POLLIN_SUB1_BS\n");
					ret = ioctl(enc_fd, DVR_ENC_QUEUE_GET_SUB1_BS, &data);
					if(ret < 0)
						continue;

					buf = bs_buf + sub1_bs_buf_offset + data.bs.offset;
					buf_size = data.bs.length;

					CanCapturePic = 0;
					memcpy(Local.yuv, (unsigned char *)buf, buf_size);
					Local.Pic_Size = buf_size;
					Local.IFrameCount = 0;
					Local.RecordPic = 0;
					Local.HavePicRecorded = 1;  //有照片已录制
					Local.recpic_tm_t = curr_tm_t;

					//fwrite (buf , 1 , buf_size , fd);
					//fclose(fd);
					ioctl(enc_fd, DVR_ENC_QUEUE_PUT, &data);

					sub1_bitstream_stop();
			 }
#endif
#endif
		//printf("video_rec_deal_thread_func 11\n");
		//get dataout buffer
		/*ret = ioctl(enc_fd, DVR_ENC_QUEUE_GET, &data);
		if(ret < 0)
			continue;

		buf = bs_buf + data.bs.offset;
		buf_size = data.bs.length;
		ioctl(enc_fd, DVR_ENC_QUEUE_PUT, &data);*/

//		buf = cambuf;
//		buf_size = CamCapture(buf);
//		if(buf_size <= 0)
//		{
//			buf_size = 0;
//			P_Debug("buf_size<=0 \n" );
//		}

		index = CamGetFrame(&pVideoBuf, &VideoLen);
		if(VideoLen < 0)
		{
			P_Debug("error:Mpeg4Len < 0 \n");
		}

		if(CamReleaseFrame(index) < 0)
		{
			P_Debug("release frame failed!\n");
		}

		memcpy(Y, pVideoBuf, pstVideo->VideoPlaner.YSize);
		memcpy(UV, pVideoBuf + pstVideo->VideoPlaner.YSize, pstVideo->VideoPlaner.CSize);

		if(MfcMpeg4EncProcess(pstVideo, &pMpeg4, &Mpeg4Len) != 0)
		{
			P_Debug("mpeg4 enc failed!\n");
		}

//		fwrite(pMpeg4, 1, Mpeg4Len, fp_mpeg4);

		memcpy(VideoBuffer, pstVideo->header, pstVideo->headerSize);
		memcpy(VideoBuffer + pstVideo->headerSize, pMpeg4, Mpeg4Len);

//		if(iPos == 5)
//		{
//			fwrite(VideoBuffer, 1, Mpeg4Len + pstVideo->headerSize, fp_mpeg4);
//			if(fp_mpeg4 != NULL)
//			{
//				fclose(fp_mpeg4);
//				fp_mpeg4 = NULL;
//
//				return ;
//			}
//			iPos++;
//		}

//		if(isFirstSend == 0)
//		{
//			buf = pstVideo;
//			buf_size = pstVideo->headerSize;
//			isFirstSend = 1;
//		}
//		else
//		{
			buf = VideoBuffer;
			buf_size = Mpeg4Len + pstVideo->headerSize;

//			sprintf(Local.DebugInfo, "xxxx Mpeg4Len %d\n", Mpeg4Len );
//						P_Debug(Local.DebugInfo);
//		}

		//帧数据长度
		talkdata.Framelen = buf_size;
		//printf("data.bs.is_keyframe 3 = %d\n", data.bs.is_keyframe);
		//I帧
		//if(data.bs.is_keyframe == 1)
		{
//			sprintf(Local.DebugInfo, "Local.RecPicSize %d\n", Local.RecPicSize );
//			P_Debug(Local.DebugInfo);
			//数据类型
			if(Local.RecPicSize == 2)
				talkdata.DataType = 4;  //720*480 I帧
			else
				talkdata.DataType = 2;  //352*240 I帧
		}
		/*else
		{
			//数据类型
			if(Local.RecPicSize == 2)
				talkdata.DataType = 5;    //720*480 P帧
			else
				talkdata.DataType = 3;  //352*240 P帧
		}*/

		//   if(talkdata.Framelen > 20000)
		//     printf("talkdata.Framelen = %d, talkdata.DataType = %d\n", talkdata.Framelen, talkdata.DataType);

		talkdata.PackLen = PACKDATALEN;
		//总包数
		if((talkdata.Framelen%talkdata.PackLen)==0)
			TotalPackage=talkdata.Framelen/talkdata.PackLen;
		else
			TotalPackage=talkdata.Framelen/talkdata.PackLen+1;
		talkdata.TotalPackage = TotalPackage;
//		sprintf(Local.DebugInfo, "Framelen = %d, TotalPackage = %d \n", talkdata.Framelen, TotalPackage);
//		P_Debug(Local.DebugInfo);
		sprintf(RemoteHost, "%d.%d.%d.%d\0",Remote.DenIP[0], Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);

		for(j=1; j<=TotalPackage; j++)         //包的顺序从大到小P_Debug
		{
			if(j==TotalPackage)
			{
				talkdata.CurrPackage = j;          //当前包
				talkdata.Datalen = buf_size-(j-1)*talkdata.PackLen;     //数据长度
				memcpy(mpeg4_out + 9, &talkdata, sizeof(talkdata));
				memcpy(mpeg4_out +DeltaLen, buf + (j-1)*talkdata.PackLen, (buf_size-(j-1)*talkdata.PackLen));
				//UDP发送
				if(Remote.isDirect == 0)  //直通呼叫
					RemotePort = RemoteVideoPort;
				else                      //中转呼叫
					RemotePort = RemoteVideoServerPort;
				UdpSendBuff(m_VideoSocket, RemoteHost, RemotePort, mpeg4_out , DeltaLen + (buf_size-(j-1)*talkdata.PackLen));
			}
			else
			{
				talkdata.CurrPackage = j;           //当前包
				talkdata.Datalen = PACKDATALEN;              //数据长度
				memcpy(mpeg4_out + 9, &talkdata, sizeof(talkdata));
				memcpy(mpeg4_out +DeltaLen, buf + (j-1)*talkdata.PackLen, talkdata.PackLen);
				//UDP发送
				if(Remote.isDirect == 0)  //直通呼叫
					RemotePort = RemoteVideoPort;
				else                      //中转呼叫
					RemotePort = RemoteVideoServerPort;
				// if(Local.nowvideoframeno%5 != 0)
				UdpSendBuff(m_VideoSocket, RemoteHost, RemotePort, mpeg4_out , DeltaLen + talkdata.PackLen);
			}
		}
		//解锁
		//pthread_mutex_unlock(&sync_s.video_rec_lock);
	}

//	if(fp_mpeg4 != NULL)
//	{
//		fclose(fp_mpeg4);
//		fp_mpeg4 = NULL;
//	}

//	if(VideoFd > 0)
//	{
//		if(CamClose() < 0)
//		{
//			printf("CamClose faild! \n");
//		}
//		P_Debug("CamClose success!\n");
//	}

	MfcMpeg4EncExit(pstVideo);
	if(pstVideo != NULL)
	{
		free(pstVideo);
	}
}
#endif
//---------------------------------------------------------------------------
