#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>

#define SERVO_FILE_NAME    "/dev/servo_driver"

void write_pwm(int, int, int);

int main(int argc, char **argv) {
    int servo_fd;
    char data;
    int i;

    servo_fd = open(SERVO_FILE_NAME, O_RDWR);
    if (servo_fd < 0) {
        fprintf(stderr, "Can't open %s\n", SERVO_FILE_NAME);
        return -1;
    }

    int t;
    for (t = 1;t<100;t++) {
            write_pwm(servo_fd, 1240, 20000);
    }

    sleep(2);

    for (t = 1;t<100;t++) {
            write_pwm(servo_fd, 1240, 20000);
    }

    sleep(2);
    for (i = 0; i < 90; i++) { // 시계 반대 방향
    	write_pwm(servo_fd, 1420, 20000);
    }
    sleep(2);
    for (t = 1;t<90;t++) {
        write_pwm(servo_fd, 1420, 20000);
    }

    close(servo_fd);
    return 0;
}

void write_pwm(int fd, int pwm, int pwm_max){
	char data = '1';
	write(fd, &data, sizeof(char));
	usleep(pwm);
	data = '0';
	write(fd, &data, sizeof(char));
	usleep(pwm_max - pwm);
}
