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
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/gpio/driver.h>


typedef struct virt_gpio_plat_data_s
{
	int base_gpio;
	int gpio_num;
}virt_gpio_plat_data_t;


typedef struct virt_gpio_chip_s
{
	struct gpio_chip chip;
	spinlock_t reg_lock;
	uint64_t gpio_dir;
	uint64_t gpio_in;
	uint64_t gpio_out;
	uint64_t gpio_irq;
	/*此处不需要该变量，因为我们是模拟的*/
//	void __iomem             *reg_base;
}virt_gpio_chip_t;
