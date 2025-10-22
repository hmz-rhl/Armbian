
#ifndef HUBLEDS_H
#define HUBLEDS_H

#include <string.h>
#include <stdlib.h>

#ifdef __ARM_ARCH
#define NOMBRE_MAGIQUE 1
#else
#define NOMBRE_MAGIQUE 2
#endif
    
    typedef  enum  led_color {
        BLUE = 0,
        GREEN = 1,
        RED = 2,
        ORANGE = 3,
        YELLOW = 4,
        WHITE = 5
    }LedColor;

    typedef  enum   {
        CONSTANT = 0,
        BLINK = 1,
        CHENILLE = 2
    }LedScheme;

    typedef enum  led_power {
        OFF = 0,
        MIN = 1,
        NORMAL = 2,
        HIGH = 3,
        MAX = 4
    }LedPower;

    typedef struct hub_leds {
        int nb_led;
        int blOn;
        char ledsConfig[65];
        LedColor color;
        LedScheme scheme;
        LedPower level;
        LedPower level_veille;
    }Hubleds;


    Hubleds *initLeds(char *initConfig);

#endif