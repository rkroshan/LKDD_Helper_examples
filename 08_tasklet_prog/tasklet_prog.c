#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>

/*dummy tasklet data to pass*/
char data[] = "I came as unsigned long data type LOL";

/*tasklet function to call*/
static void tasklet_handler(unsigned long data)
{
	/*we know that data is an addr of charp*/
	pr_info("%s gets data as: %s\n",__FUNCTION__,(char*)data);
}

/*delclare tasklet struct*/
DECLARE_TASKLET(data_tasklet,tasklet_handler,(unsigned long)data);

static int __init tasklet_prog_init(void)
{
	pr_info("TASKLET PROG EXAMPLE");
	/*schedule the tasklet*/
	pr_info("%s schedule the tasklet\n",__FUNCTION__);
	tasklet_schedule(&data_tasklet);
	return 0;
}

static void __exit tasklet_prog_exit(void)
{
	/*kill the tasklet*/
	tasklet_kill(&data_tasklet);

	pr_info("Bye bye world\n");
}

module_init(tasklet_prog_init);
module_exit(tasklet_prog_exit);
MODULE_DESCRIPTION("tasklet_prog example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");