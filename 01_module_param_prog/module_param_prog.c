#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

static int myint = 7;
static char *myattr = "cool program";
static int myarr[3] = {9,8,7};

module_param(myint,int,S_IRUGO);
module_param(myattr, charp, S_IRUGO);
module_param_array(myarr,int,NULL,S_IRUGO|S_IWUSR);


static int __init module_param_prog_init(void)
{
	pr_info("module_param_prog myint:%d\n", myint);
	pr_info("module_param_prog myattr:%s\n", myattr);
	pr_info("module_param_prog myarr[0]:%d myarr[1]:%d myarr[2]:%d\n", myarr[0],myarr[1],myarr[2]);

	return 0;
}

static void __exit module_param_prog_exit(void)
{
	pr_info("Bye Bye world\n");
}

module_init(module_param_prog_init);
module_exit(module_param_prog_exit);
MODULE_DESCRIPTION("module_param_prog example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");