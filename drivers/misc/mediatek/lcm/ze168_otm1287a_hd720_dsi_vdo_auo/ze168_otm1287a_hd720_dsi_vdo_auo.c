#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"
//#include <cust_gpio_usage.h>
#ifdef BUILD_LK
//	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
//#else
//	#include <mach/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

#define OTM1287A_HD720_ID    (0x40)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//add by darren for adc use
//#define COMPATIBLE_BY_ADC
#ifdef COMPATIBLE_BY_ADC
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
typedef enum {
    HX8394D_MODULE_OFILM = 0x00,
    HX8394D_MODULE_BYD = 0x01,
    HX8394D_MODULE_UNKNOWN = 0xFF,
}HX8394D_LCM_MODULE_TYPE;
#endif
//add end

#ifndef BUILD_LK
//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
#endif
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


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

#define REGFLAG_DELAY             								0xFC
#define REGFLAG_UDELAY             								0xFB

#define REGFLAG_END_OF_TABLE      								0xFD   // END OF REGISTERS MARKER
#define REGFLAG_RESET_LOW       								0xFE
#define REGFLAG_RESET_HIGH      								0xFF

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//static LCM_setting_table_V3 lcm_init_setting[] = {
//};

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
		params->dsi.mode   = SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
        #endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
	    params->dsi.data_format.color_order 	= LCM_COLOR_ORDER_RGB;
	    params->dsi.data_format.trans_seq   	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	    params->dsi.data_format.padding     	= LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
		params->dsi.vertical_sync_active				= 2;
		params->dsi.vertical_backporch					= 16;
		params->dsi.vertical_frontporch					= 14;//9
		params->dsi.vertical_active_line				= FRAME_HEIGHT;

		params->dsi.horizontal_sync_active				= 20;
		params->dsi.horizontal_backporch				= 85;
		params->dsi.horizontal_frontporch				= 85;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
    	params->dsi.ssc_disable							= 1;

#if (LCM_DSI_CMD_MODE)
	    params->dsi.PLL_CLOCK = 500; //this value must be in MTK suggested table
#else
	    params->dsi.PLL_CLOCK = 227; //208; //450; //this value must be in MTK suggested table
#endif
		//params->dsi.clk_lp_per_line_enable = 0;
		params->dsi.esd_check_enable = 0;
		params->dsi.customization_esd_check_enable = 1;

#if 0
		params->dsi.lcm_esd_check_table[0].cmd          = 0x0A; //0x0a
		params->dsi.lcm_esd_check_table[0].count        = 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c; //0x1c
#endif
		params->dsi.lcm_esd_check_table[0].cmd          = 0xAC;
		params->dsi.lcm_esd_check_table[0].count        = 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x00;
#if 0	
		params->dsi.lcm_esd_check_table[2].cmd          = 0x0E;
		params->dsi.lcm_esd_check_table[2].count        = 1;
		params->dsi.lcm_esd_check_table[2].para_list[0] = 0x80;
#endif
		params->dsi.cont_clock = 1;
}


static void lcm_init(void)
{
	unsigned int data_array[16];

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(50);

	
	data_array[0] = 0x00002300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00042902;
	data_array[1] = 0x018712FF;
	dsi_set_cmdq(data_array, 2, 1);//EXTC = 1
	
	data_array[0] = 0x80002300;
	dsi_set_cmdq(data_array, 1, 1);//Orise mode enable

	data_array[0] = 0x00032902;
	data_array[1] = 0x008712FF;
	dsi_set_cmdq(data_array, 2, 1);
//disable timeout
	data_array[0] = 0xa0002300;         
	dsi_set_cmdq(data_array, 1, 1);        
	data_array[0] = 0x02C12300;     
	dsi_set_cmdq(data_array, 1, 1);
//disable timeout

	data_array[0] = 0x92002300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x000230FF;
	dsi_set_cmdq(data_array, 2, 1);
#if 0
//////////////add ian top//////////// disable timeout
	data_array[0] = 0xa0002300;  									
	dsi_set_cmdq(data_array, 1, 1);										
											
	data_array[0] = 0x02C12300;  										
	dsi_set_cmdq(data_array, 1, 1);
//////////////add ian  end/////////////	disable timeout
#endif
//-------------------- panel setting --------------------//
	data_array[0] = 0x80002300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000A2902;
	data_array[1] = 0x006400C0;
	data_array[2] = 0x64001010;
	data_array[3] = 0x00001010;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x90002300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00072902; 
	data_array[1] = 0x005C00C0;
	data_array[2] = 0x00040001;
	dsi_set_cmdq(data_array, 3, 1); //Panel Timing Setting

	data_array[0] = 0xA2002300;
	dsi_set_cmdq(data_array, 1, 1);	

	data_array[0] = 0x00042902;  
	data_array[1] = 0x000001C0;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0xB3002300;  
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x005500C0;
	dsi_set_cmdq(data_array, 2, 1);  //Interval Scan Frame: 0 frame, column inversion



	data_array[0] = 0x81002300;  
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x55C12300;  
	dsi_set_cmdq(data_array, 1, 1); //frame rate:60Hz

//-------------------- power setting --------------------//

	data_array[0] = 0xA0002300;  ///dcdc setting
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x041005C4;
	data_array[2] = 0x11150502;
	data_array[3] = 0x02071005;
	data_array[4] = 0x00111505;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xB0002300;   //clamp voltage setting
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x000000C4;
	dsi_set_cmdq(data_array, 2, 1); 

	data_array[0] = 0x91002300;  //VGH=13V, VGL=-12V, pump ratio:VGH=6x, VGL=-5x
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x005229C5;
	dsi_set_cmdq(data_array, 2, 1); 

	data_array[0] = 0x00002300;  
	dsi_set_cmdq(data_array, 1, 1);//GVDD=4.204V, NGVDD=-4.204V

	data_array[0] = 0x00032902;
	data_array[1] = 0x009494D8;  //94=4.396
	dsi_set_cmdq(data_array, 2, 1); 

	data_array[0] = 0x00002300;   	//VCOM=-0.240V
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x58D92300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xb3002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x84C52300; 
	dsi_set_cmdq(data_array, 1, 1);//VDD_18V=1.7V, LVDSVDD=1.6V


	data_array[0] = 0xBB002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x8AC52300; 
	dsi_set_cmdq(data_array, 1, 1);   //LVD voltage level setting

	data_array[0] = 0x82002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0AC42300; 
	dsi_set_cmdq(data_array, 1, 1);//chopper

	data_array[0] = 0xC6002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x03B02300; 
	dsi_set_cmdq(data_array, 1, 1); //debounce
//-------------------- control setting --------------------//

	data_array[0] = 0x00002300;   //ID1
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x40D02300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x000000D1; 
	dsi_set_cmdq(data_array, 2, 1);  //ID2, ID3
//-------------------- power on setting --------------------//
	data_array[0] = 0x80002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00C42300; 
	dsi_set_cmdq(data_array, 1, 1); //source blanking frame = black, defacult='30'

	data_array[0] = 0x98002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x10C52300; 
	dsi_set_cmdq(data_array, 1, 1);   //vcom discharge=gnd:'10', '00'=disable

	data_array[0] = 0x81002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1); // ibias off


	data_array[0] = 0x83002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1); // lvd off


	data_array[0] = 0x85002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// gvdd off


	data_array[0] = 0x87002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// lvdsvdd off


	data_array[0] = 0x89002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// nvdd_18 off


	data_array[0] = 0x8B002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// en_vcom off


	data_array[0] = 0x95002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// pump3 off


	data_array[0] = 0x97002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// pump4 off

	data_array[0] = 0x99002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// pump5 off

	data_array[0] = 0xA1002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// gamma off

	data_array[0] = 0xA3002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// sd ibias off

	data_array[0] = 0xA5002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// sdpch off

	data_array[0] = 0xA7002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15F52300; 
	dsi_set_cmdq(data_array, 1, 1);// sdpch bias off

	data_array[0] = 0xAB002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x18F52300; 
	dsi_set_cmdq(data_array, 1, 1);// ddc osc off

	data_array[0] = 0x94002300;   
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x000000F5;
	dsi_set_cmdq(data_array, 2, 1); //VCL pump dis


	data_array[0] = 0xD2002300;   
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x001506F5;
	dsi_set_cmdq(data_array, 2, 1);    //VCL reg. en

	data_array[0] = 0xB2002300;   
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x000000F5;
	dsi_set_cmdq(data_array, 2, 1);     //VGLO1


	data_array[0] = 0xB6002300;   
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x000000F5;
	dsi_set_cmdq(data_array, 2, 1);     //VGLO2


	data_array[0] = 0xB4002300; 
	dsi_set_cmdq(data_array, 1, 1);//VGLO1/2 Pull low setting

	data_array[0] = 0xCCC52300; 
	dsi_set_cmdq(data_array, 1, 1);//d[7] vglo1 d[6] vglo2 => 0: pull vss, 1: pull vgl

//-------------------- for Power IC ---------------------------------
	data_array[0] = 0x90002300; 
	dsi_set_cmdq(data_array, 1, 1);  //Mode-3

	data_array[0] = 0x00052902;
	data_array[1] = 0x021102F5;
	data_array[2] = 0x00000015;
	dsi_set_cmdq(data_array, 3, 1); 

	data_array[0] = 0x90002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x50C52300; 
	dsi_set_cmdq(data_array, 1, 1);//2xVPNL, 1.5*=00, 2*=50, 3*=a0

	data_array[0] = 0x94002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x77C52300; 
	dsi_set_cmdq(data_array, 1, 1);  //Frequency
//-------------------- panel timing state control --------------------//
	data_array[0] = 0x80002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000C2902;
	data_array[1] = 0x000000CB;
	data_array[2] = 0x00000000;
	data_array[3] = 0x00000000;
	dsi_set_cmdq(data_array, 4, 1); 

	data_array[0] = 0x90002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0x000000CB;
	data_array[2] = 0x00000000;
	data_array[3] = 0x00000000;
	data_array[4] = 0x00FF00FF;
	dsi_set_cmdq(data_array,5, 1); 

	data_array[0] = 0xA0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0xFF00FFCB;
	data_array[2] = 0x00000000;
	data_array[3] = 0x00000000;
	data_array[4] = 0x00000000;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xB0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0x000000CB;
	data_array[2] = 0x00FF00FF;
	data_array[3] = 0x00FF00FF;
	data_array[4] = 0x00000000;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xC0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0x000000CB;
	data_array[2] = 0x00000000;
	data_array[3] = 0x05000505;
	data_array[4] = 0x05050505;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xD0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0x050505CB;
	data_array[2] = 0x00000505;
	data_array[3] = 0x00000000;
	data_array[4] = 0x05000000;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xE0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x050005CB;
	data_array[2] = 0x05050505;
	data_array[3] = 0x05050505;
	data_array[4] = 0x00000005;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xF0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000C2902;
	data_array[1] = 0xFFFFFFCB;
	data_array[2] = 0xFFFFFFFF;
	data_array[3] = 0xFFFFFFFF;
	dsi_set_cmdq(data_array, 4, 1); 
//-------------------- panel pad mapping control --------------------//
	data_array[0] = 0x80002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0x000000CC;
	data_array[2] = 0x00000000;
	data_array[3] = 0x11000705;
	data_array[4] = 0x0D171315;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0x90002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0x0B0F09CC;
	data_array[2] = 0x00000301;
	data_array[3] = 0x00000000;
	data_array[4] = 0x06000000;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xA0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x120008CC;
	data_array[2] = 0x0E181416;
	data_array[3] = 0x020C100A;
	data_array[4] = 0x00000004;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xB0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0x000000CC;
	data_array[2] = 0x00000000;
	data_array[3] = 0x14000204;
	data_array[4] = 0x0C161218;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xC0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0x0E0A10CC;
	data_array[2] = 0x00000608;
	data_array[3] = 0x00000000;
	data_array[4] = 0x03000000;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0xD0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x130001CC;
	data_array[2] = 0x0B151117;
	data_array[3] = 0x070D090F;
	data_array[4] = 0x00000005;
	dsi_set_cmdq(data_array, 5, 1); 

//-------------------- panel timing setting --------------------//
	data_array[0] = 0x80002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000D2902;
	data_array[1] = 0x280387CE;
	data_array[2] = 0x85280386;
	data_array[3] = 0x03842803;
	data_array[4] = 0x00000028;
	dsi_set_cmdq(data_array, 5, 1); 

	data_array[0] = 0x90002300; 
	dsi_set_cmdq(data_array, 1, 1); //panel VEND setting

	data_array[0] = 0x000F2902;
	data_array[1] = 0x28FC34CE;
	data_array[2] = 0x3428FD34;
	data_array[3] = 0xFF3428FE;
	data_array[4] = 0x00000028;
	dsi_set_cmdq(data_array, 5, 1); 


	data_array[0] = 0xA0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x050738CE;
	data_array[2] = 0x00280000;
	data_array[3] = 0x01050638;
	data_array[4] = 0x00002800;
	dsi_set_cmdq(data_array, 5, 1);  //panel CLKA1/2 setting

	data_array[0] = 0xB0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x050538CE;
	data_array[2] = 0x00280002;
	data_array[3] = 0x03050438;
	data_array[4] = 0x00002800;
	dsi_set_cmdq(data_array, 5, 1); 


	data_array[0] = 0xC0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x050338CE;
	data_array[2] = 0x00280004;
	data_array[3] = 0x05050238;
	data_array[4] = 0x00002800;
	dsi_set_cmdq(data_array, 5, 1); //panel CLKb1/2 setting


	data_array[0] = 0xD0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x050138CE;
	data_array[2] = 0x00280006;
	data_array[3] = 0x07050038;
	data_array[4] = 0x00002800;
	dsi_set_cmdq(data_array, 5, 1); //panel CLKb3/4 setting



	data_array[0] = 0x80002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x050738CF;
	data_array[2] = 0x25180000;
	data_array[3] = 0x01050638;
	data_array[4] = 0x00251800;
	dsi_set_cmdq(data_array, 5, 1); //panel CLKc1/2 setting

	data_array[0] = 0x90002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x050538CF;
	data_array[2] = 0x25180002;
	data_array[3] = 0x03050438;
	data_array[4] = 0x00251800;
	dsi_set_cmdq(data_array, 5, 1);  //panel CLKc3/4 setting

	data_array[0] = 0xA0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x050338CF;
	data_array[2] = 0x25180004;
	data_array[3] = 0x05050238;
	data_array[4] = 0x00251800;
	dsi_set_cmdq(data_array, 5, 1); //panel CLKd1/2 setting

	data_array[0] = 0xB0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000F2902;
	data_array[1] = 0x050138CF;
	data_array[2] = 0x25180006;
	data_array[3] = 0x07050038;
	data_array[4] = 0x00251800;
	dsi_set_cmdq(data_array, 5, 1);  //panel CLKd3/4 setting

	data_array[0] = 0xC0002300; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x000C2902;
	data_array[1] = 0x200101CF;
	data_array[2] = 0x01000020;
	data_array[3] = 0x08030081;
	dsi_set_cmdq(data_array, 4, 1); //panel ECLK setting

//-------------------- gamma --------------------//

	data_array[0] = 0x00002300;         
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00152902;
	data_array[1] = 0x2A1B04E1;    
	data_array[2] = 0x5C584B39;
	data_array[3] = 0x6E957B8A;  
	data_array[4] = 0x42446A59;  
	data_array[5] = 0x0A192433;    
	data_array[6] = 0x00000006;     
	dsi_set_cmdq(data_array, 7, 1);

	data_array[0] = 0x00002300;         
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00152902;
	data_array[1] = 0x2A1B04E2;    
	data_array[2] = 0x5C584B39;
	data_array[3] = 0x6E957B8A;  
	data_array[4] = 0x42436A59;  
	data_array[5] = 0x0A192433;    
	data_array[6] = 0x00000006;     
	dsi_set_cmdq(data_array, 7, 1);

#if 0
//.new add for  esd  lcd blinking
	data_array[0] = 0xC7002300;  									
	dsi_set_cmdq(data_array, 1, 1);										
											
	data_array[0] = 0x00CF2300;  										
	dsi_set_cmdq(data_array, 1, 1);
////
	data_array[0] = 0xB9002300;  									
	dsi_set_cmdq(data_array, 1, 1);										
											
	data_array[0] = 0x51B02300;  										
	dsi_set_cmdq(data_array, 1, 1);

	//add end for esd lcd blinking
#endif
	data_array[0] = 0x00002300;          //Orise mode disable
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0042902;
	data_array[1] = 0xFFFFFFFF;    
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00352300;          //te on 
	dsi_set_cmdq(data_array, 1, 1);


//add by gaomeitao
	data_array[0] = 0x00110500;		// Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	
	data_array[0] = 0x00290500;		// Display On
	dsi_set_cmdq(data_array, 1, 1);
	//MDELAY(50);



#ifdef BUILD_LK
	dprintf(0,"%s,lk enter\n", __func__);
#else
	printk("%s, enter\n", __func__);
#endif
}



static void lcm_suspend(void)
{
	unsigned int data_array[16];

	
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(50);

	
	data_array[0]=0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
}



static void lcm_resume(void)
{
	lcm_init();

    #ifdef BUILD_LK
	  printf("[LK]---cmd---otm1287a_hd720_dsi_vdo----%s------\n",__func__);
    #else
	  printk("[KERNEL]---cmd---otm1287a_hd720_dsi_vdo----%s------\n",__func__);
    #endif	
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


static unsigned int lcm_compare_id(void)
{
	 unsigned int array[4];
        char  buffer[3]; 
        char  id0=0; 
        //char  id1=0; 
	 //int  id=0; 
	 //unsigned int data_array[16];
                
#ifdef COMPATIBLE_BY_ADC
    int data[4] = {0,0,0,0};
    int res = 0;
    int channel_data = 0;
    int module_id = HX8394D_MODULE_UNKNOWN;
#endif

	 SET_RESET_PIN(1); 
        MDELAY(20); 
        SET_RESET_PIN(0); 
        MDELAY(10); 
        SET_RESET_PIN(1); 
        MDELAY(50); 


        array[0] = 0x00023700;// read id return two byte,version and id 
        dsi_set_cmdq(array, 1, 1); 
                
        read_reg_v2(0xda, buffer, 1); 
        id0 = buffer[0]; //should be 0x83 
        
        
#ifdef BUILD_LK
	 printf("%s, LK debug: otm1287a id = 0x%08x\n", __func__, buffer);
#else
	// printk("%s, kernel debug: hx8394d id = 0x%08x\n", __func__, buffer);
#endif

//	return (id1 == OTM1287A_HD720_ID ? 1 : 0);
#ifdef COMPATIBLE_BY_ADC //add by darren
	channel_data = PMIC_IMM_GetOneChannelValue(12, 5, 1); //auxin2 -- 12
    #ifdef BUILD_LK
	printf("READ ADC data %d\n", channel_data);
    #else
	printk("READ ADC data %d\n", channel_data);
    #endif	
	if (channel_data < 100 )   
	{
	    module_id = HX8394D_MODULE_OFILM; //OFILM modules
	} 
   
	if ((id1 == OTM1287A_HD720_ID) && (module_id == HX8394D_MODULE_OFILM))
    {
		return 1;
    }
    else
        return 0;
#else
    return (id0 == OTM1287A_HD720_ID ? 1 : 0);
#endif

}


/*
static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}

	char  buffer;
	read_reg_v2(0x0a, &buffer, 1);
	printk("%s, kernel debug: reg = 0x%08x\n", __func__, buffer);

	return FALSE;
	
#else
	return FALSE;
#endif

}


static unsigned int lcm_esd_recover(void)
{
	lcm_init();

	return TRUE;
}
*/

LCM_DRIVER ze168_otm1287a_hd720_dsi_vdo_auo_lcm_drv=
{
    .name           	= "ze168_otm1287a_hd720_dsi_vdo_auo",
    .set_util_funcs 	= lcm_set_util_funcs,
	.get_params     	= lcm_get_params,
    .init           	= lcm_init,/*tianma init fun.*/
    .suspend        	= lcm_suspend,
    .resume         	= lcm_resume,
    .compare_id     	= lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };

