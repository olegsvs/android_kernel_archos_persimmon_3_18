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
    	params->dsi.ssc_disable							= 1;

#if (LCM_DSI_CMD_MODE)
	    params->dsi.PLL_CLOCK = 500; //this value must be in MTK suggested table
#else
	    params->dsi.PLL_CLOCK = 205; //221//208; //450; //this value must be in MTK suggested table
#endif
		//params->dsi.clk_lp_per_line_enable = 0;
		params->dsi.esd_check_enable = 1;
		params->dsi.customization_esd_check_enable = 1;
//		params->dsi.lcm_esd_check_table[0].cmd          = 0x0a; //0x0a
//		params->dsi.lcm_esd_check_table[0].count        = 1;
//		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x1c; //0x1c
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
	//unsigned char data = 0xFF;
	unsigned int data_array[16];

	//int ret=0;

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
       data_array[0] = 0x00073902;                                                   
       data_array[1] = 0x680363BA;  
       data_array[2] = 0x00C0B26B;
       dsi_set_cmdq(data_array, 3, 1); 
        
       //------------------B1h----------------// 
       data_array[0] = 0x000B3902;                           
       data_array[1] = 0x721250B1;   
	   data_array[2] = 0x71543209;    //by TDT 0x71543209 1102
	   data_array[3] = 0x001f3d31;
       
       dsi_set_cmdq(data_array, 4, 1); 
       MDELAY(1);
       //------------------B2h----------------// 
       data_array[0] = 0x00073902;                                           
       data_array[1] = 0x648000B2; //0x648000B2;1102
       data_array[2] = 0x002F0D10;    // 0x081C2207;         
                        
       dsi_set_cmdq(data_array, 3, 1); 
       MDELAY(1);
       //------------------B4h----------------// 
       data_array[0] = 0x00163902;                           
       data_array[1] = 0x037903B4;   
       data_array[2] = 0x01510351; 
       data_array[3] = 0x00357E05; 
       data_array[4] = 0x0379033F; 
       data_array[5] = 0x05510351;
       data_array[6] = 0x00007E01;
       dsi_set_cmdq(data_array, 7, 1); 

       //------------------BCh----------------// 
       /*data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000007BC;                         
       dsi_set_cmdq(data_array, 2, 1); */

       //------------------BFh----------------// 
       data_array[0] = 0x00083902;                                           
       data_array[1] = 0x508140BF;  
       data_array[2] = 0x01FC1A00;
       dsi_set_cmdq(data_array, 3, 1); 
        
        
       //------------------D3h----------------// 
       data_array[0] = 0x00223902;                   
       data_array[1] = 0x0F0000D3;   
       data_array[2] = 0x082F010F; 
       data_array[3] = 0x0B103200; 
       data_array[4] = 0x15320B00; 
       data_array[5] = 0x32070507; 
       data_array[6] = 0x00000010; 
       data_array[7] = 0x0B0B3337; 
       data_array[8] = 0x07071037;

       data_array[9] = 0x00004010;
       dsi_set_cmdq(data_array,10, 1); 

       //------------------D5h----------------// 
       data_array[0] = 0x002D3902;                   
       data_array[1] = 0x181818D5;   
       data_array[2] = 0x18181818; 
       data_array[3] = 0x18181818; 
       data_array[4] = 0x18181818; 
       data_array[5] = 0x18181818; 
       data_array[6] = 0x02181818; 
       data_array[7] = 0x00181803; 
       data_array[8] = 0x04070601; 
       data_array[9] = 0x22212005; 
       data_array[10] = 0x18181823; 
       data_array[11] = 0x18181818; 
       data_array[12] = 0x00000018; 
       dsi_set_cmdq(data_array,13, 1); 

       //------------------E0h----------------// 
       data_array[0] = 0x003B3902;                   
       data_array[1] = 0x504F4FE0;   
       data_array[2] = 0x53525152;
       data_array[3] = 0xaca2994e;
       data_array[4] = 0xb6b5a8a4;
       data_array[5] = 0xb9bfbfb5;
       data_array[6] = 0x6466d1c4;
       data_array[7] = 0x7c6e6b68;
       data_array[8] = 0x4f4f7f7f;
       data_array[9] = 0x52515250;
       data_array[10] = 0xa2984e53;
       data_array[11] = 0xb5a8a4ac;
       data_array[12] = 0xbfbfb5b6;
       data_array[13] = 0x66d1c4b9;
       data_array[14] = 0x6e6b6864;
       data_array[15] = 0x007f7f7c;
       dsi_set_cmdq(data_array,16, 1);     

		//SET 3GAMMA 	//目视效果较接近
		data_array[0] = 0x00023902;                                           
			   
		data_array[1] = 0x000000BD;                
			   
		dsi_set_cmdq(data_array, 2, 1);
		 
		
		data_array[0] = 0x002c3902;
		data_array[1] = 0x080001c1;
		data_array[2] = 0x29221911;
		data_array[3] = 0x47403830;
		data_array[4] = 0x645c554e;
		data_array[5] = 0x837c746c;
		data_array[6] = 0xa199928a;
		data_array[7] = 0xbfb8b1a9;
		data_array[8] = 0xded7cec6;
		data_array[9] = 0x22f7eee6;
		data_array[10] = 0x79c40719;
		data_array[11] = 0x00fd6fe2;
		dsi_set_cmdq(data_array, 12, 1);
		  
       //------------------CCh----------------// 
       data_array[0] = 0x00023902;                                           
		data_array[1] = 0x000001BD;                         
			   
		dsi_set_cmdq(data_array, 2, 1);     
		 
		
		data_array[0] = 0x002b3902;
		data_array[1] = 0x100800c1;
		data_array[2] = 0x2f282018;
		data_array[3] = 0x4d463f37;
		data_array[4] = 0x6b635c54;
		data_array[5] = 0x89827b73;
		data_array[6] = 0xa8a09991;
		data_array[7] = 0xc5beb7b0;
		data_array[8] = 0xe5ddd6cd;
		data_array[9] = 0x8806f5ec;
		data_array[10] = 0x257d0517;
		data_array[11] = 0x0000635a;
		dsi_set_cmdq(data_array, 12, 1);
		 
		
		data_array[0] = 0x00023902;                                           
			   
		data_array[1] = 0x000002BD;                         
			   
		dsi_set_cmdq(data_array, 2, 1);     
		 
		
		data_array[0] = 0x002b3902;
		data_array[1] = 0x100800c1;
		data_array[2] = 0x31292018;
		data_array[3] = 0x51494139;
		data_array[4] = 0x71686059;
		data_array[5] = 0x90898179;
		data_array[6] = 0xb1a9a199;
		data_array[7] = 0xcfc7bfb8;
		data_array[8] = 0xefe7dfd8;
		data_array[9] = 0xc216fff7;
		data_array[10] = 0x0becbbd4;
		data_array[11] = 0x00c0f5ee;
		dsi_set_cmdq(data_array, 12, 1);
		 
		
		data_array[0] = 0x00023902;                                           
			   
		data_array[1] = 0x000000BD;           
			   
		dsi_set_cmdq(data_array, 2, 1);
		
		//set 3gamma end
 

		data_array[0] = 0x00023902;                                           
       data_array[1] = 0x00000BCC;                         
       dsi_set_cmdq(data_array, 2, 1);         
       MDELAY(1);

       //------------------c0h----------------// 
       data_array[0] = 0x00033902;                                           
       data_array[1] = 0x00731fc0;
       dsi_set_cmdq(data_array, 2, 1);         

       data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000002d4;                         
       dsi_set_cmdq(data_array, 2, 1);  
       
       data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000001BD;                         
       dsi_set_cmdq(data_array, 2, 1);    
       
       data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000060B1;                         
       dsi_set_cmdq(data_array, 2, 1);  
       
       data_array[0] = 0x00023902;                                           
       data_array[1] = 0x000000BD;                         
       dsi_set_cmdq(data_array, 2, 1);  
       
       data_array[0] = 0x00033902;
       data_array[1] = 0x00c0c0b6;
       dsi_set_cmdq(data_array, 2, 1);  
       
       data_array[0] = 0x00350500;
       dsi_set_cmdq(data_array, 1, 1);

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
	if ((channel_data > 800) && (channel_data < 1000) )   
	{
	    module_id = HX8394D_MODULE_TCL; //OFILM modules
	} 
   
	if ((id1 == HX8394D_HD720_ID) && (module_id == HX8394D_MODULE_TCL))
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

LCM_DRIVER ze168_hx8394f_hd720_dsi_vdo_tcl_lcm_drv=
{
    .name           	= "ze168_hx8394f_hd720_dsi_vdo_tcl",
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

