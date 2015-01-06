#ifndef __DVR_ENC_API_H__
#define __DVR_ENC_API_H__

#include <linux/ioctl.h>
#include "dvr_type_define.h"
#include "dvr_enc_ioctl.h"

#define     ENC_TYPE_FROM_CAPTURE       0
#define     ENC_TYPE_FROM_CASCADE       1
#define     ENC_TYPE_FROM_BUFFER        2
#define     ENC_SRC_TYPE_COUNT          3

/**
 * @brief no longer used, replace by EncParam_Ext3
 */
typedef struct  EncParam_Ext1_tag{
    unsigned int feature_enable;
    unsigned int target_rate_max;
    unsigned int reaction_delay_max;
}EncParam_Ext1;

/**
 * @brief no longer used, replace by EncParam_Ext3
 */
typedef struct  EncParam_Ext2_tag{
    unsigned int feature_enable;
    unsigned int target_rate_max;
    unsigned int reaction_delay_max;
    int          enc_type;  
    int          MJ_quality;   
}EncParam_Ext2;

/**
 * @brief encode parameter extension
 */
typedef struct  EncParam_Ext3_tag{
    /*! #DVR_ENC_ENHANCE_H264_RATECONTROL   */     
    /*! #DVR_ENC_MJPEG_FUNCTION             */
    /*! #DVR_ENC_H264_WATERMARK             */     
    unsigned int    feature_enable;    ///< this is a bit combination. If you want to involve watermark(Ë®Ó¡), then feature_enable |= DVR_ENC_H264_WATERMARK.
    /*! ECBR: usually stays at target bitrate,  */
    /*! produce large bitrate when big motion, but never more than max bitrate */
    unsigned int    target_rate_max;
    /*! max frame count will the rate-control stay when away from the target bitrate, only valid at ECBR */
    unsigned int    reaction_delay_max;    
    /*! #ENC_TYPE_H264           */
    /*! #ENC_TYPE_MPEG           */
    /*! #ENC_TYPE_MJPEG          */
    /*! #ENC_TYPE_COUNT          */
    int             enc_type;  
    /*! 1(worst) ~100(best)     */
    int             MJ_quality;    ///< Motion JPEG quality    
    /*! Indicate watermark enable or not                        */
    /*! if 0: disable,                                          */
    /*! if 1: enable, but only insert watermark at intra-mb,    */
    /*! if 3: enable, and insert warermark at intra/inter-mb,   */
    /*! if others: not allowed  */                        
    int             watermark_enable;
    /*! specify the interval, to insert watermark each "interval" frame,    */
    /*! valid when watermark function is enabled,                           */
    /*! if 1: will be inserted watermark each frames,                       */
    /*! if N: will be inserted watermark each N frames                      */
    int             watermark_interval;
    /*! The initial pattern for watermark           */
    /*! valid when watermark function is enabled    */  
    int             watermark_init_pattern;
    /*! specify the interval, to reinit with init_pattern each "watermark"ed frames     */
    /*! valid when watermark function is enabled                                        */
    /*! if 1: every "watermark"ed frame with init_pattern                               */
    /*! if N: initial with init_pattern each N "watermark"ed frame                      */
    int             watermark_init_interval;    
}EncParam_Ext3;

/**
 * @brief encode parameter 
 */
typedef struct EncParam_tag {
    /*! #ENC_TYPE_H264       */
    /*! #ENC_TYPE_MPEG       */
    /*! #ENC_TYPE_MJPEG      */
    /*! #ENC_TYPE_COUNT      */
    int     input_type;
    /*! frame rate per second   */
    int     frame_rate;
    /*! bit rate per second     */
    int     bit_rate;
    /*! The I-P interval frames */
    int     ip_interval;
    /*! The initial quant value */
    int     init_quant;
    /*! The max/min quant value */
    int     max_quant, min_quant;
    /*! Enable the ROI function */
    int     is_use_ROI;
    /*! #RECT_tag               */
    RECT    ROI_win;
    /*! encode parameter extension size  */
    unsigned int ext_size;
    /*! point to encode parameter extension structure  */
    void    *pext_data;
    int     reserved[6];
} EncParam;

/**
 * @brief snapshot parameter 
 */
typedef struct snapshot_param_tag {
    /*! #JCS_yuv420     */
    /*! #JCS_yuv422     */
    /*! #JCS_yuv211     */
    /*! #JCS_yuv333     */
    /*! #JCS_yuv222     */
    /*! #JCS_yuv111     */
    /*! #JCS_yuv400     */
    int sample;
    /*! not in use */
    int RestartInterval;
    /*! #JENC_INPUT_MP42D    */
    /*! #JENC_INPUT_1D420    */
    /*! #JENC_INPUT_H2642D   */
    int u82D;
    /*! 1(worst) ~100(best) */
    int quality;   
    int reserved[8];
} snapshot_param;

/**
 * @brief dvr bit stream parameter, include main, sub1, sub2 bit-stream.
 */
typedef struct ReproduceBitStream_tag {
    /*! #DVR_ENC_EBST_ENABLE: enabled  */  
    /*! #DVR_ENC_EBST_DISABLE: disabled  */
    int enabled;    
    /*! 0: main-bitstream  */
    /*! 1: sub-bitstream1  */
    /*! 2: sub-bitstream2  */
    int out_bs;     
    /*! #ENC_TYPE_H264       */
    /*! #ENC_TYPE_MPEG       */
    /*! #ENC_TYPE_MJPEG      */
    /*! #ENC_TYPE_COUNT      */
    int enc_type;   
    /*!  indicate all system-call for this channel is "blocked" or "non-block" type  */
    int is_blocked;
    /*! #DVR_ENC_EBST_ENABLE: enabled  */
    /*! #DVR_ENC_EBST_DISABLE: disabled  */
    int en_snapshot;    ///< enable/disable snapshot 
    /*! #DIM_tag    */
    DIM             dim; //±àÂë·Ö±æÂÊ
    /*! #EncParam_tag   */    
    EncParam        enc;
    /*! #ScalerParamtag */
    ScalerParam     scl;
    /*! #snapshot_param_tag */
    snapshot_param  snap;
    int reserved[8];
} ReproduceBitStream;

/**
 * @brief dvr encode source parameter
 */
typedef struct dvr_enc_src_tag {
    /*!   channle number  0~7   */
    int     channel;
    /*! #ENC_TYPE_FROM_CAPTURE      */      
    /*! #ENC_TYPE_FROM_CASCADE      */    
    /*! #ENC_TYPE_FROM_BUFFER       */  
    /*! #ENC_SRC_TYPE_COUNT         */
    int     enc_src_type;
    /*! #DIM_tag    */
    DIM     dim;
    /*! #LVFRAME_EVEN_ODD               */      
    /*! #LVFRAME_ENLARGE_ONE_FIELD      */
    /*! #LVFRAME_WEAVED_TWO_FIELDS      */
    /*! #LVFRAME_GM3DI_FORMAT           */
    int     di_mode;
    /*! #LVFRAME_FRAME_MODE       */
    /*! #LVFRAME_FIELD_MODE       */
    /*! #LVFRAME_FIELD_MODE2      */ 
    int     mode;
    /*! #DMAORDER_PACKET      */
    /*! #DMAORDER_3PLANAR     */
    /*! #DMAORDER_2PLANAR     */
    int     dma_order;
    /*! #CAPSCALER_KEEP_RATIO        */        
    /*! #CAPSCALER_NOT_KEEP_RATIO    */    
    int     scale_indep;
    /*! #MCP_VIDEO_NTSC  */
    /*! #MCP_VIDEO_PAL   */
    /*! #MCP_VIDEO_VGA   */
    int     input_system;
    /*! #CAPCOLOR_RGB888      */
    /*! #CAPCOLOR_RGB565      */
    /*! #CAPCOLOR_YUV422      */
    /*! #CAPCOLOR_YUV420_M0   */
    /*! #CAPCOLOR_YUV420_M1   */
    int     color_mode;
    /*! #video_process_tag  */
    video_process vp_param;
    int reserved[8];
} dvr_enc_src_param;

/**
 * @brief dvr encode channel parameter.
 */
typedef struct dvr_enc_channel_param_tag {
    dvr_enc_src_param src;
    ReproduceBitStream main_bs;
}dvr_enc_channel_param;

typedef enum dvr_enc_channel_param_name_tag {
    ENC_PARAM_SRC_DIM,
    ENC_PARAM_DST_WIN
}dvr_enc_channel_param_name;

/**
 * @brief get dvr encode buffer 
 */
typedef struct dvr_enc_queue_get_tag {
    dvr_bs_data bs;
    int new_bs;     ///< To indicate whether the packet using new setting
    int mb_len;     ///< Only for debug H.264: D1:103680  HD1:50688  CIF:25344
    int channel;    ///< Only for debug H.264: sub ==> bs.stream
}dvr_enc_queue_get;

#define    ENC_START    0
#define    ENC_STOP     1
#define    ENC_SNAP     2
#define    ENC_UPDATE   3

/**
 * @brief update dvr encode channel parameter
 */
typedef struct dvr_enc_update_channel_param_tag {
  struct
  { 
    /*! #LVFRAME_EVEN_ODD               */      
    /*! #LVFRAME_ENLARGE_ONE_FIELD      */
    /*! #LVFRAME_WEAVED_TWO_FIELDS      */
    /*! #LVFRAME_GM3DI_FORMAT           */
    int     di_mode;
    /*! #LVFRAME_FRAME_MODE       */
    /*! #LVFRAME_FIELD_MODE       */
    /*! #LVFRAME_FIELD_MODE2      */ 
    int     mode;
    /*! #CAPSCALER_KEEP_RATIO        */        
    /*! #CAPSCALER_NOT_KEEP_RATIO    */    
    int     scale_indep;
    /*! TRUE : enable 3D deinterlace , False : disable 3D deinterlace   */
    int     is_3DI;
    /*! TRUE :  enable 3D denoise , False : disable 3D denoise  */
    int     is_denoise;
    int     reserved[8];
  } src;
    int     stream_enable;  ///< 0:disable, 1:enable
    int     frame_rate;     ///< frame rate per second
    int     bit_rate;       ///< Bitrate per second
    int     ip_interval;    ///< The I-P interval frames
    /*! #DIM_tag    */
    DIM     dim;    
    int     init_quant;     ///< The initial quant value
    int     max_quant;      ///< The max quant value
    int     min_quant;      ///< The min quant value
    /*! encode parameter extension size  */        
    unsigned int ext_size;
    /*! point to encode parameter extension structure  */
    void    *pext_data;
    int     reserved[8];
} dvr_enc_update_channel_param;

/**
 * @brief dvr encode control parameter
 */
typedef struct dvr_enc_control_tag {
    int     command;        ///< enc control command , such as ENC_START, ENC_STOP, etc.
    union {
        int *count;
        int *repd_bs_num;   ///< 0: all, 1: SUB1, 2: SUB2.
    }output;
    int         stream;     ///< stream number 0 : main, 1 : sub stream 1, 2 : sub stream 2
    /*! #dvr_enc_update_channel_param_tag   */
    dvr_enc_update_channel_param    update_parm;
    int reserved[8];
}dvr_enc_control;

#define POLLIN_MAIN_BS     0x0001
#define POLLIN_SUB1_BS     (POLLIN_MAIN_BS << 1)
#define POLLIN_SUB2_BS     (POLLIN_MAIN_BS << 2)
#define POLLIN_SUB3_BS     (POLLIN_MAIN_BS << 3)
#define POLLIN_SUB4_BS     (POLLIN_MAIN_BS << 4)
#define POLLIN_SUB5_BS     (POLLIN_MAIN_BS << 5)
#define POLLIN_SUB6_BS     (POLLIN_MAIN_BS << 6)
#define POLLIN_SUB7_BS     (POLLIN_MAIN_BS << 7)
#define POLLIN_SUB8_BS     (POLLIN_MAIN_BS << 8)
#define POLLIN_SNAP_BS     (1 << DVR_ENC_REPD_BT_NUM)

#define DVR_ENC_MAGIC      0x1689
#define DVR_ENC_MAGIC_SHIFT   16
#define DVR_ENC_MAGIC_ADD_VAL(val)  ((DVR_ENC_MAGIC << DVR_ENC_MAGIC_SHIFT)|(val))

#define DVR_ENC_CHECK_MAGIC(v)  (((v)>>DVR_ENC_MAGIC_SHIFT)==DVR_ENC_MAGIC)
#define DVR_ENC_GET_VALUE(v)    ((v)&((1<<DVR_ENC_MAGIC_SHIFT)-1))
#define DVR_ENC_ENHANCE_H264_RATECONTROL 1
#define DVR_ENC_MJPEG_FUNCTION           (1 << 1)
#define DVR_ENC_H264_WATERMARK           (1 << 2)

#define DVR_ENC_EBST_ENABLE     0x55887799
#define DVR_ENC_EBST_DISABLE    0x0
#endif /* __DVR_ENC_API_H__ */


