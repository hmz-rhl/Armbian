#include "lib/BSP_wrappers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MASK_ADC    (1<<0)
#define MASK_RELAY   (1<<1)
#define MASK_PWM    (1<<2)
#define MASK_INT    (1<<3)

#define ONE_KHZ_PERIOD_US   1000000U

static int onInterrupt = 0;

void myflush ( FILE *in )

{
  int ch;

  do
    ch = fgetc ( in ); 
  while ( ch != EOF && ch != '\n' ); 

  clearerr ( in );
}

void mypause (void) 
{ 
//   printf ( "%s", msg );
  fflush ( stdout );
  getchar();
} 

void PP_IN_interrupt(wrp_gpio_edge_t edge){
    printf("%s edge \n", edge == GPIO_EDGE_FALLING ? "falling" : "rising");
    return;
}

extern int optind, optopt, opterr;

int main(int argc, char **argv){
    char key[1];
    uint8_t maskTest = 0;
    
    int c;
    char *filename;
    extern char *optarg;
    
    uint16_t readValue = 0;
    uint8_t PP,CP;

    while ((c = getopt(argc, argv, ":arpi")) != -1) {
        switch(c) {
        case 'a':
            maskTest |= MASK_ADC;
            break;
        case 'r':
            maskTest |= MASK_RELAY;
            break;
        case 'p':
            maskTest |= MASK_PWM;
            break;
        case 'i':
            maskTest |= MASK_INT;
            break;
        case '?':
            printf("unknown arg %c\n", optopt);
            break;
        }
    }

    printf("________BSP_TEST________\n");
    WRP_init();
    
    WRP_setDirGpio(PIN_CP_DIS, GPIO_OUTPUT);
    WRP_setDirGpio(PIN_PP_IN, GPIO_INPUT);
    WRP_setDirGpio(PIN_RCD_DIS, GPIO_OUTPUT);
    WRP_setDirGpio(PIN_TYPE2_L2L3_ON, GPIO_OUTPUT);
    WRP_setDirGpio(PIN_TYPE2_NL1_ON, GPIO_OUTPUT);
    WRP_setDirGpio(PIN_LOCK_FB, GPIO_INPUT);
    
    printf("LOCK_FB = %u\n", WRP_readGpio(PIN_LOCK_FB));

/*PWM*/
    if(maskTest & MASK_PWM){
        printf("PWM:\n");
        printf("setting pwm clock to %d\n", ONE_KHZ_PERIOD_US);
        WRP_setClockPwm(ONE_KHZ_PERIOD_US );
        printf("setting polarity to normal\n");
        WRP_setPolarityPwm(PWM_NORMAL);

        printf("setting dutycyle to 90%\n");
        WRP_writePwm(90);

        printf("enabling pwm signal\n");
        WRP_enablePwm(PWM_ENABLE);
        puts("Press any key to continue to next test (relay)...");
        myflush(stdin);
        mypause();
        printf("disabling pwm signal\n");
        WRP_enablePwm(PWM_DISABLE);
    }
/*RELAY*/
    if(maskTest & MASK_RELAY){
        printf("RELAYs:\n");
        printf("(disabling RCD)\n");
        WRP_writeGpio(PIN_RCD_DIS, GPIO_LOW);
        printf("relay (TYPE2 L2/L3)\n");
        printf("switch ON relay TYPE2_L2L3:\n");
        WRP_writeGpio(PIN_TYPE2_L2L3_ON, GPIO_HIGH);

        printf("wait for 5 secs:\n");
        sleep(5);

        printf("switch OFF relay TYPE2_L2L3:\n");
        WRP_writeGpio(PIN_TYPE2_L2L3_ON, GPIO_LOW);

        printf("next relay (TYPE2 N/L1)\n");
        printf("switch ON relay TYPE2_NL1:\n");
        WRP_writeGpio(PIN_TYPE2_NL1_ON, GPIO_HIGH);
        
        printf("wait for 5 secs:\n");
        sleep(5);

        printf("switch OFF relay TYPE2_NL1:\n");
        WRP_writeGpio(PIN_TYPE2_NL1_ON, GPIO_LOW);

        printf("next relay (TYPE EF)\n");
        printf("switch ON relay TYPE_EF:\n");
        WRP_writeGpio(PIN_TYPE_EF_ON, GPIO_HIGH);
        
        printf("wait for 5 secs:\n");
        sleep(5);

        printf("switch OFF relay TYPE_EF:\n");
        WRP_writeGpio(PIN_TYPE_EF_ON, GPIO_LOW);

        puts("Press any key to continue to next test (ADCs)...");
        myflush(stdin);
        mypause();
        printf("(enabling RCD)\n");
        WRP_writeGpio(PIN_RCD_DIS, GPIO_HIGH);
    }
/*ADC*/
    if(maskTest & MASK_ADC){
        printf("ADCs:");
        printf("TEMP adc = %u\n", WRP_readAdc(ADC_TEMPERATURE, 0));
        
        printf("(pwm clock to %d)\n", ONE_KHZ_PERIOD_US);
        WRP_setClockPwm(ONE_KHZ_PERIOD_US );
        printf("(pwm polarity to normal)\n");
        WRP_setPolarityPwm(PWM_NORMAL);
        printf("(CP_DIS# = 1)\n");
        WRP_writeGpio(PIN_CP_DIS, GPIO_HIGH);
        printf("(PWM = 100%%)\n");
        WRP_writePwm(100);
        WRP_enablePwm(PWM_ENABLE);
        
        readValue = WRP_readAdc(ADC_CP, 0);
        for (int cnt = 0; cnt < 7; cnt++) readValue += WRP_readAdc(ADC_CP, 0);
        readValue = readValue >> 3;
		if (readValue > 3257 )       CP = 12;    // supérieur à 10.5V (norme 12V)
		else if ( readValue > 2327 ) CP = 9;	 // supérieur à 7.5V (norme 9V)
		else if ( readValue > 1396 ) CP = 6;	 // supérieur à 4.5V (norme 6V)
		else if ( readValue > 465 )  CP = 3;	 // supérieur à 1.5V (norme 3V)
		else                         CP = 0;     // Attention, 0V et -12V ne sont pas distingués
        printf("CP adc = %u(%u)\n", readValue, CP);
        printf("(disable PWM)\n");
        WRP_writeGpio(PIN_CP_DIS, GPIO_LOW);
        WRP_enablePwm(PWM_DISABLE);
        
        readValue = WRP_readAdc(ADC_PP, 0);
        for (int cnt = 0; cnt < 7; cnt++) readValue += WRP_readAdc(ADC_PP, 0);
        readValue = readValue >> 3;
        if (readValue > 4100 )       PP = 0;    // plus de 1800 ohms 
        else if ( readValue > 3617 ) PP = 6;    // plus de 1650 ohms (norme > 1500 ohms)
        else if ( readValue > 2544 ) PP = 13;   // plus de 1090 ohms (norme = 1500 ohms)
        else if ( readValue > 1710 ) PP = 20;   // plus de 450 ohms (norme = 680 ohms)
        else if ( readValue > 1040 ) PP = 32;   // plus de 160 ohms (norme = 220 ohms)
        else if ( readValue > 750 )  PP = 63;   // plus de 50 ohms (norme = 100 ohms)
        else                         PP = 80;    // moins de 50 ohms
        printf("PP adc = %u(%u)\n", readValue, PP);
    }
/*INT*/
    if(maskTest & MASK_INT){

        puts("Press any key to continue to next test (interrupt PP_IN)...");
        myflush(stdin);
        mypause();
        printf("interrupt PP_IN(rinsing):\n");
        WRP_interruptGpio(PIN_PP_IN, GPIO_EDGE_BOTH, &PP_IN_interrupt);
        
        puts("Press any key to end (interrupt PP_IN)...");
        myflush(stdin);
        WRP_removeInterruptGpio(PIN_PP_IN);
        mypause();
    }

    return 0;

    // WRP_setDirGpio(PIN_CP_DIS, GPIO_OUTPUT);
    // WRP_setDirGpio(PIN_PP_IN, GPIO_INPUT);
    // WRP_setDirGpio(PIN_RCD_TRIP_AC_DIS, GPIO_OUTPUT);
    // WRP_setDirGpio(PIN_TYPE2_L2L3_ON, GPIO_OUTPUT);
    // WRP_setDirGpio(PIN_TYPE2_NL1_ON, GPIO_OUTPUT);

    // WRP_writeGpio(PIN_CP_DIS, GPIO_HIGH);
    // WRP_writeGpio(PIN_RCD_TRIP_AC_DIS, GPIO_HIGH);
    // WRP_writeGpio(PIN_TYPE2_L2L3_ON, GPIO_LOW);
    // WRP_writeGpio(PIN_TYPE2_NL1_ON, GPIO_LOW);

    return 0;

}