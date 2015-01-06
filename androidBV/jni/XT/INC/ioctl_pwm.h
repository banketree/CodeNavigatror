#ifndef _FTPWMTMR010_API_H_
#define _FTPWMTMR010_API_H_

//============================================================================
// data struct
//============================================================================
#define PWM_EXTCLK          240000000   // 24Mhz
#define NUM_OF_PWM          4

#define PWM_CLKSRC_PCLK     0
#define PWM_CLKSRC_EXTCLK   1

#define PWM_DEF_CLKSRC      PWM_CLKSRC_PCLK
#define PWM_DEF_FREQ        20000
#define PWM_DEF_DUTY_STEPS  100
#define PWM_DEF_DUTY_RATIO  50

typedef struct pwm_info {
    int clksrc;
    unsigned int freq;
    unsigned int duty_steps;
    unsigned int duty_ratio;
} pwm_info_t;

//============================================================================
// I/O control ID
//============================================================================
/* Use 'p' as magic number */
#define PWM_IOC_MAGIC               'p'
#define PWM_IOCTL_START             _IOW(PWM_IOC_MAGIC, 1, int)
#define PWM_IOCTL_STOP              _IOW(PWM_IOC_MAGIC, 2, int)
#define PWM_IOCTL_GET_INFO          _IOW(PWM_IOC_MAGIC, 3, pwm_info_t)
#define PWM_IOCTL_SET_CLKSRC        _IOW(PWM_IOC_MAGIC, 4, int)
#define PWM_IOCTL_SET_FREQ          _IOW(PWM_IOC_MAGIC, 5, unsigned int)
#define PWM_IOCTL_SET_DUTY_STEPS    _IOW(PWM_IOC_MAGIC, 6, unsigned int)
#define PWM_IOCTL_SET_DUTY_RATIO    _IOW(PWM_IOC_MAGIC, 7, unsigned int)
//temp
#define PWM_IOCTL_VCM_ENABLE        _IOW(PWM_IOC_MAGIC, 20, int)
#define PWM_IOCTL_VCM_DISABLE       _IOW(PWM_IOC_MAGIC, 21, int)

#endif /*_FTPWMTMR010_API_H_*/
