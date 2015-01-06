#ifndef __CFIMC_MPEG4_H__
#define __CFIMC_MPEG4_H__



#ifdef __cplusplus
    extern "C" {
#endif

int CamInit(int videoid, int width, int height);
int CamGetFrame(void **pFrameBuf, int *FrameLen);
int CamClose(void);
int CamGetYAddr(int index);
int CamGetCAddr(int index);
int CamReleaseFrame(int index);






#ifdef __cplusplus
    }
#endif


#endif
