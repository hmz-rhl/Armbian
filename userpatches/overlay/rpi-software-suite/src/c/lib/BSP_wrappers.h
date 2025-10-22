/**
 * @file BSP_wrappers.h
 * @author	Hamza RAHAL (hamza.rahal@gmx.fr)
 * @brief wrappers pour acces aux ressources HW
 * @version 1.1.0
 * @date 2025-01-07
 * 
 * @copyright Copyright (c) 2025
 */

 #ifndef __BSP_WRAPPERS__
 #define __BSP_WRAPPERS__

#include <gpiod.h>
// #include <iio.h>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

typedef enum {
    GPIO_OUTPUT = 0,
    GPIO_INPUT,
    GPIO_DIR_N
} wrp_gpio_dir_t;

typedef enum {
    GPIO_LOW = 0,
    GPIO_HIGH,
    GPIO_VAL_N
} wrp_gpio_val_t;

typedef enum {
    GPIO_PULL_DISABLE = GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE,
    GPIO_PULL_UP = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP,
    GPIO_PULL_DOWN = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN,
    GPIO_BIAS_N
} wrp_gpio_bias_t;

typedef enum {
    GPIO_EDGE_FALLING = 0,
    GPIO_EDGE_RISING,
    GPIO_EDGE_BOTH,
}wrp_gpio_edge_t;

typedef void (*GpioCallback)(wrp_gpio_edge_t);

typedef struct {
    char *chipName;
    uint16_t pinNumber;
    char pinName[32];
    wrp_gpio_dir_t pinDirection;
    wrp_gpio_val_t pinValue;
    wrp_gpio_bias_t pinBias;
    GpioCallback pinCallback;
    struct gpiod_line *pinLine;
    wrp_gpio_edge_t pinEdge;
    int onInterrupt;

} wrp_gpio_t;

extern int WRP_init(void);


// TODO numero a mdifier
typedef enum {
    PIN_LOCK_P = 0, // = 0,					// Type2 lock motor on/off (Active low)
    // PIN_CP_PWM, // = 23,				// CP signal PWM
    // PIN_I2C_D, // = 8,					// I2C
    // PIN_I2C_C, // = 9,					// I2C
    PIN_SM_TIC_D, // = 2,				// S0 signal
    PIN_SM_TIC_D_BIS, // = 16,			// TIC signal
    PIN_USER_KEY, // = 3,				// User key (Active low)
    // PIN_MOSI, // = 12,					// SPI
    // PIN_MISO, // = 13,					// SPI
    // PIN_SCLK, // = 14,					// SPI
    PIN_PP_IN, // = 22,					// EV cable in (Active low)
    PIN_WD_TRIP, // = 24,				// WatchDog Type2 and E/F relays (Active low)
    PIN_CF4, // = 25,					// ADE interrupt
    PIN_RCD_TRIP_DC, // = 1,			// DC fault 6mA (Active low)
    PIN_RCD_TRIP_AC, // = 4,			// AC fault 20mA (Active low)
    PIN_IRQ1, // = 5,					// ADE interrupt
    PIN_IRQ0, // = 6,					// ADE interrupt
    PIN_LOCK_FB, // = 31,				// Type 2 lock feedback (Active low, MIDA product locked)
    PIN_DISPLAY_POWER, // = 30,			// Display BL on/off (active low)
    PIN_DISPLAY_REDUCE_POWER, // = 26,	// Reduced Display BL
    PIN_LED_STRIP_D, // = 29,			// ws2815 strip command line   
    
    //expander A
    PIN_TYPE2_NL1_ON,
    PIN_TYPE2_L2L3_ON,
    PIN_TYPE_EF_ON,
    PIN_LOCK_D,
    PIN_RCD_DIS,
    PIN_RCD_TST,
    PIN_RCD_TRIP_RESET,
    PIN_RCD_TRIP_AC_DIS,
    
    //expander B
    PIN_LED_DIS,
    PIN_CP_DIS,
    /* used by driver so just for info
    PIN_PP_CS,
    PIN_CP_CS,
    PIN_T_CS,
    PIN_PM_CS,
    */
    PIN_PM1,
    PIN_PM0,

    PIN_GPIO_N
} wrp_pin_gpio_e;

// GPIO
extern wrp_gpio_val_t WRP_readGpio(wrp_pin_gpio_e pin);
extern void WRP_writeGpio(wrp_pin_gpio_e pin, wrp_gpio_val_t value);
extern void WRP_setDirGpio(wrp_pin_gpio_e pin, wrp_gpio_dir_t direction);
extern void WRP_setPullModeGpio(wrp_pin_gpio_e pin, wrp_gpio_bias_t pull);
extern void WRP_interruptGpio(wrp_pin_gpio_e pin, wrp_gpio_edge_t edge,  void (*cb)(wrp_gpio_edge_t));
extern void WRP_removeInterruptGpio(wrp_pin_gpio_e pin);
// PWM


typedef enum {
    PWM_NORMAL = 0,
    PWM_INVERSED,
    PWM_POLARITY_N
} wrp_pwm_polarity_t;

typedef enum {
    PWM_DISABLE = 0,
    PWM_ENABLE,
    PWM_STATE_N
} wrp_pwm_state_t;

typedef struct{
    char                path[64];
    uint32_t            period_us; // us
    uint8_t             dutyCycle; // pourcent
    wrp_pwm_polarity_t  polarity;
    wrp_pwm_state_t     state;
} wrp_pwm_t;

extern void WRP_setClockPwm(uint32_t period_us);

extern void WRP_setPolarityPwm(wrp_pwm_polarity_t polarity);
extern void WRP_enablePwm(wrp_pwm_state_t state);
// pourcentage
extern void WRP_writePwm(uint8_t dutyCycle);

// ADC
typedef enum {
    ADC_TEMPERATURE = 0,
    ADC_PP,
    ADC_CP,
    ADC_N
} wrp_adc_t;

typedef struct {
    char        label[64];
    char        path[64];
    uint32_t    nbChannels;
} wrp_adcs_t;

extern uint16_t WRP_readAdc(wrp_adc_t adc, uint8_t channel);

//LED
typedef struct ws2811_t
{
    uint64_t render_wait_time;                   //< time in µs before the next render can run
    void *device;                //< Private data for driver use
    const void *rpi_hw;                      //< RPI Hardware Information
    uint32_t freq;                               //< Required output frequency
    int dmanum;                                  //< DMA number _not_ already in use
    void *channel;
} ws2811_t;
#define WS2811_RETURN_STATES(X)                                                             \
            X(0, WS2811_SUCCESS, "Success"),                                                \
            X(-1, WS2811_ERROR_GENERIC, "Generic failure"),                                 \
            X(-2, WS2811_ERROR_OUT_OF_MEMORY, "Out of memory"),                             \
            X(-3, WS2811_ERROR_HW_NOT_SUPPORTED, "Hardware revision is not supported"),     \
            X(-4, WS2811_ERROR_MEM_LOCK, "Memory lock failed"),                             \
            X(-5, WS2811_ERROR_MMAP, "mmap() failed"),                                      \
            X(-6, WS2811_ERROR_MAP_REGISTERS, "Unable to map registers into userspace"),    \
            X(-7, WS2811_ERROR_GPIO_INIT, "Unable to initialize GPIO"),                     \
            X(-8, WS2811_ERROR_PWM_SETUP, "Unable to initialize PWM"),                      \
            X(-9, WS2811_ERROR_MAILBOX_DEVICE, "Failed to create mailbox device"),          \
            X(-10, WS2811_ERROR_DMA, "DMA error"),                                          \
            X(-11, WS2811_ERROR_ILLEGAL_GPIO, "Selected GPIO not possible"),                \
            X(-12, WS2811_ERROR_PCM_SETUP, "Unable to initialize PCM"),                     \
            X(-13, WS2811_ERROR_SPI_SETUP, "Unable to initialize SPI"),                     \
            X(-14, WS2811_ERROR_SPI_TRANSFER, "SPI transfer error")                         \

#define WS2811_RETURN_STATES_ENUM(state, name, str) name = state
#define WS2811_RETURN_STATES_STRING(state, name, str) str

typedef enum {
    WS2811_RETURN_STATES(WS2811_RETURN_STATES_ENUM),

    WS2811_RETURN_STATE_COUNT
} ws2811_return_t;

ws2811_return_t ws2811_init(ws2811_t *ws2811);                               //< Initialize buffers/hardware
void ws2811_fini(ws2811_t *ws2811);                                             //< Tear it all down
ws2811_return_t ws2811_render(ws2811_t *ws2811);                                //< Send LEDs off to hardware
ws2811_return_t ws2811_wait(ws2811_t *ws2811);                                  //< Wait for DMA completion
const char * ws2811_get_return_t_str(const ws2811_return_t state);  


#endif //__BSP_WRAPPERS__