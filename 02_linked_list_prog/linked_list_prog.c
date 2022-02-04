#include <linux/init.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>

/*static declaration of list head*/
static LIST_HEAD(basic_info_list);

/*basic info struct*/
struct basic_info{
	int data;
	struct list_head list;
};

/*static struct of basic info*/
struct basic_info b0 = { .data = 0,};
struct basic_info *b1=NULL,*b2=NULL,*b3=NULL,*b4=NULL,*b5=NULL;

static int __init linked_list_prog_init(void)
{
	/*pre variable initializations*/
	struct basic_info *loop_counter;

	/*allocate memory to dynamic structs*/
	b1 = (struct basic_info*)kzalloc(sizeof(*b1),GFP_KERNEL);
	b2 = (struct basic_info*)kzalloc(sizeof(*b2),GFP_KERNEL);
	b3 = (struct basic_info*)kzalloc(sizeof(*b3),GFP_KERNEL);
	b4 = (struct basic_info*)kzalloc(sizeof(*b4),GFP_KERNEL);
	b5 = (struct basic_info*)kzalloc(sizeof(*b5),GFP_KERNEL);

	/*init the list heads*/
	INIT_LIST_HEAD(&(b0.list));
	INIT_LIST_HEAD(&b1->list);
	INIT_LIST_HEAD(&b2->list);
	INIT_LIST_HEAD(&b3->list);
	INIT_LIST_HEAD(&b4->list);
	INIT_LIST_HEAD(&b5->list);

	/*add the struct to basic_info_list*/
	list_add(&(b0.list), &basic_info_list);
	list_add(&b1->list, &basic_info_list); /*add from head*/
	list_add_tail(&b2->list, &basic_info_list); /*add from last*/
	list_add(&b3->list, &basic_info_list);
	list_add(&b4->list, &basic_info_list);
	list_add(&b5->list, &basic_info_list);

	/*add data element to each struct*/
	b1->data = 1;
	b2->data = 2;
	b3->data = 3;
	b4->data = 4;
	b5->data = 5;

	/*recursive list traversal*/
	list_for_each_entry(loop_counter,&basic_info_list,list)
	{
		pr_info("LIST ELEMENT DATA :%d\n",loop_counter->data);
	}

	return 0;
}

static void __exit linked_list_prog_exit(void)
{
	/*del list elemets, this will clear the prev and next pointers to NULL for each list*/
	list_del(&(b0.list));
	list_del(&b1->list);
	list_del(&b2->list);
	list_del(&b3->list);
	list_del(&b4->list);
	list_del(&b5->list);
	list_del(&basic_info_list);

	/*free the dynamic mem*/
	kfree(b1);
	kfree(b2);
	kfree(b3);
	kfree(b4);
	kfree(b5);

	pr_info("Bye bye world\n");
}

module_init(linked_list_prog_init);
module_exit(linked_list_prog_exit);
MODULE_DESCRIPTION("linked_list_prog example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");