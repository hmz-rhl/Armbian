/**
 * @file expander-get-gpio.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.3
 * @date 2022-06-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../lib/rtc_eeprom.h"

int main(int argc, char* argv[]) {
    rtc_eeprom_t *rtc_eeprom = rtc_eeprom_init();
    uint8_t val1 = eeprom_readProtected (rtc_eeprom, 0xF0);
    uint8_t val2 = eeprom_readProtected (rtc_eeprom, 0xF1);

    // TODO : gestion des erreurs, s'il y a un pb de lecture de l'EEPROM
    
    uint16_t result = ((val2 << 8) | val1);
	
	rtc_eeprom_closeAndFree(rtc_eeprom);
    rtc_eeprom_Free(rtc_eeprom);
    printf("Le RTC_eeprom est fermé\n");

    if (result > 0) {
        FILE* file = fopen("/usr/share/hubload/notif/VALUE_COMPTEUR","w+");
        if (file != NULL) {
            fprintf(file,"%d",result);
            fclose(file);
        }
    }
 
    return EXIT_SUCCESS;
}