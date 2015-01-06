


#ifdef _YK_XT8126_BV_

#include "sndtools.h"
#include "common.h"
#include "LOGIntfLayer.h"
#include "YK8126Audio.h"
#include <errno.h>
//***********add by xuqd************
#include <XTIntfLayer.h>
int g_ossPlayFlag = 0;
//**********************************

extern pthread_mutex_t audio_open_lock;
extern pthread_mutex_t audio_close_lock;
extern pthread_mutex_t audio_lock;
//extern pthread_mutex_t audio_lock;
extern int fdNull;

static int YK8126BVAudioOpen(YK8126AudioData* d, int *minsz)
{
	int i = 0;
	int status;

	LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio open begin");
	pthread_mutex_lock(&audio_lock);

	LOG_RUNLOG_DEBUG("LP 8126 SndRecAudioOpen = %d, devrecfd = %d, SndPlayAudioOpen = %d, devplayfd = %d", SndRecAudioOpen, devrecfd, SndPlayAudioOpen, devplayfd);
	if (SndRecAudioOpen == 0)
	{
		if (devrecfd < 0)
		{
			LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio SndRecAudioOpen = 0 and devrecfd < 0 , open the audio and get devrecfd");
			devrecfd = open("/dev/dsp2", O_RDONLY, 0);
			if (devrecfd < 0)
			{
				devrecfd = -1;
				LOG_RUNLOG_ERROR("LP 8126 YK8126BVAudio SndRecAudioOpen = 0 but open devrecfd failed");
				pthread_mutex_unlock(&audio_lock);
				return FALSE;
			}
			else
			{
				d->pcmfd_read = devrecfd;
				SipRecAudioOpen = 1;

				devrecfd = configure_fd(devrecfd, d->bits, d->stereo, d->rate,minsz);
				LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio SndRecAudioOpen = 0 ,open devrecfd succeed");
			}
		}
		else
		{
			//LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio SndRecAudioOpen = 0 but devrecfd > 0, warning:devrecfd is invalid");
			close(devrecfd);
			devrecfd = -1;
			LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio SndRecAudioOpen = 0 but devrecfd > 0, devrecfd is invalid, so close devrecfd and set devrecfd = -1");
			pthread_mutex_unlock(&audio_lock);
			return FALSE;
		}
	}
	else
	{
		// SndRecAudioOpen = 1
		if (devrecfd < 0)
		{
			//LOG_RUNLOG_DEBUG(" YK8126BVAudio SndRecAudioOpen = 1,but devrecfd < 0,warning:devrecfd is invalid\n");
			SndRecAudioOpen = 0;
			LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio SndRecAudioOpen = 1,but devrecfd < 0,devrecfd is invalid, so set SndRecAudioOpen = 0");
			pthread_mutex_unlock(&audio_lock);
			return FALSE;
		}
		else
		{
			d->pcmfd_read = devrecfd;
			SipRecAudioOpen = 1;
			LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndRecAudioOpen = 1,and devrecfd > 0,so use the devrecfd which is opened by snd");
		}
	}

	if (SndPlayAudioOpen == 0)
	{
		if (devplayfd < 0)
		{
			LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndPlayAudioOpen = 0 and devplayfd < 0 , open the audio and get devplayfd");
			devplayfd = open("/dev/dsp2", O_WRONLY, 0);
			if (devplayfd < 0)
			{
				devplayfd = -1;
				perror("open");
				LOG_RUNLOG_ERROR("LP 8126 8126BVAudio SndPlayAudioOpen = 0 but open devplayfd failed");
				pthread_mutex_unlock(&audio_lock);
				return FALSE;
			}
			else
			{
				d->pcmfd_write = devplayfd;
				SipPlayAudioOpen = 1;
				devplayfd = configure_fd(devplayfd, d->bits, d->stereo, d->rate,minsz);
				LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndPlayAudioOpen = 0 ,open devplayfd succeed");
			}
		}
		else
		{
			LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndPlayAudioOpen = 0 but devplayfd > 0,devplayfd is invalid");
			close(devplayfd);
			devplayfd = -1;
			LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndPlayAudioOpen = 0 but devplayfd > 0, so close devplayfd and set devplayfd = -1");
//			if (devrecfd > 0)
//			{
//				close(devrecfd);
//				devrecfd = -1;
//			}
//			LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndPlayAudioOpen = 0 but devplayfd is invalid,so close devrecfd if it is exist");
			pthread_mutex_unlock(&audio_lock);
			return FALSE;
		}
	}
	else
	{
		// SndPlayAudioOpen = 1
		if (devplayfd < 0)
		{
			LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndPlayAudioOpen = 1,but devplayfd < 0,warning:devplayfd is invalid");
			SndPlayAudioOpen = 0;
			LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndPlayAudioOpen = 1, set SndPlayAudioOpen = 0");
			LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndPlayAudioOpen = 1,but devplayfd < 0,so set SndPlayAudioOpen = 0");
			pthread_mutex_unlock(&audio_lock);
			return FALSE;
		}
		else
		{
			d->pcmfd_write = devplayfd;
			SipPlayAudioOpen = 1;
			LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio SndPlayAudioOpen = 1,and devplayfd > 0,so use the devplayfd which is opened by snd");
		}
	}
	pthread_mutex_unlock(&audio_lock);
	LOG_RUNLOG_DEBUG("LP 8126 8126BVAudio Open finished");
	return TRUE;
}


static int iOssExeCount = 0;
void *YK8126BVAudioThread(void *p)
{
	MSSndCard *card = (MSSndCard*) p;
	YK8126AudioData *d = (YK8126AudioData*) card->data;
	int bsize = 0;
	uint8_t *rtmpbuff = NULL;
	uint8_t *wtmpbuff = NULL;
	int err;
	mblk_t *rm = NULL;
	bool_t did_read = FALSE;
	int i=0;
	int j=0;
	int k=0;

	if (YK8126BVAudioOpen(d, &bsize) == FALSE)
	{
		LOG_RUNLOG_WARN("LP 8126 YK8126BVAudioThread YK8126BVAudioOpen is failed");

		//声卡设备打开不成功，关闭本次通话 --added by lcx
		sleep(2); //等待兴联建立起通话后再挂断，防止一些标志位置错
		if (INVALID_SIP_CALL_ID != g_sipCallID)
		{
			LOG_RUNLOG_DEBUG("LP 8126 open snd dev failed, end this talking!");
			g_callIDUFlag = 0;
			g_softSipClearFlag = 1;
			TerminateLpCall(g_sipCallID);
		}
		TalkEnd_Func();

		//return NULL;
	}

	if (d->pcmfd_read >= 0)
	{
		rtmpbuff = (uint8_t*) alloca(bsize);
	}
	if (d->pcmfd_write >= 0)
	{
		wtmpbuff = (uint8_t*) alloca(bsize);
	}

	ms_bufferizer_flush(d->bufferizer);

	while (d->read_started || d->write_started)
	{
		////printf("i am here0!*************\n");

		did_read = FALSE;
		if (d->pcmfd_read > 0)
		{
			if (d->read_started)
			{
				if (rm == NULL)
					rm = allocb(bsize, 0);
				err = read(d->pcmfd_read, rm->b_wptr, bsize);
				if (err < 0)
				{
					ms_warning("Fail to read %i bytes from soundcard: %s", bsize, strerror(errno));
				}
				else
				{
					did_read = TRUE;
					rm->b_wptr += err;
					ms_mutex_lock(&d->mutex);
					putq(&d->rq, rm);
					ms_mutex_unlock(&d->mutex);
					rm = NULL;
				}
			}
			else
			{
				/* case where we have no reader filtern the read is performed for synchronisation */
				int sz = read(d->pcmfd_read, rtmpbuff, bsize);
				if (sz == -1)
					ms_warning("sound device read error %s ", strerror(errno));
				else
					did_read = TRUE;
			}
		}


		if( (++iOssExeCount)%5000  == 0 )
		{
			LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudioThread call_idu_flag:%d, g_ossPlayFlag:%d iOssExeCount:%d, Local.Status=%d", g_callIDUFlag, g_ossPlayFlag, iOssExeCount, Local.Status);
			iOssExeCount= 0;
		}
		////printf("i am here1!*************\n");


		if (d->pcmfd_write > 0)
		{
			//printf("d->write_started = %d,g_ossPlayFlag = %d*************\n",d->write_started,g_ossPlayFlag);
			if (d->write_started)
			{
				err = ms_bufferizer_read(d->bufferizer, wtmpbuff, bsize);
				//printf("d->write_started = %d,g_ossPlayFlag = %d****read pass****\n",d->write_started,g_ossPlayFlag);

				if (err == bsize)
				{
					if (g_callIDUFlag == 1)
					{
						LOG_RUNLOG_DEBUG("LP 8126 g_callIDUFlag = %d**bsize = %d**read pass",g_callIDUFlag,bsize);
						//err = write(fdNull, wtmpbuff, bsize);
					}
					else
					{
						if (g_ossPlayFlag == 1)
						{
							err = write(d->pcmfd_write, wtmpbuff, bsize);
							if (err < 0)
							{
								ms_warning("Fail to write %i bytes from soundcard: %s",bsize, strerror(errno));
							}
							else
							{
							}

							// 测试代码
//							if(i > 100)
//							{
//								printf("g_ossPlayFlag = %d**bsize = %d**read pass****\n",g_ossPlayFlag,bsize);
//								i = 0;
//							}
//							else
//							{
//								i ++;
//							}
						}
						else
						{

							// 测试代码
//							if(j > 100)
//							{
//								printf("g_ossPlayFlag = %d**bsize = %d**read pass****\n",g_ossPlayFlag,bsize);
//								j = 0;
//							}
//							else
//							{
//								j ++;
//							}

							// 既然不允许播放，可以进行一定的延时以让出CPU
//							sleep(1);

							err = write(fdNull, wtmpbuff, bsize);
						}
					}
				}
				else
				{
					// 既然此时没有数据可以读出，就进行一定的延时以让出CPU
						// 测试代码
//						if(k > 100)
//						{
//							printf("err = %d****read pass****\n",err);
//							k = 0;
//						}
//						else
//						{
//							k ++;
//						}


					sleep(1);
				}
			}
			else
			{
				int sz;
				LOG_RUNLOG_DEBUG("LP 8126 d->pcmfd_write=%d,bsize=%d,g_ossPlayFlag=%d",d->pcmfd_write,bsize,g_ossPlayFlag);
				if (g_ossPlayFlag == 1)
				{
					sz = write(d->pcmfd_write, wtmpbuff, bsize);
					if (sz != bsize)
						ms_warning("sound device write returned %i !", sz);
				}
				else
				{
					sz = write(fdNull, wtmpbuff, bsize);
				}
			}
		}
		if (!did_read)
		{
			////printf("i am here6!*************\n");
			usleep(40000); /*avoid 100%cpu loop for nothing*/
		}
		////printf("i am here7!*************\n");
	}

	LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudioThread close audio begin");
	//printf(" error = %d\n", errno);
	pthread_mutex_lock(&audio_lock);
	if (d->pcmfd_read == d->pcmfd_write && d->pcmfd_read >= 0)
	{
		if (close(d->pcmfd_read) != 0)
		{
			LOG_RUNLOG_WARN("LP 8126 YK8126BVAudio close pcmfd_read error 1");
		}
		else
		{
			LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio close pcmfd_read ok 1");
		}
		d->pcmfd_read = d->pcmfd_write = -1;
	}
	else
	{
		if (d->pcmfd_read == devrecfd)
		{
			if (d->pcmfd_read >= 0)
			{
				if (SndRecAudioOpen == 0)
				{
					if (close(d->pcmfd_read) != 0)
					{
						LOG_RUNLOG_WARN("LP 8126 YK8126BVAudio close pcmfd_read error 2");
					}
					else
					{
						LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio close pcmfd_read ok 2");
					}
					d->pcmfd_read = -1;
					devrecfd = -1;
					SipRecAudioOpen = 0;
					LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio SndRecAudioOpen = 0,so close(d->pcmfd_read) succeed!");
				}
				else
				{
					d->pcmfd_read = -1;
					SipRecAudioOpen = 0;
					LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio close(d->pcmfd_read)**but snd is using the audio,so don't close the devrecfd!\n");
				}
			}
			else
			{
				LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio ***warning:d->pcmfd_read = devrecfd,but d->pcmfd_read < 0,so can't close devrecfd");
			}
		}
		else
		{
			if (d->pcmfd_read >= 0)
			{
				if (close(d->pcmfd_read) != 0)
				{
					LOG_RUNLOG_WARN("LP 8126 YK8126BVAudio close pcmfd_read error 3");
				}
				else
				{
					LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio close pcmfd_read ok 3");
				}
				d->pcmfd_read = -1;
				SipRecAudioOpen = 0;
				LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio ***warning:d->pcmfd_read != devrecfd");
			}
			else
			{
				LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio ***warning:d->pcmfd_read != devrecfd,but d->pcmfd_read < 0,so can't close devrecfd!");
			}
		}

		if (d->pcmfd_write == devplayfd)
		{
			if (d->pcmfd_write >= 0)
			{
				if (SndPlayAudioOpen == 0)
				{
					//printf(" error = %d\n", errno);
					if (close(d->pcmfd_write) != 0)
					{
						LOG_RUNLOG_WARN("LP 8126 YK8126BVAudio close pcmfd_write error 4");
					}
					else
					{
						LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio close pcmfd_write ok 4");
						//printf(" error = %d\n", errno);


					}
					d->pcmfd_write = -1;
					devplayfd = -1;
					SipPlayAudioOpen = 0;
					LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio SndPlayAudioOpen = 0,so close(d->pcmfd_write) succeed!");
				}
				else
				{
					d->pcmfd_write = -1;
					SipPlayAudioOpen = 0;
					LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio close(d->pcmfd_write)**but snd is using the audio,so don't close the devplayfd!");
				}
			}
			else
			{
				LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio ***warning:d->pcmfd_write = devplayfd,but d->pcmfd_write < 0,so can't close devplayfd!");
			}
		}
		else
		{
			if (d->pcmfd_read >= 0)
			{
				if (close(d->pcmfd_write) != 0)
				{
					LOG_RUNLOG_WARN("LP 8126 YK8126BVAudio close pcmfd_write error 5");
				}
				else
				{
					LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio close pcmfd_write ok 5");
				}
				d->pcmfd_write = -1;
				SipPlayAudioOpen = 0;
				LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio ***warning:d->pcmfd_write != devplayfd");
			}
			else
			{
				LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudio ***warning:d->pcmfd_write == devplayfd,but d->pcmfd_write < 0,so can't close devplayfd!");
			}
		}
	}

	pthread_mutex_unlock(&audio_lock);
	LOG_RUNLOG_DEBUG("LP 8126 YK8126BVAudioThread close audio finished");
	return NULL;
}
#endif
