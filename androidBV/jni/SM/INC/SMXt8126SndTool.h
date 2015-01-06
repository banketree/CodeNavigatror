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

#define AUDIOBLK        128             //ÿ֡8ms
#define TAIL            1280            //1600//1280
#define AFRAMETIME      (AUDIOBLK/16)   //ÿ֡ms


//WAV�ļ��ļ�ͷ����
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


//��Ƶ�ļ����ͻ���������
typedef struct
 {
  int iPut;           //���λ������ĵ�ǰ����λ��
  int iGet;           //�������ĵ�ǰȡ��λ��
  int iAudioLen;      //���λ���������
  unsigned char *pucBuf;
 }WAV_BUF_ST;


/**
 *@brief 		��ʼ��XT8126��Ƶ����
 *@param 		��
 *@return 		��
 */
void SMXt8126AudioInit(void);

/**
 *@brief 		���ű���wav�ļ�
 *@param[in] 	pcFilePath-��Ƶ�ļ�·��
 *@param[in] 	iPlayFlg-�ظ����ű�־λ��0-һ��  1-�ظ���
 *@return 		TRUE-�ɹ�	FALSE-ʧ��
 */
void SMPlayWavFile(char *pcFilePath, int iPlayFlg);

/**
 *@brief 		ֹͣ���ű���wav�ļ�
 *@param     	��
 *@return 		��
 */
void SMStopPlayWavFile(void);


#endif /* SMXT8126SNDTOOL_H_ */
