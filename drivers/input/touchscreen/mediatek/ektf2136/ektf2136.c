#include "tpd.h"
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/hrtimer.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>
#include "tpd_custom_ektf2136.h"


// for linux 2.6.36.3
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/ioctl.h>
//dma
#include <linux/dma-mapping.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#define I2C_NUM 1
#define ELAN_BUTTON
#define TPD_HAVE_BUTTON

#define LCT_VIRTUAL_KEY
#define PACKET_SIZE	18

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

#define PWR_STATE_DEEP_SLEEP              0
#define PWR_STATE_NORMAL                  1
#define PWR_STATE_MASK                    BIT(3)

#define CMD_S_PKT                         0x52
#define CMD_R_PKT                         0x53
#define CMD_W_PKT                         0x54

#define HELLO_PKT                         0x55
#define TWO_FINGERS_PKT             			0x5A
#define FIVE_FINGERS_PKT                  0x5D
#define MTK_FINGERS_PKT                   0x6D    /** 2 Fingers: 5A, 5 Fingers: 5D, 10 Fingers: 62 **/

#define RESET_PKT                    			0x77
#define CALIB_PKT                    			0xA8

#define TPD_OK 0
#define MTK_TP_DEBUG(fmt, args ...)


#ifdef TPD_HAVE_BUTTON
#define TPD_KEY_COUNT           3
#define TPD_KEYS                { KEY_MENU, KEY_HOMEPAGE, KEY_BACK}
#define TPD_KEYS_DIM            {{120,900,120,100},{240,900,120,100},{360,900,120,100}}


static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

// modify
#define SYSTEM_RESET_PIN_SR   135

//Add these Define
#define PAGERETRY  					30
#define IAPRESTART 					5

// For Firmware Update 
#define ELAN_IOCTLID    						0xD0
#define IOCTL_I2C_SLAVE       			_IOW(ELAN_IOCTLID, 1, int)
#define IOCTL_MAJOR_FW_VER  				_IOR(ELAN_IOCTLID, 2, int)
#define IOCTL_MINOR_FW_VER  				_IOR(ELAN_IOCTLID, 3, int)
#define IOCTL_CHECK_RECOVERY_MODE  	_IOR(ELAN_IOCTLID, 6, int)
#define IOCTL_FW_VER  							_IOR(ELAN_IOCTLID, 7, int)
#define IOCTL_X_RESOLUTION  				_IOR(ELAN_IOCTLID, 8, int)
#define IOCTL_Y_RESOLUTION  				_IOR(ELAN_IOCTLID, 9, int)
#define IOCTL_FW_ID  								_IOR(ELAN_IOCTLID, 10, int)
#define IOCTL_ROUGH_CALIBRATE  			_IOR(ELAN_IOCTLID, 11, int)
#define IOCTL_RESUME  							_IOR(ELAN_IOCTLID, 14, int)
#define IOCTL_POWER_LOCK  					_IOR(ELAN_IOCTLID, 15, int)
#define IOCTL_POWER_UNLOCK  				_IOR(ELAN_IOCTLID, 16, int)
#define IOCTL_FW_UPDATE  						_IOR(ELAN_IOCTLID, 17, int)
#define IOCTL_BC_VER  							_IOR(ELAN_IOCTLID, 18, int)
#define IOCTL_2WIREICE  						_IOR(ELAN_IOCTLID, 19, int)

#define CUSTOMER_IOCTLID 						0xA0
#define IOCTL_CIRCUIT_CHECK  				_IOR(CUSTOMER_IOCTLID, 1, int)
#define IOCTL_GET_UPDATE_PROGREE    _IOR(CUSTOMER_IOCTLID, 2, int)

extern struct tpd_device *tpd;

uint8_t RECOVERY=0x00;
int FW_VERSION=0x00;
int X_RESOLUTION=480;  
int Y_RESOLUTION=854;
int FW_ID=0x00;
int BC_VERSION = 0x00;
int work_lock=0x00;
int power_lock=0x00;
int circuit_ver=0x01;
int button_state = 0;
static int probe_flage=0;

/*++++i2c transfer start+++++++*/
int file_fops_addr=0x15;
/*++++i2c transfer end+++++++*/
int tpd_down_flag=0;

struct i2c_client *i2c_client;
struct task_struct *thread;
struct task_struct *update_thread;

static DECLARE_WAIT_QUEUE_HEAD(waiter);

static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id);


static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
static int tpd_flag = 0;
unsigned int tpd_rst_gpio_number = 0;
unsigned int tpd_int_gpio_number = 0;
unsigned int touch_irq = 0;

static const struct i2c_device_id tpd_id[] = 
{
         { "ektf3248", 0 },
         { }
};
static const struct of_device_id ektf3248_dt_match[] = {
	{.compatible = "mediatek,cap_touch"},
	{},
};
MODULE_DEVICE_TABLE(of, ektf3248_dt_match);

static struct i2c_board_info __initdata ektf3248_i2c_tpd = { I2C_BOARD_INFO("ektf3248", (0x2a>>1))};

static struct i2c_driver tpd_i2c_driver =
{
    .driver = {
    .of_match_table = of_match_ptr(ektf3248_dt_match),
    .name = "ektf3248",
},
    .probe = tpd_probe,
    .remove =  tpd_remove,
    .id_table = tpd_id,
    .detect = tpd_i2c_detect,
};

struct elan_ktf2k_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct workqueue_struct *elan_wq;
	struct work_struct work;
	int intr_gpio;
// Firmware Information
	int fw_ver;
	int fw_id;
	int bc_ver;
	int x_resolution;
	int y_resolution;
	struct hrtimer timer;
};

static struct elan_ktf2k_ts_data *private_ts;
static int __hello_packet_handler(struct i2c_client *client);
static int __fw_packet_handler(struct i2c_client *client);
static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client);
static void tpd_resume(struct device *h);
static void tpd_suspend(struct device *h);

// For Firmware Update 
int elan_iap_open(struct inode *inode, struct file *filp){ 

      printk("[ELAN]into elan_iap_open\n");
      if (private_ts == NULL)  printk("private_ts is NULL~~~");
                   
      return 0;
}

int elan_iap_release(struct inode *inode, struct file *filp){    
      return 0;
}

static ssize_t elan_iap_write(struct file *filp, const char *buff, size_t count, loff_t *offp){  
    int ret;
    char *tmp;

    printk("[ELAN]into elan_iap_write\n");
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);
    
    if (tmp == NULL)
        return -ENOMEM;

    ret = i2c_master_send(private_ts->client, tmp, count);   
    kfree(tmp);
    return (ret == 1) ? count : ret;

}

ssize_t elan_iap_read(struct file *filp, char *buff, size_t count, loff_t *offp){    
    char *tmp;
    int ret;

    printk("[ELAN]into elan_iap_read\n");
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);

    if (tmp == NULL)
        return -ENOMEM;  
    ret = i2c_master_recv(private_ts->client, tmp, count);
    kfree(tmp);

    //return ret;
    return (ret == 1) ? count : ret;
         
}

static long elan_iap_ioctl(/*struct inode *inode,*/ struct file *filp,    unsigned int cmd, unsigned long arg){
         printk("[ELAN]into elan_iap_ioctl\n");
         printk("[ELAN]cmd value %x\n",cmd);
         
         switch (cmd) {        
                   case IOCTL_I2C_SLAVE: 
                            private_ts->client->addr = (int __user)arg;
                            private_ts->client->addr &= I2C_MASK_FLAG; 
                            private_ts->client->addr |= I2C_ENEXT_FLAG;
                            //file_fops_addr = 0x15;
                            break;   
                   case IOCTL_MAJOR_FW_VER:            
                            break;        
                  case IOCTL_MINOR_FW_VER:            
                            break;
                   case IOCTL_CHECK_RECOVERY_MODE:
                            return RECOVERY;
                            break;
                   case IOCTL_FW_VER:
                            __fw_packet_handler(private_ts->client);
                            return FW_VERSION;
                            break;
                   case IOCTL_X_RESOLUTION:
                            __fw_packet_handler(private_ts->client);
                            return X_RESOLUTION;
                            break;
                   case IOCTL_Y_RESOLUTION:
                            __fw_packet_handler(private_ts->client);
                            return Y_RESOLUTION;
                            break;
                   case IOCTL_FW_ID:
                            __fw_packet_handler(private_ts->client);
                            return FW_ID;
                            break;
                   case IOCTL_ROUGH_CALIBRATE:
                            return elan_ktf2k_ts_rough_calibrate(private_ts->client);     
                   case IOCTL_RESUME:
                            tpd_resume(NULL);
                            break;       
                   case IOCTL_CIRCUIT_CHECK:
                            return circuit_ver;
                            break;
                   case IOCTL_POWER_LOCK:
                            power_lock=1;
                            break;
                   case IOCTL_POWER_UNLOCK:
                            power_lock=0;
                            break;
                   case IOCTL_BC_VER:
                            __fw_packet_handler(private_ts->client);
                            return BC_VERSION;
                            break;

                   default:            
                            break;   
         }       
         return 0;
}

struct file_operations elan_touch_fops = {    
        .open =       		elan_iap_open,    
        .write =      		elan_iap_write,    
        .read =       		elan_iap_read,    
        .release = 				elan_iap_release,    
        .unlocked_ioctl = elan_iap_ioctl, 
 };

static ssize_t elan_ktf2k_gpio_show(struct device *dev, char *buf)
{
	int ret = 0;
	struct elan_ktf2k_ts_data *ts = private_ts;

	//ret = gpio_get_value(ts->intr_gpio);
	ret = gpio_direction_input(tpd_int_gpio_number);
	printk(KERN_DEBUG "GPIO_TP_INT_N=%d\n", ts->intr_gpio);
	sprintf(buf, "GPIO_TP_INT_N=%d\n", ret);
	ret = strlen(buf) + 1;
	return ret;
}

static DEVICE_ATTR(gpio, S_IRUGO, elan_ktf2k_gpio_show, NULL);

static ssize_t elan_ktf2k_vendor_show(struct device *dev, char *buf)
{
	ssize_t ret = 0;
	struct elan_ktf2k_ts_data *ts = private_ts;

	sprintf(buf, "%s_x%4.4x\n", "ELAN_KTF2K", ts->fw_ver);
	ret = strlen(buf) + 1;
	return ret;
}

static int __elan_ktf2k_ts_poll(struct i2c_client *client)
{
	int status = 0, retry = 10;

         do {
	status = gpio_request_one(tpd_int_gpio_number, GPIOF_IN,
				 "tpd_int");
                   retry--;
                   mdelay(20);
         } while (status == 1 && retry > 0);
         return (status == 0 ? 0 : -ETIMEDOUT);
}

static int elan_ktf2k_ts_poll(struct i2c_client *client)
{
	return __elan_ktf2k_ts_poll(client);
}

static int elan_ktf2k_ts_get_data(struct i2c_client *client, uint8_t *cmd,
			uint8_t *buf, size_t size)
{
         int rc;

	dev_dbg(&client->dev, "[elan]%s: enter\n", __func__);

	if (buf == NULL)
		return -EINVAL;


	if ((i2c_master_send(client, cmd, 4)) != 4) {
		dev_err(&client->dev,
			"[elan]%s: i2c_master_send failed\n", __func__);
		return -EINVAL;
	}

         rc = elan_ktf2k_ts_poll(client);
         if (rc < 0)
                   return -EINVAL;
         else {

                   if (i2c_master_recv(client, buf, size) != size ||
                       buf[0] != CMD_S_PKT)
                       {
                       printk("mtk-tpd:[elan_ktf2k_ts_get_data] buf[0]=%x buf[1]=%x buf[2]=%x buf[3]=%x\n", buf[0], buf[1], buf[2], buf[3]);
                            return -EINVAL;
                          }
         }

         return 0;
}

static int __hello_packet_handler(struct i2c_client *client)
{
         int rc;
         uint8_t buf_recv[8] = { 0 };
         mdelay(100);
         rc = elan_ktf2k_ts_poll(client);
         if (rc < 0) {
                   printk( "mtk-tpd:[elan] %s: Int poll failed!\n", __func__);
                   RECOVERY=0x80;
                   return RECOVERY;  
         }

         rc = i2c_master_recv(client, buf_recv, 8);

         printk("mtk-tpd:[elan] %s: Hello Packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
         if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x80 && buf_recv[3]==0x80)
         {
        RECOVERY=0x80;

        rc = i2c_master_recv(client, buf_recv, 8);

        printk("mtk-tpd:[elan] %s: Bootcode Verson %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
        return RECOVERY; 
         }

         return 0;
}

static int __fw_packet_handler(struct i2c_client *client)
{
         int rc;
         int major, minor;
         uint8_t cmd[] = {CMD_R_PKT, 0x00, 0x00, 0x01};	/* Get Firmware Version*/
         uint8_t cmd_x[] = {0x53, 0x60, 0x00, 0x00}; 		/*Get x resolution*/
         uint8_t cmd_y[] = {0x53, 0x63, 0x00, 0x00}; 		/*Get y resolution*/
         uint8_t cmd_id[] = {0x53, 0xf0, 0x00, 0x01}; 	/*Get firmware ID*/
         uint8_t cmd_bc[] = {CMD_R_PKT, 0x10, 0x00, 0x01};/* Get BootCode Version*/
         uint8_t buf_recv[8] = {0};

printk( "mtk-tpd:[elan] %s: n", __func__);

#if 1
// Firmware version
         rc = elan_ktf2k_ts_get_data(client, cmd, buf_recv, 4);
         if (rc < 0)
            return rc;
         major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
         minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
//      ts->fw_ver = major << 8 | minor;
        FW_VERSION = major << 8 | minor;

#endif
         
#if 1
// Firmware ID
         rc = elan_ktf2k_ts_get_data(client, cmd_id, buf_recv, 4);
         if (rc < 0)
         		return rc;
         major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
         minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
         //ts->fw_id = major << 8 | minor;
         FW_ID = major << 8 | minor;
#endif

#if 1
// X Resolution
         rc = elan_ktf2k_ts_get_data(client, cmd_x, buf_recv, 4);
         if (rc < 0)
         		return rc;
         minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
         X_RESOLUTION = minor;
#endif

#if 1        
// Y Resolution          
         rc = elan_ktf2k_ts_get_data(client, cmd_y, buf_recv, 4);
         if (rc < 0)
           return rc;
         minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
         Y_RESOLUTION = minor;
#endif

#if 1                             
// Bootcode version
         rc = elan_ktf2k_ts_get_data(client, cmd_bc, buf_recv, 4);
         if (rc < 0)
           return rc;
         major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
         minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
         //ts->bc_ver = major << 8 | minor;
         BC_VERSION = major << 8 | minor;
#endif
         
         printk( "mtk-tpd:[elan] %s: firmware version: 0x%4.4x\n",
                            __func__, FW_VERSION);
         printk( "mtk-tpd:[elan] %s: firmware ID: 0x%4.4x\n",
                            __func__, FW_ID);
         printk( "mtk-tpd:[elan] %s: x resolution: %d, y resolution: %d\n",
                            __func__, X_RESOLUTION, Y_RESOLUTION);
         printk( "mtk-tpd:[elan] %s: bootcode version: 0x%4.4x\n",
                            __func__, BC_VERSION);
         return 0;
}

static inline int elan_ktf2k_ts_parse_xy(uint8_t *data,
                            uint16_t *x, uint16_t *y)
{
         *x = *y = 0;

         *x = (data[0] & 0xf0);
         *x <<= 4;
         *x |= data[1];

         *y = (data[0] & 0x0f);
         *y <<= 8;
         *y |= data[2];

         return 0;
}

static int elan_ktf2k_ts_setup(struct i2c_client *client)
{
         int rc;
   
         rc = __hello_packet_handler(client);
         printk("[elan] hellopacket's rc = %d\n",rc);

         mdelay(10);
         if (rc != 0x80){
             rc = __fw_packet_handler(client);
             if (rc < 0)
                       printk("mtk-tpd:[elan] %s, fw_packet_handler fail, rc = %d", __func__, rc);
                     else
                  printk("mtk-tpd:[elan] %s: firmware checking done.\n", __func__);
                            /* Check for FW_VERSION, if 0x0000 means FW update fail! */
             if ( FW_VERSION == 0x00)
             {
                   rc = 0x80;
                   printk("mtk-tpd:[elan] FW_VERSION = %d, last FW update fail\n", FW_VERSION);
             }
         }
         return rc; /* Firmware need to be update if rc equal to 0x80(Recovery mode)   */
}

static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client){
      uint8_t cmd[] = {CMD_W_PKT, 0x29, 0x00, 0x01};

         printk("[elan] %s: enter\n", __func__);
         printk("[elan] dump cmd: %02x, %02x, %02x, %02x\n",
                   cmd[0], cmd[1], cmd[2], cmd[3]);

         if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
                   dev_err(&client->dev,
                            "[elan] %s: i2c_master_send failed\n", __func__);
                   return -EINVAL;
         }

         return 0;
}

static int elan_ktf2k_ts_recv_data(struct i2c_client *client, uint8_t *buf)
{
         int rc, bytes_to_recv=PACKET_SIZE;

         if (buf == NULL)
                   return -EINVAL;
         memset(buf, 0, bytes_to_recv);         
         #ifdef NON_MTK_MODE	//I2C support > 8bits transfer
         rc = i2c_master_recv(client, buf, bytes_to_recv);		//for two finger and non-mtk five finger and ten finger
         if (rc != bytes_to_recv)
                   printk("mtk-tpd:[elan_debug] The package error.\n");
         MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15],buf[16], buf[17]);
         #else    
         rc = i2c_master_recv(client, buf, 8);	//for two finger and non-mtk five finger and ten finger
         if (rc != 8)
                   printk("mtk-tpd:[elan_debug] The first package error.\n");
         MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
         mdelay(1);
         
         if (buf[0] == MTK_FINGERS_PKT) {    		//for mtk five finger
                   rc = i2c_master_recv(client, buf+ 8, 8);  
                   if (rc != 8)
                            printk("mtk-tpd:[elan_debug] The second package error.\n");
                   MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
                   
                   rc = i2c_master_recv(client, buf+ 16, 2);
                   if (rc != 2)
                            printk("mtk-tpd:[elan_debug] The third package error.\n");
                   MTK_TP_DEBUG("[elan_recv] %x %x \n", buf[16], buf[17]);
                   
         }
         #endif      
         
         return rc;
}

static  void tpd_down(int x, int y, int p) 
{
	 input_report_abs(tpd->dev, ABS_MT_PRESSURE, 1);
	 input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 20);
	 input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	 input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
         input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, p); 
	 input_mt_sync(tpd->dev);
	 if(y > TPD_RES_Y) //virtual key debounce to avoid android ANR issue
	 {
		 printk("D virtual key \n");
	 }
	 TPD_EM_PRINT(x, y, x, y, p-1, 1);
 }

 static  void tpd_up(int x, int y,int *count)
{	
		 input_mt_sync(tpd->dev);
		 TPD_EM_PRINT(x, y, x, y, 0, 0); 		 
}

static void elan_ktf2k_ts_report_data(struct i2c_client *client, uint8_t *buf)
{
         struct input_dev *idev = tpd->dev;
         uint16_t fbits=0;
         uint8_t num, reported = 0;
         uint8_t idx, btn_idx;
         int finger_num;
	 static	int tmp_x = 0;
	 static int tmp_y = 0;
// for 5 fingers 
          if ((buf[0] == MTK_FINGERS_PKT) || (buf[0] == FIVE_FINGERS_PKT)){
                  finger_num = 5;
                  num = buf[1] & 0x07; 
        fbits = buf[1] >>3;
                  idx=2;
                  btn_idx=17;
         }else{
// for 2 fingers      
                   finger_num = 2;
                   num = buf[7] & 0x03; 
                   fbits = buf[7] & 0x03;
                   idx=1;
                   btn_idx=7;
         }
                   
         switch (buf[0]) {
                   case MTK_FINGERS_PKT:
                   case TWO_FINGERS_PKT:
                   case FIVE_FINGERS_PKT:
                            if (num == 0)
                            {
                                     dev_dbg(&client->dev, "no press\n");
                                               
#if 1 //def ELAN_BUTTON
					switch (buf[btn_idx]) {
				    	case ELAN_KEY_1:
						tmp_x = tpd_keys_dim_local[2][0];
						tmp_y = tpd_keys_dim_local[2][1];
						tpd_down(tmp_x, tmp_y, 1);
						button_state = ELAN_KEY_1;
						break;
				    	case ELAN_KEY_2:
						tmp_x = tpd_keys_dim_local[1][0];
						tmp_y = tpd_keys_dim_local[1][1];
						tpd_down(tmp_x, tmp_y, 1);
						button_state = ELAN_KEY_2;
						break;
						case ELAN_KEY_3:
						tmp_x = tpd_keys_dim_local[0][0];
						tmp_y = tpd_keys_dim_local[0][1];
						tpd_down(tmp_x, tmp_y, 1);
						button_state = ELAN_KEY_3;
						break;
					default:
						tpd_up(tmp_x, tmp_y, 0);
						tpd_down_flag = 0;
               					break;
				    }
								  
                //input_sync(idev);     
#endif		      
                            }
                            if (reported)
                                     input_sync(idev);
                            else 
                            {
                                     input_mt_sync(idev);
                                     input_sync(idev);
                            }
                            break;
                  default:
                                               MTK_TP_DEBUG("mtk-tpd:[elan] %s: unknown packet type: %0x\n", __func__, buf[0]);
                                     break;
         } // end switch
         return;
}

static int touch_event_handler(void *unused)
{
         int rc;
         uint8_t buf[PACKET_SIZE] = { 0 };
         struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
         sched_setscheduler(current, SCHED_RR, &param);
         MTK_TP_DEBUG("mtk-tpd interrupt touch_event_handler\n");

         do
         {
                   MTK_TP_DEBUG("mtk-tpd touch_event_handler mt_eint_unmask\n");
                   set_current_state(TASK_INTERRUPTIBLE);
                   wait_event_interruptible(waiter, tpd_flag != 0);
                   tpd_flag = 0;
                   set_current_state(TASK_RUNNING);
                   MTK_TP_DEBUG("mtk-tpd touch_event_handler mt_eint_mask\n");
                   rc = elan_ktf2k_ts_recv_data(private_ts->client, buf);

                   if (rc < 0)
                   {
                            printk("mtk-tpd:[elan] rc<0\n");
         
                            continue;
                   }

                   elan_ktf2k_ts_report_data(/*ts*/private_ts->client, buf);

         }while(!kthread_should_stop());

         return 0;
}

static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, TPD_DEVICE);

	return 0;
}

static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id)
{
	TPD_DEBUG("TPD interrupt has been triggered\n");
	tpd_flag = 1;
	wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
}
static int tpd_irq_registration(void)
{
	struct device_node *node = NULL;
	int ret = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,cap_touch");
	if (node) {
		/*touch_irq = gpio_to_irq(tpd_int_gpio_number);*/
		touch_irq = irq_of_parse_and_map(node, 0);
		ret = request_irq(touch_irq, tpd_eint_interrupt_handler,
					IRQF_TRIGGER_FALLING, TPD_DEVICE, NULL);
			if (ret > 0)
				TPD_DMESG("tpd request_irq IRQ LINE NOT AVAILABLE!.");
	} else {
		TPD_DMESG("[%s] tpd request_irq can not find touch eint device node!.", __func__);
	}
	return 0;
}
static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
         int fw_err = 0; 
         int retval = TPD_OK;
         static struct elan_ktf2k_ts_data ts;
         client->addr |= I2C_ENEXT_FLAG;
         printk("mtk-tpd:[elan] %s:client addr is %x, TPD_DEVICE = ektf3248\n",__func__,client->addr);
         client->timing =  100;
         i2c_client = client;
         private_ts = &ts;
         private_ts->client = client;
         msleep(10);
       
	/* configure the gpio pins */
	retval = gpio_request_one(tpd_rst_gpio_number, GPIOF_OUT_INIT_LOW,
				 "touchp_reset");
	if (retval < 0) {
		TPD_DMESG("Unable to request gpio reset_pin\n");
		return -1;
	}
	retval = gpio_request_one(tpd_int_gpio_number, GPIOF_IN,
				 "tpd_int");
	if (retval < 0) {
		TPD_DMESG("Unable to request gpio int_pin\n");
		gpio_free(tpd_rst_gpio_number);
		return -1;
	}
	gpio_direction_output(tpd_rst_gpio_number, 0);
	msleep(20);
	gpio_direction_output(tpd_rst_gpio_number, 1);
	msleep(50);

	TPD_DMESG("mtk_tpd: tpd_probe ft5x0x\n");


	retval = regulator_enable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);

	/* set INT mode */

	gpio_direction_input(tpd_int_gpio_number);

	tpd_irq_registration();
	msleep(100);


         fw_err = elan_ktf2k_ts_setup(client);
         if (fw_err < 0) {
             printk(KERN_INFO "[elan] No Elan chip inside\n");
         }   
         
         tpd_load_status = 1;
         
	#ifndef LCT_VIRTUAL_KEY
	set_bit( KEY_BACK,  tpd->dev->keybit );
    	set_bit( KEY_HOMEPAGE,  tpd->dev->keybit );
    	set_bit( KEY_MENU,  tpd->dev->keybit );
	#endif

         thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
         if(IS_ERR(thread))
         {
             retval = PTR_ERR(thread);
         }

	probe_flage = 1;
	gpio_free(tpd_rst_gpio_number);
	gpio_free(tpd_int_gpio_number);
              
    return 0;
}

static int tpd_remove(struct i2c_client *client)
{
    printk("mtk-tpd:[elan] TPD removed\n");
   return 0;
}


static void tpd_suspend(struct device *h)
{
	int retval = TPD_OK;
	static char data = 0x3;

	TPD_DEBUG("TPD enter sleep\n");
    
	disable_irq(touch_irq);
	i2c_smbus_write_i2c_block_data(i2c_client, 0xA5, 1, &data);  /* TP enter sleep mode */

	retval = regulator_disable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to disable reg-vgp6: %d\n", retval);

}


static void tpd_resume(struct device *h)
{
	int retval = TPD_OK;

	TPD_DEBUG("TPD wake up\n");

	retval = regulator_enable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);

	msleep(100);

	gpio_direction_output(tpd_rst_gpio_number, 0);
	msleep(20);
	gpio_direction_output(tpd_rst_gpio_number, 1);
	msleep(20);
	enable_irq(touch_irq);
}

static int tpd_local_init(void)
{
	int retval;

	TPD_DMESG("Focaltech FT5x0x I2C Touchscreen Driver...\n");
	tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);
	if (retval != 0) {
		TPD_DMESG("Failed to set reg-vgp6 voltage: %d\n", retval);
		return -1;
	}
	if (i2c_add_driver(&tpd_i2c_driver) != 0) {
		TPD_DMESG("unable to add i2c driver.\n");
		return -1;
	}
     /* tpd_load_status = 1; */
	if (tpd_dts_data.use_tpd_button) {
		tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
		tpd_dts_data.tpd_key_dim_local);
	}

#ifdef TPD_HAVE_BUTTON
	#ifdef LCT_VIRTUAL_KEY
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
	#endif
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
         memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
         memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);         
#endif 

    printk("mtk-tpd:end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;
    return 0;
}


static struct tpd_driver_t tpd_device_driver =
{
    .tpd_device_name = "ektf3248",       
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
    .tpd_have_button = 1,
#else
    .tpd_have_button = 0,
#endif
};

static int __init tpd_driver_init(void)
{
         printk("mtk-tpd EKTF3248 touch panel driver init\n");

         i2c_register_board_info(I2C_NUM, &ektf3248_i2c_tpd, 1);

         if(tpd_driver_add(&tpd_device_driver) < 0)
         {
             printk("[mtk-tpd]: %s driver failed\n", __func__);
         }
         return 0;
}


static void __exit tpd_driver_exit(void)
{
    printk("[mtk-tpd]: %s elan ektf touch panel driver exit\n", __func__);
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);

