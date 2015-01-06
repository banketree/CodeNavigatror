#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>
#include <errno.h>

#define VAR_STATIC

#define CommonH
#include "common.h"
#include "g711common.h"
#include "sndtools.h"
#include <IDBTCfg.h>
#include <LANIntfLayer.h>
#include <LOGIntfLayer.h>
int devrecfd = -1;
int devplayfd = -1;
extern int g_ossPlayFlag;
extern g_iOutputLocalWavVol;



#ifdef _ECHO_CANCEL_AUDIO   //回声消除
int rec_total_num = 0;
#endif

int audio, abuf_size, zbuf_size;

int PlayPcmTotalNum; //一个接收音频包总的幅度值
extern int AudioMuteFlag; //静音标志
extern int SipAudioOpen;
extern pthread_mutex_t audio_open_lock;
extern pthread_mutex_t audio_close_lock;
extern pthread_mutex_t audio_lock;


extern  int SndCardDetect(void);
extern  int SndCardAudioRecordInit(void);
extern  int AudioRecord(char *readBuf, int len);
extern  int SndCardAudioRecordUninit(void);
extern  int SndCardAudioPlayInit(void);
extern  int AudioPlay(char *readBuf, int writeLen);
extern  int SndCardAudioPlayUnnit(void);


//音频
int curr_audio_timestamp;       //当前已播放音频帧时间戳
int audio_rec_flag;
int audio_play_flag;
pthread_t audio_rec_deal_thread; //音频采集数据处理线程
pthread_t audio_play_deal_thread; //音频播放数据处理线程
pthread_t audio_rec_thread; //音频采集线程
pthread_t audio_play_thread; //音频播放线程
void audio_rec_deal_thread_func(void);
void audio_play_deal_thread_func(void);
void audio_rec_thread_func(void);
void audio_play_thread_func(void);

struct audiobuf1 playbuf; //音频播放环形缓冲
struct audiobuf1 recbuf; //音频采集环形缓冲

sem_t audioplaysem;
sem_t audiorecsem;
sem_t audiorec2playsem;

void StartRecAudio(void);
void StopRecAudio(void);
void StartPlayAudio(void);
void StopPlayAudio(void);

void WaitAudioUnuse(int MaxTime); //等待声卡空闲;//等待声卡空闲

struct _SYNC sync_s;
struct timeval ref_time; //基准时间,音频或视频第一帧

extern int curr_audio_timestamp;

//为防止多次操作导致错误
int AudioRecIsStart = 0;
int AudioPlayIsStart = 0;

struct pcm *pcm_play;

void InitAudioParam(void); //初始化音频参数
// 等待回放结束
void SyncPlay(void);

//家庭留言
struct wavbuf1 wavbuf; //家庭留言环形缓冲
int audio_play_wav_flag;
pthread_t audio_play_wav_thread; //留言播放线程

void audio_play_wav_thread_func(int PlayFlag);
struct WaveFileHeader hWaveFileHeader;

//用于声音播放
void StartPlayWav(char *srcname, int PlayFlag); //0 单次播放  1 循环播放
//void StartPlayWavFile(void); //0 单次播放  1 循环播放
void StopPlayWavFile(void);

//目录及文件链表
// *h保存表头结点的指针，*p指向当前结点的前一个结点，*s指向当前结点
TempAudioNode1 * init_audionode(void); //初始化单链表的函数
//功能描述：求单链表长度
int length_audionode(TempAudioNode1 *h);
//功能描述：尾部添加
int creat_audionode(TempAudioNode1 *h, struct talkdata1 talkdata, unsigned char *r_buf, int r_length);

//功能描述：删除函数
int delete_audionode(TempAudioNode1 *p);
int delete_all_audionode(TempAudioNode1 *h);
int delete_lost_audionode(TempAudioNode1 *h, uint32_t currframeno, uint32_t currtimestamp); //删除不全帧
//功能描述：查找函数
TempAudioNode1 * find_audionode(TempAudioNode1 *h, int currframeno, int currpackage);
//查找最老的帧
TempAudioNode1 * search_audionode(TempAudioNode1 *h);
//
int free_audionode(TempAudioNode1 *h);
//---------------------------------------------------------------------------
void InitAudioParam(void) //初始化音频参数
{
	char acVolSet[128]={0x0};

	//set a IPGA volume, from -27 ~ +36 dB
	P_Debug("0000\n");
	system("echo 0 > /proc/asound/adda300_RIV"); //6
	P_Debug("1111\n"); //6
	system("echo 0 > /proc/asound/adda300_LIV");
	P_Debug("1112\n");
	//set a digital input volume, from -50 ~ +30 dB
	sprintf(acVolSet,"echo %d > /proc/asound/adda300_LADV",g_stIdbtCfg.iInputVol);
	system(acVolSet);
	//system("echo 7 > /proc/asound/adda300_LADV");
	P_Debug("1113\n");
	system("echo 7 > /proc/asound/adda300_RADV");
	P_Debug("1114\n");
	//set digital output volume, from 0 ~ -40 dB
	system("echo -30 > /proc/asound/adda300_LDAV");
	P_Debug("1115\n");
	system("echo -30 > /proc/asound/adda300_RDAV");
	//set speaker analog output gain, from -40 ~ +6 dB, this affect the final frontend output
	sprintf(acVolSet,"echo %d > /proc/asound/adda300_SPV",g_stIdbtCfg.iOutputCallVol);
	system(acVolSet);
	P_Debug("1116\n");
	system("echo 0 > /proc/asound/adda300_input_mode input_mode");
	P_Debug("1117\n");
	system("echo -6 > /proc/asound/adda300_ALCMIN");
	P_Debug("1118\n");
	system("echo 1 > /proc/asound/adda300_BTL_mode");
}

//---------------------------------------------------------------------------
/*
 * Open Sound device
 * Return 1 if success, else return 0.
 */
int OpenSnd(/* add by new version */int nWhich)
{
	int status; // ???????????
	int setting;

	P_Debug("***********snd_open**********\n");
	//pthread_mutex_lock(&audio_open_lock);
	pthread_mutex_lock(&audio_lock);

	P_Debug("*******SndRecAudioOpen = %d, devrecfd = %d, SndPlayAudioOpen = %d, devplayfd = %d****\n",
			SndRecAudioOpen, devrecfd, SndPlayAudioOpen, devplayfd);

	if (nWhich == 1)
	{
		if (SipRecAudioOpen == 0)
		{
			if (devrecfd < 0)
			{
				devrecfd = open("/dev/dsp2", O_RDONLY, 0); //open("/dev/dsp", O_RDWR);
				if (devrecfd < 0)
				{
					devrecfd = -1;
					P_Debug("**Snd**SipRecAudioOpen = 0,but open devrecfd failed******\n");
					pthread_mutex_unlock(&audio_lock);
					return 0;
				}
				else
				{
					SndRecAudioOpen = 1;

//					setting = 0x00040009;//0x00040009;
//					status = ioctl(devrecfd, SNDCTL_DSP_SETFRAGMENT, &setting);
//					if (status == -1)
//					{
//						close(devrecfd);
//						devrecfd = -1;
//						printf("**Snd**SipRecAudioOpen = 0,open devrecfd succeed,but set failed******\n");
//						pthread_mutex_unlock(&audio_lock);
//						return 0;
//					}
//
//					status = ioctl(devrecfd, SNDCTL_DSP_RESET, 0);
//					if (status == -1)
//					{
//						printf("*****************ioctl devrecfd  SNDCTL_DSP_RESET failed\n");
//					}
//					else
//					{
//						printf("*****************ioctl devrecfd  SNDCTL_DSP_RESET ok\n");
//					}
					P_Debug("**Snd**SipRecAudioOpen = 0,open devrecfd succeed******\n");
				}
			}
			else
			{
				// devrecfd > 0
				P_Debug("**Snd**SipRecAudioOpen = 0,but devrecfd > 0,warning:devrecfd is invalid******\n");
				close(devrecfd);
				devrecfd = -1;
				P_Debug("**Snd**SipRecAudioOpen = 0,but devrecfd > 0,so close devrecfd and set devrecfd = -1******\n");
				pthread_mutex_unlock(&audio_lock);
				return 0;
			}
		}
		else
		{
			// SipRecAudioOpen == 1
			if (devrecfd < 0)
			{
				P_Debug("**Snd**SipRecAudioOpen = 1,but devrecfd < 0,warning:devrecfd is invalid******\n");
				SipRecAudioOpen = 0;
				P_Debug("**Snd**SipRecAudioOpen = 1,but devrecfd < 0,so set SipRecAudioOpen = 0******\n");
				pthread_mutex_unlock(&audio_lock);
				return 0;
			}
			else
			{
				SndRecAudioOpen = 1;
				P_Debug("**Snd**SipRecAudioOpen = 1,and devrecfd > 0,so use the devrecfd which is opened by oss****\n");
			}
		}
	}
	else
	{
		//nWhich == 2
		if (SipPlayAudioOpen == 0)
		{
			if (devplayfd < 0)
			{
				devplayfd = open("/dev/dsp2", O_WRONLY, 0); //open ("/dev/dsp2", O_WRONLY);//, 0);//
				if (devplayfd < 0)
				{
					devplayfd = -1;
					P_Debug("**Snd**SipPlayAudioOpen = 0,but open devplayfd failed******\n");
					pthread_mutex_unlock(&audio_lock);
					return 0;
				}
				else
				{
					SndPlayAudioOpen = 1;

//					setting = 0x00040009;//0x0004000B;
//					status = ioctl(devplayfd, SNDCTL_DSP_SETFRAGMENT, &setting);
//					if (status == -1)
//					{
//						close(devplayfd);
//						devplayfd = -1;
//						printf("**Snd**SipPlayAudioOpen = 0,open devplayfd succeed,but set failed******\n");
//						pthread_mutex_unlock(&audio_lock);
//						return 0;
//					}
//
//					status = ioctl(devplayfd, SNDCTL_DSP_RESET, 0);
//					if (status == -1)
//					{
//						printf("*****************ioctl devplayfd  SNDCTL_DSP_RESET failed\n");
//					}
//					else
//					{
//						printf("*****************ioctl devplayfd  SNDCTL_DSP_RESET ok\n");
//					}

					P_Debug("**Snd**SipPlayAudioOpen = 0,open devplayfd succeed******\n");
				}
			}
			else
			{
				P_Debug("**Snd**SipPlayAudioOpen = 0,but devplayfd > 0,warning:open devplayfd failed******\n");
				close(devplayfd);
				devplayfd = -1;
				P_Debug("**Snd**SipPlayAudioOpen = 0,but devplayfd > 0,so close devplayfd and set devplayfd = -1******\n");
				pthread_mutex_unlock(&audio_lock);
				return 0;
			}
		}
		else
		{
			//SipPlayAudioOpen == 1
			if (devplayfd < 0)
			{
				P_Debug("**Snd**SipPlayAudioOpen = 1,but devplayfd < 0,warning:devplayfd is invalid******\n");
				SipPlayAudioOpen = 0;
				P_Debug("**Snd**SipPlayAudioOpen = 1,but devplayfd < 0,so set SipPlayAudioOpen = 0******\n");
				pthread_mutex_unlock(&audio_lock);
				return 0;
			}
			else
			{
				SndPlayAudioOpen = 1;
//				status = ioctl(devrecfd, SNDCTL_DSP_RESET, 0);
//				if (status == -1)
//				{
//					printf("*****************ioctl devrecfd  SNDCTL_DSP_RESET failed\n");
//				}
//				else
//				{
//					printf("*****************ioctl devrecfd  SNDCTL_DSP_RESET ok\n");
//				}
				P_Debug("**Snd**SipPlayAudioOpen = 1,and devplayfd > 0,so use the devplayfd which is opened by oss****\n");
				P_Debug("**Snd**SipPlayAudioOpen = 1, set SndPlayAudioOpen = 1\n");
			}
		}
	}

	//pthread_mutex_unlock(&audio_open_lock);
	pthread_mutex_unlock(&audio_lock);
	P_Debug("***********snd_open finished**********\n");
	return 1;
}
//---------------------------------------------------------------------------
/*
 * Close Sound device
 * return 1 if success, else return 0.
 */
int CloseSnd(/* add by new version */int nWhich)
{

	P_Debug("***********snd_close begin********\n");
	sprintf(Local.DebugInfo, " error = %d\n", errno);
	P_Debug(Local.DebugInfo);

	//pthread_mutex_lock(&audio_close_lock);
	pthread_mutex_lock(&audio_lock);
	if (nWhich == 1)
	{
		sprintf(Local.DebugInfo, "nWhich:1 CloseSnd::devrecfd = %d, devplayfd = %d\n", devrecfd, devplayfd);
		P_Debug(Local.DebugInfo);
		//close(devrecfd);
		//devrecfd = 0;
		if (SipRecAudioOpen == 0)
		{
			if (devrecfd > 0)
			{
				if(close(devrecfd) == 0)
				{
					P_Debug("**snd**SipRecAudioOpen = 0,close(devrecfd) succeed***********************************\n");
					devrecfd = -1;
					SndRecAudioOpen = 0;
				}
				else
				{
					sprintf(Local.DebugInfo, "error = %d, Error Message is %s\n", errno, strerror(errno));
					P_Debug(Local.DebugInfo);

				}
			}
			else
			{
				P_Debug("**snd**SipRecAudioOpen = 0,but devrecfd < 0,warning: devrecfd is invalid!***************************\n");
				pthread_mutex_unlock(&audio_lock);
				return 0;
			}
		}
		else
		{
			// SipRecAudioOpen == 1
			if (devrecfd < 0)
			{
				P_Debug("**snd**SipRecAudioOpen = 1,but devrecfd < 0,warning: can't close devrecfd*******\n");
				pthread_mutex_unlock(&audio_lock);
				return 0;
			}
			else
			{
				SndRecAudioOpen = 0;
				P_Debug("**snd**SipAudioOpen = 1 and devrecfd > 0, so the audio is occupied by oss,don't need to close the devrecfd!*****\n");
			}
		}
	}
	else
	{
		if (SipPlayAudioOpen == 0)
		{
			if (devplayfd > 0)
			{
				SyncPlay(); // 将缓冲区中的音频数据播放完
				if(close(devplayfd) == 0)
				{
					P_Debug("**snd**SipPlayAudioOpen = 0,close(devplayfd) succeed***********************************\n");
					devplayfd = -1;
					SndPlayAudioOpen = 0;
					P_Debug("**snd**SipPlayAudioOpen = 0,set SndPlayAudioOpen = 0***********************************\n");
				}
				else
				{
					printf(" error = %d, Error Message is %s\n", errno, strerror(errno));
					P_Debug(Local.DebugInfo);

				}
			}
			else
			{
				P_Debug("**snd**SipPlayAudioOpen = 0,but devplayfd < 0,warning: devplayfd is invalid!***************************\n");
				pthread_mutex_unlock(&audio_lock);
				return 0;
			}
		}
		else
		{
			// SipPlayAudioOpen == 1
			if (devplayfd < 0)
			{
				P_Debug("**snd**SipPlayAudioOpen = 1,but devplayfd < 0,warning: can't close devplayfd*******\n");
				pthread_mutex_unlock(&audio_lock);
				return 0;
			}
			else
			{
				SndPlayAudioOpen = 0;
				P_Debug("**snd**SipPlayAudioOpen = 1 and devplayfd > 0, so the audio is occupied by oss,don't need to close the devplayfd!*****\n");
				P_Debug("**snd**set SndPlayAudioOpen = 0\n");
			}
		}
	}

	//pthread_mutex_unlock(&audio_close_lock);
	pthread_mutex_unlock(&audio_lock);
	sprintf(Local.DebugInfo, " error = %d\n", errno);
	P_Debug(Local.DebugInfo);
	P_Debug("***********snd_close finished********\n");
	return 1;
}

//---------------------------------------------------------------------------
// 等待回放结束
void SyncPlay(void)
{
#if 1
	int status;
	if (devplayfd > 0)
	{
		status = ioctl(devplayfd, SOUND_PCM_SYNC, 0);
		if (status == -1)
			perror("SOUND_PCM_SYNC ioctl failed");
	}
#endif   
}
//---------------------------------------------------------------------------
/*
 * Set Record an Playback format
 * return 1 if success, else return 0.
 * bits -- FMT8BITS(8bits), FMT16BITS(16bits)
 * hz -- FMT8K(8000HZ), FMT16K(16000HZ), FMT22K(22000HZ), FMT44K(44000HZ)
 chn -- MONO 1 STERO 2
 */
int SetFormat(int nWhich, int bits, int hz, int chn)
{
	int samplesize;
	int tmp;
	int dsp_stereo;
	int setting;

	int arg; // 用于ioctl调用的参数
	int status; // 系统调用的返回值
	int devfd;
	if (nWhich == 1)
		devfd = devrecfd;
	else
		devfd = devplayfd;

	// 设置采样时的量化位数       FIC8120只支持16位
	sprintf(Local.DebugInfo, "bits = %d, hz = %d, chn = %d, devfd = %d\n", bits, hz, chn, devfd);
	P_Debug(Local.DebugInfo);
	if (devfd <= 0)
		return -1;
#if 0
	arg = bits;
	ioctl(devfd, SNDCTL_DSP_SAMPLESIZE, &arg);
#else
	status = ioctl(devfd, SOUND_PCM_READ_BITS, &arg);
	if (arg != bits)
	{
		arg = bits;
		status = ioctl(devfd, SOUND_PCM_WRITE_BITS, &arg);
		if (status == -1)
			P_Debug("SOUND_PCM_WRITE_BITS ioctl failed\n");
		if (arg != bits)
			P_Debug("unable to set sample size\n");
	}
#endif

	// 设置采样时的采样频率
#if 0
	arg = hz;
	status = ioctl(devfd, SNDCTL_DSP_SPEED, &arg);
#else
	status = ioctl(devfd, SOUND_PCM_READ_RATE, &arg);
	if (arg != hz)
	{
		arg = hz;
		status = ioctl(devfd, SOUND_PCM_WRITE_RATE, &arg);
		if (status == -1)
			perror("SOUND_PCM_WRITE_RATE ioctl failed");
	}
#endif

	// 设置采样时的声道数目
#if 0
	arg = chn;
	arg = 0;
	ioctl(devfd, SNDCTL_DSP_STEREO, &arg);
#else
	status = ioctl(devfd, SOUND_PCM_READ_CHANNELS, &arg);
	if (arg != chn)
	{
		arg = chn;
		status = ioctl(devfd, SOUND_PCM_WRITE_CHANNELS, &arg);
		if (status == -1)
			perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
		if (arg != chn)
			perror("unable to set number of channels");
	}
#endif

	abuf_size = AUDIOBLK;
	ioctl(devfd, SNDCTL_DSP_GETBLKSIZE, &abuf_size);
	if (DebugMode == 1)
	{
		sprintf(Local.DebugInfo, "abuf_size= %d\n", abuf_size);
		P_Debug(Local.DebugInfo);
	}
	if (abuf_size < 4 || abuf_size > 65536)
	{
		sprintf(Local.DebugInfo, "Invalid audio buffers size %d\n", nWhich);
		P_Debug(Local.DebugInfo);
		exit(-1);
	}

	return 1;
}

//---------------------------------------------------------------------------
/*
 * Record
 * return numbers of byte for read.
 */
int Record(char *buf, int size)
{
	int status;
	status = 0;
	if (devrecfd > 0)
	{
		//printf("Record 1, devrecfd = %d\n", devrecfd);
		//pthread_mutex_lock(&sync_s.audio_lock);
		status = read(devrecfd, buf, size);
		//printf("Record 2, status = %d\n", status);
		//pthread_mutex_unlock(&sync_s.audio_lock);
	}
	return status;
}
//---------------------------------------------------------------------------
/* 
 * Playback
 * return numbers of byte for write.
 */

int Play(char *buf, int size)
{
	int status;
	status = 0;

	if (devplayfd > 0)
	{
		//pthread_mutex_lock(&sync_s.audio_lock);
		//printf("Play 1\n");

		/*if(SipRecAudioOpen == 1)
		 {
		 status = write(fdNull, buf, size);
		 }
		 else
		 {
		 status = write(devplayfd, buf, size);
		 }*/

		status = write(devplayfd, buf, size);
		//printf("the status is %d\n", status);


		//printf("Play 2\n");
		//pthread_mutex_unlock(&sync_s.audio_lock);
	}
	return status;
}

void G711Encoder(short *pPcmBuf, char *pAudioBuf, int AudioLen, int type)
{
	int i;

	for(i=0; i<AudioLen; i++)
	{
		*pAudioBuf = s16_to_ulaw(*pPcmBuf);
		pPcmBuf ++;
		pAudioBuf ++;
	}
	return;
}


//---------------------------------------------------------------------------
void audio_rec_deal_thread_func(void)
{
	short *rec_pcm;
	short recppcm[AUDIOBLK];
	char *input_buf;
	char e_buf[AUDIOBLK * 2 + 1];char
	*ref_buf;
	struct timeval tv, tv1;
	int dwSize;
	int i;
	unsigned char adpcm_out[3072];
	char RemoteHost[20];
	int RemotePort;
	int FrameNum;
#ifdef _ECHO_CAL_TIME       //统计时间
	int echo_cancel_num = 0;
	int echo_cancel_time = 0;
#endif
	//通话数据结构
	struct talkdata1 talkdata;

#ifdef _RECORD_PCM_FILE
	FILE *echo_fd;
	if ((echo_fd = fopen("/data/data/com.fsti.ladder/echo", "wb")) == NULL)
		P_Debug("don't create echo pcm file \n");
#endif

	if (DebugMode == 1)
		P_Debug("don't create audio recv deal thread \n");
	while (audio_rec_flag == 1)
	{
		//等待采集线程有数据的信号
		sem_wait(&audiorecsem);
		//加锁
		pthread_mutex_lock(&sync_s.audio_rec_lock);
		FrameNum = recbuf.n / AUDIOBLK;

		// if(FrameNum > 3)
		//   FrameNum -= 3;
		for (i = 0; i < FrameNum; i++)
		{
			//头部
			memcpy(adpcm_out, UdpPackageHead, 6);
			//命令
			if (Remote.isDirect == 0) //直通呼叫
				adpcm_out[6] = VIDEOTALK;
			else
				//中转呼叫
				adpcm_out[6] = VIDEOTALKTRANS;
			adpcm_out[7] = 1;
			//子命令
			if ((Local.Status == 1) || (Local.Status == 3) || (Local.Status == 5)
					|| (Local.Status == 7) || (Local.Status == 9)) //本机为主叫方
			{
				adpcm_out[8] = CALLUP;
				memcpy(talkdata.HostAddr, LocalCfg.Addr, 20);
				memcpy(talkdata.HostIP, LocalCfg.IP, 4);
				memcpy(talkdata.AssiAddr, Remote.Addr[0], 20);
				if (Remote.DenNum == 1)
					memcpy(talkdata.AssiIP, Remote.IP[0], 4);
				else
					memcpy(talkdata.AssiIP, Remote.GroupIP, 4);
			}
			if ((Local.Status == 2) || (Local.Status == 4) || (Local.Status == 6)
					|| (Local.Status == 8) || (Local.Status == 10)) //本机为被叫方
			{
				adpcm_out[8] = CALLDOWN;
				memcpy(talkdata.HostAddr, Remote.Addr[0], 20);
				if (Remote.DenNum == 1)
					memcpy(talkdata.HostIP, Remote.IP[0], 4);
				else
					memcpy(talkdata.HostIP, Remote.GroupIP, 4);
				memcpy(talkdata.AssiAddr, LocalCfg.Addr, 20);
				memcpy(talkdata.AssiIP, LocalCfg.IP, 4);
			}
			//时间戳
			talkdata.timestamp = recbuf.timestamp[recbuf.iget / AUDIOBLK];

			//数据类型
			talkdata.DataType = 1;

			//帧序号
			talkdata.Frameno = recbuf.frameno[recbuf.iget / AUDIOBLK];

			//帧数据长度
			talkdata.Framelen = AUDIOBLK / 2;

			//总包数
			talkdata.TotalPackage = 1;

			//当前包
			talkdata.CurrPackage = 1;

			//数据长度
			talkdata.Datalen = AUDIOBLK / 2;

			talkdata.PackLen = PACKDATALEN;

			memcpy(adpcm_out + 9, &talkdata, sizeof(talkdata));

#ifdef _ECHO_CANCEL_AUDIO   //回声消除
			/*ref_buf = (char *) (playbuf.buffer + playbuf.i_echo);
			if ((playbuf.i_echo + AUDIOBLK) >= NMAX)
				playbuf.i_echo = 0;
			else
				playbuf.i_echo += AUDIOBLK;

			//printf("%d, %d\n", playbuf.echo_timestamp[echo_pos/AUDIOBLK], recbuf.timestamp[recbuf.iput/AUDIOBLK]);
			input_buf = (char *) (recbuf.buffer + recbuf.iget);

			Echo_Audio_Deal(ref_buf, input_buf, e_buf);

			memcpy((char *) (recbuf.buffer + recbuf.iget), (char *)e_buf, AUDIOBLK);*/

#ifdef _SOFTWARE_REC_MAGNIFY       //音频录制软件放大
			rec_pcm = (short *) (recbuf.buffer + recbuf.iget);

			for (i = 0; i < AUDIOBLK / 2; i++)
				rec_pcm[i] = rec_pcm[i] << _SOFTWARE_REC_MULTIPLE;
#endif

			rec_total_num++;
#endif

			G711Encoder((short *) (recbuf.buffer + recbuf.iget), adpcm_out + DeltaLen, AUDIOBLK/2, 1);

#ifdef _RECORD_PCM_FILE
//			fwrite((char *)(recbuf.buffer + recbuf.iget), AUDIOBLK, 1, echo_fd);
			fwrite((char *)(adpcm_out + DeltaLen), AUDIOBLK/2, 1, echo_fd);
#endif

			if((recbuf.iget + AUDIOBLK) >= NMAX)
				recbuf.iget = 0;
			else
				recbuf.iget += AUDIOBLK;
			recbuf.n -= AUDIOBLK;
			//UDP发送
			sprintf(RemoteHost, "%d.%d.%d.%d\0", Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
			//printf("%d.%d.%d.%d\n",Remote.DenIP[0], Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
			if (Remote.isDirect == 0) //直通呼叫
				RemotePort = RemoteVideoPort;
			else//中转呼叫
				RemotePort = RemoteVideoServerPort;
#ifdef _ECHO_CANCEL_AUDIO   //回声消除
			if (rec_total_num > 50)
#endif
				UdpSendBuff(m_VideoSocket, RemoteHost, RemotePort, adpcm_out, DeltaLen + AUDIOBLK / 2);
		}
		//解锁
		pthread_mutex_unlock(&sync_s.audio_rec_lock);
		//发送消息至播放数据处理线程,测试用
		//  sem_post(&audioplaysem);
	}
#ifdef _RECORD_PCM_FILE
	fclose(echo_fd);
#endif
}

//#define _RECORD_PCM_FILE

//---------------------------------------------------------------------------
//int iAudioCount = 0;
//int iSize = 0;
int recEndFlg = 0;
void audio_rec_thread_func(void)
{
//	iSize = 0;
	recEndFlg = 0;
	struct timeval tv, tv1;
	uint32_t nowtime;
	int dwSize;

	int i;
	int RecPcmTotalNum;
	short recppcm[AUDIOBLK / 2];

#ifdef _RECORD_PCM_FILE
	FILE *pcm_fd;
	if((pcm_fd=fopen("/data/data/com.fsti.ladder/record", "wb")) == NULL)
		P_Debug("don't record pcm file \n");
#endif

#ifdef _READ_PCM_FILE
	FILE *readpcm_fd;
	if((readpcm_fd = fopen("/data/data/com.fsti.ladder/cartoon.pcm", "r")) == NULL)
		P_Debug("don't open read pcm file \n");
#endif

	short 	nullrecppcm[AUDIOBLK / 2];
	for(i = 0; i < AUDIOBLK/2; i++)
		nullrecppcm[i] = 0;

	if (DebugMode == 1)
	{
		P_Debug("create audio recv thread \n");
		sprintf(Local.DebugInfo, "********************audio_rec_flag=%d\n", audio_rec_flag);
		P_Debug(Local.DebugInfo);
	}
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	while (audio_rec_flag == 1)
	{
		recbuf.frameno[recbuf.iput / AUDIOBLK] = Local.nowaudioframeno;
		Local.nowaudioframeno++;
		if (Local.nowaudioframeno >= 65536)
			Local.nowaudioframeno = 0;
		//时间戳
		gettimeofday(&tv, NULL);
		//第一帧,设定起始时间戳
		if ((ref_time.tv_sec == 0) && (ref_time.tv_usec == 0))
		{
			ref_time.tv_sec = tv.tv_sec;
			ref_time.tv_usec = tv.tv_usec;
		}
		nowtime = (tv.tv_sec - ref_time.tv_sec) * 1000 + (tv.tv_usec - ref_time.tv_usec) / 1000;
		recbuf.timestamp[recbuf.iput / AUDIOBLK] = nowtime;

		//   printf("tv.tv_sec=%d, tv.tv_usec=%d\n", tv.tv_sec, tv.tv_usec);
#ifdef _READ_PCM_FILE
		dwSize = fread(recbuf.buffer + recbuf.iput, sizeof(char), AUDIOBLK, readpcm_fd);
#else
//		dwSize = record_pcm((char *) (recbuf.buffer + recbuf.iput), AUDIOBLK, 0.4);
//		dwSize = Record((char *) (recbuf.buffer + recbuf.iput), AUDIOBLK);
		dwSize = AudioRecord((char *) (recbuf.buffer + recbuf.iput), AUDIOBLK);
#endif

//				if(dwSize == AUDIOBLK)
//				{
////					P_Debug("dwSize == 160\n");
//					memcpy((char *) (recbuf.buffer + recbuf.iput), tmpBuf, AUDIOBLK);
//				}
//				else
//				{
//					continue;
//				}

		//printf("after record\n");
		if (dwSize != AUDIOBLK)
		{
//			sprintf(Local.DebugInfo, "AUDIOBLK != dwSize  %d, %d\n", AUDIOBLK, dwSize);
//			P_Debug(Local.DebugInfo);
			continue;
		}
		else
		{
//			if(++iAudioCount  % 10 == 0)
//			{
//				iAudioCount = 0;
//				iSize += 1;
//				printf("******************AUDIOBLK == dwSize  %d, %d, audio_rec_flag=(%d), size=%d\n", AUDIOBLK, dwSize, audio_rec_flag, iSize);
//			}
		}

#ifdef _RECORD_PCM_FILE
		fwrite(recbuf.buffer + recbuf.iput, AUDIOBLK, 1, pcm_fd);
#endif

		//加锁
		//printf("************************audio_rec_lock try,  audio_rec_flag=(%d)\n", audio_rec_flag);
		pthread_mutex_lock(&sync_s.audio_rec_lock);
		//printf("************************audio_rec_lock ok,  audio_rec_flag=(%d)\n", audio_rec_flag);

		if ((recbuf.iput + AUDIOBLK) >= NMAX)
			recbuf.iput = 0;
		else
			recbuf.iput += AUDIOBLK;
		if (recbuf.n < NMAX)
			recbuf.n += AUDIOBLK;
		else
			P_Debug("rec.Buffer is full\n");

		//解锁
		//printf("************************audio_rec_lock unlock  try,  audio_rec_flag=(%d)\n", audio_rec_flag);
		pthread_mutex_unlock(&sync_s.audio_rec_lock);
		//printf("************************audio_rec_lock unlock  ok,  audio_rec_flag=(%d)\n", audio_rec_flag);
		sem_post(&audiorecsem);
		//printf("************************ sem_post  ok,  audio_rec_flag=(%d)\n", audio_rec_flag);
	}
#ifdef _RECORD_PCM_FILE
	fclose(pcm_fd);
#endif
#ifdef _READ_PCM_FILE
	fclose(readpcm_fd);
#endif
	recEndFlg = 1;
	LOG_RUNLOG_DEBUG("AndroidAudio recEndFlg == 1 rec function end");
}

void G711Decoder(short *pPcmBuf, char *pAudioBuf, int AudioLen, int type)
{
	int i;

	for(i=0; i<AudioLen; i++)
	{
		*pPcmBuf = ulaw_to_s16(*pAudioBuf);
		pPcmBuf ++;
		pAudioBuf ++;
	}

	return;
}

//---------------------------------------------------------------------------
void audio_play_deal_thread_func(void)
{
	short playppcm[AUDIOBLK * 2];int i,j;
	TempAudioNode1 * tmp_audionode;
	uint32_t dellostframeno;
	uint32_t dellosttimestamp;
#ifdef _RECORD_PCM_FILE
	FILE *pcm_fd;
	if((pcm_fd=fopen("/data/data/com.fsti.ladder/play711", "wb")) == NULL)
		P_Debug("don't create play pcm file \n");
#endif
	if(DebugMode == 1)
		P_Debug("create audio play deal thread \n" );

	while(audio_play_flag == 1)
	{
		//等待采集线程有数据的信号, 测试用
		//等待UDP接收线程有数据的信号
		sem_wait(&audiorec2playsem);
		if(audio_play_flag == 0)
			break;
		//加锁
		pthread_mutex_lock(&sync_s.audio_play_lock);
//    while(temp_audio_n > 0)
		if(temp_audio_n > 0)
		{
			//解码
			//查找最老的帧
			tmp_audionode = search_audionode(TempAudioNode_h);
//			sprintf(Local.DebugInfo, "Content.frameno = %d, Content.timestamp = %d \n", tmp_audionode->Content.frameno, tmp_audionode->Content.timestamp);
//			P_Debug(Local.DebugInfo);
			if(tmp_audionode != NULL)
			{
				playbuf.frameno[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.frameno;
				playbuf.timestamp[playbuf.iput/AUDIOBLK] = tmp_audionode->Content.timestamp;
				//memcpy(playbuf.buffer + playbuf.iput, tmp_audionode->Content.buffer, AUDIOBLK);
				G711Decoder((short *)(playbuf.buffer + playbuf.iput), tmp_audionode->Content.buffer, AUDIOBLK/2, 1);
#ifdef _RECORD_PCM_FILE
//				fwrite(tmp_audionode->Content.buffer, AUDIOBLK/2, 1, pcm_fd);
				fwrite((char *)(playbuf.buffer + playbuf.iput), AUDIOBLK, 1, pcm_fd);
#endif
//				sprintf(Local.DebugInfo, "playbuf.iput = %d, playbuf.n = %d \n", playbuf.iput, playbuf.n);
//				P_Debug(Local.DebugInfo);

				if((playbuf.iput + AUDIOBLK) >= NMAX)
					playbuf.iput = 0;
				else
					playbuf.iput += AUDIOBLK;
				if(playbuf.n < NMAX)
					playbuf.n += AUDIOBLK;
				else
					P_Debug("play1.Buffer is full 1\n");

				sem_post(&audioplaysem);

				if(temp_audio_n > 0)
					temp_audio_n --;
				dellostframeno = tmp_audionode->Content.frameno;
				dellosttimestamp = tmp_audionode->Content.timestamp;
				delete_audionode(tmp_audionode);
			}
			//删除不全帧
			delete_lost_audionode(TempAudioNode_h, dellostframeno, dellosttimestamp);
		}
		//解锁
		pthread_mutex_unlock(&sync_s.audio_play_lock);
	}
#ifdef _RECORD_PCM_FILE
	fclose(pcm_fd);
#endif
}
//---------------------------------------------------------------------------
int playEndFlg = 0;
void audio_play_thread_func(void)
{
	playEndFlg = 0;
	short *play_pcmbuf;
	int dwSize;
	int i;
	int jump_buf; //已解码缓冲区跳帧数
	int jump_tmp; //接收缓冲区跳帧数
	int jump_frame;
	int aframe;
	struct timeval tv;
	TempAudioNode1 * tmp_audionode;

#ifdef _RECORD_PCM_FILE
	FILE *pcm_fd;
	if((pcm_fd=fopen("/data/data/com.fsti.ladder/play", "wb")) == NULL)
		P_Debug("don't create play pcm file \n");
#endif
	if (DebugMode == 1)
	{
		P_Debug("create audio play thread \n");
		sprintf(Local.DebugInfo, "audio_play_flag=%d\n", audio_play_flag);
		P_Debug(Local.DebugInfo);
	}
	aframe = AFRAMETIME;
	while (audio_play_flag == 1)
	{
		sem_wait(&audioplaysem);
		//printf("audio_play_thread_func \n");
		if (audio_play_flag == 0)
			break;
		//  while(playbuf.n > 0)
		if (playbuf.n > 0)
		{
//         printf("playbuf.timestamp[playbuf.iget/AUDIOBLK] =%d\n",playbuf.timestamp[playbuf.iget/AUDIOBLK]);
			curr_audio_timestamp = playbuf.timestamp[playbuf.iget / AUDIOBLK];

#ifdef _SOFTWARE_PLAY_MAGNIFY       //音频播放软件放大
			play_pcmbuf = (short *)(playbuf.buffer + playbuf.iget);

			for(i=0; i<AUDIOBLK/2; i++)
			play_pcmbuf[i] = play_pcmbuf[i] << _SOFTWARE_MULTIPLE;
#endif

//			sprintf(Local.DebugInfo, "audio_play_thread_func: pcm_play = %d\n", pcm_play);
//								P_Debug(Local.DebugInfo);
			//dwSize = Play((char *) (playbuf.buffer + playbuf.iget), AUDIOBLK);

			AudioPlay((char *) (playbuf.buffer + playbuf.iget), AUDIOBLK);

//			dwSize = play_pcm(pcm_play, (char *) (playbuf.buffer + playbuf.iget), AUDIOBLK);		//modify by xuqd
#ifdef _RECORD_PCM_FILE
			fwrite((char *)(playbuf.buffer + playbuf.iget), AUDIOBLK, 1, pcm_fd);
#endif
			//printf("play thread dwSize = %d\n", dwSize);
//			sprintf(Local.DebugInfo, "audio_play_thread_func: dwSize = %d\n", dwSize);
//					P_Debug(Local.DebugInfo);
//			if (dwSize != AUDIOBLK)
//			{
//				sprintf(Local.DebugInfo, "dwSize = %d\n", dwSize);
//				P_Debug(Local.DebugInfo);
//				continue;
//			}

			//加锁
			pthread_mutex_lock(&sync_s.audio_play_lock);

			dwSize = AUDIOBLK;
			if ((playbuf.iget + dwSize) >= NMAX)
				playbuf.iget = 0;
			else
				playbuf.iget += dwSize;
			if (playbuf.n >= dwSize)
				playbuf.n -= dwSize;
			else
				P_Debug("play2.Buffer is full\n");

			if ((TimeStamp.OldCurrAudio != TimeStamp.CurrAudio) //上一次当前视频时间
					&& (TimeStamp.OldCurrAudio != 0) && (TimeStamp.CurrAudio != 0))
			{
				if ((TimeStamp.CurrAudio - curr_audio_timestamp) > 32 * 4)
				{
					jump_frame = (TimeStamp.CurrAudio - curr_audio_timestamp - 40) / aframe;
					if ((playbuf.n / AUDIOBLK) >= jump_frame)
					{
						jump_buf = jump_frame;
						jump_tmp = 0;
					}
					else
					{
						temp_audio_n = length_audionode(TempAudioNode_h);
						jump_buf = (playbuf.n / AUDIOBLK);
						if (temp_audio_n > (jump_frame - (playbuf.n / AUDIOBLK)))
							jump_tmp = jump_frame - (playbuf.n / AUDIOBLK);
						else
							jump_tmp = temp_audio_n;
					}

//					sprintf(Local.DebugInfo, "audio jump_buf =%d , jump_tmp = %d, jump_frame = %d\n",
//							jump_buf, jump_tmp, jump_frame);
//					P_Debug(Local.DebugInfo);

					for (i = 0; i < jump_buf; i++)
					{
						if ((playbuf.iget + AUDIOBLK) >= NMAX)
							playbuf.iget = 0;
						else
							playbuf.iget += AUDIOBLK;
						if (playbuf.n >= AUDIOBLK)
							playbuf.n -= AUDIOBLK;
					}
					for (i = 0; i < jump_tmp; i++)
					{
						//查找最老的帧
						tmp_audionode = search_audionode(TempAudioNode_h);
						if ((tmp_audionode != NULL) && (temp_audio_n > 0))
						{
							delete_audionode(tmp_audionode);
							temp_audio_n--;
						}
					}
				}
			}
			//解锁
			pthread_mutex_unlock(&sync_s.audio_play_lock);
		}

	}
#ifdef _RECORD_PCM_FILE
	fclose(pcm_fd);
#endif

	playEndFlg = 1;

}
//---------------------------------------------------------------------------
static int iRecInitFlg = 0;
void StartRecAudio(void)
{
	int i;
	pthread_attr_t attr;
	if (AudioRecIsStart == 0)
	{
		AudioRecIsStart = 1;

#ifdef _ECHO_CANCEL_AUDIO   //回声消除
		//Init_Echo_Audio(FMT8K, AUDIOBLK);
		rec_total_num = 0;
#endif

		PlayPcmTotalNum = 0; //一个接收音频包总的幅度值

#if 1
		//音频
		//if (!OpenSnd(AUDIODSP))
//		if(record_open(FMT8K, 1) == -1)        //modify by xuqd
//		{
//			P_Debug("******StartRecAudio()******Open record sound device error!\\n");
//			return;
//		}
#endif
		//SetFormat(AUDIODSP, FMT16BITS, FMT8K, 1/*STERO, WAVOUTDEV*/); //录音 del by zlin

//		if(iRecInitFlg == 0)
//		{
//			iRecInitFlg++;
			if(SndCardAudioRecordInit() == -1)
			{
				P_Debug("snd card init failed.");
			}
//		}


		recbuf.iput = 0;
		recbuf.iget = 0;
		recbuf.n = 0;
		for (i = 0; i < NMAX / AUDIOBLK; i++)
		{
			recbuf.frameno[i] = 0;
			recbuf.timestamp[i] = 0;
		}

		sem_init(&audiorecsem, 0, 0);

		audio_rec_flag = 1;
		pthread_mutex_init(&sync_s.audio_rec_lock, NULL);

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_rec_deal_thread, &attr, (void *) audio_rec_deal_thread_func, NULL);
		if (audio_rec_deal_thread == 0)
		{
			P_Debug("don't create audio recv deal thread \n");
			pthread_attr_destroy(&attr);
			return;
		}

		pthread_create(&audio_rec_thread, &attr, (void *) audio_rec_thread_func, NULL);
		pthread_attr_destroy(&attr);
		if (audio_rec_thread == 0)
		{
			P_Debug("don't create audio recv thread \n");
			return;
		}
		P_Debug("StartRecAudio finished\n");
	}
	else
	{
		if (DebugMode == 1)
			P_Debug("repeat AudioRecIsStart\n");
	}
}
//---------------------------------------------------------------------------
void StopRecAudio(void)
{
	int delaytime;
	delaytime = 40;
//	sprintf(Local.DebugInfo,"AudioRecIsStart=%d\n", AudioRecIsStart);
//	P_Debug(Local.DebugInfo);

	if (AudioRecIsStart == 1)
	{
		P_Debug("cccccccccc");
		ref_time.tv_sec = 0; //初始时间戳
		ref_time.tv_usec = 0;

		AudioRecIsStart = 0;
		audio_rec_flag = 0;
		usleep(delaytime * 1000);
		/*if (pthread_cancel(audio_rec_thread) == 0)
			printf("audio_rec_thread cancel success\n");
		else
			printf("audio_rec_thread cancel fail\n");
		usleep(delaytime * 1000);
		if (pthread_cancel(audio_rec_deal_thread) == 0)
			printf("audio_rec_deal_thread cancel success\n");
		else
			printf("audio_rec_deal_thread cancel fail\n");
		usleep(delaytime * 1000);*/
#if 1
//		record_close();			//modify by xuqd
		//CloseSnd(AUDIODSP);
#endif    
		//静音MIC
//    ioctl(gpio_fd, IO_PUT, 4);
//		while(!recEndFlg)
//		{
//			LOG_RUNLOG_DEBUG("AndroidAudio recEndFlg != 1 will usleep 1000");
//			usleep(1000);
//		}
		LOG_RUNLOG_DEBUG("AndroidAudio will SndCardAudioRecordUninit");
		//sleep(5);
		SndCardAudioRecordUninit();

		sem_destroy(&audiorecsem);
		pthread_mutex_destroy(&sync_s.audio_rec_lock);

#ifdef _ECHO_CANCEL_AUDIO   //回声消除
		//UnInit_Echo_Audio();
#endif
	}
	else
	{
		if (DebugMode == 1)
			P_Debug("repeat AudioRecIsStart\n");
	}
}
//---------------------------------------------------------------------------
static int iPlayInitFlg = 0;
void StartPlayAudio(void)
{
	pthread_attr_t attr;
	int i;
	P_Debug("StartPlayAudio \n");
	if (AudioPlayIsStart == 0)
	{
		TimeStamp.OldCurrVideo = 0; //上一次当前视频时间
		TimeStamp.CurrVideo = 0;
		TimeStamp.OldCurrAudio = 0; //上一次当前音频时间
		TimeStamp.CurrAudio = 0;

		AudioPlayIsStart = 1;
		PlayPcmTotalNum = 0; //一个接收音频包总的幅度值
		//音频

		//if (!OpenSnd(AUDIODSP1))

//		pcm_play = play_open(FMT8K, 1);		//modify xuqd
//		sprintf(Local.DebugInfo, "StartPlayAudio: pcm_play = %d\n", pcm_play);
//										P_Debug(Local.DebugInfo);
//		if(pcm_play == NULL)
//		{
//			P_Debug("******StartPlayAudio()****Open play sound device error!\n");
//			return;
//		}

//		if(iPlayInitFlg == 0)
//		{
//			iPlayInitFlg++;
			if(SndCardAudioPlayInit() == -1)
			{
				P_Debug("SndCardAudioPlayInit failed!");
			}
			else
			{
				P_Debug("SndCardAudioPlayInit succeed!");
			}
//		}



		//SetFormat(AUDIODSP1, FMT16BITS, FMT8K, 1); //放音 STERO, WAVOUTDEV del by zlin

		playbuf.iput = 0;
		playbuf.iget = 0;
		playbuf.i_echo = 0;
		playbuf.n = 0;
		for (i = 0; i < NMAX / AUDIOBLK; i++)
		{
			playbuf.frameno[i] = 0;
			playbuf.timestamp[i] = 0;
		}

		temp_audio_n = 0;
		//音频接收缓冲链表
		delete_all_audionode(TempAudioNode_h);

		sem_init(&audioplaysem, 0, 0);
		sem_init(&audiorec2playsem, 0, 0);

		audio_play_flag = 1;
		pthread_mutex_init(&sync_s.audio_play_lock, NULL);
		pthread_mutex_init(&sync_s.audio_lock, NULL);

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_play_deal_thread, &attr, (void *) audio_play_deal_thread_func, NULL);
		if (audio_play_deal_thread == 0)
		{
			P_Debug("don't create audio play deal thread \n");
			pthread_attr_destroy(&attr);
			return;
		}
		pthread_create(&audio_play_thread, &attr, (void *) audio_play_thread_func, NULL);
		pthread_attr_destroy(&attr);
		if (audio_play_thread == 0)
		{
			P_Debug("don't create audio play thread \n");
			return;
		}
	}
	else
	{
		if (DebugMode == 1)
			P_Debug("repeat AudioPlayStart \n");
	}
}
//---------------------------------------------------------------------------
void StopPlayAudio(void)
{
	int delaytime;
	delaytime = 40;
	sprintf(Local.DebugInfo, "AudioPlayStart=%d\n", AudioPlayIsStart);
	P_Debug(Local.DebugInfo);

	if (AudioPlayIsStart == 1)
	{
		AudioPlayIsStart = 0;
		audio_play_flag = 0;
//		sem_post(&audiorec2playsem);
//		usleep(delaytime * 1000);
		/*if (pthread_cancel(audio_play_deal_thread) == 0)//del by zlin
			printf("audio_play_deal_thread cancel success\n");
		else
			printf("audio_play_deal_thread cancel fail\n");*/

		sem_post(&audioplaysem);
		usleep(delaytime * 1000);

//		while(!playEndFlg)
//		{
//			usleep(1000);
//		}
		SndCardAudioPlayUnnit();

		/*if (pthread_cancel(audio_play_thread) == 0)
			printf("audio_play_thread cancel success\n");
		else
			printf("audio_play_thread cancel fail\n");
		usleep(delaytime * 1000);*/
		// 等待回放结束
		//SyncPlay();
//		play_close();				//modify by xuqd
		//CloseSnd(AUDIODSP1); del by zlin
		sem_destroy(&audioplaysem);
		sem_destroy(&audiorec2playsem);
		pthread_mutex_destroy(&sync_s.audio_play_lock);
		pthread_mutex_destroy(&sync_s.audio_lock);
		temp_audio_n = 0;
		//音频接收缓冲链表
		delete_all_audionode(TempAudioNode_h);
	}
	else
	{

		if (DebugMode == 1)
			P_Debug("repeat AudioPlayStop\n");
	}
}
//---------------------------------------------------------------------------
void StartPlayWav(char *srcname, int PlayFlag) //0 单次播放  1 循环播放
{
	char acVolSet[128]={0x0};
	
    sprintf(acVolSet,"echo %d > /proc/asound/adda300_SPV",g_stIdbtCfg.iOutputPlayWavVol); //设置开锁声音大小
    system(acVolSet);
    sprintf(Local.DebugInfo, "XT debug LOCK_AUDIO_OUT_VOL is :%s", acVolSet);
	P_Debug(Local.DebugInfo);

    
#if 1
	pthread_attr_t attr;
	int i;
	FILE* fd;
	int bytes_read;
	//查看音频设备是否空闲
	sprintf(Local.DebugInfo, "AudioPlayIsStart:%d\n", AudioPlayIsStart);
	P_Debug(Local.DebugInfo);
	// 测试代码 add by xuqd
	//AudioPlayIsStart = 1;

	if (AudioPlayIsStart == 0)
	{
		AudioPlayIsStart = 1;
		if ((fd = fopen(srcname, "rb")) == NULL)
		{
			sprintf(Local.DebugInfo, "Open Error:%s\n", srcname);
			P_Debug(Local.DebugInfo);
			AudioPlayIsStart = 0;
			return;
		}

		bytes_read = fread(hWaveFileHeader.chRIFF, sizeof(hWaveFileHeader.chRIFF), 1, fd);
		bytes_read = fread(&hWaveFileHeader.dwRIFFLen, sizeof(hWaveFileHeader.dwRIFFLen), 1, fd);
		bytes_read = fread(hWaveFileHeader.chWAVE, sizeof(hWaveFileHeader.chWAVE), 1, fd);
		bytes_read = fread(hWaveFileHeader.chFMT, sizeof(hWaveFileHeader.chFMT), 1, fd);
		bytes_read = fread(&hWaveFileHeader.dwFMTLen, sizeof(hWaveFileHeader.dwFMTLen), 1, fd);
		bytes_read = fread(&hWaveFileHeader.wFormatTag, sizeof(hWaveFileHeader.wFormatTag), 1, fd);
		bytes_read = fread(&hWaveFileHeader.nChannels, sizeof(hWaveFileHeader.nChannels), 1, fd);
		bytes_read = fread(&hWaveFileHeader.nSamplesPerSec, sizeof(hWaveFileHeader.nSamplesPerSec), 1, fd);
		bytes_read = fread(&hWaveFileHeader.nAvgBytesPerSec, sizeof(hWaveFileHeader.nAvgBytesPerSec), 1, fd);
		bytes_read = fread(&hWaveFileHeader.nBlockAlign, sizeof(hWaveFileHeader.nBlockAlign), 1, fd);
		bytes_read = fread(&hWaveFileHeader.wBitsPerSample, sizeof(hWaveFileHeader.wBitsPerSample), 1, fd);

		fseek(fd,sizeof(hWaveFileHeader.chRIFF)
						+ sizeof(hWaveFileHeader.dwRIFFLen)
						+ sizeof(hWaveFileHeader.chWAVE)
						+ sizeof(hWaveFileHeader.chFMT)
						+ sizeof(hWaveFileHeader.dwFMTLen)
						+ hWaveFileHeader.dwFMTLen, SEEK_SET);
		bytes_read = fread(hWaveFileHeader.chDATA, sizeof(hWaveFileHeader.chDATA), 1, fd);
		//   printf("hWaveFileHeader.chDATA=%s\n", hWaveFileHeader.chDATA);
		if ((hWaveFileHeader.chDATA[0] == 'f')
				&& (hWaveFileHeader.chDATA[1] == 'a')
				&& (hWaveFileHeader.chDATA[2] == 'c')
				&& (hWaveFileHeader.chDATA[3] == 't'))
		{
			bytes_read = fread(&hWaveFileHeader.dwFACTLen,
					sizeof(hWaveFileHeader.dwFACTLen), 1, fd);
			fseek(fd, hWaveFileHeader.dwFACTLen, SEEK_CUR);
			bytes_read = fread(hWaveFileHeader.chDATA,
					sizeof(hWaveFileHeader.chDATA), 1, fd);
			//    printf("hWaveFileHeader.chDATA=%s\n", hWaveFileHeader.chDATA);
		}
		if ((hWaveFileHeader.chDATA[0] == 'd')
				&& (hWaveFileHeader.chDATA[1] == 'a')
				&& (hWaveFileHeader.chDATA[2] == 't')
				&& (hWaveFileHeader.chDATA[3] == 'a'))
		{
			bytes_read = fread(&hWaveFileHeader.dwDATALen,
					sizeof(hWaveFileHeader.dwDATALen), 1, fd);
			//     printf("hWaveFileHeader.dwDATALen=%d\n", hWaveFileHeader.dwDATALen);
		}

		if ((hWaveFileHeader.chRIFF[0] != 'R')
				|| (hWaveFileHeader.chRIFF[1] != 'I')
				|| (hWaveFileHeader.chRIFF[2] != 'F')
				|| (hWaveFileHeader.chRIFF[3] != 'F')
				|| (hWaveFileHeader.chWAVE[0] != 'W')
				|| (hWaveFileHeader.chWAVE[1] != 'A')
				|| (hWaveFileHeader.chWAVE[2] != 'V')
				|| (hWaveFileHeader.chWAVE[3] != 'E')
				|| (hWaveFileHeader.chFMT[0] != 'f')
				|| (hWaveFileHeader.chFMT[1] != 'm')
				|| (hWaveFileHeader.chFMT[2] != 't')
				|| (hWaveFileHeader.chFMT[3] != ' '))
		{
			P_Debug("wav file format error\n");
			fclose(fd);
			AudioPlayIsStart = 0;
			return;
		}

		wavbuf.iput = 0;
		wavbuf.iget = 0;
		//最长留言时间30秒,在分配内存时多分配一倍
		wavbuf.buffer = (unsigned char *) malloc(hWaveFileHeader.dwDATALen);
		bytes_read = fread(wavbuf.buffer, hWaveFileHeader.dwDATALen, 1, fd);
		if (bytes_read != 1)
		{
			sprintf(Local.DebugInfo, "Read Error:%s\n", srcname);
			P_Debug(Local.DebugInfo);
			if (wavbuf.buffer != NULL)
			{
				free(wavbuf.buffer);
				wavbuf.buffer = NULL;
			}
			fclose(fd);
			AudioPlayIsStart = 0;
			return;
		}
		wavbuf.n = hWaveFileHeader.dwDATALen;
		fclose(fd);
#if 1
		//   printf("wavbuf.n = %d\n",wavbuf.n);
		//音频
		if (!OpenSnd(AUDIODSP1))
		{
			P_Debug("******StartPlayWav()****Open play sound device error!\n");
			AudioPlayIsStart = 0; //one key open door no sound
			return;
		}
		//printf("hWaveFileHeader.wBitsPerSample=%d\n", hWaveFileHeader.wBitsPerSample);
		//printf("hWaveFileHeader.nSamplesPerSec=%d\n", hWaveFileHeader.nSamplesPerSec);
		//printf("hWaveFileHeader.nChannels=%d\n", hWaveFileHeader.nChannels);

		SetFormat(AUDIODSP1, hWaveFileHeader.wBitsPerSample,
				hWaveFileHeader.nSamplesPerSec, hWaveFileHeader.nChannels); //放音
		//  SetFormat(AUDIODSP1, FMT16BITS, FMT8K, 1);    //放音   STERO, WAVOUTDEV

		audio_play_wav_flag = 1;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio_play_wav_thread, &attr, (void *) audio_play_wav_thread_func, (void *) PlayFlag);
		pthread_attr_destroy(&attr);
		if (audio_play_wav_thread == 0)
		{
			P_Debug("can't create sound play thread!!!!!\n");
			AudioPlayIsStart = 0;
			return;
		}
#else
		if(wavbuf.buffer != NULL)
		{
			printf("free wavbuf.buffer\n");
			free(wavbuf.buffer);
			wavbuf.buffer = NULL;
		}
		AudioPlayIsStart = 0;
#endif      
	}
	else
	{
		P_Debug("sound card is busy now!!!\n");
		return;
	}
#endif
}
//---------------------------------------------------------------------------
void StopPlayWavFile(void)
{
	char acVolSet[128]={0x0};
	int delaytime;
	delaytime = 40;
	audio_play_wav_flag = 0;
	sprintf(acVolSet,"echo %d > /proc/asound/adda300_SPV",g_stIdbtCfg.iOutputCallVol); //恢复原来声音值
	system(acVolSet);
	sprintf(Local.DebugInfo, "XT debug LOCK AUDIO g_iOutputVol is :%s", acVolSet);
	P_Debug(Local.DebugInfo);
}
//---------------------------------------------------------------------------
void audio_play_wav_thread_func(int PlayFlag)
{
	int arg; // 用于ioctl调用的参数
	int status; // 系统调用的返回值
	struct timeval tv;

	int dwSize;
	int i;
	int AudioLen;

	if (DebugMode == 1)
		printf("audio play wav thread begin to run!!!!!!\n");
#if 1
	while (audio_play_wav_flag == 1)
	{
		if ((wavbuf.iget + AUDIOPLAYBLK) < wavbuf.n)
			AudioLen = AUDIOPLAYBLK;
		else
			AudioLen = wavbuf.n - wavbuf.iget;
		dwSize = Play((char *) (wavbuf.buffer + wavbuf.iget), AudioLen);

		if (dwSize != AUDIOPLAYBLK)
		{
			sprintf(Local.DebugInfo, "audio_play_wav_thread_func::dwSize = %d, AUDIOPLAYBLK = %d\n", dwSize, AUDIOPLAYBLK);
			P_Debug(Local.DebugInfo);
		}
		wavbuf.iget += AudioLen;

		//if(wavbuf.iget >= wavbuf.n)
		if ((wavbuf.iget + AUDIOPLAYBLK) >= wavbuf.n)
		{
			if (PlayFlag == 0) //单次播放
			{
				WaitAudioUnuse(1000);
				StopPlayWavFile();
			}
			else //循环播放
			{
				wavbuf.iget = 0;
			}
		}
	}
#endif   

	//CloseSnd(AUDIODSP1);del by zlin
	// 等待回放结束
	//printf("wavbuf.n = %d, wavbuf.iget = %d\n", wavbuf.n, wavbuf.iget);
	gettimeofday(&tv, NULL);
	//printf("1 tv.tv_sec=%d, tv.tv_usec=%d\n",
	//         tv.tv_sec, tv.tv_usec);
	SyncPlay();
	//gettimeofday(&tv, NULL);
	//printf("2 tv.tv_sec=%d, tv.tv_usec=%d\n",
	//         tv.tv_sec, tv.tv_usec);
	if (wavbuf.buffer != NULL)
	{
		//printf("free wavbuf.buffer\n");
		free(wavbuf.buffer);
		wavbuf.buffer = NULL;
	}

	if (DebugMode == 1)
		P_Debug("audio play wav thread exit now!!!!!!\n");
	AudioPlayIsStart = 0;
}
//---------------------------------------------------------------------------
TempAudioNode1 * init_audionode(void) //初始化单链表的函数
{
	TempAudioNode1 *h; // *h保存表头结点的指针，*p指向当前结点的前一个结点，*s指向当前结点
	if ((h = (TempAudioNode1 *) malloc(sizeof(TempAudioNode1))) == NULL) //分配空间并检测
	{
		P_Debug("don't audio malloc!");
		return NULL;
	}
	h->llink = NULL; //左链域
	h->rlink = NULL; //右链域
	return (h);
}
//---------------------------------------------------------------------------
int creat_audionode(TempAudioNode1 *h, struct talkdata1 talkdata, unsigned char *r_buf, int r_length)
{
	TempAudioNode1 *t;
	TempAudioNode1 *p;
	int j;
	int DataOk;
//    uint32_t newframeno;
//    int currpackage;
	t = h;
	//  t=h->next;
	while (t->rlink != NULL) //循环，直到t指向空
		t = t->rlink; //t指向下一结点
	DataOk = 1;
	if ((talkdata.DataType < 1) || (talkdata.DataType > 5))
		DataOk = 0;
	if (talkdata.Framelen > VIDEOMAX)
		DataOk = 0;
	if (talkdata.CurrPackage > talkdata.TotalPackage)
		DataOk = 0;
	if (talkdata.CurrPackage <= 0)
		DataOk = 0;
	if (talkdata.TotalPackage <= 0)
		DataOk = 0;
	if (t && (DataOk == 1))
	{
		//尾插法建立链表
		if ((p = (TempAudioNode1 *) malloc(sizeof(TempAudioNode1))) == NULL) //生成新结点s，并分配内存空间
		{
			P_Debug("don't malloc memory!\n");
			return 0;
		}
		if ((p->Content.buffer = (unsigned char *) malloc(talkdata.Framelen))
				== NULL)
		{
			P_Debug("don't malloc audio receive memory!\n");
			return 0;
		}
		p->Content.isFull = 0;
		for (j = 0; j < MAXPACKNUM; j++)
			p->Content.CurrPackage[j] = 0;

		p->Content.frame_flag = talkdata.DataType;
		//  newframeno = (r_buf[63] << 8) + r_buf[64];
		//  currpackage = (r_buf[67] << 8) + r_buf[68];
		p->Content.frameno = talkdata.Frameno;
		p->Content.TotalPackage = talkdata.TotalPackage;
		p->Content.timestamp = talkdata.timestamp;
		p->Content.CurrPackage[talkdata.CurrPackage - 1] = 1;

		if (talkdata.CurrPackage == p->Content.TotalPackage)
			p->Content.Len = (talkdata.CurrPackage - 1) * talkdata.PackLen + talkdata.Datalen;
		memcpy(p->Content.buffer + (talkdata.CurrPackage - 1) * talkdata.PackLen,
				r_buf + DeltaLen, talkdata.Datalen);

//      if(talkdata.CurrPackage == p->Content.TotalPackage)
//        p->Content.Len =  (talkdata.CurrPackage - 1) * talkdata.PackLen + r_length - DeltaLen;
//      memcpy(p->Content.buffer + (talkdata.CurrPackage - 1) * talkdata.PackLen,
//             r_buf + DeltaLen, r_length - DeltaLen);

		p->Content.isFull = 1;
		for (j = 0; j < p->Content.TotalPackage; j++)
			if (p->Content.CurrPackage[j] == 0)
			{
				p->Content.isFull = 0;
				break;
			}
		p->rlink = NULL; //p的指针域为空
		p->llink = t;
		t->rlink = p; //p的next指向这个结点
		t = p; //t指向这个结点
		return p->Content.isFull;
	}
}
//---------------------------------------------------------------------------
//函数名称：length
//功能描述：求单链表长度
//返回类型：无返回值
//函数参数：h:单链表头指针
int length_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p;
	int i = 0; //记录链表长度
	p = h->rlink;
	while (p != NULL) //循环，直到p指向空
	{
		i = i + 1;
		p = p->rlink; //p指向下一结点
	}
	return i;
	//    printf(" %d",i); //输出p所指接点的数据域
}
//---------------------------------------------------------------------------
//函数名称：delete_
//功能描述：删除函数
//返回类型：整型
//函数参数：h:单链表头指针 i:要删除的位置
int delete_audionode(TempAudioNode1 *p)
{
	if (p != NULL)
	{
		//不为最后一个结点
		if (p->rlink != NULL)
		{
			(p->rlink)->llink = p->llink;
			(p->llink)->rlink = p->rlink;
			if (p->Content.buffer)
				free(p->Content.buffer);
			if (p)
				free(p);
		}
		else
		{
			(p->llink)->rlink = p->rlink;
			if (p->Content.buffer)
				free(p->Content.buffer);
			if (p)
				free(p);
		}
		return (1);
	}
	else
		P_Debug("audio delete null\n");
	return (0);
}
//---------------------------------------------------------------------------
int delete_all_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p, *q;
	p = h->rlink; //此时p为首结点
	while (p != NULL) //找到要删除结点的位置
	{
		//不为最后一个结点
		q = p;
		if (p->rlink != NULL)
		{
			(p->rlink)->llink = p->llink;
			(p->llink)->rlink = p->rlink;
		}
		else
			(p->llink)->rlink = p->rlink;
		p = p->rlink;
		if (q->Content.buffer)
			free(q->Content.buffer);
		if (q)
			free(q);
	}
}
//---------------------------------------------------------------------------
int delete_lost_audionode(TempAudioNode1 *h, uint32_t currframeno,
		uint32_t currtimestamp) //删除不全帧
{
	TempAudioNode1 *p, *q;
	p = h->rlink; //此时p为首结点
	while (p != NULL) //找到要删除结点的位置
	{
		//不为最后一个结点
		q = p;
		if (p->rlink != NULL)
		{
			//      if(p->Content.frameno < currframeno) //进入循环，直到p为空，或找到x
			if (p->Content.timestamp < currtimestamp)
			{
				(p->rlink)->llink = p->llink;
				(p->llink)->rlink = p->rlink;
				p = p->llink;
				if (q->Content.buffer)
					free(q->Content.buffer);
				if (q)
					free(q);
				if (temp_audio_n > 0)
					temp_audio_n--;
			}
		}
		else
		{
			//      if(p->Content.frameno < currframeno) //进入循环，直到p为空，或找到x
			if (p->Content.timestamp < currtimestamp)
			{
				(p->llink)->rlink = p->rlink;
				p = p->llink;
				if (q->Content.buffer)
					free(q->Content.buffer);
				if (q)
					free(q);
				if (temp_audio_n > 0)
					temp_audio_n--;
			}
		}
		p = p->rlink;
	}
	return 1;

}
//---------------------------------------------------------------------------
//函数名称：find_
//功能描述：查找函数
//返回类型：整型
//函数参数：h:单链表头指针 x:要查找的值
//查找该帧该包是否已存在
TempAudioNode1 * find_audionode(TempAudioNode1 *h, int currframeno,
		int currpackage)
{
	TempAudioNode1 *p;
	int PackIsExist; //数据包已接收标志
	int FrameIsNew; //数据包是否是新帧的开始
	p = h->rlink; //此时p为首结点
	PackIsExist = 0;
	FrameIsNew = 1;
	while (p != NULL)
	{
		if (p->Content.frameno == currframeno) //进入循环，直到p为空，或找到x
		{
			FrameIsNew = 0;
			if (p->Content.CurrPackage[currpackage - 1] == 1)
			{
				if (DebugMode == 1)
				{
					sprintf(Local.DebugInfo, "pack exist %d\n", currframeno);
					P_Debug(Local.DebugInfo);
				}
				PackIsExist = 1;
			}
			break;
		}
		p = p->rlink; //s指向p的下一结点
	}
	if (p != NULL
		)
		return p;
	else
		return NULL;
}
//---------------------------------------------------------------------------
//函数名称：find_
//功能描述：查找函数
//返回类型：整型
//函数参数：h:单链表头指针 x:要查找的值
//查找最老的帧
TempAudioNode1 * search_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p;
	TempAudioNode1 *tem_p;

	p = h->rlink; //此时p为首结点
	tem_p = p;

	while (p != NULL)
	{
		if (p->Content.isFull == 1)
			//   if(p->Content.frameno < tem_p->Content.frameno) //进入循环，直到p为空，或找到x
			if (p->Content.timestamp < tem_p->Content.timestamp) //进入循环，直到p为空，或找到x
			{
				tem_p = p;
			}
		p = p->rlink; //s指向p的下一结点
	}

	return tem_p;

}
//---------------------------------------------------------------------------
int free_audionode(TempAudioNode1 *h)
{
	TempAudioNode1 *p, *t;
	int i = 0; //记录链表长度
	p = h->rlink;
	while (p != NULL) //循环，直到p指向空
	{
		i = i + 1;
		t = p;
		p = p->rlink; //p指向下一结点
		free(t);
	}
	return i;
}
//-------------------------------------------------------------------------
void WaitAudioUnuse(int MaxTime) //等待声卡空闲
{
	//等待声卡空闲
	int DelayT = 0;
	while (AudioPlayIsStart == 1)
	{
		usleep(1000);
		DelayT++;
		if (DelayT > (MaxTime / 10))
			break;
	}
//  printf("DelayT = %d\n", DelayT);
}
//---------------------------------------------------------------------------
