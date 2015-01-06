#ifndef __CFIMC_MJJPEG_H__
#define __CFIMC_MJJPEG_H__


#ifdef __cplusplus
    extern "C" {
#endif

int CamInit(int videoid, int width, int height);
unsigned int CamCapture(unsigned char *pLibjpegBuf);
int CamClose(void);








#ifdef __cplusplus
    }
#endif


#endif
