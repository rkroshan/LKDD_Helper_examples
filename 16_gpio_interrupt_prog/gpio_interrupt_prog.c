#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include<linux/platform_device.h>
#include<linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

struct gpio_desc *gpiod;
struct pinctrl *pinctrld;
int irq;

/*declare timer struct*/
static struct timer_list my_timer;

/*define timer callback*/
static void timer_callback(struct timer_list* timerlist)
{
	// struct gpio_desc *mygpiod = (struct gpio_desc*) (*((void*)timerlist->data));

	pr_info("Timer mygpiod value = %d\n", gpiod_get_value(gpiod));

	mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
}


/*OF match table registration*/
static const struct of_device_id of_pcdevs_ids[] = 
{
	{.compatible = "gpio-interrupt,eg", .data = NULL },
	{ } /*Null termination */
};

MODULE_DEVICE_TABLE(of,of_pcdevs_ids);

/*platform device id table registration*/
struct platform_device_id pcdevs_ids[] = 
{
	[0] = {.name = "gpio-interrupt,eg",.driver_data = 0},
	{ } /*Null termination */
};

MODULE_DEVICE_TABLE(platform,pcdevs_ids);

static irqreturn_t gpinterrupt_eg_handler(int irq, void *dev_id)
{
	pr_info("Interrupt triggered\n");
	return IRQ_HANDLED;
}

/*Defining static functions*/
static int gpio_interrupt_prog_probe(struct platform_device *pdev)
{
	/*pre initialization*/
	int ret;

	pr_info("New device probed with name :%s\n", pdev->name);
	/*set the pinctrl configuration first*/
	pinctrld = devm_pinctrl_get_select_default(&pdev->dev);
	if(IS_ERR(pinctrld))
	{
		pr_err("Failed to configure the pin\n");
		return -ENODEV;
	}
	/*get the gpio*/
	gpiod = gpiod_get(&pdev->dev, "inter", GPIOD_IN);
	if(IS_ERR(gpiod))
	{
		pr_err("Failed to get gpio");
		return -ENODEV;
	}
	/*get the irq*/
	irq = gpiod_to_irq(gpiod);

	/*setup interrup handler*/
	ret = request_any_context_irq(irq,gpinterrupt_eg_handler,(IRQF_TRIGGER_RISING),"interrupt-gpio-eg",NULL);

	if(ret < 0)
	{
		pr_err("Failed to acquire the irq %d\n", ret);
		goto free_gpio;
	}

	pr_info("irq registered successfully\n");

	timer_setup(&my_timer,timer_callback,0);
	my_timer.data = (long)(&gpiod);
	/*reset the timeout*/
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));

	/*timer timeout set (ret = 0) | failed (ret = 1)*/
	if(ret)
		pr_info("Timer timeout init failed\n");

	return 0;

	free_gpio:
		gpiod_put(gpiod);
		return ret;
}

static int gpio_interrupt_prog_remove(struct platform_device *pdev)
{
	
	int ret;
	ret=del_timer(&my_timer);

	/*if timer active (ret=1) else ret=0*/
	if(ret)
		pr_info("timer is still is use ...\n");

	/*free irq*/
	free_irq(irq,NULL);
	/*free gpio*/
	gpiod_put(gpiod);
	pr_info("Device named : %s is removed\n",pdev->name);
	return 0;
}

/*platform driver structure initialization*/
struct platform_driver pltfm_drv_st = 
{
	.probe = gpio_interrupt_prog_probe,
	.remove = gpio_interrupt_prog_remove,
	.id_table = pcdevs_ids,
	.driver = {
		.name = "gpio-interrupt-eg-driver",
		.of_match_table = of_match_ptr(of_pcdevs_ids),
		.owner = THIS_MODULE
	}

};


static int __init gpio_interrupt_prog_init(void)
{
	pr_info("module init\n");
	/*Register a platform driver */
	platform_driver_register(&pltfm_drv_st);
	
	pr_info("platform driver loaded\n");
	return 0;
}

static void __exit gpio_interrupt_prog_exit(void)
{
	/*Unregister the platform driver */
	platform_driver_unregister(&pltfm_drv_st);
	pr_info("platform driver unloaded\n");
	pr_info("module unloaded\n");
	pr_info("bye bye world\n");
}

module_init(gpio_interrupt_prog_init);
module_exit(gpio_interrupt_prog_exit);
MODULE_DESCRIPTION("gpio_interrupt_prog example code");
MODULE_AUTHOR("Roshan Kumar <rkroshan.1999@gmail.com>");
MODULE_INFO(board,"Beaglebone black rev3");
MODULE_LICENSE("GPL");