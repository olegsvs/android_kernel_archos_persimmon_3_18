
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_typedef.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#ifdef CONFIG_COMPAT
#include <linux/fs.h>
#include <linux/compat.h>
#endif
#include "kd_flashlight.h"
#include <mt-plat/mt_pwm.h>
#include <mt-plat/mt_boot.h>

/******************************************************************************
 * Debug configuration
******************************************************************************/
/* availible parameter */
/* ANDROID_LOG_ASSERT */
/* ANDROID_LOG_ERROR */
/* ANDROID_LOG_WARNING */
/* ANDROID_LOG_INFO */
/* ANDROID_LOG_DEBUG */
/* ANDROID_LOG_VERBOSE */
#define TAG_NAME "[sub_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_WARN(fmt, arg...)        pr_warn(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_NOTICE(fmt, arg...)      pr_notice(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_INFO(fmt, arg...)        pr_info(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_TRC_FUNC(f)              pr_debug(TAG_NAME "<%s>\n", __func__)
#define PK_TRC_VERBOSE(fmt, arg...) pr_debug(TAG_NAME fmt, ##arg)
#define PK_ERROR(fmt, arg...)       pr_err(TAG_NAME "%s: " fmt, __func__ , ##arg)

#define DEBUG_LEDS_STROBE
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#define PK_VER PK_TRC_VERBOSE
#define PK_ERR PK_ERROR
#else
#define PK_DBG(a, ...)
#define PK_VER(a, ...)
#define PK_ERR(a, ...)
#endif

static int g_duty=-1;
#define KTD231_ENF (5)
#define KTD231_ENM (59)

#define GPIO_OUT_ONE	(1)
#define GPIO_OUT_ZERO	(0)
#define GPIO_OUT_PWM   (3)

extern void flash_gpio_output(int pin, int level);

static int mt_strobe_set_pwm(int on)
{
	struct pwm_spec_config pwm_setting;
	
	pwm_setting.pwm_no = PWM4;
    pwm_setting.mode = PWM_MODE_OLD;
    
	PK_DBG("mt_strobe_set_pwm enter g_duty = %d\n", g_duty);

	/* We won't choose 32K to be the clock src of old mode because of system performance. */
	/* The setting here will be clock src = 26MHz, CLKSEL = 26M/1625 (i.e. 16K) */
	pwm_setting.clk_src = PWM_CLK_OLD_MODE_BLOCK;

	if(on)
	{
		if(FACTORY_BOOT==get_boot_mode())
		{
	 	    pwm_setting.PWM_MODE_OLD_REGS.THRESH = 0;
		}
		else
		{
    		pwm_setting.PWM_MODE_OLD_REGS.THRESH = 5 * g_duty + 4; //60/2;  //g_duty
		}
	}
	else
	{
		pwm_setting.PWM_MODE_OLD_REGS.THRESH = 0;
	}
	
	pwm_setting.clk_div = CLK_DIV128;			
	pwm_setting.PWM_MODE_OLD_REGS.DATA_WIDTH = 100/2;
	
	pwm_setting.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GDURATION = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;
	pwm_set_spec_config(&pwm_setting);

	return 0;
}

static ssize_t FL_Init(void)
{
    PK_DBG("front flash gpio_FL_init\n");
    PK_DBG("KTD231_ENF 	= GPIO5, \n");
    PK_DBG("KTD231_ENM 	= GPIO59, \n");

    /*set torch mode*/
    flash_gpio_output(KTD231_ENF,GPIO_OUT_ZERO);
    /*Init. to disable*/
    flash_gpio_output(KTD231_ENM,GPIO_OUT_ZERO);
 //    INIT_WORK(&workTimeOut,work_timeOutFunc);
     return 0;
}

static ssize_t FL_dim_duty(kal_uint32 duty)
{
	PK_DBG(" FL_dim_duty line=%d\n",__LINE__);
	g_duty = duty;
    return 0;
}


static ssize_t FL_Enable(void)
{
    PK_DBG("gpio_FL_enable\n");
    /*Enable*/

    
    flash_gpio_output(KTD231_ENF,GPIO_OUT_ZERO);
    flash_gpio_output(KTD231_ENM,GPIO_OUT_PWM);
    mt_strobe_set_pwm(1);					
       
    return 0;
}

static ssize_t FL_Disable(void)
{
    PK_DBG("gpio_FL_disable\n");
    /*Enable*/
    flash_gpio_output(KTD231_ENM,GPIO_OUT_ZERO);
    mt_strobe_set_pwm(0);
    mt_pwm_disable(PWM4, 0);

    return 0;
}

static ssize_t FL_Uninit(void)
{
    PK_DBG("gpio_FL_uninit\n");
    FL_Disable();
    /*Uninit. to disable*/
    flash_gpio_output(KTD231_ENF,GPIO_OUT_ZERO);
    /*Init. to disable*/
    flash_gpio_output(KTD231_ENM,GPIO_OUT_ZERO);
    return 0;
}





static int sub_strobe_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));
    PK_DBG("front flash dummy ioctl\n");
    switch(cmd)
        {

    		case FLASH_IOC_SET_TIME_OUT_TIME_MS:
    			PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n",(int)arg);
//    			g_timeOutTimeMs=arg;
    		break;


        	case FLASH_IOC_SET_DUTY :
        		PK_DBG("FLASHLIGHT_DUTY: %d\n",(int)arg);
        		FL_dim_duty(arg);
        		break;


        	case FLASH_IOC_SET_STEP:
        		PK_DBG("FLASH_IOC_SET_STEP: %d\n",(int)arg);

        		break;

        	case FLASH_IOC_SET_ONOFF :
        		PK_DBG("FLASHLIGHT_ONOFF: %d\n",(int)arg);
        		if(arg==1)
        		{

//        		    int s;
//        		    int ms;
//        		    if(g_timeOutTimeMs>1000)
//                	{
//                		s = g_timeOutTimeMs/1000;
//                		ms = g_timeOutTimeMs - s*1000;
//                	}
//                	else
//                	{
//                		s = 0;
//                		ms = g_timeOutTimeMs;
//                	}
//
//    				if(g_timeOutTimeMs!=0)
//    	            {
//    	            	ktime_t ktime;
//    					ktime = ktime_set( s, ms*1000000 );
//    					hrtimer_start( &g_timeOutTimer, ktime, HRTIMER_MODE_REL );
//    	            }
        			FL_Enable();
        		}
        		else
        		{
        			FL_Disable();
//    				hrtimer_cancel( &g_timeOutTimer );
        		}
        		break;
    		default :
        		PK_DBG(" No such command \n");
        		i4RetValue = -EPERM;
        		break;
        }
        return i4RetValue;
    
}

static int sub_strobe_open(void *pArg)
{
	PK_DBG("sub dummy open");
	FL_Init();
	return 0;
}

static int sub_strobe_release(void *pArg)
{
    PK_DBG("sub dummy release");
    FL_Uninit();
    return 0;
}

FLASHLIGHT_FUNCTION_STRUCT subStrobeFunc = {
	sub_strobe_open,
	sub_strobe_release,
	sub_strobe_ioctl
};


MUINT32 subStrobeInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &subStrobeFunc;
	return 0;
}

static int mt_strobe_set_pwm2(int on, int level)		
{		
       struct pwm_spec_config pwm_setting;		
       		
       pwm_setting.pwm_no = PWM4;		
       pwm_setting.mode = PWM_MODE_OLD;		
    		
       PK_DBG("mt_strobe_set_pwm enter level = %d\n", level);		
		
       /* We won't choose 32K to be the clock src of old mode because of system performance. */		
       /* The setting here will be clock src = 26MHz, CLKSEL = 26M/1625 (i.e. 16K) */		
       pwm_setting.clk_src = PWM_CLK_OLD_MODE_BLOCK;		
		
       if(on)		
           pwm_setting.PWM_MODE_OLD_REGS.THRESH = level; //60/2;  //level 		
       else		
       pwm_setting.PWM_MODE_OLD_REGS.THRESH = 0;		
       		
       pwm_setting.clk_div = CLK_DIV128;                     		
       pwm_setting.PWM_MODE_OLD_REGS.DATA_WIDTH = 100/2;		
       		
       pwm_setting.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;		
       pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;		
       pwm_setting.PWM_MODE_FIFO_REGS.GDURATION = 0;		
       pwm_setting.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;		
       pwm_set_spec_config(&pwm_setting);		
		
       return 0;		
}		
		
int sub_strobe_onoff(int level)		
{		
    if (level > 0)		
    {		
        flash_gpio_output(KTD231_ENF,GPIO_OUT_ZERO);
        flash_gpio_output(KTD231_ENM,GPIO_OUT_PWM);
        mt_strobe_set_pwm2(1, level);		
        		
    }		
    else		
    {		
        flash_gpio_output(KTD231_ENF,GPIO_OUT_ZERO);		
        flash_gpio_output(KTD231_ENM,GPIO_OUT_ZERO);		
        mt_strobe_set_pwm2(0, 0);		
    }		
		
    return 0;		
}




