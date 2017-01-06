#subdir-ccflags-y += -Werror
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek
# In case the platform does NOT support this type of sensors
obj-y   +=  mtk_tpd.o
obj-y   +=  tpd_button.o
obj-y   +=  tpd_calibrate.o
obj-y   +=  tpd_debug.o
obj-y   +=  tpd_default.o
obj-y   +=  tpd_init.o
obj-y   +=  tpd_misc.o
obj-y   +=  tpd_setting.o

obj-$(CONFIG_TOUCHSCREEN_MTK_GT1151)	+=  GT1151/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT910)	+=  GT910/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XXTB_HOTKNOT)	+=  GT9XXTB_hotknot/
obj-$(CONFIG_TOUCHSCREEN_MTK_SYNAPTICS_I2C_RMI4)	+=  synaptics_i2c_rmi4/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT928)	+=  GT928/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XX_HOTKNOT_SCP)	+=  GT9XX_hotknot_scp/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT911)	+=  GT911/
obj-$(CONFIG_TOUCHSCREEN_MTK_FT5X0X)	+=  ft5x0x/
obj-$(CONFIG_TOUCHSCREEN_MTK_FT5X0X_AF168)	+=  ft5x0x_af168/
obj-$(CONFIG_TOUCHSCREEN_MTK_FT5X0X_WE168)	+=  ft5x0x_we168/
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG2238_CF168)   +=  msg2238_cf168/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT915L_BF168)	+=  GT915L_BF168/
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG28XX)   +=  msg28xx/
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG28XX_V39)   +=  msg28xx_3.9/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT915L)	+=  GT915L/
obj-$(CONFIG_TOUCHSCREEN_AEON_MSTAR_8536)	+= msg28xx_3.9/

