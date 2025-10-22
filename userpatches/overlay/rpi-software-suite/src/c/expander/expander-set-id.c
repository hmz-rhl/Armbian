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
    char ID[13];
    if(argc != 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1],"--help")){
    
        printf("Usage: expander-set-id <id_borne> \n");
        printf("exemple: expander-set-id AA23BB34CC56 \n");
        return EXIT_SUCCESS;

    }

    strcpy(ID, argv[1]);
	rtc_eeprom_t *rtc_eeprom = rtc_eeprom_init();

	// on reset le compteur d'energie
    // HRA: pourquoi ?
	eeprom_writeProtected(rtc_eeprom, 0xF0, 0x00);
	eeprom_writeProtected(rtc_eeprom, 0xF1, 0x00);
    // HRA: pourquoi id2 ?
	char id2[15] = "0x";
    strcat(id2,ID);
    // HRA: long quoi ?
    long value = strtol( id2,NULL, 16 );

	// on ecrit l'id
	for (size_t i = 0; i < 6; i++) {
		/* code */
		eeprom_writeProtected(rtc_eeprom, 0xF2 + i, (value >> 8*i) & 0xFF);
	}

	eeprom_printProtected(rtc_eeprom);
	rtc_eeprom_closeAndFree(rtc_eeprom);

    
    printf("id : %s\n", ID);

    // Ecriture dans le fichier FILE* file = fopen("/usr/share/hubload/id.conf","r");
    FILE* file = fopen("/usr/share/hubload/notif/VALUE_ID_EVSE","w+");
    if (file != NULL) {
        fprintf(file,"%s",ID);
        fclose(file);
    }

    return EXIT_SUCCESS;
}