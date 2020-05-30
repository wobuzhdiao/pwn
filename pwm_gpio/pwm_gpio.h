#ifndef PWM_GPIO_H_
#define PWM_GPIO_H_
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

struct pwm_gpio_platform_data
{
	int ngpios;
	int base;
	unsigned int *gpios;
};



#endif
