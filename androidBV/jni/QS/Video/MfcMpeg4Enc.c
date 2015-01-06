#include "MfcMpeg4Enc.h"
#include "stdint.h"
#include <android/log.h>

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "ProjectName", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "ProjectName", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , "ProjectName", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , "ProjectName", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "ProjectName", __VA_ARGS__)


OMX_ERRORTYPE MfcEncMpeg4Init(VIDEO_MPEG4ENC_ST *pVideo)
{
   OMX_HANDLETYPE  hMFCHandle = NULL;
   OMX_S32 returnCodec = 0;
   OMX_S32 ret = OMX_ErrorNone;
   MFC_MPEG4ENC_ST *pMfcMpeg4EncData= &pVideo->MfcMpeg4Enc;

	pMfcMpeg4EncData->mpeg4MFCParam.codecType = MPEG4_ENC;
	pMfcMpeg4EncData->mpeg4MFCParam.SourceHeight = pVideo->VideoPlaner.Height;
	pMfcMpeg4EncData->mpeg4MFCParam.SourceWidth = pVideo->VideoPlaner.Width;
	pMfcMpeg4EncData->mpeg4MFCParam.IDRPeriod            = 1;//pVideo->Frames + 1;
	pMfcMpeg4EncData->mpeg4MFCParam.SliceMode            = 1;
	pMfcMpeg4EncData->mpeg4MFCParam.RandomIntraMBRefresh = 0;
	pMfcMpeg4EncData->mpeg4MFCParam.Bitrate              = pVideo->Bitrate;
	pMfcMpeg4EncData->mpeg4MFCParam.QSCodeMax            = 30;
	pMfcMpeg4EncData->mpeg4MFCParam.QSCodeMin            = 10;
	pMfcMpeg4EncData->mpeg4MFCParam.PadControlOn         = 0;    /* 0: Use boundary pixel, 1: Use the below setting value */
	pMfcMpeg4EncData->mpeg4MFCParam.LumaPadVal           = 0;
	pMfcMpeg4EncData->mpeg4MFCParam.CbPadVal             = 0;
	pMfcMpeg4EncData->mpeg4MFCParam.CrPadVal             = 0;

	pMfcMpeg4EncData->mpeg4MFCParam.ProfileIDC           = 66;//0:sp 1:ap;				//OMXMpeg4ProfileToMFCProfile(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eProfile);
	pMfcMpeg4EncData->mpeg4MFCParam.LevelIDC             = 22;//13;				//OMXMpeg4LevelToMFCLevel(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eLevel);
	pMfcMpeg4EncData->mpeg4MFCParam.TimeIncreamentRes    = pVideo->Frames;//4;//pVideo->Frames;		//(15 << 16);	//(pSECInputPort->portDefinition.format.video.xFramerate) >> 16;
	pMfcMpeg4EncData->mpeg4MFCParam.VopTimeIncreament    = 1;
	pMfcMpeg4EncData->mpeg4MFCParam.SliceArgument        = 50;    /* MB number or byte number */
	pMfcMpeg4EncData->mpeg4MFCParam.NumberBFrames        = 0;    /* 0(not used) ~ 2 */
	pMfcMpeg4EncData->mpeg4MFCParam.DisableQpelME        = 1;

	//???????要不要设置
	pMfcMpeg4EncData->mpeg4MFCParam.FrameQp              = pVideo->Quality;	//pVideoEnc->quantization.nQpI;
	pMfcMpeg4EncData->mpeg4MFCParam.FrameQp_P            = pVideo->Quality;	//pVideoEnc->quantization.nQpP;
	pMfcMpeg4EncData->mpeg4MFCParam.FrameQp_B            = 0;					//pVideoEnc->quantization.nQpB;

	pMfcMpeg4EncData->mpeg4MFCParam.EnableFRMRateControl = 1;
	pMfcMpeg4EncData->mpeg4MFCParam.CBRPeriodRf			  = 10;

	/* MFC(Multi Format Codec) encoder and CMM(Codec Memory Management) driver open */
	hMFCHandle = SsbSipMfcEncOpen();
	if (hMFCHandle == NULL)
	{
		ret = OMX_ErrorInsufficientResources;
		printf("SsbSipMfcEncOpen() failed!\n");
		goto EXIT;
	}

	pMfcMpeg4EncData->hMFCHandle = hMFCHandle;

	returnCodec = SsbSipMfcEncInit(hMFCHandle, &(pMfcMpeg4EncData->mpeg4MFCParam));
	if(returnCodec != MFC_RET_OK)
	{
		ret = OMX_ErrorInsufficientResources;
		printf("SsbSipMfcEncInit() failed!\n");
	   goto EXIT;
	}

	returnCodec = SsbSipMfcEncGetInBuf(hMFCHandle, &(pMfcMpeg4EncData->inputInfo));
	if (returnCodec != MFC_RET_OK)
	{
		ret = OMX_ErrorInsufficientResources;
		printf("SsbSipMfcEncGetInBuf() failed!\n");
		goto EXIT;
	}

	returnCodec = SsbSipMfcEncGetOutBuf(hMFCHandle, &(pMfcMpeg4EncData->outputInfo));
	if(returnCodec != MFC_RET_OK)
	{
		ret = OMX_ErrorInsufficientResources;
		printf("SsbSipMfcEncSetInBuf fail\n");
		goto EXIT;
	}

	pVideo->headerSize = pMfcMpeg4EncData->outputInfo.headerSize;

	memcpy(pVideo->header, pMfcMpeg4EncData->outputInfo.StrmVirAddr, pVideo->headerSize);

	printf("MfcEncMpeg4Init() init succeed!\n");


	printf("MFC init success:: Yphy(0x%08x) Cphy(0x%08x)\n", pMfcMpeg4EncData->inputInfo.YPhyAddr, pMfcMpeg4EncData->inputInfo.CPhyAddr);
	printf("MFC init success:: Yvir(0x%08x) Cvir(0x%08x)\n", pMfcMpeg4EncData->inputInfo.YVirAddr, pMfcMpeg4EncData->inputInfo.CVirAddr);
	return ret;

EXIT:

	return ret;
}

void MfcGetInputBuf(VIDEO_MPEG4ENC_ST *pVideo, void **Y,void **UV)
{
	MFC_MPEG4ENC_ST *pMfcMpeg4Enc = &pVideo->MfcMpeg4Enc;
	SSBSIP_MFC_ENC_INPUT_INFO *pInputInfo = &(pMfcMpeg4Enc->inputInfo);

    *Y=pInputInfo->YVirAddr;
    *UV=pInputInfo->CVirAddr;
}


OMX_ERRORTYPE MfcMpeg4EncProcess(VIDEO_MPEG4ENC_ST *pVideo, char **pMpeg4, int *mpeg4Len)
{
	OMX_S32 returnCodec = 0;
	OMX_S32 ret = OMX_ErrorNone;

	OMX_HANDLETYPE  hMFCHandle = pVideo->MfcMpeg4Enc.hMFCHandle;
	MFC_MPEG4ENC_ST *pMfcMpeg4Enc = &pVideo->MfcMpeg4Enc;
	SSBSIP_MFC_ENC_INPUT_INFO *pInputInfo = &(pMfcMpeg4Enc->inputInfo);
	SSBSIP_MFC_ENC_OUTPUT_INFO *pOutputInfo = &(pMfcMpeg4Enc->outputInfo);
	VIDEO_PLANER_ST *pVideoPlaner = &pVideo->VideoPlaner;

//	pInputInfo->YPhyAddr = pVideoPlaner->YPhyAddr;
//	pInputInfo->CPhyAddr = pVideoPlaner->CPhyAddr;
//	pInputInfo->YVirAddr = pVideoPlaner->YPhyAddr;
//	pInputInfo->CVirAddr = pVideoPlaner->CPhyAddr;
	pInputInfo->YSize = pVideoPlaner->YSize;
	pInputInfo->CSize = pVideoPlaner->CSize;

//	printf("pInputInfo->YSize = %d, pInputInfo->CSize = %d \n", pInputInfo->YSize, pInputInfo->CSize);

	if((pInputInfo->YPhyAddr == NULL) || (pInputInfo->CPhyAddr == NULL))
	{
		printf("(pInputInfo->YPhyAddr == NULL) || (pInputInfo->CPhyAddr == NULL) \n");
		ret = OMX_ErrorInsufficientResources;
		goto EXIT;
	}

	returnCodec = SsbSipMfcEncSetInBuf(hMFCHandle, pInputInfo);
	if(returnCodec != MFC_RET_OK)
	{
		printf("returnCodec = %d\n", returnCodec);
		ret = OMX_ErrorInsufficientResources;
		printf("SsbSipMfcEncSetInBuf fail\n");
		goto EXIT;
	}

	returnCodec = SsbSipMfcEncExe(hMFCHandle);
	if(returnCodec != MFC_RET_OK)
	{
		printf("returnCodec = %d\n", returnCodec);
		printf("SsbSipMFCEncExe fail\n");
		ret = OMX_ErrorInsufficientResources;
		goto EXIT;

	}

	returnCodec = SsbSipMfcEncGetOutBuf(hMFCHandle, pOutputInfo);
	if(returnCodec != MFC_RET_OK)
	{
		printf("returnCodec = %d\n", returnCodec);
		ret = OMX_ErrorInsufficientResources;
		printf("SsbSipMfcEncGetOutBuf fail\n");
		goto EXIT;
	}

	*pMpeg4 = pOutputInfo->StrmVirAddr;
	*mpeg4Len = pOutputInfo->dataSize;

	return OMX_ErrorNone;

EXIT:
	return ret;
}

OMX_ERRORTYPE MfcMpeg4EncExit(VIDEO_MPEG4ENC_ST *pVideo)
{
	OMX_ERRORTYPE          ret = OMX_ErrorNone;
	OMX_HANDLETYPE         hMFCHandle = NULL;
	MFC_MPEG4ENC_ST *pMfcMpeg4Enc = &pVideo->MfcMpeg4Enc;

	hMFCHandle = pMfcMpeg4Enc->hMFCHandle;
	if(hMFCHandle != NULL)
	{
		SsbSipMfcEncClose(hMFCHandle);
		pMfcMpeg4Enc->hMFCHandle = NULL;
	}
}

