#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#undef dev_fmt
#define dev_fmt(fmt) "%s : " fmt,__func__

#define W25Q32_DEVICE_ID	0x15
#define W25Q32_DEVICE_NAME	"w25q32"

#define WINBOARD_SERIAL_FLASH_MF_ID	0xEF

#define WRITE_EN 	 				0x06
#define WRITE_DI	 				0x04
#define READ_STATUS_REG1			0x05
#define READ_STATUS_REG2			0x35
#define WRITE_STATUS_REG			0x01
#define PAGE_PROGRAM				0x02
#define QUAD_PAGE_PROGRAM			0x32
#define BLOCK_ERASE_64				0xD8
#define BLOCK_ERASE_32				0x52
#define SECTOR_ERASE_4				0x20
#define CHIP_ERASE					0xC7
#define ERASE_SUSPEND				0x75
#define ERASE_RESUME				0x7A
#define POWER_DOWN					0xB9
#define HIGH_PERFORMANCE_MODE		0xA3
#define MODE_BIT_RESET				0xFF
#define RELEASE_POWER_DOWN_HPM		0xAB
#define MANUFACTURER_ID_READ_INST	0x90
#define UNIQUE_ID_READ				0x4B
#define JEDEC_ID					0x9F

#define READ_DATA_INST				0x03

#define W25Q32_PAGE_SIZE	256
#define W25Q32_PAGE_COUNT	(16*1024)
#define W25Q32_TOTAL_MEM	(W25Q32_PAGE_SIZE*W25Q32_PAGE_COUNT)	

struct w25q32_prv{
	struct cdev cdev;
	dev_t device_num;
	struct spi_device *spi;
};

unsigned int minor_count =0;
/*holds the class pointer needed for device creation and it's destruction*/
struct class *w25q32_class;

static bool w25q32_check_busy(struct spi_device *spi)
{
    int8_t ret,status_reg1 = READ_STATUS_REG1,busy = 1;
    
    while(busy == 1)
    {
	ret = spi_write_then_read(spi,&status_reg1,1,&busy,1);
	if(ret != 0)
	{
	    pr_err("w25q_check_busy 1 failed\n");
	}
	busy = (busy & 0x01);
	mdelay(10);
    }
    return 0;
}

static bool w25q32_write_instruction(struct spi_device *spi, uint8_t instruction)
{
    int8_t ret,status_reg1 = instruction;
     
	ret = spi_write(spi,&status_reg1,1);
	if(ret != 0)
	{
	    pr_err("w25q_check_busy 1 failed\n");
	}
    return 0;
}


static long driver_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
	return 0;
}

static loff_t driver_llseek(struct file *filp, loff_t offset, int whence)
{
	/*pre initialization*/
	loff_t temp;

	pr_info("lseek requested \n");
	pr_info("Current value of the file position = %lld\n",filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset > W25Q32_TOTAL_MEM -1) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > W25Q32_TOTAL_MEM -1) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = W25Q32_TOTAL_MEM - 1 + offset;
			if((temp > W25Q32_TOTAL_MEM -1) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL;
	}
	
	pr_info("New value of the file position = %lld\n",filp->f_pos);

	return filp->f_pos;
}

static ssize_t driver_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	int tmp_count,ret;
	char *rxbuff;
	uint32_t caddr;
	
	struct w25q32_prv *prv_data = (struct w25q32_prv *) filp->private_data;
	caddr = ((READ_DATA_INST << 24) |(*f_pos & 0x0FFF));
	tmp_count = count;

	if((count+*f_pos) >= W25Q32_TOTAL_MEM)
	{
		tmp_count = W25Q32_TOTAL_MEM - *f_pos - 1; /*restrict count value*/
	}

	/*allocate rx buff in kernel space*/
	rxbuff = (char*)kzalloc(sizeof(char)*tmp_count,GFP_KERNEL);
	if(!rxbuff){
		ret = -ENOMEM;
		dev_err(&prv_data->spi->dev,"No Mem Available\n");
		return ret;
	}

	dev_info(&prv_data->spi->dev,":read, checking busy flag\n");
	w25q32_check_busy(prv_data->spi);

	ret = spi_write_then_read(prv_data->spi,&caddr,4,rxbuff,tmp_count);
	if(ret){
		dev_err(&prv_data->spi->dev,"read failed\n");
		return -EFAULT;
	}

	if (copy_to_user(buff, rxbuff, tmp_count) != 0){
        ret = -EFAULT;
        return ret;
    }

    /*free the allocated rxbuff*/
    kfree(rxbuff);

    *f_pos = ((*f_pos & 0x0FFF) + tmp_count);

	return tmp_count;
}

static ssize_t driver_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	int tmp_count,ret,roll_count,tmp_index=0;
	char *txbuff,*tmp_char;
	uint32_t caddr,tmp_addr,offset;
	
	struct w25q32_prv *prv_data = (struct w25q32_prv *) filp->private_data;
	caddr = ((PAGE_PROGRAM << 24) |(*f_pos & 0x0FFF));
	tmp_addr = *f_pos & 0x0FFF;
	tmp_count = count;
	if((count+*f_pos) >= W25Q32_TOTAL_MEM)
	{
		count = W25Q32_TOTAL_MEM - *f_pos - 1; /*restrict count value*/
	}
	tmp_count = count;
	/*allocate tx buff in kernel space*/
	txbuff = (char*)kzalloc(sizeof(char)*(tmp_count+4),GFP_KERNEL);
	if(!txbuff){
		ret = -ENOMEM;
		dev_err(&prv_data->spi->dev,"No Mem Available\n");
		return ret;
	}

	tmp_char = (char*)caddr;
	txbuff[0] = tmp_char[0];
	txbuff[1] = tmp_char[1];
	txbuff[2] = tmp_char[2];
	txbuff[3] = tmp_char[3];

	if (copy_from_user(txbuff+4, buff, tmp_count) != 0){
        ret = -EFAULT;
        return ret;
    }


	offset = ((*f_pos & 0x0FFF) % W25Q32_PAGE_SIZE);
	do{
		if((offset + tmp_count) > (W25Q32_PAGE_SIZE)){
			roll_count = W25Q32_PAGE_SIZE - offset;

			dev_info(&prv_data->spi->dev,":write, checking busy flag\n");
			w25q32_check_busy(prv_data->spi);

			/*enable write*/
			w25q32_write_instruction(prv_data->spi,WRITE_EN);
			/*write the data*/
			ret=spi_write(prv_data->spi,txbuff+tmp_index,roll_count+4);
			if(ret){
				dev_err(&prv_data->spi->dev,"Write failed\n");
				return -EFAULT;
			}

			tmp_addr = ((PAGE_PROGRAM << 24))|((tmp_addr + roll_count)&0x0FFF);
			tmp_char = (char*)tmp_addr;
			txbuff[roll_count] = tmp_char[0];
			txbuff[roll_count+1] = tmp_char[0];
			txbuff[roll_count+2] = tmp_char[1];
			txbuff[roll_count+3] = tmp_char[2];

			tmp_index = roll_count;
			tmp_count = tmp_count - roll_count;
			offset=0;
		}
		else{
			dev_info(&prv_data->spi->dev,":write, checking busy flag\n");
			w25q32_check_busy(prv_data->spi);

			/*enable write*/
			w25q32_write_instruction(prv_data->spi,WRITE_EN);
			/*write the data*/
			ret=spi_write(prv_data->spi,txbuff+tmp_index,tmp_count+4);
			if(ret){
				dev_err(&prv_data->spi->dev,"Write failed\n");
				return -EFAULT;
			}
			tmp_addr = ((PAGE_PROGRAM << 24))|((tmp_addr + tmp_count)&0x0FFF);
			tmp_count=0;
		}
	}while(tmp_count > 0);

	kfree(txbuff);

	*f_pos = ((*f_pos & 0x0FFF) + (tmp_addr&0x0FFF));
	return count;

}

static int driver_open(struct inode *inode, struct file *filp)
{
	struct w25q32_prv *prv_data = container_of(inode->i_cdev,struct w25q32_prv,cdev); 
	filp->private_data = prv_data;	/*private data*/
	pr_info("open was successful\n");
	return 0;
}

static int driver_release(struct inode *inode, struct file *flip)
{
	pr_info("release was successful\n");
	return 0;
}

struct file_operations driver_fops=
{
	.open = driver_open,
	.release = driver_release,
	.read = driver_read,
	.write = driver_write,
	.llseek = driver_llseek,
	.unlocked_ioctl = driver_ioctl,
	.owner = THIS_MODULE
};

static int w25q32_probe(struct spi_device *spi){
	struct w25q32_prv *prv_data;
	int ret;
	uint8_t addr[4],id[2] = {0,};
	struct device *device_w25q32;

	/*allocate memory for private structure*/
	prv_data = (struct w25q32_prv*) devm_kzalloc(&spi->dev,sizeof(struct w25q32_prv),GFP_KERNEL);
	if(!prv_data){
		ret = -ENOMEM;
		dev_err(&spi->dev,"No Mem Available\n");
		goto out;
	}

	spi->mode = SPI_MODE_0;
	spi->max_speed_hz = 2000000;
	spi->bits_per_word = 8;
	prv_data->spi = spi;

	ret = spi_setup(spi);
	ret = spi_setup(spi);
	if(ret){
		dev_err(&spi->dev, "Spi setup failed\n");
		goto free_mem;
	}

	/*Test communication*/
	addr[0] = MANUFACTURER_ID_READ_INST;
    addr[1] = 0x00; 
    addr[2] = 0x00;
    addr[3] = 0x00;
    
    spi_write_then_read(spi,addr,4,id,2);
    pr_info("spi_read = %x %x\n",id[0],id[1]);
    
    if(id[0] != WINBOARD_SERIAL_FLASH_MF_ID || id[1] != W25Q32_DEVICE_ID)
    {
		dev_info(&spi->dev,"w25q25 not found!!\n");
    }else{
    	dev_info(&spi->dev,"w25q25 found!!\n");
    }
    
    /*allocate the device number*/
	ret = alloc_chrdev_region(&prv_data->device_num,minor_count,1,W25Q32_DEVICE_NAME);
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		goto free_mem;
	}
	pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(prv_data->device_num),MINOR(prv_data->device_num));

	/*create device class under /sys/class/ */
	w25q32_class = class_create(THIS_MODULE, W25Q32_DEVICE_NAME);
	if(IS_ERR(w25q32_class)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(w25q32_class);
		goto unreg_chrdev;
	}

	/*Initialize the cdev structure with fops*/
	cdev_init(&prv_data->cdev,&driver_fops);

	/*Register a device (cdev structure) with VFS */
	prv_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&prv_data->cdev,prv_data->device_num,1);
	if(ret < 0){
		pr_err("Cdev add failed\n");
		goto class_del;
	}

	/*populate the sysfs with device information and save the prv_data info*/
	device_w25q32 = device_create(w25q32_class,NULL,prv_data->device_num,(void*)prv_data,"w25q32-dev%d",minor_count);
	if(IS_ERR(device_w25q32)){
		pr_err("Device create failed\n");
		ret = PTR_ERR(device_w25q32);
		goto cdev_del;
	}

	pr_info("Module init was successful\n");


	/*now we are assured device is created successfully so can increase the minor count*/
	minor_count++;
	return 0;

	cdev_del:
		cdev_del(&prv_data->cdev);
	class_del:
		class_destroy(w25q32_class);
	unreg_chrdev:
		unregister_chrdev_region(prv_data->device_num,1);
	free_mem:
		devm_kfree(&spi->dev,prv_data); /*not needed but still for completness*/
	out:
		pr_info("Module insertion failed\n");
		return ret;

}

static int w25q32_remove(struct spi_device *spi){
	
	/*get the prv data*/
	struct w25q32_prv *prv_data = (struct w25q32_prv*) spi_get_drvdata(spi);
	/*destory the device*/
	device_destroy(w25q32_class,prv_data->device_num);
	/*destory the class*/
	class_destroy(w25q32_class);
	/*delete cdev struct*/
	cdev_del(&prv_data->cdev);
	/*unregister device number*/
	unregister_chrdev_region(prv_data->device_num,1);
	/*decrement minor_count*/
	minor_count--;

	//devm will handle everything
	dev_info(&spi->dev,"Bye Bye world\n");
	return 0;
}

/*OF match table registration*/
static const struct of_device_id of_table_w25q32_ids[] = 
{
	{ .compatible = "w25q32,flash", .data = NULL},
	{ } /*NULL TERMINATION*/
};

MODULE_DEVICE_TABLE(of, of_table_w25q32_ids);

/*spi device id table registration*/
static const struct spi_device_id w25q32_id[] = {
	{ "w25q32,flash", W25Q32_DEVICE_ID },
	{ }
};

MODULE_DEVICE_TABLE(spi, w25q32_id);

static struct spi_driver w25q32_driver = {
	.driver = {
		.name = "w25q32",
		.of_match_table = of_match_ptr(of_table_w25q32_ids),
		.owner = THIS_MODULE,
	},
	.probe = w25q32_probe,
	.remove = w25q32_remove,
	.id_table = w25q32_id,
};

/*register i2c_driver module using Macro*/
module_spi_driver(w25q32_driver);

MODULE_DESCRIPTION("w25q32_drv example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");
