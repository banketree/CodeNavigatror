#ifndef __DVR_TYPE_DEFINE_H__
#define __DVR_TYPE_DEFINE_H__

#include "dvr_scenario_define.h"

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#define GMDVR_MEM_CFG_FILE  "/mnt/mtd/gmdvr_mem.cfg"
#define GMDVR_MAKE_FOURCC(a,b,c,d)         (int)((a)|(b)<<8|(c)<<16|(d)<<24)

#define GMVAL_DO_NOT_CARE       (-54172099)
#define GMVAL_RATE(val,base)    ((base<<16)|val)

#define GMVAL_RT_GET_VAL(rate)  (rate&0x0000FFFF)
#define GMVAL_RT_GET_BASE(rate) (rate>>16)

//Graph ID
#define GFID_DISP(plane_num)(0x10000+plane_num)
#define GFID_ENC(ch_num)    (0x20000+ch_num)
#define GFID_DEC(ch_num)    (0x30000+ch_num)

/**
 * @brief function tag for videograph level
 */
typedef struct FuncTag_tag{
    //bit24~27 is reserved
    int func : 28;          ///< functional tag
    int cas_ch : 4;         ///< cascade channel
    int lv_ch;              ///< liveview channel
    int rec_ch;             ///< record channel
    int pb_ch;              ///< playback channel
}FuncTag;
/*
                 func cas_ch                  lv_ch                 rec_ch                  pb_ch
                    |      |                      |                      |                      |
  xxxx xxxx xxxx   xx     xx    xxxx xxxx xxxx xxxx    xxxx xxxx xxxx xxxx    xxxx xxxx xxxx xxxx
*/
#define MTHD_USE_CMP        0x1

#define FN_NONE             0x0
#define FN_LIVEVIEW         0x1
#define FN_RECORD           0x2
#define FN_PLAYBACK         0x4
#define FN_CASCADE          0x8
#define FN_LCD_PARAM        0x10
#define FN_PLANE_PARAM      0x20
#define FN_SUB1_RECORD      0x40
#define FN_SUB2_RECORD      0x80
#define FN_SUB3_RECORD      0x100
#define FN_SUB4_RECORD      0x200
#define FN_SUB5_RECORD      0x400
#define FN_SUB6_RECORD      0x800
#define FN_SUB7_RECORD      0x1000
#define FN_SUB8_RECORD      0x2000
#define FN_UPDATE_METHOD    0x10000
#define FN_PB_SCL_LINK      0x20000
#define FN_METHOD_USE_CMP   0x8000000
#define FN_SUB_ALL_RECORD   (FN_RECORD|FN_SUB1_RECORD|FN_SUB2_RECORD|FN_SUB3_RECORD|FN_SUB4_RECORD|FN_SUB5_RECORD|FN_SUB6_RECORD|FN_SUB7_RECORD|FN_SUB8_RECORD)


#define FN_RESET_TAG(ptag)           {   (ptag)->func=0; (ptag)->lv_ch=0; (ptag)->rec_ch=0; (ptag)->pb_ch=0; (ptag)->cas_ch=0; }
#define FN_COPY_TAG(newt,ptag)       {   (newt)->func=(ptag)->func; (newt)->lv_ch=(ptag)->lv_ch; (newt)->rec_ch=(ptag)->rec_ch; (newt)->pb_ch=(ptag)->pb_ch; (newt)->cas_ch=(ptag)->cas_ch; }
#define FN_SET_LV_CH(ptag, ch_num)   {   (ptag)->func|=FN_LIVEVIEW;   (ptag)->lv_ch|=(0x1<<ch_num);  }
#define FN_SET_REC_CH(ptag, ch_num)  {   (ptag)->func|=(FN_RECORD); (ptag)->rec_ch|=(0x1<<ch_num); }
#define FN_SET_SUB1_REC_CH(ptag, ch_num)  {   (ptag)->func|=(FN_SUB1_RECORD); (ptag)->rec_ch|=(0x1<<ch_num); }
#define FN_SET_SUB2_REC_CH(ptag, ch_num)  {   (ptag)->func|=(FN_SUB2_RECORD); (ptag)->rec_ch|=(0x1<<ch_num); }
#define FN_SET_SUB3_REC_CH(ptag, ch_num)  {   (ptag)->func|=(FN_SUB3_RECORD); (ptag)->rec_ch|=(0x1<<ch_num); }
#define FN_SET_SUB4_REC_CH(ptag, ch_num)  {   (ptag)->func|=(FN_SUB4_RECORD); (ptag)->rec_ch|=(0x1<<ch_num); }
#define FN_SET_SUB5_REC_CH(ptag, ch_num)  {   (ptag)->func|=(FN_SUB5_RECORD); (ptag)->rec_ch|=(0x1<<ch_num); }
#define FN_SET_SUB6_REC_CH(ptag, ch_num)  {   (ptag)->func|=(FN_SUB6_RECORD); (ptag)->rec_ch|=(0x1<<ch_num); }
#define FN_SET_SUB7_REC_CH(ptag, ch_num)  {   (ptag)->func|=(FN_SUB7_RECORD); (ptag)->rec_ch|=(0x1<<ch_num); }
#define FN_SET_SUB8_REC_CH(ptag, ch_num)  {   (ptag)->func|=(FN_SUB8_RECORD); (ptag)->rec_ch|=(0x1<<ch_num); }
#define FN_SET_PB_CH(ptag, ch_num)   {   (ptag)->func|=FN_PLAYBACK;   (ptag)->pb_ch|=(0x1<<ch_num);  }
#define FN_SET_CAS_CH(ptag, ch_num)  {   (ptag)->func|=FN_CASCADE;    (ptag)->cas_ch|=(0x1<<ch_num);  }
#define FN_SET_FUNC(ptag, fnc)       {   (ptag)->func|=fnc;        }
#define FN_REMOVE_FUNC(ptag, fnc)    {   (ptag)->func&=(~fnc);     }
#define FN_SET_ALL(ptag)             {   (ptag)->func=(0xFF);   /* Note: 'func' value doesn't include FN_UPDATE_METHOD. */ \
                                           (ptag)->lv_ch=0xFFFFFFFF; (ptag)->rec_ch=0xFFFFFFFF; (ptag)->pb_ch=0xFFFFFFFF; (ptag)->cas_ch=0xF; }
#define FN_IS_UPDATE(ptag)           ((ptag)->func&FN_UPDATE_METHOD)
#define FN_COMPARE(ptagA, ptagB)     ( ((ptagA)->func==(ptagB)->func) && ((ptagA)->lv_ch==(ptagB)->lv_ch) && ((ptagA)->rec_ch==(ptagB)->rec_ch) && ((ptagA)->pb_ch==(ptagB)->pb_ch) && ((ptagA)->cas_ch==(ptagB)->cas_ch) )
#define FN_IS_EMPTY(ptag)            ( ((ptag)->func==0) && ((ptag)->lv_ch==0) && ((ptag)->rec_ch==0) && ((ptag)->pb_ch==0) && ((ptag)->cas_ch==0) )
#define FN_IS_FN(ptag, fn)           ( (ptag)->func&(fn) )

/* FN_CHECK_MASK uses AND(&) operation on ptagA with ptagB, and check channel values for LV,REC,PB,CAS */
#if 1
#define FN_CHECK_MASK(ptagA,ptagB) \
        ((((ptagA)->func&(ptagB)->func)==FN_LIVEVIEW)?((ptagA)->lv_ch&(ptagB)->lv_ch):              \
            ((((ptagA)->func&(ptagB)->func)==FN_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch):          \
                ((((ptagA)->func&(ptagB)->func)==FN_SUB1_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                    ((((ptagA)->func&(ptagB)->func)==FN_SUB2_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                    ((((ptagA)->func&(ptagB)->func)==FN_SUB3_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                    ((((ptagA)->func&(ptagB)->func)==FN_SUB4_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                    ((((ptagA)->func&(ptagB)->func)==FN_SUB5_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                    ((((ptagA)->func&(ptagB)->func)==FN_SUB6_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                    ((((ptagA)->func&(ptagB)->func)==FN_SUB7_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                    ((((ptagA)->func&(ptagB)->func)==FN_SUB8_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                        ((((ptagA)->func&(ptagB)->func)==FN_PLAYBACK)?((ptagA)->pb_ch&(ptagB)->pb_ch):  \
                            ((((ptagA)->func&(ptagB)->func)==FN_CASCADE)?((ptagA)->cas_ch&(ptagB)->cas_ch): \
                        ((ptagA)->func&(ptagB)->func) ))))))))))))
#else
#define FN_CHECK_MASK(ptagA,ptagB) \
        ((((ptagA)->func&(ptagB)->func)==FN_LIVEVIEW)?((ptagA)->lv_ch&(ptagB)->lv_ch):              \
            ((((ptagA)->func&(ptagB)->func)==FN_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch):          \
                ((((ptagA)->func&(ptagB)->func)==FN_SUB1_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                    ((((ptagA)->func&(ptagB)->func)==FN_SUB2_RECORD)?((ptagA)->rec_ch&(ptagB)->rec_ch): \
                        ((((ptagA)->func&(ptagB)->func)==FN_PLAYBACK)?((ptagA)->pb_ch&(ptagB)->pb_ch):  \
                            ((((ptagA)->func&(ptagB)->func)==FN_CASCADE)?((ptagA)->cas_ch&(ptagB)->cas_ch): \
                        ((ptagA)->func&(ptagB)->func) ))))))
#endif
#define FN_ITEMS(ptag)               (ptag)->func, (ptag)->lv_ch, (ptag)->rec_ch, (ptag)->pb_ch, (ptag)->cas_ch

/* Queue name. 
   Note => the length of queue must be smaller than MAX_NAME_SIZE. */
#define QNAME_LCD           "lcd"
#define QNAME_3DI_SCL       "3di_scl"
#define QNAME_LV_SCL        "lv_scl"
#define QNAME_ENC_IN        "enc_in"
#define QNAME_ENC_OUT       "enc_out"
#define QNAME_SS_ENC_IN     "ssenc_in"
#define QNAME_SS_ENC_OUT    "ssenc_out"
#define QNAME_SUB1_ENC_IN   "sub1enc_in"
#define QNAME_SUB1_ENC_OUT  "sub1enc_out"
#define QNAME_SUB2_ENC_IN   "sub2enc_in"
#define QNAME_SUB2_ENC_OUT  "sub2enc_out"
#define QNAME_DEC_IN        "dec_in"
#define QNAME_PB_SCL        "pb_scl"
#if TWO_STAGES_SCALER
#define QNAME_PB_SCL2       "pb_scl2"
#endif

typedef enum QueueID_tag {
    QID_LCD=10,
    QID_3DI_SCL,
    QID_LV_SCL,
    QID_ENC_IN,
    QID_ENC_OUT,
    QID_SS_ENC_IN,
    QID_SS_ENC_OUT,
    QID_SUB1_ENC_IN,
    QID_SUB1_ENC_OUT,
    QID_SUB2_ENC_IN,
    QID_SUB2_ENC_OUT,
    QID_DEC_IN,
    QID_PB_SCL
}QueueID;

/**
 * @brief set width and height
 */
typedef struct DIM_tag {
    int  width;     ///< width in pixel
    int  height;    ///< height in pixel
}DIM;

/**
 * @brief set size and position
 */
typedef struct RECT_tag {
    int  x;             ///< x position
    int  y;             ///< y positon
    int  width;         ///< width in pixel
    int  height;        ///< height in pixel
}RECT;

/**
 * @brief set queue memory configuration
 */
typedef struct QueueMemConfig_tag {
    int  size;          ///< size of buffer
    int  count;         ///< the number of buffers 
    int  ddr_num;       ///< the DDR number
    int  limit_count;   ///< Max count of in-used buffer
}QueMemCfg;

/**
 * @brief set dvr graph queue configuration
 */
typedef struct dvr_graph_vqueuet_tag
{
    struct v_queue_t    *que;
    int size;               ///< size of buffer
    int count;              ///< the number of buffers
    int ddr;                ///< the DDR number
    int limit_count;        ///< Max count of in-used buffer
    FuncTag user_tag;       ///< function tag for videograph level
}dvr_graph_vqueuet;

/**
 * @brief dvr bit-stream data
 */
typedef struct dvr_bs_data_tag {
    struct timeval timestamp;
    int  offset;            ///< bitstream buffer offset. 
    int  length;            ///< bitstream buffer length.
    int  is_keyframe;       ///< 1: current frame is a keyframe. 0: current frame is not a keyframe.
    int  mv_offset;         ///< motion vector offset 运动向量
    int  mv_length;         ///< motion vector length

    //internal use
    void  *p_job;           ///< internal use
    int  stream;            ///< internal use
    /*! 0: current P frame was encoded as referenced. */
    /*! 1: current P frame was encoded as non-referenced */ 
    int NonRef;                          
    int reserved[8];
}dvr_bs_data;

typedef struct dvr_rate_tag {
    int numerator;      ///< Not in use  
    int denominator;    ///< Not in use      
    int reserved[4];    ///< Not in use  
}dvr_ratio;

/**
 * @brief dvr video parameter.
 */
typedef struct video_process_tag {
    /*! TRUE : enable 3D deinterlace , False : disable 3D deinterlace   */
    int  is_3DI;
    /*! TRUE :  enable 3D denoise , False : disable 3D denoise  */
    int  is_denoise;        
    /*!#GM3DI_FIELD      */
    /*!#GM3DI_FRAME      */
    int  denoise_mode;         
    int  reserved[4];
}video_process;

/**
 * @brief scalar parameter
 */
typedef struct ScalerParamtag {
    /*! #SCALE_RGB888              */
    /*! #SCALE_RGB565              */
    /*! #SCALE_H264_YUV420_MODE0   */
    /*! #SCALE_H264_YUV420_MODE1   */        
    /*! #SCALE_YUV444              */
    /*! #SCALE_YUV422              */
    /*! #SCALE_MP4_YUV420_MODE0    */
    /*! #SCALE_MP4_YUV420_MODE1    */
    int     src_fmt;        ///< source format 
    /*! #SCALE_RGB888              */
    /*! #SCALE_RGB565              */
    /*! #SCALE_H264_YUV420_MODE0   */
    /*! #SCALE_H264_YUV420_MODE1   */        
    /*! #SCALE_YUV444              */
    /*! #SCALE_YUV422              */
    /*! #SCALE_MP4_YUV420_MODE0    */
    /*! #SCALE_MP4_YUV420_MODE1    */
    int     dst_fmt;        ///< destination format     
    /*! #SCALE_NON_LINEAR           */
    /*! #SCALE_METHOD_COUNT        */
    int     scale_mode;
    int     is_dither;          ///< Not in use
    int     is_correction;      ///< Not in use
    int     is_album;           ///< Not in use
    int     des_level;          ///< Not in use    
    int     reserved[8];
} ScalerParam;

#define     ENC_TYPE_H264       0           ///< H264
#define     ENC_TYPE_MPEG       1           ///< MPEG4
#define     ENC_TYPE_MJPEG      2           ///< Motion JPEG
#define     ENC_TYPE_COUNT      3           ///< do not use


/* debug level */
#define     DBG_ENTITY_FNC          0x01
#define     DBG_ENTITY_JOB_FLOW     0x02
#define     DBG_DVR_FNC             0x04
#define     DBG_DVR_DATA_FLOW       0x08
#define     DBG_GRAPH_FNC           0x10
#define     DBG_GRAPH_DATA          0x20

#define     LCD_COLOR_YUV422    0
#define     LCD_COLOR_YUV420    1
#define     LCD_COLOR_RGB       16
#define     LCD_COLOR_ARGB      16
#define     LCD_COLOR_RGB888    2
#define     LCD_COLOR_RGB565    3
#define     LCD_COLOR_RGB555    4
#define     LCD_COLOR_RGB444    5
#define     LCD_COLOR_RGB8      6

#define     MCP_VIDEO_NTSC  0   ///< video mode : NTSC
#define     MCP_VIDEO_PAL   1   ///< video mode : PAL
#define     MCP_VIDEO_VGA   2   ///< video mode : VGA

#define     LCD_PROGRESSIVE     0    
#define     LCD_INTERLACING     1     

#define     LVFRAME_EVEN_ODD             0
#define     LVFRAME_ENLARGE_ONE_FIELD    1
#define     LVFRAME_WEAVED_TWO_FIELDS    2
#define     LVFRAME_GM3DI_FORMAT         3

#define     LVFRAME_FRAME_MODE       0
#define     LVFRAME_FIELD_MODE       1
#define     LVFRAME_FIELD_MODE2      2 

#define     GM3DI_FIELD         1
#define     GM3DI_FRAME         2

#define     DMAORDER_PACKET     0
#define     DMAORDER_3PLANAR    1
#define     DMAORDER_2PLANAR    2

#define     CAPSCALER_KEEP_RATIO         0
#define     CAPSCALER_NOT_KEEP_RATIO     1

#define     CAPCOLOR_RGB888     0
#define     CAPCOLOR_RGB565     1
#define     CAPCOLOR_YUV422     2
#define     CAPCOLOR_YUV420_M0  3
#define     CAPCOLOR_YUV420_M1  4

#define     ENC_INPUT_H2642D    0
#define     ENC_INPUT_MP42D     1
#define     ENC_INPUT_1D420     2
#define     ENC_INPUT_1D422     3

#define     DEC_OUTPUT_COLOR_YUV420      4
#define     DEC_OUTPUT_COLOR_YUV422      5

/*! Y HxV,  U HxV,  V HxV,  u82D = 0,   u82D=1,     u82D=2  */
#define     JCS_yuv420      0   ///<    2x2,        1x1,        1x1,        support,        support,    support
#define     JCS_yuv422      1   ///<    4x1,        2x1,        2x1,                            support 
#define     JCS_yuv211      2   ///<    2x1,        1x1,        1x1,                            support
#define     JCS_yuv333      3   ///<    3x1,        3x1,        3x1,                            support
#define     JCS_yuv222      4   ///<    2x1,        2x1,        2x1,                            support
#define     JCS_yuv111      5   ///<    1x1,        1x1,        1x1,                            support
#define     JCS_yuv400      6   ///<    1x1,        0x0,        0x0,        support,        support,    support                     

#define     JENC_INPUT_MP42D        0
#define     JENC_INPUT_1D420        1
#define     JENC_INPUT_H2642D       2
#define     JENC_INPUT_DMAWRP420    3
#define     JENC_INPUT_1D422        4

#define     SCALE_LINEAR            0
#define     SCALE_NON_LINEAR        1
#define     SCALE_METHOD_COUNT      2

#define     SCALE_RGB888                0
#define     SCALE_RGB565                1
#define     SCALE_H264_YUV420_MODE0     2
#define     SCALE_H264_YUV420_MODE1     3        
#define     SCALE_YUV444                4
#define     SCALE_YUV422                5
#define     SCALE_MP4_YUV420_MODE0      6
#define     SCALE_MP4_YUV420_MODE1      7

#define     CAPPATH_DEFAULT         0
#define     CAPPATH_PATH_1          1
#define     CAPPATH_PATH_2          2

#define     LCD_RES_D1              1       ///<    720x576 or 720x480 
#define     LCD_RES_SVGA            2       ///<    800x600 
#define     LCD_RES_XGA             3       ///<    1024x768 
#define     LCD_RES_XVGA            4       ///<    1280x960 
#define     LCD_RES_SXGA            5       ///<    1280x1024
#define     LCD_RES_1360x768        6       ///<    1360x768 
#define     LCD_RES_COUNT           7

#define     DEFAULT_D1_WIDTH        720
#define     DEFAULT_D1_HEIGHT       576
#define     DEFAULT_CIF_WIDTH       352 
#define     DEFAULT_CIF_HEIGHT      288 
#define     DEFAULT_QCIF_WIDTH      176 
#define     DEFAULT_QCIF_HEIGHT     144 

#endif /* __DVR_TYPE_DEFINE_H__ */





