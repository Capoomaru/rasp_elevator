KDIR = /lib/modules/$(shell uname -r)/build
MODULE_NAME = fnd_driver
APP_NAME = test_fnd
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
	mknod /dev/fnd_driver c 222 0
	insmod fnd_driver.ko
rm:
	rm /dev/fnd_driver
	rmmod fnd_driver
