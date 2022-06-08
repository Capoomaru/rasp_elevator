#!/bin/bash

# shellcheck disable=SC2046
# shellcheck disable=SC2006
# shellcheck disable=SC2005
# USE ternimal command : sudo bash init.sh
# Make Node for modules
echo `mknod /dev/servo_driver c 219 0`
echo `mknod /dev/button_drvier c 220 0` # 해당 부분 버그로 인해 수동 작업 필요
echo `mknod /dev/led_driver c 221 0`
echo `mknod /dev/fnd_driver c 222 0`
echo `mknod /dev/updown_fnd_driver c 223 0`
echo `mknod /dev/dc_driver c 224 0`
echo `mknod /dev/buzzer_driver c 225 0`
# Insert modules
echo `insmod ./SERVO_Driver/servo_driver.ko`
echo `insmod ./Button/button_driver.ko`
echo `insmod ./LED_Driver/led_driver.ko`
echo `insmod ./FND_Driver/fnd_driver.ko`
echo `insmod ./UpDownFnd/updown_fnd_driver.ko`
echo `insmod ./DC_Driver/dc_driver.ko`
echo `insmod ./Buzzer/buzzer_driver.ko`

echo `gcc -o main ./test_main.c -lpthread -lnfc -lwiringPi`