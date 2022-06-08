APP_NAME = test_nfc
TOOLCHAIN = arm-linux-gnueabihf-
CC = $(TOOLCHAIN)gcc
obj-m	:= $(MODULE_NAME).o
all: app
app:
	$(CC) -o $(APP_NAME) $(APP_NAME).c -lnfc
clean:
	rm -f *.ko
	rm -f *.o
	rm -f $(APP_NAME)
