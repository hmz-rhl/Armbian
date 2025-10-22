/**
 * @file expander-set-gpio.c
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
    if (argc ==2 && !(!strcmp(argv[1], "-h") || !strcmp(argv[1],"--help"))) {
        printf("Usage: expander-add-wh [-h]\n");
        return EXIT_SUCCESS;
    }
    else if(argc != 1){
    
        printf("Usage: expander-add-wh [-h] \n");
        return EXIT_SUCCESS;

    }

    rtc_eeprom_t *rtc_eeprom = rtc_eeprom_init();

    uint8_t val_F0 = eeprom_readProtected(rtc_eeprom,0xF0);
    uint8_t val_F1 = eeprom_readProtected(rtc_eeprom,0xF1);

    // TODO : gestion des erreurs, s'il y a un pb de lecture de l'EEPROM
    //printf("val_F0 = %d, val_F1 = %d \n", val_F0, val_F1);
    uint16_t result = ((val_F1 << 8) | val_F0) + 1;
    //printf("compteur = %d \n", result);

    if(val_F0 == 0xFF) {
        eeprom_writeProtected(rtc_eeprom, 0xF0, 0x00);       
        if(val_F1 == 0xFF) {
            eeprom_writeProtected(rtc_eeprom, 0xF1, 0x00);
        }
        else {            
            eeprom_writeProtected(rtc_eeprom, 0xF1, val_F1 + 1);
        }
    }    
    else {        
        eeprom_writeProtected(rtc_eeprom, 0xF0, val_F0 + 1);
    }
    rtc_eeprom_closeAndFree(rtc_eeprom);
    rtc_eeprom_Free(rtc_eeprom);

    // Ecriture dans le fichier FILE* file = fopen("/usr/share/hubload/id.conf","r");
    //printf("On écrit %d dans le fichier %s\n", result, "/usr/share/hubload/notif/VALUE_COMPTEUR");
    
    // TODO : sécuriser l'écriture de la valeur dans le fichier
    /*FILE* file = fopen("/usr/share/hubload/notif/VALUE_COMPTEUR","w");
    if (file != NULL) {
        fprintf(file,"%d",result);
        fclose(file);
    }
    else {
        printf("file NULL\n");
    }*/

    return EXIT_SUCCESS;
}