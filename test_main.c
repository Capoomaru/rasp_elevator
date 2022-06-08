#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>
#include <pthread.h>
#include <poll.h>
// nfc
#include <nfc/nfc.h>
#include "NFC/nfc_poll.h"
// gyrosensor
#include <wiringPi.h>
#include <errno.h>
#include <wiringPiI2C.h>

#define SERVO_FILE_NAME "/dev/servo_driver"           // 219	GPIO 18
#define BUTTON_FILE_NAME "/dev/button_driver"         // 220 GPIO 17, 22, 27
#define FLOOR_FND_FILE_NAME "/dev/fnd_driver"         // 222  GPIO 12,13,16,19,20,21,26
#define UPDOWN_FND_FILE_NAME "/dev/updown_fnd_driver" // 223  GPIO 7, 8, 9, 10, 11, 25
#define DC_FILE_NAME "/dev/dc_driver"                 // 224  GPIO 18, 23, 24
#define BUZZER_FILE_NAME "/dev/buzzer_driver"         // 225 GPIO 4
#define LED_FILE_NAME "/dev/led_driver"               // 221 GPIO 0 1

///////////////NOT USED///////////////
// gyro sensor GPIO 2 3
#define REG_POWER_CTL 0x2D
#define REG_DATA_X_LOW 0x32
#define REG_DATA_X_HIGH 0x33
#define REG_DATA_Y_LOW 0x34
#define REG_DATA_Y_HIGH 0x35
#define REG_DATA_Z_LOW 0x36
#define REG_DATA_Z_HIGH 0x37
//////////////////////////////////////

#define MAX_FLOOR 3

/* 모듈들을 오픈하는 함수 선언 */
int init_modules();

/* nfc 쓰레드 실행 함수 선언 */
void *nfc_thread(void *);

/* button 쓰레드 실행 함수 선언 */
void *button_thread(void *);

/* servo 쓰레드 실행 함수 선언 */
void *servo_thread(void *);

///////////////NOT USED///////////////
/* dc 쓰레드 실행 함수 선언 */
void *dc_thread(void *);
/* gyro 쓰레드 실행 함수 선언 */
void *gr_sensor_thread(void *);
//////////////////////////////////////

/* pwm 출력 함수 선언*/
void write_pwm(int fd, int pwm, int pwm_max);

// file descriptor
int floor_fnd_fd, updown_fnd_fd, dc_fd, button_fd, buzzer_fd, led_fd, servo_fd, gr_sensor_fd, servo_fd;
/* int cur_floor -> 현재 엘리베이터의 층을 나타내기 위한 변수 */
int cur_floor = 1;
/* int tatget_floor -> 현재 엘리베이터가 가고자 하는 층 */
char target_floor = 1;
/* int max_floor_nfc -> nfc를 통해 입력된 층 중 가장 높은 층 */
char max_floor_nfc = 1;
/* int max_floor _button-> 버튼을 통해 입력된 층 중 가장 높은 층 */
char max_floor_button = 1;
/* int direction -> 엘리베이터 동작 방향
 * 1 : 위
 * -1 : 아래
 * 0 : 정지
 */
int direction;
/* bool readyQueue[5] -> 엘리베이터의 동작을 요구하는 경우의 수는 각 층의 버튼(1/2/3) or 엘리베이터 내부에서 nfc 태그를 통한 층 입력(2/3)
 * 해당 값은 각각의 상황이 실행될 때 true로 설정되고 원하는 층에 도착하게되면 false로 다시 바꾸어준다.
 * 각 상황에 대한 변수를 따로 가지고 있어서 각 값에 대한 제어는 각각의 쓰레드에서만 진행되므로 mutex를 사용하지 않는다.
 * 0 : 1층에서 button
 * 1 : 2층에서 button
 * 2 : 3층에서 button
 * 3 : 2층에서 nfc
 * 4 : 3층에서 nfc
 */
bool readyQueue[5];
char isLedOn;
bool isServoDone = true;
/* mutex로 잠기게 되는 값은 target_floor이다. */
pthread_mutex_t mutx;
/* mutex로 잠기게 되는 값은 isServoDone이다. */
pthread_mutex_t mutServo;

int main(int argc, char **argv)
{
    pthread_mutex_init(&mutx, NULL);
    pthread_mutex_init(&mutServo, NULL);

    pthread_t dc_thread_id, button_thread_id, nfc_thread_id, gr_sensor_thread_id, servo_thread_id;

    wiringPiSetup();

    if (init_modules() < 0)
        return -1;

    pthread_create(&button_thread_id, NULL, button_thread, NULL);
    pthread_create(&nfc_thread_id, NULL, nfc_thread, NULL);
    ///////////////NOT USED///////////////
    // pthread_create(&gr_sensor_thread_id, NULL, gr_sensor_thread, NULL);
    //////////////////////////////////////

    pthread_detach(button_thread_id);
    pthread_detach(nfc_thread_id);
    ///////////////NOT USED///////////////
    // pthread_detach(gr_sensor_thread_id);
    //////////////////////////////////////

    /* Start main */
    /*
     * 메인에서는 버튼, nfc로부터 입력받은 값들을 종합하여
     * 1. 엘리베이터의 방향 조절 및 target 층 조절
     *   target층은 입력된 최대 층(max_floor_nfc or max_floor_button) 혹은 1층으로 설정됨
     * 2. 해당 층 도착시 입력버퍼(readyQueue)값을 false로 초기화
     *
     * 엘리베이터의 특성상 1층에서 탄 nfc의 입력이 우선적이므로
     * 1층에서 올라갈 때의 target_floor는 max_floor_nfc, 이후 max_floor_button으로 진행한다.
     * max_floor_button에 도착한 후에는 target_floor를 1층으로 지정하고 가는길에 button이 눌린 층에서 멈추었다가 간다.
     *
     */
    while (1)
    {
        if (cur_floor == target_floor)
        {
            if (direction == -1)
            {
                readyQueue[cur_floor - 1] = false;
                if (cur_floor == 1)
                {
                    max_floor_button = 1;
                    for (int i = MAX_FLOOR; i > 1; i--)
                    {
                        if (readyQueue[i - 1] == true)
                        {
                            target_floor = i;
                            max_floor_button = i;
                            break;
                        }
                    }
                }
            }
            if (direction == 1)
            {
                if (cur_floor == max_floor_button)
                {                         //여기 온 이유가 button이면,
                    max_floor_button = 1; // target을 1층으로
                    readyQueue[cur_floor - 1] = false;
                }
                readyQueue[cur_floor + 1] = false;

                max_floor_nfc = 1;
            }

            direction = target_floor < max_floor_button ? 1 : (target_floor == max_floor_button ? 0 : -1);
            char tmp = direction + 1;
            write(updown_fnd_fd, (char *)&(tmp), sizeof(char));

            if (direction == 0)
            {
                isLedOn = 0;
                write(led_fd, &isLedOn, sizeof(char));
            }
            target_floor = max_floor_button;
        }

        /* 엘리베이터가 멈춰있을 때 nfc나 버튼 입력이 생기면 타겟층을 변경해준다
         * 이 때 엘리베이터에 타있는 nfc에게 우선권을 부여한다. -> nfc의 동작이 우선시 되므로
         */
        if (direction == 0 && (max_floor_button != cur_floor || max_floor_nfc != cur_floor))
        {

            target_floor = max_floor_nfc != cur_floor ? max_floor_nfc : max_floor_button;
            direction = target_floor > cur_floor ? 1 : (target_floor == cur_floor ? 0 : -1);
            char tmp = direction + 1;
            isLedOn = 1;
            write(led_fd, &isLedOn, sizeof(char));
            write(updown_fnd_fd, (char *)&tmp, sizeof(char));
        }

        if (isServoDone)
        {
            bool is_stop = false;
            if (direction == -1)
            {
                if (readyQueue[cur_floor - 1 - 1] == true)
                    is_stop = true;
                readyQueue[cur_floor - 1] = false;
            }
            else if (direction == 1)
            { //특정 층에 멈춰야 하는데 멈췄으니
                is_stop = true;
                if (readyQueue[cur_floor + 1] == true)
                {                                      // nfc로 온 경우
                    readyQueue[cur_floor + 1] = false; // 이동층의 입력 지우기
                    if (readyQueue[cur_floor + 1 - 1])
                        is_stop = true;
                }
                else if (cur_floor == max_floor_button && readyQueue[cur_floor - 1] == true)
                {                                      // 버튼에 의한 호출
                    readyQueue[cur_floor - 1] = false; // 이동층의 입력 지우기
                    if (readyQueue[cur_floor + 1 + 1])
                        is_stop = true;
                }
            }
            else
                continue;

            pthread_create(&servo_thread_id, NULL, servo_thread, (void *)&is_stop);
            pthread_detach(servo_thread_id);
        }

        //특정 층에 도착했을 때
    }

    close(dc_fd);
    close(floor_fnd_fd);
    close(updown_fnd_fd);
    close(button_fd);
    close(buzzer_fd);
    close(led_fd);
    close(servo_fd);
    close(gr_sensor_fd);
    close(servo_fd);
    return 0;
}


///////////////NOT USED///////////////
/* DC 쓰레드 정의 부분 */
/*
void *dc_thread(void *arg)
{
    char data = '0';

    if (direction == 1)
    { // 올라가기
        for (int i = 0; i < 50; i++)
        {
            data = '1' write(dc_fd, &data, sizeof(char));
            usleep(700);
            data = '0';
            write(dc_fd, &data, sizeof(char));
            usleep(1300);
        }
        data = '0';
        write(dc_fd, &data);
    }
    else if (direction == 0)
    { // 정지
        write(dc_fd, &data, sizeof(char));
    }
    else if (direction == -1)
    { // 내려가기
        data = '-1';
        for (int i = 0; i < 50; i++)
        {
            data = '-1' write(dc_fd, &data, sizeof(char));
            usleep(700);
            data = '0';
            write(dc_fd, &data, sizeof(char));
            usleep(1300);
        }
        data = '0';
        write(dc_fd, &data, sizeof(char));
    }
}
*/
//////////////////////////////////////

/* nfc 쓰레드 실행 함수 정의 */
/*
 *
 * nfc_poll 함수는 사용자 정의 함수로
 * nfc를 open하고 한번의 태그를 입력받아
 * 기록된 주민 DB에 읽은 nfc uid값이 존재하면 해당 주민의 거주 층(int)을 반환
 * 주민 DB에 해당하지 않는 값이 들어오면 -1 반환
 *
 */
void *nfc_thread(void *arg)
{
    while (1)
    {
        int poll_floor = nfc_poll();
        printf("floor : %d", poll_floor);
        readyQueue[1 + poll_floor] = true;
        if (max_floor_nfc < poll_floor)
        {
            pthread_mutex_lock(&mutx);
            max_floor_nfc = poll_floor;
            target_floor = poll_floor;
            pthread_mutex_unlock(&mutx);
        }
    }
}

/* button 쓰레드 실행 함수 정의 */
/*
 * 사용 핀 : GPIO 17,22,27 세가지 핀
 *
 * poll 발생시 read()를 통해 button에 입력된 버튼 값 저장하고
 * read를 통해 int button에 버튼으로 눌린 층을 읽어옴
 *
 */

void *button_thread(void *arg)
{
    struct pollfd events;
    char button = -1;
    while (1)
    {
        events.fd = button_fd;
        events.events = POLLIN;              // waiting read
        int retval = poll(&events, 1, 1000); // event waiting
        if (retval < 0)
        {
            fprintf(stderr, "Poll error\n");
            exit(0);
        }
        if (events.revents & POLLIN)
        {
            read(button_fd, &button, 1);
            printf("call by %d floor\n", button);
            readyQueue[button - 1] = true;
            if (max_floor_button < button)
            {
                pthread_mutex_lock(&mutx);
                max_floor_button = button;
                pthread_mutex_unlock(&mutx);
            }
            usleep(1000);
        }
    }

    close(button_fd);
    return 0;
}

///////////////NOT USED///////////////
/* gyro 쓰레드 실행 함수 정의 */
/*
 * 자이로센서로부터 x,y,z값을 출력하는 함수
 * 해당 값의 z축 변화를 통해 특정 층에 도착할 수 있도록 구현하려 했으나,
 * 실제로 값을 활용할 수 있는 범위로 출력되지않아 구현에 미포함하였음
 *
 */
void *gr_sensor_thread(void *arg)
{
    wiringPiI2CWriteReg8(gr_sensor_fd, REG_POWER_CTL, 0b00001000);
    while (1)
    {
        int dataX = wiringPiI2CReadReg16(gr_sensor_fd, REG_DATA_X_LOW);
        dataX = -(~(int16_t)dataX + 1);
        int dataY = wiringPiI2CReadReg16(gr_sensor_fd, REG_DATA_Y_LOW);
        dataY = -(~(int16_t)dataY + 1);
        int dataZ = wiringPiI2CReadReg16(gr_sensor_fd, REG_DATA_Z_LOW);
        dataZ = -(~(int16_t)dataZ + 1);
        // printf("x: %d, y: %d, z: %d\n", dataX, dataY, dataZ);
        usleep(100 * 1000 * 10);
    }
    return 0;
}
//////////////////////////////////////

/* servo 쓰레드 실행 함수 정의 */
// arg : bool* is_stop => 타겟층이 아닌 층에서 이번 층에서 정지해야 하는 층인지 알려주는 인자
/*
 * 동작 : 현재 방향(direction)으로 한 층만 이동
 *        이동한 후, 해당 층으로 Floor_fnd 값(char _floor = cur_floor + 1)을 출력하고
 *    if  정지하는 층일 경우, (즉, 타겟층이거나(target_floor == cur_floor +-1), nfc 요청 or button 요청이 있는 경우)
 *        부저를 0.3초 출력, 0.3초 대기를 층수 만큼 반복 재생 후, 2초 대기하고
 *  endif
 *        종료
 *
 * ※전역 변수인 int direction의 값에 따라 동작 방향 결정
 * ※  1 : 위
 * ※ -1 : 아래
 * ※  0 : 정지
 *
 *
 * 동작기간 : 시작할 때 isServoDone을 false로 바꾸고, 동작 종료 시 true로 변환 => 메인 쓰레드에서 이 값을 확인하므로 mutex 사용
 *
 * 물리적인 구조의 한계로 인해, 위아래로 올라갔다 내려오는 동작을 각 층마다 다르게 출력하는 구조
 *
 *
 */

void *servo_thread(void *arg)
{
    char data;
    bool is_stop = *((bool *)arg);
    int i;
    printf("start %d %d %d =", cur_floor, target_floor, direction);
    pthread_mutex_lock(&mutServo);
    isServoDone = false;
    pthread_mutex_unlock(&mutServo);
    if (direction == 1)
    { // 올라가기
        printf("UP\n");
        // 해당하는 층에 맞게 서보모터 출력
        if (cur_floor == 2)
            for (int k = 0; k < 100; k++)
                write_pwm(servo_fd, 1240, 20000);
        else
            for (int k = 0; k < 100; k++)
                write_pwm(servo_fd, 1240, 20000);

        char _floor = cur_floor + 1;                          //이동 후의 층수
        write(floor_fnd_fd, (char *)&(_floor), sizeof(char)); //변화한 층 수 출력
        if (_floor == target_floor || is_stop)
        {
            for (int j = 1; j <= _floor; j++)
            {
                char data = '1';
                write_pwm(buzzer_fd, 100000, 300000); // 0.3초간 부저 출력
                data = '0';
                write(buzzer_fd, (char *)&data, sizeof(char)); //부저를 확실하게 종료시키기
                usleep(300000);                                //횟수 구분을 위해 0.3초 대기
            }
            sleep(2); // 2초간 문열림
        }

        cur_floor++;
    }
    else if (direction == -1)
    { //내려가기
        printf("down\n");
        // 해당하는 층에 맞게 서보모터 출력
        if (cur_floor == 3)
            for (int i = 0; i < 90; i++)
                write_pwm(servo_fd, 1420, 20000);
        else
            for (int i = 0; i < 90; i++)
                write_pwm(servo_fd, 1430, 20000);

        char _floor = cur_floor - 1;                          //이동 후의 층수
        write(floor_fnd_fd, (char *)&(_floor), sizeof(char)); //변화한 층 수 출력
        if (_floor == target_floor || is_stop)
        {
            for (int j = 1; j <= _floor; j++)
            {
                char data;
                write_pwm(buzzer_fd, 100000, 300000); // 0.3초간 부저 출력
                data = '0';
                write(buzzer_fd, (char *)&data, sizeof(char)); //부저를 확실하게 종료시키기
                usleep(300000);                                //횟수 구분을 위해 0.3초 대기
            }
            sleep(2); // 2초간 문열림
        }

        cur_floor--;
    }

    pthread_mutex_lock(&mutServo);
    isServoDone = true;
    pthread_mutex_unlock(&mutServo);

    return 0;
}

/* 모듈들을 오픈하는 함수 정의 */
/*
모든 모듈들의 open을 진행하는 함수
오픈 실패시 종료
nfc, gyro는 드라이버를 구현하지 않은 센서들로
함수 실행 시 자동으로 실행됨
*/
int init_modules()
{
    updown_fnd_fd = open(UPDOWN_FND_FILE_NAME, O_RDWR);
    if (updown_fnd_fd < 0)
    {
        fprintf(stderr, "Can't open %s\n", UPDOWN_FND_FILE_NAME);
        return -1;
    }
    floor_fnd_fd = open(FLOOR_FND_FILE_NAME, O_RDWR);
    if (floor_fnd_fd < 0)
    {
        fprintf(stderr, "Can't open %s\n", FLOOR_FND_FILE_NAME);
        return -1;
    }
    /* ///////////////NOT USED///////////////
     *
    dc_fd = open(DC_FILE_NAME, O_RDWR);
    if (dc_fd < 0) {
         fprintf(stderr, "Can't open %s\n", DC_FILE_NAME);
         return -1;
     }
    gr_sensor_fd = wiringPiI2CSetup(0x53);
    if (gr_sensor_fd < 0)
    {
        fprintf(stderr, "Can't open gyro sensor\n");
        return -1;
    }
    ////////////////////////////////////// */
    button_fd = open(BUTTON_FILE_NAME, O_RDWR | O_NONBLOCK);
    if (button_fd < 0)
    {
        fprintf(stderr, "Can't open %s\n", BUTTON_FILE_NAME);
        return -1;
    }
    buzzer_fd = open(BUZZER_FILE_NAME, O_RDWR);
    if (buzzer_fd < 0)
    {
        fprintf(stderr, "Can't open %s\n", BUZZER_FILE_NAME);
        return -1;
    }
    led_fd = open(LED_FILE_NAME, O_RDWR);
    if (led_fd < 0)
    {
        fprintf(stderr, "Can't open %s\n", LED_FILE_NAME);
        return -1;
    }
    servo_fd = open(SERVO_FILE_NAME, O_RDWR);
    if (servo_fd < 0)
    {
        fprintf(stderr, "Can't open %s\n", SERVO_FILE_NAME);
        return -1;
    }

    return 0;
}

/* pwm 출력 함수 정의*/
/*
pwm_max/pwm 만큼의 출력을 pwm_max동안 진행하는 함수
모듈에 pwm만큼 1을 유지하고 pwm_max - pwm 만큼 0을 유지하여
pwm을 간접적으로 구현한 함수
*/
void write_pwm(int fd, int pwm, int pwm_max)
{ // servo_motor, buzzer
    char data = '1';
    write(fd, &data, sizeof(char));
    usleep(pwm);
    data = '0';
    write(fd, &data, sizeof(char));
    usleep(pwm_max - pwm);
}
