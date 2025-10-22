
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../lib/hubtypes.h"
#include <time.h>
#include <sys/time.h>

#include "../lib/pn532.h"
#include "../lib/PN532_Rpi_I2C.h"

char str_scan[20];

int main(int argc, char *argv[]) {
	uint8_t buff[255];
    uint8_t uid[MIFARE_UID_MAX_LENGTH];
    int32_t uid_len = 0;
    PN532 pn532;

    while(PN532_I2C_Init(&pn532) < 0){
        usleep(100000);
    }
    while(PN532_GetFirmwareVersion(&pn532, buff) != PN532_STATUS_OK) {
        usleep(100000);
    }
    //printf("Found PN532 with firmware version: %d.%d\r\n", buff[1], buff[2]);
    PN532_SamConfiguration(&pn532);

    for (int i = 0; i < 10; i++) {
        // Check if a card is available to read
        uid_len = PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 1000);
        if (uid_len == PN532_STATUS_ERROR) {
            fflush(stdout);
        }
        else {
            //printf("\n Found card with UID: ");
    
            char message[256];
            printf("%02x%02x%02x%02x%02x%02x%02x%02x \n",uid[0],uid[1],uid[2],uid[3],uid[4],uid[5],uid[6],uid[7]);
            printf("%02x%02x%02x%02x%02x%02x%02x%02x \n",uid[7],uid[6],uid[5],uid[4],uid[3],uid[2],uid[1],uid[0]);
            
            sprintf(message, "%02x%02x%02x%02x%02x%02x%02x%02x", uid[7],uid[6],uid[5],uid[4],uid[3],uid[2],uid[1],uid[0]);
            for (i = 0; message[i]!='\0'; i++) {
                /* si les caractères sont en minuscules, convertissez-les
                    en majuscules en soustrayant 32 de leur valeur ASCII. */
                if(message[i] >= 'a' && message[i] <= 'z') {
                    message[i] = message[i] -32;
                }
            }
            usleep(100000);

            // Ecriture dans le fichier FILE* file = fopen("/usr/share/hubload/SCAN","r");
            printf("On écrit %s dans le fichier %s\n", message, "/usr/share/hubload/SCAN");
            FILE* file = fopen("/usr/share/hubload/notif/VALUE_SCAN","w+");
            if (file != NULL) {
                fprintf(file,"%s",message);
                fclose(file);
            }
            else {
                printf("file NULL\n");
            }
            usleep(100000);
            break;
        }
        usleep(500000);
    }

    PN532_I2C_Close();

    return EXIT_SUCCESS;
}

