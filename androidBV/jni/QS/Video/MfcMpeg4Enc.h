#ifndef __MPEG4_ENC__
#define __MPEG4_ENC__

#include "OMX_Types.h"
#include "OMX_Core.h"
#include "SsbSipMfcApi.h"
#include "mfc_interface.h"

typedef enum _CODEC_TYPE
{
    CODEC_TYPE_H263,
    CODEC_TYPE_MPEG4
} CODEC_TYPE;

typedef struct _MFC_MPEG4ENC_ST
{
	OMX_HANDLETYPE             hMFCHandle;
	SSBSIP_MFC_ENC_MPEG4_PARAM mpeg4MFCParam;
	SSBSIP_MFC_ENC_INPUT_INFO  inputInfo;
	SSBSIP_MFC_ENC_OUTPUT_INFO outputInfo;
	OMX_U32                    indexTimestamp;
	OMX_BOOL                   bConfiguredMFC;
	CODEC_TYPE                 codecType;
} MFC_MPEG4ENC_ST;


typedef struct _VIDEO_PLANER_ST
{
	int Height;
	int Width;
	void *YPhyAddr;
	void *CPhyAddr;
	int YSize;
	int CSize;
} VIDEO_PLANER_ST;


typedef struct
{
	int Quality;
	int Bitrate;
	int Frames;
	int headerSize;
	char header[100];
	VIDEO_PLANER_ST VideoPlaner;
	MFC_MPEG4ENC_ST MfcMpeg4Enc;
}VIDEO_MPEG4ENC_ST;


OMX_ERRORTYPE MfcMpeg4EncProcess(VIDEO_MPEG4ENC_ST *pVideo, char **pMpeg4, int *mpeg4Len);
OMX_ERRORTYPE MfcEncMpeg4Init(VIDEO_MPEG4ENC_ST *pVideo);
OMX_ERRORTYPE MfcMpeg4EncExit(VIDEO_MPEG4ENC_ST *pVideo);
void MfcGetInputBuf(VIDEO_MPEG4ENC_ST *pVideo, void **Y,void **UV);


#endif
