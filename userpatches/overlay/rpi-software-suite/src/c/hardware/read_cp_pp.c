#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "../lib/hubtypes.h"

#include "../lib/BSP_wrappers.h"


int main(int argc, char *argv[]) {
	
	int CP, PP;
	int readValue, oldValue;
	
    int toModify = 0;

    if(1 < 0) {
		fprintf(stderr, "fonction %s: Unable to init WRP: %s\n", __func__, strerror(errno));
		exit(EXIT_FAILURE);
	}
   	WRP_setDirGpio(PIN_PP_IN, GPIO_INPUT);					// EV cable in (Active low)
	WRP_setPullModeGpio(PIN_PP_IN, GPIO_PULL_DISABLE);

// Lecture du dernier PP pour comparaison
    FILE* filePp = fopen("/usr/share/hubload/notif/VALUE_PP","r");
    if (filePp != NULL) {
        fscanf(filePp,"%d",&oldValue);
        fclose(filePp);
    } else oldValue = -1; 		// initialisation a une valeur différente des cas réels

    int cablePlugged = (WRP_readGpio(PIN_PP_IN)?OFF:ON);
    printf("cablePlugged : %d\n", cablePlugged);
	if (!cablePlugged) {					// Si le cable est absent,
		WRP_writeGpio(PIN_TYPE2_L2L3_ON, GPIO_LOW);	// couper la tension sur la Type2 le plus tôt possible
		WRP_writeGpio(PIN_TYPE2_NL1_ON, GPIO_LOW);	// on termine toujours par le Neutre
		PP = 0;					// Cable non connecté, pas de courant possible
		CP = 12;				// Voiture non connectée, pas d'erreur
        if ( oldValue != PP ) {	// Le cable vient d'être déconnecté ou le fichier est encore inexistant,
            toModify = 1;		// demande d'écriture dans le fichier de sauvegarde 
        }
	   	printf("Cable non présent, PP = %d et CP = %d\n", PP, CP);
	} else {
        // on s'assure que tous les Chip Select sont à 1
        // WRP_writeGpio(27 --pin 2 1");	// PP_CS
        // WRP_writeGpio(27 --pin 3 1");	// CP_CS
        // WRP_writeGpio(27 --pin 4 1");	// T_CS
        // WRP_writeGpio(27 --pin 5 1");	// PM_CS

		if (oldValue <= 0) {	// Un cable est connecté, lecture du PP au premier passage seulement
		    readValue = WRP_readAdc(0,PP_CS);           // Attention, PIN_PP_IN remonte à 1 le temps de la lecture
            for (int cnt = 0; cnt < 7; cnt++) readValue += WRP_readAdc(ADC_PP, 0);
            readValue = readValue >> 3;
            if (readValue > 3247 )       PP = 0;    // plus de 1800 ohms 
            else if ( readValue > 3187 ) PP = 6;    // plus de 1650 ohms (norme > 1500 ohms)
            else if ( readValue > 2861 ) PP = 13;   // plus de 1090 ohms (norme = 1500 ohms)
            else if ( readValue > 2003 ) PP = 20;   // plus de 450 ohms (norme = 680 ohms)
            else if ( readValue > 1040 ) PP = 32;   // plus de 160 ohms (norme = 220 ohms)
            else if ( readValue > 894 )  PP = 63;   // plus de 50 ohms (norme = 100 ohms)
            else                         PP = 80;    // moins de 50 ohms
			
	    	printf("PP lu = %d, PP = %d ", readValue, PP);
            toModify = 1;	// Demande d'écriture dans le fichier de sauvegarde
		}

		// lecture du CP systématique si un cable est connecté
		readValue = WRP_readAdc(ADC_CP, 0);
        for (int cnt = 0; cnt < 7; cnt++) readValue += WRP_readAdc(ADC_CP, 0);
        readValue = readValue >> 3;
		if (readValue > 3257 )       CP = 12;    // supérieur à 10.5V (norme 12V)
		else if ( readValue > 2327 ) CP = 9;	 // supérieur à 7.5V (norme 9V)
		else if ( readValue > 1396 ) CP = 6;	 // supérieur à 4.5V (norme 6V)
		else if ( readValue > 465 )  CP = 3;	 // supérieur à 1.5V (norme 3V)
		else                         CP = 0;     // Attention, 0V et -12V ne sont pas distingués

		// ----------------------- GESTION DES ERREURS A PRÉVOIR ----------------------- //

	    printf("CP lu = %d, CP = %d\n", readValue, CP);
    }

// Écriture du PP sur demande
    if (toModify > 0) {
        filePp = fopen("/usr/share/hubload/notif/VALUE_PP","w");
        if (filePp != NULL) {
            fprintf(filePp,"%d",PP);
            fclose(filePp);
        }
        toModify = 0;	// reset pour le test CP suivant
    }

// Lecture du CP dans son fichier et comparaison s'il existe, demande de creation sinon
    FILE* fileCp = fopen("/usr/share/hubload/notif/VALUE_CP","r");
    if (fileCp != NULL) {
        fscanf(fileCp,"%d",&oldValue);
        fclose(fileCp);
        if (oldValue != CP) {
            toModify = 1;
        }
    }
    else toModify = 1;

// Écriture du CP sur demande
    if (toModify > 0) {
        fileCp = fopen("/usr/share/hubload/notif/VALUE_CP","w");
        if (fileCp != NULL) {
            fprintf(fileCp,"%d",CP);
            fclose(fileCp);
        }
    }

    return EXIT_SUCCESS;
}