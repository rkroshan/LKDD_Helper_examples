#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/uaccess.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

#define GENERIC_I2C_DRV_NAME	"generic_i2c_dev_rk"

/*global variable to keep track of minors assigned*/
unsigned long minor_count = 0;

/*i2c device prv data structure*/
struct i2c_private_data_struct{
	dev_t device_num;
	struct cdev cdev;
	struct i2c_client *client;
};

/*defining file operation struct*/
/*first declare the file operation*/
static ssize_t i2c_generic_driver_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	int ret; char buf[count];
	struct i2c_private_data_struct *prv_data ;

	prv_data = (struct i2c_private_data_struct*) filp->private_data;
	/*send the buf data to slave device*/

	if((ret = i2c_master_recv(prv_data->client,buf,count)) != count){
		return ret; 
	}

	if (copy_to_user(buff, buf, count) != 0){
        ret = -EFAULT;
        return ret;
    }

	return count;
}
static ssize_t i2c_generic_driver_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	int ret; char buf[count];
	struct i2c_private_data_struct *prv_data ;

	prv_data = (struct i2c_private_data_struct*) filp->private_data;

	/*send the buf data to slave device*/
	if (copy_from_user(buf, buff, count) != 0){
        ret = -EFAULT;
        return ret;
    }

	if((ret = i2c_master_send(prv_data->client,buf,count)) != count){
		return ret; 
	}

	return count;
}
static int i2c_generic_driver_open(struct inode *inode, struct file *filp)
{
	struct i2c_private_data_struct *prv_data;
	prv_data = container_of(inode->i_cdev,struct i2c_private_data_struct,cdev);
	pr_info("open was successful\n");
	/*save prv_data into filp, we know that prv_data is global struct but just for example what if we need to store data structure which is not global*/
	filp->private_data = (void*) prv_data;
	return 0;
}
static int i2c_generic_driver_release(struct inode *inode, struct file *flip)
{
	pr_info("release was successful\n");
	return 0;
}

struct file_operations i2c_generic_driver_fops=
{
	.open = i2c_generic_driver_open,
	.release = i2c_generic_driver_release,
	.read = i2c_generic_driver_read,
	.write = i2c_generic_driver_write,
	.owner = THIS_MODULE
};

/*holds the class pointer needed for device creation and it's destruction*/
struct class *i2c_genclass;

static int initiate_dummy_transfer(struct i2c_client* client)
{
	/*YOU CAN MODIFY client->addr,buf or count before initiating the transfer*/
	/*sending 2 bytes*/
	int count  = 2, ret;
	char buf[2] = {0x00,0x00};
	if((ret = i2c_master_send(client,buf,count)) != count){
		return ret; 
	}
	/*receive 2 bytes*/
	if((ret = i2c_master_recv(client,buf,count)) != count)
	{
		return ret;
	}

	return count;
}

/*Defining static functions*/
static int generic_i2c_driver_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
	/*pre initialization*/
	int ret;
	struct device *device_i2c_generic;
	struct i2c_private_data_struct *prv_data;

	pr_info("Device named : %s is probed\n",client->name);

	/*check whether i2c bus controller of the soc supports the functionality needed by your device*/
	/*I2C_FUNC_SMBUS_BYTE_DATA : I2C functionality to read write byte data*/
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -EIO;

    /*initiate a dummy transfer to check if i2c communication working with slave*/
    if( (ret = initiate_dummy_transfer(client)) < 0){
    	pr_err("i2c dummy transfer failed err_code:%d\n", ret);    	
    }

    /*Register the device so that user can access it*/
    /*allocate the memory for prv struct linked to dev struct*/
    prv_data = devm_kmalloc(&client->dev,sizeof(*prv_data),GFP_KERNEL);
    if(prv_data == NULL){
    	pr_err("failed to allocate memory for private data struct");
    	return -ENOMEM;
    }

    /*allocate the device number*/
	ret = alloc_chrdev_region(&prv_data->device_num,minor_count,1,GENERIC_I2C_DRV_NAME);
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		goto out;
	}
	pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(prv_data->device_num),MINOR(prv_data->device_num));

	/*create device class under /sys/class/ */
	i2c_genclass = class_create(THIS_MODULE, GENERIC_I2C_DRV_NAME);
	if(IS_ERR(i2c_genclass)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(i2c_genclass);
		goto unreg_chrdev;
	}

	/*Initialize the cdev structure with fops*/
	cdev_init(&prv_data->cdev,&i2c_generic_driver_fops);

	/*save i2c client pointer*/
	prv_data->client = client;

	/*Register a device (cdev structure) with VFS */
	prv_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&prv_data->cdev,prv_data->device_num,1);
	if(ret < 0){
		pr_err("Cdev add failed\n");
		goto class_del;
	}

	/*populate the sysfs with device information and save the prv_data info*/
	device_i2c_generic = device_create(i2c_genclass,NULL,prv_data->device_num,(void*)prv_data,"i2c-gen-dev%ld",minor_count);
	if(IS_ERR(device_i2c_generic)){
		pr_err("Device create failed\n");
		ret = PTR_ERR(device_i2c_generic);
		goto cdev_del;
	}

	pr_info("Module init was successful\n");


	/*now we are assured device is created successfully so can increase the minor count*/
	minor_count++;
	return 0;

	cdev_del:
		cdev_del(&prv_data->cdev);
	class_del:
		class_destroy(i2c_genclass);
	unreg_chrdev:
		unregister_chrdev_region(prv_data->device_num,1);
	out:
		devm_kfree(&client->dev,prv_data); /*not needed but still for completness*/
		pr_info("Module insertion failed\n");
		return ret;
}

static int generic_i2c_driver_remove(struct i2c_client* client)
{
	/*get the prv data*/
	struct i2c_private_data_struct *prv_data = (struct i2c_private_data_struct*) i2c_get_clientdata(client);
	/*destory the device*/
	device_destroy(i2c_genclass,prv_data->device_num);
	/*destory the class*/
	class_destroy(i2c_genclass);
	/*delete cdev struct*/
	cdev_del(&prv_data->cdev);
	/*unregister device number*/
	unregister_chrdev_region(prv_data->device_num,1);
	/*decrement minor_count*/
	minor_count--;

	pr_info("Device named : %s is removed\n",client->name);
	return 0;
}


/*Defining the generic_i2c_device_ids*/
static const struct i2c_device_id generic_i2c_device_ids[] = {
	{.name = "generic-i2c-device", .driver_data = 0},
	{ } /*NULL Termination*/
};

MODULE_DEVICE_TABLE(i2c,generic_i2c_device_ids);

/*Defining of_generic_i2c_device_ids table */
static const struct of_device_id of_generic_i2c_device_ids[] = {
	{.compatible = "generic,i2c-device" , .data = NULL},
	{ }/*Null Termination*/
};

MODULE_DEVICE_TABLE(of,of_generic_i2c_device_ids);

/*Defining generic i2c driver structure*/
static struct i2c_driver generic_i2c_driver = {
	.driver = {
		.name = "generic_i2c_driver",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(of_generic_i2c_device_ids),
	},
	.id_table = generic_i2c_device_ids,
	.probe = generic_i2c_driver_probe,
	.remove = generic_i2c_driver_remove,
};

/*register i2c_driver module using Macro*/
module_i2c_driver(generic_i2c_driver);

MODULE_DESCRIPTION("generic_i2c_driver example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");

