#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <asm-generic/gpio.h>

#include "lcm_drv.h"
#include "ddp_irq.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
//	#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)

#define LCM_ID_NT35590 (0x90)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};
static unsigned int GPIO_LCD_PWR_EN;


#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#define REGFLAG_DELAY             								0xFC
#define REGFLAG_UDELAY             								0xFB

#define REGFLAG_END_OF_TABLE      							    0xFD   // 


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)   

#define dsi_lcm_set_gpio_out(pin, out)										lcm_util.set_gpio_out(pin, out)
#define dsi_lcm_set_gpio_mode(pin, mode)									lcm_util.set_gpio_mode(pin, mode)
#define dsi_lcm_set_gpio_dir(pin, dir)										lcm_util.set_gpio_dir(pin, dir)
#define dsi_lcm_set_gpio_pull_enable(pin, en)								lcm_util.set_gpio_pull_enable(pin, en)

#define   LCM_DSI_CMD_MODE							0

static bool lcm_is_init = false;

																			
struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {

{0xFF,1,{0xEE}},//
{0xFB,1,{0x01}},//
{0x18,1,{0x40}},//
{REGFLAG_DELAY, 10,  {0x0}},
{0x18,1,{0x00}},
{REGFLAG_DELAY, 20,  {0x0}},

{0xFF,1,{0x05}},//
{0xFB,1,{0x01}},//
{0xC5,1,{0x31}},//TurnonNT50198
{REGFLAG_DELAY, 40,  {0x0}},//wait more than 120ms

{0xFF,1,{0xEE}},
{0xFB,1,{0x01}},
{0x24,1,{0x2F}},
{0x25,1,{0x02}},
{0x38,1,{0xC8}},
{0x39,1,{0x27}},
{0x1E,1,{0x77}},
{0x1D,1,{0x0F}},
{0x7E,1,{0x71}},
{0x7C,1,{0x31}},
	
{0xFF,1,{0x01}},//CMDpageselect
{0xFB,1,{0x01}},//NON-RELOADCMD
{0x00,1,{0x01}},//
{0x01,1,{0x55}},//
{0x02,1,{0x40}},//
{0x05,1,{0x50}},//00
{0x06,1,{0x4A}},//1B//ModifiedVGHLevel
{0x07,1,{0x29}},//
{0x08,1,{0x0C}},//
{0x0B,1,{0x9B}},//
{0x0C,1,{0x9B}},//
{0x0E,1,{0xB0}},//
{0x0F,1,{0xB3}},//AE//SetVGLO=-8.0V
{0x11,1,{0x10}},//VCOMDC,noneedtoissueforOTPLCM
{0x12,1,{0x10}},//
{0x13,1,{0x03}},//05//VCOMforForwardandBackwardScan
{0x14,1,{0x4A}},//
{0x15,1,{0x12}},//18//SetAVDDR=4.7V
{0x16,1,{0x12}},//18//SetAVEER=-4.7V
{0x18,1,{0x00}},//
{0x19,1,{0x77}},//
{0x1A,1,{0x55}},//
{0x1B,1,{0x13}},//
{0x1C,1,{0x00}},//
{0x1D,1,{0x00}},//
{0x1E,1,{0x00}},//
{0x1F,1,{0x00}},//
{0x58,1,{0x82}},//
{0x59,1,{0x02}},//
{0x5A,1,{0x02}},//
{0x5B,1,{0x02}},//
{0x5C,1,{0x82}},//
{0x5D,1,{0x82}},//
{0x5E,1,{0x02}},//
{0x5F,1,{0x02}},//
{0x72,1,{0x31}},//
     
{0xFF,1,{0x05}},//CMDpageselect
{0xFB,1,{0x01}},//NON-RELOADCMD
{0x00,1,{0x01}},//
{0x01,1,{0x0B}},//
{0x02,1,{0x0C}},//
{0x03,1,{0x09}},//
{0x04,1,{0x0A}},//
{0x05,1,{0x00}},//
{0x06,1,{0x0F}},//
{0x07,1,{0x10}},//
{0x08,1,{0x00}},//
{0x09,1,{0x00}},//
{0x0A,1,{0x00}},//
{0x0B,1,{0x00}},//
{0x0C,1,{0x00}},//
{0x0D,1,{0x13}},//
{0x0E,1,{0x15}},//
{0x0F,1,{0x17}},//
{0x10,1,{0x01}},//
{0x11,1,{0x0B}},//
{0x12,1,{0x0C}},//
{0x13,1,{0x09}},//
{0x14,1,{0x0A}},//
{0x15,1,{0x00}},//
{0x16,1,{0x0F}},//
{0x17,1,{0x10}},//
{0x18,1,{0x00}},//
{0x19,1,{0x00}},//
{0x1A,1,{0x00}},//
{0x1B,1,{0x00}},//
{0x1C,1,{0x00}},//
{0x1D,1,{0x13}},//
{0x1E,1,{0x15}},//
{0x1F,1,{0x17}},//
{0x20,1,{0x00}},//
{0x21,1,{0x03}},//
{0x22,1,{0x01}},//
{0x23,1,{0x36}},//
{0x24,1,{0x36}},//
{0x25,1,{0xED}},//
{0x29,1,{0x58}},//
{0x2A,1,{0x12}},//
{0x2B,1,{0x01}},//
{0x4B,1,{0x06}},//
{0x4C,1,{0x11}},//
{0x4D,1,{0x20}},//
{0x4E,1,{0x02}},//
{0x4F,1,{0x02}},//
{0x50,1,{0x20}},//
{0x51,1,{0x61}},//
{0x52,1,{0x01}},//
{0x53,1,{0x72}},//
{0x54,1,{0x75}},//
{0x55,1,{0xED}},//
{0x5B,1,{0x00}},//
{0x5C,1,{0x00}},//
{0x5F,1,{0x15}},//
{0x60,1,{0x75}},//
{0x63,1,{0x00}},//
{0x64,1,{0x00}},//
{0x67,1,{0x00}},//
{0x68,1,{0x04}},//
{0x6C,1,{0x40}},//
{0x7A,1,{0x80}},//
{0x7B,1,{0xA3}},//C5//ModifiedVBP,VFPDummyLines
{0x7C,1,{0xD8}},//
{0x7D,1,{0x60}},//
{0x7E,1,{0x08}},//
{0x7F,1,{0x1C}},//10//ModifiedMUXPulseWidth
{0x80,1,{0x00}},//
{0x83,1,{0x00}},//
{0x93,1,{0x08}},//
{0x94,1,{0x0A}},//
{0x8A,1,{0x00}},//
{0x9B,1,{0x0F}},//
{0xE7,1,{0x80}},//
       
{0xFF,1,{0x01}},//CMDpageselect
{0xFB,1,{0x01}},//NON-RELOADCMD
{0x75,1,{0x00}},//GammaR+CTRL1
{0x76,1,{0x18}},//GammaR+CTRL2
{0x77,1,{0x00}},//GammaR+CTRL3
{0x78,1,{0x38}},//GammaR+CTRL4
{0x79,1,{0x00}},//GammaR+CTRL5
{0x7A,1,{0x65}},//GammaR+CTRL6
{0x7B,1,{0x00}},//GammaR+CTRL7
{0x7C,1,{0x84}},//GammaR+CTRL8
{0x7D,1,{0x00}},//GammaR+CTRL9
{0x7E,1,{0x9B}},//GammaR+CTRL10
{0x7F,1,{0x00}},//GammaR+CTRL11
{0x80,1,{0xAF}},//GammaR+CTRL12
{0x81,1,{0x00}},//GammaR+CTRL13
{0x82,1,{0xC1}},//GammaR+CTRL14
{0x83,1,{0x00}},//GammaR+CTRL15
{0x84,1,{0xD2}},//GammaR+CTRL16
{0x85,1,{0x00}},//GammaR+CTRL17
{0x86,1,{0xDF}},//GammaR+CTRL18
{0x87,1,{0x01}},//GammaR+CTRL19
{0x88,1,{0x11}},//GammaR+CTRL20
{0x89,1,{0x01}},//GammaR+CTRL21
{0x8A,1,{0x38}},//GammaR+CTRL22
{0x8B,1,{0x01}},//GammaR+CTRL23
{0x8C,1,{0x76}},//GammaR+CTRL24
{0x8D,1,{0x01}},//GammaR+CTRL25
{0x8E,1,{0xA7}},//GammaR+CTRL26
{0x8F,1,{0x01}},//GammaR+CTRL27
{0x90,1,{0xF3}},//GammaR+CTRL28
{0x91,1,{0x02}},//GammaR+CTRL29
{0x92,1,{0x2F}},//GammaR+CTRL30
{0x93,1,{0x02}},//GammaR+CTRL31
{0x94,1,{0x30}},//GammaR+CTRL32
{0x95,1,{0x02}},//GammaR+CTRL33
{0x96,1,{0x66}},//GammaR+CTRL34
{0x97,1,{0x02}},//GammaR+CTRL35
{0x98,1,{0xA0}},//GammaR+CTRL36
{0x99,1,{0x02}},//GammaR+CTRL37
{0x9A,1,{0xC5}},//GammaR+CTRL38
{0x9B,1,{0x02}},//GammaR+CTRL39
{0x9C,1,{0xF8}},//GammaR+CTRL40
{0x9D,1,{0x03}},//GammaR+CTRL41
{0x9E,1,{0x1B}},//GammaR+CTRL42
{0x9F,1,{0x03}},//GammaR+CTRL43
{0xA0,1,{0x46}},//GammaR+CTRL44
{0xA2,1,{0x03}},//GammaR+CTRL45
{0xA3,1,{0x52}},//GammaR+CTRL46
{0xA4,1,{0x03}},//GammaR+CTRL47
{0xA5,1,{0x62}},//GammaR+CTRL48
{0xA6,1,{0x03}},//GammaR+CTRL49
{0xA7,1,{0x71}},//GammaR+CTRL50
{0xA9,1,{0x03}},//GammaR+CTRL51
{0xAA,1,{0x83}},//GammaR+CTRL52
{0xAB,1,{0x03}},//GammaR+CTRL53
{0xAC,1,{0x94}},//GammaR+CTRL54
{0xAD,1,{0x03}},//GammaR+CTRL55
{0xAE,1,{0xA3}},//GammaR+CTRL56
{0xAF,1,{0x03}},//GammaR+CTRL57
{0xB0,1,{0xAD}},//GammaR+CTRL58
{0xB1,1,{0x03}},//GammaR+CTRL59
{0xB2,1,{0xCC}},//GammaR+CTRL60
{0xB3,1,{0x00}},//GammaR-CTRL1
{0xB4,1,{0x18}},//GammaR-CTRL2
{0xB5,1,{0x00}},//GammaR-CTRL3
{0xB6,1,{0x38}},//GammaR-CTRL4
{0xB7,1,{0x00}},//GammaR-CTRL5
{0xB8,1,{0x65}},//GammaR-CTRL6
{0xB9,1,{0x00}},//GammaR-CTRL7
{0xBA,1,{0x84}},//GammaR-CTRL8
{0xBB,1,{0x00}},//GammaR-CTRL9
{0xBC,1,{0x9B}},//GammaR-CTRL10
{0xBD,1,{0x00}},//GammaR-CTRL11
{0xBE,1,{0xAF}},//GammaR-CTRL12
{0xBF,1,{0x00}},//GammaR-CTRL13
{0xC0,1,{0xC1}},//GammaR-CTRL14
{0xC1,1,{0x00}},//GammaR-CTRL15
{0xC2,1,{0xD2}},//GammaR-CTRL16
{0xC3,1,{0x00}},//GammaR-CTRL17
{0xC4,1,{0xDF}},//GammaR-CTRL18
{0xC5,1,{0x01}},//GammaR-CTRL19
{0xC6,1,{0x11}},//GammaR-CTRL20
{0xC7,1,{0x01}},//GammaR-CTRL21
{0xC8,1,{0x38}},//GammaR-CTRL22
{0xC9,1,{0x01}},//GammaR-CTRL23
{0xCA,1,{0x76}},//GammaR-CTRL24
{0xCB,1,{0x01}},//GammaR-CTRL25
{0xCC,1,{0xA7}},//GammaR-CTRL26
{0xCD,1,{0x01}},//GammaR-CTRL27
{0xCE,1,{0xF3}},//GammaR-CTRL28
{0xCF,1,{0x02}},//GammaR-CTRL29
{0xD0,1,{0x2F}},//GammaR-CTRL30
{0xD1,1,{0x02}},//GammaR-CTRL31
{0xD2,1,{0x30}},//GammaR-CTRL32
{0xD3,1,{0x02}},//GammaR-CTRL33
{0xD4,1,{0x66}},//GammaR-CTRL34
{0xD5,1,{0x02}},//GammaR-CTRL35
{0xD6,1,{0xA0}},//GammaR-CTRL36
{0xD7,1,{0x02}},//GammaR-CTRL37
{0xD8,1,{0xC5}},//GammaR-CTRL38
{0xD9,1,{0x02}},//GammaR-CTRL39
{0xDA,1,{0xF8}},//GammaR-CTRL40
{0xDB,1,{0x03}},//GammaR-CTRL41
{0xDC,1,{0x1B}},//GammaR-CTRL42
{0xDD,1,{0x03}},//GammaR-CTRL43
{0xDE,1,{0x46}},//GammaR-CTRL44
{0xDF,1,{0x03}},//GammaR-CTRL45
{0xE0,1,{0x52}},//GammaR-CTRL46
{0xE1,1,{0x03}},//GammaR-CTRL47
{0xE2,1,{0x62}},//GammaR-CTRL48
{0xE3,1,{0x03}},//GammaR-CTRL49
{0xE4,1,{0x71}},//GammaR-CTRL50
{0xE5,1,{0x03}},//GammaR-CTRL51
{0xE6,1,{0x83}},//GammaR-CTRL52
{0xE7,1,{0x03}},//GammaR-CTRL53
{0xE8,1,{0x94}},//GammaR-CTRL54
{0xE9,1,{0x03}},//GammaR-CTRL55
{0xEA,1,{0xA3}},//GammaR-CTRL56
{0xEB,1,{0x03}},//GammaR-CTRL57
{0xEC,1,{0xAD}},//GammaR-CTRL58
{0xED,1,{0x03}},//GammaR-CTRL59
{0xEE,1,{0xCC}},//GammaR-CTRL60
{0xEF,1,{0x00}},//GammaG+CTRL1
{0xF0,1,{0x18}},//GammaG+CTRL2
{0xF1,1,{0x00}},//GammaG+CTRL3
{0xF2,1,{0x38}},//GammaG+CTRL4
{0xF3,1,{0x00}},//GammaG+CTRL5
{0xF4,1,{0x65}},//GammaG+CTRL6
{0xF5,1,{0x00}},//GammaG+CTRL7
{0xF6,1,{0x84}},//GammaG+CTRL8
{0xF7,1,{0x00}},//GammaG+CTRL9
{0xF8,1,{0x9B}},//GammaG+CTRL10
{0xF9,1,{0x00}},//GammaG+CTRL11
{0xFA,1,{0xAF}},//GammaG+CTRL12
{0xFF,1,{0x02}},//CMDpageselect
{0xFB,1,{0x01}},//NON-RELOADCMD
{0x00,1,{0x00}},//GammaG+CTRL13
{0x01,1,{0xC1}},//GammaG+CTRL14
{0x02,1,{0x00}},//GammaG+CTRL15
{0x03,1,{0xD2}},//GammaG+CTRL16
{0x04,1,{0x00}},//GammaG+CTRL17
{0x05,1,{0xDF}},//GammaG+CTRL18
{0x06,1,{0x01}},//GammaG+CTRL19
{0x07,1,{0x11}},//GammaG+CTRL20
{0x08,1,{0x01}},//GammaG+CTRL21
{0x09,1,{0x38}},//GammaG+CTRL22
{0x0A,1,{0x01}},//GammaG+CTRL23
{0x0B,1,{0x76}},//GammaG+CTRL24
{0x0C,1,{0x01}},//GammaG+CTRL25
{0x0D,1,{0xA7}},//GammaG+CTRL26
{0x0E,1,{0x01}},//GammaG+CTRL27
{0x0F,1,{0xF3}},//GammaG+CTRL28
{0x10,1,{0x02}},//GammaG+CTRL29
{0x11,1,{0x2F}},//GammaG+CTRL30
{0x12,1,{0x02}},//GammaG+CTRL31
{0x13,1,{0x30}},//GammaG+CTRL32
{0x14,1,{0x02}},//GammaG+CTRL33
{0x15,1,{0x66}},//GammaG+CTRL34
{0x16,1,{0x02}},//GammaG+CTRL35
{0x17,1,{0xA0}},//GammaG+CTRL36
{0x18,1,{0x02}},//GammaG+CTRL37
{0x19,1,{0xC5}},//GammaG+CTRL38
{0x1A,1,{0x02}},//GammaG+CTRL39
{0x1B,1,{0xF8}},//GammaG+CTRL40
{0x1C,1,{0x03}},//GammaG+CTRL41
{0x1D,1,{0x1B}},//GammaG+CTRL42
{0x1E,1,{0x03}},//GammaG+CTRL43
{0x1F,1,{0x46}},//GammaG+CTRL44
{0x20,1,{0x03}},//GammaG+CTRL45
{0x21,1,{0x52}},//GammaG+CTRL46
{0x22,1,{0x03}},//GammaG+CTRL47
{0x23,1,{0x62}},//GammaG+CTRL48
{0x24,1,{0x03}},//GammaG+CTRL49
{0x25,1,{0x71}},//GammaG+CTRL50
{0x26,1,{0x03}},//GammaG+CTRL51
{0x27,1,{0x83}},//GammaG+CTRL52
{0x28,1,{0x03}},//GammaG+CTRL53
{0x29,1,{0x94}},//GammaG+CTRL54
{0x2A,1,{0x03}},//GammaG+CTRL55
{0x2B,1,{0xA3}},//GammaG+CTRL56
{0x2D,1,{0x03}},//GammaG+CTRL57
{0x2F,1,{0xAD}},//GammaG+CTRL58
{0x30,1,{0x03}},//GammaG+CTRL59
{0x31,1,{0xCC}},//GammaG+CTRL60
{0x32,1,{0x00}},//GammaG-CTRL1
{0x33,1,{0x18}},//GammaG-CTRL2
{0x34,1,{0x00}},//GammaG-CTRL3
{0x35,1,{0x38}},//GammaG-CTRL4
{0x36,1,{0x00}},//GammaG-CTRL5
{0x37,1,{0x65}},//GammaG-CTRL6
{0x38,1,{0x00}},//GammaG-CTRL7
{0x39,1,{0x84}},//GammaG-CTRL8
{0x3A,1,{0x00}},//GammaG-CTRL9
{0x3B,1,{0x9B}},//GammaG-CTRL10
{0x3D,1,{0x00}},//GammaG-CTRL11
{0x3F,1,{0xAF}},//GammaG-CTRL12
{0x40,1,{0x00}},//GammaG-CTRL13
{0x41,1,{0xC1}},//GammaG-CTRL14
{0x42,1,{0x00}},//GammaG-CTRL15
{0x43,1,{0xD2}},//GammaG-CTRL16
{0x44,1,{0x00}},//GammaG-CTRL17
{0x45,1,{0xDF}},//GammaG-CTRL18
{0x46,1,{0x01}},//GammaG-CTRL19
{0x47,1,{0x11}},//GammaG-CTRL20
{0x48,1,{0x01}},//GammaG-CTRL21
{0x49,1,{0x38}},//GammaG-CTRL22
{0x4A,1,{0x01}},//GammaG-CTRL23
{0x4B,1,{0x76}},//GammaG-CTRL24
{0x4C,1,{0x01}},//GammaG-CTRL25
{0x4D,1,{0xA7}},//GammaG-CTRL26
{0x4E,1,{0x01}},//GammaG-CTRL27
{0x4F,1,{0xF3}},//GammaG-CTRL28
{0x50,1,{0x02}},//GammaG-CTRL29
{0x51,1,{0x2F}},//GammaG-CTRL30
{0x52,1,{0x02}},//GammaG-CTRL31
{0x53,1,{0x30}},//GammaG-CTRL32
{0x54,1,{0x02}},//GammaG-CTRL33
{0x55,1,{0x66}},//GammaG-CTRL34
{0x56,1,{0x02}},//GammaG-CTRL35
{0x58,1,{0xA0}},//GammaG-CTRL36
{0x59,1,{0x02}},//GammaG-CTRL37
{0x5A,1,{0xC5}},//GammaG-CTRL38
{0x5B,1,{0x02}},//GammaG-CTRL39
{0x5C,1,{0xF8}},//GammaG-CTRL40
{0x5D,1,{0x03}},//GammaG-CTRL41
{0x5E,1,{0x1B}},//GammaG-CTRL42
{0x5F,1,{0x03}},//GammaG-CTRL43
{0x60,1,{0x46}},//GammaG-CTRL44
{0x61,1,{0x03}},//GammaG-CTRL45
{0x62,1,{0x52}},//GammaG-CTRL46
{0x63,1,{0x03}},//GammaG-CTRL47
{0x64,1,{0x62}},//GammaG-CTRL48
{0x65,1,{0x03}},//GammaG-CTRL49
{0x66,1,{0x71}},//GammaG-CTRL50
{0x67,1,{0x03}},//GammaG-CTRL51
{0x68,1,{0x83}},//GammaG-CTRL52
{0x69,1,{0x03}},//GammaG-CTRL53
{0x6A,1,{0x94}},//GammaG-CTRL54
{0x6B,1,{0x03}},//GammaG-CTRL55
{0x6C,1,{0xA3}},//GammaG-CTRL56
{0x6D,1,{0x03}},//GammaG-CTRL57
{0x6E,1,{0xAD}},//GammaG-CTRL58
{0x6F,1,{0x03}},//GammaG-CTRL59
{0x70,1,{0xCC}},//GammaG-CTRL60
{0x71,1,{0x00}},//GammaB+CTRL1
{0x72,1,{0x18}},//GammaB+CTRL2
{0x73,1,{0x00}},//GammaB+CTRL3
{0x74,1,{0x38}},//GammaB+CTRL4
{0x75,1,{0x00}},//GammaB+CTRL5
{0x76,1,{0x65}},//GammaB+CTRL6
{0x77,1,{0x00}},//GammaB+CTRL7
{0x78,1,{0x84}},//GammaB+CTRL8
{0x79,1,{0x00}},//GammaB+CTRL9
{0x7A,1,{0x9B}},//GammaB+CTRL10
{0x7B,1,{0x00}},//GammaB+CTRL11
{0x7C,1,{0xAF}},//GammaB+CTRL12
{0x7D,1,{0x00}},//GammaB+CTRL13
{0x7E,1,{0xC1}},//GammaB+CTRL14
{0x7F,1,{0x00}},//GammaB+CTRL15
{0x80,1,{0xD2}},//GammaB+CTRL16
{0x81,1,{0x00}},//GammaB+CTRL17
{0x82,1,{0xDF}},//GammaB+CTRL18
{0x83,1,{0x01}},//GammaB+CTRL19
{0x84,1,{0x11}},//GammaB+CTRL20
{0x85,1,{0x01}},//GammaB+CTRL21
{0x86,1,{0x38}},//GammaB+CTRL22
{0x87,1,{0x01}},//GammaB+CTRL23
{0x88,1,{0x76}},//GammaB+CTRL24
{0x89,1,{0x01}},//GammaB+CTRL25
{0x8A,1,{0xA7}},//GammaB+CTRL26
{0x8B,1,{0x01}},//GammaB+CTRL27
{0x8C,1,{0xF3}},//GammaB+CTRL28
{0x8D,1,{0x02}},//GammaB+CTRL29
{0x8E,1,{0x2F}},//GammaB+CTRL30
{0x8F,1,{0x02}},//GammaB+CTRL31
{0x90,1,{0x30}},//GammaB+CTRL32
{0x91,1,{0x02}},//GammaB+CTRL33
{0x92,1,{0x66}},//GammaB+CTRL34
{0x93,1,{0x02}},//GammaB+CTRL35
{0x94,1,{0xA0}},//GammaB+CTRL36
{0x95,1,{0x02}},//GammaB+CTRL37
{0x96,1,{0xC5}},//GammaB+CTRL38
{0x97,1,{0x02}},//GammaB+CTRL39
{0x98,1,{0xF8}},//GammaB+CTRL40
{0x99,1,{0x03}},//GammaB+CTRL41
{0x9A,1,{0x1B}},//GammaB+CTRL42
{0x9B,1,{0x03}},//GammaB+CTRL43
{0x9C,1,{0x46}},//GammaB+CTRL44
{0x9D,1,{0x03}},//GammaB+CTRL45
{0x9E,1,{0x52}},//GammaB+CTRL46
{0x9F,1,{0x03}},//GammaB+CTRL47
{0xA0,1,{0x62}},//GammaB+CTRL48
{0xA2,1,{0x03}},//GammaB+CTRL49
{0xA3,1,{0x71}},//GammaB+CTRL50
{0xA4,1,{0x03}},//GammaB+CTRL51
{0xA5,1,{0x83}},//GammaB+CTRL52
{0xA6,1,{0x03}},//GammaB+CTRL53
{0xA7,1,{0x94}},//GammaB+CTRL54
{0xA9,1,{0x03}},//GammaB+CTRL55
{0xAA,1,{0xA3}},//GammaB+CTRL56
{0xAB,1,{0x03}},//GammaB+CTRL57
{0xAC,1,{0xAD}},//GammaB+CTRL58
{0xAD,1,{0x03}},//GammaB+CTRL59
{0xAE,1,{0xCC}},//GammaB+CTRL60
{0xAF,1,{0x00}},//GammaB-CTRL1
{0xB0,1,{0x18}},//GammaB-CTRL2
{0xB1,1,{0x00}},//GammaB-CTRL3
{0xB2,1,{0x38}},//GammaB-CTRL4
{0xB3,1,{0x00}},//GammaB-CTRL5
{0xB4,1,{0x65}},//GammaB-CTRL6
{0xB5,1,{0x00}},//GammaB-CTRL7
{0xB6,1,{0x84}},//GammaB-CTRL8
{0xB7,1,{0x00}},//GammaB-CTRL9
{0xB8,1,{0x9B}},//GammaB-CTRL10
{0xB9,1,{0x00}},//GammaB-CTRL11
{0xBA,1,{0xAF}},//GammaB-CTRL12
{0xBB,1,{0x00}},//GammaB-CTRL13
{0xBC,1,{0xC1}},//GammaB-CTRL14
{0xBD,1,{0x00}},//GammaB-CTRL15
{0xBE,1,{0xD2}},//GammaB-CTRL16
{0xBF,1,{0x00}},//GammaB-CTRL17
{0xC0,1,{0xDF}},//GammaB-CTRL18
{0xC1,1,{0x01}},//GammaB-CTRL19
{0xC2,1,{0x11}},//GammaB-CTRL20
{0xC3,1,{0x01}},//GammaB-CTRL21
{0xC4,1,{0x38}},//GammaB-CTRL22
{0xC5,1,{0x01}},//GammaB-CTRL23
{0xC6,1,{0x76}},//GammaB-CTRL24
{0xC7,1,{0x01}},//GammaB-CTRL25
{0xC8,1,{0xA7}},//GammaB-CTRL26
{0xC9,1,{0x01}},//GammaB-CTRL27
{0xCA,1,{0xF3}},//GammaB-CTRL28
{0xCB,1,{0x02}},//GammaB-CTRL29
{0xCC,1,{0x2F}},//GammaB-CTRL30
{0xCD,1,{0x02}},//GammaB-CTRL31
{0xCE,1,{0x30}},//GammaB-CTRL32
{0xCF,1,{0x02}},//GammaB-CTRL33
{0xD0,1,{0x66}},//GammaB-CTRL34
{0xD1,1,{0x02}},//GammaB-CTRL35
{0xD2,1,{0xA0}},//GammaB-CTRL36
{0xD3,1,{0x02}},//GammaB-CTRL37
{0xD4,1,{0xC5}},//GammaB-CTRL38
{0xD5,1,{0x02}},//GammaB-CTRL39
{0xD6,1,{0xF8}},//GammaB-CTRL40
{0xD7,1,{0x03}},//GammaB-CTRL41
{0xD8,1,{0x1B}},//GammaB-CTRL42
{0xD9,1,{0x03}},//GammaB-CTRL43
{0xDA,1,{0x46}},//GammaB-CTRL44
{0xDB,1,{0x03}},//GammaB-CTRL45
{0xDC,1,{0x52}},//GammaB-CTRL46
{0xDD,1,{0x03}},//GammaB-CTRL47
{0xDE,1,{0x62}},//GammaB-CTRL48
{0xDF,1,{0x03}},//GammaB-CTRL49
{0xE0,1,{0x71}},//GammaB-CTRL50
{0xE1,1,{0x03}},//GammaB-CTRL51
{0xE2,1,{0x83}},//GammaB-CTRL52
{0xE3,1,{0x03}},//GammaB-CTRL53
{0xE4,1,{0x94}},//GammaB-CTRL54
{0xE5,1,{0x03}},//GammaB-CTRL55
{0xE6,1,{0xA3}},//GammaB-CTRL56
{0xE7,1,{0x03}},//GammaB-CTRL57
{0xE8,1,{0xAD}},//GammaB-CTRL58
{0xE9,1,{0x03}},//GammaB-CTRL59
{0xEA,1,{0xCC}},//GammaB-CTRL60

{0xFF, 1, {0x01}},
{0x01, 1, {0x55}},
{0x15, 1, {0x0F}},
{0x16, 1, {0x0F}},
{0x1B, 1, {0x1B}},
{0x1C, 1, {0xF7}},
{0x60, 1, {0x0F}},
{0x58, 1, {0x82}},
{0x59, 1, {0x00}},
{0x5A, 1, {0x02}},
{0x5B, 1, {0x00}},
{0x5C, 1, {0x82}},
{0x5D, 1, {0x80}},
{0x5E, 1, {0x02}},
{0x5F, 1, {0x00}},
{0x66, 1, {0x01}},
{0xFB, 1, {0x01}},
{0xFF, 1, {0x05}},
{0x54, 1, {0x75}},
{0x85, 1, {0x05}},
{0xA6, 1, {0x04}},
{0xFB, 1, {0x01}},
{0xFF, 1, {0xFF}},
{0x4F, 1, {0x03}},
{0xFB, 1, {0x01}},    
    
{0xFF,1,{0x01}},//CMDpageselect
{0xFB,1,{0x01}},//NON-RELOADCMD
     
{0xFF,1,{0x02}},//CMDpageselect
{0xFB,1,{0x01}},//NON-RELOADCMD
     
{0xFF,1,{0x04}},//CMDpageselect
{0xFB,1,{0x01}},//NON-RELOADCMD

{0xFF,1,{0x00}},//CMDpageselect
{0xFB,1,{0x01}},//NON-RELOADCMD

{0xD3,1,{0x14}},
{0xD4,1,{0x14}},          

//{0x00,1,{0x00}},
{0x11,0,{} },   
                     
{REGFLAG_DELAY, 120,  {}},//wait more than 120ms
                     
//{0x00,1,{0x00}},
{0x29,0,{} },  
                     
	{REGFLAG_DELAY, 10,  {}},//wait more than 120ms
	{REGFLAG_END_OF_TABLE, 0x00, {}}		
};

static struct LCM_setting_table lcm_sleep_mode_in_setting[] = 
{
	{0xFF,1,{0x01}}, 
    {0xB1,1,{0x03}}, 
    {0xB2,1,{0xFF}}, 
    {0xED,1,{0x03}}, 
    {0xEE,1,{0xFF}}, 
    {0xFF,1,{0x02}}, 
    {0xFB,1,{0x01}}, 
    {0x30,1,{0x03}}, 
    {0x31,1,{0xFF}}, 
    {0x6F,1,{0x03}}, 
    {0x70,1,{0xFF}}, 
    {0xAD,1,{0x03}}, 
    {0xAE,1,{0xFF}}, 
    {0xE9,1,{0x03}}, 
    {0xEA,1,{0xFF}}, 
    {0xFF,1,{0x00}}, 

    {0x28, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},
    {0xFF,1,{0x01}}, 
    {0x11,1,{0x72}}, 
    {0xFF,1,{0x00}}, 
    
	{REGFLAG_DELAY, 10, {}},
    // Sleep Mode On
	// {0x10, 0, {0x00}},
	// {REGFLAG_DELAY, 120, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                if(table[i].count <= 10)
                    MDELAY(table[i].count);
                else
                    MDELAY(table[i].count);
                break;
				
			case REGFLAG_UDELAY :
				UDELAY(table[i].count);
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
/*add by major for BF168 LCM POWER ENABLE PIN*/
void lcm_get_gpio_infor(void)
{
	static struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,lcm_mode");

	GPIO_LCD_PWR_EN = of_get_named_gpio(node, "lcm_power_gpio", 0);
}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, output);
}

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
		//params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		//params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		params->dsi.mode   = SYNC_PULSE_VDO_MODE;
        #endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		//params->dsi.word_count=720*3;	

		params->dsi.ssc_disable=1;

		params->dsi.vertical_sync_active				= 2;
		params->dsi.vertical_backporch					= 18;
		params->dsi.vertical_frontporch					= 20;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				=10;
		params->dsi.horizontal_backporch				= 120;
		params->dsi.horizontal_frontporch				= 120;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		params->dsi.PLL_CLOCK = 471;
		// Bit rate calculation
		//1 Every lane speed
	//	params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
	//	params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
//		params->dsi.fbk_div =0x12;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

		params->dsi.esd_check_enable = 0;
		params->dsi.customization_esd_check_enable = 0;
		params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
		params->dsi.lcm_esd_check_table[0].count        = 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
		
		params->dsi.lcm_esd_check_table[1].cmd          = 0xAB;
		params->dsi.lcm_esd_check_table[1].count        = 1;
		params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
		params->dsi.lcm_esd_check_table[1].para_list[1] = 0x00;


}

static void lcm_init(void)
{
	lcm_is_init = true;
	
	lcm_get_gpio_infor();
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, 1);// GPIO 5 power en for BF168
	
	
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(20);
	
	SET_RESET_PIN(1);
	MDELAY(30);      

	SET_RESET_PIN(0);
	MDELAY(20);
	
	SET_RESET_PIN(1);
	MDELAY(30);      

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);  
}

static void lcm_suspend(void)
{
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(20);

	push_table(lcm_sleep_mode_in_setting, sizeof(lcm_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);  


	lcm_set_gpio_output(GPIO_LCD_PWR_EN, 1);

	lcm_is_init = false;
}


static void lcm_resume(void)
{
	if(!lcm_is_init)
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

	data_array[0]= 0x00290508; //HW bug, so need send one HS packet
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif
#if 0
static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[2];
	unsigned int array[16];  

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	
	SET_RESET_PIN(1);
	MDELAY(20); 

	array[0] = 0x00023700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	
	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0]; //we only need ID
    #ifdef BUILD_LK
		printf("%s, LK nt35590 debug: nt35590 id = 0x%08x\n", __func__, id);
    #else
		printk("%s, kernel nt35590 horse debug: nt35590 id = 0x%08x\n", __func__, id);
    #endif

    if(id == LCM_ID_NT35590)
    	return 1;
    else
        return 0;


}
#endif

LCM_DRIVER nt35596_fhd_dsi_vdo_boyi_lcm_drv = 
{
    .name			= "nt35596_fhd_dsi_vdo_boyi",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	//.compare_id     = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };
