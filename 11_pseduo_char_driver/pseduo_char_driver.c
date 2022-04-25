#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

#define CHRDEV_NAME	"psd_device"
#define CHRDEV_CLASS_NAME "psd_class"
#define CHRDEV_DEVICE_NAME "psd"
/*Pseduo device with DEV_MEM memory*/
#define DEV_MEM_SIZE 512
/* pseudo device's memory */
char device_buffer[DEV_MEM_SIZE];
/*Device private data structure */
struct device_private_data
{
	char *buffer;
	unsigned size;
	struct cdev cdev;
	/* This holds the device number */
	dev_t device_number;
};
/*defining one pseduo device*/
struct device_private_data prv_data = {
	.buffer = device_buffer,
	.size = DEV_MEM_SIZE,
};
/*defining file operation struct*/
/*Declare mutex*/
DEFINE_MUTEX(psd_mutex);
/*first declare the file operation*/
static loff_t psd_llseek(struct file *filp, loff_t offset, int whence);
static ssize_t psd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
static ssize_t psd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
static int psd_open(struct inode *inode, struct file *filp);
static int psd_release(struct inode *inode, struct file *flip);

struct file_operations psd_fops=
{
	.open = psd_open,
	.release = psd_release,
	.read = psd_read,
	.write = psd_write,
	.llseek = psd_llseek,
	.owner = THIS_MODULE
};
/*holds the class pointer needed for devicd creation and it's destruction*/
struct class *class_psd;


static int __init pseduo_char_driver_init(void)
{
	/*pre initialization*/
	int ret;
	struct device *device_psd;

	pr_info("MODULE INIT\n");
	/*allocate the device number*/
	ret = alloc_chrdev_region(&prv_data.device_number,0,1,CHRDEV_NAME);
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		goto out;
	}
	pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(prv_data.device_number),MINOR(prv_data.device_number));

	/*create device class under /sys/class/ */
	class_psd = class_create(THIS_MODULE, CHRDEV_CLASS_NAME);
	if(IS_ERR(class_psd)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(class_psd);
		goto unreg_chrdev;
	}

	/*Initialize the cdev structure with fops*/
	cdev_init(&prv_data.cdev,&psd_fops);

	/*Register a device (cdev structure) with VFS */
	prv_data.cdev.owner = THIS_MODULE;
	ret = cdev_add(&prv_data.cdev,prv_data.device_number,1);
	if(ret < 0){
		pr_err("Cdev add failed\n");
		goto class_del;
	}

	/*populate the sysfs with device information */
	device_psd = device_create(class_psd,NULL,prv_data.device_number,NULL, CHRDEV_DEVICE_NAME);
	if(IS_ERR(device_psd)){
		pr_err("Device create failed\n");
		ret = PTR_ERR(device_psd);
		goto cdev_del;
	}

	pr_info("Module init was successful\n");


	return 0;

	cdev_del:
		cdev_del(&prv_data.cdev);
	class_del:
		class_destroy(class_psd);
	unreg_chrdev:
		unregister_chrdev_region(prv_data.device_number,1);
	out:
		pr_info("Module insertion failed\n");
		return ret;
}

static void __exit pseduo_char_driver_exit(void)
{
	device_destroy(class_psd,prv_data.device_number);
	class_destroy(class_psd);
	cdev_del(&prv_data.cdev);
	unregister_chrdev_region(prv_data.device_number,1);
	pr_info("module unloaded\n");
	pr_info("Bye Bye World \n");
}

static loff_t psd_llseek(struct file *filp, loff_t offset, int whence)
{
	/*pre initialization*/
	struct device_private_data *xprv_data = (struct device_private_data*)filp->private_data;
	loff_t temp;

	pr_info("lseek requested \n");
	pr_info("Current value of the file position = %lld\n",filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset > xprv_data->size -1) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > xprv_data->size -1) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = xprv_data->size - 1 + offset;
			if((temp > xprv_data->size -1) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL;
	}
	
	pr_info("New value of the file position = %lld\n",filp->f_pos);

	return filp->f_pos;
}
static ssize_t psd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	/*pre initialization*/
	struct device_private_data *xprv_data = (struct device_private_data*)filp->private_data;

	if(mutex_lock_interruptible(&psd_mutex))
			return -EINTR;
		
	pr_info("Read requested for %zu bytes \n",count);
	pr_info("Current file position = %lld\n",*f_pos);

	
	/* Adjust the 'count' */
	if((*f_pos + count) > xprv_data->size)
		count = xprv_data->size - *f_pos;

	/*copy to user */
	if(copy_to_user(buff,&(xprv_data->buffer[*f_pos]),count)){
		mutex_unlock(&psd_mutex);
		return -EFAULT;
	}
	/*update the current file postion */
	*f_pos += count;

	pr_info("Number of bytes successfully read = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_pos);

	//spin_unlock(&pcd_spin_lock);
	mutex_unlock(&psd_mutex);
	
	/*Return number of bytes which have been successfully read */
	return count;
}
static ssize_t psd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	/*pre initialization*/
	struct device_private_data *xprv_data = (struct device_private_data*)filp->private_data;

	if(mutex_lock_interruptible(&psd_mutex))
			return -EINTR;

	pr_info("Write requested for %zu bytes\n",count);
	pr_info("Current file position = %lld\n",*f_pos);

	
	/* Adjust the 'count' */
	if((*f_pos + count) > xprv_data->size)
		count = xprv_data->size - *f_pos;

	if(!count){
		pr_err("No space left on the device \n");
		return -ENOMEM;
	}

	/*copy from user */
	if(copy_from_user(&(xprv_data->buffer[*f_pos]),buff,count)){
		mutex_unlock(&psd_mutex);
		return -EFAULT;
	}
	/*update the current file postion */
	*f_pos += count;

	pr_info("Number of bytes successfully written = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_pos);
	
	//spin_unlock(&pcd_spin_lock);
	mutex_unlock(&psd_mutex);

	/*Return number of bytes which have been successfully written */
	return count;
}
static int psd_open(struct inode *inode, struct file *filp)
{
	pr_info("open was successful\n");
	/*save prv_data into filp, we know that prv_data is global struct but just for example what if we need to store data structure which is not global*/
	filp->private_data = (void*) &prv_data;
	return 0;
}
static int psd_release(struct inode *inode, struct file *flip)
{
	pr_info("release was successful\n");
	return 0;
}

module_init(pseduo_char_driver_init);
module_exit(pseduo_char_driver_exit);
MODULE_DESCRIPTION("pseduo_char_driver example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");