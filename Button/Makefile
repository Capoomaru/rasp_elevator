KDIR = /lib/modules/$(shell uname -r)/build
MODULE_NAME = button_driver
APP_NAME = test_button
TOOLCHAIN = arm-linux-gnueabihf-
CC = $(TOOLCHAIN)gcc
obj-m	:= $(MODULE_NAME).o
all: app driver
driver:
	$(MAKE) -C $(KDIR) M=$$PWD modules
app:
	$(CC) -o $(APP_NAME) $(APP_NAME).c
clean:
	rm -f *.ko
	rm -f *.mod.*
	rm -f *.o
	rm -f $(APP_NAME)
	rm -f modules.order
	rm -f Module.symvers
set:
	mknod /dev/$(MODULE_NAME) c 220 0
	insmod button_driver.ko
rm:
	rm /dev/$(MODULE_NAME)
	rmmod $(MODULE_NAME)

