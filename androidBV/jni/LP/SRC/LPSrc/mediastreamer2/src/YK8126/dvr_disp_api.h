#ifndef __DVR_DISP_API_H__
#define __DVR_DISP_API_H__

#include <linux/ioctl.h>

#include "dvr_type_define.h"

#define DVR_PLANE_ID(d,p)   ((d<<8)|p)
#define GET_DISP_NUM_FROM_PLANE_ID(h)     (h>>8)
#define GET_PLANE_NUM_FROM_PLANE_ID(h)    (0xFF&h)


enum dvr_disp_type {
    DISP_TYPE_LIVEVIEW,
    DISP_TYPE_CASCADE,
    DISP_TYPE_PLAYBACK,
    DISP_TYPE_ON_BUFFER,
};

/* TODO: the following should be replaced by including capture header file.... */
#define VIDEO_MODE_PAL      0
#define VIDEO_MODE_NTSC     1
#define VIDEO_MODE_SECAM    2
#define VIDEO_MODE_AUTO     3
/**/


//### Declaration for DVR_DISP_GET_DISP_PARAM and DVR_DISP_SET_DISP_PARAM ###
typedef struct dvr_disp_color_attribute_tag {
    int brightness;
    int saturation;
    int contrast;
    int huesin;
    int huecos;
    int sharpnessk0;
    int sharpnessk1;
    int sharpness_thres0;
    int shaprness_thres1;
	int reserved[2];
}dvr_disp_color_attribute;
typedef struct dvr_disp_color_key_tag {
    int is_enable;
    int color;
	int reserved[2];
}dvr_disp_color_key;
typedef enum dvr_disp_plane_combination_tag {
    BG_ONLY         =0,   // only backgraound
    BG_AND_1PLANE   =1,   // background and another plane
    BG_AND_2PLANE   =2    // background and another 2 planes
}dvr_disp_plane_combination;
typedef struct dvr_disp_vbi_info_tag {
    int filler;     //0:driver do it,  1:ap can set values for it(need to fill the following fields)
    int lineno;
    int lineheight;
    int fb_offset;
    int fb_size;
	int reserved[2];
}dvr_disp_vbi_info;
typedef struct dvr_disp_scaler_info_tag {
    int is_enable;
    DIM dim;        // real output dimention
	int reserved[2];
}dvr_disp_scaler_info;
typedef struct dvr_disp_resolution_tag {
    int input_res;
    int output_type;
	int reserved[2];
}dvr_disp_resolution;


typedef struct dvr_disp_disp_param_tag {
    // input(write only)
    int disp_num;

    // input or ourput
    int target_id;      // where to output.  1:LCD1, 2:LCD2
    int plane_comb;     // the number of active plane for this LCD
    int output_system;  // DVR_VIDEO_SYSTEM_NTSC or DVR_VIDEO_SYSTEM_PAL
    int output_mode;    // DVR_VIDEIO_MODE_PROGRESSIVE or DVR_VIDEIO_MODE_INTERLACING
    dvr_disp_color_attribute    color_attrib;
    dvr_disp_color_key transparent_color[2];
    dvr_disp_vbi_info   vbi_info;
    dvr_disp_scaler_info    scl_info;

    //output(read only)
    DIM dim;            // default background dimension
    dvr_disp_resolution res;
	int reserved[8];
}dvr_disp_disp_param;



//### Declaration for DVR_DISP_UPDATE_DISP_PARAM ###
typedef enum dvr_disp_disp_param_name_tag {
    DISP_PARAM_TARGET,
    DISP_PARAM_PLANE_COMBINATION,
    DISP_PARAM_OUTPUT_MODE,
    DISP_PARAM_OUTPUT_SYSTEM,
    DISP_PARAM_COLOR_ATTRIBUTE,
    DISP_PARAM_TRANSPARENT_COLOR,
    DISP_PARAM_RESOLUTION,
    DISP_PARAM_APPLY        /* Use this to apply all parameters. */
}dvr_disp_disp_param_name; 


typedef struct dvr_disp_update_disp_param_tag {
    // input
    int disp_num;
    dvr_disp_disp_param_name  param;
    // output
    struct val_t {
        int target_id;
        int plane_comb;
        int output_system;
        int output_mode;
        int display_rate;
        dvr_disp_color_attribute    color_attrib;
        dvr_disp_color_key transparent_color[2];
        dvr_disp_vbi_info   vbi_info;
        dvr_disp_scaler_info    scl_info;
        dvr_disp_resolution res;
		int	reserved[8];
    }val;
}dvr_disp_update_disp_param;



//### Declaration for DVR_DISP_GET_PLANE_PARAM and DVR_DISP_SET_PLANE_PARAM ###
typedef struct dvr_disp_plane_param_tag {
    int   plane_id;
    RECT  win;
    int   data_mode;              // input data(for display) is DVR_VIDEIO_MODE_PROGRESSIVE or DVR_VIDEIO_MODE_INTERLACING
    int   color_mode;
	int   reserved[2];
}dvr_disp_plane_param_st;
typedef struct dvr_disp_plane_param_st_tag {
    //input
    int disp_num;
    int plane_num;
    //input/output
    dvr_disp_plane_param_st  param;
	int reserved[2];
}dvr_disp_plane_param;


//### Declaration for DVR_DISP_UPDATE_PLANE_PARAM ###
typedef enum dvr_disp_plane_param_name_tag {
    PLANE_PARAM_COLOR_MODE,
    PLANE_PARAM_WINDOW,
    PLANE_PARAM_DATA_MODE,
    PLANE_PARAM_APPLY
}dvr_disp_plane_param_name;
typedef struct dvr_disp_update_plane_param_tag {
    //input
    int   plane_id;
    dvr_disp_plane_param_name   param;
	int reserved[8];
    union {
        RECT  win;
        int   data_mode;
        int   color_mode;
    }val;
	int reserved1[8];
}dvr_disp_update_plane_param;


//### Declaration for DVR_DISP_CONTROL ###
typedef enum dvr_disp_ctrl_cmd_tag {
    DISP_START,
    DISP_STOP,
    DISP_UPDATE,
    DISP_RUN,
}dvr_disp_ctrl_cmd;

#define DVR_PARAM_MAGIC 0x1688
#define DVR_PARAM_MAGIC_SHIFT   16
#define CAP_BUF_ID(id)  ((DVR_PARAM_MAGIC << DVR_PARAM_MAGIC_SHIFT)|(id))

typedef struct dvr_disp_control_tag {
    //input
    int   type;
    int   channel;
    dvr_disp_ctrl_cmd   command;

    int   reserved[8];
    
    struct
    {
        struct {
            int   cap_path;
            int   di_mode;          // 0:even-odd, 1:enlarge even, 2: weaved of two-fields, 3:3D deinterlacing
            int   mode;
            int   dma_order;        // DMA_PACKET, DMA_3PLANAR_MB, or DMA_2PLANAR_MB
            int   scale_indep;      // 0:fixed aspect-ratio
            int   input_system;     // VIDEO_MODE_NTSC or VIDEO_MODE_PAL
            int   cap_rate;
            int   color_mode;
            int   is_use_scaler;    //if TRUE, capture as "src_para.lv.dim" and scale it to "des_param.lv.win"
                                    //if FALSE, capture as "des_param.lv.win" and display it directly
            DIM   dim;
            RECT  win;              //reserved for ROI
            video_process vp_param;
            ScalerParam scl_param;  //if is_use_scaler is TRUE, user needs to fill this structure
            int   cap_buf_id;       
            int   reserved[4];
        }lv;
        struct {
            RECT  win;
            int   rate;
            int   di_mode;          // 0:even-odd, 1:enlarge even, 2: weaved of two-fields, 3:3D deinterlacing
            int   mode;
            int   dma_order;        // DMA_PACKET, DMA_3PLANAR_MB, or DMA_2PLANAR_MB
            int   scale_indep;      // 0:fixed aspect-ratio
            int   input_system;     // VIDEO_MODE_NTSC or VIDEO_MODE_PAL 
            int   cap_rate;
            int   color_mode;
            int   is_use_scaler;    //if TRUE, capture as "src_para.lv.dim" and scale it to "des_param.lv.win"
                                    //if FALSE, capture as "des_param.lv.win" and display it directly
            DIM   dim;              
            video_process vp_param;
            ScalerParam scl_param;  // if is_use_scaler is TRUE, user needs to fill this structure
            int   reserved[5];
        }cas;        
        int   reserved[5];
    }src_param;

    struct 
    {
        struct {
            RECT  win;
            int   plane_id;
            int   reserved[5];
        }lv;

        struct {
    		int   path;
            RECT  win;
            RECT  rect0;
            RECT  rect1;
            RECT  swc_rect0;
            RECT  swc_rect1;
            RECT  bg_rect0;
            RECT  bg_rect1;
            int   plane_id;
            int   reserved[5];
        }cas;
        int   reserved[5];
    }dst_param;
}dvr_disp_control;


typedef struct dvr_disp_clear_param_tag
{
  int           plane_id;
  RECT          win;
  unsigned int  pattern;
  int 			reserved[2];
}dvr_disp_clear_param;


#define DVR_DISP_IOC_MAGIC  'I'
#define DVR_DISP_INITIATE           _IO(DVR_DISP_IOC_MAGIC, 1)
#define DVR_DISP_TERMINATE          _IO(DVR_DISP_IOC_MAGIC, 2)
#define DVR_DISP_GET_DISP_PARAM     _IOWR(DVR_DISP_IOC_MAGIC, 4, dvr_disp_disp_param)
#define DVR_DISP_SET_DISP_PARAM     _IOWR(DVR_DISP_IOC_MAGIC, 5, dvr_disp_disp_param)
#define DVR_DISP_UPDATE_DISP_PARAM  _IOWR(DVR_DISP_IOC_MAGIC, 6, dvr_disp_update_disp_param)
#define DVR_DISP_GET_PLANE_PARAM    _IOWR(DVR_DISP_IOC_MAGIC, 7, dvr_disp_plane_param)
#define DVR_DISP_SET_PLANE_PARAM    _IOWR(DVR_DISP_IOC_MAGIC, 8, dvr_disp_plane_param)
#define DVR_DISP_UPDATE_PLANE_PARAM _IOWR(DVR_DISP_IOC_MAGIC, 9, dvr_disp_update_plane_param) 
#define DVR_DISP_CONTROL            _IOR(DVR_DISP_IOC_MAGIC, 10, dvr_disp_control)
#define DVR_DISP_CLEAR_WIN          _IOWR(DVR_DISP_IOC_MAGIC, 11, dvr_disp_clear_param) 



#endif /* __DVR_DISP_API_H__ */


