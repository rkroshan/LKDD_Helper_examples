
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#undef dev_fmt
#define dev_fmt(fmt) "%s : " fmt,__func__

/*in bytes*/
#define TOTAL_MEM_SIZE	(256*1024/8)
#define PAGE_SIZE_AT24C256	64
#define PAGE_COUNT	((TOTAL_MEM_SIZE)/PAGE_SIZE_AT24C256)

/*global variable to keep track of minors assigned*/
unsigned long minor_count = 0;

/*i2c device prv data structure*/
struct i2c_private_data_struct{
	struct i2c_client *client;
	uint16_t reg_addr;
};

/*static declaration for functions*/
static int at24c256_i2c_byte_read(struct i2c_client* client, uint16_t addr, uint8_t* send_byte);
static int at24c256_i2c_byte_write(struct i2c_client* client, uint16_t addr, uint8_t data);

/*
Create sysfs attributes for user space interaction with the device
1. reg_addr : starting address to which read/write will happen (has to be hexadecimal (has to be in 2bytes format 0x0000) but remove 0x, e.g 0xad43 => ad43)\n
2. read_byte : read a single byte from dev_addr, after read the dev_addr will be incremented by 1 (address roll over happens from last byte of last mempage to first byte of first mempage)\n
3. write_byte : write a single byte from dev_addr, after write the dev_addr will be incremented by 1 (address roll over happens from last byte of the page to first byte of the same page)\n
4. read_page (TODO) : read a page of bytes, starting from dev_addr (Note: dev_addr should be multiple of 64 or should be zero), after operation dev_addr will be incremented by no of bytes read\n
5. write_page (TODO): write a page of bytes, staring from dev_addr (Note: dev_addr should be multiple of 64 or should be zero), after operation dev_addr will be incremented by no of bytes write\n
6. usage : prints usage of each functionality
*/

/*creating usage attr*/
static ssize_t show_usage(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "\n"
		"Device name: at24c256 eeprom\n"
		"TOTAL_MEM_SIZE : %d bytes (0 - %d)\n" 
		"PAGE_SIZE_AT24C256 : %d bytes\n"
		"PAGE_COUNT : %d\n"
		"Attribues: \n"
		"1. reg_addr : starting address to which read/write will happen (has to be hexadecimal (has to be in 2bytes format 0x0000)\n"
		"2. read_byte : read a single byte from dev_addr, after read the dev_addr will be incremented by 1 (address roll over happens from last byte of last mempage to first byte of first mempage)\n"
		"3. write_byte : write a single byte from dev_addr (but be in hex format), after write the dev_addr will be incremented by 1 (address roll over happens from last byte of the page to first byte of the same page)\n"
		"4. read_page (TODO) : read a page of bytes, starting from dev_addr (Note: dev_addr should be multiple of 64 or should be zero), after operation dev_addr will be incremented by no of bytes read\n"
		"5. write_page (TODO): write a page of bytes (must be hex/decimal format), staring from dev_addr (Note: dev_addr should be multiple of 64 or should be zero), after operation dev_addr will be incremented by no of bytes write\n"
		"6. usage : prints usage of each functionality\n", TOTAL_MEM_SIZE, TOTAL_MEM_SIZE-1, PAGE_SIZE_AT24C256, PAGE_COUNT);
}

static DEVICE_ATTR(usage_attr, S_IRUGO, show_usage, NULL);


static int convert_char_to_hex_reg(char* in,int count, int *out)
{
	unsigned char tmp_char;
	int i; 
	*out = 0;
	for(i=0;i<count;i++)
	{
		if(in[i] <= '9' && in[i] >= '0'){
			tmp_char = '0';
		}
		else if(in[i] <= 'f' && in[i] >='a'){
			tmp_char = 'a';
		}
		else if(in[i] <= 'F' && in[i] >='A'){
			tmp_char = 'A';
		}
		else
		{
			*out = -1;
			return -1;
		}

		*out = (((in[i] - tmp_char) << (4*(count-1-i))) | *out);
	}
	return 0;
}

/*
static void convert_hex_to_char(unsigned int in, unsigned char* out)
{
	unsigned int tmp_in;
	int len = 4; 
	int i;

	for(i=0;i<len;i++)
	{
		tmp_in = in & (0x000f << i*4);

		if(tmp_in >= 10 ){
			out[i] = 'a' + (tmp_in - 10);
		}
		else{
			out[i] = '0' + tmp_in;
		}
	}
}
*/

/*creating dev_addr attr*/
static ssize_t store_reg_addr(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp_addr;

	struct i2c_private_data_struct *prv_data = (struct i2c_private_data_struct *) dev->driver_data;

	dev_info(dev, " buf given by usr: %s\n",buf);

	if((buf[0]=='0') && (buf[1]=='x' || buf[1]=='X'))
	{
		if(convert_char_to_hex_reg((char*)(buf+2),4,&tmp_addr) > -1)
		{
			dev_info(dev,"convert_char_to_hex_reg passed\n");
			dev_info(dev,"reg_addr: %d or 0x%04X\n",tmp_addr, (uint16_t) tmp_addr);
			if(tmp_addr < (TOTAL_MEM_SIZE)){
				prv_data->reg_addr = (uint16_t)tmp_addr;
			}
			else{
				dev_err(dev,"reg addr exceeds total accessible addr\n");
			}
		}else{
			dev_err(dev,"convert_char_to_hex_reg failed\n");
			return -EINVAL;
		}

	}else{
		dev_err(dev,"please check usage_attr for how to give reg address\n");
	}
	
	// ret = kstrtouint(buf,0,&prv_data->reg_addr);
	// if(!ret){
	// 	dev_err(dev,"invalid address\n");
	// }

	return count;
}

static ssize_t show_reg_addr(struct device *dev,struct device_attribute *attr, char *buf)
{
	struct i2c_private_data_struct *prv_data = (struct i2c_private_data_struct *) dev->driver_data;

	return sprintf(buf,"0x%04x\n",prv_data->reg_addr);
}

static DEVICE_ATTR(reg_addr_attr, S_IRUGO | S_IWUSR, show_reg_addr, store_reg_addr);

/*creating read_byte attr*/

static ssize_t show_read_byte(struct device *dev,struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t data;
	struct i2c_private_data_struct *prv_data = (struct i2c_private_data_struct *) dev->driver_data;

	ret = at24c256_i2c_byte_read(prv_data->client, prv_data->reg_addr, &data);
	if(ret==-1)
	{
		dev_err(dev," read_byte error %d\n",ret);
		return ret;
	}

	/*increment the reg_addr taking into account usage*/
	prv_data->reg_addr = ((prv_data->reg_addr + 1) & (0x7FFF)) ;

	return sprintf(buf,"0x%02x\n", data);
}

static DEVICE_ATTR(read_byte_attr, S_IRUGO, show_read_byte, NULL);

/*creating write_byte attr*/
static ssize_t store_write_byte(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret,data;
	struct i2c_private_data_struct *prv_data = (struct i2c_private_data_struct *) dev->driver_data;

	dev_info(dev, " buf given by usr: %s\n",buf);

	// ret = kstrtoint(buf,0,&data);
	// if(!ret){
	// 	dev_err(dev,"invalid format\n");
	// 	return ret;
	// }

	if((buf[0]=='0') && (buf[1]=='x' || buf[1]=='X'))
	{
		if(convert_char_to_hex_reg((char*)(buf+2),2,&data) > -1)
		{
			dev_info(&prv_data->client->dev,"byte to write: %d : %02X\n",data, (uint8_t) data);
			ret = at24c256_i2c_byte_write(prv_data->client, prv_data->reg_addr, (uint8_t) data);

			if(ret==-1)
			{
				dev_err(dev," write_byte error %d\n",ret);
				return ret;
			}
		}else{
			dev_err(&prv_data->client->dev,"convert_char_to_hex_reg failed\n");
			return -EINVAL;
		}

	}else{
		dev_err(&prv_data->client->dev,"please check usage_attr for how to give write byte\n");
	}

	/*increment the reg_addr taking into account usage*/
	prv_data->reg_addr = ((prv_data->reg_addr + 1) & (0x7FFF)) ;

	return count;	
}

static DEVICE_ATTR(write_byte_attr, S_IWUSR, NULL, store_write_byte);

static int at24c256_sysfs_register(struct device *dev)
{
	int ret;

	ret = device_create_file(dev, &dev_attr_usage_attr);
	if (ret){
		dev_err(dev, "failed to create usage_attr %d\n", ret);
	}

	ret = device_create_file(dev, &dev_attr_reg_addr_attr);
	if (ret){
		dev_err(dev, "failed to create reg_addr_attr %d\n", ret);
	}

	ret = device_create_file(dev, &dev_attr_read_byte_attr);
	if (ret){
		dev_err(dev, "failed to create read_byte_attr %d\n", ret);
	}

	ret = device_create_file(dev, &dev_attr_write_byte_attr);
	if (ret){
		dev_err(dev, "failed to create write_byte_attr %d\n", ret);
	}

	return ret;
}

static void at24c256_sysfs_unregister(struct device *dev)
{
	device_remove_file(dev, &dev_attr_usage_attr);
	device_remove_file(dev, &dev_attr_reg_addr_attr);
	device_remove_file(dev, &dev_attr_read_byte_attr);	
	device_remove_file(dev, &dev_attr_write_byte_attr);	
}

static int at24c256_i2c_byte_read(struct i2c_client* client, uint16_t addr, uint8_t* data)
{
	struct i2c_msg buff[2];
	uint8_t reg[2];
	reg[0] = (uint8_t)((addr & 0x7F00) >> 8); //MSB ignore the highest bit
	reg[1] = (uint8_t)(addr & 0x00FF); //LSB

	//read
	//first write the addr
	buff[0].addr = client->addr;
    buff[0].flags = 0; //write
    buff[0].len = 2;
    buff[0].buf = reg;
	//second get the data
	buff[1].addr = client->addr;
    buff[1].flags = I2C_M_RD; //read
    buff[1].len = 1;
    buff[1].buf = data;

    if(i2c_transfer(client->adapter,buff,2) < 0)
    {
    	pr_err("failed to read the data\n");
    	return -1;
    }else{
    	pr_info("addr: 0x%02X%02X data: %02X\n", reg[0],reg[1],*data);
    }
    return 0;
}

static int at24c256_i2c_byte_write(struct i2c_client* client, uint16_t addr, uint8_t data)
{
	struct i2c_msg buff[1];
	uint8_t reg[3];
	reg[0] = (uint8_t)((addr & 0x7F00) >> 8); //MSB ignore the highest bit
	reg[1] = (uint8_t)(addr & 0x00FF); //LSB
	reg[2] = data;

	buff[0].addr = client->addr;
    buff[0].flags = 0; //write
    buff[0].len = 3;
    buff[0].buf = (uint8_t*)reg;

	if(i2c_transfer(client->adapter,buff,1) < 0)
    {
    	dev_err( &client->dev,"failed to read the data\n");
    	return -1;
    }else{
    	dev_info( &client->dev,"data write successful at addr : 0x%02X%02X wrote data: %02X\n", reg[0],reg[1],reg[2]);
    	return 0;
    }

}

static void dummy_read_write(struct i2c_client* client)
{
	struct i2c_msg buff[2];
	uint8_t reg[3];
	uint8_t data, data_old;

	reg[0] = 0x00; reg[1] = 0x11; reg[2]= 0xF9;

	//read
	//first write the addr
	buff[0].addr = client->addr;
    buff[0].flags = 0; //write
    buff[0].len = 2;
    buff[0].buf = reg;
	//second get the data
	buff[1].addr = client->addr;
    buff[1].flags = I2C_M_RD; //read
    buff[1].len = 1;
    buff[1].buf = &data;

    if(i2c_transfer(client->adapter,buff,2) < 0)
    {
    	pr_err("failed to read the data\n");
    }else{
    	pr_info("addr: 0x%02X%02X data_old: %02X\n", reg[0],reg[1],data);
    }

    data_old = data;

    //write
    buff[0].addr = client->addr;
    buff[0].flags = 0; //write
    buff[0].len = 3;
    buff[0].buf = reg;

    if(i2c_transfer(client->adapter,buff,1) < 0)
    {
    	pr_err("failed to read the data\n");
    }else{
    	pr_info("data write successful at addr : 0x%02X%02X wrote data: %02X\n", reg[0],reg[1],reg[2]);
    }

    //read
	//first write the addr
	buff[0].addr = client->addr;
    buff[0].flags = 0; //write
    buff[0].len = 2;
    buff[0].buf = reg;
	//second get the data
	buff[1].addr = client->addr;
    buff[1].flags = I2C_M_RD; //read
    buff[1].len = 1;
    buff[1].buf = &data;

    if(i2c_transfer(client->adapter,buff,2) < 0)
    {
    	pr_err("failed to read the data\n");
    }else{
    	pr_info("addr: 0x%02X%02X data: %02X\n", reg[0],reg[1],data);
    }

    reg[2] = data_old;
    //write
    buff[0].addr = client->addr;
    buff[0].flags = 0; //write
    buff[0].len = 3;
    buff[0].buf = reg;

    if(i2c_transfer(client->adapter,buff,1) < 0)
    {
    	pr_err("failed to read the data\n");
    }else{
    	pr_info("restoring old data at addr : 0x%02X%02X wrote data: %02X\n", reg[0],reg[1],reg[2]);
    }
}

/*Defining static functions*/
static int at24c256_i2c_driver_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
	/*pre initialization*/
	int ret;
	struct i2c_private_data_struct *prv_data;

	pr_info("Device named : %s is probed\n",client->name);

	/*check whether i2c bus controller of the soc supports the functionality needed by your device*/
	/*I2C_FUNC_SMBUS_BYTE_DATA : I2C functionality to read write byte data*/
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -EIO;

    /*Register the device so that user can access it*/
    /*allocate the memory for prv struct linked to dev struct*/
    prv_data = devm_kmalloc(&client->dev,sizeof(*prv_data),GFP_KERNEL);
    if(prv_data == NULL){
    	pr_err("failed to allocate memory for private data struct");
    	return -ENOMEM;
    }

	/*test the communication, if fails then return here only*/
	/*read*/
	dummy_read_write(client);

	/*store prv_data in driver_data pointer*/
	prv_data->client = client;
	i2c_set_clientdata(client, prv_data);

	/*create sysfs files*/
	ret = at24c256_sysfs_register(&client->dev);
	if (ret){
		dev_err(&client->dev,"unable to create sysfs entries for at24c256 eeprom\n");
		goto out;
	}
    
	return 0;

	
	out:
		devm_kfree(&client->dev,prv_data); //not needed but still for completness
		pr_info("Module insertion failed\n");
		return ret;
}

static int at24c256_i2c_driver_remove(struct i2c_client* client)
{	
	/*remove sysfs entries*/
	at24c256_sysfs_unregister(&client->dev);
	pr_info("Device named : %s is removed\n",client->name);
	return 0;
}

/*Defining the at24c256_i2c_device_ids*/
static const struct i2c_device_id at24c256_i2c_device_ids[] = {
	{.name = "at24c256-i2c-device", .driver_data = 0},
	{ } /*NULL Termination*/
};

MODULE_DEVICE_TABLE(i2c,at24c256_i2c_device_ids);

/*Defining of_at24c256_i2c_device_ids table */
static const struct of_device_id of_at24c256_i2c_device_ids[] = {
	{.compatible = "at24c256,i2c" , .data = NULL},
	{ }/*Null Termination*/
};

MODULE_DEVICE_TABLE(of,of_at24c256_i2c_device_ids);

/*Defining at24c256 i2c driver structure*/
static struct i2c_driver at24c256_i2c_drv = {
	.driver = {
		.name = "at24c256_i2c_drv",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(of_at24c256_i2c_device_ids),
	},
	.id_table = at24c256_i2c_device_ids,
	.probe = at24c256_i2c_driver_probe,
	.remove = at24c256_i2c_driver_remove,
};


/*register i2c_driver module using Macro*/
module_i2c_driver(at24c256_i2c_drv);

MODULE_DESCRIPTION("at24c256_i2c_drv example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");
