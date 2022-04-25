
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/bcd.h>
#include <linux/timekeeping.h>
#include <linux/time.h>

#undef dev_fmt
#define dev_fmt(fmt) "%s : " fmt,__func__

#define DS1302_DEVICE_ID 0
#define RTC_DEV_NAME "ds1302"

/*Reg Definitions*/
#define DS1302_SEC_REG	0x80
#define DS1302_MIN_REG	0x82
#define DS1302_HR_REG	0x84
#define DS1302_DATE_REG	0x86
#define DS1302_MON_REG	0x88
#define DS1302_DAY_REG	0x8A
#define DS1302_YR_REG	0x8C
#define DS1302_WP_REG	0x8E
#define DS1302_TCC_REG	0x90

#define CMD_WRITE_EN	0x00
#define CMD_READ_EN		0x01
#define DS1302_REG_COUNT	0x07
#define WP_ENABLE		0x80
#define WP_DISABLE 		0x00
	
struct ds1302_prv{
	struct spi_device *spi;
	struct regmap *map;
	struct rtc_device *rtc;
};

static int ds1302_ioctl(struct device *dev, unsigned int cmd, unsigned long arg);
static int ds1302_read_time(struct device *dev, struct rtc_time *dt);
static int ds1302_set_time(struct device *dev, struct rtc_time *dt);

static const struct rtc_class_ops ds1302_rtc_ops = {
	.ioctl		= ds1302_ioctl,
	.read_time	= ds1302_read_time,
	.set_time	= ds1302_set_time,
};

static int ds1302_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	return 0;
}
static int ds1302_read_time(struct device *dev, struct rtc_time *dt)
{
	struct ds1302_prv *prv = dev_get_drvdata(dev);
	unsigned char buf[DS1302_REG_COUNT];
	int ret;
	/*read the registers in one go and parse the values from them*/
	ret = regmap_bulk_read(prv->map,DS1302_SEC_REG,buf,DS1302_REG_COUNT);
	if(ret)
	{
		dev_err(dev,"regmap bulk read error %d\n",ret);
		return ret;
	}

	dt->tm_sec	= bcd2bin(buf[0]);
	dt->tm_min	= bcd2bin(buf[1]);
	dt->tm_hour	= bcd2bin(buf[2] & 0x3F);
	dt->tm_wday	= bcd2bin(buf[3]) - 1;
	dt->tm_mday	= bcd2bin(buf[4]);
	dt->tm_mon	= bcd2bin(buf[5] & 0x1F) - 1;
	dt->tm_year	= bcd2bin(buf[6]) + 100; /* year offset from 1900 */

	return rtc_valid_tm(dt);
}
static int ds1302_set_time(struct device *dev, struct rtc_time *dt)
{
	struct ds1302_prv *prv = dev_get_drvdata(dev);
	int ret;

	ret = regmap_write(prv->map,DS1302_WP_REG,WP_ENABLE);
	if(ret)
	{
		dev_err(dev,"regmap write WP error %d\n",ret);
		return ret;
	}

	ret = regmap_write(prv->map,DS1302_SEC_REG,bin2bcd(dt->tm_sec));
	if(ret)
	{
		dev_err(dev,"regmap write sec error %d\n",ret);
		return ret;
	}

	ret = regmap_write(prv->map,DS1302_MIN_REG,bin2bcd(dt->tm_min));
	if(ret)
	{
		dev_err(dev,"regmap write min error %d\n",ret);
		return ret;
	}

	ret = regmap_write(prv->map,DS1302_HR_REG,bin2bcd(dt->tm_hour));
	if(ret)
	{
		dev_err(dev,"regmap write hr error %d\n",ret);
		return ret;
	}

	ret = regmap_write(prv->map,DS1302_DATE_REG,bin2bcd(dt->tm_mday));
	if(ret)
	{
		dev_err(dev,"regmap write date error %d\n",ret);
		return ret;
	}

	ret = regmap_write(prv->map,DS1302_MON_REG,bin2bcd(dt->tm_mon + 1));
	if(ret)
	{
		dev_err(dev,"regmap write mon error %d\n",ret);
		return ret;
	}

	ret = regmap_write(prv->map,DS1302_DAY_REG,bin2bcd(dt->tm_wday + 1));
	if(ret)
	{
		dev_err(dev,"regmap write day error %d\n",ret);
		return ret;
	}

	ret = regmap_write(prv->map,DS1302_YR_REG,bin2bcd(dt->tm_year % 100));
	if(ret)
	{
		dev_err(dev,"regmap write sec error %d\n",ret);
		return ret;
	}

	/*disable writing*/
	ret = regmap_write(prv->map,DS1302_WP_REG,WP_DISABLE);
	if(ret)
	{
		dev_err(dev,"regmap write WP error %d\n",ret);
		return ret;
	}

	return 0;
}

static int ds1302_probe(struct spi_device *spi)
{
	struct regmap_config config;
	struct ds1302_prv *prv;
	int ret,data;
	struct rtc_time current_rtc_time;

	/*set the regmap config with reg addr size and val bits number*/
	memset(&config, 0, sizeof(config));
	config.reg_bits = 8;
	config.val_bits = 8;

	/*allocate memory for private structure*/
	prv = (struct ds1302_prv*) devm_kzalloc(&spi->dev,sizeof(struct ds1302_prv),GFP_KERNEL);
	if(!prv){
		ret = -ENOMEM;
		goto out;
	}

	spi->mode = SPI_MODE_1 | SPI_CS_HIGH | SPI_LSB_FIRST | SPI_3WIRE;
	spi->bits_per_word = 8;
	prv->spi = spi;

	ret = spi_setup(spi);
	ret = spi_setup(spi);
	if(ret){
		dev_err(&spi->dev, "Spi setup failed\n");
		goto free_mem;
	}

	/*initialize the regmap for spi device*/
	prv->map = devm_regmap_init_spi(spi,&config);
	if(IS_ERR(prv->map))
	{
		dev_err(&spi->dev, "spi regmap init failed for rtc ds1302\n");
		ret = PTR_ERR(prv->map);
		goto free_mem;
	}

	/*test the communication, if fails then return here only*/
	/*read*/
	ret = regmap_read(prv->map,DS1302_SEC_REG,&data);
	if(ret)
	{
		dev_err(&spi->dev,"regmap write sec error %d\n",ret);
		return ret;
	}
	/*write*/
	current_rtc_time = rtc_ktime_to_tm(ktime_get_real());
	ds1302_set_time(&spi->dev,&current_rtc_time);	

	/*register the device as rtc device*/
	prv->rtc = devm_rtc_device_register(&spi->dev,RTC_DEV_NAME,&ds1302_rtc_ops,THIS_MODULE);
	if(IS_ERR(prv->rtc))
	{
		dev_err(&spi->dev, "spi rtc device registration failed\n");
		ret = PTR_ERR(prv->rtc);
		goto regmap_deinit;
	}

	/*save the prv struct in dev->driver_data*/
	spi_set_drvdata(spi,prv);
	dev_info(&spi->dev,"new device got probed\n");
	return 0;

regmap_deinit:
	regmap_exit(prv->map);
free_mem:
	devm_kfree(&spi->dev,prv);
out:
	dev_err(&spi->dev,"spi device probe init failed %d\n",ret);
	return ret;
}

static int ds1302_remove(struct spi_device *spi)
{
	//devm will handle everything
	dev_info(&spi->dev,"Bye Bye world\n");
	return 0;
}


/*OF match table registration*/
static const struct of_device_id of_table_ds1302_ids[] = 
{
	{ .compatible = "ds1302,rtc", .data = NULL},
	{ } /*NULL TERMINATION*/
};

MODULE_DEVICE_TABLE(of, of_table_ds1302_ids);

/*spi device id table registration*/
static const struct spi_device_id ds1302_id[] = {
	{ "ds1302", DS1302_DEVICE_ID },
	{ }
};

MODULE_DEVICE_TABLE(spi, ds1302_id);

static struct spi_driver ds1302_driver = {
	.driver = {
		.name = "ds1302",
		.of_match_table = of_match_ptr(of_table_ds1302_ids),
		.owner = THIS_MODULE,
	},
	.probe = ds1302_probe,
	.remove = ds1302_remove,
	.id_table = ds1302_id,
};

module_spi_driver(ds1302_driver);

MODULE_DESCRIPTION("ds1302 drv code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");