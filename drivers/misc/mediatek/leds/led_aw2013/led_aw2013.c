/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <mach/irqs.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/ctype.h>
#include "leds_sw.h"

#define LED_I2C_CHANNEL     (2)
#define LEDAW2013_I2C_DEVNAME "leds_aw2013"
#define IIC_ADDRESS_WRITE			0x8a             //I2C write address
#define IIC_ADDRESS_READ			0x8b             //I2C read  address


// I2C variable
static struct i2c_client *new_client = NULL;

// new I2C register method
static const struct i2c_device_id led_aw2013_i2c_id[] = {{LEDAW2013_I2C_DEVNAME, 0}, {}};

//function declration
static int LEDaw2013_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int LEDaw2013_i2c_remove(struct i2c_client *client);
static DEFINE_MUTEX(access_lock);

#ifdef CONFIG_OF
static const struct of_device_id led_of_match[] = {	
		{.compatible = "mediatek,led_i2c"},
		{},
};
#endif


ssize_t  AW2013_i2c_read_reg(u8 addr, u8 *returnData);

// read write implementation
//read one register
ssize_t  AW2013_i2c_read_reg(u8 addr, u8 *returnData)
{
    char     cmd_buf[1] = {0x00};
    char     readData = 0;
    int     ret = 0;
    cmd_buf[0] = addr;

    if (!new_client)
    {
        printk("AW2013_i2c_read_reg I2C client not initialized!!");
        return -1;
    }
    ret = i2c_master_send(new_client, &cmd_buf[0], 1);
    if (ret < 0)
    {
        printk("AW2013_i2c_read_reg read sends command error!!\n");
        return -1;
	}
    ret = i2c_master_recv(new_client, &readData, 1);
    if (ret < 0)
    {
        printk("AW2013_i2c_read_reg reads recv data error!!\n");
        return -1;
    }
    *returnData = readData;
    printk("addr 0x%x data 0x%x \n", addr, readData);
    return 0;
}

//write register
ssize_t  AW2013_i2c_write_reg(u8 addr, u8 writeData)
{
	unsigned char write_data[2] = {0};
    int    ret = 0;

    if (!new_client)
    {
        printk("I2C client not initialized!!");
        return -1;
    }
        write_data[0] = addr;         // ex. 0x01
    write_data[1] = writeData;
    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0)
    {
        printk("write sends command error!!");
        return -1;
    }
   // printk("addr 0x%x data 0x%x \n", addr, writeData);
    return 0;
}


#define AW2013_I2C_MAX_LOOP 		50   
#define I2C_delay 		2    //according to platform to adjust  be less than 400k

//adjust led breath mode params
#define Imax          0x01   //LED max current config,0x00=omA,0x01=5mA,0x02=10mA,0x03=15mA,
//#define Rise_time   0x02   //LED breathe up time set,0x00=0.13s,0x01=0.26s,0x02=0.52s,0x03=1.04s,0x04=2.08s,0x05=4.16s,0x06=8.32s,0x07=16.64s
#define Hold_time   0x01   //LED on time set when max brightness 0x00=0.13s,0x01=0.26s,0x02=0.52s,0x03=1.04s,0x04=2.08s,0x05=4.16s
//#define Fall_time     0x02   //LED breathe down time set,0x00=0.13s,0x01=0.26s,0x02=0.52s,0x03=1.04s,0x04=2.08s,0x05=4.16s,0x06=8.32s,0x07=16.64s
#define Off_time      0x01   //LED off time set 0x00=0.13s,0x01=0.26s,0x02=0.52s,0x03=1.04s,0x04=2.08s,0x05=4.16s,0x06=8.32s,0x07=16.64s
#define Delay_time   0x00   //LED delay time set when breathe start 0x00=0s,0x01=0.13s,0x02=0.26s,0x03=0.52s,0x04=1.04s,0x05=2.08s,0x06=4.16s,0x07=8.32s,0x08=16.64s
#define Period_Num  0x00   //LED breathe  times 0x00=infinite,0x01=1,0x02=2.....0x0f=15


#define LED_RED_ON 		 0x1
#define LED_GREEN_ON 	 0x2
#define LED_BLUE_ON 	 0x4

/*********************************************************/
/********* AW2013 LED REGISTERS **************************/
/*********************************************************/
#define LED_RESET_REG    0x00
#define LED_GCR_REG      0x01

#define LED_CONTRL_REG   0x30

#define LED0_CONFIG_REG  0x31
#define LED1_CONFIG_REG  0x32
#define LED2_CONFIG_REG  0x33

#define LED0_PWM_REG     0x34
#define LED1_PWM_REG     0x35
#define LED2_PWM_REG     0x36

/*************************************/
/*           t2                      */
/*        /-------\                  */
//     t1/         \t3
// -----/           \-------
//  t0                 t4
/*************************************/
#define LED0_T1T2_REG    0x37
#define LED1_T1T2_REG    0x3A
#define LED2_T1T2_REG    0x3D

#define LED0_T3T4_REG    0x38
#define LED1_T3T4_REG    0x3B
#define LED2_T3T4_REG    0x3E

#define LED0_T0_REG      0x39
#define LED1_T0_REG      0x3C
#define LED2_T0_REG      0x3F
/***********************************************************/
static int Rise_time =0x02;
static int Fall_time =0x02;


//*******************************AW2013***********************************///
#if 0
static void aw2013_breath_all(int led0,int led1,int led2)  //led on=LED_GCR_REG   ledoff=0x00
{ 
	AW2013_i2c_write_reg(0x00, 0x55);				// Reset 
	AW2013_i2c_write_reg(0x01, 0x01);		// enable LED		

	AW2013_i2c_write_reg(0x31, Imax|0x70);	//config mode, IMAX = 5mA	
	AW2013_i2c_write_reg(0x32, Imax|0x70);	//config mode, IMAX = 5mA	
	AW2013_i2c_write_reg(0x33, Imax|0x70);	//config mode, IMAX = 5mA	

	AW2013_i2c_write_reg(0x34, 0xff);	// LED0 level,
	AW2013_i2c_write_reg(0x35, 0xff);	// LED1 level,
	AW2013_i2c_write_reg(0x36, 0xff);	// LED2 level,
											
	AW2013_i2c_write_reg(0x37, Rise_time<<4 | Hold_time);	//led0  					
	AW2013_i2c_write_reg(0x38, Fall_time<<4 | Off_time);	       //led0
	AW2013_i2c_write_reg(0x39, Delay_time<<4| Period_Num);   //led0  

	AW2013_i2c_write_reg(0x3a, Rise_time<<4 | Hold_time);	//led1		
	AW2013_i2c_write_reg(0x3b, Fall_time<<4 | Off_time);	       //led1 
	AW2013_i2c_write_reg(0x3c, Delay_time<<4| Period_Num);   //led1  

	AW2013_i2c_write_reg(0x3d, Rise_time<<4 | Hold_time);	//led2  
	AW2013_i2c_write_reg(0x3e, Fall_time<<4 | Off_time);	       //led2 
	AW2013_i2c_write_reg(0x3f, Delay_time<<4| Period_Num);    //

	AW2013_i2c_write_reg(0x30, led2<<2|led1<<1|led0);	       //led on=0x01 ledoff=0x00	
	//(8);//
 
}
#endif
void led_breath_rhytm(int on ,int off)
{
	if ( (0 < on) && (on <= 500))
	{
		Rise_time =0x04;//2.08s
	}else if ((500 < on )&&(on <= 1000))
	{
		Rise_time =0x03;//1.04s
	}else
	{
		Rise_time =0x02;//0.52s
	}

	if ( (0 < off) && (off <= 500))
	{
		Fall_time =0x04;//2.08s
	}else if ((500 < off )&&(off <= 1000))
	{
		Fall_time =0x03;//1.04s
	}else
	{
		Fall_time =0x02;//0.52s
	}
}
// led0 --> red
void aw2013_red_breath_mode(int on ,int off)
{
	mutex_lock(&access_lock);
	AW2013_i2c_write_reg(0x00, 0x55);               // Reset 
	AW2013_i2c_write_reg(LED_GCR_REG, 0x01);       // enable LED     
	AW2013_i2c_write_reg(LED0_CONFIG_REG, Imax|0x70);  //config mode, IMAX = 5mA  
	AW2013_i2c_write_reg(LED0_PWM_REG, 0xff);   // LED0 level,
	
	led_breath_rhytm(on,off);

	AW2013_i2c_write_reg(LED0_T1T2_REG, Rise_time<<4 | Hold_time);	//led0  					
	AW2013_i2c_write_reg(LED0_T3T4_REG, Fall_time<<4 | Off_time);	       //led0
	AW2013_i2c_write_reg(LED0_T0_REG, Delay_time<<4| Period_Num);   //led0  
	
	AW2013_i2c_write_reg(LED_CONTRL_REG, LED_RED_ON);	       //led0 on  led1 off led2 off	
	mutex_unlock(&access_lock);
}
//led1 --> green
void aw2013_green_breath_mode(int on , int off)
{
	mutex_lock(&access_lock);
	AW2013_i2c_write_reg(0x00, 0x55);               // Reset 
	AW2013_i2c_write_reg(LED_GCR_REG, 0x01);       // enable LED     
	AW2013_i2c_write_reg(LED1_CONFIG_REG, Imax|0x70);  //config mode, IMAX = 5mA  
	AW2013_i2c_write_reg(LED1_PWM_REG, 0xff);   // LED0 level,

	led_breath_rhytm(on,off);
	
	AW2013_i2c_write_reg(LED1_T1T2_REG, Rise_time<<4 | Hold_time);	//led0  					
	AW2013_i2c_write_reg(LED1_T3T4_REG, Fall_time<<4 | Off_time);	       //led0
	AW2013_i2c_write_reg(LED1_T0_REG, Delay_time<<4| Period_Num);   //led0  
	
	AW2013_i2c_write_reg(LED_CONTRL_REG, LED_GREEN_ON);	       //led0 on  led1 off led2 off	
	mutex_unlock(&access_lock);
}
//led2 -->blue
void aw2013_blue_breath_mode(int on , int off)
{
	mutex_lock(&access_lock);
	AW2013_i2c_write_reg(0x00, 0x55);               // Reset 
	AW2013_i2c_write_reg(LED_GCR_REG, 0x01);                // enable LED     
	AW2013_i2c_write_reg(LED2_CONFIG_REG, Imax|0x70);           // LED3 config mode, IMAX = 5mA  
	AW2013_i2c_write_reg(LED2_PWM_REG, 0xff);                // LED3 level,
	
	led_breath_rhytm(on,off);
	
	AW2013_i2c_write_reg(LED2_T1T2_REG, Rise_time<<4 | Hold_time);	//led0  					
	AW2013_i2c_write_reg(LED2_T3T4_REG, Fall_time<<4 | Off_time);	       //led0
	AW2013_i2c_write_reg(LED2_T0_REG, Delay_time<<4| Period_Num);   //led0  
	
	AW2013_i2c_write_reg(LED_CONTRL_REG, LED_BLUE_ON);	  //led0 off  led1 off led2 on
	mutex_unlock(&access_lock);
}

void aw2013_red_pwm_mode(int level)
{
	mutex_lock(&access_lock);
	AW2013_i2c_write_reg(LED_RESET_REG,0x55);               //reset all regs 
	AW2013_i2c_write_reg(LED_GCR_REG, 0x01);                // enable LED     
	AW2013_i2c_write_reg(LED0_CONFIG_REG, Imax);       // LED0 config mode, IMAX = 5mA  
	AW2013_i2c_write_reg(LED0_PWM_REG, 0xff);               // LED0 level,

	if (level)
	{
		AW2013_i2c_write_reg(LED_CONTRL_REG, LED_RED_ON);	       //led0 on  led1 off led2 off	
	}
	else
	{
		AW2013_i2c_write_reg(LED_CONTRL_REG, 0x0);	       //led0 off  led1 off led2 off	
	}
	mutex_unlock(&access_lock);
}
void aw2013_green_pwm_mode(int level)
{
	mutex_lock(&access_lock);
	AW2013_i2c_write_reg(LED_RESET_REG,0x55);               //reset all regs 
	AW2013_i2c_write_reg(LED_GCR_REG, 0x01);                // enable LED     
	AW2013_i2c_write_reg(LED1_CONFIG_REG, Imax);       // LED1 config mode, IMAX = 5mA  
	AW2013_i2c_write_reg(LED1_PWM_REG, 0xff);               // LED1 level,
	
	if (level)
	{
		AW2013_i2c_write_reg(LED_CONTRL_REG, LED_GREEN_ON);	       //led0 on  led1 off led2 off	
	}else
	{
		AW2013_i2c_write_reg(LED_CONTRL_REG,0x0);        //led0 on  led1 off led2 off 
	}
	mutex_unlock(&access_lock);
}
void aw2013_blue_pwm_mode(int level)
{
	mutex_lock(&access_lock);
	AW2013_i2c_write_reg(LED_RESET_REG,0x55);               //reset all regs 
	AW2013_i2c_write_reg(LED_GCR_REG, 0x01);                // enable LED     
	AW2013_i2c_write_reg(LED2_CONFIG_REG, Imax);       // LED2 config mode, IMAX = 5mA  
	AW2013_i2c_write_reg(LED2_PWM_REG, 0xff);               // LED2 level,

	if(level)
	{
		AW2013_i2c_write_reg(LED_CONTRL_REG, LED_BLUE_ON);	       //led0 on  led1 off led2 off	
	}else
	{
		AW2013_i2c_write_reg(LED_CONTRL_REG,0x0);	       //led0 on  led1 off led2 off	
	}
	mutex_unlock(&access_lock);
}

/*****************************************************************************
 * FILE OPERATION FUNCTION
 *  led_aw2013_driver_ioctl
 *
 * DESCRIPTION
 *  IOCTL Msg handle
 *
 *****************************************************************************
 */
//i2c driver
struct i2c_driver LEDAW2013_i2c_driver =
{
    .probe = LEDaw2013_i2c_probe,
    .remove = LEDaw2013_i2c_remove,
    .driver = {
        .name = LEDAW2013_I2C_DEVNAME,
#ifdef CONFIG_OF
		.of_match_table = led_of_match,
#endif
    },
    .id_table = led_aw2013_i2c_id,
};

static int LEDaw2013_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    new_client = client;
    new_client->timing = 400;
//	aw2013_breath_all(1,1,1);
//	aw2013_red_pwm_mode(1);
    printk("LEDaw2013_i2c_probe  ok\n");
   return 0;
}
static int LEDaw2013_i2c_remove(struct i2c_client *client)
{
    new_client = NULL;
    i2c_unregister_device(client);
    i2c_del_driver(&LEDAW2013_i2c_driver);
    return 0;
}
static int LEDaw2013_probe(struct platform_device *dev)
{
    printk("LEDaw2013_probe \n");

	if (i2c_add_driver(&LEDAW2013_i2c_driver))
    {
        printk("fail to add device into i2c");
        return -1;
    }

	printk("gaomeitao %s 3333\n",__func__);
    return 0;

}
static int __init led_aw2013_driver_mod_init(void)
{
    int ret = 0;
    printk("+led_aw2013_driver_mod_init \n");

	ret = LEDaw2013_probe(NULL);
	if (ret < 0)
	{
		return ret;
	}
   return 0;
}

static void __exit led_aw2013_driver_mod_exit(void)
{
    printk("+led_aw2013_driver_mod_exit \n");
}

module_init(led_aw2013_driver_mod_init);
module_exit(led_aw2013_driver_mod_exit);
