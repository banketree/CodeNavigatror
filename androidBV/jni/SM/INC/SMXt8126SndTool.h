/*
 * SMXt8126SndTool.h
 *
 *  Created on: 2012-8-21
 *      Author: root
 */

#ifndef SMXT8126SNDTOOL_H_
#define SMXT8126SNDTOOL_H_

#define FMT8BITS        8
#define FMT16BITS       16

#define FMT8K           8000
#define FMT16K          16000
#define FMT22K          22000
#define FMT44K          44000

#define MONO            0
#define STERO           1

#define AUDIOPLAYBLK    128

#define AUDIOBLK        128             //每帧8ms
#define TAIL            1280            //1600//1280
#define AFRAMETIME      (AUDIOBLK/16)   //每帧ms


//WAV文件文件头定义
typedef struct
{
	char chRIFF[4];
	uint32_t dwRIFFLen;
	char chWAVE[4];

	char chFMT[4];
	uint32_t dwFMTLen;
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;

	char chFACT[4];
	uint32_t dwFACTLen;

	char chDATA[4];
	uint32_t dwDATALen;
}WAV_FILE_HEADER_ST;


//音频文件缓型缓冲区定义
typedef struct
 {
  int iPut;           //环形缓冲区的当前放入位置
  int iGet;           //缓冲区的当前取出位置
  int iAudioLen;      //环形缓冲区长度
  unsigned char *pucBuf;
 }WAV_BUF_ST;


/**
 *@brief 		初始化XT8126音频参数
 *@param 		无
 *@return 		无
 */
void SMXt8126AudioInit(void);

/**
 *@brief 		播放本地wav文件
 *@param[in] 	pcFilePath-音频文件路径
 *@param[in] 	iPlayFlg-重复播放标志位（0-一次  1-重复）
 *@return 		TRUE-成功	FALSE-失败
 */
void SMPlayWavFile(char *pcFilePath, int iPlayFlg);

/**
 *@brief 		停止播放本地wav文件
 *@param     	无
 *@return 		无
 */
void SMStopPlayWavFile(void);


#endif /* SMXT8126SNDTOOL_H_ */
