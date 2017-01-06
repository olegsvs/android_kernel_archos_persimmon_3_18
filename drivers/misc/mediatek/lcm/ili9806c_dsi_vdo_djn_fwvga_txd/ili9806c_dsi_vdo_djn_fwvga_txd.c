#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mt-plat/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//RGK add
// ---------------------------------------------------------------------------
//#include <cust_adc.h>    	// zhoulidong  add for lcm detect
//#define MIN_VOLTAGE (900)     // zhoulidong  add for lcm detect
//#define MAX_VOLTAGE (1000)     // zhoulidong  add for lcm detect
#define AUXADC_LCM_VOLTAGE_CHANNEL 2
#define REGFLAG_DELAY                                       0XFD
#define REGFLAG_END_OF_TABLE                                0xFE   // END OF REGISTERS MARKER

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(854)

#define LCM_ID_ILI9806									0x9816 


#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif

// unsigned static int lcm_esd_test = FALSE;      ///only for ESD test

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util ;

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE							0

// zhoulidong  add for lcm detect ,read adc voltage
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);

struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};

#if 1
static struct LCM_setting_table lcm_initialization_setting[] = {

	/*
	Note :

	Data ID will depends on the following rule.

	    count of parameters > 1 => Data ID = 0x39
	    count of parameters = 1 => Data ID = 0x15
	    count of parameters = 0 => Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag

	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/
	{0xFF,5,    {0xFF, 0x98, 0x06, 0x04, 0x01}},
	{0x08,1,    {0x10}},
	{0x21,1,    {0x01}},// Enable cmd
	{0x30,1,    {0x01}},
	{0x31,1,    {0x02}},
	{0x40,1,    {0x16}},
	//{0xB3,5,    {0x00, 0x00, 0x20, 0x00, 0x00}},//Command Set Option Parameter
	{0x41,1,    {0x44}},
	{0x42,1,    {0x02}},
	{0x43,1,    {0x85}},
	{0x44,1,    {0x8b}},
	{0x45,1,    {0x1b}},
	{0x50,1,    {0x78}},
	{0x51,1,    {0x78}},
	{0x52,1,    {0x00}},
	{0x53,1,    {0x53}},
	{0x57,1,    {0x50}},
	{0x60,1,    {0x07}},// Enable cmd
	{0x61,1,    {0x00}},
	{0x62,1,    {0x07}},//OTP select region
	{0x63,1,    {0x00}},
	{0xA0,1,    {0x00}},
	{0xA1,1,    {0x09}},
	{0xA2,1,    {0x0A}},
	{0xA3,1,    {0x0F}},
	{0xA4,1,    {0x07}},
	{0xA5,1,    {0x0C}},
	{0xA6,1,    {0x07}},
	{0xA7,1,    {0x07}},
	{0xA8,1,    {0x06}},
	{0xA9,1,    {0x07}},
	{0xAA,1,    {0x0F}},
	{0xAB,1,    {0x09}},
	{0xAC,1,    {0x0F}},
	{0xAD,1,    {0x19}},
	{0xAE,1,    {0x11}},
	{0xAF,1,    {0x00}},
	{0xC0,1,    {0x00}},
	{0xC1,1,    {0x08}},
	{0xC2,1,    {0x15}},
	{0xC3,1,    {0x0B}},
	{0xC4,1,    {0x05}},
	{0xC5,1,    {0x0B}},
	{0xC6,1,    {0x07}},
	{0xC7,1,    {0x05}},
	{0xC8,1,    {0x06}},
	{0xC9,1,    {0x0C}},
	{0xCA,1,    {0x11}},
	{0xCB,1,    {0x0B}},
	{0xCC,1,    {0x0E}},
	{0xCD,1,    {0x1B}},
	{0xCE,1,    {0x0C}},
	{0xCF,1,    {0x00}},
	{0xFF,5,    {0xFF, 0x98, 0x06, 0x04, 0x06}},
	{0x00,1,    {0x20}},
	{0x01,1,    {0x05}},
	{0x02,1,    {0x00}},
	{0x03,1,    {0x00}},
	{0x04,1,    {0x01}},
	{0x05,1,    {0x01}},
	{0x06,1,    {0x88}},
	{0x07,1,    {0x04}},
	{0x08,1,    {0x01}},
	{0x09,1,    {0x90}},
	{0x0A,1,    {0x04}},
	{0x0B,1,    {0x01}},
	{0x0C,1,    {0x01}},
	{0x0D,1,    {0x01}},
	{0x0E,1,    {0x00}},
	{0x0F,1,    {0x00}},
	{0x10,1,    {0x55}},
	{0x11,1,    {0x50}},
	{0x12,1,    {0x01}},
	{0x13,1,    {0x0c}},
	{0x14,1,    {0x0d}},
	{0x15,1,    {0x43}},
	{0x16,1,    {0x0b}},
	{0x17,1,    {0x00}},
	{0x18,1,    {0x00}},
	{0x19,1,    {0x00}},
	{0x1A,1,    {0x00}},
	{0x1B,1,    {0x00}},
	{0x1C,1,    {0x00}},
	{0x1D,1,    {0x00}},
	{0x20,1,    {0x01}},
	{0x21,1,    {0x23}},
	{0x22,1,    {0x45}},
	{0x23,1,    {0x67}},
	{0x24,1,    {0x01}},
	{0x25,1,    {0x23}},
	{0x26,1,    {0x45}},
	{0x27,1,    {0x67}},
	{0x30,1,    {0x02}},
	{0x31,1,    {0x22}},
	{0x32,1,    {0x11}},
	{0x33,1,    {0xAA}},
	{0x34,1,    {0xBB}},
	{0x35,1,    {0x66}},
	{0x36,1,    {0x00}},
	{0x37,1,    {0x22}},
	{0x38,1,    {0x22}},
	{0x39,1,    {0x22}},
	{0x3A,1,    {0x22}},
	{0x3B,1,    {0x22}},
	{0x3C,1,    {0x22}},
	{0x3D,1,    {0x22}},
	{0x3E,1,    {0x22}},
	{0x3F,1,    {0x22}},
	{0x40,1,    {0x22}},
	{0x52,1,    {0x10}},
  {0x53,1,    {0x10}},
	{0xFF,5,    {0xFF, 0x98, 0x06, 0x04, 0x07}},
	{0x18,1,    {0x1D}},
	{0x17,1,    {0x22}},
	{0x02,1,    {0x77}},
	{0xE1,1,    {0x79}},//Oscillator Adjustment for Idle/Normal Mode
	{0x26,1,    {0xB2}},
	{0xB3,1,    {0x10}},
	{0xFF,5,    {0xFF, 0x98, 0x06, 0x04, 0x00}},
	{0x11,0,{}},
	{REGFLAG_DELAY, 120, {}},
	{0x29,0,{}},
	{REGFLAG_DELAY, 40, {}},


	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.

	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif


void static push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {

		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

			case REGFLAG_DELAY :
				MDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE :
				break;

			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}

}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{

		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
        #endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
		params->dsi.vertical_sync_active				= 4;// 3    2
		params->dsi.vertical_backporch					= 16;// 20   1
		params->dsi.vertical_frontporch					= 20; // 1  12
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 20;// 50  2
		params->dsi.horizontal_backporch				= 120;
		params->dsi.horizontal_frontporch				= 120;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    //params->dsi.LPX=8; 

		// Bit rate calculation
		//1 Every lane speed
		//params->dsi.pll_select=1;
		//params->dsi.PLL_CLOCK  = LCM_DSI_6589_PLL_CLOCK_377;
		params->dsi.PLL_CLOCK=217;
#if 0
		params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
		params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
#if (LCM_DSI_CMD_MODE)
		params->dsi.fbk_div =7;
#else
		params->dsi.fbk_div =7;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
#endif
		//params->dsi.compatibility_for_nvk = 1;		// this parameter would be set to 1 if DriverIC is NTK's and when force match DSI clock for NTK's
#endif

}


static void lcm_init(void)
{
	printk("wilson debug lk lcm_init start\n");
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

		//dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	printk("wilson debug lk lcm_init end\n");
}

#if 1
static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {}},
	{REGFLAG_DELAY, 10, {}},

    // Sleep Mode On
	{0x10, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

static void lcm_suspend(void)
{	

	//dsi_set_cmdq_V3(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting)/sizeof(lcm_deep_sleep_mode_in_setting[0]), 1);
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	SET_RESET_PIN(1);	
	SET_RESET_PIN(0);
	MDELAY(20); // 1ms
	
	SET_RESET_PIN(1);
	MDELAY(120);      
}


static void lcm_resume(void)
{
	lcm_init();

}

#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif



/*
static unsigned int lcm_compare_id(void)
{
	int array[4];
	char buffer[5];
	char id_high=0;
	char id_low=0;
	int id=0;

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(200);


	array[0] = 0x00053700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xD3, buffer, 5);

	id_high = buffer[2];
	id_low = buffer[3];
	id = (id_high<<8) | id_low;
	
       #ifdef BUILD_LK
       #else

	#if defined(BUILD_UBOOT)
		printf("ILI9806 uboot %s \n", __func__);
	       printf("%s id = 0x%08x \n", __func__, id);
	#else
		printk("ILI9806 kernel %s \n", __func__);
		printk("%s id = 0x%08x \n", __func__, id);
	#endif
       #endif
	   
	return 1;
}
*/

// zhoulidong  add for lcm detect (start)
static unsigned int rgk_lcm_compare_id(void)
{
    int data[4] = {0,0,0,0};
    int res = 0;
    int rawdata = 0;
    int lcm_vol = 0;

#ifdef AUXADC_LCM_VOLTAGE_CHANNEL
    res = IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL,data,&rawdata);
    if(res < 0)
    { 
	#ifdef BUILD_LK
	printf("[adc_uboot]: get data error\n");
	#endif
	printk("wilson 2 [adc_uboot]: get data error\n");
	return 0;
		   
    }
#endif

    lcm_vol = data[0]*1000+data[1]*10;

    #ifdef BUILD_LK
    printf("[adc_uboot]: lcm_vol= %d\n",lcm_vol);
    #endif
	printk("wilson 2 [adc_uboot]: lcm_vol= %d\n",lcm_vol);
	
    if (lcm_vol>=0 &&lcm_vol <= 300)
    {
	printk("wilson tongxinda lcm\n");
	return 0;
    }
	printk("wilson not tongxinda lcm\n");
    return 1;
	
}


/*
// zhoulidong add for eds(start)
static unsigned int lcm_esd_check(void)
{
	#ifdef BUILD_LK
		//printf("lcm_esd_check()\n");
	#else
		//printk("lcm_esd_check()\n");
	#endif 
 #ifndef BUILD_LK
	char  buffer[3];
	int   array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0a, buffer, 1);
	if(buffer[0]==0x9c)
	{
		//#ifdef BUILD_LK
		//printf("%s %d\n FALSE", __func__, __LINE__);
		//#else
		//printk("%s %d\n FALSE", __func__, __LINE__);
		//#endif
		return FALSE;
	}
	else
	{	
		//#ifdef BUILD_LK
		//printf("%s %d\n FALSE", __func__, __LINE__);
		//#else
		//printk("%s %d\n FALSE", __func__, __LINE__);
		//#endif		 
		return TRUE;
	}
 #endif

}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	
	#ifndef BUILD_LK
	printk("lcm_esd_recover  ili9806c_dsi_vdo_djn_fwvga \n");
	#endif
	return TRUE;
}
*/



LCM_DRIVER ili9806c_dsi_vdo_djn_fwvga_txd_lcm_drv = 
{
    .name			= "ili9806c_dsi_vdo_djn_fwvga_txd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = rgk_lcm_compare_id,
//	.esd_check = lcm_esd_check,
//	.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
};

