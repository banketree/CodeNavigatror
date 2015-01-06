#ifndef __LINUX_AUDIO_H__
#define __LINUX_AUDIO_H__






int linux_snd_card_detect(void);
int linux_snd_read_init(void);
int linux_snd_read(char *read_buf);
void linux_snd_read_uninit(void);
int linux_snd_write_init(void);
int linux_snd_write(char *read_buf, int write_len);
void linux_snd_write_uninit(void);
int linux_snd_is_write_end(void);

extern "C" int SndCardDetect(void);

extern "C" int SndCardAudioRecordInit(void);

//extern "C" int AudioRecord(char *readBuf);
extern "C" int AudioRecord(char *readBuf, int readLen);

extern "C" int SndCardAudioRecordUninit(void);

extern "C" int SndCardAudioPlayInit(void);

extern "C" int AudioPlay(char *readBuf, int writeLen);

extern "C" int SndCardAudioPlayUnnit(void);










#endif
