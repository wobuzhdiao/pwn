#include "virt_gpio.h"

/*
typedef struct virt_gpio_chip_s
{
	uint64_t gpio_dir;
	uint64_t gpio_in;
	uint64_t gpio_out;
	uint64_t gpio_irq;
//	void __iomem             *reg_base;
}virt_gpio_chip_t;

gpio_dir对应方向寄存器,1表示输入；0表示输出
gpio_in对应输入方向gpio的值读取寄存器（只读）
gpio_out对应输出方向gpio的设置与读取寄存器；
gpio_irq为gpio中断配置寄存器，1表示中断模式；0表示gpio模式

*/
typedef enum 
{
	VIRT_DIR_IN,
	VIRT_DIR_OUT
}virt_gpio_dir_e;


static ssize_t vgpio_dir_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct virt_gpio_chip_s *virt_chip_ptr = dev_get_drvdata(dev);


    return  sprintf(buf, "%lld\n", virt_chip_ptr->gpio_dir);
}
static ssize_t vgpio_in_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct virt_gpio_chip_s *virt_chip_ptr = dev_get_drvdata(dev);


	return  sprintf(buf, "%lld\n", virt_chip_ptr->gpio_in);
}

static ssize_t vgpio_in_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct virt_gpio_chip_s *virt_chip_ptr = dev_get_drvdata(dev);
	u64 gpio_value = simple_strtoul(buf, NULL, 10);

	
	printk("%s:%d need set value=%lld\n", __FUNCTION__, __LINE__, gpio_value);
	virt_chip_ptr->gpio_in = gpio_value;

	return count;
}

static ssize_t vgpio_out_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct virt_gpio_chip_s *virt_chip_ptr = dev_get_drvdata(dev);


	return  sprintf(buf, "%lld\n", virt_chip_ptr->gpio_out);
}

static DEVICE_ATTR(vgpio_dir, S_IRUSR, vgpio_dir_show, NULL);
static DEVICE_ATTR(vgpio_in, S_IWUSR|S_IRUSR, vgpio_in_show, vgpio_in_store);
static DEVICE_ATTR(vgpio_out, S_IRUSR, vgpio_out_show, NULL);


static struct attribute *virt_gpio_attrs[] =
{
	&dev_attr_vgpio_dir.attr,
	&dev_attr_vgpio_in.attr,
	&dev_attr_vgpio_out.attr,
	NULL
};
static const struct attribute_group virt_gpio_attr_group = 
{
	.attrs = virt_gpio_attrs,
};



static int virt_gpio_dir_set(struct virt_gpio_chip_s *virt_chip_ptr, int bit_ops, virt_gpio_dir_e dir)
{
	unsigned long flags;

	spin_lock_irqsave(&virt_chip_ptr->reg_lock, flags);

	if(dir == VIRT_DIR_IN)
		virt_chip_ptr->gpio_dir |= (1UL<<bit_ops);
	else
		virt_chip_ptr->gpio_dir &= ~(1UL<<bit_ops);
	spin_unlock_irqrestore(&virt_chip_ptr->reg_lock, flags);

	return 0;
}
static int virt_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct virt_gpio_chip_s *virt_chip_ptr = container_of(chip, struct virt_gpio_chip_s, chip);

	return virt_gpio_dir_set(virt_chip_ptr, offset, VIRT_DIR_IN);
}

static int virt_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	struct virt_gpio_chip_s *virt_chip_ptr = container_of(chip, struct virt_gpio_chip_s, chip);

	return virt_gpio_dir_set(virt_chip_ptr, offset, VIRT_DIR_OUT);
}

static int virt_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct virt_gpio_chip_s *virt_chip_ptr = container_of(chip, struct virt_gpio_chip_s, chip);
	u64 gpio_dir_value = 0;
	u64 gpio_value = 0;
	gpio_dir_value = virt_chip_ptr->gpio_dir;

	if(gpio_dir_value&(1UL<<offset))
	{
		gpio_value = virt_chip_ptr->gpio_in;
		return !!(gpio_value&(1UL<<offset));
	}
	else
	{
		gpio_value = virt_chip_ptr->gpio_out;
		return !!(gpio_value&(1UL<<offset));
	}
}


static void virt_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct virt_gpio_chip_s *virt_chip_ptr = container_of(chip, struct virt_gpio_chip_s, chip);
	unsigned long flags;
	u64 gpio_dir_value = 0;


	gpio_dir_value = virt_chip_ptr->gpio_dir;
	if(gpio_dir_value&(1UL<<offset))
		return;

	spin_lock_irqsave(&virt_chip_ptr->reg_lock, flags);
	if(value)
		virt_chip_ptr->gpio_out |= (1UL<<offset);
	else
		virt_chip_ptr->gpio_out &= ~(1UL<<offset);
	spin_unlock_irqrestore(&virt_chip_ptr->reg_lock, flags);
}

static int virt_gpio_request(struct gpio_chip *chip, unsigned offset)
{

	printk("%s:%d offset=%d\n", __FUNCTION__, __LINE__, offset);
	if (offset >= (chip->ngpio))
		return -EINVAL;
	else
		return 0;
}

static int virt_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	return -1;
}

static void virt_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	printk("%s:%d\n", __FUNCTION__, __LINE__);
}


static int virtual_gpio_platform_probe(struct platform_device *platform_dev)
{
	struct device *dev = &platform_dev->dev;
	struct virt_gpio_plat_data_s *pdata = dev->platform_data;
	struct virt_gpio_chip_s *virt_chip_ptr = NULL;
	int ret = 0;
	
	virt_chip_ptr = devm_kzalloc(dev, sizeof(struct virt_gpio_chip_s), GFP_KERNEL);
	if (!virt_chip_ptr)
		return -ENOMEM;
	
	platform_set_drvdata(platform_dev, virt_chip_ptr);

	virt_chip_ptr->chip.direction_input = virt_gpio_direction_input;
	virt_chip_ptr->chip.get = virt_gpio_get;
	virt_chip_ptr->chip.direction_output = virt_gpio_direction_output;
	virt_chip_ptr->chip.set = virt_gpio_set;
	virt_chip_ptr->chip.owner = THIS_MODULE;
	virt_chip_ptr->chip.request = virt_gpio_request;
	virt_chip_ptr->chip.free = virt_gpio_free;
	virt_chip_ptr->chip.ngpio  = pdata->gpio_num;
	virt_chip_ptr->chip.base = pdata->base_gpio;
	virt_chip_ptr->chip.label = dev_name(&platform_dev->dev);
	virt_chip_ptr->chip.dev = &platform_dev->dev;
	virt_chip_ptr->chip.owner = THIS_MODULE;
	spin_lock_init(&virt_chip_ptr->reg_lock);

	
	ret = gpiochip_add(&virt_chip_ptr->chip);
	if (ret) 
	{
		printk("%s:%d\n", __FUNCTION__, __LINE__);
		return ret;
	}
	printk("%s:%d\n", __FUNCTION__, __LINE__);

	printk("gpio_base=%d gpio_num=%d\n", pdata->base_gpio, pdata->gpio_num);
	sysfs_create_group(&platform_dev->dev.kobj, &virt_gpio_attr_group);
	
	return 0;
}


static int virtual_gpio_platform_remove(struct platform_device *platform_dev)
{
	struct virt_gpio_chip_s *virt_chip_ptr = platform_get_drvdata(platform_dev);
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	gpiochip_remove(&virt_chip_ptr->chip);
	printk("%s:%d\n", __FUNCTION__, __LINE__);
	sysfs_remove_group(&platform_dev->dev.kobj, &virt_gpio_attr_group);
	return 0;
}

static struct platform_driver virt_gpio_platform_driver = {
    .driver = {
        .name = "virt_gpio_dev",
        .owner = THIS_MODULE,
    },
    .probe = virtual_gpio_platform_probe,
    .remove = virtual_gpio_platform_remove,
};


static int __init virtual_gpio_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&virt_gpio_platform_driver);

	return ret;
}

static void __exit virtual_gpio_exit(void)
{
    printk("%s:%d\n", __FUNCTION__, __LINE__);

    platform_driver_unregister(&virt_gpio_platform_driver);
}



module_init(virtual_gpio_init);
module_exit(virtual_gpio_exit);
MODULE_DESCRIPTION("Virtual GPIO Platform Device Drivers");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerry_chg");

