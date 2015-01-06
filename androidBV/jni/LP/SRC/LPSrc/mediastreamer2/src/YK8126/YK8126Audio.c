


#ifdef _YK_XT8126_

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"
#include <sys/soundcard.h>

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
#include "YK8126AudioBV.h"
#include "YK8126Audio.h"
#define FMT8BITS 8
#define FMT16BITS 16
#define FMT8K 8000
#define FMT16K 16000
#define FMT22K 22000
#define FMT44K 44000
#define MONO 0
#define STERO 1


#ifndef ADUIOBLK
#define AUDIOBLK 128 //128   //8*16*1= 128
#endif
MSFilter *YK8126AudioReadNew(MSSndCard *card);
MSFilter *YK8126AudioWriteNew(MSSndCard *card);


void YK8126AudioParamInit(void)
{
	char acOutputVolSet[128]={0x0};
    char acInputVolSet[128]={0x0};
	//set a IPGA volume, from -27 ~ +36 dB
	system("echo 0 > /proc/asound/adda300_RIV");// -27 36
	system("echo 0 > /proc/asound/adda300_LIV");//

	//set a digital input volume, from -50 ~ +30 dB
	//system("echo 7 > /proc/asound/adda300_LADV");                 // modify by cs for set input volume
	sprintf(acInputVolSet,"echo %d > /proc/asound/adda300_LADV",g_iInputVol);
    system(acInputVolSet);
	system("echo 7 > /proc/asound/adda300_RADV");

	//set digital output volume, from 0 ~ -40 dB
	system("echo -30 > /proc/asound/adda300_LDAV");
	system("echo -30 > /proc/asound/adda300_RDAV");
	//set speaker analog output gain, from -40 ~ +6 dB, this affect the final frontend output
	sprintf(acOutputVolSet,"echo %d > /proc/asound/adda300_SPV",g_iOutputVol);
	system(acOutputVolSet);
	//system("echo -10 > /proc/asound/adda300_SPV");//4
	system("echo 0 > /proc/asound/adda300_input_mode input_mode");
	system("echo -6 > /proc/asound/adda300_ALCMIN");
	system("echo 1 > /proc/asound/adda300_BTL_mode");
}

int YK8126AudioSetFormat(int devfd, int bits, int hz, int chn, int *buffSize)
{
	int arg;
	int status;

	printf("bits = %d, hz = %d, chn = %d, devfd = %d\n", bits, hz, chn, devfd);
	if (devfd <= 0)
		return -1;

	status = ioctl(devfd, SOUND_PCM_READ_BITS, &arg);
	if (arg != bits) {
		arg = bits;
		status = ioctl(devfd, SOUND_PCM_WRITE_BITS, &arg);
		if (status == -1)
			printf("SOUND_PCM_WRITE_BITS ioctl failed\n");
		if (arg != bits)
			printf("unable to set sample size\n");
	}

	status = ioctl(devfd, SOUND_PCM_READ_RATE, &arg);
	if (arg != hz) {
		arg = hz;
		status = ioctl(devfd, SOUND_PCM_WRITE_RATE, &arg);
		if (status == -1)
			perror("SOUND_PCM_WRITE_RATE ioctl failed");
	}

	status = ioctl(devfd, SOUND_PCM_READ_CHANNELS, &arg);
	if (arg != chn) {
		arg = chn;
		status = ioctl(devfd, SOUND_PCM_WRITE_CHANNELS, &arg);
		if (status == -1)
			perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
		if (arg != chn)
			perror("unable to set number of channels");
	}

	*buffSize = AUDIOBLK;
	ioctl(devfd, SNDCTL_DSP_GETBLKSIZE, buffSize);
	printf("get buf_size= %d\n", *buffSize);
	if (*buffSize < 4 || *buffSize > 65536) {
		printf("Invalid audio buffers size %d\n", *buffSize);
		exit(-1);
	}
	return 0;
}



int configure_fd(int fd, int bits,int stereo, int rate, int *minsz)
{
	YK8126AudioSetFormat(fd, FMT16BITS, FMT8K, STERO, minsz);
	return fd;
}


static void YK8126AudioOpen(YK8126AudioData* d, int *minsz){
    //LOG_RUNLOG_DEBUG("YK8126AudioOpen test sleep 500ms");
    usleep(500000);                     // sleep 500ms add by cs 
	d->pcmfd_read=open(d->pcmdev,O_RDONLY|O_NONBLOCK);
	if (d->pcmfd_read > 0) {
		d->pcmfd_read=configure_fd(d->pcmfd_read, d->bits, d->stereo, d->rate, minsz);
		LOG_RUNLOG_DEBUG("LP 8126 YK8126AudioOpen pcmfd_read open ok");
	} else {
		ms_error("Cannot open fd in ro mode for [%s]",d->pcmdev);
		LOG_RUNLOG_ERROR("LP 8126 YK8126AudioOpen pcmfd_read open error");
	}
	d->pcmfd_write=open(d->pcmdev,O_WRONLY|O_NONBLOCK);
	if (d->pcmfd_write > 0) {
		d->pcmfd_write=configure_fd(d->pcmfd_write, d->bits, d->stereo, d->rate, minsz);
		LOG_RUNLOG_DEBUG("LP 8126 YK8126AudioOpen pcmfd_write open ok");
	} else {
		ms_error("Cannot open fd in wr mode for [%s]",d->pcmdev);
		LOG_RUNLOG_ERROR("LP 8126 YK8126AudioOpen pcmfd_write open error");
	}
	return ;
}

static void YK8126AudioSetLevel(MSSndCard *card, MSSndCardMixerElem e, int percent)
{

}

static int YK8126AudioGetLevel(MSSndCard *card, MSSndCardMixerElem e)
{
	return -1;
}

static void YK8126AudioSetSource(MSSndCard *card, MSSndCardCapture source)
{
}

static void YK8126AudioInit(MSSndCard *card){
	YK8126AudioData *d=ms_new(YK8126AudioData,1);
	d->pcmdev=NULL;
	d->mixdev=NULL;
	d->pcmfd_read=-1;
	d->pcmfd_write=-1;
	d->read_started=FALSE;
	d->write_started=FALSE;

	d->bits = FMT16BITS;
	d->rate = FMT8K;
	d->stereo = TRUE;
	
	YK8126AudioParamInit();


	qinit(&d->rq);
	d->bufferizer=ms_bufferizer_new();
	ms_mutex_init(&d->mutex,NULL);
	card->data=d;
}

static void YK8126AudioUninit(MSSndCard *card){
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	if (d->pcmdev!=NULL) ms_free(d->pcmdev);
	if (d->mixdev!=NULL) ms_free(d->mixdev);
	ms_bufferizer_destroy(d->bufferizer);
	flushq(&d->rq,0);
	ms_mutex_destroy(&d->mutex);
	ms_free(d);
}

#define DSP_NAME "/dev/dsp"
#define MIXER_NAME "/dev/mixer"

static void YK8126AudioDetect(MSSndCardManager *m);

MSSndCardDesc YK8126AudioCardDesc={
	.driver_type="YK8126Audio",
	.detect=YK8126AudioDetect,
	.init=YK8126AudioInit,
	.set_level=YK8126AudioSetLevel,
	.get_level=YK8126AudioGetLevel,
	.set_capture=YK8126AudioSetSource,
	.create_reader=YK8126AudioReadNew,
	.create_writer=YK8126AudioWriteNew,
	.uninit=YK8126AudioUninit
};

static MSSndCard *YK8126AudioCardNew(const char *pcmdev, const char *mixdev){
	MSSndCard *card=ms_snd_card_new(&YK8126AudioCardDesc);
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	d->pcmdev=ms_strdup(pcmdev);
	d->mixdev=ms_strdup(mixdev);
	card->name=ms_strdup(pcmdev);
	return card;
}

static void YK8126AudioDetect(MSSndCardManager *m){
	int i;
	char pcmdev[sizeof(DSP_NAME)+3];
	char mixdev[sizeof(MIXER_NAME)+3];
	if (access(DSP_NAME,F_OK)==0){
		MSSndCard *card=YK8126AudioCardNew(DSP_NAME,MIXER_NAME);
		ms_snd_card_manager_add_card(m,card);
	}
	for(i=0;i<10;i++){
		snprintf(pcmdev,sizeof(pcmdev),"%s%i",DSP_NAME,i);
		snprintf(mixdev,sizeof(mixdev),"%s%i",MIXER_NAME,i);
		if (access(pcmdev,F_OK)==0){
			MSSndCard *card=YK8126AudioCardNew(pcmdev,mixdev);
			ms_snd_card_manager_add_card(m,card);
		}
	}
}

//#define YK8126AUDIO_PLAY_DEBUG
//#define YK8126AUDIO_REC_DEBUG
static void * YK8126AudioThread(void *p){

#ifdef YK8126AUDIO_PLAY_DEBUG
	FILE *pstPlay;
	if ((pstPlay = fopen("/play", "wb")) == NULL)
	{
		LOG_RUNLOG_ERROR("LP 8126 create play file error");
	}
#endif

#ifdef YK8126AUDIO_REC_DEBUG
	FILE *pstRec;
	if ((pstRec = fopen("/rec", "wb")) == NULL)
	{
		LOG_RUNLOG_ERROR("LP 8126 create rec file error");
	}
#endif
	LOG_RUNLOG_DEBUG("LP 8126 YK8126AudioThread begin");
	MSSndCard *card=(MSSndCard*)p;
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	int bsize=0;
	uint8_t *rtmpbuff=NULL;
	uint8_t *wtmpbuff=NULL;
	int err;
	mblk_t *rm=NULL;
	bool_t did_read=FALSE;
	
	YK8126AudioOpen(d,&bsize);
	if (d->pcmfd_read>=0){
		rtmpbuff=(uint8_t*)alloca(bsize);
	}
	if (d->pcmfd_write>=0){
		wtmpbuff=(uint8_t*)alloca(bsize);
	}

	ms_bufferizer_flush(d->bufferizer); //add by chensq 20120526

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
#ifdef YK8126AUDIO_REC_DEBUG
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
#ifdef YK8126AUDIO_PLAY_DEBUG
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
			LOG_RUNLOG_ERROR("LP 8126 close pcmfd_read and pcmfd_write fail");
		}
		d->pcmfd_read = d->pcmfd_write =-1;
	}else {
		if (d->pcmfd_read>=0)
		{
			if(close(d->pcmfd_read) != 0)
			{
				LOG_RUNLOG_ERROR("LP 8126 close pcmfd_read fail");
			}
			else
			{
				LOG_RUNLOG_DEBUG("LP 8126 close pcmfd_read ok");
			}
			d->pcmfd_read=-1;
		}
		if (d->pcmfd_write>=0)
		{
			if(close(d->pcmfd_write) != 0)
			{
				LOG_RUNLOG_ERROR("LP 8126 close pcmfd_write fail");
			}
			else
			{
				LOG_RUNLOG_DEBUG("LP 8126 close pcmfd_write ok");
			}
			d->pcmfd_write=-1;
		}
	}


#ifdef YK8126AUDIO_PLAY_DEBUG
	fclose(pstPlay);
#endif
#ifdef YK8126AUDIO_REC_DEBUG
	fclose(pstRec);
#endif

	LOG_RUNLOG_DEBUG("LP 8126 YK8126AudioThread end");
	return NULL;
}

static void YK8126AudioStartR(MSSndCard *card){
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	if (d->read_started==FALSE && d->write_started==FALSE){
		d->read_started=TRUE;
#ifdef _YK_XT8126_AV_
		ms_thread_create(&d->thread,NULL,YK8126AudioThread,card);
#endif
#ifdef _YK_XT8126_BV_
		ms_thread_create(&d->thread,NULL,YK8126BVAudioThread,card);
#endif
	}else d->read_started=TRUE;
}

static void YK8126AudioStopR(MSSndCard *card){
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	d->read_started=FALSE;
	if (d->write_started==FALSE){
		ms_thread_join(d->thread,NULL);
	}
}

static void YK8126AudioStartW(MSSndCard *card){
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	if (d->read_started==FALSE && d->write_started==FALSE){
		d->write_started=TRUE;
#ifdef _YK_XT8126_AV_
		ms_thread_create(&d->thread,NULL,YK8126AudioThread,card);
#endif
#ifdef _YK_XT8126_BV_
		ms_thread_create(&d->thread,NULL,YK8126BVAudioThread,card);
#endif
	}else{
		d->write_started=TRUE;
	}
}

static void YK8126AudioStopW(MSSndCard *card){
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	d->write_started=FALSE;
	if (d->read_started==FALSE){
		ms_thread_join(d->thread,NULL);
	}
}

static mblk_t *YK8126AudioGet(MSSndCard *card){
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	mblk_t *m;
	ms_mutex_lock(&d->mutex);
	m=getq(&d->rq);
	ms_mutex_unlock(&d->mutex);
	return m;
}

static void YK8126AudioPut(MSSndCard *card, mblk_t *m){
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	ms_mutex_lock(&d->mutex);
	ms_bufferizer_put(d->bufferizer,m);
	ms_mutex_unlock(&d->mutex);
}


static void YK8126AudioReadPrepross(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	YK8126AudioStartR(card);
}

static void YK8126AudioReadPostProcess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	YK8126AudioStopR(card);
}

static void YK8126AudioReadProcess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=YK8126AudioGet(card))!=NULL){
		ms_queue_put(f->outputs[0],m);
	}
}

static void YK8126AudioWritePreprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	YK8126AudioStartW(card);
}

static void YK8126AudioWritePostprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	YK8126AudioStopW(card);
}

static void YK8126AudioWriteProcess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		YK8126AudioPut(card,m);
	}
}

static int set_rate(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	d->rate=*((int*)arg);
	return 0;
}

static int get_rate(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	*((int*)arg)=d->rate;
	return 0;
}

static int set_nchannels(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	YK8126AudioData *d=(YK8126AudioData*)card->data;
	d->stereo=(*((int*)arg)==2);
	return 0;
}

static MSFilterMethod YK8126AudioMethods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_GET_SAMPLE_RATE	, get_rate },
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	0				, NULL		}
};

MSFilterDesc YK8126AudioReadDesc={
	.id=MS_8126_AUDIO_READ_ID,
	.name="YK8126AudioRead",
	.text="Sound capture filter for 8126Audio drivers",
	.category=MS_FILTER_OTHER,
	.ninputs=0,
	.noutputs=1,
	.preprocess=YK8126AudioReadPrepross,
	.process=YK8126AudioReadProcess,
	.postprocess=YK8126AudioReadPostProcess,
	.methods=YK8126AudioMethods
};


MSFilterDesc YK8126AudioWriteDesc={
	.id=MS_8126_AUDIO_WRITE_ID,
	.name="MS8126AudioWrite",
	.text="Sound playback filter for 8126Audio drivers",
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=0,
	.preprocess=YK8126AudioWritePreprocess,
	.process=YK8126AudioWriteProcess,
	.postprocess=YK8126AudioWritePostprocess,
	.methods=YK8126AudioMethods
};

MSFilter *YK8126AudioReadNew(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&YK8126AudioReadDesc);
	f->data=card;
	return f;
}


MSFilter *YK8126AudioWriteNew(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&YK8126AudioWriteDesc);
	f->data=card;
	return f;
}

MS_FILTER_DESC_EXPORT(YK8126AudioReadDesc)
MS_FILTER_DESC_EXPORT(YK8126AudioWriteDesc)


#endif
