KDIR = /lib/modules/$(shell uname -r)/build
MODULE_NAME = updown_fnd_driver
APP_NAME = test_updown_fnd
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
	mknod /dev/updown_fnd_driver c 223 0
	insmod updown_fnd_driver.ko
rm:
	rm /dev/updown_fnd_driver
	rmmod updown_fnd_driver
