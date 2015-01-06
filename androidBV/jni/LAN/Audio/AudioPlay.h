#ifndef 	__AUDIO_PLAY_H__
#define	__AUDIO_PLAY_H__

#include "alsa_audio.h"

int play_pcm(struct pcm *pcm_play, char *pPcmBuf, int PcmBufLen);
struct pcm *play_open(int SampleRate, int ChannelNumber);
int play_close(struct pcm *pcm_play);
int AudioPlay(struct pcm *pcm_play, char *pAudioBuf, int AudioLen);


#endif
