#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/upmu_common.h>
	#include <platform/upmu_hw.h>

	#include <platform/mt_gpio.h>
	#include <platform/mt_i2c.h>
	#include <platform/mt_pmic.h>
	#include <string.h>
#else
#ifdef CONFIG_MTK_LEGACY
	#include <mach/mt_pm_ldo.h>	/* hwPowerOn */
	#include <mach/upmu_common.h>
	#include <mach/upmu_sw.h>
	#include <mach/upmu_hw.h>
	#include <mach/mt_gpio.h>
#endif
#endif

#ifdef CONFIG_MTK_LEGACY
#include <cust_gpio_usage.h>
#ifndef CONFIG_FPGA_EARLY_PORTING
#include <cust_i2c.h>
#endif
#endif

#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL, fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                         (720)
#define FRAME_HEIGHT                                        (1280)
#define LCM_ID                       (0x1283)

#define REGFLAG_DELAY               (0XFE)
#define REGFLAG_END_OF_TABLE        (0x100) // END OF REGISTERS MARKER


#define LCM_DSI_CMD_MODE                                    0

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                           (lcm_util.udelay(n))
#define MDELAY(n)                                           (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                           lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


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
/*
{0xFF,3,{0x98,0x81,0x03}},
{0x01,1,{0x08}},
{0x02,1,{0x00}},
{0x03,1,{0x73}},
{0x04,1,{0x73}},
{0x05,1,{0x14}},
{0x06,1,{0x06}}, 
{0x07,1,{0x02}}, 
{0x08,1,{0x05}}, 
{0x09,1,{0x00}}, 
{0x0a,1,{0x0c}}, 
{0x0b,1,{0x00}},
{0x0c,1,{0x1c}},
{0x0d,1,{0x1c}},
{0x0e,1,{0x00}},
{0x0f,1,{0x0c}},
{0x10,1,{0x0c}},
{0x11,1,{0x01}},
{0x12,1,{0x01}},
{0x13,1,{0x1b}},
{0x14,1,{0x0b}},
{0x15,1,{0x00}},
{0x16,1,{0x00}},
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1a,1,{0x00}},
{0x1b,1,{0x00}},
{0x1c,1,{0x00}},
{0x1d,1,{0x00}},
{0x1e,1,{0xC8}},
{0x1f,1,{0x80}}, 
{0x20,1,{0x02}}, 
{0x21,1,{0x00}}, 
{0x22,1,{0x02}}, 
{0x23,1,{0x00}}, 
{0x24,1,{0x00}}, 
{0x25,1,{0x00}}, 
{0x26,1,{0x00}}, 
{0x27,1,{0x00}}, 
{0x28,1,{0xFB}}, 
{0x29,1,{0x43}}, 
{0x2a,1,{0x00}}, 
{0x2b,1,{0x00}}, 
{0x2c,1,{0x07}}, 
{0x2d,1,{0x07}}, 
{0x2e,1,{0xFF}}, 
{0x2f,1,{0xFF}}, 
{0x30,1,{0x11}}, 
{0x31,1,{0x00}}, 
{0x32,1,{0x00}}, 
{0x33,1,{0x00}}, 
{0x34,1,{0x84}}, 
{0x35,1,{0x80}}, 
{0x36,1,{0x07}}, 
{0x37,1,{0x00}}, 
{0x38,1,{0x00}}, 
{0x39,1,{0x00}}, 
{0x3a,1,{0x00}}, 
{0x3b,1,{0x00}}, 
{0x3c,1,{0x00}}, 
{0x3d,1,{0x00}}, 
{0x3e,1,{0x00}}, 
{0x3f,1,{0x00}}, 
{0x40,1,{0x00}}, 
{0x41,1,{0x00}}, 
{0x42,1,{0x00}}, 
{0x43,1,{0x80}}, 
{0x44,1,{0x08}},                                     
{0x50,1,{0x01}}, 
{0x51,1,{0x23}}, 
{0x52,1,{0x45}}, 
{0x53,1,{0x67}}, 
{0x54,1,{0x89}}, 
{0x55,1,{0xAB}}, 
{0x56,1,{0x01}}, 
{0x57,1,{0x23}}, 
{0x58,1,{0x45}}, 
{0x59,1,{0x67}}, 
{0x5a,1,{0x89}}, 
{0x5b,1,{0xAB}}, 
{0x5c,1,{0xCD}}, 
{0x5d,1,{0xEF}},                                       
{0x5e,1,{0x10}}, 
{0x5f,1,{0x02}}, 
{0x60,1,{0x02}}, 
{0x61,1,{0x02}}, 
{0x62,1,{0x02}}, 
{0x63,1,{0x02}}, 
{0x64,1,{0x02}}, 
{0x65,1,{0x02}}, 
{0x66,1,{0x08}}, 
{0x67,1,{0x09}}, 
{0x68,1,{0x02}}, 
{0x69,1,{0x0c}}, 
{0x6a,1,{0x0e}}, 
{0x6b,1,{0x0d}}, 
{0x6c,1,{0x0f}}, 
{0x6d,1,{0x10}}, 
{0x6e,1,{0x12}}, 
{0x6f,1,{0x11}}, 
{0x70,1,{0x13}}, 
{0x71,1,{0x06}}, 
{0x72,1,{0x07}}, 
{0x73,1,{0x02}}, 
{0x74,1,{0x02}}, 
{0x75,1,{0x02}}, 
{0x76,1,{0x02}}, 
{0x77,1,{0x02}}, 
{0x78,1,{0x02}}, 
{0x79,1,{0x02}}, 
{0x7a,1,{0x02}}, 
{0x7b,1,{0x02}}, 
{0x7c,1,{0x07}}, 
{0x7d,1,{0x06}}, 
{0x7e,1,{0x02}}, 
{0x7f,1,{0x0f}}, 
{0x80,1,{0x0d}}, 
{0x81,1,{0x0e}}, 
{0x82,1,{0x0c}},
{0x83,1,{0x11}},
{0x84,1,{0x13}},
{0x85,1,{0x10}},
{0x86,1,{0x12}},
{0x87,1,{0x09}},
{0x88,1,{0x08}},
{0x89,1,{0x02}},
{0x8A,1,{0x02}},
{0xFF,3,{0x98,0x81,0x04}},
{0x6C,1,{0x15}},        
{0x6E,1,{0x2B}},
{0x6F,1,{0x35}},
{0x3A,1,{0x24}},
{0x8D,1,{0x1F}},
{0x87,1,{0xBA}},
{0x26,1,{0x76}},
{0xB2,1,{0xD1}},
{0xFF,3,{0x98,0x81,0x01}},
{0x22,1,{0x3a}},
{0x31,1,{0x0B}},
{0x53,1,{0x4C}},
{0x55,1,{0x8F}},
{0x50,1,{0x96}},
{0x51,1,{0x96}},
{0x60,1,{0x14}},                                      
{0xA0,1,{0x08}},
{0xA1,1,{0x12}},
{0xA2,1,{0x1B}},
{0xA3,1,{0x10}},
{0xA4,1,{0x11}},
{0xA5,1,{0x21}},
{0xA6,1,{0x14}},
{0xA7,1,{0x19}},
{0xA8,1,{0x63}},
{0xA9,1,{0x1C}},
{0xAA,1,{0x29}},
{0xAB,1,{0x61}},
{0xAC,1,{0x17}},
{0xAD,1,{0x10}},
{0xAE,1,{0x3D}},
{0xAF,1,{0x08}},
{0xB0,1,{0x0A}},
{0xB1,1,{0x57}},
{0xB2,1,{0x67}},
{0xB3,1,{0x39}},                                      
{0xC0,1,{0x08}}, 
{0xC1,1,{0x12}}, 
{0xC2,1,{0x1B}}, 
{0xC3,1,{0x10}}, 
{0xC4,1,{0x10}}, 
{0xC5,1,{0x20}}, 
{0xC6,1,{0x15}}, 
{0xC7,1,{0x19}}, 
{0xC8,1,{0x64}}, 
{0xC9,1,{0x1C}}, 
{0xCA,1,{0x29}}, 
{0xCB,1,{0x60}}, 
{0xCC,1,{0x17}}, 
{0xCD,1,{0x11}}, 
{0xCE,1,{0x3D}}, 
{0xCF,1,{0x09}}, 
{0xD0,1,{0x0A}}, 
{0xD1,1,{0x57}}, 
{0xD2,1,{0x67}}, 
{0xD3,1,{0x39}}, 
{0xFF,3,{0x98,0x81,0x00}},
*/


{0xFF,3,{0x98,0x81,0x03}},
{0x01,1,{0x08}},
{0x02,1,{0x00}},
{0x03,1,{0x73}},
{0x04,1,{0x73}},
{0x05,1,{0x14}},
{0x06,1,{0x06}},
{0x07,1,{0x02}},
{0x08,1,{0x05}},
{0x09,1,{0x00}},  
{0x0a,1,{0x0c}},  
{0x0b,1,{0x00}},  
{0x0c,1,{0x1c}},  
{0x0d,1,{0x1c}},  
{0x0e,1,{0x00}},  
{0x0f,1,{0x0c}},  
{0x10,1,{0x0c}},  
{0x11,1,{0x01}},  
{0x12,1,{0x01}},  
{0x13,1,{0x1b}},  
{0x14,1,{0x0b}},  
{0x15,1,{0x00}},  
{0x16,1,{0x00}},  
{0x17,1,{0x00}},  
{0x18,1,{0x00}},  
{0x19,1,{0x00}},  
{0x1a,1,{0x00}},  
{0x1b,1,{0x00}},  
{0x1c,1,{0x00}},  
{0x1d,1,{0x00}},  
{0x1e,1,{0xC8}},  
{0x1f,1,{0x80}},  
{0x20,1,{0x02}},  
{0x21,1,{0x00}},  
{0x22,1,{0x02}},  
{0x23,1,{0x00}},  
{0x24,1,{0x00}},  
{0x25,1,{0x00}},  
{0x26,1,{0x00}},  
{0x27,1,{0x00}},  
{0x28,1,{0xFB}},  
{0x29,1,{0x43}},  
{0x2a,1,{0x00}},  
{0x2b,1,{0x00}},  
{0x2c,1,{0x07}},  
{0x2d,1,{0x07}},  
{0x2e,1,{0xFF}},  
{0x2f,1,{0xFF}},  
{0x30,1,{0x11}},  
{0x31,1,{0x00}},  
{0x32,1,{0x00}},  
{0x33,1,{0x00}},  
{0x34,1,{0x84}},  
{0x35,1,{0x80}},  
{0x36,1,{0x07}},  
{0x37,1,{0x00}},  
{0x38,1,{0x00}},  
{0x39,1,{0x00}},  
{0x3a,1,{0x00}},  
{0x3b,1,{0x00}},  
{0x3c,1,{0x00}},  
{0x3d,1,{0x00}},  
{0x3e,1,{0x00}},  
{0x3f,1,{0x00}},  
{0x40,1,{0x00}},  
{0x41,1,{0x00}},  
{0x42,1,{0x00}},  
{0x43,1,{0x80}},  
{0x44,1,{0x08}},  
               
{0x50,1,{0x01}},  
{0x51,1,{0x23}},  
{0x52,1,{0x45}},  
{0x53,1,{0x67}},  
{0x54,1,{0x89}},  
{0x55,1,{0xAB}},  
{0x56,1,{0x01}},  
{0x57,1,{0x23}},  
{0x58,1,{0x45}},  
{0x59,1,{0x67}},  
{0x5a,1,{0x89}},  
{0x5b,1,{0xAB}},  
{0x5c,1,{0xCD}},  
{0x5d,1,{0xEF}},  
               
{0x5e,1,{0x10}},  
{0x5f,1,{0x02}},  
{0x60,1,{0x02}},  
{0x61,1,{0x02}},  
{0x62,1,{0x02}},  
{0x63,1,{0x02}},  
{0x64,1,{0x02}},  
{0x65,1,{0x02}},  
{0x66,1,{0x08}},  
{0x67,1,{0x09}},  
{0x68,1,{0x02}},  
{0x69,1,{0x0c}},  
{0x6a,1,{0x0e}},  
{0x6b,1,{0x0d}},  
{0x6c,1,{0x0f}},  
{0x6d,1,{0x10}},  
{0x6e,1,{0x12}},  
{0x6f,1,{0x11}},  
{0x70,1,{0x13}},  
{0x71,1,{0x06}},  
{0x72,1,{0x07}},  
{0x73,1,{0x02}},  
{0x74,1,{0x02}},  
{0x75,1,{0x02}},  
{0x76,1,{0x02}},  
{0x77,1,{0x02}},  
{0x78,1,{0x02}},  
{0x79,1,{0x02}},  
{0x7a,1,{0x02}},  
{0x7b,1,{0x02}},  
{0x7c,1,{0x07}},  
{0x7d,1,{0x06}},  
{0x7e,1,{0x02}},  
{0x7f,1,{0x0f}},  
{0x80,1,{0x0d}},  
{0x81,1,{0x0e}},  
{0x82,1,{0x0c}},  
{0x83,1,{0x11}},  
{0x84,1,{0x13}},  
{0x85,1,{0x10}},  
{0x86,1,{0x12}},  
{0x87,1,{0x09}},  
{0x88,1,{0x08}},  
{0x89,1,{0x02}},  
{0x8A,1,{0x02}},

{0xFF,3,{0x98,0x81,0x04}},
{0x6C,1,{0x15}},
{0x6E,1,{0x2B}},   
{0x6F,1,{0x35}}, 
{0x3A,1,{0x24}},  
{0x8D,1,{0x1F}},               //VGL clamp -11V=1A  -12V=1F
{0x87,1,{0xBA}},               //ESD  
{0x26,1,{0x76}},            
{0xB2,1,{0xD1}},



{0xFF,3,{0x98,0x81,0x01}},
{0x22,1,{0x3A}},               //  0A
{0x31,1,{0x0B}},     
{0x53,1,{0x4C}},		//VCOM1
{0x55,1,{0x8F}},		//VCOM2      
{0x50,1,{0x96}},		//VREG1OUT=5V
{0x51,1,{0x96}},		//VREG2OUT=-5V
{0x60,1,{0x14}},		//SDT  14

{0xA0,1,{0x08}},		//VP255	Gamma P
{0xA1,1,{0x12}},		//VP251
{0xA2,1,{0x1B}},		//VP247
{0xA3,1,{0x10}},		//VP243
{0xA4,1,{0x11}},               //VP239 
{0xA5,1,{0x21}},               //VP231
{0xA6,1,{0x14}},               //VP219
{0xA7,1,{0x19}},               //VP203
{0xA8,1,{0x63}},               //VP175
{0xA9,1,{0x1C}},               //VP144
{0xAA,1,{0x29}},               //VP111
{0xAB,1,{0x61}},               //VP80
{0xAC,1,{0x17}},               //VP52
{0xAD,1,{0x10}},               //VP36
{0xAE,1,{0x3D}},               //VP24
{0xAF,1,{0x08}},               //VP16
{0xB0,1,{0x0A}},               //VP12
{0xB1,1,{0x57}},               //VP8
{0xB2,1,{0x67}},               //VP4
{0xB3,1,{0x39}},               //VP0
             
{0xC0,1,{0x08}},		
{0xC1,1,{0x12}},               //VN251     
{0xC2,1,{0x1B}},               //VN247     
{0xC3,1,{0x10}},               //VN243     
{0xC4,1,{0x10}},               //VN239     
{0xC5,1,{0x20}},               //VN231     
{0xC6,1,{0x15}},               //VN219     
{0xC7,1,{0x19}},               //VN203     
{0xC8,1,{0x64}},               //VN175     
{0xC9,1,{0x1C}},               //VN144     
{0xCA,1,{0x29}},               //VN111     
{0xCB,1,{0x60}},               //VN80      
{0xCC,1,{0x17}},               //VN52      
{0xCD,1,{0x11}},               //VN36      
{0xCE,1,{0x3D}},               //VN24      
{0xCF,1,{0x09}},               //VN16      
{0xD0,1,{0x0A}},               //VN12      
{0xD1,1,{0x57}},               //VN8       
{0xD2,1,{0x67}},               //VN4       
{0xD3,1,{0x39}},               //VN0  


{0xFF,3,{0x98,0x81,0x00}},
{0x11,01,{0x00}},
{REGFLAG_DELAY, 120, {}},  
{0x29,01,{0x00}},	
{REGFLAG_DELAY, 20, {}},  
{REGFLAG_END_OF_TABLE, 0x00, {}} 

};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    // Display ON
    //{0x2C, 1, {0x00}},
    //{0x13, 1, {0x00}},
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
/*
static struct LCM_setting_table lcm_compare_id_setting[] = {
    // Display off sequence
    {0xf0, 5, {0x55, 0xaa, 0x52, 0x08, 0x01}},
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
    {0x51, 1, {0xFF}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/
//static int vcom=0x40;
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++) {
        
        unsigned cmd;
        cmd = table[i].cmd;
        
        switch (cmd) {
			/*case 0xd9:
			table[i].para_list[0]=vcom;
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
            vcom+=2;
			break;
			*/
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
    params->dbi.te_mode             = LCM_DBI_TE_MODE_DISABLED;
    params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;



    params->dsi.mode   = SYNC_EVENT_VDO_MODE;


    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine. 
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST; 
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
    // Highly depends on LCD driver capability.
    // Not support in MT6573
    params->dsi.packet_size=256;
    // Video mode setting       
    params->dsi.intermediat_buffer_num = 2;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active				= 10; //8;	//2;
	params->dsi.vertical_backporch					= 20; //18;	//14;
	params->dsi.vertical_frontporch					= 10; //20;	//16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 40;	//2;
	params->dsi.horizontal_backporch				= 100;//120;	//60;	//42;
	params->dsi.horizontal_frontporch				= 80;//100;	//60;	//44;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

// Bit rate calculation
//1 Every lane speed
//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
//params->dsi.fbk_div =0x12;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

// zhangxiaofei add for test
params->dsi.PLL_CLOCK = 220;//208;	
}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
#ifdef BUILD_LK
    printf("[erick-lk]%s\n", __func__);
#else
    printk("[erick-k]%s\n", __func__);
#endif
}


static void lcm_suspend(void)
{
#ifndef BUILD_LK
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);   //wqtao. enable
    #ifdef BUILD_LK
        printf("[erick-lk]%s\n", __func__);
    #else
        printk("[erick-k]%s\n", __func__);
    #endif
#endif
}


static void lcm_resume(void)
{
#ifndef BUILD_LK
    push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
    #ifdef BUILD_LK
        printf("[erick-lk]%s\n", __func__);
    #else
        printk("[erick-k]%s\n", __func__);
    #endif
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

    data_array[0]= 0x00290508; //HW bug, so need send one HS packet
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0]= 0x002c3909;
    dsi_set_cmdq(data_array, 1, 0);
}
#endif

#if 0   //wqtao.        
static void lcm_setbacklight(unsigned int level)
{
    unsigned int default_level = 145;
    unsigned int mapped_level = 0;

    //for LGE backlight IC mapping table
    if(level > 255) 
            level = 255;

    if(level >0) 
            mapped_level = default_level+(level)*(255-default_level)/(255);
    else
            mapped_level=0;

    // Refresh value of backlight level.
    lcm_backlight_level_setting[0].para_list[0] = mapped_level;

    push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}
#endif

/*static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
    if(lcm_esd_test)
    {
        lcm_esd_test = FALSE;
        return TRUE;
    }

    /// please notice: the max return packet size is 1
    /// if you want to change it, you can refer to the following marked code
    /// but read_reg currently only support read no more than 4 bytes....
    /// if you need to read more, please let BinHan knows.
    
            unsigned int data_array[16];
            unsigned int max_return_size = 1;
            
            data_array[0]= 0x00003700 | (max_return_size << 16);    
            
            dsi_set_cmdq(&data_array, 1, 1);
    

    if(read_reg(0x0a) == 0x9c)
    {
        return FALSE;
    }
    else
    {            
        return TRUE;
    }
#endif
}

static unsigned int lcm_esd_recover(void)
{
    unsigned char para = 0;

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(120);
      push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(10);
      push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(10);
    dsi_set_cmdq_V2(0x35, 1, &para, 1);     ///enable TE
    MDELAY(10);

    return TRUE;
}

*/
static unsigned int lcm_compare_id(void)
{
		return 1;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER ili9881c_hd720_cf1_fs650 = 
{
    .name           = "ili9881c_hd720_cf1_fs650",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,   
    .compare_id    = lcm_compare_id,    
};

