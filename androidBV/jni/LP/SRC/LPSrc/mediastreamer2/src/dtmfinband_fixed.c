
#ifdef _MSC_VER
#include <malloc.h>
#endif

#include "mediastreamer2/dtmfinband.h"
#include "mediastreamer2/msticker.h"
//#include "LOGIntfLayer.h"
#ifndef  DTMF_INBAND_FLOAT
/**
 * @brief ???
 * Vk(i) = 2 cos(2PIk/N)Vk(i-1)-Vk(i-2)+x(i)
 * x(i)?????Vk(i)?????
 * yk(i) = Vk(i)-Vk(i-1)exp(-jWk)
 * ??????????????8??filter?????????
 * yk(i)???????DTMF?????????8??yk(i)??
 * ???????????????????????????
 * ?????DTMF??
 */

// ???????
typedef int32_t fixedpt;

#define FIXEDPT_BITS             32
#ifndef FIXEDPT_WBITS
#define FIXEDPT_WBITS	        8
#endif
#define FIXEDPT_FBITS	        (FIXEDPT_BITS - FIXEDPT_WBITS)
#define FIXEDPT_FMASK	        ((1 << FIXEDPT_FBITS) - 1)

#define fixedpt_rconst(R)       (int32_t)(R * (1LL << FIXEDPT_FBITS) + (R >= 0 ? 0.5 : -0.5))
#define fixedpt_fromint(I)       ((int64_t)I << FIXEDPT_FBITS)
#define fixedpt_toint(F)          (F >> FIXEDPT_FBITS)
#define fixedpt_add(A,B)        (A + B)
#define fixedpt_sub(A,B)        (A - B)
#define fixedpt_mul(A, B)       (((int64_t)A * (int64_t)B) >> FIXEDPT_FBITS)

// how much we keep from old value when averaging, must be below 1
#define MOVING_AVG_KEEP     fixedpt_rconst(0.97f)
#define MOVING_AVG_REST     fixedpt_rconst(0.03f)
// minimum square of signal energy to even consider detecting
#define THRESHOLD2_ABS       fixedpt_rconst(4.75111f)
// sum of tones (low+high) from total
#define THRESHOLD2_REL_ALL  fixedpt_rconst(0.60f)
// each tone from threshold from total
#define THRESHOLD2_REL_DTMF fixedpt_rconst(0.33f)
// hysteresis after tone detection
#define THRESHOLD2_REL_HIST fixedpt_rconst(0.75f)

// minimum DTMF detect time
#define DETECT_DTMF_MSEC 34

// 2-pole filter parameters
typedef struct
{
    double gain;
    double y0;
    double y1;
} Params2Pole;

// generated DTMF component filter parameters
// 2-pole butterworth bandpass, +-1% @ -3dB
static Params2Pole s_params_dtmf_low[] = 
{
    { 1.836705768e+02, -0.9891110494, 1.6984655220 }, // 697Hz
    { 1.663521771e+02, -0.9879774290, 1.6354206881 }, // 770Hz
    { 1.504376844e+02, -0.9867055777, 1.5582944783 }, // 852Hz
    { 1.363034877e+02, -0.9853269818, 1.4673997821 }, // 941Hz
};
static Params2Pole s_params_dtmf_high[] = 
{
    { 1.063096655e+02, -0.9811871438, 1.1532059506 }, // 1209Hz
    { 9.629842594e+01, -0.9792313229, 0.9860778489 }, // 1336Hz
    { 8.720029263e+01, -0.9770643703, 0.7895131023 }, // 1477Hz
    { 7.896493565e+01, -0.9746723483, 0.5613790789 }, // 1633Hz
};

static char s_dtmf_table[][5] = 
{
    "123A", "456B", "789C", "*0#D"
};

// Half 2-pole filter - the other part is common to all filters
typedef struct _Tone2PoleFilter
{
    fixedpt mult;
    fixedpt y0;
    fixedpt y1;
    fixedpt val;
    fixedpt y[3];
} Tone2PoleFilter;

typedef struct _DtmfInbandState
{
    int dtmf_tone;
    int dtmf_count;
    fixedpt xv[3];
    fixedpt pwr;
    Tone2PoleFilter dtmf_low[4];
    Tone2PoleFilter dtmf_high[4];
    int min_detected_time;
    MSBufferizer *buf;
} DtmfInbandState;

static void init_tone_2pole_filter(Tone2PoleFilter *filter, Params2Pole *param)
{
    filter->mult = fixedpt_rconst(1.0/param->gain); 
    filter->y0 = fixedpt_rconst(param->y0); 
    filter->y1 = fixedpt_rconst(param->y1);
    filter->val = filter->y[1] = filter->y[2] = 0;
}

static void dtmfinband_init(MSFilter *f)
{
//	LOG_RUNLOG_DEBUG("LP dtmfinband_init fixed");
    int i;
    DtmfInbandState *s = (DtmfInbandState *)ms_new(DtmfInbandState, 1);
    for (i = 0; i < 4; i++) 
    {
        init_tone_2pole_filter(&s->dtmf_low[i], &s_params_dtmf_low[i]);
        init_tone_2pole_filter(&s->dtmf_high[i], &s_params_dtmf_high[i]);
    }
    s->xv[1] = s->xv[2] = 0;
    s->pwr = 0;
    s->dtmf_tone = '\0';
    s->dtmf_count = 0;
    s->min_detected_time = DETECT_DTMF_MSEC;
    s->buf = ms_bufferizer_new();
    f->data=s;
}

static void dtmfinband_fini(MSFilter *f)
{
    DtmfInbandState *s=(DtmfInbandState *)f->data;
    ms_bufferizer_destroy (s->buf);
    ms_free(f->data);
}

static int dtmfinband_set_min_detect_time(MSFilter *f, void *arg)
{
    DtmfInbandState *s=(DtmfInbandState *)f->data;
    s->min_detected_time = *(int*)arg;
    return 0;
}

static inline void check_dtmf(MSFilter *f, DtmfInbandState *s)
{
	//printf("debug:check_dtmf\n");
    int i;
    char buf[2];
    int row = 0;
    int col = 0;
    fixedpt limit_all;
    fixedpt limit_one;
    fixedpt max_low = s->dtmf_low[0].val;
    fixedpt max_high = s->dtmf_high[0].val;
    char c = s->dtmf_tone;
    s->dtmf_tone = '\0';

    for (i = 1; i < 4; i++) 
    {
        if (max_low < s->dtmf_low[i].val) 
        {
            max_low = s->dtmf_low[i].val;
            row = i;
        }
    }
    
    for (i = 1; i < 4; i++) 
    {
        if (max_high < s->dtmf_high[i].val) 
        {
            max_high = s->dtmf_high[i].val;
            col = i;
        }
    }

    limit_all = fixedpt_mul(s->pwr, THRESHOLD2_REL_ALL);
    limit_one = fixedpt_mul(limit_all, THRESHOLD2_REL_DTMF);
    if (c) 
    {
        limit_all = fixedpt_mul(limit_all, THRESHOLD2_REL_HIST);
        limit_one = fixedpt_mul(limit_one, THRESHOLD2_REL_HIST);
    }

    // ????????
    if ((max_low < limit_one) ||
        (max_high < limit_one) ||
        ((max_low + max_high) < limit_all)) 
    {
        return;
    }
    
    buf[0] = s_dtmf_table[row][col];
    buf[1] = '\0';
    if (buf[0] != c) 
    {
        s->dtmf_tone = buf[0];
        s->dtmf_count = 1;
        return;
    }

    s->dtmf_tone = c;

    if (s->dtmf_count++ == s->min_detected_time) 
    {
    	static iCount = 0;
//    	LOG_RUNLOG_DEBUG("LP fixed check_dtmf:%d(%d)", s->dtmf_tone, ++iCount);
        ms_filter_notify(f, MS_DTMF_INBAND_DETECTED, &s->dtmf_tone);
        //printf("debug:ms_filter_notify end:%d\n", s->dtmf_tone);
    }
}

static void dtmfinband_process(MSFilter *f)
{
    mblk_t *m;
    int buf_size;
    DtmfInbandState *s = (DtmfInbandState *)f->data;
    
    ms_filter_lock(f);

    while ((m = ms_queue_get(f->inputs[0])) != NULL)
    {
        ms_queue_put(f->outputs[0], m);
        ms_bufferizer_put(s->buf, dupmsg(m));
    }

    buf_size = s->buf->size;
    if (s->buf->size%2)
    {
        buf_size -= 1;
    }

    if (buf_size)
    {
        unsigned int nsamp;
        const int16_t* samp;
        uint8_t *buf = (uint8_t *)alloca(buf_size);
        ms_bufferizer_read(s->buf, buf, buf_size);
        
        nsamp = buf_size / 2;
        samp = (const int16_t*)buf;
        while (nsamp--) 
        {
            Tone2PoleFilter *filter;
            fixedpt dx;
            int16_t curr_sample = *samp++;
            
            s->xv[0] = s->xv[1]; 
            s->xv[1] = s->xv[2];
            s->xv[2] = fixedpt_rconst((curr_sample>>12));
            dx = s->xv[2] - s->xv[0];

            s->pwr = fixedpt_mul(MOVING_AVG_KEEP, s->pwr) +
                fixedpt_mul(fixedpt_mul(MOVING_AVG_REST, s->xv[2]),s->xv[2]);
            
            //////////////////////////////////////////////////////////////////////////
            filter = &s->dtmf_low[0];
            filter->y[0] = filter->y[1]; 
            filter->y[1] = filter->y[2];
            filter->y[2] = fixedpt_mul(filter->mult, dx) + 
                fixedpt_mul(filter->y0, filter->y[0]) + 
                fixedpt_mul(filter->y1, filter->y[1]);
            filter->val = fixedpt_mul(MOVING_AVG_KEEP, filter->val) + 
                fixedpt_mul(fixedpt_mul(MOVING_AVG_REST, filter->y[2]), filter->y[2]);

            //////////////////////////////////////////////////////////////////////////
            filter = &s->dtmf_low[1];
            filter->y[0] = filter->y[1]; 
            filter->y[1] = filter->y[2];
            filter->y[2] = fixedpt_mul(filter->mult, dx) + 
                fixedpt_mul(filter->y0, filter->y[0]) + 
                fixedpt_mul(filter->y1, filter->y[1]);
            filter->val = fixedpt_mul(MOVING_AVG_KEEP, filter->val) + 
                fixedpt_mul(fixedpt_mul(MOVING_AVG_REST, filter->y[2]), filter->y[2]);

            //////////////////////////////////////////////////////////////////////////
            filter = &s->dtmf_low[2];
            filter->y[0] = filter->y[1]; 
            filter->y[1] = filter->y[2];
            filter->y[2] = fixedpt_mul(filter->mult, dx) + 
                fixedpt_mul(filter->y0, filter->y[0]) + 
                fixedpt_mul(filter->y1, filter->y[1]);
            filter->val = fixedpt_mul(MOVING_AVG_KEEP, filter->val) + 
                fixedpt_mul(fixedpt_mul(MOVING_AVG_REST, filter->y[2]), filter->y[2]);

            //////////////////////////////////////////////////////////////////////////
            filter = &s->dtmf_low[3];
            filter->y[0] = filter->y[1]; 
            filter->y[1] = filter->y[2];
            filter->y[2] = fixedpt_mul(filter->mult, dx) + 
                fixedpt_mul(filter->y0, filter->y[0]) + 
                fixedpt_mul(filter->y1, filter->y[1]);
            filter->val = fixedpt_mul(MOVING_AVG_KEEP, filter->val) + 
                fixedpt_mul(fixedpt_mul(MOVING_AVG_REST, filter->y[2]), filter->y[2]);

            //////////////////////////////////////////////////////////////////////////
            filter = &s->dtmf_high[0];
            filter->y[0] = filter->y[1]; 
            filter->y[1] = filter->y[2];
            filter->y[2] = fixedpt_mul(filter->mult, dx) + 
                fixedpt_mul(filter->y0, filter->y[0]) + 
                fixedpt_mul(filter->y1, filter->y[1]);
            filter->val = fixedpt_mul(MOVING_AVG_KEEP, filter->val) + 
                fixedpt_mul(fixedpt_mul(MOVING_AVG_REST, filter->y[2]), filter->y[2]);

            //////////////////////////////////////////////////////////////////////////
            filter = &s->dtmf_high[1];
            filter->y[0] = filter->y[1]; 
            filter->y[1] = filter->y[2];
            filter->y[2] = fixedpt_mul(filter->mult, dx) + 
                fixedpt_mul(filter->y0, filter->y[0]) + 
                fixedpt_mul(filter->y1, filter->y[1]);
            filter->val = fixedpt_mul(MOVING_AVG_KEEP, filter->val) + 
                fixedpt_mul(fixedpt_mul(MOVING_AVG_REST, filter->y[2]), filter->y[2]);

            //////////////////////////////////////////////////////////////////////////
            filter = &s->dtmf_high[2];
            filter->y[0] = filter->y[1]; 
            filter->y[1] = filter->y[2];
            filter->y[2] = fixedpt_mul(filter->mult, dx) + 
                fixedpt_mul(filter->y0, filter->y[0]) + 
                fixedpt_mul(filter->y1, filter->y[1]);
            filter->val = fixedpt_mul(MOVING_AVG_KEEP, filter->val) + 
                fixedpt_mul(fixedpt_mul(MOVING_AVG_REST, filter->y[2]), filter->y[2]);

            //////////////////////////////////////////////////////////////////////////
            filter = &s->dtmf_high[3];
            filter->y[0] = filter->y[1]; 
            filter->y[1] = filter->y[2];
            filter->y[2] = fixedpt_mul(filter->mult, dx) + 
                fixedpt_mul(filter->y0, filter->y[0]) + 
                fixedpt_mul(filter->y1, filter->y[1]);
            filter->val = fixedpt_mul(MOVING_AVG_KEEP, filter->val) + 
                fixedpt_mul(fixedpt_mul(MOVING_AVG_REST, filter->y[2]), filter->y[2]);

            // only do checks every millisecond
            if (nsamp % 8) continue;

            // is it enough total power to accept a signal?
            if (s->pwr >= THRESHOLD2_ABS) 
            {
//            	static long i = 0;
//            	i++;
//            	if(i % 6 == 0)
//            	{
//            		printf("fixed debug:check_dtmf\n");
//            	}
                check_dtmf(f, s);
            }
            else 
            {
                s->dtmf_tone = '\0';
                s->dtmf_count = 0;
            }
        }
    }

    ms_filter_unlock(f);
}

MSFilterMethod dtmfinband_methods[] =
{
	{	MS_DTMF_INBAND_SET_MIN_DETECT_TIME,	dtmfinband_set_min_detect_time },
    {	0 ,	NULL }
};

#ifdef _MSC_VER

MSFilterDesc ms_dtmf_inband_desc = 
{
	MS_DTMF_INBAND_ID,
	"MSDtmfInband",
	N_("DTMF inband detector"),
	MS_FILTER_OTHER,
	NULL,
    1,
	1,
	dtmfinband_init,
	NULL,
    dtmfinband_process,
	NULL,
    dtmfinband_fini,
	dtmfinband_methods
};

#else

MSFilterDesc ms_dtmf_inband_desc=
{
	.id=MS_DTMF_INBAND_ID,
	.name="MSDtmfInband",
	.text=N_("DTMF inband detector"),
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=1,
	.init=dtmfinband_init,
	.process=dtmfinband_process,
	.uninit=dtmfinband_fini,
	.methods=dtmfinband_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_dtmf_inband_desc)

#endif//!DTMF_INBAND_FLOAT

