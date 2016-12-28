#define ELAN_X_MAX 	 960//576  
#define ELAN_Y_MAX	 1856 //1792//960 


#define LCM_X_MAX	simple_strtoul(CONFIG_LCM_WIDTH, NULL, 0)
#define LCM_Y_MAX	simple_strtoul(CONFIG_LCM_HEIGHT, NULL, 0)

#define ELAN_KEY_1	0x10
#define ELAN_KEY_2	0x08
#define ELAN_KEY_3	0x04

#ifndef _LINUX_ELAN_KTF2K_H
#define _LINUX_ELAN_KTF2K_H

#define ELAN_KTF2K_NAME "elan-ktf2k"
#define TPD_POWER_SOURCE_CUSTOM	MT6328_POWER_LDO_VGP1

struct elan_ktf2k_i2c_platform_data {
	uint16_t version;
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int intr_gpio;
	int (*power)(int on);
};
struct osd_offset{
	int left_x;
	int right_x;
	unsigned int key_event;
};
#endif /* _LINUX_ELAN_KTF2K_H */
