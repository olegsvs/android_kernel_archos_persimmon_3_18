#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"
#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

#define OTM1287A_HD720_ID    (0x1287)
#define COMPATIBLE_BY_ADC
#ifdef COMPATIBLE_BY_ADC
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
typedef enum {
    OTM1287A_MODULE_TXD = 0x00,
    OTM1287A_MODULE_BOYI = 0x01,
    OTM1287A_MODULE_UNKNOWN = 0xFF,
}OTM1287A_LCM_MODULE_TYPE;
#endif
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))
#define REGFLAG_DELAY                                       0XFD
#define REGFLAG_END_OF_TABLE                                0xFE   // END OF REGISTERS MARKER


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                  lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)              lcm_util.dsi_write_regs(addr, pdata, byte_nums)
//#define read_reg                                          lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)



//#define LCM_DSI_CMD_MODE

struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
		{0x00, 1 , {0x00}},
		{0xFF,  3 ,{0x12,0x87,0x01}},

		{0x00, 1 , {0x80}},
		{0xFF,  2 ,{0x12,0x87}},

		{0x00, 1 , {0x92}},
		{0xFF,  2 ,{0x30,0x02}},

	{0x00,1,{0x91}},         
	{0xb0,1,{0x92}},
		{0x00, 1 , {0x80}},
		{0xC0,  9 ,{0x00,0x64,0x00,0x0F,0x11,0x00,0x64,0x0F,0x11}},

		{0x00, 1 , {0x90}},
		{0xC0,  6 ,{0x00,0x5C,0x00,0x01,0x00,0x04}},

		{0x00, 1 , {0xA4}},
		{0xC0,  1 ,{0x00}},

		{0x00, 1 , {0xB3}},
		{0xC0,  2 ,{0x00,0x55}},

	{0x00,1,{0x81}},             
	{0xc1,1,{0x55}},

		{0x00, 1 , {0xA0}},
		{0xC4, 14 ,{0x05,0x10,0x06,0x02,0x05,0x15,0x10,0x05,0x10,0x07,0x02,0x05,0x15,0x10}},

		{0x00, 1 , {0xB0}},
		{0xC4,  2 ,{0x00,0x00}},

		{0x00, 1 , {0x00}},
		{0xD8,  2 ,{0x8C,0x8C}},  //{0xD8,  2 ,{0x8C,0x8C}}, change by alaric, resolve face red

	{0x00,1,{0x00}},   //add..2015/12/07          
//	{0xD9,1,{0x3f}},   //does not config reg 0xD9

	{0x00,1,{0x00}},            
	{0xE1,20,{0x05,0x16,0x22,0x2C,0x3B,0x47,0x47,0x70,0x5F,0x77,0x8C,0x77,0x88,0x60,0x5C,0x4E,0x3E,0x31,0x1A,0x05}},

	{0x00,1,{0x00}},            
	{0xE2,20,{0x05,0x16,0x22,0x2C,0x3B,0x47,0x47,0x70,0x5F,0x77,0x8C,0x77,0x88,0x60,0x5C,0x4E,0x3E,0x31,0x1A,0x05}},	

		{0x00, 1 , {0x91}},
		{0xC5,  2 ,{0x19,0x52}},

		{0x00, 1 , {0xB3}},
		{0xC5,  1 ,{0x84}},

		{0x00, 1 , {0xBB}},
		{0xC5,  1 ,{0x8A}},

		{0x00, 1 , {0xB2}},
		{0xC5,  1 ,{0x40}},

		{0x00, 1 , {0x81}},
		{0xC4,  2 ,{0x82,0x0A}},

		{0x00, 1 , {0xC6}},
		{0xB0,  1 ,{0x03}},

	{0x00,1,{0xb1}},		
	{0xc6,1,{0x02}},

	{0x00,1,{0xb4}},		
	{0xc6,1,{0x10}},

	{0x00,1,{0x87}},         
	{0xc4,1,{0x18}},

		{0x00, 1 , {0xB2}},
		{0xF5,  2 ,{0x00,0x00}},

		{0x00, 1 , {0xB6}},
		{0xF5,  2 ,{0x00,0x00}},

		{0x00, 1 , {0x94}},
		{0xF5,  2 ,{0x00,0x00}},

	{0x00,1,{0xd2}},              
	{0xf5,2,{0x06,0x15}},

	{0x00,1,{0x90}},            
	{0xf5,4,{0x02,0x11,0x02,0x15}},

	{0x00,1,{0x90}},         
	{0xc5,1,{0x50}},  //{0xc5,1,{0x50}}, change by alaric, resolve face red

	{0x00,1,{0x94}},            
	{0xc5,1,{0x66}},

	{0x00,1,{0xb4}},            
	{0xc5,1,{0xcc}},		          

	{0x00,1,{0x80}},           
	{0xcb,11,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	
	{0x00,1,{0x90}},           
	{0xcb,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xa0}},          
	{0xcb,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xb0}},          
	{0xcb,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xc0}},       
	{0xcb,15,{0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xd0}},          
	{0xcb,15,{0x00,0x00,0x00,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05}},

	{0x00,1,{0xe0}},           
	{0xcb,14,{0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x05,0x05,0x05}},

	{0x00,1,{0xf0}},         
	{0xcb,11,{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}},

	{0x00,1,{0x80}},            
	{0xcc,15,{0x2d,0x2d,0x0a,0x0c,0x0e,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0x90}},            
	{0xcc,15,{0x00,0x00,0x00,0x2e,0x2e,0x02,0x04,0x2d,0x2d,0x09,0x0b,0x0d,0x0f,0x00,0x00}},

	{0x00,1,{0xa0}},            
	{0xcc,14,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2e,0x2e,0x01,0x03}},

	{0x00,1,{0xb0}},          
	{0xcc,15,{0x2d,0x2e,0x0f,0x0d,0x0b,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xc0}},
	{0xcc,15,{0x00,0x00,0x00,0x2e,0x2d,0x03,0x01,0x2d,0x2e,0x10,0x0e,0x0c,0x0a,0x00,0x00}},

	{0x00,1,{0xd0}},           
	{0xcc,14,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2e,0x2d,0x04,0x02}},

	{0x00,1,{0x80}},             
	{0xce,12,{0x8D,0x03,0x29,0x8C,0x03,0x29,0x8B,0x03,0x29,0x8A,0x03,0x29}},

	{0x00,1,{0x90}},          
	{0xce,14,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xa0}},           
	{0xce,14,{0x38,0x0B,0x8D,0x00,0x8d,0x29,0x00,0x38,0x0A,0x8D,0x01,0x8d,0x29,0x00}},   

	{0x00,1,{0xb0}},           
	{0xce,14,{0x38,0x09,0x8D,0x02,0x8d,0x29,0x00,0x38,0x08,0x8D,0x03,0x8d,0x29,0x00}},

	{0x00,1,{0xc0}},         
	{0xce,14,{0x38,0x07,0x8D,0x04,0x8d,0x29,0x00,0x38,0x06,0x8D,0x05,0x8d,0x29,0x00}},

	{0x00,1,{0xd0}},       
	{0xce,14,{0x38,0x05,0x8D,0x06,0x8d,0x29,0x00,0x38,0x04,0x8D,0x07,0x8d,0x29,0x00}},

	{0x00,1,{0x80}},           
	{0xcf,14,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0x90}},           
	{0xcf,14,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xa0}},          
	{0xcf,14,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xb0}},          
	{0xcf,14,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xc0}},         
	{0xcf,11,{0x01,0x01,0x20,0x20,0x00,0x00,0x01,0x01,0x00,0x00,0x00}},

	{0x00,1,{0xb5}},             
	{0xc5,6,{0x3c,0x01,0xff,0x3c,0x01,0xff}},  

	{0x00,1,{0xa0}},            
	{0xc1,1,{0x02}},

	{0x00,1,{0x80}},            
	{0xc4,1,{0x01}},

	{0x00,1,{0x92}},            
	{0xb3,1,{0x02}},

	{0x00,1,{0x90}},            
	{0xb6,1,{0xb6}},

	{0x00,1,{0xA2}},            
	{0xc4,1,{0x00}},

	{0x00,1,{0xA3}},            
	{0xc4,1,{0x02}},

	{0x00,1,{0xA4}},            
	{0xc4,1,{0x02}},

	{0x00,1,{0x88}},            
	{0xc4,1,{0x80}},

	{0x00,1,{0xc2}},            
	{0xf5,1,{0x00}},

	{0x00,1,{0xc3}},           
	{0xf5,1,{0x85}},
	{0x00, 1 , {0x00}},
	{0xFF, 3 ,{0xFF,0xFF,0xFF}},

	{0x35,1,{0}},
	{0x11,0,{}},
	{REGFLAG_DELAY, 120, {}},
	
	{0x29,0,{}},
	{REGFLAG_DELAY, 40, {}},


	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.


	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

#if 0
static struct LCM_setting_table lcm_sleep_out_setting[] = {
	// Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},

	// Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

//static int vcom = 0x3F ;
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {

		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {
                  //case 0xD9:
                       //table[i].para_list[0] = vcom ;
                       //dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                       //vcom += 1 ;
                       //break ;

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

	// enable tearing-free
	params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

#if defined(LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

	// DSI
	/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573

	params->dsi.DSI_WMEM_CONTI=0x3C;
	params->dsi.DSI_RMEM_CONTI=0x3E;

	params->dsi.packet_size=256;

	// Video mode setting
	params->dsi.intermediat_buffer_num = 2;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active                = 4;
	params->dsi.vertical_backporch                  = 20;
	params->dsi.vertical_frontporch                 = 16;
	params->dsi.vertical_active_line                = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active              = 10;
	params->dsi.horizontal_backporch                = 63;
	params->dsi.horizontal_frontporch               = 63;
	params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

	// Bit rate calculation
	params->dsi.PLL_CLOCK               = 205;

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}

static unsigned int lcm_compare_id(void)
{
	int   array[4];
	char  buffer[3];
	char  id0=0;
	char  id1=0;
	char  id2=0;
	char  id3=0;
                
#ifdef COMPATIBLE_BY_ADC
    int data[4] = {0,0,0,0};
    int res = 0;
    int channel_data = 0;
    int module_id = OTM1287A_MODULE_UNKNOWN;
#endif
	
	SET_RESET_PIN(0);
	MDELAY(200);
	SET_RESET_PIN(1);
	MDELAY(200);

	array[0] = 0x00053700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xA1,buffer, 5);

#if 0
	array[0] = 0x00033700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDB,buffer+1, 1);


	array[0] = 0x00033700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xDC,buffer+2, 1);
#endif
	id0 = buffer[0]; //should be 0x01
	id1 = buffer[1];//should be 0x8b
	id2 = buffer[2];//should be 0x12
	id3 = buffer[3];//should be 0x87
#ifdef BUILD_LK
	printf("%s, id0 = 0x%08x\n", __func__, id0);//should be 0x01
	printf("%s, id1 = 0x%08x\n", __func__, id1);//should be 0x8b
	printf("%s, id2 = 0x%08x\n", __func__, id2);//should be 0x12
	printf("%s, id3 = 0x%08x\n", __func__, id3);//should be 0x87
#else
	printk("%s, id0 = 0x%08x\n", __func__, id0);//should be 0x01
	printk("%s, id1 = 0x%08x\n", __func__, id1);//should be 0x8b
	printk("%s, id2 = 0x%08x\n", __func__, id2);//should be 0x12
	printk("%s, id3 = 0x%08x\n", __func__, id3);//should be 0x87
#endif

#ifdef COMPATIBLE_BY_ADC //add by darren
	res = IMM_GetOneChannelValue(12, data, NULL); //auxin2 -- 12
	if (res < 0)
	{
#ifdef BUILD_LK
		printf("READ ADC data %d\n", channel_data);
#endif
		return 0;
	}
	else
	{
		channel_data = data[0] * 1000 + data[1] * 10; //mv 	
	}
    #ifdef BUILD_LK
	printf("READ ADC data %d\n", channel_data);
    #else
	printk("READ ADC data %d\n", channel_data);
    #endif	
	if (channel_data >1300 && channel_data < 1900 )   
	{
	    module_id = OTM1287A_MODULE_BOYI; //BOYI modules
	} 
   
	if (((id2<<8 | id3) == OTM1287A_HD720_ID) && (module_id == OTM1287A_MODULE_BOYI))
    {
    #ifdef BUILD_LK
	printf("READ ADC return 1!\n");
    #else
	printk("READ ADC lk->return 1!");
    #endif	
		return 1;
    }
    else
        return 0;
#else
    return ((id2<<8 | id3) == OTM1287A_HD720_ID ? 1 : 0);
#endif

	//return ((id2<<8 | id3) == OTM1287A_HD720_ID ? 1 : 0);
}

static void lcm_init(void)
{
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}



static void lcm_suspend(void)
{
	unsigned int data_array[2];

	//add by darren beg 
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
	//add by darren end 
	
	data_array[0] = 0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	printk("kernel %s\n", __func__);
#endif
}


static void lcm_resume(void)
{
#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	printk("kernel %s\n", __func__);
#endif
//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
	lcm_init();
}

#ifdef LCM_DSI_CMD_MODE
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

#ifdef BUILD_LK
	printf("uboot %s\n", __func__);
#else
	printk("kernel %s\n", __func__);
#endif

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}
#endif


LCM_DRIVER af168_otm1287a_hd720_dsi_vdo_boyi_lcm_drv = {
	.name           = "af168_otm1287a_hd720_dsi_vdo_boyi",
	.set_util_funcs = lcm_set_util_funcs,
	.compare_id     = lcm_compare_id,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if defined(LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif
};
