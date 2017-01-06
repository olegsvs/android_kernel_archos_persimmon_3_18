/*
 * cpu-i2s.c  --  ALSA Soc Audio Layer dump platform
 *
 * (c) 2014 Wolfson Microelectronics PLC.
 * karl.sun@wolfsonmicro.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/pcm.h>
#include <sound/core.h>
#include "mt_soc_pcm_common.h"


extern void snd_soc_unregister_dai(struct device *dev);
extern int snd_soc_register_dai(struct device *dev,
		struct snd_soc_dai_driver *dai_drv);

/*
 * Set dummy I2S DAI format
 */
static int cpu_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
//	u32 iismod;

	printk(KERN_ERR "es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);

	return 0;
}

static int cpu_i2s_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
//	struct snd_soc_pcm_runtime *rtd = substream->private_data;
//	struct s3c_dma_params *dma_data;
//	u32 iismod;

	printk(KERN_ERR "WM es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);

	return 0;
}

static int cpu_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
			       struct snd_soc_dai *dai)
{
	int ret = 0;
	printk(KERN_ERR "es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	return ret;
}

/*
 * Set dummy Clock source
 */
static int cpu_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
	int clk_id, unsigned int freq, int dir)
{

	printk(KERN_ERR "es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);

	return 0;
}

/*
 * Set dummy Clock dividers
 */
static int cpu_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int div)
{
	printk(KERN_ERR "es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);

	return 0;
}

static int cpu_i2s_probe(struct snd_soc_dai *dai)
{
	printk(KERN_ERR "es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	return 0;
}

#ifdef CONFIG_PM
static int cpu_i2s_suspend(struct snd_soc_dai *cpu_dai)
{
	printk(KERN_ERR "es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	return 0;
}

static int cpu_i2s_resume(struct snd_soc_dai *cpu_dai)
{
	printk(KERN_ERR "es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	return 0;
}
#else
#define cpu_i2s_suspend NULL
#define cpu_i2s_resume NULL
#endif


#define CPU_I2S_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

static const struct snd_soc_dai_ops cpu_i2s_dai_ops = {
	.trigger	= cpu_i2s_trigger,
	.hw_params	= cpu_i2s_hw_params,
	.set_fmt	= cpu_i2s_set_fmt,
	.set_clkdiv	= cpu_i2s_set_clkdiv,
	.set_sysclk	= cpu_i2s_set_sysclk,
};



static struct snd_soc_dai_driver cpu_i2s_dai = {
	//.name = "mtk-es9018-i2s.0",
	.name = MT_SOC_DUMMY_I2S_DAI_NAME,
	.probe = cpu_i2s_probe,
	.suspend = cpu_i2s_suspend,
	.resume = cpu_i2s_resume,
	.playback = {
		.stream_name = MT_SOC_DUMMY_I2S_STREAM_PCM,
		.channels_min = 2,
		.channels_max = 2,
		.rates = CPU_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE,},
	.capture = {
		.stream_name = MT_SOC_DUMMY_I2S_STREAM_PCM,
		.channels_min = 2,
		.channels_max = 2,
		.rates = CPU_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &cpu_i2s_dai_ops,
};

static const struct snd_soc_component_driver cpu_i2s_dai_component =
{
    .name       = MT_SOC_DUMMY_I2S_DAI_NAME,
};

static int cpu_iis_dev_probe(struct platform_device *pdev)
{
	int rc = 0;

	printk(KERN_ERR "es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);

    	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
    	if (pdev->dev.dma_mask == NULL)
    	{
        	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
    	}

    	if (pdev->dev.of_node)
    	{
        	dev_set_name(&pdev->dev, "%s", MT_SOC_DUMMY_I2S_DAI_NAME);
    	}
    	printk("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	//return snd_soc_register_dai(&pdev->dev, &cpu_i2s_dai);	//DTEST999

    	rc = snd_soc_register_component(&pdev->dev, &cpu_i2s_dai_component,
                                    &cpu_i2s_dai, 1/*ARRAY_SIZE(cpu_i2s_dai)*/);

	printk("%s: rc  = %d\n", __func__, rc);
	return rc;
}

static int cpu_iis_dev_remove(struct platform_device *pdev)
{
	printk(KERN_ERR "es9018-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	//snd_soc_unregister_dai(&pdev->dev);//DTEST999
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id Mt_es9018_i2s_dump_platform_of_ids[] =
{
    { .compatible = "mediatek,Mt_es9018_i2s_dump_platform", },
    {}
};
#endif

static struct platform_driver mtk_es9018_i2s_driver = {
	.probe  = cpu_iis_dev_probe,
	.remove = cpu_iis_dev_remove,
	.driver = {
		.name = "mtk-es9018-i2s",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = Mt_es9018_i2s_dump_platform_of_ids,
#endif
	},
};

static int __init mtk_es9018_i2s_dai_init(void)
{
//    int ret = 0;
    printk("%s:\n", __func__);
    return platform_driver_register(&mtk_es9018_i2s_driver);
}
module_init(mtk_es9018_i2s_dai_init);

static void __exit mtk_es9018_i2s_dai_exit(void)
{
    printk("%s:\n", __func__);

    platform_driver_unregister(&mtk_es9018_i2s_driver);
}

module_exit(mtk_es9018_i2s_dai_exit);

/* Module information */
MODULE_AUTHOR("dalvin.fu@dewav.com>");
MODULE_DESCRIPTION("cpu I2S SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cpu-iis");
