#ifdef _YK_IMX27_AV_
/******************************************************
Filename            :       jpg�ļ�����
yuvData             :       �����yuv�����ַ
quality               :        ѹ������ 1-100
image_width     :       ͼ�����
image_height    :       ͼ��߶�
******************************************************/

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/timeb.h>
#include "jpeglib.h"





//#define TRUE 0
//#define FALSE -1
#define YUVFilename "/tftpboot/test288.yuv"
//#define YUV_SIZE 84480+21120+21120      // CIF=352*240
#define YUV_SIZE 152064 //101376+25344+25344		// CIF=352*288
#define JPEG_NAME "./test288.jpeg"


#if 0
int write_JPEG_file (char * filename, unsigned char* yuvData, int quality,int image_width,int image_height)
{

     struct jpeg_compress_struct cinfo;
     struct jpeg_error_mgr jerr;
     FILE * outfile;    // target file 
     JSAMPROW row_pointer[1];  // pointer to JSAMPLE row[s] 
     int row_stride;    // physical row width in image buffer
     JSAMPIMAGE  buffer;
     unsigned char *pSrc,*pDst;

     int counter, band,i,buf_width[3],buf_height[3];

     int src_width[3],src_height[3];

     unsigned char *rawData[3];

     cinfo.err = jpeg_std_error(&jerr);
     jpeg_create_compress(&cinfo);

     // 以二进制的方式打开jpeg图像要存放的文件
     if ((outfile = fopen(filename, "wb")) == NULL)
     {
         fprintf(stderr, "can't open %s\n", filename);
         exit(1);
     }

     // 指定压缩后图像所存放的目标文件
     jpeg_stdio_dest(&cinfo, outfile);

     cinfo.image_width = image_width;  // 位图的宽和高，image width and height, in pixels

     cinfo.image_height = image_height;

     cinfo.input_components = 3;    // 1表示灰度图，3表示彩色位图，# of color components per pixel

     cinfo.in_color_space = JCS_RGB;  // JCS_GRAYSCALE表示灰度图，JCS_RGB表示彩色图像，colorspace of input image

     jpeg_set_defaults(&cinfo);

     jpeg_set_quality(&cinfo, quality, TRUE );

     //////////////////////////////

     cinfo.raw_data_in = TRUE;

     cinfo.jpeg_color_space = JCS_YCbCr;

     cinfo.comp_info[0].h_samp_factor = 2;

     cinfo.comp_info[0].v_samp_factor = 2;

     /////////////////////////


     // 上面的工作准备完成之后，就可以压缩里，压缩过程非常简单，首先调用jpeg_start_compress，然后
     // 可以对每一行进行压缩，也可以对若干行进行压缩，甚至可以对整个图像进行一次压缩，压缩完成之后，
     // 调用jpeg_finish_compress函数

     jpeg_start_compress(&cinfo, TRUE);

     buffer = (JSAMPIMAGE) (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo,

                                 JPOOL_IMAGE, 3 * sizeof(JSAMPARRAY)); 

     for(band=0; band <3; band++)
     {

         buf_width[band] = cinfo.comp_info[band].width_in_blocks * DCTSIZE;

         buf_height[band] = cinfo.comp_info[band].v_samp_factor * DCTSIZE;

         buffer[band] = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo,

                                 JPOOL_IMAGE, buf_width[band], buf_height[band]);

     }

     rawData[0]=yuvData;

     rawData[1]=yuvData+image_width*image_height;

     rawData[2]=yuvData+image_width*image_height*5/4;

     for(i=0;i<3;i++)
     {
         src_width[i]=(i==0)?image_width:image_width/2;

         src_height[i]=(i==0)?image_height:image_height/2;
     }

     //max_line一般为16，外循环每次处理16行数据。

     int max_line = cinfo.max_v_samp_factor*DCTSIZE; 

     for(counter=0; cinfo.next_scanline < cinfo.image_height; counter++)
     {   
         //buffer image copy.

         for(band=0; band <3; band++)  //每个分量分别处理
         {
              int mem_size = src_width[band];//buf_width[band];

              pDst = (unsigned char *) buffer[band][0];

              pSrc = (unsigned char *) rawData[band] + counter*buf_height[band] * src_width[band];//buf_width[band];  //yuv.data[band]分别表示YUV起始地址

              for(i=0; i <buf_height[band]; i++)  //处理每行数据
              {
                   memcpy(pDst, pSrc, mem_size);

                   pSrc += src_width[band];//buf_width[band];

                   pDst += buf_width[band];
              }
         }
         jpeg_write_raw_data(&cinfo, buffer, max_line);
     }

     jpeg_finish_compress(&cinfo);

     fclose(outfile);

     jpeg_destroy_compress(&cinfo);

     return 0;
}
#endif

int ReadYUVFile(const char *filename, unsigned char *pYUVBuf)
{
//	int fp;
	FILE *fp = NULL;
	unsigned long count=0;

	if( (fp = fopen(filename, "r")) == NULL)
	{
  		printf( "Can not open YUV file: %s\n", filename );
  		return FALSE;
 	}
 	else
 	{
 		printf( "Open YUV file: %s\n", filename );
 	}

	// 将第一帧去除，YUV文件从第二帧开始
	fseek(fp, YUV_SIZE, SEEK_CUR);
//	if(ftruncate(fp, YUV_SIZE) == -1)
//	{
//		return FALSE;
//	}

	if ((count= fread(pYUVBuf, sizeof(unsigned char), YUV_SIZE, fp)) != YUV_SIZE)
 	{
  		printf( "Read YUV file failed: YUV_SIZE=%ld, but count=%ld\n", YUV_SIZE, count);
  		fclose(fp);
  		return FALSE;
 	}
 	else
 	{
 		printf( "Read YUV file from the second fram\n");
 		fclose(fp);
 		return TRUE;
 	}
}

static int convert_yuv420p_to_jpeg_file(char * filename, unsigned char *image, int width, int height, int quality)
{
    int i, j;

    JSAMPROW y[16],cb[16],cr[16]; // y[2][5] = color sample of row 2 and pixel column 5; (one plane)
    JSAMPARRAY data[3]; // t[0][2][5] = color sample 0 of row 2 and column 5

    FILE *fp;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    data[0] = y;
    data[1] = cb;
    data[2] = cr;

    cinfo.err = jpeg_std_error(&jerr);  // Errors get written to stderr

    jpeg_create_compress(&cinfo);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    jpeg_set_defaults(&cinfo);

    jpeg_set_colorspace(&cinfo, JCS_YCbCr);

    cinfo.raw_data_in = TRUE; // Supply downsampled data
#if JPEG_LIB_VERSION >= 70
#warning using JPEG_LIB_VERSION >= 70
    cinfo.do_fancy_downsampling = FALSE;  // Fix segfault with v7
#endif
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 2;
    cinfo.comp_info[1].h_samp_factor = 1;
    cinfo.comp_info[1].v_samp_factor = 1;
    cinfo.comp_info[2].h_samp_factor = 1;
    cinfo.comp_info[2].v_samp_factor = 1;

    jpeg_set_quality(&cinfo, quality, TRUE);
    cinfo.dct_method = JDCT_FASTEST;

    // 以二进制的方式打开jpeg图像要存放的文件
	if ((fp = fopen(filename, "wb")) == NULL)
	{
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}

    jpeg_stdio_dest(&cinfo, fp);        // Data written to file
    jpeg_start_compress(&cinfo, TRUE);

    for (j = 0; j < height; j += 16) {
        for (i = 0; i < 16; i++) {
            y[i] = image + width * (i + j);
            if (i % 2 == 0) {
                cb[i / 2] = image + width * height + width / 2 * ((i + j) / 2);
                cr[i / 2] = image + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
            }
        }
        jpeg_write_raw_data(&cinfo, data, 16);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return TRUE;
}



int Y2J(const char *yuv_file_name, const char *jpeg_file_name)
{
	unsigned char *pYUVBuf = NULL;

	pYUVBuf = (unsigned char *)malloc(YUV_SIZE);

	if(pYUVBuf == NULL)
	{
		printf("malloc for pYUVBuf failed!\n");
		return FALSE;
	}

	if(ReadYUVFile(yuv_file_name, pYUVBuf) == TRUE)
	{
		//if(write_JPEG_file(JPEG_NAME, pYUVBuf, 20, 352, 240) == FALSE)
		if(convert_yuv420p_to_jpeg_file(jpeg_file_name, pYUVBuf, 352, 288, 100) == FALSE)
		{
			printf("write to jpeg failed!\n");
		}
		else
		{
			printf("write jpeg file succeeded!\n");
		}

		if(pYUVBuf)
		{
			free(pYUVBuf);
		}
	}

	return TRUE;
}

#endif
