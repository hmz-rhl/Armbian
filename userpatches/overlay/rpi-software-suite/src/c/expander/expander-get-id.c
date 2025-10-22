/**
 * @file expander-get-gpio.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.3
 * @date 2022-06-20
 * // TODO : à optimiser HR
 * @copyright Copyright (c) 2022
 * 
 */


#include "../lib/rtc_eeprom.h"

int main(int argc, char* argv[]) {
	char ID[21];
	rtc_eeprom_t *rtc_eeprom = rtc_eeprom_init();

	unsigned int id = 0;
	unsigned int id2 = 0;

	for (size_t i = 0; i < 3; i++) {
		/* code */
		id = id + (eeprom_readProtected(rtc_eeprom, 0xF2 + i) << (8*i));
		//printf("id: %X \n", id);
	}

	for (size_t i = 0; i < 3; i++) {
		/* code */
		id2 = id2 + (eeprom_readProtected(rtc_eeprom, 0xF5 + i) << (8*i));
		//printf("id2: %X \n", id2);
	}
	
	char str_id1[10];
	char str_id2[10];

	sprintf(str_id1, "%6X", id);
	int i=0;
    while(str_id1[i]!='\0') {
        if(str_id1[i]==' ') {
            str_id1[i]='0';
        }
        i++;
    }
	sprintf(str_id2, "%6X", id2);
	i=0;
    while(str_id2[i]!='\0') {
        if(str_id2[i]==' ') {
            str_id2[i]='0';
        }
        i++;
    }
	printf("Chaine id1 = %s\n", str_id1);
	printf("Chaine id2 = %s\n", str_id2);
	strcpy(ID,str_id2);
	strcat(ID,str_id1);
	//correction des 3 valeurs chelou :/
	
	rtc_eeprom_closeAndFree(rtc_eeprom);
    rtc_eeprom_Free(rtc_eeprom);
    printf("Le RTC_eeprom est fermé\n");

    if (strlen(ID) >= 8) {
        // On teste l'existence du fichier id.conf
        int conf_exists = 0;
        FILE* file = fopen("/usr/share/hubload/notif/VALUE_ID_EVSE","r+");
        if (file != NULL) {
            conf_exists = 1;
            fclose(file);
        }

        if (conf_exists == 0) {
            FILE* file = fopen("/usr/share/hubload/notif/VALUE_ID_EVSE","w");
            if (file != NULL) {
                fprintf(file,"%s",ID);
                fclose(file);
            }
        }
    }
 
}