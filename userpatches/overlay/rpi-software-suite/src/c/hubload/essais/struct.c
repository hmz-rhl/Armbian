#include <stdio.h>
#include "struct.h"

Hubleds *initLeds(char *initConfig) {
    printf("MagicNumber = %d\n", NOMBRE_MAGIQUE);
	Hubleds *leds = (Hubleds*)malloc(sizeof(Hubleds));

    strcpy(leds->ledsConfig, initConfig);
    leds->color = GREEN;
    leds->level = HIGH;
    leds->scheme = CONSTANT;
    leds->level_veille = MAX;
    leds->blOn = 1;
    leds->nb_led = 10;

	FILE* file = fopen("/usr/share/hubload/nb_led","r");
	if(file == NULL){
		printf("Impossible d'ouvrir le fichier leds\n");
	}
	else{
		if(fscanf(file,"%d",&(leds->nb_led)) == -1){		
			printf("nb_leds = %d\n", leds->nb_led);
		}
		fclose(file);
	}
	printf("thread_led : nb_led = %d\n", leds->nb_led);
	//unsigned char c[3];
	//unsigned char WheelPos;
    return leds;

}