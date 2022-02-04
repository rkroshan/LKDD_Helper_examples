#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>

#define ms_delay	700

/*declare timer struct*/
static struct timer_list my_timer;

/*define timer callback*/
static void timer_callback(unsigned long data)
{
	pr_info("%s called at %ld data:%ld\n", __FUNCTION__,jiffies,data);
}

static int __init std_timer_prog_init(void)
{
	int ret;
	
	pr_info("Standard Timer example");
	/*setup the timer*/
	setup_timer(&my_timer,timer_callback,9);
	/*reset the timeout*/
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(ms_delay));

	/*timer timeout set (ret = 0) | failed (ret = 1)*/
	if(ret)
		pr_info("Timer timeout init failed\n");

	return 0;
}

static void __exit std_timer_prog_exit(void)
{
	int ret;
	ret=del_timer(&my_timer);

	/*if timer active (ret=1) else ret=0*/
	if(ret)
		pr_info("timer is still is use ...\n");
	
	pr_info("Bye bye world\n");
}

module_init(std_timer_prog_init);
module_exit(std_timer_prog_exit);
MODULE_DESCRIPTION("std_timer_prog example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");