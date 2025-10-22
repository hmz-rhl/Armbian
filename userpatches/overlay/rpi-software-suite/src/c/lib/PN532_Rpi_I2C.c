#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <time.h>
#include <sys/time.h>

#include "BSP_wrappers.h"
#include "PN532_Rpi_I2C.h"
// todo : a migrer dans wrapper

#define _RESET_PIN                    (8)  //(20)
#define _REQ_PIN                      (8)  //(16)    

#define _I2C_READY                      (0x01)
#define _I2C_ADDRESS                    (0x48 >> 1)
#define _I2C_CHANNEL                    (5)

#if 1

static int fd = 0;



int PN532_Reset(void) {
    // WRP_writeGpio(_RESET_PIN, HIGH);
    // delay(100);
    // WRP_writeGpio(_RESET_PIN, LOW);
    // delay(500);
    // WRP_writeGpio(_RESET_PIN, HIGH);
    // delay(100);
    return PN532_STATUS_OK;
}

void PN532_Log(const char* log) {
    printf(log);
    printf("\r\n");
}



/**************************************************************************
 * I2C
 **************************************************************************/
int PN532_I2C_ReadData(uint8_t* data, uint16_t count) {
    uint8_t status[] = {0x00};
    uint8_t frame[count + 1];
    read(fd, status, sizeof(status));
    if (status[0] != _I2C_READY) {
        return PN532_STATUS_ERROR;
    }
    read(fd, frame, count + 1);
    for (uint8_t i = 0; i < count; i++) {
        data[i] = frame[i + 1];
    }
    return PN532_STATUS_OK;
}

int PN532_I2C_WriteData(uint8_t *data, uint16_t count) {
    write(fd, data, count);
    return PN532_STATUS_OK;
}

bool PN532_I2C_WaitReady(uint32_t timeout) {
    uint8_t status[] = {0x00};
    struct timespec timenow;
    struct timespec timestart;
    clock_gettime(CLOCK_MONOTONIC, &timestart);
    while (true) {
        read(fd, status, sizeof(status));
        if (status[0] == _I2C_READY) {
            return true;
        } else {
            // attente 10 msec
            usleep(10000);
        }
        clock_gettime(CLOCK_MONOTONIC, &timenow);
        if ((timenow.tv_sec - timestart.tv_sec) * 1000 + \
            (timenow.tv_nsec - timestart.tv_nsec) / 1000000 > timeout) {
            break;
        }
    }
    // Time out!
    return false;
}

int PN532_I2C_Wakeup(void) {
    // WRP_writeGpio(_REQ_PIN, HIGH);
    // delay(100);
    // WRP_writeGpio(_REQ_PIN, LOW);
    // delay(100);
    // WRP_writeGpio(_REQ_PIN, HIGH);
    // delay(500);
    return PN532_STATUS_OK;
}

int PN532_I2C_Init(PN532* pn532) {
    // init the pn532 functions
    pn532->reset = PN532_Reset;
    pn532->read_data = PN532_I2C_ReadData;
    pn532->write_data = PN532_I2C_WriteData;
    pn532->wait_ready = PN532_I2C_WaitReady;
    pn532->wakeup = PN532_I2C_Wakeup;
    pn532->log = PN532_Log;
    char devname[20];
    snprintf(devname, 19, "/dev/i2c-%d", _I2C_CHANNEL);
    fd = open(devname, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "%s: Unable to open i2c device: %s\n",__func__, strerror(errno));
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE, _I2C_ADDRESS) < 0) {
        fprintf(stderr, "%s: Unable to open i2c device: %s\n",__func__, strerror(errno));
        return -1;
    }
    // if (wiringPiSetupGpio() < 0) {  // using Broadcom GPIO pin mapping
    //     return;
    // }
    // pinMode(_REQ_PIN, OUTPUT);
    // pinMode(_RESET_PIN, OUTPUT);
    // hardware reset
    pn532->reset();
    // hardware wakeup
    pn532->wakeup();
    return 0;
}

void PN532_I2C_Close(){

    if(close(fd) <0){

        fprintf(stderr, "%s: Unable to close i2c device: %s\n",__func__, strerror(errno));

    }
}
/**************************************************************************
 * End: I2C
 **************************************************************************/
#else
int PN532_Reset(void) {
    return PN532_STATUS_OK;
}
void PN532_Log(const char* log) { }

//PN532_Rpi_I2C.c
int PN532_I2C_ReadData(uint8_t* data, uint16_t count) {
    return PN532_STATUS_OK;
}
int PN532_I2C_WriteData(uint8_t *data, uint16_t count) {
    return PN532_STATUS_OK;
}
bool PN532_I2C_WaitReady(uint32_t timeout) {   
    return true;
}
int PN532_I2C_Wakeup(void) {
    return PN532_STATUS_OK;
}
int PN532_I2C_Init(PN532* pn532) {
    // init the pn532 functions
    pn532->reset = PN532_Reset;
    pn532->read_data = PN532_I2C_ReadData;
    pn532->write_data = PN532_I2C_WriteData;
    pn532->wait_ready = PN532_I2C_WaitReady;
    pn532->wakeup = PN532_I2C_Wakeup;
    pn532->log = PN532_Log;
    pn532->reset();
    // hardware wakeup
    pn532->wakeup();
    return 0;
}
void PN532_I2C_Close(){}

#endif
