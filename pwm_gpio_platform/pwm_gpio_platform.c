#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include "../pwm_gpio/pwm_gpio.h"




static unsigned int pwm_gpios[] =
{
	256,
	257,
	259,
	260
};
static struct pwm_gpio_platform_data pwm_gpio_platform_data = 
{
	.ngpios = 4,
	.base = -1,
	.gpios = pwm_gpios
};




static void virt_pwm_gpio_release(struct device *dev)
{

}
static struct platform_device virt_pwm_gpio_device = {
	.name = "pwm-gpio",
	.id = -1,
	.dev = {
		.platform_data = &pwm_gpio_platform_data,
		.release = virt_pwm_gpio_release,
	}
};



static int __init virt_pwm_gpio_dev_init(void)
{
	int ret = 0;
	int i = 0;
	struct pwm_gpio_platform_data *datap = virt_pwm_gpio_device.dev.platform_data;

	for(i = 0; i < 4; i++)
	{
		printk("%d\n", datap->gpios[i]);
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	ret = platform_device_register(&virt_pwm_gpio_device);


	return ret;
}

static void __exit virt_pwm_gpio_dev_exit(void)
{
    printk("%s:%d\n", __FUNCTION__, __LINE__);
    platform_device_unregister(&virt_pwm_gpio_device);
}



module_init(virt_pwm_gpio_dev_init);
module_exit(virt_pwm_gpio_dev_exit);
MODULE_DESCRIPTION("virt pwm gpio platform device");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerry_chg");
