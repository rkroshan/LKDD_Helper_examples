#include <linux/init.h>
#include <linux/module.h>
#include<linux/platform_device.h>
#include "pltfm_dvc_by_device_id.h"

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
	.name = DEV1,
	.id = 0,
	.dev = {
		.platform_data = &prv_dvc1,
		.release = pcdev_release /*this function is called after every removal for device*/
	}
};


static int __init pltfm_dvc_by_device_id_init(void)
{
	pr_info("module init\n");
	/*registering the pseudo device*/
	platform_device_register(&plt_dvc);
	return 0;
}

static void __exit pltfm_dvc_by_device_id_exit(void)
{
	/*unregister the pseudo device*/
	platform_device_unregister(&plt_dvc);
	pr_info("module unloaded\n");
	pr_info("bye bye world\n");
}

module_init(pltfm_dvc_by_device_id_init);
module_exit(pltfm_dvc_by_device_id_exit);
MODULE_DESCRIPTION("pltfm_dvc_by_device_id example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");