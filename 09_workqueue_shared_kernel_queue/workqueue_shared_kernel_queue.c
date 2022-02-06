#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define LEN 7

/*decalre data struct*/
struct work_data{
	/*declare workstruct*/
	struct work_struct wrk;
	char* attr;
};

/*define work handler*/
static void work_handler(struct work_struct *work)
{
	/*pre initialization*/
	struct work_data *wrk_data;

	pr_info("workqueue work handler %s\n",__FUNCTION__);
	/*get the work_data structure pointer*/
	wrk_data = container_of(work,struct work_data,wrk);
	pr_info("%s retrived data : %s\n",__FUNCTION__,wrk_data->attr);
	kfree(wrk_data);	
}

static int __init workqueue_shared_kernel_queue_init(void)
{
	/*pre initialization*/
	int i;
	struct work_data *wrk_data;
	char tmp[LEN] = "ROSHAN";

	pr_info("workqueue_shared_kernel_queue example");

	/*dynamically allocate work_data struct*/
	wrk_data = (struct work_data*)kmalloc(sizeof(*wrk_data),GFP_KERNEL);
	/*allocate memory for attr*/
	wrk_data->attr = (char*)kmalloc(sizeof(LEN),GFP_KERNEL);
	for(i=0;i<LEN-1;i++)
	{
		wrk_data->attr[i]=tmp[i];
	}
	wrk_data->attr[i]='\0';

	/*initialize and schedule the work*/
	INIT_WORK(&(wrk_data->wrk),work_handler);
	schedule_work(&(wrk_data->wrk));

	pr_info("woken up by work queue\n");
	return 0;
}

static void __exit workqueue_shared_kernel_queue_exit(void)
{

	pr_info("Bye bye world\n");
}

module_init(workqueue_shared_kernel_queue_init);
module_exit(workqueue_shared_kernel_queue_exit);
MODULE_DESCRIPTION("workqueue_shared_kernel_queue example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");