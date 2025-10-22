#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "../lib/hubtypes.h"

#include "../lib/BSP_wrappers.h"

char str_temp[32];

/**
 * @brief  fonction qui passe de volt a degre pour le composant TMP36
 * 
 * @param  tension tension a convertir
 **/
static double toDegres(int tension) {
	if(tension < 0){
		printf("%s: ne peut pas convertir une tension negative en degres\n", __func__);
		return tension;
	}
	return (tension-500)/10.0;
}

int main(int argc, char *argv[]) {
    double temp = toDegres(WRP_readAdc(ADC_TEMPERATURE, 0));
    sprintf(str_temp, "%.2f", temp);
    printf("%.2f deg\n", temp);

    FILE* fileTemp = fopen("/usr/share/hubload/notif/VALUE_TEMP","w");
    if (fileTemp != NULL) {
        fprintf(fileTemp,"%s",str_temp);
        fclose(fileTemp);
    }

    return EXIT_SUCCESS;
}