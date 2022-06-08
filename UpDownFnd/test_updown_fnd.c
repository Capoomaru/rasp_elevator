#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>
#define FND_FILE_NAME "/dev/updown_fnd_driver"
int main(int argc, char **argv)
{
	int fnd_fd;
	char data;
	int number;
	char fnd_values[3] = { 0xC4, 0x38, 0x00 }; // up, down, Off
	fnd_fd = open(FND_FILE_NAME, O_RDWR);
	if (fnd_fd < 0)
	{
		fprintf(stderr, "Can't open %s\n", FND_FILE_NAME);
		return -1;
	}
	while (1)
	{
		printf("[app] Input number : ");
		scanf("%d", &number);
		if (number < 0 || number > 2)
			fprintf(stderr, "Invalid value. Try again(0~12).\n");
		else
			write(fnd_fd, (char*)(&number), sizeof(char));
	}
	close(fnd_fd);
	return 0;
}
