#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/reboot.h>
#include <linux/pwm.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include "pwm_gpio.h"


struct pwm_gpio_device_data
{
	unsigned int gpio;
	u8 gpio_value;
	struct hrtimer timer;
	struct pwm_device *pwm_dev;
	bool is_actived;
	struct mutex lock;
	
};

struct pwm_gpio_chip
{
	struct pwm_chip chip;
	int ngpios;
	struct pwm_gpio_device_data pwm_dev_data[];
};


static enum hrtimer_restart pwm_gpio_function(struct hrtimer *data)
{
	int delay_on = 0;
	int delay_off = 0;
	int ns = 0;
	
	struct pwm_gpio_device_data *pwm_data = container_of(data, struct pwm_gpio_device_data, timer);
	struct pwm_device *pwm_dev = pwm_data->pwm_dev;

	delay_on = pwm_dev->duty_cycle;
	delay_off = pwm_dev->period - pwm_dev->duty_cycle;
	if(pwm_data->gpio_value>0)
	{
		ns = delay_off;
	}
	else
	{
		ns = delay_on;
	}
	
	pwm_data->gpio_value = (pwm_data->gpio_value > 0)?0:1;
	gpio_set_value(pwm_data->gpio, pwm_data->gpio_value);

	hrtimer_forward_now(&pwm_data->timer, ns_to_ktime(ns));

	return HRTIMER_RESTART;
}


static int pwm_gpio_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	int ret = 0;
	/*in pwmchip_sysfs_export, does not call pwm_get/pwm_request, so move gpio_request to pwm_enable*/
	struct pwm_gpio_chip *pwm_chip_ptr = container_of(chip, struct pwm_gpio_chip, chip);
	struct pwm_gpio_device_data *dev_data = &(pwm_chip_ptr->pwm_dev_data[pwm->hwpwm]);
	printk("%s:%d\n", __FUNCTION__, __LINE__);

	gpio_free(dev_data->gpio);
	ret = gpio_request_one(dev_data->gpio, GPIOF_DIR_OUT, pwm->label);

	return ret;
}

static void pwm_gpio_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	/*in pwmchip_sysfs_export, does not call pwm_get/pwm_request, so move pwm_free to pwm_disable*/
	struct pwm_gpio_chip *pwm_chip_ptr = container_of(chip, struct pwm_gpio_chip, chip);
	struct pwm_gpio_device_data * dev_data = &(pwm_chip_ptr->pwm_dev_data[pwm->hwpwm]);
	printk("%s:%d\n", __FUNCTION__, __LINE__);

	mutex_lock(&dev_data->lock);
	if(dev_data->is_actived)
	{
		printk("%s:%d\n", __FUNCTION__, __LINE__);
		hrtimer_cancel(&dev_data->timer);
		dev_data->is_actived = false;
	}
	mutex_unlock(&dev_data->lock);
	gpio_free(dev_data->gpio);

}



static int pwm_gpio_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct pwm_gpio_chip *pwm_chip_ptr = container_of(chip, struct pwm_gpio_chip, chip);
	struct pwm_gpio_device_data * dev_data = &(pwm_chip_ptr->pwm_dev_data[pwm->hwpwm]);

	mutex_lock(&dev_data->lock);
	if(!dev_data->is_actived)
	{
		hrtimer_init(&dev_data->timer, CLOCK_REALTIME, HRTIMER_MODE_ABS);
		dev_data->timer.function = pwm_gpio_function;
		dev_data->is_actived = true;
	}	
	mutex_unlock(&dev_data->lock);
	hrtimer_start(&dev_data->timer, ktime_add_ns(ktime_get(), pwm->duty_cycle), HRTIMER_MODE_ABS);
	return 0;
}


static int pwm_gpio_config(struct pwm_chip *chip, struct pwm_device *pwm,
	int duty_ns, int period_ns)
{

	pwm->period = period_ns;
	pwm->duty_cycle = duty_ns;
	return 0;
}
static void pwm_gpio_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct pwm_gpio_chip *pwm_chip_ptr = container_of(chip, struct pwm_gpio_chip, chip);
	struct pwm_gpio_device_data * dev_data = &(pwm_chip_ptr->pwm_dev_data[pwm->hwpwm]);

	mutex_lock(&dev_data->lock);
	if(dev_data->is_actived)
	{
		hrtimer_cancel(&dev_data->timer);
		dev_data->is_actived = false;
	}
	mutex_unlock(&dev_data->lock);
}

static struct pwm_ops pwm_gpio_ops = {
	.request = pwm_gpio_request,
	.free = pwm_gpio_free,
	.enable = pwm_gpio_enable,
	.disable = pwm_gpio_disable,
	.config = pwm_gpio_config,
	.owner = THIS_MODULE,
	
};




static int pwm_gpio_probe(struct platform_device *pdev)
{
	struct pwm_gpio_platform_data *pdata = (struct pwm_gpio_platform_data *)(pdev->dev.platform_data);
	struct pwm_gpio_chip * pwm_chip_ptr = NULL;
	int i = 0;
	int ret = 0;

	if(pdata->ngpios <= 0 || pdata->gpios == NULL)
		return -EINVAL;

	pwm_chip_ptr = devm_kzalloc(&pdev->dev, 
		sizeof(struct pwm_gpio_chip) + (pdata->ngpios*sizeof(struct pwm_gpio_device_data)) , GFP_KERNEL);

	if(pwm_chip_ptr == NULL)
		return -ENOMEM;

	pwm_chip_ptr->ngpios = pdata->ngpios;
	for(i = 0; i < pdata->ngpios; i++)
	{
		pwm_chip_ptr->pwm_dev_data[i].gpio = pdata->gpios[i];
		mutex_init(&(pwm_chip_ptr->pwm_dev_data[i].lock));
		pwm_chip_ptr->pwm_dev_data[i].is_actived = false;
	}


	pwm_chip_ptr->chip.base = pdata->base;
	pwm_chip_ptr->chip.npwm = pdata->ngpios;
	pwm_chip_ptr->chip.ops = &pwm_gpio_ops;
	pwm_chip_ptr->chip.dev = &pdev->dev;
	#ifdef CONFIG_OF 
	pwm_chip_ptr->chip.dev->of_node = pdev->dev.of_node;
	#endif
	
	ret = pwmchip_add(&pwm_chip_ptr->chip);
	if(ret)
	{
		return ret;
	}
	
	for(i = 0; i < pdata->ngpios; i++)
	{
		pwm_chip_ptr->pwm_dev_data[i].pwm_dev = &pwm_chip_ptr->chip.pwms[i];
	}

	platform_set_drvdata(pdev, pwm_chip_ptr);

	return 0;
}

static int pwm_gpio_remove(struct platform_device *pdev)
{
	struct pwm_gpio_chip *pwm_chip_ptr = platform_get_drvdata(pdev);

	pwmchip_remove(&pwm_chip_ptr->chip);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id of_pwm_gpio_match[] = {
	{ .compatible = "pwm-gpio", },
	{},
};
#endif
static struct platform_driver pwm_gpio_driver = {
	.probe		= pwm_gpio_probe,
	.remove		= pwm_gpio_remove,
	.driver		= {
		.name	= "pwm-gpio",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(of_pwm_gpio_match),
	},
};

module_platform_driver(pwm_gpio_driver);

MODULE_AUTHOR("jerry_chg");
MODULE_DESCRIPTION("PWM gpio chip driver");
MODULE_LICENSE("GPL");
