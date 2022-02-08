#include <linux/init.h>
#include <linux/module.h>
#include<linux/platform_device.h>
#include "plfm_dvc_by_name.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

void pcdev_release(struct device *dev)
{
	pr_info("Device released having data: %d\n", ((struct prv_dvc_st *)(dev->platform_data))->data);
}

struct prv_dvc_st prv_dvc1 = {
	.data = 1729, /*unique data just for fun*/
};

struct platform_device plt_dvc = {
	.name = "pseudo-char-device",
	.id = 0,
	.dev = {
		.platform_data = &prv_dvc1,
		.release = pcdev_release /*this function is called after every removal for device*/
	}
};


static int __init plfm_dvc_by_name_init(void)
{
	pr_info("module init\n");
	/*registering the pseudo device*/
	platform_device_register(&plt_dvc);
	return 0;
}

static void __exit plfm_dvc_by_name_exit(void)
{
	/*unregister the pseudo device*/
	platform_device_unregister(&plt_dvc);
	pr_info("module unloaded\n");
	pr_info("bye bye world\n");
}

module_init(plfm_dvc_by_name_init);
module_exit(plfm_dvc_by_name_exit);
MODULE_DESCRIPTION("plfm_dvc_by_name example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");