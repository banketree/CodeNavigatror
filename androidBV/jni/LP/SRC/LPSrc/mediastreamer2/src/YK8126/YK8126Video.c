//========================================================================================================
/******************************************************************************************
*File	:V8126_set_devfile
*Auth	:lichengda
*Version:
*Desc	:
********************************************************************************************/
#ifdef _YK_XT8126_

#include "YK8126Video.h"
#include "YK8126VideoBV.h"
#include "YK8126Gpio.h"
#include "rfc3984.h"
#include "msfilter.h"
#include "msvideo.h"
#include "msticker.h"
#include "mswebcam.h"
#include "mscommon.h"
#include "dvr_enc_api.h"
#include "dvr_common_api.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "LPIntfLayer.h"
#include "LOGIntfLayer.h"



int 	fps8126;
int 	br8126;


static int dvr_fd = -1;
static int enc_fd = -1;

typedef struct V8126State{
    ms_thread_t     thread; 		//
    ms_mutex_t      mutex;  		//

#ifdef _YK_MEDIA_ABRC_
    ms_mutex_t      update_mutex;
#endif


    char*           devname;   		//
	int				fdDvr;
	int				fdEnc;
	int				fdIoLight;
    int             hvCapture;      //
    bool_t          bCaptureStart;  //
	bool_t			bauto_started;
	unsigned char	*mmapbuf;		//��Ƶ�ڴ�ӳ������ʼ��ַ
	int				msize;			//��Ƶ�ڴ�ӳ�����
    queue_t         captureQ;       /**<! add capture video data to this message queue. */
}V8126State;


/*************************************************
*Name	:V8126DoMunmap
*Input	:
*Output	:
*Desc	:ֹͣ�ɼ���ݣ������ڴ�ӳ��
**************************************************/
static void V8126DoMunmap(V8126State *s)
{
	int ch_num = 0;
	dvr_enc_control  enc_ctrl;
	FuncTag tag;

	//record stop
	printf("V8126DoMunmap:record stop\n");
	memset(&enc_ctrl, 0x0, sizeof(dvr_enc_control));
	enc_ctrl.stream 	= 0;
	enc_ctrl.command 	= ENC_STOP;
	ioctl(s->fdEnc, DVR_ENC_CONTROL, &enc_ctrl);


	FN_RESET_TAG(&tag);
	FN_SET_REC_CH(&tag, ch_num);
	ioctl(s->fdDvr, DVR_COMMON_APPLY, &tag);
	//munmap
	/*
	printf("V8126DoMunmap:munmap mem\n");
	if(s->mmapbuf != NULL)
	{
		if(munmap((void*)s->mmapbuf,s->msize)<0)
		{
			printf("V8126DoMunmap:Fail to unmap\n");
		}
		s->mmapbuf = NULL;
	}
	s->msize = 0;
	*/
}
/*************************************************
*Name	:V8126GrapImageMap
*Input	:
*Output	:
*Desc	:
**************************************************/
static mblk_t * V8126GrapImageMap(V8126State *s)
{
	int 				err = 0;
	mblk_t				*ret;
	struct pollfd 		rec_fds;
	dvr_enc_queue_get   data;

	printf("��ʼ�ɼ�֡���\n");

	//���������ɼ����
	rec_fds.fd 		= s->fdEnc;
	rec_fds.revents = 0;
	rec_fds.events 	= POLLIN_MAIN_BS;

	poll(&rec_fds, 1, 500);

	if (!(rec_fds.revents & POLLIN_MAIN_BS))
		return NULL;

	// get dataout buffer
	err = ioctl(s->fdEnc, DVR_ENC_QUEUE_GET, &data);
	if(err < 0)
		return NULL;

	ret = esballoc(s->mmapbuf + data.bs.offset,data.bs.length,0,NULL);
	ret->b_wptr+=data.bs.length;

	ioctl(s->fdEnc, DVR_ENC_QUEUE_PUT, &data);

	return ret;
}

#ifdef _CODEC_H264_

int find_start_code(unsigned char *buf,int *start_code_len,int size)
{
	int i=0,j=0;
	while( i<(size-4) )
	{
		j=i;
		if(*(buf+j) == 0)  // first zero
		{
			if(*(buf+j+1) == 0 ) //second zero
			{
				if(*(buf+j+2) == 1)  //three 1
				{
					//printf("%d:3,",i);
					*start_code_len = 3;
					return i;

					//i+=3;
					//continue;

				}
				else if(*(buf+j+2) == 0) //three zero
				{
					if(*(buf+j+3) == 1) // four 1
					{
						//printf("%d:4,",i);
						*start_code_len = 4;
						return i;
						//i+=4;
						//continue;
					}
				}
			}

		}
		i++;
	}
	//printf("\n");
	return -1;
}
/*************************************************
*Name	:V8126Capturethread
*Input	:
*Output	:
*Desc	:�ɼ��߳�
**************************************************/
static void* V8126Capturethread(void * data)
{
	V8126State 	*s    	= (V8126State *)data;
	int 		ret 	= 0;
	int 		pos		=-1;
	int start_code_len = 0;
	unsigned char *pbuf;
	int buf_len;

	dvr_enc_queue_get 	encdata;
	struct		pollfd 	rec_fds;

	//sleep(5);
	printf("The thread for video cap begin to run\n");
	unsigned char        NALU_BEGIN_FLAG1[4]={0x00,0x00,0x00,0x01};
	unsigned char        NALU_END_FLAG1[4]  ={0x00,0x00,0x00,0x01};

	unsigned char        NALU_BEGIN_FLAG2[4]={0x00,0x00,0x01};
	unsigned char        NALU_END_FLAG2[4]  ={0x00,0x00,0x01};

	if(s->mmapbuf == (void *)-1)			//�ڴ�ӳ����?�˳��߳�
	{
		printf("MMAP error,out the thread\n");
		ms_thread_exit(NULL);
	}

	while(s->bCaptureStart)
	{
		mblk_t *m;

		//m = V8126GrapImageMap(s);
		//printf("���������ɼ����\n");

		//���������ɼ����
#ifdef _YK_MEDIA_ABRC_
		ms_mutex_lock(&s->update_mutex);
#endif
		rec_fds.fd 		= s->fdEnc;
		rec_fds.revents = 0;
		rec_fds.events 	= POLLIN_MAIN_BS;

		poll(&rec_fds, 1, 500);

		if (!(rec_fds.revents & POLLIN_MAIN_BS))
			continue;


		ret = ioctl(s->fdEnc, DVR_ENC_QUEUE_GET, &encdata);
		if(ret < 0)
			continue;
		pbuf = s->mmapbuf + encdata.bs.offset;
		buf_len=encdata.bs.length;

		ioctl(s->fdEnc, DVR_ENC_QUEUE_PUT, &encdata);

		start_code_len = 0;
		if(memcmp(pbuf,NALU_BEGIN_FLAG1,4) == 0)
			start_code_len = 4;
		else if(memcmp(pbuf,NALU_BEGIN_FLAG2,3) == 0)
			start_code_len = 3;

		pbuf+=start_code_len;
		buf_len-=start_code_len;

		while(1)
		{
			pos = find_start_code(pbuf,&start_code_len,buf_len);
			if(pos<0)
			{
				m = esballoc(pbuf,buf_len,0,NULL);
				m->b_wptr+=buf_len;
				//printf("nalu,len=%d,",buf_len);

				if(m)
				{
					mblk_t	*dm = dupmsg(m);
					ms_mutex_lock(&s->mutex);
					putq(&s->captureQ,dm);
					ms_mutex_unlock(&s->mutex);
				}
				break;
			}
			else
			{
				m = esballoc(pbuf,pos,0,NULL);
				m->b_wptr+=pos;
				//printf("nalu,len=%d,",pos);

				if(m)
				{
					mblk_t	*dm = dupmsg(m);
					ms_mutex_lock(&s->mutex);
					putq(&s->captureQ,dm);
					ms_mutex_unlock(&s->mutex);
				}
				pbuf+=pos+start_code_len;
				buf_len-=pos+start_code_len;
				continue;
			}
		}
		//printf("\n");
#ifdef _YK_MEDIA_ABRC_
		ms_mutex_unlock(&s->update_mutex);
		usleep(200);
#endif
	}
	printf("Out the loop for video cap,stop the video device\n");
	V8126DoMunmap(s);
	printf("Stop the thread for video cap,Out\n");
	ms_thread_exit(NULL);
	return NULL;

}

#endif

#ifdef _CODEC_MPEG4_
/*************************************************
*Name	:V8126Capturethread
*Input	:
*Output	:
*Desc	:�ɼ��߳�
**************************************************/
static void* V8126Capturethread(void * data)
{
	V8126State 	*s    	= (V8126State *)data;
	int 		ret 	= 0;
	dvr_enc_queue_get 	encdata;
	struct		pollfd 	rec_fds;

	//sleep(5);
	printf("The thread for video cap begin to run\n");

	if(s->mmapbuf == (void *)-1)
	{
		printf("MMAP error,out the thread\n");
		ms_thread_exit(NULL);
	}

	while(s->bCaptureStart)
	{
		mblk_t *m;

		rec_fds.fd 		= s->fdEnc;
		rec_fds.revents = 0;
		rec_fds.events 	= POLLIN_MAIN_BS;

		poll(&rec_fds, 1, 500);

		if (!(rec_fds.revents & POLLIN_MAIN_BS))
			continue;

		ret = ioctl(s->fdEnc, DVR_ENC_QUEUE_GET, &encdata);
		if(ret < 0)
			continue;

		m = esballoc(s->mmapbuf + encdata.bs.offset,encdata.bs.length,0,NULL);
		m->b_wptr+=encdata.bs.length;
		ioctl(s->fdEnc, DVR_ENC_QUEUE_PUT, &encdata);

		if(m == NULL)
		{
			printf("Frame is NULL,cap error!!!\n");
			continue;
		}
		else if(m)
		{
			mblk_t	*dm = dupmsg(m);
			ms_mutex_lock(&s->mutex);
			putq(&s->captureQ,dm);
			ms_mutex_unlock(&s->mutex);
		}
		else
		{

		}
	}
	printf("Out the loop for video cap,stop the video device\n");
	V8126DoMunmap(s);
	printf("Stop the thread for video cap,Out\n");
	ms_thread_exit(NULL);
	return NULL;

}
#endif
#if 0
/*************************************************
*Name	:V8126Capturethread
*Input	:
*Output	:
*Desc	:�ɼ��߳�
**************************************************/
#define CAPTURE_DATA_BUF_SIZE    8192
#define CAPTURE_DATA_LMT_TIMES   5

#define BEGIN_FLAG_NOT_FOUND     0x01
#define BEGIN_FLAG_FOUND         0x02
#define NALU_END_FLAG1_FOUND     0x03
#define NALU_END_FLAG2_FOUND     0x04
#define NALU_END_FLAG_NOT_FOUND  0x05



typedef struct
{
    unsigned char  buf[CAPTURE_DATA_BUF_SIZE]	;
    unsigned int   len;            //the length of valid data in the ring buf
    unsigned char *head;          //the point to the valid data
    unsigned char  counter;
    unsigned char  state;
}CAPTURE_DATA_CTX_ST;

int FindByte(unsigned char *data, int len, unsigned char byte)
{
    int  i = 1;

    while( (i <= len) && (len > 0) )
    {
        if( *(data + i - 1) == byte )
        {
            return i;
        }
        else
        {
            i++;
        }
    }
    return 0;
}

int FindBytes(unsigned char *data, int len, unsigned char *bytes, int byteNum)
{
    int i = 1;
    int firstIndex = 0, index = 0;

    do
    {
        firstIndex = FindByte(data + index, len - index, bytes[0]);

        if(firstIndex == 0)
        {
            return 0;
        }

        index += firstIndex;

        while(len >= index)
        {
            if(i < byteNum)
            {
                if(data[index] == bytes[i])
                {
                    index++;
                    i++;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        if(i >= byteNum)
        {
            return index;
        }
    }while(index <= len);

    return 0;
}

static void* V8126Capturethread(void * data)
{
	V8126State 	*s    	 = (V8126State *)data;
	int 		ret 	 = 0;
	dvr_enc_queue_get 	 encdata;
	struct		pollfd 	 rec_fds;

	CAPTURE_DATA_CTX_ST  ctx;
	unsigned char       *tmpbuf;
	unsigned int         tmpbuflen;
	unsigned int         offset;
	unsigned char        naluend = NALU_END_FLAG_NOT_FOUND;
	unsigned char       *NALUData = NULL;

	unsigned char        NALU_BEGIN_FLAG1[4]={0x00,0x00,0x00,0x01};
	unsigned char        NALU_END_FLAG1[4]  ={0x00,0x00,0x00,0x01};

	unsigned char        NALU_BEGIN_FLAG2[4]={0x00,0x00,0x01};
	unsigned char        NALU_END_FLAG2[4]  ={0x00,0x00,0x01};

	//init ctx
	memset(ctx.buf,0x0,CAPTURE_DATA_BUF_SIZE * sizeof(unsigned char));
	ctx.len     = 0;
	ctx.head    = ctx.buf;
	ctx.counter = 0;
	ctx.state   = BEGIN_FLAG_NOT_FOUND;

	printf("The thread for video cap begin to run\n");

	if(s->mmapbuf == (void *)-1)			//�ڴ�ӳ����?�˳��߳�
	{
		printf("MMAP error,out the thread\n");
		ms_thread_exit(NULL);
	}

	while(s->bCaptureStart)
	{
		mblk_t *m;

		//m = V8126GrapImageMap(s);
		//printf("���������ɼ����\n");

        if(ctx.counter >= CAPTURE_DATA_LMT_TIMES)
        {
        	ctx.len     = 0;
        	ctx.head    = ctx.buf;
        	ctx.counter = 0;
        	ctx.state   = BEGIN_FLAG_NOT_FOUND;
        }

		//���������ɼ����
		rec_fds.fd 		= s->fdEnc;
		rec_fds.revents = 0;
		rec_fds.events 	= POLLIN_MAIN_BS;

		poll(&rec_fds, 1, 500);

		if (!(rec_fds.revents & POLLIN_MAIN_BS))
			continue;

		ret = ioctl(s->fdEnc, DVR_ENC_QUEUE_GET, &encdata);

		if(ret < 0)
			continue;

		//m = esballoc(s->mmapbuf + encdata.bs.offset,encdata.bs.length,0,NULL);
		//m->b_wptr += encdata.bs.length;

		tmpbuf     = s->mmapbuf + encdata.bs.offset;
		tmpbuflen  = encdata.bs.length;

		ioctl(s->fdEnc, DVR_ENC_QUEUE_PUT, &encdata);

		//printf("���βɼ����:buf add = 0x%8X,size = %d\n",s->mmapbuf + encdata.bs.offset,encdata.bs.length);
		//printf("���βɼ����:db_base = 0x%8X,size = %d\n",m->b_datap->db_base,m->b_datap->db_lim-m->b_datap->db_base);

		if(tmpbuflen == 0)
		{
			continue;
		}

		if( &ctx.buf[CAPTURE_DATA_BUF_SIZE] < ctx.head + ctx.len + tmpbuflen)
		{
            ctx.len     = 0;
            ctx.head    = ctx.buf;
            ctx.counter = 0;
            ctx.state   = BEGIN_FLAG_NOT_FOUND;
		}

		memcpy(ctx.head + ctx.len,tmpbuf,tmpbuflen);
		ctx.len = ctx.len + tmpbuflen;

		offset = 0xFFFF;

		while( (ctx.len > 0) && (offset != 0))
		{
            if(ctx.state == BEGIN_FLAG_NOT_FOUND)
            {
            	offset = FindBytes(ctx.head ,ctx.len,NALU_BEGIN_FLAG1,sizeof(NALU_BEGIN_FLAG1));    //Ѱ��NALU ��־ͷ 00 00 00 01

            	if(offset == 0)
            	{
                	offset = FindBytes(ctx.head ,ctx.len,NALU_BEGIN_FLAG2,sizeof(NALU_BEGIN_FLAG2));//Ѱ��NALU ��־ͷ 00 00 01
            	}

            	if(offset !=0 )  //�ҵ���־ͷ
            	{
                    ctx.head  = ctx.head + offset;
                    ctx.len   = ctx.len  - offset;
                    ctx.state = BEGIN_FLAG_FOUND;
            	}
            	else             //û���ҵ���־ͷ����ݽṹ����
            	{
                    ctx.head  = ctx.buf;
                    ctx.len   = 0;
            	}
            }
            else
            {
            	offset = FindBytes(ctx.head ,ctx.len,NALU_END_FLAG1,sizeof(NALU_END_FLAG1));        //Ѱ��NALU ��־β 00 00 00 01

                if(offset != 0)
                {
                	naluend = NALU_END_FLAG1_FOUND;                                                   //�ҵ�NALU ��־β 00 00 00 01
                }
                else
                {
                	offset = FindBytes(ctx.head ,ctx.len,NALU_END_FLAG2,sizeof(NALU_END_FLAG2));    //Ѱ��NALU ��־β 00 00  01

                	if(offset != 0)
                	{
                		naluend = NALU_END_FLAG2_FOUND;
                	}
                }

            	if(offset == 0)  //δ��⵽�����־��counter��ʼ����
            	{
            		ctx.counter++;
            		naluend = NALU_END_FLAG_NOT_FOUND;
            	}
            	else             //��⵽�����־
            	{
            		ctx.counter = 0; //counter����

            		if( naluend == NALU_END_FLAG1_FOUND )
            		{
            			NALUData = (unsigned char *)malloc(offset-sizeof(NALU_END_FLAG1));
            		}
            		else if( naluend == NALU_END_FLAG2_FOUND)
            		{
            			NALUData = (unsigned char *)malloc(offset-sizeof(NALU_END_FLAG2));
            		}
            		else
            		{

            		}

            		if(NALUData != NULL) //�ڴ�����ɹ�
            		{
            			if( naluend == NALU_END_FLAG1_FOUND )
            			{
                            memcpy(NALUData,ctx.head,offset-sizeof(NALU_END_FLAG1));

                		    m = esballoc(NALUData , offset-sizeof(NALU_END_FLAG1) , 0 , NULL);

                		    m->b_wptr += ( offset - sizeof(NALU_END_FLAG1) );
            			}
            			else if( naluend == NALU_END_FLAG2_FOUND )
            			{
            				memcpy(NALUData,ctx.head,offset-sizeof(NALU_END_FLAG2));

            				m = esballoc(NALUData , offset-sizeof(NALU_END_FLAG2) , 0 , NULL);

            				m->b_wptr += ( offset - sizeof(NALU_END_FLAG2) );

            			}
            			else
            			{

            			}


            			printf("db_base = 0x%8X,size = %d\n",m->b_datap->db_base,m->b_datap->db_lim-m->b_datap->db_base);


                		ms_mutex_lock(&s->mutex);

                		putq(&s->captureQ,m);

                		ms_mutex_unlock(&s->mutex);

                		free(NALUData);
            		}

            		if( naluend == NALU_END_FLAG1_FOUND )
            		{
            			ctx.head = ctx.head + ( offset - sizeof(NALU_END_FLAG1) );

            			ctx.len  = ctx.len  - ( offset - sizeof(NALU_END_FLAG1) );
            		}
            		else if( naluend == NALU_END_FLAG2_FOUND )
            		{
            			ctx.head = ctx.head + ( offset - sizeof(NALU_END_FLAG2) );

            			ctx.len  = ctx.len  - ( offset - sizeof(NALU_END_FLAG2) );
            		}
            		else
            		{

            		}

            		ctx.state  = BEGIN_FLAG_NOT_FOUND;

            	}
            }
		}

        if(ctx.head != ctx.buf)
        {
        	memcpy(ctx.buf,ctx.head,ctx.len);
        	ctx.head = ctx.buf;
        }
	}

	printf("Out the loop for video cap,stop the video device\n");
	V8126DoMunmap(s);
	printf("Stop the thread for video cap,Out\n");
	ms_thread_exit(NULL);
	return NULL;

}
#endif
/*************************************************
*Name	:IDBT_mpeg4_encode_init
*Input	:
*Output	:
*Desc	:return enc_buf_size
**************************************************/
static int IDBT_mpeg4_encode_init(int fdDvr,int fdEnc,int enc_width, int enc_height, int framerate,int br,unsigned char **bs_buf)
{
    int ret 		= 0;
	int ch_num 		= 0;
	int enc_buf_size= 0;

    dvr_enc_channel_param   ch_param;
    EncParam_Ext3 			enc_param_ext = {0};
    dvr_enc_control  		enc_ctrl;
    FuncTag 				tag;
	printf("fdDvr=%d,fdEnc=%d\n",fdDvr,fdEnc);

    //set dvr encode source parameter ���ñ���Դ����
    ch_param.src.input_system 			= MCP_VIDEO_NTSC;//������ƵģʽNTSC
    ch_param.src.channel 				= ch_num;
    ch_param.src.enc_src_type 			= ENC_TYPE_FROM_CAPTURE;
    ch_param.src.dim.width 				= enc_width;	//1280;
    ch_param.src.dim.height 			= enc_height;	//720;
    ch_param.src.di_mode 				= LVFRAME_EVEN_ODD;
    ch_param.src.mode 					= LVFRAME_FRAME_MODE;
    ch_param.src.dma_order 				= DMAORDER_PACKET;
    ch_param.src.scale_indep 			= CAPSCALER_NOT_KEEP_RATIO;
    ch_param.src.input_system 			= MCP_VIDEO_NTSC;
    ch_param.src.color_mode 			= CAPCOLOR_YUV422;
    ch_param.src.vp_param.is_3DI 		= FALSE;
    ch_param.src.vp_param.is_denoise	= FALSE;
    ch_param.src.vp_param.denoise_mode 	= GM3DI_FIELD;

    //set dvr encode main bitstream parameter
    ch_param.main_bs.enabled 			= DVR_ENC_EBST_ENABLE;
    ch_param.main_bs.out_bs 			= 0;
    ch_param.main_bs.enc_type 			= ENC_TYPE_MPEG;
    ch_param.main_bs.is_blocked 		= FALSE;
    ch_param.main_bs.en_snapshot 		= DVR_ENC_EBST_DISABLE;
    ch_param.main_bs.dim.width 			= enc_width;//1280;
    ch_param.main_bs.dim.height 		= enc_height;//720;

    //set main bitstream encode parameter
    ch_param.main_bs.enc.input_type 	= ENC_INPUT_H2642D;
    ch_param.main_bs.enc.frame_rate 	= framerate;//30;

	//if(Local.RecPicSize == 2)       				//D1
    //ch_param.main_bs.enc.bit_rate 		= 1024*1024;
	//else
	//ch_param.main_bs.enc.bit_rate 	= 512*1024;
	ch_param.main_bs.enc.bit_rate 	    = br*1024;

    ch_param.main_bs.enc.ip_interval 	= 20;//30;
    ch_param.main_bs.enc.init_quant 	= 20;//25;
    ch_param.main_bs.enc.max_quant 		= 31;
    ch_param.main_bs.enc.min_quant 		= 1;
    ch_param.main_bs.enc.is_use_ROI 	= FALSE;
    ch_param.main_bs.enc.ROI_win.x 		= 0;
    ch_param.main_bs.enc.ROI_win.y 		= 0;
    ch_param.main_bs.enc.ROI_win.width 	= 320;
    ch_param.main_bs.enc.ROI_win.height = 240;

    //set main bitstream scalar parameter
    ch_param.main_bs.scl.src_fmt 		= SCALE_YUV422;
    ch_param.main_bs.scl.dst_fmt 		= SCALE_MP4_YUV420_MODE0;
    ch_param.main_bs.scl.scale_mode 	= SCALE_LINEAR;
    ch_param.main_bs.scl.is_dither 		= FALSE;
    ch_param.main_bs.scl.is_correction 	= FALSE;
    ch_param.main_bs.scl.is_album 		= TRUE;
    ch_param.main_bs.scl.des_level 		= 0;

    //set main bitstream snapshot parameter
    ch_param.main_bs.snap.sample 			= JCS_yuv420;
    ch_param.main_bs.snap.RestartInterval 	= 0;
    ch_param.main_bs.snap.u82D 				= JENC_INPUT_MP42D;
    ch_param.main_bs.snap.quality 			= 70;//70;

    //associate the ext. structure
    ch_param.main_bs.enc.ext_size 			= DVR_ENC_MAGIC_ADD_VAL(sizeof(enc_param_ext));
    ch_param.main_bs.enc.pext_data 			= &enc_param_ext;

    enc_param_ext.feature_enable 			= 0;      //CBR

    ioctl(fdEnc, DVR_ENC_SET_CHANNEL_PARAM, &ch_param);
    ioctl(fdEnc, DVR_ENC_QUERY_OUTPUT_BUFFER_SIZE, &enc_buf_size);
	printf("enc_buf_size=%d\n",enc_buf_size);
    *bs_buf = (unsigned char*) mmap(NULL, enc_buf_size, PROT_READ|PROT_WRITE,MAP_SHARED, fdEnc, 0);

	printf("bs_buf = 0x%08X\n", *bs_buf);

    //record start
    memset(&enc_ctrl, 0x0, sizeof(dvr_enc_control));
    enc_ctrl.command 	= ENC_START;
    enc_ctrl.stream 	= 0;
    ret = ioctl(fdEnc, DVR_ENC_CONTROL, &enc_ctrl);

    // set function tag paremeter to dvr graph level
    FN_RESET_TAG(&tag);
    FN_SET_REC_CH(&tag, ch_num);
    ioctl(fdDvr, DVR_COMMON_APPLY, &tag);

	return(enc_buf_size);
}

/*************************************************
*Name	:IDBT_H264_encode_init
*Input	:
*Output	:
*Desc	:return enc_buf_size
**************************************************/
static int IDBT_H264_encode_init(int fdDvr,int fdEnc,int enc_width, int enc_height, int framerate,int br,unsigned char **bs_buf)
{
    int ret 		= 0;
	int ch_num 		= 0;
	int enc_buf_size= 0;

    dvr_enc_channel_param   ch_param;
    EncParam_Ext3 			enc_param_ext = {0};
    dvr_enc_control  		enc_ctrl;
    FuncTag 				tag;
	printf("fdDvr=%d,fdEnc=%d\n",fdDvr,fdEnc);

    //set dvr encode source parameter ���ñ���Դ����
    ch_param.src.input_system 			= MCP_VIDEO_NTSC;//������ƵģʽNTSC
    ch_param.src.channel 				= ch_num;
    ch_param.src.enc_src_type 			= ENC_TYPE_FROM_CAPTURE;
    ch_param.src.dim.width 				= enc_width;	//1280;
    ch_param.src.dim.height 			= enc_height;	//720;
    ch_param.src.di_mode 				= LVFRAME_EVEN_ODD;
    ch_param.src.mode 					= LVFRAME_FRAME_MODE;
    ch_param.src.dma_order 				= DMAORDER_PACKET;
    ch_param.src.scale_indep 			= CAPSCALER_NOT_KEEP_RATIO;
    ch_param.src.input_system 			= MCP_VIDEO_NTSC;
    ch_param.src.color_mode 			= CAPCOLOR_YUV422;
    ch_param.src.vp_param.is_3DI 		= FALSE;
    ch_param.src.vp_param.is_denoise	= FALSE;
    ch_param.src.vp_param.denoise_mode 	= GM3DI_FIELD;

    //set dvr encode main bitstream parameter
    ch_param.main_bs.enabled 			= DVR_ENC_EBST_ENABLE;
    ch_param.main_bs.out_bs 			= 0;
    ch_param.main_bs.enc_type 			= ENC_TYPE_H264;			//lcd for 264
    ch_param.main_bs.is_blocked 		= FALSE;
    ch_param.main_bs.en_snapshot 		= DVR_ENC_EBST_DISABLE;
    ch_param.main_bs.dim.width 			= enc_width;//1280;
    ch_param.main_bs.dim.height 		= enc_height;//720;

    //set main bitstream encode parameter
    ch_param.main_bs.enc.input_type 	= ENC_INPUT_H2642D;
    ch_param.main_bs.enc.frame_rate 	= framerate;//30;

	//if(Local.RecPicSize == 2)       				//D1
    //ch_param.main_bs.enc.bit_rate 		= 1024*1024;
	//else
	//ch_param.main_bs.enc.bit_rate 	= 512*1024;
	ch_param.main_bs.enc.bit_rate 	    = br*1024;

    ch_param.main_bs.enc.ip_interval 	= 30;//30;				//lcd for 264
    ch_param.main_bs.enc.init_quant 	= 25;
    ch_param.main_bs.enc.max_quant 		= 51;					//lcd for 264
    ch_param.main_bs.enc.min_quant 		= 1;
    ch_param.main_bs.enc.is_use_ROI 	= FALSE;
    ch_param.main_bs.enc.ROI_win.x 		= 0;
    ch_param.main_bs.enc.ROI_win.y 		= 0;
    ch_param.main_bs.enc.ROI_win.width 	= 352;					//lcd for 264
    ch_param.main_bs.enc.ROI_win.height = 240;

    //set main bitstream scalar parameter
    ch_param.main_bs.scl.src_fmt 		= SCALE_YUV422;
    ch_param.main_bs.scl.dst_fmt 		= SCALE_YUV422;			//lcd for 264
    ch_param.main_bs.scl.scale_mode 	= SCALE_LINEAR;
    ch_param.main_bs.scl.is_dither 		= FALSE;
    ch_param.main_bs.scl.is_correction 	= FALSE;
    ch_param.main_bs.scl.is_album 		= TRUE;
    ch_param.main_bs.scl.des_level 		= 0;

    //set main bitstream snapshot parameter
    ch_param.main_bs.snap.sample 			= JCS_yuv420;
    ch_param.main_bs.snap.RestartInterval 	= 0;
    ch_param.main_bs.snap.u82D 				= JENC_INPUT_MP42D;
    ch_param.main_bs.snap.quality 			= 70;//70;

    //associate the ext. structure
    ch_param.main_bs.enc.ext_size 			= DVR_ENC_MAGIC_ADD_VAL(sizeof(enc_param_ext));
    ch_param.main_bs.enc.pext_data 			= &enc_param_ext;

    enc_param_ext.feature_enable 			= 0;      //CBR

    ioctl(fdEnc, DVR_ENC_SET_CHANNEL_PARAM, &ch_param);
    ioctl(fdEnc, DVR_ENC_QUERY_OUTPUT_BUFFER_SIZE, &enc_buf_size);
	printf("enc_buf_size=%d\n",enc_buf_size);
    *bs_buf = (unsigned char*) mmap(NULL, enc_buf_size, PROT_READ|PROT_WRITE,MAP_SHARED, fdEnc, 0);

	printf("bs_buf = 0x%08X\n", *bs_buf);

    //record start
    memset(&enc_ctrl, 0x0, sizeof(dvr_enc_control));
    enc_ctrl.command 	= ENC_START;
    enc_ctrl.stream 	= 0;
    ret = ioctl(fdEnc, DVR_ENC_CONTROL, &enc_ctrl);

    // set function tag paremeter to dvr graph level
    FN_RESET_TAG(&tag);
    FN_SET_REC_CH(&tag, ch_num);

    ret = ioctl(fdDvr, DVR_COMMON_APPLY, &tag);
    if(ret < 0)
    {
    	printf("DVR COMMON APPLY is failed!\n");
    	LOG_RUNLOG_ERROR("LP: DVR COMMON APPLY failed, system will reboot");
    	LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_REBOOT, 0, "");
    	system("reboot");
    }
    else
    {
    	printf("DVR COMMON APPLY is OK!\n");
    }
	return(enc_buf_size);
}

#ifdef _YK_MEDIA_ABRC_

static int IDBT_H264_encode_update(int fdDvr,int fdEnc,int framerate,int br)
{
    FuncTag tag;

	dvr_enc_control   enc_update;

	 //set update recode parameter
    memset(&enc_update, 0x0, sizeof(dvr_enc_control));
    enc_update.stream = 0;
    enc_update.update_parm.stream_enable 		= 1;
    enc_update.update_parm.frame_rate 			= framerate;
    enc_update.update_parm.bit_rate 				= br*1024;
    enc_update.update_parm.ip_interval 			= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.dim.width 				= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.dim.height 			= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.src.di_mode 			= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.src.mode 				= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.src.scale_indep 	= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.src.is_3DI 			= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.src.is_denoise 	= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.init_quant 			= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.max_quant 				= GMVAL_DO_NOT_CARE;
    enc_update.update_parm.min_quant 				= GMVAL_DO_NOT_CARE;


	enc_update.command = ENC_UPDATE;
	ioctl(fdEnc, DVR_ENC_CONTROL, &enc_update);

	FN_RESET_TAG(&tag);
	FN_SET_REC_CH(&tag, 0);
	ioctl(fdDvr, DVR_COMMON_APPLY, &tag);

}
#endif

extern int VideoRecIsStart;

/*************************************************
*Name	:V8126Start
*Input	:
*Output	:
*Desc	:���豸�ļ�����ʼ��
**************************************************/
static int V8126Start(MSFilter *f,void *arg)
{
//	int fdDvr;
//	int fdEnc;
	int width;
	int height;
	char *p;


#ifdef _YK_XT8126_BV_
	YK8126VideoBVResWait();
#endif

	V8126State *s = (V8126State *)f->data;

    if(dvr_fd >0 || enc_fd>0)
    {
    	printf("video device already open:fdDvr=%d,fdEnc=%d\n",dvr_fd,enc_fd);
    }
    else
    {
    	dvr_fd = open("/dev/dvr_common", O_RDWR);   	//open_dvr_common
    	enc_fd = open(s->devname, O_RDWR);      				//open_dvr_encode
    	printf("Open video device:fdDvr=%d,fdEnc=%d\n",dvr_fd,enc_fd);
    }

	if( (dvr_fd < 0) || (enc_fd < 0))				//�豸�ļ����쳣
	{
		//��ʼ��˽����ݽṹ V8126State
		s->devname			= ms_strdup("/dev/dvr_enc");
		s->fdDvr			= -1;
		s->fdEnc			= -1;
		s->fdIoLight		= -1;
    	s->hvCapture 	 	= -1;
    	s->bCaptureStart 	= FALSE;
		s->bauto_started	= FALSE;
		s->mmapbuf			= NULL;
		s->msize			= 0;

		f->data				= s;
		printf("Open video device exception:fdDvr or fdEnc Error!!!\n");
	}
	else										//�豸�ļ�����
	{
		//modified by lcx
		if(LPGetCallDir() == CALL_DIR_OUTGOING)
		{
			//��ʼ��IO��
			if((s->fdIoLight = YK8126GpioInit2())>=0)
			{
					YK8126CloseFillLight();
			}
		}

		//��ʼ����Ƶ�豸
		printf("=======================================================\n");
		printf("Video format:CIF\n");
		width	= 352;
		height 	= 240;
		fps8126 = 10;// h264 20->15
		br8126  = 128;
		printf("Resolution=%d x %d\nFrame Rate=%d\nCode Rate=%d\n",width,height,fps8126,br8126);
		printf("=========================================================\n");

#if defined(_CODEC_MPEG4_)
		printf("ENCODER:MPEG-4\n");
		s->msize = IDBT_mpeg4_encode_init(fdDvr,fdEnc,width,height,fps8126,br8126,&s->mmapbuf);	//CIF
#endif

#if defined(_CODEC_H264_)
		printf("ENCODER:H264\n");
		s->msize = IDBT_H264_encode_init(dvr_fd,enc_fd,width,height,fps8126,br8126,&s->mmapbuf);	//CIF
#endif



		printf("Using mmap buffer at %d,len = %d\n",s->mmapbuf,s->msize);

		//����˽����ݽṹ V8126State
		s->devname			= ms_strdup("/dev/dvr_enc");
		s->fdDvr			= dvr_fd;
		s->fdEnc			= enc_fd;
    	s->hvCapture 	 	= -1;
    	s->bCaptureStart 	= FALSE;
		s->bauto_started	= TRUE;

    	//qinit(&s->captureQ);
    	//ms_mutex_init(&s->mutex,NULL);

		//f->data				= s;
	}
	return 0;
}
/*************************************************
*Name	:V8126StartCapture
*Input	:
*Output	:
*Desc	:�����ɼ��߳�
**************************************************/
static void V8126StartCapture(V8126State *s)
{
    if(s->fdDvr>=0 ||s->fdEnc >=0)
    {
    	printf("Create the thread for video capture\n");
		s->bCaptureStart = TRUE;
        ms_thread_create(&s->thread,NULL,V8126Capturethread,s);
    }
}
/*************************************************
*Name	:V8126Stop
*Input	:
*Output	:
*Desc	:�ر��豸�ļ�
**************************************************/
static int V8126Stop(MSFilter *f,void *arg)
{
	V8126State *s = (V8126State *)f->data;

	if((s->fdDvr >= 0)||(s->fdEnc >= 0))
	{
		printf("Unmap mem\n");
		if(s->mmapbuf != NULL)
		{
			if(munmap((void*)s->mmapbuf,s->msize)<0)
			{
				printf("V8126DoMunmap:Fail to unmap\n");
			}
			s->mmapbuf = NULL;
		}
		s->msize = 0;
		printf("Close the dev Fd\n");
//		if(close(s->fdEnc)<0)
//		{
//			printf("V8126Stop:V8126 could not close fdEnc=%d\n",s->fdEnc);
//		}
//		if(close(s->fdDvr)<0)
//		{
//			printf("V8126Stop:V8126 could not close fdDvr=%d\n",s->fdDvr);
//		}
//
//		s->fdDvr = -1;
//		s->fdEnc = -1;
	}
	printf("Close light fill\n");
	if(s->fdIoLight >= 0)
	{
 		//�رղ��⺯��
		YK8126CloseFillLight();
		YK8126GpioUninit2();
		s->fdIoLight = -1;
	}
	return 0;
}
/*************************************************
*Name	:V8126StopCapture
*Input	:
*Output	:
*Desc	:�ر��߳�
**************************************************/
static void V8126StopCapture(V8126State *s)
{
    if(s->bCaptureStart)
    {
    	s->bCaptureStart = FALSE;
        ms_thread_join(s->thread,NULL);
		printf("V8126StopCapture:V8126 thread joined\n");
        flushq(&s->captureQ,0);
    }
}
/*************************************************
*Name	:MS8126CaptureFilterInit
*Input	:
*Output	:
*Desc	:init
**************************************************/
static void MS8126CaptureFilterInit(MSFilter *f)
{
	V8126State *s = ms_new0(V8126State,1);

	//printf("����Webcam Filter��ʼ��\n");

	s->devname			= ms_strdup("/dev/dvr_enc");
	s->fdDvr			= -1;
	s->fdEnc			= -1;
	s->fdIoLight		= -1;
    s->hvCapture 	 	= -1;
    s->bCaptureStart 	= FALSE;
	s->bauto_started	= FALSE;
	s->mmapbuf			= NULL;
	s->msize			= 0;

	qinit(&s->captureQ);
    ms_mutex_init(&s->mutex,NULL);

#ifdef _YK_MEDIA_ABRC_
    ms_mutex_init(&s->update_mutex,NULL);
#endif

	f->data				= s;
	//printf("���Webcam Filter��ʼ��\n");
}
/*************************************************
*Name	:MS8126CaptureFilterPreprocess
*Input	:
*Output	:
*Desc	:preprocess
**************************************************/
static void MS8126CaptureFilterPreprocess(MSFilter *f)
{
	V8126State *s=(V8126State *)f->data;

	printf("Enter Webcam Filter Preprocess\n");

	if((s->fdDvr == -1) || (s->fdEnc == -1) )	//��Ƶ�豸δ��
	{
		//printf("Ԥ����:��Ƶ�豸δ��\n");
		s->bauto_started = TRUE;
		V8126Start(f,NULL);						//���´���Ƶ�豸,��ʼ�����ã��ڴ�ӳ��
    	V8126StartCapture(s);					//������Ƶ�ɼ��߳�
	}
	else										//��Ƶ�豸�Ѵ򿪣��˴�δ�����ڴ�ӳ�� ���ܳ�������
	{
		printf("Webcam Filter Preprocess:The video Device has been opened\n");
    	V8126StartCapture(s);
	}

	//printf("���Webcam FilterԤ����\n");

}
/*************************************************
*Name	:MS8126CaptureFilterProcess
*Input	:
*Output	:
*Desc	:process
**************************************************/
static void MS8126CaptureFilterProcess(MSFilter *f)
{
    V8126State *s = (V8126State *)f->data;
	uint32_t	timestamp;

   	mblk_t *om = NULL;

	//printf("����Webcam Filter����\n");
	if(s->bCaptureStart == FALSE)
	{
		return;
	}

	ms_mutex_lock(&s->mutex);
    while((om=getq(&s->captureQ))!=0)
	{
		//printf("Filter�յ����֡:db_base = 0x%8X,size=%d\n",om->b_datap->db_base,om->b_datap->db_lim-om->b_datap->db_base);
		timestamp = f->ticker->time*90;

		mblk_set_timestamp_info(om,timestamp);
		mblk_set_marker_info(om,TRUE);
	 	ms_queue_put(f->outputs[0],om);
    }
    ms_mutex_unlock(&s->mutex);
	//printf("���Webcam Filter����\n");



}
/*************************************************
*Name	:MS8126CaptureFilterPostProcess
*Input	:
*Output	:
*Desc	:postprocess
**************************************************/
static void MS8126CaptureFilterPostProcess(MSFilter *f)
{
	V8126State *s = (V8126State *)f->data;

	printf("Enter Webcam Filter Postprocess\n");

	if(s->bauto_started)
	{
		V8126StopCapture(s);		//�ر��߳�
		V8126Stop(f,NULL);			//�ر��豸�ļ�ָ��
	}
	else
	{
		printf("AAAAAAAAAAAAAAA AAAAAAAAAA AAAAAAAAAAAAOut Webcam Filter Postprocess\n");
		V8126StopCapture(s);		//�ر��߳�
	}

	printf("Out Webcam Filter Postprocess\n");

}
/*************************************************
*Name	:MS8126CaptureFilterUninit
*Input	:
*Output	:
*Desc	:uninit
**************************************************/
static void MS8126CaptureFilterUninit(MSFilter *f)
{
    V8126State* s =(V8126State*)f->data;

	printf("Enter FILTER unint:MS8126CaptureFilterUninit\n");

	if((s->fdDvr >= 0)||(s->fdEnc >= 0))
	{
		V8126Stop(f,NULL);//�ر��ļ�ָ��
	}
	if(NULL != s->devname)
		ms_free(s->devname);

    flushq(&s->captureQ,0);
    ms_mutex_destroy(&s->mutex);
#ifdef _YK_MEDIA_ABRC_
    ms_mutex_destroy(&s->update_mutex);
#endif
    ms_free(s);

	printf("Out FILTER unint:MS8126CaptureFilterUninit\n");

}

#ifdef _YK_MEDIA_ABRC_

typedef struct
{
	unsigned int fps;
	unsigned int bitrate;
	unsigned int last_jitter;
}abrc_para;

static int MS8126SetBitrate(MSFilter *f,void *arg)
{
	abrc_para *bc = (abrc_para *)arg;

	V8126State* s =(V8126State*)f->data;

	printf("MS8126SetBitrate new fps=%d,bitrate=%d\n",bc->fps,bc->bitrate);

	ms_mutex_lock(&s->update_mutex);

	IDBT_H264_encode_update(s->fdDvr,s->fdEnc,bc->fps,bc->bitrate);	//CIF

	ms_mutex_unlock(&s->update_mutex);

	return 0;
}

static MSFilterMethod MS8126CaptureMethods[] = {
		{MS_FILTER_SET_BITRATE 	, MS8126SetBitrate},
		{0						, NULL}
};
#endif

//��Ƶ�ɼ��豸��Ӧ��Filter
MSFilterDesc ms_V8126_desc = {
    MS_8126_VIDEO_CAPTURE_ID,
    "MsV8126",
    N_("A filter that captures 8126 video."),
    MS_FILTER_OTHER,
    NULL,
    0,
    1,
    MS8126CaptureFilterInit,
    MS8126CaptureFilterPreprocess,
    MS8126CaptureFilterProcess,
    MS8126CaptureFilterPostProcess,
    MS8126CaptureFilterUninit,
#ifdef _YK_MEDIA_ABRC_
    MS8126CaptureMethods//ms_8126_capture_methods
#else
    NULL
#endif
};
MS_FILTER_DESC_EXPORT(ms_V8126_desc)
/*************************************************
*Name	:V8126_set_devfile
*Input	:
*Output	:
*Desc	:
**************************************************/
static int V8126_set_devfile(MSFilter *f,void *arg)
{
	V8126State *s = (V8126State*)f->data;
	if(s->devname)
		ms_free(s->devname);
	s->devname = ms_strdup((char*)arg);
	return 0;
}
/*************************************************
*Name	:V8126CaptureDetect
*Input	:
*Output	:
*Desc	:��Ƶ�豸̽�⣬�Ƿ������Ӧ���豸��������
* 		 ��ӵ���Ƶ�豸����������
**************************************************/
static void V8126CaptureDetect(MSWebCamManager *obj)
{
	const char *devicenameDVR="/dev/dvr_common";
	const char *devicenameENC="/dev/dvr_enc";

	dvr_fd = open(devicenameDVR, O_RDWR);   	//open_dvr_common
    enc_fd = open(devicenameENC, O_RDWR);      	//open_dvr_encode

	printf("Enter Webcam Detect:Open Video Devicedvrfd=%d,encfd=%d\n",dvr_fd,enc_fd);

    if((dvr_fd >= 0) && (enc_fd >= 0))
    {
    	MSWebCam *cam	= ms_web_cam_new(&V8126WebCamDesc);
		cam->name		= ms_strdup(devicenameENC);
    	ms_web_cam_manager_add_cam(obj, cam);

    	close(enc_fd);
    	close(dvr_fd);

    	enc_fd = -1;
    	dvr_fd = -1;
	}
	printf("Out Webcam Detect:Close Video Device\n");


}
/*************************************************
*Name	:V8126CaptureCamInit
*Input	:
*Output	:
*Desc	:��Ƶ�豸��ʼ��
**************************************************/
static void V8126CaptureCamInit(MSWebCam *cam)
{
	printf("Enter Webcam Init:NULL\n");
	printf("Out   Webcam Init:NULL\n");
}
/*************************************************
*Name	:V8126CaptureCreateReader
*Input	:
*Output	:
*Desc	:������Ӧ��Filter
**************************************************/
static MSFilter *V8126CaptureCreateReader(MSWebCam *obj)
{
    MSFilter 	*f = ms_filter_new_from_desc(&ms_V8126_desc);
    V8126State	*s = (V8126State *)f->data;

	printf("Enter Webcam CreateReader\n");

	V8126_set_devfile(f,obj->name);

	printf("Out Webcam CreateReader\n");

    return f;

}
/*************************************************
*Name	:V8126CaptureCamUninit
*Input	:
*Output	:
*Desc	:uninit
**************************************************/
static void V8126CaptureCamUninit(MSWebCam *obj)
{
	printf("Enter Webcam uninit:NULL\n");
	printf("Out   Webcam uninit:NULL\n");
}

//������Ƶ�ɼ��豸����
MSWebCamDesc V8126WebCamDesc={
    "V8126WebCam",
    &V8126CaptureDetect,
    &V8126CaptureCamInit,
    &V8126CaptureCreateReader,
    &V8126CaptureCamUninit
};
//===============================================================================================
/*************************************************
*Name	:enc_8126mpeg4_init
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126Mpeg4_init(MSFilter *f)
{
printf("Complete 8126Mpeg4 encoder init\n");
}
/*************************************************
*Name	:enc_8126preprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126Mpeg4_preprocess(MSFilter *f)
{
printf("Complete 8126Mpeg4 encoder preprocess\n");
}
/*************************************************
*Name	:enc_8126process
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126Mpeg4_process(MSFilter *f)
{
	mblk_t 		*inm;
	mblk_t 		*packet		=NULL;
	uint8_t 	*rptr;
	int 		len 		= 0;
	int 		mtu 		= ms_get_payload_max_size() - 2;
	uint32_t 	timestamp	= f->ticker->time*90LL;

	//printf("����8126Mpeg4���봦����\n");

	ms_filter_lock(f);
	while((inm=ms_queue_get(f->inputs[0]))!=0)
	{
		for (rptr=inm->b_rptr;rptr<inm->b_wptr;)
		{
			len				= MIN(mtu,(inm->b_wptr-rptr));
			packet			= dupb(inm);
			packet->b_rptr	= rptr;
			packet->b_wptr	= rptr+len;
			mblk_set_timestamp_info(packet, timestamp);
			ms_queue_put(f->outputs[0],packet);
			rptr+=len;
		}
		mblk_set_marker_info(packet,TRUE);
		freemsg(inm);
		//printf("��������RTP������Ϣ\n");
		//printf("��ȡ��Ƶ֡��Ϣ\n");
		//timestamp = f->ticker->time*90LL;
		//printf("��ʱ���=%d\n",timestamp);
		//mblk_set_timestamp_info(inm,timestamp);
		//ms_queue_put(f->outputs[0],inm);
		//printf("������Ƶ֡��Ϣ\n");
		/*set marker bit on last packet*/
		//mblk_set_marker_info(inm,TRUE);
		//freemsg(inm);
	}
	ms_filter_unlock(f);
	//printf("�˳�8126Mpeg4���봦����\n");
}
/*************************************************
*Name	:enc_8126postprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126Mpeg4_postprocess(MSFilter *f)
{
	printf("Complete 8126Mpeg4 encoder postprocess\n");
}
/*************************************************
*Name	:enc_8126uninit
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126Mpeg4_uninit(MSFilter  *f)
{
	printf("Complete 8126Mpeg4 encoder uninit\n");
}
//========================================================================
/*************************************************
*Name	:dec_8126mpeg4_init
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126Mpeg4_init(MSFilter *f)
{
printf("Complete 8126Mpeg4 decoder init\n");
}
/*************************************************
*Name	:dec_8126preprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126Mpeg4_preprocess(MSFilter *f)
{
printf("Complete 8126Mpeg4 decoder preprocess\n");
}

/*************************************************
*Name	:dec_8126Mpeg4_process
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126Mpeg4_process(MSFilter *f)
{
	printf("Complete 8126Mpeg4 decoder process\n");
}
/*************************************************
*Name	:dec_8126Mpeg4_postprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126Mpeg4_postprocess(MSFilter *f)
{
	printf("Complete 8126Mpeg4 decoder postprocess\n");
}
/*************************************************
*Name	:dec_8126uninit
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126Mpeg4_uninit(MSFilter  *f)
{
	printf("Complete 8126Mpeg4 decoder uninit\n");
}
//===============================================================================================
//
//===============================================================================================
//===============================================================================================
//									H 2 6 4
//===============================================================================================
/*************************************************
*Name	:enc_8126H264_init
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126H264_init(MSFilter *f)
{
	printf("Complete 8126H264 encoder init\n");
}
/*************************************************
*Name	:enc_8126H264_preprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126H264_preprocess(MSFilter *f)
{
	printf("Complete 8126H264 encoder preprocess\n");
}
/*************************************************
*Name	:enc_8126H264_process
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126H264_process(MSFilter *f)
{
    Rfc3984Context	*Context4H264;
	uint32_t 		timestamp	= f->ticker->time*90LL;

	Context4H264 = rfc3984_new();
	ms_filter_lock(f);
	rfc3984_pack(Context4H264, f->inputs[0], f->outputs[0], timestamp);
	ms_filter_unlock(f);
}
/*************************************************
*Name	:enc_8126H264_postprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126H264_postprocess(MSFilter *f)
{
	printf("Complete 8126H264 encoder postprocess\n");
}
/*************************************************
*Name	:enc_8126H264_uninit
*Input	:
*Output	:
*Desc	:
**************************************************/
static void enc_8126H264_uninit(MSFilter  *f)
{
	printf("Complete 8126H264 encoder uninit\n");
}
//========================================================================
/*************************************************
*Name	:dec_8126H264_init
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126H264_init(MSFilter *f)
{
	printf("Complete 8126H264 decoder init\n");
}
/*************************************************
*Name	:dec_8126H264_preprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126H264_preprocess(MSFilter *f)
{
	printf("Complete 8126H264 decoder preprocess\n");
}
/*************************************************
*Name	:dec_8126H264_process
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126H264_process(MSFilter *f)
{
	printf("Complete 8126H264 decoder process\n");
}
/*************************************************
*Name	:dec_8126H264_postprocess
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126H264_postprocess(MSFilter *f)
{
	printf("Complete 8126H264 decoder postprocess\n");
}
/*************************************************
*Name	:dec_8126H264_uninit
*Input	:
*Output	:
*Desc	:
**************************************************/
static void dec_8126H264_uninit(MSFilter  *f)
{
	printf("Complete 8126H264decoder uninit\n");
}

#ifdef _MSC_VER
//Xintel V8126 ms_V8126Mpeg4Enc_Desc  ������
MSFilterDesc ms_V8126Mpeg4Enc_Desc={
	MS_8126_VIDEO_MPEG4ENC_ID,
	"MSMpeg4Enc",
	N_("A video MPEG4 encoder for Xintel 8126"),
	MS_FILTER_ENCODER,
	"MP4V-ES",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_8126Mpeg4_init,
	enc_8126Mpeg4_preprocess,
	enc_8126Mpeg4_process,
	enc_8126Mpeg4_postprocess,
	enc_8126Mpeg4_uninit,

	NULL
};

//Xingtel V8126 ms_V8126Mpeg4Dec_Desc ������
MSFilterDesc ms_V8126Mpeg4Dec_Desc={
	MS_8126_VIDEO_MPEG4DEC_ID,
	"MSMpeg4Dec",
	N_("A video MPEG4 decoder for Xintel 8126"),
	MS_FILTER_DECODER,
	"MP4V-ES",
	1, /*MS_YUV420P is assumed on this input */
	1,
	dec_8126Mpeg4_init,
	dec_8126Mpeg4_preprocess,
	dec_8126Mpeg4_process,
	dec_8126Mpeg4_postprocess,
	dec_8126Mpeg4_uninit,
	NULL
};
MS_FILTER_DESC_EXPORT(ms_V8126Mpeg4Enc_Desc)
MS_FILTER_DESC_EXPORT(ms_V8126Mpeg4Dec_Desc)

//Xintel V8126 ms_V8126H264Enc_Desc
MSFilterDesc ms_V8126H264Enc_Desc={
	MS_8126_VIDEO_H264ENC_ID,
	"MSH264Enc",
	N_("A video H264 encoder for Xintel 8126"),
	MS_FILTER_ENCODER,
	"H264",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_8126H264_init,
	enc_8126H264_preprocess,
	enc_8126H264_process,
	enc_8126H264_postprocess,
	enc_8126H264_uninit,
	NULL
};
//Xingtel V8126 ms_V8126H264Dec_Desc
MSFilterDesc ms_V8126H264Dec_Desc={
	MS_8126_VIDEO_H264DEC_ID,
	"MSH264Dec",
	N_("A video H264 decoder for Xintel 8126"),
	MS_FILTER_DECODER,
	"H264",
	1, /*MS_YUV420P is assumed on this input */
	1,
	dec_8126H264_init,
	dec_8126H264_preprocess,
	dec_8126H264_process,
	dec_8126H264_postprocess,
	dec_8126H264_uninit,
	NULL
};
MS_FILTER_DESC_EXPORT(ms_V8126H264Enc_Desc)
MS_FILTER_DESC_EXPORT(ms_V8126H264Dec_Desc)
#else
//Xintel V8126 ms_V8126Mpeg4Enc_Desc  ������
MSFilterDesc ms_V8126Mpeg4Enc_Desc=
{
	.id			= MS_8126_VIDEO_MPEG4ENC_ID,
	.name		= "MSMpeg4Enc",
	.text		= N_("A video MPEG4 encoder for Xintel 8126"),
	.category	= MS_FILTER_ENCODER,
	.enc_fmt	= "MP4V-ES",
	.ninputs	= 1, /*MS_YUV420P is assumed on this input */
	.noutputs	= 1,
	.init		= enc_8126Mpeg4_init,
	.preprocess	= enc_8126Mpeg4_preprocess,
	.process	= enc_8126Mpeg4_process,
	.postprocess= enc_8126Mpeg4_postprocess,
	.uninit		= enc_8126Mpeg4_uninit,

	.methods	= NULL
};
//Xingtel V8126 ms_V8126Mpeg4Dec_Desc ������
MSFilterDesc ms_V8126Mpeg4Dec_Desc=
{
	.id			= MS_8126_VIDEO_MPEG4DEC_ID,
	.name		= "MSMpeg4Dec",
	.text		= N_("A video MPEG4 decoder for Xintel 8126"),
	.category	= MS_FILTER_DECODER,
	.enc_fmt	= "MP4V-ES",
	.ninputs	= 1, /*MS_YUV420P is assumed on this input */
	.noutputs	= 1,
	.init		= dec_8126Mpeg4_init,
	.preprocess	= dec_8126Mpeg4_preprocess,
	.process	= dec_8126Mpeg4_process,
	.postprocess= dec_8126Mpeg4_postprocess,
	.uninit		= dec_8126Mpeg4_uninit,

	.methods	= NULL
};
MS_FILTER_DESC_EXPORT(ms_V8126Mpeg4Enc_Desc)
MS_FILTER_DESC_EXPORT(ms_V8126Mpeg4Dec_Desc)
//Xintel V8126 ms_V8126H264Enc_Desc
MSFilterDesc ms_V8126H264Enc_Desc=
{
	.id			= MS_8126_VIDEO_H264ENC_ID,
	.name		= "MSH264Enc",
	.text		= N_("A video H264 encoder for Xintel 8126"),
	.category	= MS_FILTER_ENCODER,
	.enc_fmt	= "H264",
	.ninputs	= 1, /*MS_YUV420P is assumed on this input */
	.noutputs	= 1,
	.init		= enc_8126H264_init,
	.preprocess	= enc_8126H264_preprocess,
	.process	= enc_8126H264_process,
	.postprocess= enc_8126H264_postprocess,
	.uninit		= enc_8126H264_uninit,
	.methods	= NULL
};
//Xingtel V8126 ms_V8126H264Dec_Desc
MSFilterDesc ms_V8126H264Dec_Desc=
{
	.id			= MS_8126_VIDEO_H264DEC_ID,
	.name		= "MSH264Dec",
	.text		= N_("A video H264 decoder for Xintel 8126"),
	.category	= MS_FILTER_DECODER,
	.enc_fmt	= "H264",
	.ninputs	= 1, /*MS_YUV420P is assumed on this input */
	.noutputs	= 1,
	.init		= dec_8126H264_init,
	.preprocess	= dec_8126H264_preprocess,
	.process	= dec_8126H264_process,
	.postprocess= dec_8126H264_postprocess,
	.uninit		= dec_8126H264_uninit,
	.methods	= NULL
};
MS_FILTER_DESC_EXPORT(ms_V8126H264Enc_Desc)
MS_FILTER_DESC_EXPORT(ms_V8126H264Dec_Desc)
#endif

#endif
