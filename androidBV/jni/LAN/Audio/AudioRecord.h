#ifndef 	__AUDIO_RECORD_H__
#define	__AUDIO_RECORD_H__


int record_pcm(char *pPcmBuf, int PcmBufLen);
int record_open(int SampleRate, int ChannelNumber);
int record_close(void);
int AudioRecord(char *pAudioBuf, int FrameSize, int nFrame, double AmplifyRatio);


#endif
