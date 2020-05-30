	在上面一章，我们介绍了pwm子系统的框架以及数据结构等内容，而pwm 子系统中pwm_chip的注
册与注销接口也就是实现pwm_chip与pwm_device之间的关联，并将pwm_chip放入系统pwm_chip链表中，而这些接口实现也较简单，就没有展开说明。本章我们主要介绍如何实现一个pwm驱动，本章的目的如下：
	1. 实现一个基于gpio的pwm chip驱动（即通过gpio模拟pwm）；
	2. 为了让大家都可以验证该驱动，我们实现了一个虚拟的gpio controller驱动（在之前的linux gpio子系统专栏中已经说明如何实现一个虚拟的gpio controller），并借助sysfs文件系统提供的接口，为该虚拟gpio 控制器的输出值创建一个名为vgpio_out文件，我们可以通过查看该文件的值，确定是否输入高低电平；
	3. 本次实现的驱动均在ubuntu16.04系统下验证通过；
	4. 本次代码已放入gitee上（链接地址）

gpio-pwm功能设计说明
	1. 该驱动可根据传递的gpio参数个数n，创建一个pwm_chip，并为该pwm_chip创建n个pwm device；
	2. 本驱动使用高精度定时器hrtimer（精度为ns），实现周期控制gpio值，从而输出pwm信号。

gpio-pwm驱动涉及的知识点
	1. Platform device、platform driver；
	2. Pwm 子系统中pwm_chip添加接口
	
gpio-pwm数据结构说明
主要定义了pwm_gpio_chip、pwm_gpio_device_data两个数据结构，其中:
	1. struct pwm_gpio_device_data表示一个gpio对应的pwm device，该数据结构中包含一个高精度定时器、gpio号、当前设置的gpio值、对应的pwm_device、定时器是否启用标志is_actived;
	2. Struct pwm_gpio_chip对应gpio pwm控制器，包含该gpio pwm控制器所支持的pwm个数，每一个pwm对应的struct pwm_gpio_device_data类型的变量。


Gpio-pwm接口实现说明
定时器超时处理函数
     该接口中实现对gpio的设置，并进行下一次定时器超时时间的设置，根据pwm_device的period、duty_cycle的值驱动定时器的超时事件；
Pwm_ops相关接口的设置
request接口实现gpio的request操作；
free接口实现gpio的free操作，同时若定时器未取消，还需要完成定时器的取消操作
enable接口实现高精度定时器的设置，并启动定时器；
disable接口实现高精度定时器的取消操作；
config接口实现pwm的period、duty_cycle值的设置操作。

Platform driver
Platform driver的probe接口中实现pwm个数以及gpio值的获取，并调用pwmchip_add完成pwm chip的注册操作；
Platform driver的remove接口完成pwm chip的注销等操作。

编译步骤：
在代码的顶级目录执行make&&make install，则在images下即为构建完成的文件。

测试验证
	1. 完成virt gpio controller相关的platform device、platform driver相关驱动的insmod，完成该操作后，则在/sys/class/gpio中即可看到该虚拟gpio控制器；
	insmod virt_gpio_platform/virt_gpio_dev.ko
	insmod virt_gpio_chip/virt_gpio.ko
	2. 完成pwm-gpio对应的platform device、platform driver相关驱动的insmod，完成操作后，则在/sys/class/pwm中即可看到我们创建的pwmchip
	insmod pwm_gpio_platform/pwm_gpio_platform.ko
	insmod pwm_gpio/pwm_gpio.ko
	3. 执行如下命令，即可开启pwm-gpio
	
	4. 开启pwm-gpio后，我们可以查看虚拟gpio控制器的gpio out文件，确认是否生效，该文件的路径如下：/sys/devices/platform/virt_gpio_dev/vgpio_out
	5. 关闭pwm-gpio,只需执行echo 0 >/sys/class/pwm/pwmchip0/pwm1/enable即可。

