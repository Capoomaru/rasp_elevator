#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>

#define DC_FILE_NAME "/dev/dc_driver"

int main(int argc, char **argv)
{
	int dc_fd;
	char data;
	int n = 1;

	dc_fd = open(DC_FILE_NAME, O_RDWR);
	if (dc_fd < 0)
	{
		fprintf(stderr, "Can't open %s\n", DC_FILE_NAME);
		return -1;
	}
	while (1)
	{

		printf("[app] Input number : ");
		scanf("%d", &number);
		if (number < 1 || number > 3)
		{
		}
		else
		{
			switch (number)
			{
			case -1:
				data = '-1';	//역방향
				write(dc_fd, &data, sizeof(char));
				sleep(5)
					data = '0';
				write(dc_fd, &data, sizeof(char));
				break;
			case 0:
				data = '0';		//정지
				write(dc_fd, &data, sizeof(char));
				sleep(3);
				break;
			case 1:
				data = '1';		//정방향
				write(dc_fd, &data, sizeof(char));
				sleep(5)
					data = '0';
				write(dc_fd, &data, sizeof(char));
				break;
			default:
				fprintf(stderr, "Invalid value. Try again(-1/0/1).\n");
				break;
			}
		}

		close(dc_fd);
		return 0;
	}
