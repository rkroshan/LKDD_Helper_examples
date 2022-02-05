#include <linux/init.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#define ms_delay	900
/*milli secons to nano seconds 1E6L == 1000000*/
#define MS_TO_NS(x) (x*1E6L)

/*Declare HR timer*/
static struct hrtimer my_hr_timer;

/*hr_timer callback function*/
enum hrtimer_restart hrtimer_restart_callback(struct hrtimer* hr_timer)
{
	pr_info("%s is called\n", __FUNCTION__);
	return HRTIMER_NORESTART; /*HRTIMER_RESTART which will restart the hr timer with same timeout*/
}

static int __init hr_timer_prog_init(void)
{
	/*pre initialization*/
	ktime_t my_ktime;

	pr_info("HR TIMER MODULE INIT\n");

	/*setup ktime*/
	my_ktime = ktime_set(0, MS_TO_NS(ms_delay));
	/*init hr_timer*/
	hrtimer_init(&my_hr_timer,CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	my_hr_timer.function = &hrtimer_restart_callback;

	pr_info("starting HR timer in %dms jiffies:%ld",ms_delay,jiffies);
	hrtimer_start(&my_hr_timer,my_ktime, HRTIMER_MODE_REL);

	return 0;
}

static void __exit hr_timer_prog_exit(void)
{
	int ret;
	ret = hrtimer_cancel(&my_hr_timer);
	/*success = 0 | timer is still in use = 1*/
	if(ret)
		pr_info("hr_timer is still in use...\n");
	
	pr_info("Bye bye world\n");
}

module_init(hr_timer_prog_init);
module_exit(hr_timer_prog_exit);
MODULE_DESCRIPTION("hr_timer_prog example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");