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
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_typedef.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>

#include <linux/gpio.h>

#include <linux/pinctrl/pinctrl.h> //add by major for bf168 torch flashlight control.

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

/******************************************************************************
 * local variables
******************************************************************************/
//add by major bf168 torch flashlight control.
struct pinctrl *flash_led_pinctrl;
struct pinctrl_state *flash_pin_default;
struct pinctrl_state *torch_pin_out0,*torch_pin_out1,*flash_pin_out0,*flash_pin_out1;

#ifdef CONFIG_OF
struct of_device_id flash_led_of_match[] = {
		{.compatible = "mediatek,flash_led",},
		{}
};
#endif
//add by major  end


static DEFINE_SPINLOCK(g_strobeSMPLock);	/* cotta-- SMP proection */


static u32 strobe_Res =0;
static u32 strobe_Timeus=0;
static BOOL g_strobe_On =0;

static int g_duty = -1;
static int g_timeOutTimeMs =0;

static DEFINE_MUTEX(g_strobeSem);


#define STROBE_DEVICE_ID 0xC6


static struct work_struct workTimeOut;

/* #define FLASH_GPIO_ENF GPIO12 */
/* #define FLASH_GPIO_ENT GPIO13 */

//static int g_bLtVersion;

/*****************************************************************************
Functions
*****************************************************************************/
static void work_timeOutFunc(struct work_struct *data);


void flash_set_output(int level)
{
		if (level)
			pinctrl_select_state(flash_led_pinctrl, flash_pin_out1);
		else
			pinctrl_select_state(flash_led_pinctrl, flash_pin_out0);
}
void torch_set_output(int level)
{
		if (level)
			pinctrl_select_state(flash_led_pinctrl, torch_pin_out1);
		else
			pinctrl_select_state(flash_led_pinctrl, torch_pin_out0);
}

int FL_Enable(void)
{
	if (g_duty <= -1)
	{
		flash_set_output(0);
		torch_set_output(0);
	}
	else if (g_duty == 0)
	{
		flash_set_output(1);
		torch_set_output(0);
	}
	else
	{
		flash_set_output(1);
		torch_set_output(1);
	}
	return 0;
}



int FL_Disable(void)
{
	PK_DBG(" FL_Disable line=%d\n", __LINE__);

		flash_set_output(0);
		torch_set_output(0);
	return 0;
}

int FL_dim_duty(kal_uint32 duty)
{
	printk(" FL_dim_duty line=%d\n", __LINE__);
	g_duty = duty ;
	return 0;
}




int FL_Init(void)
{
	int ret = 0;
	
	flash_set_output(0);
	torch_set_output(0);
/*	PK_DBG(" FL_Init line=%d\n", __LINE__); */
	return ret;
}


int FL_Uninit(void)
{
	FL_Disable();
	return 0;
}

/*****************************************************************************
User interface
*****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
	FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}



enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}

static struct hrtimer g_timeOutTimer;
void timerInit(void)
{
	INIT_WORK(&workTimeOut, work_timeOutFunc);
	g_timeOutTimeMs = 1000;
	hrtimer_init(&g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_timeOutTimer.function = ledTimeOutCallback;
}



static int constant_flashlight_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;

	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC, 0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC, 0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC, 0, int));
/*	PK_DBG
	    ("LM3642 constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",
	     __LINE__, ior_shift, iow_shift, iowr_shift, (int)arg);
*/
	switch (cmd) {

	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n", (int)arg);
		g_timeOutTimeMs = arg;
		break;


	case FLASH_IOC_SET_DUTY:
		PK_DBG("FLASHLIGHT_DUTY: %d\n", (int)arg);
		FL_dim_duty(arg);
		break;


	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n", (int)arg);

		break;

	case FLASH_IOC_SET_ONOFF:
		PK_DBG("FLASHLIGHT_ONOFF: %d\n", (int)arg);
		if (arg == 1) {

			int s;
			int ms;

			if (g_timeOutTimeMs > 1000) {
				s = g_timeOutTimeMs / 1000;
				ms = g_timeOutTimeMs - s * 1000;
			} else {
				s = 0;
				ms = g_timeOutTimeMs;
			}

			if (g_timeOutTimeMs != 0) {
				ktime_t ktime;

				ktime = ktime_set(s, ms * 1000000);
				hrtimer_start(&g_timeOutTimer, ktime, HRTIMER_MODE_REL);
			}
			FL_Enable();
		} else {
			FL_Disable();
			hrtimer_cancel(&g_timeOutTimer);
		}
		break;
	default:
		PK_DBG(" No such command\n");
		i4RetValue = -EPERM;
		break;
	}
	return i4RetValue;
}




static int constant_flashlight_open(void *pArg)
{
	int i4RetValue = 0;

	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res) {
		FL_Init();
		timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


	if (strobe_Res) {
		PK_DBG(" busy!\n");
		i4RetValue = -EBUSY;
	} else {
		strobe_Res += 1;
	}


	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	return i4RetValue;

}


static int constant_flashlight_release(void *pArg)
{
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res) {
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		FL_Uninit();
	}

	PK_DBG(" Done\n");

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
//add by major for bf168 get flash light gpio control 
int flash_get_gpio_info(struct platform_device *pdev)
{
	int ret=0;
	
	flash_led_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(flash_led_pinctrl)) {
		ret = PTR_ERR(flash_led_pinctrl);
		dev_err(&pdev->dev, "fwq Cannot find flash flash_led_pinctrl!\n");
		return ret;
	}
	flash_pin_default = pinctrl_lookup_state(flash_led_pinctrl, "flash_default");
	if (IS_ERR(flash_pin_default)) {
		ret = PTR_ERR(flash_pin_default);
		dev_err(&pdev->dev, "fwq Cannot find flash pinctrl flash_pin_default %d!\n", ret);
	}
	torch_pin_out1 = pinctrl_lookup_state(flash_led_pinctrl, "torch_en1");
	if (IS_ERR(torch_pin_out1)) {
		ret = PTR_ERR(torch_pin_out1);
		dev_err(&pdev->dev, "fwq Cannot find flash pinctrl torch_pin_en1!\n");
		return ret;
	}
	torch_pin_out0 = pinctrl_lookup_state(flash_led_pinctrl, "torch_en0");
	if (IS_ERR(torch_pin_out0)) {
		ret = PTR_ERR(torch_pin_out0);
		dev_err(&pdev->dev, "fwq Cannot find flash pinctrl flash_torch_pin_0!\n");
		return ret;
	}
	flash_pin_out1 = pinctrl_lookup_state(flash_led_pinctrl, "flash_en1");
	if (IS_ERR(flash_pin_out1)) {
		ret = PTR_ERR(flash_pin_out1);
		dev_err(&pdev->dev, "fwq Cannot find flash pinctrl flash_pin_enm_out1!\n");
		return ret;
	}
	flash_pin_out0 = pinctrl_lookup_state(flash_led_pinctrl, "flash_en0");
	if (IS_ERR(flash_pin_out0)) {
		ret = PTR_ERR(flash_pin_out0);
		dev_err(&pdev->dev, "fwq Cannot find flash pinctrl flash_pin_enm_out0!\n");
		return ret;
	}
	return ret;
}
//add by darren end

static int flashlight_gpio_probe(struct platform_device *dev)
{

	PK_DBG("[flashlight_gpio_probe] start ~");
	flash_get_gpio_info(dev);//add by darren
	return 0;
}

static int flashlight_gpio_remove(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver flashlight_gpio_platform_driver = {
	.probe = flashlight_gpio_probe,
	.remove = flashlight_gpio_remove,
	.driver = {
		   .name = "flashlig_gpio",
		   .owner = THIS_MODULE,
		   .of_match_table = flash_led_of_match,
		   },
};
static int __init flashlight_gpio_init(void)
{
	int ret = 0;
	ret = platform_driver_register(&flashlight_gpio_platform_driver);
	if (ret) {
		PK_DBG("[flashlight_probe] platform_driver_register fail ~");
		return ret;
	}
	return ret;
}
static void __exit flashlight_gpio_exit(void)
{
	PK_DBG("[flashlight_exit] start ~");
	platform_driver_unregister(&flashlight_gpio_platform_driver);
	PK_DBG("[flashlight_exit] done! ~");
}

module_init(flashlight_gpio_init);
module_exit(flashlight_gpio_exit);

