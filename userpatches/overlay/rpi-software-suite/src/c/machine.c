/**
 * @file machine.c
  * @author	Gilles VIGUIE (gilles.viguie@jerecharge.com)
 *			Frederic BOMPARD (frederic.bompard@jerecharge.com)
 * @brief Gestion de la machine d'états de la borne v3
 * @version 3.3
 * @date 2024-12-15
 * 
 * @copyright Copyright (c) 2024
 *
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 #include <sys/time.h>
 #include <signal.h>
 #include <math.h>
 #include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>

#include "lib/hubfiles.h"
#include "lib/hubtypes.h"
#include "machine.h"

#include "lib/BSP_wrappers.h"
int mainLoop = true;
int reLaunch = true;				// Permet un redémarrage en cas de coupure secteur en charge, sinon Finishing

EtatMachine *machine = NULL;
LaunchValues *launch = NULL;

int max_current;					// Valeur max de courant demandée par OCPP
int conf_max_current = 32;			// Capacité max en courant de la borne
int mode_phase;						// Configuration courante demandée par OCPP
int conf_mode_phase = TRI;			// Capacité MONO / TRIPHASÉ
int voltageNl = 230;				// Tension monophasée du secteur
int stdVoltage = ON;				// Tension mesurée ou standard (110/115/120/127/220/230/240)
int backlight_on = GPIO_LOW;				// Écran allumé ou eteint

volatile int isCablePlugged = 0;

// État de la prise type 2
int type2_closed = GPIO_LOW;
int cablelock_mode = GPIO_LOW;			// 0 : auto; 1 : always locked; 2 : always unlocked;
int cablelock_feedback_order = 0;	// 0 : normal; 1 : inversé;

// État de la prise EF
int EF_closed = GPIO_LOW;

// Comptage d'energie S0
int s0_activated = GPIO_LOW;
int s0_nb_pulse_per_kwh = 800;
struct timeval startS0, endS0;
double deltaT_S0, compteurS0;
// Valeurs d'énergie, puissance, courant
long oldCompteur, compteur, power;
double current;

// Pour la gestion de la coupure de charge, avec le bouton
int keyPressed = GPIO_LOW;			// Activation du bouton, le relaché est géré dans la boucle principale
int stopCharge = GPIO_LOW;			// Fin de charge demandée par le bouton, remis à 0 si le véhicule est débranché

// Mesures du CP et PP
int PP, CP;
char type2Status = 'A';			// A, B, C, D ou E (Etat F accessible au niveau de la voiture)
int cpDisableToggle = GPIO_LOW;		// Pour la désactivation du CP si le mode PREPARING_4 est trop long
int enableCp = GPIO_LOW;				// Pour distinguer une erreur CP=0 d'un CP inactif
int dutyCycle = 0;

// Gestion des erreurs
int activatedRcd = GPIO_LOW;			// Activation de la protection sur erreur RCD
int rcdWarning = GPIO_LOW;			// Affichage du message de maintenance sur error RCD
int lockWarning = ON;			// Affichage du message de maintenance sur erreur de verrouillage
int veErr = 0;					// Erreur connexion véhicule
int pubErr = GPIO_LOW;				// L'erreur est publiée ou non
int lockErr = 0;				// Erreur de verrouillage
int rstTrials = 11;				// 0: Always; 1: Never; N: Nb of trials - 1;
int trials = 0;					// Nombre d'essais de (de)verrouillage

/*buffer to store the data of events*/
int fd,wd;
char active[25];

// Valeurs à envoyer sur MQTT
char str_temp[100];

// Gestion des delais
struct timeval startScreen, startSend, startKey, startNewState, startEF, timeNow;
time_t delayKey, delayState;
suseconds_t udelayKey;
int previousState;
int	timeoutFinishingSuspendedEV = GPIO_LOW;
int eStation = GPIO_LOW;				// Vrai pour interdire la charge programmée plus tard et pouvoir facturer le temps de post-stationnement

#define TIMEOUT_P4 29			// Timeout (-1 sec) du mode PREPARING_4 
#define TIMEOUT_SEV 1799		// Timeout du mode SuspendedEV (30 min)

#define TIMEOUT_AUTH 29			// Authentification
#define TIMEOUT_SCREEN 32		// extinction écran
#define TIMEOUT_SEND 1			// publication rapide (2 sec)
#define TIMEOUT_SEND_MED 4		// publication (5 sec)
#define TIMEOUT_SEND_LOW 59		// publication lente (1 min)
#define TIMEOUT_STATE 59		// État stable (1 min)
#define TIMEOUT_STATE_DEEP 119	// État stable (2 min)
#define TIMEOUT_REBOOT 19		// redémarrage (20 sec)
#define TIMEOUT_KEYFALSE   10000	// pour filtrer les fausse détections (0,01 sec)
#define TIMEOUT_KEYPRESSED 400000	// pour valider un appui (0,4 sec)
#define TIMEOUT_EF_ON 10799		// Etat ON de la prise optionelle latérale (3 heures)


static void rebootBorne() {
	// On stoppe une charge éventuellement en cours
	if ((machine->state > PREPARING_4) && (machine->state < Finishing)) {
		machine->state = Finishing;	// Si un véhicule était en charge, on termine la transaction
		setState();					// On positionne et on publie
		sleep(4);					// Attente pour que l'info remonte sur le serveur OCPP
	}

	// On confirme l'ouverture des relais
	openRelaisType2(10);
	switchRelaisPriseEf(GPIO_LOW);

	//  On force le déverrouillage au cas où
	openCableLock(ON, 9);

	// On supprime le fichier de statut
	remove("/usr/share/hubload/launch");

    system("reboot now");
}

static void nettoyage_machine() {
	// On supprime le fichier de statut
	remove("/usr/share/hubload/launch");

	free(launch);
	free(machine);
	/* Step 5. Remove the watch descriptor and close the inotify instance*/
	inotify_rm_watch( fd, wd );
	close( fd );
}

static void stop_signal() {
	printf("On stoppe les threads");
	mainLoop = GPIO_LOW;
}


static void initPinValues(void) {
	printf("initPinValues (hardware ARM), on enregistre les valeurs sur les GPIO\n");
	
	// LED strip control
	WRP_setDirGpio(PIN_LED_STRIP_D, GPIO_OUTPUT);

	// Display controls
	// WRP_setDirGpio(PIN_DISPLAY_POWER, GPIO_OUTPUT);
	// WRP_writeGpio(PIN_DISPLAY_POWER, !GPIO_HIGH);
	// WRP_setDirGpio(PIN_DISPLAY_REDUCE_POWER, GPIO_OUTPUT);
	// WRP_writeGpio(PIN_DISPLAY_REDUCE_POWER, !GPIO_LOW);

	// RCD inputs
	WRP_setDirGpio(PIN_RCD_TRIP_DC, GPIO_INPUT);
	WRP_setPullModeGpio(PIN_RCD_TRIP_DC, GPIO_PULL_DISABLE);

	WRP_setDirGpio(PIN_RCD_TRIP_AC, GPIO_INPUT);
	WRP_setPullModeGpio(PIN_RCD_TRIP_AC, GPIO_PULL_DISABLE);

	// User key
	WRP_setDirGpio(PIN_USER_KEY, GPIO_INPUT);
	WRP_setPullModeGpio(PIN_USER_KEY, GPIO_PULL_DISABLE);
	WRP_interruptGpio (PIN_USER_KEY, GPIO_EDGE_FALLING,  &user_key_interrupt) ;
	// WRP_interruptGpio (PIN_USER_KEY, GPIO_EDGE_RISING,  &user_key_released_interrupt) ;
	// WRP_interruptGpio (PIN_USER_KEY, GPIO_EDGE_BOTH,  &user_key_interrupt) ;

	// External measurement inputs
	WRP_setDirGpio(PIN_SM_TIC_D_BIS, GPIO_INPUT);
	WRP_setPullModeGpio(PIN_SM_TIC_D_BIS, GPIO_PULL_DISABLE);
	WRP_setDirGpio(PIN_SM_TIC_D, GPIO_INPUT);
	WRP_setPullModeGpio(PIN_SM_TIC_D, GPIO_PULL_DISABLE);
//	L'interuption S0 sera autorisée plus loin si nécessaire
//	WRP_interruptGpio (PIN_SM_TIC_D, GPIO_EDGE_FALLING,  &S0_interrupt);

	// ADE IRQ requests
	WRP_setDirGpio(PIN_CF4, GPIO_INPUT);
	WRP_setPullModeGpio(PIN_CF4, GPIO_PULL_DOWN);
	WRP_setDirGpio(PIN_IRQ1, GPIO_INPUT);
	WRP_setPullModeGpio(PIN_IRQ1, GPIO_PULL_DOWN);
	WRP_setDirGpio(PIN_IRQ0, GPIO_INPUT);
	WRP_setPullModeGpio(PIN_IRQ0, GPIO_PULL_DOWN);

	// Lock motor control
	WRP_setDirGpio(PIN_LOCK_P, GPIO_OUTPUT);	// Power
	WRP_setPullModeGpio(PIN_LOCK_P, GPIO_PULL_DISABLE);
	WRP_writeGpio(PIN_LOCK_P, !GPIO_LOW);

	WRP_setDirGpio(PIN_LOCK_FB, GPIO_INPUT);	// Feedback
	WRP_setPullModeGpio(PIN_LOCK_FB, GPIO_PULL_DISABLE);

	// Watch Dog (actif bas)
	WRP_setDirGpio(PIN_WD_TRIP, GPIO_OUTPUT);
	WRP_setPullModeGpio(PIN_WD_TRIP, GPIO_PULL_DISABLE);
	WRP_writeGpio(PIN_WD_TRIP, !GPIO_LOW);

	// PP signal
	WRP_setDirGpio(PIN_PP_IN, GPIO_INPUT);
	WRP_setPullModeGpio(PIN_PP_IN, GPIO_PULL_DISABLE);
	WRP_interruptGpio (PIN_PP_IN, GPIO_EDGE_BOTH,  &pp_interrupt) ;

	// CP signal
	WRP_setClockPwm (1000000);
	WRP_enablePwm(PWM_ENABLE);
	WRP_setPolarityPwm(PWM_NORMAL);
	WRP_writePwm(100);

	//sleep(1);

	printf("Création des expanders pour les autres GPIO\n");
// l'expander 0x26: detecteur de courant de fuite et controle des relais de sortie

//#define RCD_TRIP_AC_DIS   7   // Réarmenent du détecteur de courant de fuite AC (active low)
//#define RCD_TRIP_DC_RESET 6	// Réarmement du détecteur de courant de fuite (Active low)
//#define RCD_TST         5		// Test du détecteur de courant de fuite (Active low)
//#define RCD_DIS         4		// Désactivation du détecteur de courant de fuite AC (Active low)
//#define LOCK_D          3     // Direction du moteur de verrouillage
//#define TYPE_E_F_ON     2		// Prise latérale on/off (Active high)
//#define TYPE_2_L2L3_ON  1		// Prise type 2 L2/L3 on/off (Active high)
//#define TYPE_2_NL1_ON   0		// Prise type 2 N/L1 on/off (Active high)

//l'expander 0x27: controle de l'ADE, CS des composants périphériques sur le SPI et activation du CP en sortie

//#define PM0             7     // Pin pour controler le mode de fonctionnement de l'ADE
//#define PM1             6     // Pin pour controler le mode de fonctionnement de l'ADE
//#define PM_CS           5     // Chip Select de l'ADE (Active low)
//#define T_CS            4     // CS du convertisseur de temperature (Active low)
//#define CP_CS           3		// CS de lecture du signal CP (Active low)
//#define PP_CS           2		// CS de lecture du signal PP (Active low)
//#define PIN_CP_DIS      1		// Desactivation de la sortie CP (Active low)
//#define Unused          0		// Unused

	// system("expander-set-gpio 27 0 0 1 1 1 1 1 1");
	// system("expander-set-gpio 26 1 1 1 1 0 0 0 0");
	
	WRP_setDirGpio(PIN_RCD_TRIP_AC_DIS, GPIO_OUTPUT);
	WRP_writeGpio(PIN_RCD_TRIP_AC_DIS, GPIO_HIGH);
	
	WRP_setDirGpio(PIN_RCD_TRIP_RESET, GPIO_OUTPUT);
	WRP_writeGpio(PIN_RCD_TRIP_RESET, GPIO_HIGH);
	
	WRP_setDirGpio(PIN_RCD_TST, GPIO_OUTPUT);
	WRP_writeGpio(PIN_RCD_TST, GPIO_HIGH);
	
	WRP_setDirGpio(PIN_RCD_DIS, GPIO_OUTPUT);
	WRP_writeGpio(PIN_RCD_DIS, GPIO_HIGH);
	
	WRP_setDirGpio(PIN_LOCK_D, GPIO_OUTPUT);
	WRP_writeGpio(PIN_LOCK_D, GPIO_LOW);
	
	WRP_setDirGpio(PIN_TYPE_EF_ON, GPIO_OUTPUT);
	WRP_writeGpio(PIN_TYPE_EF_ON, GPIO_LOW);
	
	WRP_setDirGpio(PIN_TYPE2_L2L3_ON, GPIO_OUTPUT);
	WRP_writeGpio(PIN_TYPE2_L2L3_ON, GPIO_LOW);
	
	WRP_setDirGpio(PIN_TYPE2_NL1_ON, GPIO_OUTPUT);
	WRP_writeGpio(PIN_TYPE2_NL1_ON, GPIO_LOW);

	WRP_writeGpio(PIN_PM0, GPIO_LOW);
	WRP_setDirGpio(PIN_PM0, GPIO_OUTPUT);

	WRP_writeGpio(PIN_PM1, GPIO_LOW);
	WRP_setDirGpio(PIN_PM1, GPIO_OUTPUT);

	WRP_writeGpio(PIN_CP_DIS, GPIO_HIGH);
	WRP_setDirGpio(PIN_CP_DIS, GPIO_OUTPUT);


	printf("On allume le backlight\n");
	switchBl(ON, 1);
}

// Test du composant de protection différentielle: reset, test, reset
// rcdErr = 0: OK
//			1: étape 1 ou 3 KO, reset impossible
//			2: étape 2 KO, pas de détection de courant de fuite
static int testRcd() {
int	rcdErr;
	// Reset RCD: les 2 sorties sont au repos
	WRP_writeGpio(PIN_RCD_TRIP_RESET, !GPIO_HIGH);
	WRP_writeGpio(PIN_RCD_DIS, !GPIO_HIGH);
	WRP_writeGpio(PIN_RCD_TRIP_RESET, !GPIO_LOW);
	WRP_writeGpio(PIN_RCD_DIS, !GPIO_LOW);
	if (WRP_readGpio(PIN_RCD_TRIP_DC) && WRP_readGpio(PIN_RCD_TRIP_AC))  rcdErr = GPIO_LOW;
	else rcdErr = 1;
	// test RCD, les 2 sorties sont activées
	if (!rcdErr) {
		WRP_writeGpio(PIN_RCD_TST, !GPIO_HIGH);	// Pulse de test > 120µsec
		usleep(120);
		WRP_writeGpio(PIN_RCD_TST, !GPIO_LOW);
		sleep(2);			// Tempo > 2 sec
		if (!WRP_readGpio(PIN_RCD_TRIP_DC) && !WRP_readGpio(PIN_RCD_TRIP_AC)) rcdErr = GPIO_LOW;
		else rcdErr = 2;
	}
	// Reset RCD, les 2 sorties sont au repos
	if (!rcdErr) {
		WRP_writeGpio(PIN_RCD_TRIP_RESET, !GPIO_HIGH);
		WRP_writeGpio(PIN_RCD_DIS, !GPIO_HIGH);
		WRP_writeGpio(PIN_RCD_TRIP_RESET, !GPIO_LOW);
		WRP_writeGpio(PIN_RCD_DIS, !GPIO_LOW);
		if (WRP_readGpio(PIN_RCD_TRIP_DC) && WRP_readGpio(PIN_RCD_TRIP_AC))  rcdErr = GPIO_LOW;
		else rcdErr = 1;
	}
	
	// Publication du résultat et activation du différentiel 
	switch (rcdErr) {
	case 0:
		writeNotifFile(NOTIF_VALUE_RCD_RESULT, "0");
		activatedRcd = ON;
		WRP_writeGpio(PIN_RCD_DIS, !GPIO_LOW);
		break;
	case 1:
		writeNotifFile(NOTIF_VALUE_RCD_RESULT, "1");
		activatedRcd = GPIO_LOW;
		WRP_writeGpio(PIN_RCD_DIS, !GPIO_HIGH);
		break;
	case 2:
	default:
		writeNotifFile(NOTIF_VALUE_RCD_RESULT, "2");
		activatedRcd = GPIO_LOW;
		WRP_writeGpio(PIN_RCD_DIS, !GPIO_HIGH);
	}
	if (rcdErr)	printf("Erreur du test de protection différentielle: %d\n", lockErr);
	else		printf("Protection différentielle OK");
	return rcdErr;
}

static int testLock() {
	lockErr = 0;
	// Verrouillage et vérification
	closeCableLock(-1);
	// Si pas d'erreur, on tente un déverrouillage
	if (!lockErr) openCableLock(ON, -1);

	// Si erreur, on tente une inversion de lecture du retour de fermeture et on recommence
	if (lockErr) {
		cablelock_feedback_order = !cablelock_feedback_order;
		closeCableLock(-1);
		if (!lockErr) openCableLock(ON, -1);
	}

	// Publication du résultat et activation du différentiel 
	sprintf(str_temp, "%d", lockErr);
	writeNotifFile(NOTIF_VALUE_LATEST_LOCK_ERROR, str_temp);
	if (lockErr) printf("Erreur du test de verrouillage: %d\n", lockErr);
	else		 printf("Test de verrouillage OK");

	// isCablePlugged = WRP_readGpio(PIN_PP_IN)?OFF:ON;
	
	// Si un cable est présent et devrait etre verrouillé 
	if (isCablePlugged) closeCableLock(-1);

	return lockErr;
}

static void initPower() {
	gettimeofday(&startS0, NULL);
	compteurS0 = eeprom_getWh();
	oldCompteur = (long) compteurS0;
}

static void init_machine(LaunchValues *launch_values) {
	printf("Appel à malloc() (machine) -> %s\n", __func__);
    machine = (EtatMachine*)malloc(sizeof(EtatMachine));
    machine->state = launch_values->state;
    machine->A = launch_values->authenticated; // User authentificated
    machine->NAM = launch_values->nam; // No Auth Mode
    machine->OE = launch_values->OE; // Output Enabled
    machine->P0 = launch_values->p0; // PP value - Bit poids faible
    machine->P1 = launch_values->p1; // PP value - Bit poids fort
    machine->C0 = launch_values->c0; // CP value - Bit poids faible
	machine->C1 = launch_values->c1; // CP value - Bit poids fort
    machine->D0 = launch_values->contact0; // Contact sec N° 3
    machine->D1 = launch_values->contact1; // Contact sec N° 2
    machine->D2 = launch_values->contact2; // Contact sec N° 1
    machine->U = launch_values->Unavailable; // Unavailable required
	machine->F = launch_values->Faulted; // Fault

	// Si le redémarrage dans l'état précédent n'est pas demandé,
	if (!reLaunch && (machine->state > PREPARING_4) && (machine->state < Finishing)) {
		machine->state = Finishing;	// Si un véhicule était en charge avant une remise à zéro,
		setState();					// On termine la transaction, on positionne et on publie
		sleep(8);					// Attente pour que l'info remonte sur le serveur OCPP
	}

}

static void setLaunchValues(void) {
	FILE* file = fopen("/usr/share/hubload/launch","w");
	if (file != NULL) {
		fprintf(file,"%d ", machine->state);
		fprintf(file,"%d ", machine->A);
		fprintf(file,"%d ", machine->NAM);
		fprintf(file,"%d ", machine->OE);
		fprintf(file,"%d ", PP);			// Pour éviter une relecture si un cable est présent au boot
		fprintf(file,"%d ", machine->P0);
		fprintf(file,"%d ", machine->P1);
		fprintf(file,"%d ", CP);			// Pour éviter une relecture
		fprintf(file,"%d ", machine->C0);
		fprintf(file,"%d ", machine->C1);
		fprintf(file,"%d ", machine->D0);
		fprintf(file,"%d ", machine->D1);
		fprintf(file,"%d ", machine->D2);
		fprintf(file,"%d ", machine->U);
		fprintf(file,"%d ", machine->F);
		fclose(file);
	}
}

static void initLaunchValues() {
	printf("\n\n-------------------- Initialisation des valeurs de lancement\n\n");
	launch = (LaunchValues*)malloc(sizeof(LaunchValues));

	FILE* file = fopen("/usr/share/hubload/launch","r");

	// Reprise d'état en cours sur perte d'alimentation ou reboot externe
	readOneLineValue("relaunch.conf", active, GPIO_LOW);
	reLaunch = !strcmp(active, "On");

	if ((file != NULL && reLaunch)) {
		fscanf(file,"%d", &(launch->state));
		fscanf(file,"%d", &(launch->authenticated));
		fscanf(file,"%d", &(launch->nam));
		fscanf(file,"%d", &(launch->OE));
		fscanf(file,"%d", &PP);			// Pour éviter une relecture si un cable est présent au boot
		fscanf(file,"%d", &(launch->p0));
		fscanf(file,"%d", &(launch->p1));
		fscanf(file,"%d", &CP);			// Pour éviter une relecture
		fscanf(file,"%d", &(launch->c0));
		fscanf(file,"%d", &(launch->c1));
		fscanf(file,"%d", &(launch->contact0));
		fscanf(file,"%d", &(launch->contact1));
		fscanf(file,"%d", &(launch->contact2));
		fscanf(file,"%d", &(launch->Unavailable));
		fscanf(file,"%d", &(launch->Faulted));
		fclose(file);
	} else {
		launch->state = INIT;
		launch->authenticated = GPIO_LOW;
		launch->nam = GPIO_LOW;
		launch->OE = GPIO_LOW;
		PP = 0;
		// isCablePlugged = WRP_readGpio(PIN_PP_IN)?OFF:ON;
		launch->p0 = 0;
		launch->p1 = 0;
		CP = 12;
		launch->c0 = 1;
		launch->c1 = 1;
		launch->contact0 = !GPIO_LOW;
		launch->contact1 = !GPIO_LOW;
		launch->contact2 = !GPIO_LOW;
		launch->Faulted = GPIO_LOW;
		launch->Unavailable = GPIO_LOW;
		printf("*** 1ere initialisation\n");
		}
}

static void readConfFile(void) {
	char value_token[30];

	readConfFileValue("MAX_PHASE_CURRENT", value_token);
    if (strlen(value_token) > 0) {
        conf_max_current = atoi(value_token);
		max_current = conf_max_current;	// conf_max_current: courant maximal de la borne
    }

	readConfFileValue("PHASE_NUMBER", value_token);
    if (strlen(value_token)) {
		if (atoi(value_token) == 3) conf_mode_phase = TRI;
		else						conf_mode_phase = MONO;
		mode_phase = conf_mode_phase;	// conf_mode_phase: nombre de phases maximal de la borne
    }

	readConfFileValue("VOLTAGE", value_token);
    if (strlen(value_token) > 0) voltageNl = atoi(value_token);
	if (stdVoltage) {					// Standards du monde (110/115/120/127/220/230/240)
		if (voltageNl > 200) voltageNl = 230;
		else 				 voltageNl = 115;
	}

	readConfFileValue("MID_S0_ON", value_token);
	if (!strcmp(value_token, "On"))	s0_activated = ON;
	else							s0_activated = GPIO_LOW;

	readConfFileValue("MID_S0_NB_PER_KWH", value_token);
    if (strlen(value_token)) s0_nb_pulse_per_kwh = atoi(value_token);

	readConfFileValue("EF_ON", value_token);
	EF_closed = !strcmp(active, "On");
	switchRelaisPriseEf(EF_closed);
 
	readConfFileValue("AUTHENTICATION_TYPE", value_token);
	machine->NAM = !strcmp(value_token, "Off");

	readConfFileValue("CABLELOCK_MODE", value_token);
    if (strlen(value_token)) cablelock_mode = atoi(value_token);

	// Remontée des erreurs de courant de défaut
	readOneLineValue("rcd.conf", active, GPIO_LOW);
	lockWarning = !strcmp(active, "On");
	
	// Remontée des erreurs de verrouillage
	readOneLineValue("lock.conf", active, GPIO_LOW);
	lockWarning = !strcmp(active, "On");
	
	// Configuration en Station de recharge publique (pour factuation du post-stationnement)
	readOneLineValue("estation.conf", active, GPIO_LOW);
	eStation = !strcmp(active, "On");
	
	// Timeout sur SuspendedEV pour forcer en finishing
	readOneLineValue("timeoutFinishingSuspendedEV.conf", active, GPIO_LOW);
	timeoutFinishingSuspendedEV = !strcmp(active, "On");
}

// https://lib.must.edu.tw/TCT/SAE%20J1772-2010.pdf
static void calculValeurCycle(int courant) {
	if		(courant >= 80) dutyCycle = 97;
	else if (courant >  52)	dutyCycle = (int)((double)courant / 2.5) + 64;
	else if (courant >   6)	dutyCycle = (int)((double)courant / 0.6);
	else 					dutyCycle = 9;
}

// Permet de controler le relais de la prise latérale, fermé si OnOff = ON
static void switchRelaisPriseEf(int OnOff) {
	WRP_writeGpio(PIN_TYPE_EF_ON, OnOff);
	if (OnOff) gettimeofday(&startEF, NULL);
}

static void closeRelaisType2(int cause) {
	//printf("Appel à closeRelaisType2 avec cause = %d\n", cause);
	WRP_writeGpio(PIN_TYPE2_NL1_ON, ON);
	if (mode_phase == TRI)	WRP_writeGpio(PIN_TYPE2_L2L3_ON, ON);
	type2_closed = ON;
}

static void openRelaisType2(int cause) {
	//printf("Appel à openRelaisType2 avec cause = %d\n", cause);
	WRP_writeGpio(PIN_TYPE2_L2L3_ON, GPIO_LOW);
	WRP_writeGpio(PIN_TYPE2_NL1_ON, GPIO_LOW);
	type2_closed = GPIO_LOW;
}

// Retourne 1 si le cable est verrouillé, cablelock_feedback_order = 0 pour un produit MIDA
static int cable_locked_from_feedback() {
	if (!cablelock_feedback_order)	return (!WRP_readGpio(PIN_LOCK_FB));
	else							return (WRP_readGpio(PIN_LOCK_FB));
}

// Déverrouillage du cable
static void openCableLock(int forcage, int cause) {
	// forcage permet de passer outre le feedback en cas de problème
	if (forcage || ((cablelock_mode != 1) && (cable_locked_from_feedback()))) {
		WRP_writeGpio(PIN_LOCK_D, 0);
		usleep(1000);			// attente 1 msec
		WRP_writeGpio(PIN_LOCK_P,!GPIO_HIGH);
		usleep(250000);			// moteur activé pendant 250 msec
		WRP_writeGpio(PIN_LOCK_P,!GPIO_LOW);
		// Si le cable est toujours verrouillé, il y a erreur
		if (cable_locked_from_feedback())	lockErr = 7;
		else lockErr = 0;
	}
}

static void closeCableLock(int cause) {
	// Si le cable doit être toujours déverrouillé, on ne ferme pas
	if ((cablelock_mode != 2) && !(cable_locked_from_feedback())) {
		WRP_writeGpio(PIN_LOCK_D, 1);
		usleep(1000);			// attente 1 msec
		WRP_writeGpio(PIN_LOCK_P,!GPIO_HIGH);
		usleep(300000);			// moteur activé pendant 300 msec
		WRP_writeGpio(PIN_LOCK_P,!GPIO_LOW);
		// Si le cable n'est toujours pas verrouillé, il y a erreur
		if (!cable_locked_from_feedback())	lockErr = 8;
		else lockErr = 0;
	}
}

void switchBl(int OnOff, int propagNotif) {
	// WRP_writeGpio(PIN_DISPLAY_POWER, !OnOff);
	backlight_on = OnOff;
	if (OnOff) gettimeofday(&startScreen, NULL);

	if (propagNotif) {
		if (OnOff) writeNotifFile(NOTIF_BACKLIGHT, "On");
		else writeNotifFile(NOTIF_BACKLIGHT, "Off");
	}
}

// Permet d'obtenir l'ID de la borne stockée dans l'eeprom
static void eeprom_readIdValue(void) {
	char ID[21];
	int conf_exists = 0;
	// inutile !
	FILE* file = fopen("/usr/share/hubload/id.conf","r+");
	if (file != NULL) {
		conf_exists = 1;
		fscanf(file, "%s", ID);
		fclose(file);
	}

	if (conf_exists == 0) {
		system("expander-get-id");
		FILE* file = fopen("/usr/share/hubload/id.conf","r");
		if (file != NULL) {
			conf_exists = 1;
			fscanf(file, "%s", ID);
			fclose(file);
		}
	}

	writeNotifFile(NOTIF_VALUE_ID_EVSE, ID);
	printf("ID de la borne lu : %s\n", ID);
}

// Permet d'ecrire un ID dans l'eeprom
static void eeprom_writeID(char *id) {
char command[40];

	if(strlen(id) != 12) {
		printf("Error %s: Id n'est pas de la taille autorisé (12 caractères 0-F)\n", __func__);
		return;
	}

	// on verifie l'id, il doit etre dans le format suivant : ABCDEF012345
	for(int i = 0; i<12 ; i++) {
		if((id[i] >= 48 && id[i]<=57) || (id[i] >= 'A' && id[i] < 'Z')) {
			printf("Format de l'ID %s correct\n", id);
		}
		else {
			printf("Error %s: ID incorrect, les caracteres utilisés ne sont pas corrects (0-F)\n", __func__);
			return;
		}
	}
	sprintf(command, "expander-set-id %s", id);
	system(command);
}

// Permet de lire l'energie cumulée en Wh déposée dans un fichier COMPTEUR
static uint16_t eeprom_getWh() {
	uint16_t result = 0;
	readNotifFile(NOTIF_VALUE_COMPTEUR, str_temp);
	result = atol(str_temp);
    return result;
}

// Fonction d'interruption du pp, permet de détecter un arrachement de cable ou une perte de PE sur front montant
//    il y a une fausse interuption sur lecture du PP, mais les relais sont déjà ouverts
static uint32_t last_interrupt_time = 0;

static uint32_t millis(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void pp_interrupt(wrp_gpio_edge_t edge) {
	volatile uint32_t current_time = millis();
	if (current_time - last_interrupt_time < 50) {
		// Ignore cette interruption car trop rapprochée de la précédente
		return;
	}
	last_interrupt_time = current_time;

	switch (edge) {
		case GPIO_EDGE_RISING:
			isCablePlugged = OFF;
			if (type2_closed) {
				openRelaisType2(0);	// ouverture relais type2
				veErr = 6; // erreur si les relais de puissance étaient fermés
			}
			break;
		default:
			isCablePlugged = ON;
			break;
	}
	// isCablePlugged = WRP_readGpio(PIN_PP_IN)?OFF:ON; // retirer ?
}

// Fonction d'interruption de comptage d'énergie S0
static void S0_interrupt(wrp_gpio_edge_t edge) {
	// Incrémentation du compteur d'énergie
	compteurS0 += (double)s0_nb_pulse_per_kwh / 1000;
	// delai entre 2 impulsions S0 pour le calcul de puissance instantanée
	gettimeofday(&endS0, NULL);
	deltaT_S0 = (endS0.tv_sec - startS0.tv_sec) * 1000000 + (endS0.tv_usec - startS0.tv_usec);
	startS0 = endS0;
}

// Fonction d'interruption du bouton, activée sur front descendant, la gestion est dans la boucle principale
static void user_key_interrupt(wrp_gpio_edge_t edge) {
	gettimeofday(&startKey, NULL);
	keyPressed = ON;
}

// Transition à partir de l'INIT jusqu'à PREPARING_3, mais aussi Unavailable et Faulted
static void calculTransitionPreparing() {
	if		(machine->F)
		machine->state = Faulted;
	else if (machine->U)
		machine->state = Unavailable;
	else if (!machine->NAM && !machine->A && !machine->P1 && !machine->P0)
		machine->state = AVAILABLE;		// Non authentifié, pas de cable
	else if (!machine->NAM && !machine->A && (machine->P1 || machine->P0) && machine->C1 && machine->C0)
		machine->state = PREPARING_0;	// Non authentifié, un cable sans véhicule
	else if (!machine->NAM && !machine->A && ((machine->C1 && !machine->C0) || (!machine->C1 && machine->C0)))
		machine->state = PREPARING_1;	// Non authentifié, un cable et un véhicule à l'état B, C ou D
	else if ((machine->NAM || (!machine->NAM && machine->A)) && !machine->P1 && !machine->P0)
		machine->state = PREPARING_2;	// NAM ou authentifié, sans cable
	else if ((machine->NAM || (!machine->NAM && machine->A)) && (machine->P1 || machine->P0) && machine->C1 && machine->C0)
		machine->state = PREPARING_3;	// NAM ou authentifié, un cable sans véhicule
	else if (((machine->NAM && !stopCharge) || (!machine->NAM && machine->A)) && ((machine->C1 && !machine->C0) || (!machine->C1 && machine->C0)))
		machine->state = PREPARING_4;	// NAM sans arret demandé ou authentifié, un cable et un véhicule à l'état B -> Lancement de la charge
										//  	LEGACY: lancement de la charge aussi dans l'état C ou D (EV READY 1.4 PAGE 39)
}

// Transition à partir de la phase de PREPARING_4: démarrage de la charge
static void calculTransitionStartCharge() {
	if		(machine->F)
		machine->state = Faulted;
	else if (machine->U)
		machine->state = Unavailable;
	else if (!machine->NAM && !machine->A && !machine->P1 && !machine->P0)
		machine->state = AVAILABLE;		// Non authentifié, pas de cable
	else if (!machine->NAM && !machine->A && (machine->P1 || machine->P0) && machine->C1 && machine->C0)
		machine->state = PREPARING_0;	// Non authentifié, un cable sans véhicule
	else if (!machine->NAM && !machine->A && ((machine->C1 && !machine->C0) || (!machine->C1 && machine->C0)))
		machine->state = PREPARING_1;	// Non authentifié, un cable et un véhicule à l'état B, C ou D
	else if ((machine->NAM || (!machine->NAM && machine->A)) && !machine->P1 && !machine->P0)
		machine->state = PREPARING_2;	// NAM ou authentifié, sans cable
	else if ((machine->NAM || (!machine->NAM && machine->A)) && (machine->P1 || machine->P0) && machine->C1 && machine->C0)
		machine->state = PREPARING_3;	// NAM ou authentifié, un cable sans véhicule
	else if (machine->OE && !machine->C1 && machine->C0 && !machine->P1 && machine->P0)
		machine->state = CHARGING_13;	// Authentifié, véhicule en status C ou D, puissance disponible
	else if (machine->OE && !machine->C1 && machine->C0 && machine->P1 && !machine->P0)
		machine->state = CHARGING_20;
	else if (machine->OE && !machine->C1 && machine->C0 && machine->P1 && machine->P0)
		machine->state = CHARGING_32;
	else if (!machine->OE && !machine->C1 && machine->C0)
		machine->state = SuspendedEVSE;	// Authentifié, véhicule en status C ou D, puissance indisponible
}

// Transition à partir de l'état de charge: pause ou fin de charge
static void calculTransitionCharge() {
	if		(machine->F)
		machine->state = Finishing;
	else if (machine->U)
		machine->state = Finishing;		// Si Unavailable est demandé, on passe d'abord par Finishing pour terminer proprement
	else if ((machine->NAM && stopCharge) || (!machine->NAM && !machine->A))
		machine->state = Finishing;		// NAM avec arret demandé ou non authentifié
	else if (machine->C1 && machine->C0)
		machine->state = Finishing;		// Authentifié, plus de véhicule
	else if (!machine->OE)
		machine->state = SuspendedEVSE;	// Authentifié, un véhicule présent, puissance indisponible
	else if (machine->C1 && !machine->C0)
		machine->state = SuspendedEV;	// Authentifié, un véhicule en status B, puissance disponible
}

// Transition à partir de SuspendedEV: reprise ou fin de charge
static void calculTransitionSev() {
	if		(machine->F)
		machine->state = Finishing;
	else if (machine->U)
		machine->state = Finishing;		// Si Unavailable est demandé, on passe d'abord par Finishing pour terminer proprement
	else if ((machine->NAM && stopCharge) || (!machine->NAM && !machine->A))
		machine->state = Finishing;		// Plus authentifié
	else if (machine->C1 && machine->C0)
		machine->state = Finishing;		// Authentifié, plus de véhicule
	else if (!machine->OE)
		machine->state = SuspendedEVSE;	// Authentifié, un véhicule présent, puissance indisponible
	else if (!machine->C1 && machine->C0 && !machine->P1 && machine->P0)
		machine->state = CHARGING_13;	// puissance disponible, un cable et un véhicule en status C ou D
	else if (!machine->C1 && machine->C0 && machine->P1 && !machine->P0)
		machine->state = CHARGING_20;
	else if (!machine->C1 && machine->C0 && machine->P1 && machine->P0)
		machine->state = CHARGING_32;
}

// Transition à partir de SuspendedEVSE: reprise ou fin de charge
static void calculTransitionSevse() {
	if		(machine->F)
		machine->state = Finishing;
	else if (machine->U)
		machine->state = Finishing;		// Si Unavailable est demandé, on passe d'abord par Finishing pour terminer proprement
	else if ((machine->NAM && stopCharge) || (!machine->NAM && !machine->A))
		machine->state = Finishing;		// Plus authentifié
	else if (machine->C1 && machine->C0)
		machine->state = Finishing;		// Authentifié, plus de véhicule
	else if (!machine->OE)
		return;							// Authentifié, un véhicule présent, puissance toujours indisponible	
	else if (machine->C1 && !machine->C0)	// puissance disponible, authentifié, un véhicule présent en status B
		machine->state = PREPARING_4;	// Correction 18/10/2023 : pour éventuellement relancer la charge, on se remet en P4
	else if (machine->OE && !machine->C1 && machine->C0 && !machine->P1 && machine->P0)
		machine->state = CHARGING_13;	// puissance disponible, authentifié, un véhicule présent en status C ou D
	else if (machine->OE && !machine->C1 && machine->C0 && machine->P1 && !machine->P0)
		machine->state = CHARGING_20;
	else if (machine->OE && !machine->C1 && machine->C0 && machine->P1 && machine->P0)
		machine->state = CHARGING_32;
}

// Transition à partir de Finishing: déconnexion du véhicule
static void calculTransitionFinishing() {
	if		(machine->F && machine->C1 && machine->C0)
		machine->state = Faulted;		// On affiche l'écran d'erreur si le véhicule n'est plus là
	else if (machine->U)
		machine->state = Unavailable;
	else if (!machine->NAM && !machine->A && !machine->P1 && !machine->P0)
		machine->state = AVAILABLE;		// Non authentifié, pas de cable
	else if (!machine->NAM && !machine->A && (machine->P1 || machine->P0) && machine->C1 && machine->C0)
		machine->state = PREPARING_0;	// Non authentifié, un cable sans véhicule
	else if ((machine->NAM || (!machine->NAM && machine->A)) && !machine->P1 && !machine->P0)
		machine->state = PREPARING_2;	// NAM ou authentifié, sans cable
	else if ((machine->NAM || (!machine->NAM && machine->A)) && (machine->P1 || machine->P0) && machine->C1 && machine->C0)
		machine->state = PREPARING_3;	// NAM ou authentifié, un cable sans véhicule
}

// Calcule si une tranistion est nécessaire en fonction des parametres (NAM,OE,D2,D1,D0,A,U,F,P1,P0,C1,C0)
static void calculTransitionEtat() {
	switch (machine->state) {
	case INIT:
	case AVAILABLE:
	case PREPARING_0:
	case PREPARING_1:
	case PREPARING_2:
	case PREPARING_3:
		calculTransitionPreparing();
		break;
	case PREPARING_4:
		calculTransitionStartCharge();
		// Tentative de réveil du véhicule 1 fois
		if ((machine->state == PREPARING_4) && machine->C1 && !machine->C0) {
			// NORME EV READY 1.4 (EV40 PAGES 25 et 39)
			// Si le véhicule est toujours en status B, on essaie de le réveiller,
			gettimeofday(&timeNow, NULL);
			delayState = (timeNow.tv_sec - startNewState.tv_sec);
			if (eStation && (delayState > (TIMEOUT_P4 + 6))) {
				// Erreur du véhicule en sommeil "profond", dans une station service il faut le déconnecter
				veErr = 10;
			} else if (delayState > (TIMEOUT_P4 + 3)) {
				// Reconnexion par réinitialisation de la séquence de charge
				if (cpDisableToggle) {
					setStateStartCharging();
					cpDisableToggle = GPIO_LOW;
				}
			} else if (delayState > TIMEOUT_P4) {
				// Simulation d'une déconnexion du cable en desactivant le CP
				if (!cpDisableToggle) {
					WRP_writeGpio(PIN_CP_DIS, !GPIO_HIGH);
					enableCp = GPIO_LOW;
					cpDisableToggle = ON;
				}
			} 
		}
		break;
	case CHARGING_13:
	case CHARGING_20:
	case CHARGING_32:
		calculTransitionCharge();
		break;
	case SuspendedEV:
		calculTransitionSev();
		if (machine->state == SuspendedEV && timeoutFinishingSuspendedEV && !eStation) {
			// Si le véhicule reste trop longtemps en SuspendedEV, on passe en Finishing si on n'est pas dans une Estation
			gettimeofday(&timeNow, NULL);
			if ((timeNow.tv_sec - startNewState.tv_sec) > TIMEOUT_SEV)	machine->state = Finishing;
		}
		break;
	case SuspendedEVSE:
		calculTransitionSevse();
		break;
	case Finishing:
		// On ne sortira de cet état que si le véhicule est déconnecté
		calculTransitionFinishing();
		break;
	case Unavailable:
	case Faulted:
	default:
		calculTransitionPreparing();
		break;
	}
}

static void setStateError(void) {
	WRP_writePwm(98);
	if (!enableCp) {
		WRP_writeGpio(PIN_CP_DIS, !GPIO_LOW);
		enableCp = ON;
	}
	openRelaisType2(13);
	openCableLock(GPIO_LOW, 13);
}

static void setStatePassive(void) {
	// Désactivation de la sortie du CP
	if (enableCp) {
		WRP_writeGpio(PIN_CP_DIS, !GPIO_HIGH);
		enableCp = GPIO_LOW;
	}
	openRelaisType2(10);
	// On s'arrure que le verrou est ouvert si le cable n'est pas détecté
	if (!PP && (cablelock_mode == 1) && cable_locked_from_feedback()) openCableLock(ON, 11);
	openCableLock(GPIO_LOW, 10);
}

static void setStatePreparingCar(void) {
	WRP_writePwm(100);
	// Activation de la sortie du CP à +12V
	if (!enableCp) {
		WRP_writeGpio(PIN_CP_DIS, !GPIO_LOW);
		enableCp = ON;
	}
	openRelaisType2(11);
	// Verrouillage si le cable doit être toujours bloqué
	if (cablelock_mode == 1) {
		// Si le cable n'est pas détecté, le verrou doit être ouvert
		if (!PP && cable_locked_from_feedback()) openCableLock(ON, 11);

		// Tempo pour l'insertion la première fois qu'il sera verrouillé
		if (!cable_locked_from_feedback() && PP) {
			// publication du nouvel état en avance de phase pour que l'affichage ne subisse pas de retard
			sprintf(str_temp, "%d", machine->state);
			writeNotifFile (NOTIF_STATE_EVSE, str_temp);
			sleep(10);
		}
		closeCableLock(11);
	}

	openCableLock(GPIO_LOW, 11);
}

static void setStateStartCharging(void) {
	closeCableLock(12);
	openRelaisType2(12);
	WRP_writePwm(100);
	// Activation de la sortie du CP
	if (!enableCp) {
		WRP_writeGpio(PIN_CP_DIS, !GPIO_LOW);
		enableCp = ON;
	}
	if (PP <= max_current) calculValeurCycle(PP);
	else calculValeurCycle(max_current);
	usleep(600000);		// attente minimale de 0,6 sec
	WRP_writePwm(dutyCycle);
}

static void setStateCharging(int OnOff) {
	//printf("Relais et cable : \n");
	closeCableLock(14);
	if (!enableCp) {
		WRP_writeGpio(PIN_CP_DIS, !GPIO_LOW);
		enableCp = ON;
	}
	if (OnOff) {	// En charge
		closeRelaisType2(14);
		// Mise à jour éventuelle du CP
		int oldDutyCycle = dutyCycle;
		if (PP <= max_current) calculValeurCycle(PP);
		else calculValeurCycle(max_current);
		if (oldDutyCycle != dutyCycle) WRP_writePwm(dutyCycle);
	}
	else openRelaisType2(14);
}

static void setStateStopCharging() {
	if (type2_closed) {		// Demande d'arret pour éviter une ouverture en charge des relais
		WRP_writePwm(9);
		usleep(500000);		// attente minimale de 0,5 sec
		WRP_writePwm(97);
		sleep(3);			// attente de 3 sec pour que le véhicule stoppe la charge
	}
	openRelaisType2(15);
}

static void send_values(int complete) {

	sprintf(str_temp, "%d", machine->state);
	writeNotifFile (NOTIF_STATE_EVSE, str_temp);
	writeNotifFile (NOTIF_STATE_TYPE2, &type2Status);
	system("sudo read_temp");
	if (cable_locked_from_feedback()) writeNotifFile(NOTIF_CL_FEEDBACK, "Locked\n");
	else writeNotifFile(NOTIF_CL_FEEDBACK, "Unlocked\n");

	// compteurS0 est incrémenté dans la routine d'interruption du S0
	// On le fige dans compteur pour éviter une incrémentation pendant l'execution de ce qui suit
	compteur = (long) compteurS0;
	// Publication des infos d'énergie s'il y a eu consommation depuis la publication précédente
	if (compteur > oldCompteur) {
		// Écriture de la charge cumulée dans l'eeprom protegee
		for (int i = 0; i < (compteur - oldCompteur); i++) system("expander-add-wh");

		// Publication des données de charge (energie cumulée, puissance, courant moyen)
		sprintf(str_temp, "%ld", compteur);
		writeNotifFile(NOTIF_VALUE_COMPTEUR, str_temp);

		// Calcul de puissance instantané (W) en fonction de la fréquence des impulsions S0
		if (s0_activated) power = 3600000000000 / (deltaT_S0 * s0_nb_pulse_per_kwh);
		sprintf(str_temp, "%ld", power);
		writeNotifFile(NOTIF_VALUE_POWER, str_temp);

		current = (double)power/(voltageNl * mode_phase);
		sprintf(str_temp, "%.2lf", current);
		writeNotifFile(NOTIF_VALUE_CURRENT, str_temp);

		if (PP <= max_current) {
			printf("Energie Cumulée = %ld Wh; Power = %ld W; courrant moyen = %.2lf; Consigne ≤ %d; NbPhase = %d\n", compteur, power, current, PP, mode_phase);
		} else {
			printf("Energie Cumulée = %ld Wh; Power = %ld W; courrant moyen = %.2lf; Consigne ≤ %d; NbPhase = %d\n", compteur, power, current, max_current, mode_phase);
		}
		oldCompteur = compteur;
	}

	if (complete) {					// Publication complete de la machine à état
		sprintf(str_temp, "%d", dutyCycle);
		writeNotifFile(NOTIF_VALUE_DUTYCYCLE, str_temp);
		sprintf(str_temp, "NAM=%d, OE=%d, D2=%d, D1=%d, D0=%d, A=%d, U=%d, F=%d, P1=%d, P0=%d, C1=%d, C0=%d", 
				machine->NAM, machine->OE, machine->D2, machine->D1, machine->D0,
				machine->A, machine->U, machine->F, machine->P1, machine->P0,
				machine->C1, machine->C0);
		writeNotifFile(NOTIF_STATE_COMP, str_temp);
	}		
	gettimeofday(&startSend, NULL);	// horodatage envoi
}

// On positionne l'état courant et l'activation du scan RFID
static void setState() {
	switchBl(ON, 1);

	switch (machine->state) {
	case INIT:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "Off");
		setStatePassive();
		break;
	case AVAILABLE:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "On");
		setStatePassive();
		break;
	case PREPARING_0:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "On");
		setStatePreparingCar();
		break;
	case PREPARING_1:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "On");
		setStatePreparingCar();
		break;
	case PREPARING_2:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "Off");
		setStatePassive();
		break;
	case PREPARING_3:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "Off");
		setStatePreparingCar();
		break;
	case PREPARING_4:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "Off");
		// remise à 0 de la puissance Appelée
		writeNotifFile(NOTIF_VALUE_POWER, "0");
		setStateStartCharging();
		break;
	case CHARGING_13:
	case CHARGING_20:
	case CHARGING_32:
		setStateCharging(ON);
		if (previousState == PREPARING_4)	writeNotifFile(NOTIF_ACTIVATE_SCAN, "delayedOn");
		else								writeNotifFile(NOTIF_ACTIVATE_SCAN, "On");
		break;
	case SuspendedEV:
		setStateCharging(GPIO_LOW);
		if (previousState == PREPARING_4)	writeNotifFile(NOTIF_ACTIVATE_SCAN, "delayedOn");
		else								writeNotifFile(NOTIF_ACTIVATE_SCAN, "On");
		break;
	case SuspendedEVSE:
		setStateStopCharging();
		if (previousState == PREPARING_4)	writeNotifFile(NOTIF_ACTIVATE_SCAN, "delayedOn");
		else								writeNotifFile(NOTIF_ACTIVATE_SCAN, "On");
		break;
	case Finishing:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "Off");
		if (machine->NAM) stopCharge = ON;	// L'authentification est invalidée
		else machine->A = GPIO_LOW;				//    pour terminer la transaction
		setStateStopCharging();
		openCableLock(0, 15);
		break;
	case Unavailable:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "Off");
		setStatePassive();
		break;
	case Faulted:
	default:
		writeNotifFile(NOTIF_ACTIVATE_SCAN, "Off");
		if (machine->NAM) stopCharge = ON;	// L'authentification est invalidée
		else machine->A = GPIO_LOW;				//    pour terminer la transaction
		setStateError();
	}
	printf("On publie les valeurs et on initialise l'état %d\n", machine->state);
	send_values(ON);
	setLaunchValues();					// Sauvegarde de l'état courant pour reboot éventuel
	gettimeofday(&startNewState, NULL);	// Horodatage du dernier changement d'état
}

// Sur changement d'état nécessaire, on établit ce nouvel etat et on le publie 
static void checkTransitionEtat(void) {
	readNotifFile(NOTIF_STATE_JAVA, active);

	if (!strcmp(active, "On")) {
		previousState = machine->state;
		calculTransitionEtat();
		if (machine->state != previousState) {
			printf("Transition d'état : %d -> %d\n", previousState, machine->state);
			setState();
		}
	} else printf("La machine Java est Off\n");
}

static void lectureCpValue() {
	int readValue = WRP_readAdc(ADC_CP, 0);
	// filtrage par moyenne de 4 valeurs
	for (int cptMes = 0; cptMes < 3; cptMes++) readValue += WRP_readAdc(ADC_CP, 0);
	readValue = readValue >> 2;
	if		( readValue > 3257 ) CP = 12;   // supérieur à 10.5V (norme 12V)
	else if ( readValue > 2327 ) CP = 9;	// supérieur à 7.5V (norme 9V)
	else if ( readValue > 1396 ) CP = 6;	// supérieur à 4.5V (norme 6V)
	else if ( readValue >  465 ) CP = 3;	// supérieur à 1.5V (norme 3V)
	else                         CP = 0;    // Attention, 0V et -12V ne sont pas distingués
}

static void lecturePpValue() {
	int readValue = WRP_readAdc(ADC_PP, 0);			// Attention, PIN_PP_IN remonte à 1 le temps de la lecture
	// filtrage par moyenne de 4 valeurs
	for (int cptMes = 0; cptMes < 7; cptMes++) readValue += WRP_readAdc(ADC_PP, 0);
	readValue = readValue >> 3;
	if		( readValue > 4000 ) PP = 0;    // plus de 1800 ohms
	//else if	( readValue > 3670 ) PP = 6;    // plus de 1650 ohms (norme > 1500 ohms)
	else if ( readValue > 3617 ) PP = 13;   // plus de 1090 ohms (norme = 1500 ohms)
	else if ( readValue > 2544 ) PP = 20;   // plus de 450 ohms (norme = 680 ohms)
	else if ( readValue > 1710 ) PP = 32;   // plus de 160 ohms (norme = 220 ohms)
	else if ( readValue > 1047 ) PP = 03;   // plus de 50 ohms (norme = 100 ohms)
	else                         PP = 80;    // moins de 50 ohms
	
	//printf("PP lu = %d, PP = %d ", readValue, PP);
}

static void setPPMachineValues() {
	// On met les valeurs pour P0 et P1 pour la machine à états
	switch (PP) {
	case 32 :
		machine->P1 = 1;
		machine->P0 = 1;
		break;
	case 20 :
		machine->P1 = 1;
		machine->P0 = 0;
		break;
	case 13 :
		machine->P1 = 0;
		machine->P0 = 1;
		break;
	case 0 :
	default :
		machine->P1 = 0;
		machine->P0 = 0;
	}
}

static void setCPMachineValues() {
	// On met les valeurs pour C0 et C1 pour la machine à états
    switch (CP) {
    case 12 :
        machine->C1 = 1;
        machine->C0 = 1;
        type2Status = 'A';
        break;
    case 9 :
        machine->C1 = 1;
        machine->C0 = 0;
        type2Status = 'B';
        break;
    case 6 :
        machine->C1 = 0;
        machine->C0 = 1;
        type2Status = 'C';
        break;
    case 3 :
        machine->C1 = 0;
        machine->C0 = 1;
        type2Status = 'D';
        break;
    case 0:
    default :
        machine->C1 = 0;
        machine->C0 = 0;
        type2Status = 'E';
    }
}

// Fonction de détection d'un cable (PP) et d'un véhicule (CP)
static void lectureInputValues() {
	// isCablePlugged = WRP_readGpio(PIN_PP_IN)?OFF:ON;
	// Etape 1 : on vérifie s'il y a un cable dans le connecteur
	if (isCablePlugged) {		// Un cable est présent
		// Pour s'assurer que tout est correct, on remet les CS à 0
		//WRP_writeGpio(PIN_PM_CS, !GPIO_LOW);

		if (!PP) lecturePpValue();
		if (enableCp) lectureCpValue();
		else CP = 12;
	} else {	// Pas de cable
		WRP_writeGpio(PIN_TYPE2_L2L3_ON, GPIO_LOW);	// Déjà fait dans la routine d'interuption, par sécurité
		WRP_writeGpio(PIN_TYPE2_NL1_ON, GPIO_LOW);
		PP = 0;	// Pas de cable -> pas de courant

		// si un véhicule est détecté mais pas le cable, c'est une erreur par perte de continuité de masse (PE)
		if (enableCp) {
			lectureCpValue();
			if (CP != 12) veErr = 6;
		} else CP = 12;
	}

	// Si pas de véhicule, la demande d'arret en mode NAM doit être GPIO_LOW
	if ((CP == 12) && machine->NAM && stopCharge) stopCharge = GPIO_LOW;

	// Si CP est activé mais on lit CP=0, c'est une erreur générée par un cable ou un véhicule
	if (enableCp && !CP) {
		if ((machine->state > PREPARING_4) && (machine->state < Finishing)) veErr = 5;	// Erreur véhicule en charge
		else veErr = 9;	// Erreur cable ou véhicule à la connexion
	}


	// On positionne P1/P0 et C1/C0
	setCPMachineValues();
	setPPMachineValues();
}

// Fonction de lecture, publication et corrections des erreurs
static void errorCheck() {
	// Lecture de la demande de remontée sur l'écran de l'erreur RCD
    	readOneLineValue("rcd.conf", active, GPIO_LOW);
    	rcdWarning = !strcmp(active, "On");
	
	// Lecture de la demande de remontée sur l'écran de l'erreur de verrouillage
    	readOneLineValue("lock.conf", active, GPIO_LOW);
    	lockWarning = !strcmp(active, "On");
	
	// Test du différentiel si opérationnel
	if (activatedRcd) {
		if (!WRP_readGpio(PIN_RCD_TRIP_DC)) veErr = 2;
		if (!WRP_readGpio(PIN_RCD_TRIP_AC)) veErr = 3;
		if (!WRP_readGpio(PIN_RCD_TRIP_DC) && !WRP_readGpio(PIN_RCD_TRIP_AC)) veErr = 4;
	}

	// Erreur véhicule, on publie cette erreur si ce n'est pas déjà fait
	if (veErr && !pubErr) {
		sprintf(str_temp, "%d", veErr);		// Publication de l'état d'erreur
		writeNotifFile(NOTIF_VALUE_LATEST_VE_ERROR, str_temp);
		printf("Erreur connexion véhicule: %d\n", veErr);
		pubErr = ON;
	}
	
	// On stoppe une charge éventuelle au premier passage
	if (veErr && (veErr < 7) && (machine->state > PREPARING_4 && (machine->state < Finishing))) {
		machine->state = Finishing;		// Si un véhicule était en charge, on termine la transaction
		setState();						// On positionne et on publie
	}

	// Correction des erreurs véhicule après déconnexion
	if (veErr && machine->C1 && machine->C0) {
		switch (veErr) {
		case 1:			// Erreur système générée par le JAVA, on ne fait rien et on ne l'efface pas
			break;
		case 2:			// Erreur RCD DC ou AC, on réarme le différentiel
		case 3:
		case 4:
			WRP_writeGpio(PIN_RCD_TRIP_RESET, !GPIO_HIGH);
			WRP_writeGpio(PIN_RCD_DIS, !GPIO_HIGH);
			WRP_writeGpio(PIN_RCD_TRIP_RESET, !GPIO_LOW);
			WRP_writeGpio(PIN_RCD_DIS, !GPIO_LOW);
			//break;	!!!!! Pas de break, on poursuit l'execution
		case 5:			// CP <= 0 erreur véhicule, l'utilisateur devait simplement se déconnecter
		case 6:			// perte PE ou arrachement du cable en charge
		case 9:			// Véhicule en PREPARING_1/4, CP = 0 (pas de passage par Finishing)
		case 10:		//		"	en PREPARING_4, pas de passage à l'état C ou D après un 2ème essai (pas de passage par Finishing)
		default:
			printf("Erreur %d corrigée\n", veErr);
			veErr = 0;
			writeNotifFile(NOTIF_VALUE_LATEST_VE_ERROR, "0");
			pubErr = GPIO_LOW;
		}
	}

	// Tentatives de correction des erreurs de verrouillage
	if (lockErr) {
		trials++;
		if (trials < rstTrials) {
			sprintf(str_temp, "%d", lockErr);	// Publication de l'état d'erreur
			writeNotifFile(NOTIF_VALUE_LATEST_LOCK_ERROR, str_temp);
			printf("Erreur %d verrouillage: %d\n", trials, lockErr);
		}

		switch (lockErr) {
		case 7:					// Erreur de déverrouillage, on réessaie (rstTrials-1) fois
			if (trials != rstTrials) {
				sleep(2);		// On attend 2 sec pour être sûr de ne pas faire chauffer le moteur
				openCableLock(ON, 16);
			}
			else trials--;
			break;
		case 8:					// Erreur de verrouillage, on réessaie (rstTrials-1) fois
			if (trials != rstTrials) {
				sleep(2);		// On attend 2 sec pour être sûr de ne pas faire chauffer le moteur
				closeCableLock(16);
			}
			else trials--;
			break;
		default:
			break;
		}
	} else trials = 0;
	readNotifFile(NOTIF_VALUE_LATEST_LOCK_ERROR, active);
	if (!lockErr && strcmp(active, "0")) 	writeNotifFile(NOTIF_VALUE_LATEST_LOCK_ERROR, "0");

	// Affichage éventuel du message de maintenance
	readNotifFile(NOTIF_VALUE_LATEST_LOCK_ERROR, active);
	if ((lockWarning && strcmp(active, "0")) || (rcdWarning && !activatedRcd)) {
		readNotifFile(NOTIF_WARNING, active);
		if (strcmp(active, "On")) writeNotifFile(NOTIF_WARNING, "On");
	} else {
		readNotifFile(NOTIF_WARNING, active);
		if (strcmp(active, "Off")) writeNotifFile(NOTIF_WARNING, "Off");
	}


}

int main(int argc, char *argv[]) {
	writeNotifFile (NOTIF_STATE_EVSE, "0");

    if (WRP_init() < 0) {
		fprintf(stderr, "%s : failed to init wrappers\n", __func__);
		exit(EXIT_FAILURE);
	}

	// Initialisation des pins, sauf pour l'interruption S0
	initPinValues();

	// On initialise la machine à états, avec reprise éventuelle après une prte d'alimentation
	initLaunchValues();
	init_machine(launch);

	eeprom_readIdValue();

	// on configure l'execution de la fonction interruption si ctrl+C
	signal(SIGKILL, stop_signal);
	signal(SIGABRT, stop_signal);
	signal(SIGINT, stop_signal);
	signal(SIGTERM, stop_signal);
	signal(SIGQUIT, stop_signal);

	/* Step 1. Initialize inotify */
	fd = inotify_init();
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {  // error checking for fcntl
		printf("Problème fcntl\n");
		return EXIT_FAILURE;
	}
	/* Step 2. Add Watch */
	wd = inotify_add_watch(fd,notif_path,IN_MODIFY | IN_CREATE | IN_DELETE);

	if (wd < 0)	printf("Could not watch : %s\n",notif_path);
	else		printf("Watching : %s\n",notif_path);

	readConfFile();
	// S0 External measurement interrupt
	if (s0_activated) WRP_interruptGpio (PIN_SM_TIC_D, GPIO_EDGE_FALLING,  &S0_interrupt);

	//// TEST DU DIFFERENTIEL ET DU MOTEUR DE VERROUILLAGE ///
	testRcd();
	testLock();
	// Affichage éventuel du message de maintenance
	if ((lockWarning && lockErr) || (rcdWarning && !activatedRcd)) 	writeNotifFile(NOTIF_WARNING, "On");
	else writeNotifFile(NOTIF_WARNING, "Off");

	// On passe STATE_MACHINE à "On"
	readNotifFile(NOTIF_STATE_MACHINE, active);
	if (strcmp(active, "On"))	writeNotifFile (NOTIF_STATE_MACHINE, "On");

	// On attend que le programme JAVA soit lancé
	do {
		sleep(1);	// Attente 1 sec
		readNotifFile(NOTIF_STATE_JAVA, active);
	} while (strcmp(active, "On"));

	//// INITIALISATION CALCUL DE PUISSANCE ///
	initPower();

	// debut de la boucle infinie
	while (mainLoop) {
		int i=0,length;
		char buffer[BUF_LEN];

		//// CHECK DES FICHIERS DE NOTIFICATION ////

		/* Step 3. Read buffer*/
		length = read(fd,buffer,BUF_LEN);

		/* Step 4. Process the events which has occurred */
		while(i<length){
			struct inotify_event *event = (struct inotify_event *) &buffer[i];

			if(event->len){
				if ( event->mask & IN_MODIFY ) {
					if ( event->mask & IN_ISDIR ) {
						//printf( "The directory %s was modified.\n", event->name );
					}
					else {
						if (!strcmp(event->name, NOTIF_REDUCE_BL)) {
							readNotifFile(NOTIF_REDUCE_BL, active);
							// WRP_writeGpio(PIN_DISPLAY_REDUCE_POWER, (!strcmp(active, "On")));
						}
						else if (!strcmp(event->name, NOTIF_BACKLIGHT)) {
							readNotifFile(NOTIF_BACKLIGHT, active);
							switchBl(!strcmp(active, "On"), 0);
						}
						else if (!strcmp(event->name, NOTIF_EF_CLOSE)) {
							readNotifFile(NOTIF_EF_CLOSE, active);
							EF_closed = !strcmp(active, "On");
							switchRelaisPriseEf(EF_closed);
						}
						else if (!strcmp(event->name, NOTIF_SESSION_MAX_CURRENT)) {
							readNotifFile(NOTIF_SESSION_MAX_CURRENT, active);
							int value = atoi(active);
							if (value <= conf_max_current)	max_current = value;
						}
						else if (!strcmp(event->name, NOTIF_CL_LOCK)) {
							readNotifFile(NOTIF_CL_LOCK, active);
							if (!strcmp(active, "Off") && (!type2_closed)) {
								openCableLock(ON,1);
							}
						}
						else if (!strcmp(event->name, NOTIF_VALUE_PHASES)) {
							readNotifFile(NOTIF_VALUE_PHASES, active);
							if (conf_mode_phase == MONO)	mode_phase = MONO;
							else {
								if (!strcmp(active, "1"))	mode_phase = MONO;
								else						mode_phase = TRI;
							}
						}
						else if (!strcmp(event->name, NOTIF_ID_EVSE_READ)) {
							eeprom_readIdValue();
						}
						else if (!strcmp(event->name, NOTIF_ID_EVSE_WRITE)) {
							readNotifFile(NOTIF_ID_EVSE_WRITE, active);
							eeprom_writeID(active);
						}
						else if (!strcmp(event->name, NOTIF_NO_AUTH_MODE)) {
							readNotifFile(NOTIF_NO_AUTH_MODE, active);
							machine->NAM = !strcmp(active, "On");
							// Si le mode NAM est activé, alors il faut repartir de 0 et déconnecter tout véhicule déjà présent
							if (machine->NAM) stopCharge = ON;	// L'authentification est invalidée
							else machine->A = GPIO_LOW;				//    pour terminer la transaction
						}
						else if (!strcmp(event->name, NOTIF_ENERGY_ALLOWED)) {
							readNotifFile(NOTIF_ENERGY_ALLOWED, active);
							machine->OE = !strcmp(active, "On");
						}
//						else if (!strcmp(event->name, NOTIF_UNAVAILABLE)) {
//							readNotifFile(NOTIF_UNAVAILABLE, active);
//							machine->U = !strcmp(active, "On");
//						}
						else if (!strcmp(event->name, NOTIF_AUTHENTICATED)) {
							readNotifFile(NOTIF_AUTHENTICATED, active);
							machine->A = !strcmp(active, "On");
							// Si machine->A == GPIO_LOW c'est une demande d'arret de charge, il faut traiter le cas du mode NAM
							if (machine->NAM && !machine->A) stopCharge = ON;
						}
						else if (!strcmp(event->name, NOTIF_TOUCH)) {
							switchBl(ON, 1);
						}
						else if (!strcmp(event->name, NOTIF_CONF)) {
							readConfFile();
						}
						else if (!strcmp(event->name, NOTIF_STOP)) {
							stop_signal();
						}
						else if (!strcmp(event->name, NOTIF_RESTART)) {
							rebootBorne();
						}
						else if (!strcmp(event->name, NOTIF_ERROR)) {
							readNotifFile(NOTIF_ERROR, active);
							machine->F = !strcmp(active, "On");
						}
						else if (!strcmp(event->name, NOTIF_TEST_RCD)) {
							testRcd();
						}
						else if (!strcmp(event->name, NOTIF_FINISHING_SEV)) {
							readNotifFile(NOTIF_FINISHING_SEV, active);
							timeoutFinishingSuspendedEV = !strcmp(active, "On");
						}
					}
				}
			}
			i += EVENT_SIZE + event->len;
		}
		


		//// TEST DE PRÉSENCE CABLE ET ETAT DU VÉHICULE (PP/CP) ////
		lectureInputValues();



		//// TEST DE CHANGEMENT D'ÉTAT ////
		checkTransitionEtat();	// Publication des valeurs avec send_values() UNIQUEMENT si nécessaire



		//// TESTS et CORRECTION DE COURANT DE FUITE, PUBLICATION et CORRECTION D'ERREURS VÉHICULE ////
		errorCheck();



		//// GESTION DES TIMEOUTs ////
		gettimeofday(&timeNow, NULL);

		// Gestion du bouton
		// Positionnement de startKey dans user_key_interrupt()
		if (keyPressed && !WRP_readGpio(PIN_USER_KEY)) {		// Key pressed
			delayKey = timeNow.tv_sec - startKey.tv_sec;
			udelayKey = timeNow.tv_usec - startKey.tv_usec;
			if (udelayKey < 0) {	// Propagation de la retenue
				delayKey -= 1;
				udelayKey += 1000000;
			}
			// Filtrage des rebonds en microsecondes (0,01 sec)
			if (!delayKey && (udelayKey > TIMEOUT_KEYFALSE)) {
				writeNotifFile(NOTIF_BUTTON_PRESSED, "1");
				switchBl(ON, 1);
			}
			// Filtrage des appuis en microsecondes (0,4 sec)
			if (!delayKey && (udelayKey > TIMEOUT_KEYPRESSED) && !stopCharge) {
				if (machine->NAM && machine->state > PREPARING_4 && machine->state < Finishing)
					stopCharge = ON;	// En mode NAM, demande d'arret de charge
			}
			// Redémarrage sur appui très long
			if (delayKey > TIMEOUT_REBOOT) rebootBorne();	
		} else if (keyPressed && WRP_readGpio(PIN_USER_KEY)) {	//key released
			keyPressed = GPIO_LOW;
			sprintf(str_temp, "%ld sec %ld", delayKey, udelayKey);
			writeNotifFile(NOTIF_BUTTON_DELAY, str_temp);
		}
		
		// Temps écoulé depuis le dernier changement d'état
		delayState = timeNow.tv_sec - startNewState.tv_sec;

		// On publie les valeurs régulièrement si pas de changement, plus lentement au bout de TIMEOUT_SEND
		// Positionnement de startSend dans send_values()
		if (delayState > TIMEOUT_STATE_DEEP) {
			if ((timeNow.tv_sec - startSend.tv_sec) > TIMEOUT_SEND_LOW)	send_values(GPIO_LOW);
		} else if (delayState > TIMEOUT_STATE) {
			if ((timeNow.tv_sec - startSend.tv_sec) > TIMEOUT_SEND_MED)	send_values(GPIO_LOW);
		} else
			if ((timeNow.tv_sec - startSend.tv_sec) > TIMEOUT_SEND)	send_values(GPIO_LOW);

		// Annulation de l'authentification si pas de passage en charge.
		if ((machine->state == PREPARING_2 || machine->state == PREPARING_3) && (delayState > TIMEOUT_AUTH))
			if (machine->A) machine->A = GPIO_LOW;

		// Extinction du backlight si pas d'interaction utilisateur.
		// Positionnement de startScreen dans switchBl(ON, x)
		if (backlight_on && (timeNow.tv_sec - startScreen.tv_sec) > TIMEOUT_SCREEN) 
			switchBl(GPIO_LOW, 1);

		// Arret temporisé de la prise latérale
		// Positionnement de startEF dans switchRelaisPriseEf(ON)
	//	if (EF_closed && (timeNow.tv_sec - startEF.tv_sec) > TIMEOUT_EF_ON)
	//		EF_closed = GPIO_LOW;
	}
	// Fin de la boucle principale

	nettoyage_machine();
	return 0;
}
