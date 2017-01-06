
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
#define TAG_NAME "[leds_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)

/*#define DEBUG_LEDS_STROBE*/
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#else
#define PK_DBG(a, ...)
#endif

static int g_duty=-1;

#define GPIO_OUT_ONE	(1)
#define GPIO_OUT_ZERO	(0)
#define GPIO_OUT_PWM   (3)

extern void flash_gpio_output(int level);

extern unsigned long BAT_Get_Battery_Voltage(int polling_mode);
static int mt_strobe_set_pwm(int on ,int torch)
{
	struct pwm_spec_config pwm_setting;
	
	pwm_setting.pwm_no = PWM4;
    pwm_setting.mode = PWM_MODE_OLD;
    
	PK_DBG("mt_strobe_set_pwm enter g_duty = %d\n", g_duty);
	printk("wilson mt_strobe_set_pwm enter g_duty = %d on =%d  torch=%d \n", g_duty,on,torch);

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
			if ( 1 == torch )
			{
				if (4000 <= BAT_Get_Battery_Voltage(0))	
				{
					pwm_setting.PWM_MODE_OLD_REGS.THRESH = 2;
				}else{
					pwm_setting.PWM_MODE_OLD_REGS.THRESH = 3;
				}
			
			}
			else
			{
				if (4000 <= BAT_Get_Battery_Voltage(0))	
				{
					pwm_setting.PWM_MODE_OLD_REGS.THRESH = 7;
				}else{
					pwm_setting.PWM_MODE_OLD_REGS.THRESH = 9;
				}
			}

		}
	}
	else
	{
		pwm_setting.PWM_MODE_OLD_REGS.THRESH = 0;
	}
	
	pwm_setting.clk_div = CLK_DIV128;			
	pwm_setting.PWM_MODE_OLD_REGS.DATA_WIDTH = 10;
	
	pwm_setting.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GDURATION = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;
	pwm_set_spec_config(&pwm_setting);
	printk("wilson debug %s, line:%d Thresh=%d\n", __func__, __LINE__,pwm_setting.PWM_MODE_OLD_REGS.THRESH);

	return 0;
}

static ssize_t FL_Init(void)
{
    PK_DBG("front flash gpio_FL_init\n");

    flash_gpio_output(GPIO_OUT_ZERO);

     return 0;
}

static ssize_t FL_dim_duty(kal_uint32 duty)
{
	PK_DBG(" FL_dim_duty line=%d\n",__LINE__);
	g_duty = duty;
    return 0;
}

static int torch_mode(void)
{
 	flash_gpio_output(3);
    mt_strobe_set_pwm(1,1);
	return 0;

}
static int flash_mode(void)
{
 flash_gpio_output(3);
    mt_strobe_set_pwm(1,0);

	return 0;
}

static ssize_t FL_Enable(void)
{
    PK_DBG("gpio_FL_enable\n");
    /*Enable*/

	printk("wilson debug FL_Enable\n");
	if (g_duty <= 1)
	{
		torch_mode();
	}else
	{
		flash_mode();
	}
	printk("wilson debug %s, line:%d\n", __func__, __LINE__);				
       
    return 0;
}

static ssize_t FL_Disable(void)
{
    PK_DBG("gpio_FL_disable\n");
	printk("wilson debug %s, line:%d\n", __func__, __LINE__);

    flash_gpio_output(GPIO_OUT_ZERO);
    mt_strobe_set_pwm(0,0);
    mt_pwm_disable(PWM4, 0);
	printk("wilson debug %s, line:%d\n", __func__, __LINE__);

    return 0;
}

static ssize_t FL_Uninit(void)
{
    PK_DBG("gpio_FL_uninit\n");
	printk("wilson debug %s, line:%d\n", __func__, __LINE__);
    FL_Disable();

    flash_gpio_output(GPIO_OUT_ZERO);
    return 0;
}





static int constant_flashlight_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));
    PK_DBG("flash dummy ioctl\n");
	printk("wilson debug %s, line:%d\n", __func__, __LINE__);
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
				printk("wilson debug %s, line:%d\n", __func__, __LINE__);
        		if(arg==1)
        		{

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

static int constant_flashlight_open(void *pArg)
{
	PK_DBG("dummy open");
	printk("wilson debug %s, line:%d\n", __func__, __LINE__);
	FL_Init();
	return 0;
}

static int constant_flashlight_release(void *pArg)
{
    PK_DBG("dummy release");
	printk("wilson debug %s, line:%d\n", __func__, __LINE__);
    FL_Uninit();
    return 0;
}


FLASHLIGHT_FUNCTION_STRUCT constantFlashlightFunc = {
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};



MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &constantFlashlightFunc;
	return 0;
}


/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{

	return 0;
}
EXPORT_SYMBOL(strobe_VDIrq);


