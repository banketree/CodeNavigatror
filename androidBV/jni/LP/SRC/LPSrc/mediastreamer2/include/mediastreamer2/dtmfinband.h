#ifndef dtmfinband_h
#define dtmfinband_h

/**
 * @brief ??????yate
 */

#include "msfilter.h"

// ????dtmf???????????
#define MS_DTMF_INBAND_SET_MIN_DETECT_TIME  MS_FILTER_METHOD(MS_DTMF_INBAND_ID, 0, int)

// ??DTMF???
#define MS_DTMF_INBAND_DETECTED MS_FILTER_EVENT(MS_DTMF_INBAND_ID, 1, int)

extern MSFilterDesc ms_dtmf_inband_desc;

//#define DTMF_INBAND_FLOAT

#ifdef DTMF_INBAND_FLOAT

// how much we keep from old value when averaging, must be below 1
#define MOVING_AVG_KEEP     0.97f
// minimum square of signal energy to even consider detecting
#define THRESHOLD2_ABS     1e+06
// sum of tones (low+high) from total
#define THRESHOLD2_REL_ALL  0.60f
// each tone from threshold from total
#define THRESHOLD2_REL_DTMF 0.33f
// hysteresis after tone detection
#define THRESHOLD2_REL_HIST 0.75f

// minimum DTMF detect time
#define DETECT_DTMF_MSEC 32

#endif


#endif
