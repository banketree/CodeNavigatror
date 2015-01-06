
#ifdef _MSC_VER
#include <malloc.h>
#endif

#include "mediastreamer2/dtmfinband.h"
#include "mediastreamer2/msticker.h"
//#include "LOGIntfLayer.h"
#ifdef DTMF_INBAND_FLOAT

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

// 2-pole filter parameters
typedef struct
{
    float gain;
    float y0;
    float y1;
} Params2Pole;

// Half 2-pole filter - the other part is common to all filters
typedef struct _Tone2PoleFilter
{
    float mult;
    float y0;
    float y1;
    float val;
    float y[3];
} Tone2PoleFilter;

typedef struct _DtmfInbandState
{
    int dtmf_tone;
    int dtmf_count;
    float xv[3];
    float pwr;
    Tone2PoleFilter dtmf_low[4];
    Tone2PoleFilter dtmf_high[4];
    int min_detected_time;
    MSBufferizer *buf;
} DtmfInbandState;

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

#define UPDATE_PWR(x, y) (MOVING_AVG_KEEP*(x) + (0.03f)*y*y) 
#define UPDATE_2POLE_FILTER(x,z)\
	    x.y[0] = x.y[1]; \
   	    x.y[1] = x.y[2];\
	    x.y[2] = (z * x.mult) + (x.y0 * x.y[0]) + (x.y1 * x.y[1]);\
	    x.val=UPDATE_PWR(x.val, x.y[2]);


static void update_pwr(float *avg, float val)
{
    *avg = MOVING_AVG_KEEP*(*avg) + (1-MOVING_AVG_KEEP)*val*val;
}


static void update_2pole_filter(Tone2PoleFilter *filter, float xd)
{
    filter->y[0] = filter->y[1]; 
    filter->y[1] = filter->y[2];
    filter->y[2] = (xd * filter->mult) + (filter->y0 * filter->y[0]) + (filter->y1 * filter->y[1]);
    //update_pwr(&filter->val, filter->y[2]);
    filter->val=UPDATE_PWR(filter->val, filter->y[2]);
}

static void init_tone_2pole_filter(Tone2PoleFilter *filter, Params2Pole *param)
{
    filter->mult = 1.0f/param->gain; 
    filter->y0 = param->y0; 
    filter->y1 = param->y1;
    filter->val = filter->y[1] = filter->y[2] = 0.0;
}

static void check_dtmf(DtmfInbandState *s, MSFilter *f)
{
    int i;
    char buf[2];
    int row = 0;
    int col = 0;
    float limit_all;
    float limit_one;
    float max_low = s->dtmf_low[0].val;
    float max_high = s->dtmf_high[0].val;
    char c = s->dtmf_tone;
    s->dtmf_tone = '\0';

    for (i = 1; i < 4; i++)  {
        if (max_low < s->dtmf_low[i].val)  {
            max_low = s->dtmf_low[i].val;
            row = i;
        }
    }
    
    for (i = 1; i < 4; i++)  {
        if (max_high < s->dtmf_high[i].val)  {
            max_high = s->dtmf_high[i].val;
            col = i;
        }
    }
    
    limit_all = s->pwr * THRESHOLD2_REL_ALL;
    limit_one = limit_all * THRESHOLD2_REL_DTMF;
    if (c)  {
        limit_all *= THRESHOLD2_REL_HIST;
        limit_one *= THRESHOLD2_REL_HIST;
    }

    // ????????
    if ((max_low < limit_one) || (max_high < limit_one) || ((max_low + max_high) < limit_all))  {
        if (c) {
            	ms_debug("Giving up DTMF '%c' lo=%0.1f, hi=%0.1f, total=%0.1f",c, max_low, max_high, s->pwr);
        }
        return;
    }
    
    buf[0] = s_dtmf_table[row][col];
    buf[1] = '\0';
    if (buf[0] != c) {
        s->dtmf_tone = buf[0];
        s->dtmf_count = 1;
        return;
    }
    s->dtmf_tone = c;
    if (s->dtmf_count++ == s->min_detected_time) {
    	static iCount = 0;
//    	LOG_RUNLOG_DEBUG("LP float check_dtmf:%d(%d)", s->dtmf_tone, ++iCount);
		ms_filter_notify(f,MS_DTMF_INBAND_DETECTED,&s->dtmf_tone);
    }
}

static void dtmfinband_init(MSFilter *f)
{
//	LOG_RUNLOG_DEBUG("LP dtmfinband_init float");
    int i;
    DtmfInbandState *s = (DtmfInbandState *)ms_new(DtmfInbandState, 1);
    for (i = 0; i < 4; i++) 
    {
        init_tone_2pole_filter(&s->dtmf_low[i], &s_params_dtmf_low[i]);
        init_tone_2pole_filter(&s->dtmf_high[i], &s_params_dtmf_high[i]);
    }
    s->xv[1] = s->xv[2] = 0.0f;
    s->pwr = 0.0f;
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
            float dx;
            int j;
            s->xv[0] = s->xv[1]; 
            s->xv[1] = s->xv[2];
            s->xv[2] = *samp++;

            dx = s->xv[2] - s->xv[0];

            s->pwr=UPDATE_PWR(s->pwr,s->xv[2]);
			
 	     UPDATE_2POLE_FILTER(s->dtmf_low[0], dx);
	     UPDATE_2POLE_FILTER(s->dtmf_high[0], dx);

	     UPDATE_2POLE_FILTER(s->dtmf_low[1], dx)
	     UPDATE_2POLE_FILTER(s->dtmf_high[1], dx);

	     UPDATE_2POLE_FILTER(s->dtmf_low[2], dx);
	     UPDATE_2POLE_FILTER(s->dtmf_high[2], dx);

	    UPDATE_2POLE_FILTER(s->dtmf_low[3], dx);
	     UPDATE_2POLE_FILTER(s->dtmf_high[3], dx);

	     if (nsamp % 8) continue;

	     if (s->pwr >= THRESHOLD2_ABS) 
            {
//         	static long i = 0;
//         	i++;
//         	if(i % 6 == 0)
//         	{
//         		printf("float debug:check_dtmf\n");
//         	}
                check_dtmf(s, f);
            }
            else 
            {
                s->dtmf_tone = '\0';
                s->dtmf_count = 0;
            }
#if 0		
            update_pwr(&s->pwr,s->xv[2]);

            for (j = 0; j < 4; ++j)
            {
		update_2pole_filter(&s->dtmf_low[j], dx);
		update_2pole_filter(&s->dtmf_high[j], dx);
            }

            // only do checks every millisecond

            // is it enough total power to accept a signal?

#endif
        }
    }

    ms_filter_unlock(f);
}

MSFilterMethod dtmfinband_methods[] =
{
	{MS_DTMF_INBAND_SET_MIN_DETECT_TIME,	dtmfinband_set_min_detect_time },
        {0 ,	NULL }
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

#endif//#ifdef DTMF_INBAND_FLOAT

