OBJ_NAME=virt_gpio
TARGET=$(OBJ_NAME).ko
obj-m := $(OBJ_NAME).o
PWD := $(shell pwd)


KERNEL_SOURCE=/lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) modules
install:
	$(HOST_INSTALL) $(TARGET) $(IMAGES_DIR)
clean:
	rm -rf .*.cmd *.o *.mod.c *.ko .tmp_versions *.order *symvers *Module.markers
.PHONY: all clean install
