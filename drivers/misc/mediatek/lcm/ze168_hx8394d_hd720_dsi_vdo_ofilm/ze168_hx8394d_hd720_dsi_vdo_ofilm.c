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

#define HX8394D_HD720_ID    (0x94)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//add by darren for adc use
#define COMPATIBLE_BY_ADC
#ifdef COMPATIBLE_BY_ADC
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
typedef enum {
    HX8394D_MODULE_OFILM = 0x00,
    HX8394D_MODULE_TCL = 0x01,
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
		
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch					= 9;
		params->dsi.vertical_frontporch					= 8;
		params->dsi.vertical_active_line				= FRAME_HEIGHT;

		params->dsi.horizontal_sync_active				= 20;
		params->dsi.horizontal_backporch				= 70;
		params->dsi.horizontal_frontporch				= 70;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
    	//params->dsi.ssc_disable							= 1;

#if (LCM_DSI_CMD_MODE)
	    params->dsi.PLL_CLOCK = 500; //this value must be in MTK suggested table
#else
	    params->dsi.PLL_CLOCK = 205; //208; //450; //this value must be in MTK suggested table
#endif
		//params->dsi.clk_lp_per_line_enable = 0;
		params->dsi.esd_check_enable = 1;
		params->dsi.customization_esd_check_enable = 1;
//		params->dsi.lcm_esd_check_table[0].cmd          = 0x09; //0x0a
//		params->dsi.lcm_esd_check_table[0].count        = 1;
//		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80; //0x1c
//		params->dsi.lcm_esd_check_table[0].para_list[1] = 0x73; //0x1c
//		params->dsi.lcm_esd_check_table[0].para_list[2] = 0x04; //0x1c
//		params->dsi.lcm_esd_check_table[0].para_list[3] = 0x00; //0x1c

		params->dsi.lcm_esd_check_table[0].cmd          = 0xd9;
		params->dsi.lcm_esd_check_table[0].count        = 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
}
/*
static void lcm_init_power(void)
{

}

static void lcm_suspend_power(void)
{

}
*/
static void lcm_init(void)
{
	//unsigned char cmd = 0x0;
	
	unsigned int data_array[16];


	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(120);

	// when phone initial , config output high, enable backlight drv chip  
	// push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);  
       //------------------B9h----------------// 
       data_array[0] = 0x00043902;                                                   
       data_array[1] = 0x9483FFB9;                                 
       dsi_set_cmdq(data_array, 2, 1); 
MDELAY(1);
       //------------------BAh----------------// 
       data_array[0] = 0x00033902;                                                   
       data_array[1] = 0x008373BA;                                 
       dsi_set_cmdq(data_array, 2, 1); 
        
       //------------------B1h----------------// 
       data_array[0] = 0x00103902;                           
       data_array[1] = 0x12126CB1;   
       data_array[2] = 0xF1110423;   //0xF111E424
       data_array[3] = 0x239f9980; 
       data_array[4] = 0x58D2C080;   
       dsi_set_cmdq(data_array, 5, 1); 
MDELAY(1);
       //------------------B2h----------------// 
       data_array[0] = 0x000C3902;                                           
       data_array[1] = 0x0E6400B2; 
       data_array[2] = 0x0823120D;    // 0x081C2207;         
       data_array[3] = 0x004D1C08;                         
       dsi_set_cmdq(data_array, 4, 1); 
MDELAY(1);
       //------------------B4h----------------// 
       data_array[0] = 0x000D3902;                           
       data_array[1] = 0x5CFF00B4;   
       data_array[2] = 0x5C5A5C5A; 
       data_array[3] = 0x0170015A; 
       data_array[4] = 0x00000070; 
       dsi_set_cmdq(data_array, 5, 1); 

       //------------------BCh----------------// 
       /*data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000007BC;                         
       dsi_set_cmdq(data_array, 2, 1); */

       //------------------BFh----------------// 
       data_array[0] = 0x00043902;                                           
       data_array[1] = 0x010E41BF;                         
       dsi_set_cmdq(data_array, 2, 1); 
        
        
       //------------------D3h----------------// 
       data_array[0] = 0x001F3902;                   
       data_array[1] = 0x000000D3;   
       data_array[2] = 0x10100000; 
       data_array[3] = 0x00011032; 
       data_array[4] = 0xC0133201; 
       data_array[5] = 0x10320000; 
       data_array[6] = 0x47000008; 
       data_array[7] = 0x47050504; 
       data_array[8] = 0x00470004; 
       data_array[9] = 0x00000000; 
       dsi_set_cmdq(data_array,10, 1); 

       //------------------D5h----------------// 
       data_array[0] = 0x002D3902;                   
       data_array[1] = 0x030001D5;   
       data_array[2] = 0x07040502; 
       data_array[3] = 0x23202106; 
       data_array[4] = 0x18181822; 
       data_array[5] = 0x18181818; 
       data_array[6] = 0x18181818; 
       data_array[7] = 0x18181818; 
       data_array[8] = 0x18181818; 
       data_array[9] = 0x18181818; 
       data_array[10] = 0x18181818; 
       data_array[11] = 0x18191918; 
       data_array[12] = 0x00000018; 
       dsi_set_cmdq(data_array,13, 1); 

       //------------------D6h----------------// 
       data_array[0] = 0x002D3902;                   
       data_array[1] = 0x040706D6;   
       data_array[2] = 0x00030205; 
       data_array[3] = 0x20232201; 
       data_array[4] = 0x18181821; 
       data_array[5] = 0x18181818; 
       data_array[6] = 0x18181818; 
       data_array[7] = 0x1B181818; 
       data_array[8] = 0x1818181B; 
       data_array[9] = 0x18181818; 
       data_array[10] = 0x18181818; 
       data_array[11] = 0x19181818; 
       data_array[12] = 0x00000019; 
       dsi_set_cmdq(data_array,13, 1); 
        
       //------------------E0h----------------// 
       data_array[0] = 0x002B3902;                   
       data_array[1] = 0x2B2707E0;   
       data_array[2] = 0x343F3634; 
       data_array[3] = 0x0C0B084B; 
       data_array[4] = 0x15110E17; 
       data_array[5] = 0x13061313; 
       data_array[6] = 0x27071D19; 
       data_array[7] = 0x3F36342B; 
       data_array[8] = 0x0B084B34; 
       data_array[9] = 0x110E170C; 
       data_array[10] = 0x06131315; 
       data_array[11] = 0x001D1913; 
       dsi_set_cmdq(data_array,12, 1);     


    //------------------C1h----------------// 
       data_array[0] = 0x002C3902;                   
       data_array[1] = 0x080001C1;   
       data_array[2] = 0x28201810; 
       data_array[3] = 0x48403830; 
       data_array[4] = 0x68605850; 
       data_array[5] = 0x88807870; 
       data_array[6] = 0xA8A09890; 
       data_array[7] = 0xC8C0B8B0; 
       data_array[8] = 0xE8E0D8D0; 
       data_array[9] = 0x00FFF8F0; 
       data_array[10] = 0x00000000; 
       data_array[11] = 0x00000000; 
       dsi_set_cmdq(data_array,12, 1);     
  	  
	   //------------BDh---------------------//
	   data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000001BD;                         
       dsi_set_cmdq(data_array, 2, 1); 

   	  //------------------C1h----------------// 
       data_array[0] = 0x002B3902;                   
       data_array[1] = 0x100800C1;   
       data_array[2] = 0x30282018; 
       data_array[3] = 0x50484038; 
       data_array[4] = 0x70686058; 
       data_array[5] = 0x90888078; 
       data_array[6] = 0xB0A8A098; 
       data_array[7] = 0xD0C8C0B8; 
       data_array[8] = 0xF0E8E0D8; 
       data_array[9] = 0x0000FFF8; 
       data_array[10] = 0x00000000; 
       data_array[11] = 0x00000000; 
       dsi_set_cmdq(data_array,12, 1);     
	   
	   //------------BDh---------------------//
	   data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000002BD;                         
       dsi_set_cmdq(data_array, 2, 1); 

   	  //------------------C1h----------------// 
       data_array[0] = 0x002B3902;                   
       data_array[1] = 0x100800C1;   
       data_array[2] = 0x30282018; 
       data_array[3] = 0x50484038; 
       data_array[4] = 0x70686058; 
       data_array[5] = 0x90888078; 
       data_array[6] = 0xB0A8A098; 
       data_array[7] = 0xD0C8C0B8; 
       data_array[8] = 0xF0E8E0D8; 
       data_array[9] = 0x0000FFF8; 
       data_array[10] = 0x00000000; 
       data_array[11] = 0x00000000; 
       dsi_set_cmdq(data_array,12, 1);     
	   
  
       //------------------CCh----------------// 
       data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000009CC;                         
       dsi_set_cmdq(data_array, 2, 1);         
MDELAY(1);
       //------------------C0h----------------// 
       data_array[0] = 0x00033902;                                           
       data_array[1] = 0x001430C0;                         
       dsi_set_cmdq(data_array, 2, 1);         

       //------------------C7h----------------// 
       data_array[0] = 0x00053902;                                           
       data_array[1] = 0x40C000C7; 
       data_array[2] = 0x000000C0;                                 
       dsi_set_cmdq(data_array, 3, 1); 
       //------------------BCh----------------// 
       data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000007BC;                         
       dsi_set_cmdq(data_array, 2, 1);         

       //------------------B6h----------------// 
       data_array[0] = 0x00033902;                                           
       data_array[1] = 0x007575B6;                         
       dsi_set_cmdq(data_array, 2, 1);         


       //------------------11h----------------//                 
       data_array[0] = 0x00110500;                           
       dsi_set_cmdq(data_array, 1, 1); 
       MDELAY(120);         
                  
       //------------------29h----------------//   
       data_array[0] = 0x00290500;                           
       dsi_set_cmdq(data_array, 1, 1); 
       MDELAY(10); 


#ifdef BUILD_LK
	dprintf(0,"%s,lk enter\n", __func__);
#else
	printk("%s, enter\n", __func__);
#endif
}



static void lcm_suspend(void)
{
	unsigned int data_array[16];

	//data_array[0]=0x00280500; // Display Off
	//dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);

	
	SET_RESET_PIN(1);	
	SET_RESET_PIN(0);
	MDELAY(5); // 1ms
	
	SET_RESET_PIN(1);
	MDELAY(120);     
	//lcm_util.set_gpio_out(GPIO_LCD_ENN, GPIO_OUT_ZERO);
	//lcm_util.set_gpio_out(GPIO_LCD_ENP, GPIO_OUT_ZERO); 
}



static void lcm_resume(void)
{
	//lcm_util.set_gpio_out(GPIO_LCD_ENN, GPIO_OUT_ONE);
	//lcm_util.set_gpio_out(GPIO_LCD_ENP, GPIO_OUT_ONE);
	lcm_init();

    #ifdef BUILD_LK
	  printf("[LK]---cmd---hx8394d_hd720_dsi_vdo----%s------\n",__func__);
    #else
	  printk("[KERNEL]---cmd---hx8394d_hd720_dsi_vdo----%s------\n",__func__);
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
        char  id1=0; 
	// int  id=0; 
	 unsigned int data_array[16];
                
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


        //------------------B9h----------------// 
        data_array[0] = 0x00043902;                                                   
        data_array[1] = 0x9483FFB9;                                 
        dsi_set_cmdq(data_array, 2, 1); 

        //------------------BAh----------------// 
        data_array[0] = 0x00033902;                                                   
        data_array[1] = 0x008373BA;                                 
        dsi_set_cmdq(data_array, 2, 1); 

        
        array[0] = 0x00013700;// read id return two byte,version and id 
        dsi_set_cmdq(array, 1, 1); 
                
        read_reg_v2(0xda, buffer, 1); 
        id0 = buffer[0]; //should be 0x83 
        
        array[0] = 0x00013700;// read id return two byte,version and id 
        dsi_set_cmdq(array, 1, 1); 
                
        read_reg_v2(0xdb, buffer, 1); 
        id1 = buffer[0]; //should be 0x94 

#ifdef BUILD_LK
	 printf("%s, LK debug: hx8394d id = 0x%08x\n", __func__, buffer);
#else
	// printk("%s, kernel debug: hx8394d id = 0x%08x\n", __func__, buffer);
#endif

//	return (id1 == HX8394D_HD720_ID ? 1 : 0);
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
	if (channel_data < 100 )   
	{
	    module_id = HX8394D_MODULE_OFILM; //OFILM modules
	} 
   
	if ((id1 == HX8394D_HD720_ID) && (module_id == HX8394D_MODULE_OFILM))
    {
		return 1;
    }
    else
        return 0;
#else
    return (id1 == HX8394D_HD720_ID ? 1 : 0);
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

	char  buffer[1];
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

LCM_DRIVER ze168_hx8394d_hd720_dsi_vdo_ofilm_lcm_drv=
{
    .name           	= "ze168_hx8394d_hd720_dsi_vdo_ofilm",
    .set_util_funcs 	= lcm_set_util_funcs,
	.get_params     	= lcm_get_params,
    .init           	= lcm_init,/*tianma init fun.*/
    .suspend        	= lcm_suspend,
    .resume         	= lcm_resume,
    .compare_id     	= lcm_compare_id,
    //.init_power		= lcm_init_power,
    //.resume_power = lcm_resume_power,
    //.suspend_power = lcm_suspend_power,
    //.esd_check = lcm_esd_check,
    //.set_backlight = lcm_setbacklight,
    // .ata_check		= lcm_ata_check,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
     //.switch_mode		= lcm_switch_mode,
    };

