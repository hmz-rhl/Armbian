/**
 * @file BSP_wrappers.c
 * @author	Hamza RAHAL (hamza.rahal@gmx.fr)
 * @brief wrappers pour acces aux ressources HW
 * @version 1.1.0
 * @date 2025-01-07
 * 
 * @copyright Copyright (c) 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "BSP_wrappers.h"
/* TODO:
 * mécanisme de log + logrotate
*/
#ifdef BSP_DEBUG
    // #define debug_print(...)    printf("%s:", __func__, __VA_ARGS__)
    #define debug_print(fmt, ...)    printf("%s: " fmt, __func__, ##__VA_ARGS__)
#else
    #define debug_print(...)    do {} while(0);
#endif

/*
    TODO:
    - sanity check
    - (init) rendre l'attribution du nom de la chip automatique en utilisant le retour de gpiofind
    - (init) exporter le pwm


*/

#define CONSUMER    "WRAPPER"

char nameOfChip0[] = "gpiochip0";
char nameOfChip1[] = "gpiochip1";
char nameOfChip2[] = "gpiochip2";
char nameOfChip3[] = "gpiochip3";
char pwm_export_path[] = "/sys/class/pwm/pwmchip0/export";

static wrp_gpio_t gpios[PIN_GPIO_N] = {

    [PIN_RCD_TRIP_AC] = {
        .chipName = nameOfChip0,
        .pinName = "RCD_TRIP_AC",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DISABLE,

    },
    
    [PIN_RCD_TRIP_DC] = {
        .chipName = nameOfChip0,
        .pinName = "RCD_TRIP_DC",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DISABLE,
    },

    [PIN_PP_IN] = {
        .chipName = nameOfChip0,
        .pinName = "PP_IN",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DISABLE,
    },

    [PIN_CF4] = {
        .chipName = nameOfChip0,
        .pinName = "CF4",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DOWN,
    },

    [PIN_DISPLAY_POWER] = {
        .chipName = nameOfChip0,
        .pinName = "DISPLAY_POWER",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_LOW,
    },

    [PIN_IRQ0] = {
        .chipName = nameOfChip0,
        .pinName = "IRQ0",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DOWN,
    },

    [PIN_IRQ1] = {
        .chipName = nameOfChip0,
        .pinName = "IRQ1",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DOWN,
    },

    [PIN_DISPLAY_REDUCE_POWER] = {
        .chipName = nameOfChip0,
        .pinName = "DISPLAY_REDUCE_POWER",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
        .pinBias = GPIO_PULL_DOWN,
    },

    [PIN_LOCK_FB] = {
        .chipName = nameOfChip1,
        .pinName = "LOCK_FB",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DISABLE,
    },

    [PIN_USER_KEY] = {
        .chipName = nameOfChip0,
        .pinName = "USER_KEY",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DISABLE,
    },

    [PIN_WD_TRIP] = {
        .chipName = nameOfChip0,
        .pinName = "WD_TRIP",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
        .pinBias = GPIO_PULL_DISABLE,
    },

    [PIN_LOCK_P] = {
        .chipName = nameOfChip0,
        .pinName = "LOCK_P",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
        .pinBias = GPIO_PULL_DISABLE,
    },

    [PIN_SM_TIC_D] = {
        .chipName = nameOfChip0,
        .pinName = "SM_TIC_D",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DISABLE,
    },

    [PIN_SM_TIC_D_BIS] = {
        .chipName = nameOfChip0,
        .pinName = "SM_TIC_D_BIS",
        .pinDirection = GPIO_INPUT,
        .pinBias = GPIO_PULL_DISABLE,
    },

    //expander A 0x26

    [PIN_TYPE2_NL1_ON] = {
        .chipName = nameOfChip2,
        .pinName = "TYPE-2_NL1_ON",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_LOW,
    },
    [PIN_TYPE2_L2L3_ON] = {
        .chipName = nameOfChip2,
        .pinName = "TYPE-2_L2L3_ON",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_LOW,
    },
    [PIN_TYPE_EF_ON] = {
        .chipName = nameOfChip2,
        .pinName = "TYPE-E/F_ON",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_LOW,
    },
    [PIN_LOCK_D] = {
        .chipName = nameOfChip2,
        .pinName = "LOCK_D",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_LOW,
    },
    [PIN_RCD_DIS] = {
        .chipName = nameOfChip2,
        .pinName = "RCD_DIS",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },
    [PIN_RCD_TST] = {
        .chipName = nameOfChip2,
        .pinName = "RCD_TST",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },
    [PIN_RCD_TRIP_RESET] = {
        .chipName = nameOfChip2,
        .pinName = "RCD_TRIP_DC_RESET", // a modifier
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },
    [PIN_RCD_TRIP_AC_DIS] = {
        .chipName = nameOfChip2,
        .pinName = "RCD_TRIP_AC_DIS", // a modifier
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },

    // expander B


    [PIN_LED_DIS] = {
        .chipName = nameOfChip3,
        .pinName = "LED_DIS",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },
    [PIN_CP_DIS] = {
        .chipName = nameOfChip3,
        .pinName = "CP_DIS",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },
    /* controlled by driver just for info
    [PIN_PP_CS] = {
        .chipName = nameOfChip3,
        .pinName = "PP_CS",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },
    [PIN_CP_CS] = {
        .chipName = nameOfChip3,
        .pinName = "CP_CS",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },
    [PIN_T_CS] = {
        .chipName = nameOfChip3,
        .pinName = "T_CS",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },
    [PIN_PM_CS] = {
        .chipName = nameOfChip3,
        .pinName = "PM_CS",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_HIGH,
    },
    */
    [PIN_PM1] = {
        .chipName = nameOfChip3,
        .pinName = "PM1",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_LOW,
    },
    [PIN_PM0] = {
        .chipName = nameOfChip3,
        .pinName = "PM0",
        .pinDirection = GPIO_OUTPUT,
        .pinValue = GPIO_LOW,
    },
};

wrp_pwm_t pwm = {
    .path = "/sys/class/pwm/pwmchip0/pwm0/",
    //.path = "/root/pwm",
};

int init_called = 0;

static pthread_t thread_id[PIN_GPIO_N];
static bool running = true;
static unsigned int g_nb_interrupts = 0;

int WRP_init(void){

    int ret = 0;

    // export pwm
    FILE *fptr = fopen(pwm_export_path, "w");

    if(fptr == NULL){
        printf("%s: error: failed to open file, %s\n", __func__, strerror(errno));
        ret = -1;
        goto end;
    }

    ret = fprintf(fptr, "0");
    if(ret != 1){
        printf("%s: error: failed to fprintf, %s\n", __func__, strerror(errno));
        ret = -1;
        goto close_fptr;
    }
    ret = 0;

    debug_print("%s: pwm exported\n", __func__);

    close_fptr:
    fclose(fptr);

    // for(wrp_pin_gpio_e pin=0; pin< PIN_GPIO_N; pin++ ){
    //     if(gpios[pin].pinDirection == GPIO_OUTPUT){
    //         WRP_writeGpio(pin, gpios[pin].pinValue); // pour tout les pin en output mettre la conf par defaut
    //     }
    //     else{
    //         WRP_readGpio(pin); // sinon lire pour voir si pas de soucis
    //     }
    // }

    end:
    return ret;

}

wrp_gpio_val_t WRP_readGpio(wrp_pin_gpio_e pin){

    int ret = 0;

    struct gpiod_chip *chip;
    struct gpiod_line *line;

    int value;

    debug_print("%s: (%s)\n", __func__, gpios[pin].pinName);

    chip = gpiod_chip_open_by_name(gpios[pin].chipName);
    if (!chip) {
		printf("%s: Open chip failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto end;
	}

    line = gpiod_chip_find_line(chip, gpios[pin].pinName);
	if (!line) {
		printf("%s: Get line failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto close_chip;
	}

    ret = gpiod_line_request_input(line, CONSUMER);
	if (ret < 0) {
		printf("%s: Request line as input failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto release_line;
	}

    value = gpiod_line_get_value(line);
    if (value < 0) {
		printf("%s: Get line value failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto release_line;
	}

    debug_print("value is %d\n", value);
    gpios[pin].pinValue = value;

    release_line:
	gpiod_line_release(line);
    close_chip:
	gpiod_chip_close(chip);
    end:
    return value;
}

void WRP_writeGpio(wrp_pin_gpio_e pin, wrp_gpio_val_t value){

    int ret = 0;

    struct gpiod_chip *chip;
    struct gpiod_line *line;

    debug_print("%s: (%s)\n", __func__, gpios[pin].pinName);

    chip = gpiod_chip_open_by_name(gpios[pin].chipName);
    if (!chip) {
		printf("%s: Open chip failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto end;
	}

    line = gpiod_chip_find_line(chip, gpios[pin].pinName);
	if (!line) {
		printf("%s: Get line failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto close_chip;
	}

    ret = gpiod_line_request_output(line, CONSUMER, value);
	if (ret < 0) {
		printf("%s: Request line as output failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto release_line;
	}

    ret = gpiod_line_set_value(line, value); // peut etre redondant ?
    if (ret < 0) {
        printf("%s: Set line value failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
        goto release_line;
    }

    gpios[pin].pinValue = value;

    release_line:
	gpiod_line_release(line);
    close_chip:
	gpiod_chip_close(chip);

    end:
    return;

}

void WRP_setDirGpio(wrp_pin_gpio_e pin, wrp_gpio_dir_t direction){

    int ret = 0;

    struct gpiod_chip *chip;
    struct gpiod_line *line;
    
    debug_print("%s: (%s)\n", __func__, gpios[pin].pinName);
    
    chip = gpiod_chip_open_by_name(gpios[pin].chipName);
    if (!chip) {
		printf("%s: Open chip failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto end;
	}

    line = gpiod_chip_find_line(chip, gpios[pin].pinName);
	if (!line) {
		printf("%s: Get line failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto close_chip;
	}

    if(direction == GPIO_OUTPUT){

        ret = gpiod_line_request_output(line, CONSUMER, gpios[pin].pinValue);
        if (ret < 0) {
            printf("%s: Request line as output failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
            goto release_line;
        }
    }
    else{
        ret = gpiod_line_request_input(line, CONSUMER);
        if (ret < 0) {
            printf("%s: Request line as input failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
            goto release_line;
        }
    }

    gpios[pin].pinDirection = direction;

    release_line:
	gpiod_line_release(line);
    close_chip:
	gpiod_chip_close(chip);

    end:
    return;

}

void WRP_setPullModeGpio(wrp_pin_gpio_e pin, wrp_gpio_bias_t pull){

    int ret = 0;

    struct gpiod_chip *chip;
    struct gpiod_line *line;

    debug_print("%s: (%s)\n", __func__, gpios[pin].pinName);

    chip = gpiod_chip_open_by_name(gpios[pin].chipName);
    if (!chip) {
		printf("%s: Open chip failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto end;
	}

    line = gpiod_chip_find_line(chip, gpios[pin].pinName);
	if (!line) {
		printf("%s: Get line failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
		goto close_chip;
	}

    if(gpios[pin].pinDirection == GPIO_OUTPUT){

        ret = gpiod_line_request_output(line, CONSUMER, gpios[pin].pinValue);
        if (ret < 0) {
            printf("%s: Request line as output failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
            goto release_line;
        }
    }
    else{
        ret = gpiod_line_request_input(line, CONSUMER);
        if (ret < 0) {
            printf("%s: Request line as input failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
            goto release_line;
        }
    }

    ret = gpiod_line_set_flags(line, pull);
    if (ret < 0) {
        printf("%s: set up pull mode failed for pin %s, %s\n", __func__, gpios[pin].pinName, strerror(errno));
        goto release_line;
    }

    gpios[pin].pinBias = pull;

    debug_print("set pull mode of %s succeeded\n", gpios[pin].pinName);

    release_line:
	gpiod_line_release(line);
    close_chip:
	gpiod_chip_close(chip);
    end:
    return;

}

void *gpioMonitor(void *arg) {

    struct gpiod_line_event event;
    wrp_gpio_t gpio = *((wrp_gpio_t*)arg);
    wrp_gpio_edge_t edge = gpio.pinEdge;

    struct gpiod_chip *chip = gpiod_chip_open_by_name(gpio.chipName);
    if (!chip) {
        fprintf(stderr, "%s: Erreur ouverture du chip GPIO\n", __func__);
        return NULL;
    }

    struct gpiod_line *line = gpiod_chip_find_line(chip, gpio.pinName);
    if (!line) {
        fprintf(stderr, "%s: Erreur configuration GPIO %s\n", __func__, gpio.pinName);
        gpiod_chip_close(chip);
        return NULL;
    }

    if(edge == GPIO_EDGE_BOTH){
        if(gpiod_line_request_both_edges_events(line, CONSUMER) < 0){
            fprintf(stderr, "Erreur configuration both events GPIO %s\n", gpio.pinName);
            gpiod_chip_close(chip);
            return NULL;
        }
    }
    else if(edge == GPIO_EDGE_FALLING){
        if(gpiod_line_request_falling_edge_events(line, CONSUMER) < 0){
            fprintf(stderr, "Erreur configuration falling events GPIO %s\n", gpio.pinName);
            gpiod_chip_close(chip);
            return NULL;
        }

    }
    else if(edge == GPIO_EDGE_RISING){
        if(gpiod_line_request_rising_edge_events(line, CONSUMER) < 0){
            fprintf(stderr, "Erreur configuration rising events GPIO %s\n", gpio.pinName);
            return NULL;
        }
    }

    printf("%s: start of thread\n", __func__);
    // printf("pinLine %d\n", line);
    printf("%s: gpio %s\n", __func__, gpio.pinName);
    while (gpio.pinCallback) {
        int ret = gpiod_line_event_wait(line, NULL);
        // printf("%s: event !\n", __func__);
        if ((ret > 0) && (gpiod_line_event_read(line, &event) == 0)) {
            // printf("%s: call of callback\n", __func__);
            if(edge == GPIO_EDGE_BOTH){
                switch (event.event_type) {
                    case GPIOD_CTXLESS_EVENT_RISING_EDGE:
                        printf("%s: rising gpio %s\n", __func__, gpio.pinName);
                        gpio.pinCallback(GPIO_EDGE_RISING);
                        break; 
                    default:
                        printf("%s: falling gpio %s\n", __func__, gpio.pinName);
                        gpio.pinCallback(GPIO_EDGE_FALLING);
                }
            }
            else{
                gpio.pinCallback(edge);
            }
            
        }
        usleep(1000);
    }
    gpiod_chip_close(chip);
    printf("%s: exit thread\n", __func__);
    return NULL;
}

void WRP_interruptGpio(wrp_pin_gpio_e pin, wrp_gpio_edge_t edge,  void (*cb)(wrp_gpio_edge_t)){
    
    if (pin < 0 || pin >= PIN_GPIO_N) {
        fprintf(stderr, "GPIO hors limites !\n");
        return;
    }

    if( gpios[pin].onInterrupt == 1){
        fprintf(stderr, "GPIO already on interrupt (removeInterrupt before setting a new interrupt)!\n");
        return;   
    }

    gpios[pin].pinCallback = cb;
    gpios[pin].pinEdge = edge;
    printf("Interrupt configurée sur GPIO %s\n", gpios[pin].pinName);

    if(pthread_create(&thread_id[pin], NULL, gpioMonitor, &(gpios[pin])) != 0){
        fprintf(stderr, "Erreur thread_create GPIO %s\n", gpios[pin].pinName);
    }
    else{
        gpios[pin].onInterrupt = 1;
    }
    
    return;
}

void WRP_removeInterruptGpio(wrp_pin_gpio_e pin){
    if(gpios[pin].onInterrupt) {
        fprintf(stderr, "no interrupt on GPIO %s\n", gpios[pin].pinName);
        return;
    }
    if(pthread_cancel(thread_id[pin]) != 0){
        fprintf(stderr, "Erreur thread_cancel GPIO %s\n", gpios[pin].pinName);
        return;
    }

    gpios[pin].onInterrupt = 0;

    return;
}


//PWM
void WRP_setPolarityPwm(wrp_pwm_polarity_t polarity){

    int buf_size = 0, path_size = 0;
    char buf[16], polarity_path[128];
    int fd = 0;

    debug_print("polarity = %u\n", polarity);

    buf_size = sprintf(buf, "%s", polarity == PWM_INVERSED ? "inversed" : "normal");
    debug_print("sprintf returned %d\n", buf_size);

    if(buf_size < 0){
        printf("%s: failed to prepare buffer for pwm polarity, %s\n", __func__, strerror(errno));
        goto end;
    }

    path_size = sprintf(polarity_path, "%s/polarity", pwm.path);
    debug_print("sprintf returned %d\n", path_size);
    if(path_size < 0){
        printf("%s: failed to prepare buffer for pwm polarity path, %s\n", __func__, strerror(errno));
        goto end;
    }

    debug_print("polarity_path is : %s\n", polarity_path);
    debug_print("buf is : %s\n", buf);

    fd = open(polarity_path, O_RDWR);
    if(fd < 0){
        printf("%s: failed to open polarity_path, %s\n", __func__, strerror(errno));
        goto end;
    }

    if(write(fd, buf, buf_size) != buf_size){
        printf("%s: failed to write polarity, %s\n", __func__, strerror(errno));
        goto close_fd;
    }

    pwm.polarity = polarity;

    close_fd:
    close(fd);
    end:
    return;

}

void WRP_enablePwm(wrp_pwm_state_t state){

    int buf_size = 0, path_size = 0;
    char buf[16], enable_path[128];
    int fd = 0;

    debug_print("state = %u\n", state);

    buf_size = sprintf(buf, "%u", state);
    debug_print("sprintf returned %d\n", buf_size);

    if(buf_size < 0){
        printf("%s: failed to prepare buffer for pwm state, %s\n", __func__, strerror(errno));
        goto end;
    }

    path_size = sprintf(enable_path, "%s/enable", pwm.path);
    debug_print("sprintf returned %d\n", path_size);
    if(path_size < 0){
        printf("%s: failed to prepare buffer for pwm enable path, %s\n", __func__, strerror(errno));
        goto end;
    }

    debug_print("enable_path is : %s\n", enable_path);
    debug_print("buf is : %s\n", buf);

    fd = open(enable_path, O_RDWR);
    if(fd < 0){
        printf("%s: failed to open enable_path, %s\n", __func__, strerror(errno));
        goto end;
    }

    if(write(fd, buf, buf_size) != buf_size){
        printf("%s: failed to write state, %s\n", __func__, strerror(errno));
        goto close_fd; 
    }

    pwm.state = state;

    close_fd:
    close(fd);
    end:
    return;

}

void WRP_setClockPwm(uint32_t period_us){

    int buf_size = 0, path_size = 0;
    char buf[16], period_path[128];
    int fd = 0;

    buf_size = sprintf(buf, "%u", period_us);
    debug_print("sprintf returned %d\n", buf_size);
    if(buf_size < 0){
        printf("%s: failed to prepare buffer for pwm period_us, %s\n", __func__, strerror(errno));
        goto end;
    }

    path_size = sprintf(period_path, "%s/period", pwm.path);
    debug_print("sprintf returned %d\n", path_size);
    if(path_size < 0){
        printf("%s: failed to prepare buffer for pwm period path, %s\n", __func__, strerror(errno));
        goto end;
    }

    debug_print("period path is : %s\n", period_path);
    debug_print("buf is : %s\n", buf);

    fd = open(period_path, O_RDWR);
    if(fd < 0){
        printf("%s: failed to open duty_cycle, %s\n", __func__, strerror(errno));
        goto end;
    }

    if(write(fd, buf, buf_size) != buf_size){
        printf("%s: failed to write duty_cycle, %s\n", __func__, strerror(errno));
        goto close_fd;
    }

    pwm.period_us = period_us;

    close_fd:
    close(fd);
    end:
    return;

}

void WRP_writePwm(uint8_t dutyCycle){

    int buf_size = 0, path_size = 0;
    char buf[16], duty_cycle_path[128];
    int fd = 0;
    float dutyCycleFloat = ( dutyCycle * (float)pwm.period_us ) / 100.0f;
    uint32_t dutyCycle_us = (uint32_t)dutyCycleFloat;

    debug_print("dutyCycle_us = %u\n", dutyCycle_us);
    debug_print("dutyCycleFloat = %f\n", dutyCycleFloat);
    debug_print("dutyCycle = %u\n", dutyCycle);

    buf_size = sprintf(buf, "%u", dutyCycle_us);
    debug_print("sprintf returned %d\n", buf_size);
    if(buf_size < 0){
        printf("%s: failed to prepare buffer for pwm dutyCycle_us, %s\n", __func__, strerror(errno));
        goto end;
    }

    path_size = sprintf(duty_cycle_path, "%s/duty_cycle", pwm.path);
    debug_print("sprintf returned %d\n", path_size);
    if(path_size < 0){
        printf("%s: failed to prepare buffer for pwm period path, %s\n", __func__, strerror(errno));
        goto end;
    }

    debug_print("duty_cycle_path is : %s\n", duty_cycle_path);
    debug_print("buf is : %s\n", buf);

    fd = open(duty_cycle_path, O_RDWR);
    if(fd < 0){
        printf("%s: failed to open duty_cycle_path, %s\n", __func__, strerror(errno));
        goto end;
    }

    if(write(fd, buf, buf_size) != buf_size){
        printf("%s: failed to write duty_cycle, %s\n", __func__, strerror(errno));
        goto close_fd;
    }

    pwm.dutyCycle = dutyCycle;

    close_fd:
    close(fd);
    end:
    return;
}


wrp_adcs_t adcs [3] = {
    [ADC_TEMPERATURE] = {
        .label = "temperature_power_pcb",
        .nbChannels = 2,
        .path = "/sys/bus/iio/devices/iio:device2/in_voltage0_raw",
    },

    [ADC_CP] = {
        .label = "CP",
        .nbChannels = 2,
        .path = "/sys/bus/iio/devices/iio:device0/in_voltage0_raw",
    },

    [ADC_PP] = {
        .label = "PP",
        .nbChannels = 2,
        .path = "/sys/bus/iio/devices/iio:device1/in_voltage0_raw",
    },
};

//ADC
uint16_t WRP_readAdc(wrp_adc_t adc, uint8_t channel){

    uint16_t value = 0;
    int ret;
    FILE *fptr = fopen(adcs[adc].path, "r");

    if(fptr == NULL){
        printf("%s: error: failed to open file, %s\n", __func__, strerror(errno));
        goto end;
    }

    ret = fscanf(fptr, "%hu", &value);
    if(ret != 1){
        printf("%s: error: failed to scanf, %s\n", __func__, strerror(errno));
        goto close_fptr;
    }

    debug_print("%s: value read is %hu\n",adcs[adc].label, value);

    close_fptr:
    fclose(fptr);

    // struct iio_context  *ctx;
    // struct iio_device   *dev;
    // struct iio_channel  *chn;
    // struct iio_attr     *attr; 
    // uint32_t value;
    // int ret;

    // // Créer un contexte IIO
    // ctx = iio_create_context_from_uri("local:");
    // if (!ctx) {
    //     prin tf("%s:Error: Unable to create IIO context\n, %s\n", __func__, strerror(errno));
    //     goto end;
    // }

    // attr = iio_context_get_attr(ctx, "in_voltage0_raw");
    // if (!attr) {
    //     prin tf("%s:Error: Unable to create IIO attribute\n, %s\n", __func__, strerror(errno));
    //     goto end;
    // }
    // // Trouver l'appareil ADC
    // dev = iio_context_find_device(ctx, adcs[adc].label);  // Assurez-vous que le nom du périphérique est correct
    // if (!dev) {
    //     prin tf("%s:Error: Unable to find device\n, %s\n", __func__, strerror(errno));
    //     goto destroy;
    // }

    // // Trouver la première entrée du canal ADC
    // chn = iio_device_get_channel(dev, channel);  // Utilisez le bon numéro de canal
    // if (!chn) {
    //     prin tf("%s:Error: Unable to find ADC channel\n, %s\n", __func__, strerror(errno));
    //     goto destroy;
    // }

    // // Lire la valeur de l'ADC
    // ret = iio_channel_read_raw(chn, &value);
    // if (ret < 0) {
    //     prin tf("%s:Error: Unable to read from channel\n, %s\n", __func__, strerror(errno));
    //     goto destroy;
    // }

    // debug_print("ADC value: %d\n", value);

    // destroy:
    // iio_context_destroy(ctx);
    // end:
    end:
    return value;
}

//lED
ws2811_return_t ws2811_init(ws2811_t *ws2811){
    return 0;
}                                //< Initialize buffers/hardware
void ws2811_fini(ws2811_t *ws2811){

}                                          //< Tear it all down
ws2811_return_t ws2811_render(ws2811_t *ws2811){
    return 0;
}                              //< Send LEDs off to hardware
ws2811_return_t ws2811_wait(ws2811_t *ws2811){
    return 0;
}                                 //< Wait for DMA completion
const char * ws2811_get_return_t_str(const ws2811_return_t state){
    return "error";
}
