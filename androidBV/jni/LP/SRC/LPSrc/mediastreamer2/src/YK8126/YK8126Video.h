#ifndef __MS__8126__H__
#define __MS__8126__H__


#ifdef _YK_XT8126_


#ifdef HAVE_CONFIG_H
#include "../../mediastreamer-config.h"
#endif

#ifdef VIDEO_ENABLED
#include <mediastreamer2/mswebcam.h>
#include <mediastreamer2/msvideo.h>
#include "mediastreamer2/rfc3984.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "dvr_enc_api.h"
#include "dvr_common_api.h"

 //   #ifdef ENABLE_8126_CAPTURE
 //       #warning "Enable 8126 Video Capture. "
        /**@brief name of 8126 Camera device. */
 //       #define NAME_OF_8126_CAMERA "/dev/device0"

        /**@brief Declaration 8126 Camera desc. */
extern MSWebCamDesc V8126WebCamDesc;

//    #else
//        #warning "Disable 8126 Video Capture."
//    #endif//ENABLE_8126_CAPTURE

#endif//!VIDEO_ENABLED

#endif//__MS__8126__H__

#endif
