BUILD_DIRS = pwm_gpio pwm_gpio_platform virt_gpio_chip  virt_gpio_platform 
HOST_MKDIR=mkdir -p
HOST_INSTALL=install

IMAGES_DIR=$(shell pwd)/images
export IMAGES_DIR HOST_MKDIR HOST_INSTALL

all:
	@$(foreach dir, $(BUILD_DIRS),  $(MAKE) -C $(dir) || exit 1;)

install:
	$(if $(wildcard $(IMAGES_DIR)), , $(HOST_MKDIR) $(IMAGES_DIR))
	@$(foreach dir, $(BUILD_DIRS),  $(MAKE) -C $(dir) install || exit 1;)

clean:
	@$(foreach dir, $(BUILD_DIRS),  $(MAKE) -C $(dir) clean || exit 1;)
	rm -rf $(IMAGES_DIR)

.PHONY:all install clean
