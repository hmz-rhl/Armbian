/**
 * @file expander-set-gpio.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 1
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include <expander_i2c.h>

#define I2C_DEVICE          "/dev/i2c-1"
#define MCP23008_ADDR       (0x26)

int fd;
uint8_t buff = 0;;

//expander-set-gpio 26 --pin 0-7 0-1

int main(int argc, char* argv[]) {

    if(argc == 2 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))){

        printf("Usage: ./expander-set-gpio -h, --help \taffiche ce message d'aide \n");
        printf("       ./expander-set-gpio <addresse> <GPIO 7> ... <GPIO 0> \n");


        printf("exemple: ./expander-set-gpio 27 0 1 0 1 0 1 1 1\n");
        exit(EXIT_SUCCESS);

    }

    if(!((argc == 5 && !strcmp(argv[2],"--pin")) || argc == 10)){
        printf("Usage 1:   expander-set-gpio <addresse> <GPIO 7> ... <GPIO 0> \n");
        printf("exemple: expander-set-gpio 26 0 1 0 1 0 1 1 1\n");
        printf("Usage 2:   expander-set-gpio <addresse> --pin <NUM_GPIO 7> <VALEUR> \n");
        printf("exemple: expander-set-gpio 26 --pin 5 1\n");
        exit(EXIT_FAILURE);
    }
    if(!strcmp(argv[1], "26") || !strcmp(argv[1], "27") || !strcmp(argv[1], "20")) {
        int address = 0x26;
        if(!strcmp(argv[1], "27")) {
            address = 0x27;
        }
        else if(!strcmp(argv[1], "20")) {
            address = 0x20;
        }
       
        expander_t *exp = expander_init(address);
        expander_setPullup(exp, 0XFF);
        
        if(argc == 10) {
            for (size_t i = 0; i < 8; i++) {
                buff += (uint8_t)(pow(2,i)*(atoi(argv[9-i])));
            }
            expander_setAndResetSomePinsGPIO(exp, buff);
            //expander_printGPIO(exp);
        }

        if(argc == 5 && !strcmp(argv[2], "--pin")) {
            int pin = 0;
            if(!strcmp(argv[3], "0") || !strcmp(argv[3], "1") || !strcmp(argv[3], "2") || !strcmp(argv[3], "3") || !strcmp(argv[3], "4") || !strcmp(argv[3], "5") || !strcmp(argv[3], "6") || !strcmp(argv[3], "7")) {
                pin = atoi(argv[3]);
                if (!strcmp(argv[4], "1")) {
                    expander_setPinGPIO(exp,pin);
                    //expander_printGPIO(exp);
                }
                else if (!strcmp(argv[4], "0")) {
                    expander_resetPinGPIO(exp,pin);
                    //expander_printGPIO(exp);
                }
            }
        }
        expander_close(exp);
        expander_Free(exp);
    }
    else {
        printf("Les adresse valides sont 20, 26 et 27\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

