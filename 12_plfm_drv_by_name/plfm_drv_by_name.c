#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include<linux/platform_device.h>
#include "plfm_dvc_by_name.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

/*Defining static functions*/
static int plfm_drv_by_name_probe(struct platform_device *pdev)
{
	/*pre initialization*/
	struct prv_dvc_st *dvc;

	pr_info("New device probed with name :%s\n", pdev->name);
	/*get the platform device data*/
	dvc = (struct prv_dvc_st*)dev_get_platdata(&pdev->dev);
	pr_info("%s named device data is %d\n", pdev->name,dvc->data);

	return 0;
}

static int plfm_drv_by_name_remove(struct platform_device *pdev)
{
	pr_info("Device named : %s is removed\n",pdev->name);
	return 0;
}

/*platform driver structure initialization*/
struct platform_driver pltfm_drv_st = 
{
	.probe = plfm_drv_by_name_probe,
	.remove = plfm_drv_by_name_remove,
	.driver = {
		.name = "pseudo-char-device",
		.owner = THIS_MODULE
	}

};


static int __init plfm_drv_by_name_init(void)
{
	pr_info("module init\n");
	/*Register a platform driver */
	platform_driver_register(&pltfm_drv_st);
	
	pr_info("platform driver loaded\n");
	return 0;
}

static void __exit plfm_drv_by_name_exit(void)
{
	/*Unregister the platform driver */
	platform_driver_unregister(&pltfm_drv_st);
	pr_info("platform driver unloaded\n");
	pr_info("module unloaded\n");
	pr_info("bye bye world\n");
}

module_init(plfm_drv_by_name_init);
module_exit(plfm_drv_by_name_exit);
MODULE_DESCRIPTION("plfm_drv_by_name example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");