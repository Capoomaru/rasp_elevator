#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>
#define BUZZER_FILE_NAME "/dev/buzzer_driver"

int buzzer_fd;
void buzzer_on() {
	char number = 1;
	write(buzzer_fd, &number, sizeof(char));
	usleep(1000);
	number = 0;
	write(buzzer_fd, &number, sizeof(char));
}


int main(int argc, char **argv)
{
	char data;
	int number;
	buzzer_fd = open(BUZZER_FILE_NAME, O_RDWR);
	if(buzzer_fd < 0)
	{
		fprintf(stderr, "Can't open %s\n", BUZZER_FILE_NAME);
		return -1;
	}
	while (1)
	{
		write(buzzer_fd, &data, sizeof(char));
		printf("[app] Input number : ");
		scanf("%d", &number);
		if (number < 0 || number > 1)
			fprintf(stderr, "plz input 0 or 1 : your input : %s\n", number);
		else
			buzzer_on();
	}
	close(buzzer_fd);
	return 0;
}
