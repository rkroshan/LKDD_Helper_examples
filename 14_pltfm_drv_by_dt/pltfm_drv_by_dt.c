#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include<linux/platform_device.h>
#include<linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include "pltfm_dvc_by_device_id.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

/*platform device specific driver data to store*/
struct device_mapping{
	char* model_id;
	char* description;
};

struct device_mapping dev_map_array[] = {
	[DEV001] = {
		.model_id = "DEV001_AEDFG",
		.description = "Dev001 Device description"
	}
};

/*OF match table registration*/
static const struct of_device_id of_pcdevs_ids[] = 
{
	{.compatible = "pltfm,dev001", .data = (void*)&dev_map_array[DEV001]},
	{ } /*Null termination */
};

MODULE_DEVICE_TABLE(of,of_pcdevs_ids);

/*platform device id table registration*/
struct platform_device_id pcdevs_ids[] = 
{
	[0] = {.name = DEV1,.driver_data = DEV001},
	{ } /*Null termination */
};

MODULE_DEVICE_TABLE(platform,pcdevs_ids);


/*Defining static functions*/
static int pltfm_drv_by_dt_probe(struct platform_device *pdev)
{
	/*pre initialization*/
	struct prv_dvc_st *dvc;
	struct device_mapping *dev_map;
	unsigned long dev_data;
	const struct of_device_id* match;

	pr_info("New device probed with name :%s\n", pdev->name);

	/*check if device matched on basis of of_device_id table*/
	match = of_match_device(of_match_ptr(of_pcdevs_ids),&pdev->dev);
	if(match)
	{
		/*means device probed from dtb*/
		dev_map = (struct device_mapping*) match->data;
		/*print the data*/
		pr_info("model_id : %s\n", dev_map->model_id);
		pr_info("description : %s\n", dev_map->description);
	}
		/*check if device matched on basis of device id table*/
	else if(pdev->id_entry !=NULL)
	{
		pr_info("probed by device id table match\n");
		/*that means match happens from platform_device_id table*/
		/*get the driver data*/
		dev_data = pdev->id_entry->driver_data;
		dev_map = &dev_map_array[dev_data];
		/*print the data*/
		pr_info("model_id : %s\n", dev_map->model_id);
		pr_info("description : %s\n", dev_map->description);
	}

	/*get the platform device data which will be null in case of_device_id table match*/
	if(!match){
		dvc = (struct prv_dvc_st*)dev_get_platdata(&pdev->dev);
		pr_info("%s named device data is %d\n", pdev->name,dvc->data);
	}

	return 0;
}

static int pltfm_drv_by_dt_remove(struct platform_device *pdev)
{
	pr_info("Device named : %s is removed\n",pdev->name);
	return 0;
}

/*platform driver structure initialization*/
struct platform_driver pltfm_drv_st = 
{
	.probe = pltfm_drv_by_dt_probe,
	.remove = pltfm_drv_by_dt_remove,
	.id_table = pcdevs_ids,
	.driver = {
		.name = "pseudo-char-device",
		.of_match_table = of_match_ptr(of_pcdevs_ids),
		.owner = THIS_MODULE
	}

};


static int __init pltfm_drv_by_dt_init(void)
{
	pr_info("module init\n");
	/*Register a platform driver */
	platform_driver_register(&pltfm_drv_st);
	
	pr_info("platform driver loaded\n");
	return 0;
}

static void __exit pltfm_drv_by_dt_exit(void)
{
	/*Unregister the platform driver */
	platform_driver_unregister(&pltfm_drv_st);
	pr_info("platform driver unloaded\n");
	pr_info("module unloaded\n");
	pr_info("bye bye world\n");
}

module_init(pltfm_drv_by_dt_init);
module_exit(pltfm_drv_by_dt_exit);
MODULE_DESCRIPTION("pltfm_drv_by_dt example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");