/**
 *@file      Imx27Audio.c
 *@brief     封装IMX27音频操作
 *@author    chensq
 *@version   1.0
 *@date      2012-6-15
 */



#ifdef _YK_IMX27_AV_DEL

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "LOGIntfLayer.h"
#include "Imx27AudioSoundcard.h"
#include "Imx27AudioControls.h"

#define FMT8BITS AFMT_U8                     //8位
#define FMT16BITS AFMT_S16_LE                //16位
#define FMT8K 8000                           //采样率8000
#define FMT16K 16000
#define FMT22K 22000
#define FMT44K 44000
#define MONO 1                               //单声道
#define STERO 2                              //立体声
#define INPUT_TYPE SOUND_MASK_MIC            //录音来源

#ifndef ADUIOBLK
#define AUDIOBLK 128                         //128(8*16*1=128)
#endif

MSFilter *IMX27AudioReadNew(MSSndCard *card);
MSFilter *Imx27AudioWriteNew(MSSndCard *card);

typedef struct Imx27AudioData{
	char *pcmdev;
	char *mixdev;
	int pcmfd_read;
	int pcmfd_write;
	int rate;
	int bits;
	ms_thread_t thread;
	ms_mutex_t mutex;
	queue_t rq;
	MSBufferizer * bufferizer;
	bool_t read_started;
	bool_t write_started;
	bool_t stereo;
} Imx27AudioData;



void Imx27AudioParamInit(void)
{

}


//type:0:play other:rec
static int configure_fd(int fd, int bits, int rate, int channels, int *minsz, int type)
{
	int tmp;

	tmp = bits;
	ioctl(fd, SNDCTL_DSP_SETFMT, &tmp);
	if (tmp != bits)
	{
		printf("error:configure_play_fd bits error\n");
		return -1;
	}

	tmp = rate;
	ioctl(fd, SOUND_PCM_WRITE_RATE, &rate);
	if (tmp != rate)
	{
		printf("error:configure_play_fd rate error\n");
		return -1;
	}

	tmp = channels;
	ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &channels);
	if (tmp != channels)
	{
		printf("error:configure_play_fd channels error\n");
		return -1;
	}

	printf("configure_play_fd : %d %d %d\n", bits, rate, channels);

	if( type != 0 )
	{
		int input_dev = INPUT_TYPE;
		tmp = ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &input_dev);
		if (tmp < 0)
		{
			printf("error:configure_play_fd SOUND_MIXER_WRITE_RECSRC error\n");
			return -1;
		}
	}

	*minsz = AUDIOBLK;
	printf("configure audio fd success:%d\n", fd);
	return fd;
}



static void Imx27AudioOpen(Imx27AudioData* d, int *minsz){
	d->pcmfd_read=open(d->pcmdev,O_RDONLY|O_NONBLOCK);
	if (d->pcmfd_read > 0) {
		d->pcmfd_read=configure_fd(d->pcmfd_read, d->bits, d->rate, d->stereo,  minsz, 1);
		LOG_RUNLOG_DEBUG("LP IMX27 Imx27AudioOpen pcmfd_read open ok");
	} else {
		ms_error("Cannot open fd in ro mode for [%s]",d->pcmdev);
		LOG_RUNLOG_ERROR("LP IMX27 Imx27AudioOpen pcmfd_read open error");
	}
	d->pcmfd_write=open(d->pcmdev,O_WRONLY|O_NONBLOCK);
	if (d->pcmfd_write > 0) {
		d->pcmfd_write=configure_fd(d->pcmfd_write, d->rate, d->bits, d->stereo,  minsz, 0);
		LOG_RUNLOG_DEBUG("LP Imx27AudioOpen pcmfd_write open ok");
	} else {
		ms_error("Cannot open fd in wr mode for [%s]",d->pcmdev);
		LOG_RUNLOG_ERROR("LP Imx27AudioOpen pcmfd_write open error");
	}
	return ;
}

static void Imx27AudioSetLevel(MSSndCard *card, MSSndCardMixerElem e, int percent)
{

}

static int Imx27AudioGetLevel(MSSndCard *card, MSSndCardMixerElem e)
{
	return -1;
}

static void Imx27AudioSetSource(MSSndCard *card, MSSndCardCapture source)
{
}

static void Imx27AudioInit(MSSndCard *card){
	printf("debug: IMX27 Imx27AudioInit begin\n");
	Imx27AudioData *d=ms_new(Imx27AudioData,1);
	d->pcmdev=NULL;
	d->mixdev=NULL;
	d->pcmfd_read=-1;
	d->pcmfd_write=-1;
	d->read_started=FALSE;
	d->write_started=FALSE;

	d->bits = FMT16BITS;
	d->rate = FMT8K;
	d->stereo = MONO;

	Imx27AudioParamInit();


	qinit(&d->rq);
	d->bufferizer=ms_bufferizer_new();
	ms_mutex_init(&d->mutex,NULL);
	card->data=d;

	printf("debug: IMX27 Imx27AudioInit end\n");
}

static void Imx27AudioUninit(MSSndCard *card){
	printf("debug: IMX27 Imx27AudioUninit begin\n");
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	if (d->pcmdev!=NULL) ms_free(d->pcmdev);
	if (d->mixdev!=NULL) ms_free(d->mixdev);
	ms_bufferizer_destroy(d->bufferizer);
	flushq(&d->rq,0);
	ms_mutex_destroy(&d->mutex);
	ms_free(d);
	printf("debug: IMX27 Imx27AudioUninit end\n");
}

#define DSP_NAME "/dev/dsp"
#define MIXER_NAME "/dev/mixer"

static void Imx27AudioDetect(MSSndCardManager *m);

MSSndCardDesc Imx27AudioCardDesc={
	.driver_type="YKImx27Audio",
	.detect=Imx27AudioDetect,
	.init=Imx27AudioInit,
	.set_level=Imx27AudioSetLevel,
	.get_level=Imx27AudioGetLevel,
	.set_capture=Imx27AudioSetSource,
	.create_reader=IMX27AudioReadNew,
	.create_writer=Imx27AudioWriteNew,
	.uninit=Imx27AudioUninit
};

static MSSndCard *Imx27AudioCardNew(const char *pcmdev, const char *mixdev){
	printf("debug: IMX27 Imx27AudioCardNew begin\n");
	MSSndCard *card=ms_snd_card_new(&Imx27AudioCardDesc);
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	d->pcmdev=ms_strdup(pcmdev);
	d->mixdev=ms_strdup(mixdev);
	card->name=ms_strdup(pcmdev);
	printf("debug: IMX27 Imx27AudioCardNew end\n");
	return card;
}

static void Imx27AudioDetect(MSSndCardManager *m){
	printf("debug: IMX27 Imx27AudioCardNew begin\n");
	int i;
	char pcmdev[sizeof(DSP_NAME)+3];
	char mixdev[sizeof(MIXER_NAME)+3];
	if (access(DSP_NAME,F_OK)==0){
		MSSndCard *card=Imx27AudioCardNew(DSP_NAME,MIXER_NAME);
		ms_snd_card_manager_add_card(m,card);
	}
	for(i=0;i<10;i++){
		snprintf(pcmdev,sizeof(pcmdev),"%s%i",DSP_NAME,i);
		snprintf(mixdev,sizeof(mixdev),"%s%i",MIXER_NAME,i);
		if (access(pcmdev,F_OK)==0){
			MSSndCard *card=Imx27AudioCardNew(pcmdev,mixdev);
			ms_snd_card_manager_add_card(m,card);
		}
	}
	printf("debug: IMX27 Imx27AudioCardNew end\n");
}

//#define Imx27Audio_PLAY_DEBUG
//#define Imx27Audio_REC_DEBUG
static void * Imx27AudioThread(void *p){


	printf("debug: IMX27 Imx27AudioThread begin\n");


#ifdef Imx27Audio_PLAY_DEBUG
	FILE *pstPlay;
	if ((pstPlay = fopen("/play", "wb")) == NULL)
	{
		LOG_RUNLOG_ERROR("create play file error");
	}
#endif

#ifdef Imx27Audio_REC_DEBUG
	FILE *pstRec;
	if ((pstRec = fopen("/rec", "wb")) == NULL)
	{
		LOG_RUNLOG_ERROR("create rec file error");
	}
#endif
	LOG_RUNLOG_DEBUG("Imx27AudioThread begin\n");
	MSSndCard *card=(MSSndCard*)p;
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	int bsize=0;
	uint8_t *rtmpbuff=NULL;
	uint8_t *wtmpbuff=NULL;
	int err;
	mblk_t *rm=NULL;
	bool_t did_read=FALSE;

	Imx27AudioOpen(d,&bsize);
	if (d->pcmfd_read>=0){
		rtmpbuff=(uint8_t*)alloca(bsize);
	}
	if (d->pcmfd_write>=0){
		wtmpbuff=(uint8_t*)alloca(bsize);
	}

	ms_bufferizer_flush(d->bufferizer);

	while(d->read_started || d->write_started){
		did_read=FALSE;
		if (d->pcmfd_read>=0){
			if (d->read_started){
				if (rm==NULL) rm=allocb(bsize,0);
				err=read(d->pcmfd_read,rm->b_wptr,bsize);
				if (err<0){
					ms_warning("Fail to read %i bytes from soundcard: %s",
					bsize,strerror(errno));
				}else{
#ifdef Imx27Audio_REC_DEBUG
					fwrite(rm->b_wptr, AUDIOBLK, 1, pstRec);
#endif
					did_read=TRUE;
					rm->b_wptr+=err;
					ms_mutex_lock(&d->mutex);
					putq(&d->rq,rm);
					ms_mutex_unlock(&d->mutex);
					rm=NULL;
				}
			}else {
				/* case where we have no reader filtern the read is performed for synchronisation */
				int sz = read(d->pcmfd_read,rtmpbuff,bsize);
				if( sz==-1) ms_warning("sound device read error %s ",strerror(errno));
				else did_read=TRUE;
			}
		}
		if (d->pcmfd_write>=0){
			if (d->write_started){
				err=ms_bufferizer_read(d->bufferizer,wtmpbuff,bsize);
				if (err==bsize){
					err=write(d->pcmfd_write,wtmpbuff,bsize);
#ifdef Imx27Audio_PLAY_DEBUG
					fwrite(wtmpbuff, AUDIOBLK, 1, pstPlay);
#endif
					if (err<0){
						ms_warning("Fail to write %i bytes from soundcard: %s",
						bsize,strerror(errno));
					}
				}
			}else {
				int sz;
				memset(wtmpbuff,0,bsize);
				sz = write(d->pcmfd_write,wtmpbuff,bsize);
				if( sz!=bsize) ms_warning("sound device write returned %i !",sz);
			}
		}
		if (!did_read) usleep(20000); /*avoid 100%cpu loop for nothing*/
	}

	if (d->pcmfd_read==d->pcmfd_write && d->pcmfd_read>=0 )
	{
		if( close(d->pcmfd_read) != 0 )
		{
			LOG_RUNLOG_ERROR("close pcmfd_read and pcmfd_write fail");
		}
		d->pcmfd_read = d->pcmfd_write =-1;
	}else {
		if (d->pcmfd_read>=0)
		{
			if(close(d->pcmfd_read) != 0)
			{
				LOG_RUNLOG_ERROR("close pcmfd_read fail");
			}
			else
			{
				LOG_RUNLOG_DEBUG("close pcmfd_read ok");
			}
			d->pcmfd_read=-1;
		}
		if (d->pcmfd_write>=0)
		{
			if(close(d->pcmfd_write) != 0)
			{
				LOG_RUNLOG_ERROR("close pcmfd_write fail");
			}
			else
			{
				LOG_RUNLOG_DEBUG("close pcmfd_write ok");
			}
			d->pcmfd_write=-1;
		}
	}


#ifdef Imx27Audio_PLAY_DEBUG
	fclose(pstPlay);
#endif
#ifdef Imx27Audio_REC_DEBUG
	fclose(pstRec);
#endif

	printf("debug: IMX27 Imx27AudioThread end\n");
	return NULL;
}

static void Imx27AudioStartR(MSSndCard *card){
	printf("debug: IMX27 Imx27AudioStartR begin\n");
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	if (d->read_started==FALSE && d->write_started==FALSE){
		d->read_started=TRUE;
		ms_thread_create(&d->thread,NULL,Imx27AudioThread,card);
	}else d->read_started=TRUE;
	printf("debug: IMX27 Imx27AudioStartR end\n");
}

static void Imx27AudioStopR(MSSndCard *card){
	printf("debug: IMX27 Imx27AudioStopR begin\n");
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	d->read_started=FALSE;
	if (d->write_started==FALSE){
		ms_thread_join(d->thread,NULL);
	}
	printf("debug: IMX27 Imx27AudioStopR end\n");
}

static void Imx27AudioStartW(MSSndCard *card){
	printf("debug: IMX27 Imx27AudioStartW begin\n");
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	if (d->read_started==FALSE && d->write_started==FALSE){
		d->write_started=TRUE;
		ms_thread_create(&d->thread,NULL,Imx27AudioThread,card);
	}else{
		d->write_started=TRUE;
	}
	printf("debug: IMX27 Imx27AudioStartW end\n");
}

static void Imx27AudioStopW(MSSndCard *card){
	printf("debug: IMX27 Imx27AudioStopW begin\n");
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	d->write_started=FALSE;
	if (d->read_started==FALSE){
		ms_thread_join(d->thread,NULL);
	}
	printf("debug: IMX27 Imx27AudioStopW end\n");
}

static mblk_t *Imx27AudioGet(MSSndCard *card){
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	mblk_t *m;
	ms_mutex_lock(&d->mutex);
	m=getq(&d->rq);
	ms_mutex_unlock(&d->mutex);
	return m;
}

static void Imx27AudioPut(MSSndCard *card, mblk_t *m){
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	ms_mutex_lock(&d->mutex);
	ms_bufferizer_put(d->bufferizer,m);
	ms_mutex_unlock(&d->mutex);
}


static void Imx27AudioReadPrepross(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	Imx27AudioStartR(card);
}

static void Imx27AudioReadPostProcess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	Imx27AudioStopR(card);
}

static void Imx27AudioReadProcess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=Imx27AudioGet(card))!=NULL){
		ms_queue_put(f->outputs[0],m);
	}
}

static void Imx27AudioWritePreprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	Imx27AudioStartW(card);
}

static void Imx27AudioWritePostprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	Imx27AudioStopW(card);
}

static void Imx27AudioWriteProcess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		Imx27AudioPut(card,m);
	}
}

static int set_rate(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	d->rate=*((int*)arg);
	return 0;
}

static int get_rate(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	*((int*)arg)=d->rate;
	return 0;
}

static int set_nchannels(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	Imx27AudioData *d=(Imx27AudioData*)card->data;
	d->stereo=(*((int*)arg)==2);
	return 0;
}





static MSFilterMethod Imx27AudioMethods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_GET_SAMPLE_RATE	, get_rate },
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	0				, NULL		}
};

MSFilterDesc Imx27AudioReadDesc={
	.id=MS_IMX27_AUDIO_READ_ID,
	.name="MSImx27AudioRead",
	.text="Sound capture filter for Imx27Audio drivers",
	.category=MS_FILTER_OTHER,
	.ninputs=0,
	.noutputs=1,
	.preprocess=Imx27AudioReadPrepross,
	.process=Imx27AudioReadProcess,
	.postprocess=Imx27AudioReadPostProcess,
	.methods=Imx27AudioMethods
};


MSFilterDesc Imx27AudioWriteDesc={
	.id=MS_IMX27_AUDIO_WRITE_ID,
	.name="MSImx27AudioWrite",
	.text="Sound playback filter for Imx27Audio drivers",
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=0,
	.preprocess=Imx27AudioWritePreprocess,
	.process=Imx27AudioWriteProcess,
	.postprocess=Imx27AudioWritePostprocess,
	.methods=Imx27AudioMethods
};

MSFilter *IMX27AudioReadNew(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&Imx27AudioReadDesc);
	f->data=card;
	return f;
}


MSFilter *Imx27AudioWriteNew(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&Imx27AudioWriteDesc);
	f->data=card;
	return f;
}

MS_FILTER_DESC_EXPORT(Imx27AudioReadDesc)
MS_FILTER_DESC_EXPORT(Imx27AudioWriteDesc)


#endif

