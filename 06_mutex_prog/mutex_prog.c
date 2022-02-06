#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

/*Declare a mutex statically*/
DEFINE_MUTEX(data_mutex);

/*Resource to access*/
#define LEN 7
char data[LEN] = "ROSHAN";

/*declare workstruct going to use shared qorkqueue*/
static struct work_struct wrk;

/*define work handler*/
static void work_handler(struct work_struct *work)
{
	/*pre initializations*/
	char tmp[LEN] = "admire";
	int i,ret;

	pr_info("workqueue work handler %s\n",__FUNCTION__);
	/*access the data resource and change name to admire*/
	/*take the mutex*/
	pr_info("%s mutex lock data:%s\n", __FUNCTION__,data);
	ret=mutex_lock_interruptible(&data_mutex);
	/*checking if mutex is actually locked or returned because of signal interruption*/
		if(ret!=0)
		{
			pr_info("%s mutex lock interrupted by signal\n", __FUNCTION__);
			return;
		}
	for(i=0;i<LEN-1;i++)
	{
		data[i] = tmp[i];
	}
	data[i] = '\0';
	/*unlock the mutex*/
	pr_info("%s mutex unlock data:%s\n", __FUNCTION__,data);
	mutex_unlock(&data_mutex);
}

static int __init mutex_prog_init(void)
{
	/*pre initializations*/
	char tmp[LEN] = "creato";
	int i,ret;

	pr_info("MUTEX PROG EXAMPLE\n");
	INIT_WORK(&wrk,work_handler);
	schedule_work(&wrk);

	/*take the mutex*/
	pr_info("%s mutex lock data:%s\n", __FUNCTION__,data);
	ret=mutex_lock_interruptible(&data_mutex);
	/*checking if mutex is actually locked or returned because of signal interruption*/
		if(ret!=0)
		{
			pr_info("%s mutex lock interrupted by signal\n", __FUNCTION__);
			return -ERESTARTSYS;
		}
	for(i=0;i<LEN-1;i++)
	{
		data[i] = tmp[i];
	}
	data[i] = '\0';
	/*unlock the mutex*/
	pr_info("%s mutex unlock data:%s\n", __FUNCTION__,data);
	mutex_unlock(&data_mutex);

	return 0;
}

static void __exit mutex_prog_exit(void)
{
	
	pr_info("Bye bye world\n");
}

module_init(mutex_prog_init);
module_exit(mutex_prog_exit);
MODULE_DESCRIPTION("mutex_prog example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");