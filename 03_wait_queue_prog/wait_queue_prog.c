#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

/*declare wait queue*/
static DECLARE_WAIT_QUEUE_HEAD(my_wq);
static int condition=0;

/*declare workqueue*/
static struct work_struct wrk;

/*define work handler*/
static void work_handler(struct work_struct *work)
{
	pr_info("workqueue work handler %s\n",__FUNCTION__);
	msleep(4000);
	pr_info("Wake up the sleeping module\n");
	condition=1;
	wake_up_interruptible(&my_wq);
}

static int __init wait_queue_prog_init(void)
{
	pr_info("Wait Queue example");
	INIT_WORK(&wrk,work_handler);
	schedule_work(&wrk);

	pr_info("Going to sleep %s\n",__FUNCTION__);
	wait_event_interruptible(my_wq,condition !=0);

	pr_info("woken up by work queue\n");
	return 0;
}

static void __exit wait_queue_prog_exit(void)
{

	pr_info("Bye bye world\n");
}

module_init(wait_queue_prog_init);
module_exit(wait_queue_prog_exit);
MODULE_DESCRIPTION("wait_queue_prog example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");