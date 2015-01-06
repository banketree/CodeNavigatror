#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msticker.h>
#include <mediastreamer2/mssndcard.h>
#include "AudioTrack.h"
#include "AudioRecord.h"
#include "String8.h"
#include "android/log.h"

//static int iReadCount = 0;
//static int iReadCb = 0;
//static int iWriteCount = 0;
//static int iWriteCb = 0;

#undef MIN
#define MIN(a,b)	((a)>(b) ? (b) : (a))
#undef MAX
#define MAX(a,b)	((a)>(b) ? (a) : (b))

#define NATIVE_USE_HARDWARE_RATE 1

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "AUDIO", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "AUDIO", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , "AUDIO", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , "AUDIO", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "AUDIO", __VA_ARGS__)

#define MAKEWORD(byte1,byte2)		((unsigned short)byte2 << 8 | byte1)
#define LOWORD(dword)				((unsigned short)dword)
#define LOBYTE(word)					((unsigned char)word)
#define HIBYTE(word)					((unsigned char)(word >> 8))

static const float audio_buf_ms=0.01;
static Library *libmedia=0;
static Library *libutils=0;

void AuidoReadBufInit(void);
void AudioReadBufUninit(void);

using namespace::fake_android;

static int std_sample_rates[]={
	48000,44100,32000,22050,16000,8000,-1
};


#define AUDIO_READ_BUF_SIZE    10000
#define AUDIO_PLAY_BUF_SIZE	 20000
typedef struct _AudioReadBuf
{
	char Buf[AUDIO_READ_BUF_SIZE];
	char *pIn;
	char *pOut;
	unsigned int BufSize;
	unsigned int BufEndAddress;
}AudioReadBuf;
AudioReadBuf *gpAudioReadBuf = NULL;

typedef struct _AudioPlayBuf
{
	char Buf[AUDIO_PLAY_BUF_SIZE];
	char *pIn;
	char *pOut;
	unsigned int BufSize;
	unsigned int BufEndAddress;
}AudioPlayBuf;
AudioPlayBuf *gpAudioPlayBuf = NULL;



void AuidoReadBufInit(void)
{
	gpAudioReadBuf = new(AudioReadBuf);
	memset(gpAudioReadBuf->Buf, 0, AUDIO_READ_BUF_SIZE);
	gpAudioReadBuf->BufSize = 0;
	gpAudioReadBuf->pIn = gpAudioReadBuf->Buf;
	gpAudioReadBuf->pOut = gpAudioReadBuf->Buf;
	gpAudioReadBuf->BufEndAddress = (unsigned int)(gpAudioReadBuf->Buf) + AUDIO_READ_BUF_SIZE -1;
}

void AuidoPlayBufInit(void)
{
	gpAudioPlayBuf = new(AudioPlayBuf);
	memset(gpAudioPlayBuf->Buf, 0, AUDIO_PLAY_BUF_SIZE);
	gpAudioPlayBuf->BufSize = 0;
	gpAudioPlayBuf->pIn = gpAudioPlayBuf->Buf;
	gpAudioPlayBuf->pOut = gpAudioPlayBuf->Buf;
	gpAudioPlayBuf->BufEndAddress = (unsigned int)(gpAudioPlayBuf->Buf) + AUDIO_PLAY_BUF_SIZE -1;
}

void AudioReadBufUninit(void)
{
	if(gpAudioReadBuf != NULL)
	{
		delete gpAudioReadBuf;
		gpAudioReadBuf = NULL;
	}
}

void AudioPlayBufUninit(void)
{
	if(gpAudioPlayBuf != NULL)
	{
		delete gpAudioPlayBuf;
		gpAudioPlayBuf = NULL;
	}
}

struct LinuxNativeSndCardData{
	LinuxNativeSndCardData(): mVoipMode(0) ,mIoHandle(0){
		/* try to use the same sampling rate as the playback.*/
		int hwrate;
//		enableVoipMode();				//modify by xuqd
		if (AudioSystem::getOutputSamplingRate(&hwrate,AUDIO_STREAM_VOICE_CALL)==0){
			printf("Hardware output sampling rate is %i\n",hwrate);
		}

		mPlayRate=mRecRate=8000;
		if (AudioRecord::getMinFrameCount(&mRecFrames, mRecRate, AUDIO_FORMAT_PCM_16_BIT,1)==0)
		{
				printf("Minimal AudioRecord buf frame size at %i Hz is %i\n",mRecRate,mRecFrames);
		}

//		mPlayRate=mRecRate=hwrate;
//		for(int i=0;;){
//			int stdrate=std_sample_rates[i];
//			if (stdrate>mRecRate) {
//				i++;
//				continue;
//			}
//			if (AudioRecord::getMinFrameCount(&mRecFrames, mRecRate, AUDIO_FORMAT_PCM_16_BIT,1)==0){
//				printf("Minimal AudioRecord buf frame size at %i Hz is %i\n",mRecRate,mRecFrames);
//				break;
//			}else{
//				printf("Recording at  %i hz is not supported\n",mRecRate);
//				i++;
//				if (std_sample_rates[i]==-1){
//					printf("Cannot find suitable sampling rate for recording !\n");
//					return;
//				}
//				mRecRate=std_sample_rates[i];
//			}
//		}
//		disableVoipMode();		//modify by xuqd
#if 0
		mIoHandle=AudioSystem::getInput(AUDIO_SOURCE_VOICE_COMMUNICATION,mRecRate,AUDIO_FORMAT_PCM_16_BIT,AUDIO_CHANNEL_IN_MONO,(audio_in_acoustics_t)0,0);
		if (mIoHandle==0){
			printf("No io handle for AUDIO_SOURCE_VOICE_COMMUNICATION, trying AUDIO_SOURCE_VOICE_CALL\n");
			mIoHandle=AudioSystem::getInput(AUDIO_SOURCE_VOICE_CALL,mRecRate,AUDIO_FORMAT_PCM_16_BIT,AUDIO_CHANNEL_IN_MONO,(audio_in_acoustics_t)0,0);
			if (mIoHandle==0){
				printf("No io handle for capture.\n");
			}
		}
#endif
	}
	void enableVoipMode(){
		mVoipMode++;
		if (mVoipMode==1){
			//hack for samsung devices
			status_t err;
			String8 params("voip=on");
			if ((err = AudioSystem::setParameters(mIoHandle,params))==0){
				printf("voip=on is set.\n");
			}else printf("Could not set voip=on: err=%d.\n", err);
		}
	}
	void disableVoipMode(){
		mVoipMode--;
		if (mVoipMode==0){
			status_t err;
			String8 params("voip=off");
			if ((err = AudioSystem::setParameters(mIoHandle,params))==0){
				printf("voip=off is set.\n");
			}else printf("Could not set voip=off: err=%d.\n", err);
		}
	}
	int mVoipMode;
	int mPlayRate;
	int mRecRate;
	int mRecFrames;
	audio_io_handle_t mIoHandle;
};

static LinuxNativeSndCardData *g_NativeSndCardData = NULL;

struct LinuxSndReadData{
	LinuxSndReadData() : rec(0){
		rate=8000;
		nchannels=1;
		qinit(&q);
		ms_mutex_init(&mutex,NULL);
		started=false;
		nbufs=0;
		audio_source=AUDIO_SOURCE_DEFAULT;
		mTickerSynchronizer=NULL;
	}
	~LinuxSndReadData(){
		ms_mutex_destroy(&mutex);
		flushq(&q,0);
		delete rec;
	}
	void setCard(LinuxNativeSndCardData *card){
		mCard=card;
#ifdef NATIVE_USE_HARDWARE_RATE
		printf("card->mRecFrames=%d\n",card->mRecFrames);
		rate=card->mRecRate;
		rec_buf_size=card->mRecFrames;
#endif
	}
	MSFilter *mFilter;
	LinuxNativeSndCardData *mCard;
	audio_source_t audio_source;
	int rate;
	int nchannels;
	ms_mutex_t mutex;
	queue_t q;
	AudioRecord *rec;
	int nbufs;
	int rec_buf_size;
	MSTickerSynchronizer *mTickerSynchronizer;
	int64_t read_samples;
	audio_io_handle_t iohandle;
	bool started;
};

static LinuxSndReadData *g_SndReadData = NULL;

struct LinuxSndWriteData
{
	LinuxSndWriteData(){
		stype=AUDIO_STREAM_VOICE_CALL;
		rate=8000;
		mCard = NULL;
		nchannels=1;
		tr = NULL;
		nFramesRequested=0;
		ms_mutex_init(&mutex,NULL);
		ms_bufferizer_init(&bf);
	}
	~LinuxSndWriteData(){
		ms_mutex_destroy(&mutex);
		ms_bufferizer_uninit(&bf);
	}
	void setCard(LinuxNativeSndCardData *card){
		if(mCard) return;
		mCard=card;
#ifdef NATIVE_USE_HARDWARE_RATE
		rate=card->mPlayRate;
#endif
	}
	LinuxNativeSndCardData *mCard;
	audio_stream_type_t stype;
	int rate;
	int nchannels;
	ms_mutex_t mutex;
	MSBufferizer bf;
	AudioTrack *tr;
	int nbufs;
	int nFramesRequested;
	bool mStarted;
	uint64_t flowControlStart;
	int minBufferFilling;
};

static LinuxSndWriteData *g_SndWriteData = NULL;
static volatile int g_doflg = 1;


int linux_snd_card_detect(void)
{
	LOGD("AndroidAudio android_snd_card_detect for qiushi");
	if (!libmedia) libmedia=Library::load("/system/lib/libmedia.so");
	if (!libutils) libutils=Library::load("/system/lib/libutils.so");

	if (libmedia && libutils)
	{
		/*perform initializations in order rather than in a if statement so that all missing symbols are shown in logs*/
		bool audio_record_loaded=AudioRecordImpl::init(libmedia);
		bool audio_track_loaded=AudioTrackImpl::init(libmedia);
		bool audio_system_loaded=AudioSystemImpl::init(libmedia);
		bool string8_loaded=String8Impl::init(libutils);
		if (audio_record_loaded && audio_track_loaded && audio_system_loaded && string8_loaded)
		{
			LOGD("Native linux sound support available.\n");
//			MSSndCard *card=android_snd_card_new();
//			ms_snd_card_manager_add_card(m,card);
			g_NativeSndCardData = new LinuxNativeSndCardData();
		}
		else
		{
			LOGE("Native linux sound support not available.\n");
			return -1;
		}
	}

	return 0;
}

static void compute_timespec(LinuxSndReadData *d) {
	static int count = 0;
	uint64_t ns = ((1000 * d->read_samples) / (uint64_t) d->rate) * 1000000;
	MSTimeSpec ts;
	ts.tv_nsec = ns % 1000000000;
	ts.tv_sec = ns / 1000000000;
	double av_skew = ms_ticker_synchronizer_set_external_time(d->mTickerSynchronizer, &ts);
	if ((++count) % 100 == 0)
		printf("sound/wall clock skew is average=%f ms\n", av_skew);
}

//int i=0;
static void linux_snd_read_cb(int event, void* user, void *p_info)
{
//	LinuxSndReadData *ad=(LinuxSndReadData*)user;
	LinuxSndReadData *ad=g_SndReadData;

	if (!ad->started) return;
//	if (ad->mTickerSynchronizer==NULL)
//	{
//		MSFilter *obj=ad->mFilter;
//		ad->mTickerSynchronizer = ms_ticker_synchronizer_new();
//		ms_ticker_set_time_func(obj->ticker,(uint64_t (*)(void*))ms_ticker_synchronizer_get_corrected_time, ad->mTickerSynchronizer);
//	}

	if (event==AudioRecord::EVENT_MORE_DATA)
	{
		AudioRecord::Buffer * info=reinterpret_cast<AudioRecord::Buffer*>(p_info);
		mblk_t *m=allocb(info->size,0);
		memcpy(m->b_wptr,info->raw,info->size);
		m->b_wptr+=info->size;
		ad->read_samples+=info->frameCount;

		ms_mutex_lock(&ad->mutex);
//		compute_timespec(ad);
		queue_t *q = &ad->q;
//		printf("&ad->q.q_mcount3 = %d\n", q->q_mcount);
		putq(&ad->q,m);
//		printf("&ad->q.q_mcount4 = %d\n", q->q_mcount);
		ms_mutex_unlock(&ad->mutex);
//		i++;
//		printf("i=%d, linux_snd_read_cb: got %i bytes\n",i,info->size);
	}
	else if (event==AudioRecord::EVENT_OVERRUN)
	{
		printf("AudioRecord overrun\n");
	}
}

int linux_snd_read_init(void)
{
	LOGV("AndroidAudio linux_snd_read_init enter:g_SndReadData=%x", g_SndReadData);
	printf("AndroidAudio linux_snd_read_init enter:g_SndReadData=%x", g_SndReadData);
	AuidoReadBufInit();

	int ret=0;

//	if(g_SndReadData == NULL)
//	{
		g_SndReadData=new LinuxSndReadData();
		LOGV("AndroidAudio linux_snd_read_init will new LinuxSndReadData:g_SndReadData=%x", g_SndReadData);
		printf("AndroidAudio linux_snd_read_init will new LinuxSndReadData:g_SndReadData=%x", g_SndReadData);
//	}


	LinuxSndReadData *ad = g_SndReadData;

	ad->setCard(g_NativeSndCardData);

	status_t  ss;
	int notify_frames=(int)(audio_buf_ms*(float)ad->rate);

	ad->rec_buf_size*=4;
	ad->read_samples=0;
	ad->audio_source=AUDIO_SOURCE_VOICE_COMMUNICATION;

	for(int i=0;i<2;i++)
	{
		ad->rec=new AudioRecord(ad->audio_source,
				ad->rate,
						AUDIO_FORMAT_PCM_16_BIT,
						audio_channel_in_mask_from_count(ad->nchannels),
						ad->rec_buf_size,
						(AudioRecord::record_flags)0 /*flags ??*/
						,linux_snd_read_cb,ad,notify_frames,0);
		ss=ad->rec->initCheck();
		if (ss!=0)
		{
			printf("Problem when setting up AudioRecord:%s  source=%i,rate=%i,framecount=%i\n",strerror(-ss),ad->audio_source,ad->rate,ad->rec_buf_size);
			delete ad->rec;
			ad->rec=0;
			if (i == 0)
			{
				printf("Retrying with AUDIO_SOURCE_MIC");
				ad->audio_source=AUDIO_SOURCE_MIC;
			}
//		}else break;
		}
		else
		{
			printf("audio initCheck succeed .......framecount=%i\n", ad->rec_buf_size);
		}
	}

//	if(i == 2)
//	{
//		ret = -1;
//	}

	LOGV("AndroidAudio linux_snd_read_init end");
	printf("AndroidAudio linux_snd_read_init end");
	return ret;
}
//int j=0;
int linux_snd_read(char *read_buf)
{
	int audio_len = 0;
	int audio_block_len = 0;
	int cplen=0;
	int sz=0;
	int ret;
	LinuxSndReadData *ad=g_SndReadData;
	mblk_t *om;

	if (ad->rec==0) return -1;
	if (!ad->started) {
		printf("linux_snd_read ad->started=true\n");
		ad->started=true;
		ad->rec->start();
	}

	ms_mutex_lock(&ad->mutex);

	if(((om=getq(&ad->q))!=NULL))
	{
		audio_block_len = msgdsize(om);
		memcpy(read_buf, om->b_rptr, audio_block_len);
		audio_len += audio_block_len;
	}

	ms_mutex_unlock(&ad->mutex);

	return audio_len;

//	int read_len = 128;
//
//	while(sz < read_len)
//	{
//		if(((om=getq(&ad->q))==NULL) && (audio_block_len == 0))
//		{
//			printf("om = NULL \n");
//			ret = -1;
//			break;
//		}
//
//		if(((om=getq(&ad->q))!=NULL) && (audio_block_len == 0))
//		{
//			audio_block_len = msgdsize(om);
//		}
//		j++;
//		cplen=MIN(audio_block_len,read_len-sz);
//		printf("j=%d, cplen = %d\n", j, cplen);
//		if(cplen != 0)
//		{
//		memcpy(read_buf, om->b_rptr, cplen);
//		sz += cplen;
//		read_buf += cplen;
//		audio_len += cplen;
//		om->b_rptr += cplen;
//		audio_block_len -= cplen;
//		}
//	}
//	ms_mutex_unlock(&ad->mutex);
//
//	if(ret = -1)
//	{
//		return -1;
//	}
//	else
//	{
//		return audio_len;
//	}
}

void linux_snd_read_uninit(void)
{
	LinuxSndReadData *ad=g_SndReadData;

	LOGV("AndroidAudio linux_snd_read_uninit enter:g_SndReadData=%x ad=%x", g_SndReadData, ad);
	printf("AndroidAudio linux_snd_read_uninit enter:g_SndReadData=%x ad=%x", g_SndReadData, ad);
	//g_doflg = 0;
	if (ad->rec!=0) {
		ad->started=false;
		ad->rec->stop();
		delete ad->rec;
		ad->rec=0;
	}

//	ad->mCard->disableVoipMode();			//modify by xuqd
	if(ad != NULL)
	{
		LOGV("AndroidAudio delete g_SndReadData");
		printf("AndroidAudio delete g_SndReadData");
		printf("delete g_SndReadData\n");
		delete ad;
		ad = NULL;
	}

	AudioReadBufUninit();
	printf("AndroidAudio linux_snd_read_uninit end:g_SndReadData=%x ad=%x", g_SndReadData, ad);
	LOGV("AndroidAudio linux_snd_read_uninit end:g_SndReadData=%x ad=%x", g_SndReadData, ad);
}

unsigned writeTotalLen = 0;
static void linux_snd_write_cb(int event, void *user, void * p_info){
	LinuxSndWriteData *ad=(LinuxSndWriteData*)user;

	if (event==AudioTrack::EVENT_MORE_DATA){
		AudioTrack::Buffer *info=reinterpret_cast<AudioTrack::Buffer *>(p_info);
		int avail;
		int ask;
//		printf("~~~~~~~~~~~android_snd_write_cb~~~~~~~~~~~~~~~~~~\n");
		ms_mutex_lock(&ad->mutex);
		ask = info->size;

		if(ask == 46)
		{
			printf("i am here!!!!!!!!!!!!!!!!!!\n");
			ms_mutex_unlock(&ad->mutex);
			return;
		}

//		avail = ms_bufferizer_get_avail(&ad->bf);
//		/* Drop the samples accumulated before the first callback asking for data. */
//		if ((ad->nbufs == 0) && (avail > (ask * 2))) {
////			printf("i am here~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
//			ms_bufferizer_skip_bytes(&ad->bf, avail - (ask * 2));
//		}
//		if (avail != 0) {
//			if ((ad->minBufferFilling == -1)) {
//				ad->minBufferFilling = avail;
//			} else if (avail < ad->minBufferFilling) {
//				ad->minBufferFilling = avail;
//			}
//		}

		avail = gpAudioPlayBuf->BufSize;

		if ((ad->nbufs == 0) && (avail > ask))
		{
			gpAudioPlayBuf->BufSize -= (avail - ask);
			for(int i; i<(avail - ask); i++)
			{
				gpAudioPlayBuf->pOut ++;
				if((unsigned int)(gpAudioPlayBuf->pOut) > gpAudioPlayBuf->BufEndAddress)
				{
					gpAudioPlayBuf->pOut = gpAudioPlayBuf->Buf;
				}
			}

			avail = ask;
		}

		info->size = MIN(avail, ask);

		//printf("linux_snd_write_cb: avail = %d, ask = %d, info->size = %d \n", avail, ask, info->size);

		uint8_t *pRaw = (uint8_t*)info->raw;

		gpAudioPlayBuf->BufSize -= info->size;

		for(int j=0; j<info->size; j++)
		{
			*pRaw = *gpAudioPlayBuf->pOut;
			pRaw ++;
			gpAudioPlayBuf->pOut++;
			if((unsigned int)(gpAudioPlayBuf->pOut) > gpAudioPlayBuf->BufEndAddress)
			{
				gpAudioPlayBuf->pOut = gpAudioPlayBuf->Buf;
			}
		}
		//printf("linux_snd_write_cb: gpAudioPlayBuf->BufSize = %d, gpAudioPlayBuf->pIn addr = 0x%x, gpAudioPlayBuf->pOut addr = 0x%x\n", gpAudioPlayBuf->BufSize, (unsigned int)(gpAudioPlayBuf->pIn), (unsigned int)gpAudioPlayBuf->pOut);
		info->frameCount = info->size / 2;

#ifdef TRACE_SND_WRITE_TIMINGS
		{
			MSTimeSpec ts;
			ms_get_cur_time(&ts);
			printf("%03u.%03u: AudioTrack ask %d, given %d, available %d [%f ms]\n", (uint32_t) ts.tv_sec, (uint32_t) (ts.tv_nsec / 1000),
				ask, info->size, avail, (avail / (2 * ad->nchannels)) / (ad->rate / 1000.0));
		}
#endif
//		MSBufferizer *msbuf = &ad->bf;
//		if (info->size > 0){
//			ms_bufferizer_read(&ad->bf,(uint8_t*)info->raw,info->size);
////			printf("linux_snd_write_cb: msbuf->size = %d \n", msbuf->size);
//			//LOGD("LP ansound ms_bufferizer_read");
//			writeTotalLen += info->size;
//			printf("writeTotalLen = %d \n", writeTotalLen);
//			info->frameCount = info->size / 2;
//		}
		ms_mutex_unlock(&ad->mutex);
		ad->nbufs++;
//		ad->nFramesRequested+=info->frameCount;
//		printf("ad->nbufs = %d, ad->nFramesRequested = %d\n", ad->nbufs, ad->nFramesRequested);
		/*
		if (ad->nbufs %100){
			uint32_t pos;
			if (ad->tr->getPosition(&pos)==0){
				ms_message("Requested frames: %i, playback position: %i, diff=%i",ad->nFramesRequested,pos,ad->nFramesRequested-pos);
			}
		}
		*/
	}else if (event==AudioTrack::EVENT_UNDERRUN)
	{
		ms_mutex_lock(&ad->mutex);
#ifdef TRACE_SND_WRITE_TIMINGS
		{
			MSTimeSpec ts;
			ms_get_cur_time(&ts);
			printf("%03u.%03u: PCM playback underrun: available %d\n", (uint32_t) ts.tv_sec, (uint32_t) (ts.tv_nsec / 1000), ms_bufferizer_get_avail(&ad->bf));
		}
#else
//		printf("PCM playback underrun: available %d\n", ms_bufferizer_get_avail(&ad->bf));
		printf("PCM playback underrun: available %d\n", gpAudioPlayBuf->BufSize);
#endif
		ms_mutex_unlock(&ad->mutex);
	}
	else
	{
		printf("Untracked event %i\n",event);
	}
}

int linux_snd_write_init(void)
{
	LOGV("AndroidAudio linux_snd_write_init enter:g_SndWriteData=%x", g_SndWriteData);
	printf("AndroidAudio linux_snd_write_init enter:g_SndWriteData=%x\n", g_SndWriteData);
	AuidoPlayBufInit();

	int play_buf_size;
	status_t s;

//	if(g_SndWriteData == NULL)
//	{
		g_SndWriteData=new LinuxSndWriteData();
		LOGV("AndroidAudio linux_snd_write_init new LinuxSndWriteData:g_SndWriteData=%x", g_SndWriteData);
		printf("AndroidAudio linux_snd_write_init new LinuxSndWriteData:g_SndWriteData=%x\n", g_SndWriteData);
//	}

	LinuxSndWriteData *ad=g_SndWriteData;

	ad->setCard(g_NativeSndCardData);

	int notify_frames=(int)(audio_buf_ms*(float)ad->rate);

//	ad->mCard->enableVoipMode();			//modify by xuqd
	ad->nFramesRequested=0;

	if (AudioTrack::getMinFrameCount(&play_buf_size,ad->stype,ad->rate)==0)
	{
		LOGV("LP ansound AudioTrack: min frame count is %i",play_buf_size);
		printf("LP ansound AudioTrack: min frame count is %i\n",play_buf_size);
	}
	else
	{
		LOGV("LP ansound AudioTrack::getMinFrameCount() error");
		printf("LP ansound AudioTrack::getMinFrameCount() error\n");
		return -1;
	}
	if(!ad->tr)
	{
		LOGV("LP ansound AudioTrack is not exist ,create it");
		printf("LP ansound AudioTrack is not exist ,create it\n");
		ad->tr=new AudioTrack(ad->stype,
	                     ad->rate,
	                     AUDIO_FORMAT_PCM_16_BIT,
	                     audio_channel_out_mask_from_count(ad->nchannels),
	                     play_buf_size,
	                     AUDIO_OUTPUT_FLAG_NONE, // AUDIO_OUTPUT_FLAG_NONE,
	                     linux_snd_write_cb, ad,notify_frames,0);

		ad->tr->start();
		s=ad->tr->initCheck();
		if (s!=0)
		{
			printf("Problem setting up AudioTrack: %s\n",strerror(-s));
			delete ad->tr;
			ad->tr=NULL;
			return -1;
		}

		ad->nbufs=0;
		LOGV("LP ansound AudioTrack latency estimated to %i ms",ad->tr->latency());
		printf("LP ansound AudioTrack latency estimated to %i ms\n",ad->tr->latency());
		ad->mStarted=true;
//		ad->flowControlStart = obj->ticker->time;
		ad->minBufferFilling = -1;

//		ad->tr->stop();
//		ad->mStarted=false;
	}



	return 0;
}

void linux_snd_card_set_playflg(int doflg)
{
	g_doflg = doflg;
	LOGD("[stdout] LP g_doflg = %d", g_doflg);
}

int linux_snd_write(char *write_buf, int write_len)
{
	LinuxSndWriteData *ad = g_SndWriteData;
	MSBufferizer *msbf = &ad->bf;

	//mblk_t *om=allocb(write_len,0);

	if (!ad->mStarted)
	{
		ms_warning("ad->mStarted is false start audiotrack");
		ad->tr->start();
		ad->mStarted = true;
	}

	ms_mutex_lock(&ad->mutex);

	gpAudioPlayBuf->BufSize += write_len;

	for(int i=0; i<write_len; i++)
	{
		*gpAudioPlayBuf->pIn = *write_buf;
		gpAudioPlayBuf->pIn ++;
		write_buf ++;
		if((unsigned int)(gpAudioPlayBuf->pIn) > gpAudioPlayBuf->BufEndAddress)
		{
//			printf("i am here!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			gpAudioPlayBuf->pIn = gpAudioPlayBuf->Buf;
		}
	}

	//printf("linux_snd_write: gpAudioPlayBuf->BufSize = %d, gpAudioPlayBuf->pIn addr = 0x%x, gpAudioPlayBuf->pOut addr = 0x%x\n", gpAudioPlayBuf->BufSize, (unsigned int)(gpAudioPlayBuf->pIn), (unsigned int)(gpAudioPlayBuf->pOut));

//	msbf->size += write_len;
//	memcpy(om->b_wptr, write_buf, write_len);
//	om->b_wptr+=write_len;
//	putq(&msbf->q,om);

	ms_mutex_unlock(&ad->mutex);
	if (ad->tr->stopped()) {
		printf("AudioTrack stopped unexpectedly, needs to be restarted\n");
		ad->tr->start();
	}
}

int linux_snd_is_write_end(void)
{
	return gpAudioPlayBuf->BufSize;
}

void linux_snd_write_uninit(void)
{

	LinuxSndWriteData *ad = g_SndWriteData;
	LOGV("AndroidAudio linux_snd_write_uninit enter:g_SndWriteData=%x ad=%x", g_SndWriteData, ad);
	printf("AndroidAudio linux_snd_write_uninit enter:g_SndWriteData=%x ad=%x\n", g_SndWriteData, ad);
	if (!ad->tr)
	{
		return;
	}



	ad->tr->stop();
	ad->tr->flush();
	delete ad->tr;
	ad->tr=NULL;
//	ad->mCard->disableVoipMode();			//modify by xuqd
	ad->mStarted=false;

	if(ad != NULL)
	{
		LOGV("AndroidAudio linux_snd_write_uninit delete g_SndWriteData");
		printf("AndroidAudio linux_snd_write_uninit delete g_SndWriteData\n");
		delete ad;
		ad = NULL;
	}

	AudioPlayBufUninit();
	printf("AndroidAudio linux_snd_write_uninit end:g_SndWriteData=%x ad=%x\n", g_SndWriteData, ad);
	LOGV("AndroidAudio linux_snd_write_uninit end:g_SndWriteData=%x ad=%x", g_SndWriteData, ad);
}




int RaiseVolum(char *buf, unsigned int buf_size, unsigned uRepeat, double AmplifyRatio)
{
	int i = 0;
	int j = 0;

	if(!buf_size)
	{
		return -1;
	}

	for(i=0; i<buf_size;)
	{
		signed long minData = -0x8000;
		signed long maxData = 0x7FFF;
		signed short wData = buf[i+1];
		wData = MAKEWORD(buf[i], buf[i+1]);
		signed long dwData = wData;

		for(j=0; j<uRepeat; j++)
		{
			dwData=dwData * AmplifyRatio;
			if(dwData < -0x8000)
			{
				dwData = -0x4000;
			}
			else if(dwData > 0x7FFF)
			{
				dwData = 0x4000;
			}
		}
		wData = LOWORD(dwData);
		buf[i] = LOBYTE(wData);
		buf[i+1] = HIBYTE(wData);
		i+=2;
	}
}

extern "C" int SndCardDetect(void)
{
	return linux_snd_card_detect();
}

extern "C" int SndCardAudioRecordInit(void)
{
	return linux_snd_read_init();
}

extern "C" int AudioRecord(char *readBuf, int readLen)
{
	int readBufLen = 0;
	char AudioReadBuf[256]={0};

	//缓冲区剩余的数据还比较多，则不再读声卡数据
	//只有缓冲区数据已经比较少了，才读声卡数据
	if(gpAudioReadBuf == NULL)
	{
		LOGE("AndroidAudio gpAudioReadBuf:%x", gpAudioReadBuf);
	}

	if(gpAudioReadBuf->BufSize < readLen * 2)
	{
		readBufLen = linux_snd_read(AudioReadBuf);

		//RaiseVolum(AudioReadBuf, readBufLen, 1, 3);

		gpAudioReadBuf->BufSize += readBufLen;
		printf("gpAudioReadBuf->BufSize1 = %d\n", gpAudioReadBuf->BufSize);
		for(int i=0; i<readBufLen; i++)
		{
			*gpAudioReadBuf->pIn++ = AudioReadBuf[i];
			if((unsigned int)(gpAudioReadBuf->pIn) > gpAudioReadBuf->BufEndAddress)
			{
				gpAudioReadBuf->pIn = gpAudioReadBuf->Buf;
			}
		}
	}

	if(gpAudioReadBuf->BufSize < readLen)
	{
		return 0;
	}
	else
	{
		gpAudioReadBuf->BufSize -= readLen;
		printf("gpAudioReadBuf->BufSize2 = %d\n", gpAudioReadBuf->BufSize);
		for(int j=0; j<readLen; j++)
		{
			readBuf[j] = *gpAudioReadBuf->pOut++;
			if((unsigned int)(gpAudioReadBuf->pOut) > gpAudioReadBuf->BufEndAddress)
			{
				gpAudioReadBuf->pOut = gpAudioReadBuf->Buf;
			}
		}
	}

	return readLen;
}

extern "C" void SndCardAudioRecordUninit(void)
{
	return linux_snd_read_uninit();
}

extern "C" int SndCardAudioPlayInit(void)
{
	//return 0;
	return linux_snd_write_init();
}

extern "C" int AudioPlay(char *readBuf, int writeLen)
{
	return linux_snd_write(readBuf, writeLen);
}

extern "C" void SndCardAudioPlayUnnit(void)
{
	//return;
	return linux_snd_write_uninit();
}
