#include <stdio.h>
#include <errno.h>
#include <math.h>
 
#include <wiringPi.h>
#include <wiringPiI2C.h>
 
#define delay_ms delay
#define delay_us delayMicroseconds
 
#define acc_value 0.00390625
 
#define OFSX        0x1E
#define OFSY        0x1F
#define OFSZ        0x20
#define BW_RATE     0x2C
#define POWER_CTL   0x2D
#define DATA_FORMAT 0x31
 
void I2C_INIT(int sensor) {
    wiringPiI2CWriteReg8(sensor, OFSX, -2) ;  // Offset x, y, x
    wiringPiI2CWriteReg8(sensor, OFSY, -1) ;
    wiringPiI2CWriteReg8(sensor, OFSZ,  4) ;
 
    wiringPiI2CWriteReg8(sensor, POWER_CTL, 0x08) ;
    wiringPiI2CWriteReg8(sensor, DATA_FORMAT, 0x0B) ;
}
 
// Main Function
int main() {
    int i, sensor ;
    short x, y, z ;
    float acc_x, acc_y, acc_z, roll, pitch, yaw ;
 
    wiringPiSetup();
 
    if((sensor = wiringPiI2CSetup(0x53)) == -1) {
        fprintf(stderr, "sensor: Unable to initialise I2C: %s\n", strerror (errno)) ;
        return 1 ;
    }
 
    printf ("I2C device ID: %d\n", sensor) ;
    printf ("ADXL345 Digital Accelerometer Test.....\n") ;
    
    I2C_INIT(sensor) ;  // Sensor init
    delay_ms(500) ;
 
    while(1) {
        x = y = z = 0 ;
 
        for(i=0; i<8; i++) {
            x += ((wiringPiI2CReadReg8(sensor, 0x33) << 8)& 0xFF00) | (wiringPiI2CReadReg8(sensor, 0x32) & 0xFF) ;
            y += ((wiringPiI2CReadReg8(sensor, 0x35) << 8)& 0xFF00) | (wiringPiI2CReadReg8(sensor, 0x34) & 0xFF) ;
            z += ((wiringPiI2CReadReg8(sensor, 0x37) << 8)& 0xFF00) | (wiringPiI2CReadReg8(sensor, 0x36) & 0xFF) ;
 
            delay_ms(10) ;  // Default Output Date Rate 100Hz
        }
 
        x /= 8 ;  y /= 8 ;  z /= 8 ;
 
        acc_x = (float)x * acc_value ;
        acc_y = (float)y * acc_value ;
        acc_z = (float)z * acc_value ;
 
        roll  = atan(acc_y / sqrt(acc_x * acc_x + acc_z * acc_z)) * 180.0 / M_PI;
        pitch = atan(acc_x / sqrt(acc_y * acc_y + acc_z * acc_z)) * 180.0 / M_PI;
        yaw   = atan(sqrt(acc_x * acc_x + acc_y * acc_y) / acc_z) * 180.0 / M_PI;
        
        printf ("Roll = %5.1f / Pitch = %5.1f / Yaw = %5.1f\n", roll, pitch, yaw) ;
 
        //delay_ms(1000) ;
    }
    
    return 0 ;
}
