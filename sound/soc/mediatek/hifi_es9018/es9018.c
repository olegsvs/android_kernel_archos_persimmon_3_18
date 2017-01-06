/*
 * es9018.c -- es9018 ALSA SoC audio driver 
 *  versoin = 2.01  
 *  
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
//#include <linux/regulator/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <trace/events/asoc.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
//#include "es9018.h"
#include <linux/i2c.h>

#include <linux/pinctrl/pinctrl.h> //add by major for bf168 es9018

#define INPUT_CONFIG_SOURCE 1
#define I2S_BIT_FORMAT_MASK (0x03 << 6)
#define MASTER_MODE_CONTROL 10
#define I2S_CLK_DIVID_MASK (0x03 << 5)
#define RIGHT_CHANNEL_VOLUME_15 15
#define LEFT_CHANNEL_VOLUME_16 16
#define MASTER_TRIM_VOLUME_17 17
#define MASTER_TRIM_VOLUME_18 18
#define MASTER_TRIM_VOLUME_19 19
#define MASTER_TRIM_VOLUME_20 20
#define HEADPHONE_AMPLIFIER_CONTROL 42



struct es9018_priv {
	struct snd_soc_codec *codec;
	struct i2c_client *i2c_client;
	struct es9018_data *es9018_data;
	struct delayed_work sleep_work;
	struct mutex power_lock;
} es9018_priv;


struct es9018_data {
	int reset_gpio;
	int power_gpio;
	int i2c_scl_gpio;
	int i2c_sda_gpio;
	int i2c_addr;
};
static struct es9018_priv *g_es9018_priv = NULL;
static int es9018_read_reg(struct i2c_client *client, int reg);

//------- add proc debug (ESS linsd)------------------------------------------------
#ifndef ES9018_PROC_a 
#define ES9018_PROC_a 1
#endif

#ifdef ES9018_PROC_a

	//#include <linuxinit.h>
	#include <linux/module.h>
	#include <linux/fs.h>
	#include <linux/proc_fs.h>
	#include <linux/uaccess.h>
	//#include <string.h>

	static struct proc_dir_entry *es9018_dir;  
	static struct proc_dir_entry *es9018_entry;  
	#define USER_ES9018_DIR "ES_dbg"  
	#define USER_ENTRY1   "esproc"
	static int es9018_write_reg(struct i2c_client *client, int reg, u8 value);
	//static struct es9018_priv *g_es9018_priv = NULL;
	struct proc_priv_dev {
		//struct proc_dir_entry *proc;
		char *buffer;
		unsigned char curr_reg_addr;
	};
	struct proc_priv_dev *proc_dev;




	//proc-------------------------------------------------------------------
static ssize_t hifi_es9018_proc_read(struct file* file,char __user * page,size_t size,loff_t *data)
	{
 		int reg;
		reg = i2c_smbus_read_byte_data(g_es9018_priv->i2c_client,proc_dev->curr_reg_addr );
		if(proc_dev->curr_reg_addr >=25)
    			proc_dev->curr_reg_addr=0;
		else
     			proc_dev->curr_reg_addr++;
		return sprintf(page,"addr=%d,data=%x",proc_dev->curr_reg_addr,reg);
	}

	//proc--------------------------------------------------------------------
static ssize_t hifi_es9018_proc_write(struct file *file,const char __user *buffer,size_t count,loff_t *data)
	{
		unsigned int xaddr,xdata,i,n;

		if(copy_from_user(proc_dev->buffer,buffer,count)) {
			return -EFAULT;
		}
		xaddr = 0;
		if(count>=2)
		{  if(proc_dev->buffer[0] >= '0' && proc_dev->buffer[0]<='9')
   		  	xaddr = (proc_dev->buffer[0]-'0')*10;
		   if(proc_dev->buffer[1] >= '0' && proc_dev->buffer[1]<='9')
   		  	xaddr += (proc_dev->buffer[0]-'0');
		}
		proc_dev->curr_reg_addr= xaddr;
		if(count>=5)
		{  
		for(i=2;i<count;i++)
		   {
		   if(proc_dev->buffer[i]=='x') {n=i;break;}
		   }
		xdata = 0x00;
		n++;		
		if(proc_dev->buffer[n] >= '0' && proc_dev->buffer[n]<='9')
			 xdata = (proc_dev->buffer[0]-'0')*0x10;
		else if(proc_dev->buffer[n] >= 'a' && proc_dev->buffer[n]<='f')
   		  	xdata = (proc_dev->buffer[0]-'a')*0x10;
		n++;
		if(proc_dev->buffer[n] >= '0' && proc_dev->buffer[n]<='9')
			 xdata += (proc_dev->buffer[0]-'0'+10);
		else if(proc_dev->buffer[n] >= 'a' && proc_dev->buffer[n]<='f')
   		  	xdata += (proc_dev->buffer[0]-'a'+10);
		 
		es9018_write_reg((struct i2c_client *)g_es9018_priv->i2c_client,xaddr,(u8)xdata);  
		}

		return count;
	}

	static int es9018_open_b(void)// power on for ES9018-------------
	{
		//int i = 0;
		//power_gpio_H();
		//reset_gpio_L();
		//udelay(10);
		//reset_gpio_H();
		//udelay(10);
		return 0;
	}

        //init proc ------------------------------------------------------------------ 
static const struct file_operations es_proc_fops = {
.owner = THIS_MODULE,
.read = hifi_es9018_proc_read,
.write = hifi_es9018_proc_write,
};

	static int  es_porc_init(void)
	{
		int ret;
		proc_dev = kzalloc(sizeof(*proc_dev),GFP_KERNEL);
		if(!proc_dev) {
			ret = -ENOMEM;
			return -1;
		}
		proc_dev->buffer = kzalloc(0X100,GFP_KERNEL);
		if(!proc_dev->buffer) {
			ret = - ENOMEM;
			goto erro_out_a;
		}
		es9018_dir =proc_mkdir(USER_ES9018_DIR, NULL); 
		if (NULL==es9018_dir) 
		{ printk(KERN_ALERT"Create dir /proc/%s error!\n", USER_ES9018_DIR); 
			//return -1;
		  goto erro_out_b; 
		} 

		es9018_entry =proc_create(USER_ENTRY1, 0666,es9018_dir,&es_proc_fops); 
		if (NULL == es9018_entry) 
		{ printk(KERN_ALERT"Create entry %s under /proc/%s error!\n", USER_ENTRY1,USER_ES9018_DIR); 
			goto erro_out; 
		}
		es9018_open_b();
		return 0;
	erro_out: 
		remove_proc_entry(USER_ES9018_DIR,es9018_dir); 
    	erro_out_b:
		kfree(proc_dev->buffer);
	erro_out_a:
		kfree(proc_dev);
		return -1;  
	}


	//
	static void es_proc_exit(void)
	{
		remove_proc_entry(USER_ES9018_DIR,es9018_dir);
		remove_proc_entry(USER_ENTRY1,es9018_entry);
		kfree(proc_dev->buffer);
		kfree(proc_dev);
	}
#endif

static  int G_volume =0x7F;
/* We only include the analogue supplies here; the digital supplies
 * need to be available well before this driver can be probed.
 */
char es9018_init_register[26]={
	0x00,//0  
	0x8c,//1  //:I2S input
	0x18,//2
	0x10,//3
	0x00,//4
	0x68,//5
	0x4a,//6: 47= 32KHz ; 57=44.1KHz; 67=48KHz
	0x80,//7
	0x10,//8   0x70
	0x22,//9 	:slave mode=0x22;  master mode= 0xa2
	0x05,//10  0x2d
	0x02,//11
	0x5a,//12
	0x40,//13
	0x8a,//14
	0x00,//15
	0x00,//16
	0xff,//17
	0xff,//18
	0xff,//19
	0x7f,//20
	0x00,//21
	0x00,//22
	0x00,//23
	0x00,//24  0x32
	0x00,//25

};

static unsigned int es9018_power_state = 0;
static unsigned int es9018_hifi_switch = 0;
//static struct es9018_priv *g_es9018_priv = NULL;
static int es9018_write_reg(struct i2c_client *client, int reg, u8 value);
#define ES9018_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |	\
		SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |	\
		SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000 |	\
		SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_192000)

#define ES9018_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE | \
		SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
		SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE | \
		SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S32_BE)
//add by major for bf168 hifi android 6.0
struct pinctrl *hifi_gpio_pinctrl;
struct pinctrl_state *hifi_gpio_default;
struct pinctrl_state *hifi_boost_en0,*hifi_boost_en1,*hifi_boost_typ0,*hifi_boost_typ1;
struct pinctrl_state *hifi_ldo1_en0,*hifi_ldo1_en1,*hifi_ldo2_en0,*hifi_ldo2_en1;
struct pinctrl_state *hifi_dac_rst0,*hifi_dac_rst1,*hifi_dcdc_en0,*hifi_dcdc_en1;
struct pinctrl_state *hifi_opa_en0,*hifi_opa_en1,*hifi_xo_en0,*hifi_xo_en1;
struct pinctrl_state *hifi_sw_sel0,*hifi_sw_sel1,*hifi_sw_mute0,*hifi_sw_mute1;

void hifi_boost_en(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_boost_en1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_boost_en0);
}
void hifi_boost_typ(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_boost_typ1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_boost_typ0);
}
void hifi_ldo1_en(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_ldo1_en1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_ldo1_en0);
}
void hifi_ldo2_en(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_ldo2_en1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_ldo2_en0);
}
void hifi_dac_rst(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_dac_rst1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_dac_rst0);
}
void hifi_dcdc_en(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_dcdc_en1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_dcdc_en0);
}
void hifi_opa_en(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_opa_en1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_opa_en0);
}
void hifi_xo_en(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_xo_en1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_xo_en0);
}
void hifi_sw_sel(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_sw_sel1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl, hifi_sw_sel0);
}
void hifi_sw_mute(int level)
{
		if (level)
			pinctrl_select_state(hifi_gpio_pinctrl,  hifi_sw_mute1);
		else
			pinctrl_select_state(hifi_gpio_pinctrl,  hifi_sw_mute0);
}
int hifi_get_gpio_info(struct platform_device *pdev)
{
	int ret=0;
	
	hifi_gpio_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(hifi_gpio_pinctrl)) {
		ret = PTR_ERR(hifi_gpio_pinctrl);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio hifi_gpio_pinctrl!\n");
		return ret;
	}
	hifi_gpio_default = pinctrl_lookup_state(hifi_gpio_pinctrl, "default");
	if (IS_ERR(hifi_gpio_default)) {
		ret = PTR_ERR(hifi_gpio_default);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_gpio_default %d!\n", ret);
	}
	hifi_boost_en0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_boost_en0");
	if (IS_ERR(hifi_boost_en0)) {
		ret = PTR_ERR(hifi_boost_en0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_boost_en0!\n");
		return ret;
	}
	hifi_boost_en1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_boost_en1");
	if (IS_ERR(hifi_boost_en1)) {
		ret = PTR_ERR(hifi_boost_en1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_boost_en1!\n");
		return ret;
	}
	hifi_boost_typ0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_boost_typ0");
	if (IS_ERR(hifi_boost_typ0)) {
		ret = PTR_ERR(hifi_boost_typ0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_boost_typ0!\n");
		return ret;
	}
	hifi_boost_typ1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_boost_typ1");
	if (IS_ERR(hifi_boost_typ1)) {
		ret = PTR_ERR(hifi_boost_typ1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_boost_typ1!\n");
		return ret;
	}
	hifi_ldo1_en0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_ldo1_en0");
	if (IS_ERR(hifi_ldo1_en0)) {
		ret = PTR_ERR(hifi_ldo1_en0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_ldo1_en0!\n");
		return ret;
	}
	hifi_ldo1_en1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_ldo1_en1");
	if (IS_ERR(hifi_ldo1_en1)) {
		ret = PTR_ERR(hifi_ldo1_en1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_ldo1_en1!\n");
		return ret;
	}
	hifi_ldo2_en0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_ldo2_en0");
	if (IS_ERR(hifi_ldo2_en0)) {
		ret = PTR_ERR(hifi_ldo2_en0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_ldo2_en0!\n");
		return ret;
	}
	hifi_ldo2_en1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_ldo2_en1");
	if (IS_ERR(hifi_ldo2_en1)) {
		ret = PTR_ERR(hifi_ldo2_en1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_ldo2_en1!\n");
		return ret;
	}
	hifi_dac_rst0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_dac_rst0");
	if (IS_ERR(hifi_dac_rst0)) {
		ret = PTR_ERR(hifi_dac_rst0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_dac_rst0!\n");
		return ret;
	}
	hifi_dac_rst1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_dac_rst1");
	if (IS_ERR(hifi_dac_rst1)) {
		ret = PTR_ERR(hifi_dac_rst1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_dac_rst1!\n");
		return ret;
	}
	hifi_dcdc_en0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_dcdc_en0");
	if (IS_ERR(hifi_dcdc_en0)) {
		ret = PTR_ERR(hifi_dcdc_en0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_dcdc_en0!\n");
		return ret;
	}
	hifi_dcdc_en1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_dcdc_en1");
	if (IS_ERR(hifi_dcdc_en1)) {
		ret = PTR_ERR(hifi_dcdc_en1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_dcdc_en1!\n");
		return ret;
	}
	hifi_opa_en0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_opa_en0");
	if (IS_ERR(hifi_opa_en0)) {
		ret = PTR_ERR(hifi_opa_en0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_opa_en0!\n");
		return ret;
	}
	hifi_opa_en1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_opa_en1");
	if (IS_ERR(hifi_opa_en1)) {
		ret = PTR_ERR(hifi_opa_en1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_opa_en1!\n");
		return ret;
	}
	hifi_xo_en0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_xo_en0");
	if (IS_ERR(hifi_xo_en0)) {
		ret = PTR_ERR(hifi_xo_en0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_xo_en0!\n");
		return ret;
	}
	hifi_xo_en1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_xo_en1");
	if (IS_ERR(hifi_xo_en1)) {
		ret = PTR_ERR(hifi_xo_en1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_xo_en1!\n");
		return ret;
	}

	hifi_sw_sel0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_sw_sel0");
	if (IS_ERR(hifi_sw_sel0)) {
		ret = PTR_ERR(hifi_sw_sel0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_sw_sel0!\n");
		return ret;
	}
	hifi_sw_sel1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_sw_sel1");
	if (IS_ERR(hifi_sw_sel1)) {
		ret = PTR_ERR(hifi_sw_sel1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_sw_sel1!\n");
		return ret;
	}
	hifi_sw_mute0 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_sw_mute0");
	if (IS_ERR(hifi_sw_mute0)) {
		ret = PTR_ERR(hifi_sw_mute0);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_sw_mute0!\n");
		return ret;
	}
	hifi_sw_mute1 = pinctrl_lookup_state(hifi_gpio_pinctrl, "hifi_sw_mute1");
	if (IS_ERR(hifi_sw_mute1)) {
		ret = PTR_ERR(hifi_sw_mute1);
		dev_err(&pdev->dev, "fwq Cannot find hifi_gpio pinctrl hifi_sw_mute1!\n");
		return ret;
	}

	return ret;
}
//add by darren end


/*************************************************************/
/***************  Hifi es9018 I2C Config  ********************/
/*************************************************************/
static void ess_i2s_config(void)
{

#if 0
	mt_set_gpio_mode(GPIO_DAC_I2S_MCLK_PIN, GPIO_MODE_03);
	mt_set_gpio_mode(GPIO_DAC_I2S_DAT_OUT_PIN, GPIO_MODE_03);
	mt_set_gpio_mode(GPIO_DAC_I2S_WS_PIN, GPIO_MODE_03);
#endif
}
#if 0
//add by major 
static void dump_register(void)
{
	int i;
	int reg_val =0;
	for(i=1; i< 27;i++)
	{
		reg_val = es9018_read_reg(g_es9018_priv->i2c_client,i);
	    printk("es9018 REG[%2d]=0x%x\n",i,reg_val);
	}
}
#endif
//add end 
static void hifi_es9018_init(void)
{
	int i;
	printk("%s ++++++++\n",__func__);

	hifi_opa_en(0);
	hifi_ldo1_en(1);
	hifi_xo_en(1);
	mdelay(10);
	hifi_dac_rst(1);
	mdelay(10);
	hifi_dac_rst(0);
	mdelay(10);
	hifi_dac_rst(1);
	mdelay(40);

	for(i=0;i<26;i++){ 
		es9018_write_reg(g_es9018_priv->i2c_client,i,es9018_init_register[i]);
	}
	if (G_volume != 0x7F)
	{
		es9018_write_reg(g_es9018_priv->i2c_client,20,G_volume);
	}
	printk("%s --------\n",__func__);

	mdelay(20);
}
static void hifi_es9018_deinit(void)
{
	printk("%s ++++++\n",__func__);
	es9018_write_reg((struct i2c_client *)g_es9018_priv->i2c_client,14,0x0a); //go power down //modify by darren
	mdelay(5);
	hifi_dac_rst(0);
	mdelay(100);
	hifi_xo_en(0);
	mdelay(1);
	printk("%s -------\n",__func__);

}
static int es9018_get_hifi_switch_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	pr_info("GAC:%s(): hifi switch = %d\n", __func__,
			es9018_hifi_switch);
	ucontrol->value.enumerated.item[0] = es9018_hifi_switch;
	pr_info("%s(): ucontrol = %d\n", __func__,
			ucontrol->value.enumerated.item[0]);

	return 0;
}

static int es9018_put_hifi_switch_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	int ret=0;
	pr_info("GAC:%s():ucontrol = %d\n", __func__,
			ucontrol->value.enumerated.item[0]);
	pr_info("GAC:%s():hifi switch= %d\n", __func__,
			es9018_hifi_switch);

	if (es9018_hifi_switch == ucontrol->value.enumerated.item[0]) {
		pr_info("GAC:%s():no hifi switch change\n", __func__);
		return ret;
	}

      es9018_hifi_switch = ucontrol->value.enumerated.item[0];

	if (ucontrol->value.enumerated.item[0])
	{
		hifi_opa_en(1);
		hifi_sw_sel(1);
		hifi_sw_mute(0); // selet:1 mute:0 ---> L2.R2 internal channel. 
	}
	else 
	{
		hifi_sw_sel(0);
		hifi_sw_mute(0);// selet:1 mute:0 ---> L1.R1 default HIFI channel. 
	}

	return ret;
}


static int es9018_get_power_state_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	pr_info("GAC:%s(): power state = %d\n", __func__,
			es9018_power_state);
	ucontrol->value.enumerated.item[0] = es9018_power_state;
	pr_info("%s(): ucontrol = %d\n", __func__,
			ucontrol->value.enumerated.item[0]);

	return 0;
}

static int es9018_put_power_state_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	int ret=0;
	pr_info("GAC:%s():ucontrol = %d\n", __func__,
			ucontrol->value.enumerated.item[0]);
	pr_info("GAC:%s():power state= %d\n", __func__,
			es9018_power_state);

	if (es9018_power_state == ucontrol->value.enumerated.item[0]) {
		pr_info("GAC:%s():no power state change\n", __func__);
		return ret;
	}

      es9018_power_state = ucontrol->value.enumerated.item[0];

	if (ucontrol->value.enumerated.item[0])
	{
		 hifi_es9018_init();
	}
	else 
		hifi_es9018_deinit();
	return ret;
}



static int es9018_get_volume_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
//	int es9018_volume_text[2];

	pr_info("GAC:%s(): Volume = %d\n", __func__,
			G_volume);
//	sprintf(es9018_volume_text[0],"0x%x",G_volume);
	ucontrol->value.enumerated.item[0] = G_volume;
	pr_info("%s(): ucontrol = %d\n", __func__,
			ucontrol->value.enumerated.item[0]);

	return 0;
}

static int es9018_put_volume_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	int ret=0;
	int volume=0;
	pr_info("GAC:%s():ucontrol = %d\n", __func__,
			ucontrol->value.enumerated.item[0]);

	pr_info("do nothing \n");
	return 0;
	if (ucontrol->value.enumerated.item[0])
	{
			volume =0x7F;
	}else
	{
			volume =0x0F;
			
	}
	pr_info("GAC:%s(): G_volume=0x%x  Volume=0x%x\n", __func__,
			G_volume,volume);
	if (G_volume == volume )
	{
		pr_info("es9018 G_vlume is same to volume ,Do nothing \n");
		return 0;
	}
	G_volume = volume;

	if (1 == es9018_power_state)
	{

		pr_info("es9018 hifi Power On  set Volume   reg[20] =[0x%x]\n",G_volume);
		es9018_write_reg(g_es9018_priv->i2c_client,20,G_volume);
	}else
	{
		pr_info("es9018 hifi Power Off and Do nothing \n");
	}
#if 0
	if (es9018_power_state == ucontrol->value.enumerated.item[0]) {
		pr_info("GAC:%s(): Volume not changed\n", __func__);
	}
	else
	{
		G_volume = ucontrol->value.enumerated.item[0];
		es9018_write_reg(g_es9018_priv->i2c_client,20,G_volume);
	}
#endif

	return ret;
}

static int es9018_get_clk_divider(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	u8 reg_val;

	reg_val = es9018_read_reg(g_es9018_priv->i2c_client,
				MASTER_MODE_CONTROL);
	reg_val = reg_val >> 5;
	ucontrol->value.integer.value[0] = reg_val;

	pr_info("%s: i2s_length = 0x%x\n", __func__, reg_val);

	return 0;
}

static int es9018_set_clk_divider(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	u8 reg_val;

	pr_info("%s: ucontrol->value.integer.value[0]  = %ld\n",
		__func__, ucontrol->value.integer.value[0]);

	reg_val = es9018_read_reg(g_es9018_priv->i2c_client,
				MASTER_MODE_CONTROL);

	reg_val &= ~(I2S_CLK_DIVID_MASK);
	reg_val |=  ucontrol->value.integer.value[0] << 5;

	es9018_write_reg(g_es9018_priv->i2c_client,
				MASTER_MODE_CONTROL, reg_val);
	return 0;
}



static const char * const es9018_power_state_texts[] = {
	 "Close","Open",};
static const char * const es9018_hifi_switch_texts[] = {
	 "Off","On",};

static const char * const es9018_clk_divider_texts[] = {
	"DIV4", "DIV8", "DIV16", "DIV16"
};

static const char * const es9018_volume_control_texts[] = {
	"SMALL", "BIG", };

static const struct soc_enum es9018_power_state_enum =
SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
		ARRAY_SIZE(es9018_power_state_texts),
		es9018_power_state_texts);

static const struct soc_enum es9018_hifi_switch_enum =
SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
		ARRAY_SIZE(es9018_hifi_switch_texts),
		es9018_hifi_switch_texts);

static const struct soc_enum es9018_volume_control_enum =
SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
		ARRAY_SIZE(es9018_volume_control_texts),
		es9018_volume_control_texts);

static const struct soc_enum es9018_clk_divider_enum =
SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(es9018_clk_divider_texts),
		es9018_clk_divider_texts);

static struct snd_kcontrol_new es9018_digital_ext_snd_controls[] = {
	/* commit controls */

	SOC_ENUM_EXT("Set_HIFI_es9018_State", es9018_power_state_enum,
			es9018_get_power_state_enum, es9018_put_power_state_enum),

	SOC_ENUM_EXT("Set_HIFI_Switch", es9018_hifi_switch_enum,
			es9018_get_hifi_switch_enum, es9018_put_hifi_switch_enum),

	SOC_ENUM_EXT("Set_Volume_Control", es9018_volume_control_enum,
			es9018_get_volume_enum, es9018_put_volume_enum),

    SOC_ENUM_EXT("Es9018 CLK Divider", es9018_clk_divider_enum,
			es9018_get_clk_divider, es9018_set_clk_divider),
	
};

static int es9018_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int es9018_write_reg(struct i2c_client *client, int reg, u8 value)
{

	int ret,i;

	for (i=0; i<3; i++)
	{
		ret = i2c_smbus_write_byte_data(client, reg, value);
		if (ret < 0)\
		{
			dev_err(&client->dev, "%s: err %d,and try again\n", __func__, ret);
			mdelay(50);
		}
		else
			break;
	}

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static unsigned int es9018_codec_read(struct snd_soc_codec *codec,
		unsigned int reg)
{
	//struct es9018_priv *priv = codec->control_data;
	return 0;
}

static int es9018_codec_write(struct snd_soc_codec *codec, unsigned int reg,
		unsigned int value)
{
	//struct es9018_priv *priv = codec->control_data;
	return 0;
}
static int es9018_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	int ret = 0;

	/* dev_dbg(codec->dev, "%s(codec, level = 0x%04x): entry\n", __func__, level); */

	switch (level) {
		case SND_SOC_BIAS_ON:
			break;

		case SND_SOC_BIAS_PREPARE:
			break;

		case SND_SOC_BIAS_STANDBY:
			break;

		case SND_SOC_BIAS_OFF:
			break;
	}
	codec->dapm.bias_level = level;

	/* dev_dbg(codec->dev, "%s(): exit\n", __func__); */
	return ret;
}

static int es9018_suspend(struct snd_soc_codec *codec)
{
	hifi_opa_en(1);
	hifi_dcdc_en(0);
	return 0;
}

static int es9018_resume(struct snd_soc_codec *codec)
{
	hifi_opa_en(1);
	hifi_dcdc_en(1);
	return 0;
}

static int es9018_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *codec_dai)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	//struct es9018_priv *priv = codec->control_data;

	return 0;
}

static int es9018_mute(struct snd_soc_dai *dai, int mute)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	//struct es9018_priv *priv = codec->control_data;

	return 0;

}

static int es9018_set_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	//struct es9018_priv *priv = codec->control_data;

	return 0;
}

static int es9018_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	//struct es9018_priv *priv = codec->control_data;

	return 0;
}


static int es9018_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	//struct es9018_priv *priv = codec->control_data;

	return 0;
}

static int es9018_set_fll(struct snd_soc_dai *codec_dai,
		int pll_id, int source, unsigned int freq_in,
		unsigned int freq_out)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	//struct es9018_priv *priv = codec->control_data;

	return 0;
}


static int es9018_pcm_trigger(struct snd_pcm_substream *substream,
		int cmd, struct snd_soc_dai *codec_dai)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	//struct es9018_priv *priv = codec->control_data;
	return 0;
}


static const struct snd_soc_dai_ops es9018_dai_ops = {
	.hw_params	= es9018_pcm_hw_params,
	.digital_mute	= es9018_mute,
	.trigger	= es9018_pcm_trigger,
	.set_fmt	= es9018_set_dai_fmt,
	.set_sysclk	= es9018_set_dai_sysclk,
	.set_pll	= es9018_set_fll,
	.set_clkdiv	= es9018_set_clkdiv,
};

static struct snd_soc_dai_driver es9018_dai = {
	.name = "es9018-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = ES9018_RATES,
		.formats = ES9018_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = ES9018_RATES,
		.formats = ES9018_FORMATS,
	},
	.ops = &es9018_dai_ops,
};
static struct proc_dir_entry *es9018 = NULL;

//add by major 

static ssize_t es9018_dump_reg_read_proc(struct file *file, char *buffer, size_t count, loff_t *ppos)
{ 
	  int i,len,err;
	  char* ptr =NULL;
	  char* page =NULL;
	  int reg_val =0;

	  page = kmalloc(PAGE_SIZE, GFP_KERNEL);	
	  if (!page) 
	  {		
		  kfree(page);		
		  return -ENOMEM;	
	  }
	 ptr = page;
	  ptr += sprintf(ptr, "==== es9018 reg value====\n");
	
	for(i=1; i< 27;i++)
	{
		reg_val = es9018_read_reg(g_es9018_priv->i2c_client,i);
		ptr += sprintf(ptr, "REG[%2d]=0x%02X\n", i,reg_val);
	}
  	len = ptr -page;
	if (*ppos >= len)
	{
		kfree(page);
		return 0;
	}
	err=copy_to_user(buffer,(char*)page,len);
	*ppos += len;
	if(err)
	{
		kfree(page);
		return err;
	}
	kfree(page);
	  return len;
}

static ssize_t es9018_dump_reg_write_proc(struct file *file, const char *buffer, size_t count, loff_t *ppos)
{
    //const char *temp[25]={0};
	int reg =0;
	int data =0;
	
#if 0 
	   if (copy_from_user(&temp,buffer, sizeof(temp)))
	   {
		 printk("copy from user fail by major \n");
	     return -EFAULT;
	   }
#endif
	   sscanf(buffer,"%d %x",&reg,&data);
	   printk("major %s  reg=%d  data=0x%x \n",__func__,reg,data);
	   if (reg >= 0 && data >=0)
	   {
	   	es9018_write_reg(g_es9018_priv->i2c_client,reg,data);
	   }
	  return count;
}

static const struct file_operations es9018_dump_reg_proc_fops = 
{
     .write = es9018_dump_reg_write_proc,
     .read = es9018_dump_reg_read_proc
 };

static  int es9018_codec_probe(struct snd_soc_codec *codec)
{
	int rc = 0;
	struct es9018_priv *priv = snd_soc_codec_get_drvdata(codec);
	//dev_info(codec->dev, "%s(): entry\n", __func__);
	//dev_info(codec->dev, "%s(): codec->name = %s\n", __func__, codec->name);
	printk("es9018_codec_probe !!!!!!!!!!");


	priv->codec = codec;

	codec->control_data = snd_soc_codec_get_drvdata(codec);

	//dev_info(codec->dev, "%s(): codec->control_data = 0x%08x\n", __func__, (unsigned int)codec->control_data);

	es9018_set_bias_level(codec, SND_SOC_BIAS_STANDBY);


	rc = snd_soc_add_codec_controls(codec, es9018_digital_ext_snd_controls,
			ARRAY_SIZE(es9018_digital_ext_snd_controls));
	if (rc)
		dev_err(codec->dev, "%s(): es325_digital_snd_controls failed\n", __func__);


	dev_info(codec->dev, "%s(): exit\n", __func__);
	return 0;
}

static int  es9018_codec_remove(struct snd_soc_codec *codec)
{
	struct es9018_priv *priv = snd_soc_codec_get_drvdata(codec);

	es9018_set_bias_level(codec, SND_SOC_BIAS_OFF);

	kfree(priv);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_es9018 = {
	.probe =	es9018_codec_probe,
	.remove =	es9018_codec_remove,
	.suspend = 	es9018_suspend,
	.resume =	es9018_resume,
	.read = es9018_codec_read,
	.write = es9018_codec_write,
	.set_bias_level = es9018_set_bias_level,
};

static int es9018_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	struct es9018_priv *priv;
	struct es9018_data *pdata;
	int ret = 0;
	int value=0;
	printk("es9018_probe begin +++++++++++++++\n");


	hifi_boost_en(1);
	hifi_boost_typ(1);
	hifi_opa_en(1);
	hifi_dcdc_en(1);//OPA +/- 5v
	hifi_ldo2_en(1);//Switch Power On
	hifi_sw_sel(0);
	hifi_sw_mute(0);//select:0 mute:0 ----->L1,R1 default HIFI Channel

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "%s: no support for i2c read/write"
				"byte data\n", __func__);
		return -EIO;
	}


	printk("alloc es9018 priv\n");
	pdata = devm_kzalloc(&client->dev,
				sizeof(struct es9018_data), GFP_KERNEL);


	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -EINVAL;
	}

	priv = devm_kzalloc(&client->dev, sizeof(struct es9018_priv),
			GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	priv->i2c_client = client;
	priv->es9018_data = pdata;
	i2c_set_clientdata(client, priv);

	g_es9018_priv = priv;

    dev_set_name(&client->dev, "%s", "es9018-codec");

	ret = snd_soc_register_codec(&client->dev, &soc_codec_dev_es9018,
			&es9018_dai, 1);

	printk("snd_soc_register_codec ret = %d\n",ret);
 
#if 1 //add by major for  noise is a litter much bigger during boot up 
	hifi_es9018_init();
	value = es9018_read_reg(g_es9018_priv->i2c_client,0x01);
	hifi_es9018_deinit();
	printk("%s probe end value  %x \n",__func__,value);
#endif
	es9018 = proc_create("es9018", 0666, NULL, &es9018_dump_reg_proc_fops);
	if (NULL == es9018)
	{
		printk("es9018 PROC create Failed\n");
	}

	return ret;

	#ifdef ES9018_PROC_a
	es_porc_init(); //***********proc******************
	#endif
	
	return ret;

}

static int  es9018_remove(struct i2c_client *client)
{
	#ifdef ES9018_PROC_a
	es_proc_exit();
	#endif

	return 0;
}

static struct of_device_id es9018_match_table[] = {
	{ .compatible = "mediatek,es9018_codec", },
	{}
};
static struct of_device_id es9018_codec_match_table[] = {
	{ .compatible = "mediatek,es9018_platform_codec", },
	{}
};

static const struct i2c_device_id es9018_id[] = {
	{ "es9018-codec", 0 },
	{ },
};
//MODULE_DEVICE_TABLE(i2c, isa1200_id);

static struct i2c_driver es9018_i2c_driver = {
	.driver	= {
		.name	= "es9018-codec",
	#ifdef CONFIG_OF 
		.of_match_table = es9018_match_table,
	#endif
	},
	.probe		= es9018_probe,
	.remove		= es9018_remove,
	//.suspend	= es9018_suspend,
	//.resume		= es9018_resume,
	.id_table	= es9018_id,
};


static int es9018_platform_probe(struct platform_device *pdev)
{
	printk("%s \n",__func__);
	hifi_get_gpio_info(pdev);
	return i2c_add_driver(&es9018_i2c_driver);
}

static int es9018_platform_remove(struct platform_device *pdev)
{
	i2c_del_driver(&es9018_i2c_driver);
	return 0;
}

static int es9018_platform_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int es9018_platform_resume(struct platform_device *pdev)
{
	ess_i2s_config();
	return 0;
}

/* platform structure */
static struct platform_driver g_stes9018_codec_Driver = {
	.probe = es9018_platform_probe,
	.remove = es9018_platform_remove,
	.suspend = es9018_platform_suspend,
	.resume = es9018_platform_resume,
	.driver = {
		   .name = "es9018",
		   .owner = THIS_MODULE,
		#ifdef CONFIG_OF 
			.of_match_table = es9018_codec_match_table,
		#endif
		   }
};

static int __init es9018_init(void)
{
	printk("es9018_init !!!!!!!!!!\n");

	if (platform_driver_register(&g_stes9018_codec_Driver)) {
	printk("failed to register es9018 codec driver\n");
	return -ENODEV;
	}
	return 0;
}

static void __exit es9018_exit(void)
{
	platform_driver_unregister(&g_stes9018_codec_Driver);
}
module_init(es9018_init);
module_exit(es9018_exit);

MODULE_DESCRIPTION("ASoC ES9018 driver");
MODULE_AUTHOR("ESS-LINshaodong");
MODULE_LICENSE("GPL");
