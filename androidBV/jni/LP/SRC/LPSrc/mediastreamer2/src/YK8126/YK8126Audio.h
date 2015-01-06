
#ifndef YK8126AUDIO_H_
#define YK8126AUDIO_H_

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"
#include <sys/soundcard.h>

#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>


typedef struct YK8126AudioData{
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
} YK8126AudioData;



void YK8126AudioParamInit(void);
int YK8126AudioSetFormat(int devfd, int bits, int hz, int chn, int *buffSize);
int configure_fd(int fd, int bits,int stereo, int rate, int *minsz);






#endif /* YK8126AUDIOAV_H_ */
