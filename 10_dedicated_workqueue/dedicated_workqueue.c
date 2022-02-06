#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define LEN 7

/*create workqueue struct */
static struct workqueue_struct *wrkqueue;

/*declare data struct*/
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

static int __init dedicated_workqueue_init(void)
{
	/*pre initialization*/
	int i;
	struct work_data *wrk_data;
	char tmp[LEN] = "ROSHAN";

	pr_info("dedicated_workqueue example");

	/*allocate memory for workqueue*/
	wrkqueue = create_singlethread_workqueue("thread_name_cool");
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
	/*queue the work*/
	queue_work(wrkqueue,&(wrk_data->wrk));

	return 0;
}

static void __exit dedicated_workqueue_exit(void)
{
	/*flush the workqueue as we know we are only using it so no other work be pending on our queue*/
	pr_info("%s workqueue flushed\n",__FUNCTION__);
	flush_workqueue(wrkqueue);
	/*destory the wrkqueue*/
	pr_info("%s workqueue destoryed\n",__FUNCTION__);
	destroy_workqueue(wrkqueue);

	pr_info("Bye bye world %s\n",__FUNCTION__);
}

module_init(dedicated_workqueue_init);
module_exit(dedicated_workqueue_exit);
MODULE_DESCRIPTION("dedicated_workqueue example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");