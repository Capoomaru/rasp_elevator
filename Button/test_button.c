#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>
#include <linux/poll.h>
#include <signal.h>
#define BUTTON_FILE_NAME "/dev/button_driver"
int main()
{
	int button_fd;
	char led_onoff = 0;
	char button;
	struct pollfd events[2];
	int retval;
	button_fd = open(BUTTON_FILE_NAME, O_RDWR | O_NONBLOCK);
	if (button_fd < 0)
	{
		fprintf(stderr, "Can't open %s\n", BUTTON_FILE_NAME);
		return -1;
	}
	while (1)
	{
		events[0].fd = button_fd;
		events[0].events = POLLIN; // waiting read
		retval = poll(events, 1, 1000); // event waiting
		if (retval < 0)
		{
			fprintf(stderr, "Poll error\n");
			exit(0);
		}
		if (retval == 0)
		{
			puts("[APP] LED Off!!");
			led_onoff = 1;
			write(button_fd, &led_onoff, 1);
		}
		if (events[0].revents & POLLIN)
		{
			puts("[APP] Wakeup_Poll_Event!!\n");
			read(button_fd, &button, 1);
			printf("[APP] BUTTON: %d\n", button);
			usleep(500);
		}
	}
	led_onoff = 0;
	write(button_fd, &led_onoff, 1);
	close(button_fd);
	return 0;
}
