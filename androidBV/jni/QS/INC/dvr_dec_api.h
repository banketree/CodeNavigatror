#ifndef __DVR_DEC_API_H__
#define __DVR_DEC_API_H__

#include <linux/ioctl.h>
#include "dvr_type_define.h"


typedef enum dvr_dec_dest_type_tag {
    DEC_TYPE_TO_DISPLAY=0,
    DEC_TYPE_TO_BUFFER,
    DEC_TYPE_COUNT
}dvr_dec_dest_type;

typedef struct DecParam_tag {
    int     output_type;
	int 	reserved[2];
} DecParam;

typedef struct dvr_dec_channel_param_tag {
    int   dec_type;
    int   dec_dest_type;
    int   channel;
    int   is_blocked;
    int   is_use_scaler;
    DecParam    dec_param;
    ScalerParam scl_param;
	int   reserved[8];
}dvr_dec_channel_param;


typedef struct dvr_dec_queue_get_tag {
    dvr_bs_data bs;
    int     channel;   //for debug only
	int 	reserved[1];
}dvr_dec_queue_get;


typedef enum dvr_dec_ctrl_cmd_tag {
    DEC_START,
    DEC_STOP,
    DEC_UPDATE
}dvr_dec_ctrl_cmd;

typedef struct dvr_dec_control_tag {
    dvr_dec_ctrl_cmd   command;
    struct
    {
        DIM     dim;
        RECT    win;
        int     bs_rate;    //bitstream frame rate
		int 	reserved[2];
    }src_param;
    struct 
    {
        int     plane_id;
        RECT    win;
        int     is_display;
        int     display_rate;
		int 	reserved[2];
    }dst_param;
}dvr_dec_control;


typedef struct dvr_dec_clear_param_tag
{
  RECT          win;
  unsigned int  pattern;
  int 			reserved[2];
}dvr_dec_clear_param;


#define DVR_DEC_IOC_MAGIC   'H'
#define DVR_DEC_SET_CHANNEL_PARAM           _IOWR(DVR_DEC_IOC_MAGIC, 2, dvr_dec_channel_param) 
#define DVR_DEC_GET_CHANNEL_PARAM           _IO(DVR_DEC_IOC_MAGIC, 3)
#define DVR_DEC_QUEUE_GET                   _IOWR(DVR_DEC_IOC_MAGIC, 5, dvr_dec_queue_get)
#define DVR_DEC_QUEUE_PUT                   _IOWR(DVR_DEC_IOC_MAGIC, 6, dvr_dec_queue_get)
#define DVR_DEC_CONTROL                     _IOW(DVR_DEC_IOC_MAGIC, 7, dvr_dec_control)
#define DVR_DEC_QUERY_OUTPUT_BUFFER_SIZE    _IOWR(DVR_DEC_IOC_MAGIC, 8, int)
#define DVR_DEC_DEBUG_PRINT					_IOW(DVR_DEC_IOC_MAGIC, 17, char *)
#define DVR_DEC_CLEAR_WIN                   _IOW(DVR_DEC_IOC_MAGIC, 18, dvr_dec_clear_param)
#define DVR_DEC_CLEAR_WIN2                  _IOW(DVR_DEC_IOC_MAGIC, 19, dvr_dec_clear_param)

#endif /* __DVR_DEC_API_H__ */

