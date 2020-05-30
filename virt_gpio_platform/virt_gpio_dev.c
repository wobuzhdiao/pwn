#include "virt_gpio_dev.h"


static void virtual_gpio_dev_release(struct device *dev)
{

}


static struct virt_gpio_plat_data_s virt_gpio_data =
{
	.base_gpio = 256,
	.gpio_num = 64 
};

static struct platform_device gvirtual_gpio_platform_device = {
	.name = "virt_gpio_dev",
	.id = -1,
	.dev = {
		.platform_data = &virt_gpio_data,
		.release = virtual_gpio_dev_release,
	}
};

static int __init virtual_gpio_platform_init(void)
{
	int ret = 0;

	ret = platform_device_register(&gvirtual_gpio_platform_device);


	return ret;
}
static void __exit virtual_gpio_platform_exit(void)
{
    printk("%s:%d\n", __FUNCTION__, __LINE__);
    platform_device_unregister(&gvirtual_gpio_platform_device);
}
module_init(virtual_gpio_platform_init);
module_exit(virtual_gpio_platform_exit);
MODULE_DESCRIPTION("Virtual GPIO Platform Device");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerry_chg");
