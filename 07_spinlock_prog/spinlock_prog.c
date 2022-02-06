#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

/*Declare a spinlock statically*/
DEFINE_SPINLOCK(data_spinlock);

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
	int i;
	unsigned long flags;

	pr_info("workqueue work handler %s\n",__FUNCTION__);
	/*access the data resource and change name to admire*/
	/*take the spinlock*/
	pr_info("%s spinlock lock data:%s\n", __FUNCTION__,data);
	spin_lock_irqsave(&data_spinlock,flags);
	for(i=0;i<LEN-1;i++)
	{
		data[i] = tmp[i];
	}
	data[i] = '\0';
	/*unlock the spinlock*/
	pr_info("%s spinlock unlock data:%s\n", __FUNCTION__,data);
	spin_unlock_irqrestore(&data_spinlock,flags);
}

static int __init spinlock_prog_init(void)
{
	/*pre initializations*/
	char tmp[LEN] = "creato";
	int i;
	unsigned long flags;

	pr_info("SPINLOCK PROG EXAMPLE\n");
	INIT_WORK(&wrk,work_handler);
	schedule_work(&wrk);

	/*take the spinlock*/
	pr_info("%s spinlock lock data:%s\n", __FUNCTION__,data);
	spin_lock_irqsave(&data_spinlock,flags);
	for(i=0;i<LEN-1;i++)
	{
		data[i] = tmp[i];
	}
	data[i] = '\0';
	/*unlock the spinlock*/
	pr_info("%s spinlock unlock data:%s\n", __FUNCTION__,data);
	spin_unlock_irqrestore(&data_spinlock,flags);

	return 0;
}

static void __exit spinlock_prog_exit(void)
{
	
	pr_info("Bye bye world\n");
}

module_init(spinlock_prog_init);
module_exit(spinlock_prog_exit);
MODULE_DESCRIPTION("spinlock_prog example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");