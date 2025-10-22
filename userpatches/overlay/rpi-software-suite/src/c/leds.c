/**
 * @file machine.c
 * @author	Gilles VIGUIE (gilles.viguie@jerecharge.com)
 * 			Frederic BOMPARD (frederic.bompard@jerecharge.com)
 * @brief Gestion de la machine d'états de la borne v3
 * @version 3.3
 * @date 2024-01-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "leds.h"

#include <sys/inotify.h>
#include <fcntl.h> // library for fcntl function
#include "lib/hubfiles.h"

#define FAULTED 14
int warningEVSE = OFF;
int oldwarningESVE;
int toggleWarning = OFF;

Hubleds *hubleds = NULL;
char * ledsConfig = "122100203321332013211321102102210221022110211121102130212021";
int oldscheme = INIT;
int oldcolor = GREEN;
int oldblOn = ON;
int refresh = ON;
int runLed = true;
int oldledVeilleOn, ledVeilleOn = ON;

char active[10];
char active2[10];


/*buffer to store the data of events*/
int fd,wd;

static void log_trace_leds(char *trace) {
	char buffer[26];
	int millisec;
	time_t t = time(NULL);
	struct timeval tv;
    struct tm tm_info = *localtime(&t);

	gettimeofday(&tv, NULL);
	millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
	if (millisec>=1000) { // Allow for rounding up to nearest second
		millisec -=1000;
		tv.tv_sec++;
	}

	strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", &tm_info);
	printf("%s.%03d - %s\n", buffer, millisec, trace);
}

static void nettoyage_leds() {
	launchSequence(BLUE, CONSTANT, LEVEL_OFF, LEVEL_OFF, hubleds->blOn);
	log_trace_leds("On a terminé la séquence, on va faire appel à free\n");
	usleep(40000);

	free(hubleds);

	/* Step 5. Remove the watch descriptor and close the inotify instance*/
	inotify_rm_watch( fd, wd );
	close( fd );
}

static void stop_leds() {
	runLed = false;
}

static void initLeds(char *initConfig) {
	printf("Appel à malloc() (leds) -> %s\n", __func__);
	if (hubleds == NULL) {
		hubleds = (Hubleds*)malloc(sizeof(Hubleds));
	}

    strcpy(hubleds->ledsConfig, initConfig);
	hubleds->state = INIT;
	hubleds->color = GREEN;
    hubleds->level = LEVEL_NORMAL;
    hubleds->scheme = CONSTANT;
    hubleds->level_veille = LEVEL_MIN;
    hubleds->blOn = ON;
    hubleds->nb_led = 112;
	hubleds->led_order = 3;

	printf("Leds - Lecture du fichier de config :\n");
	char value_token[30];

	readConfFileValue("LEDS_NB", value_token);
    if (strlen(value_token) > 0) {
        if (hubleds) {
			hubleds->nb_led = atoi(value_token);
		}
    }

	readConfFileValue("LEDS_ORDER", value_token);
    if (strlen(value_token) > 0) {
        if (hubleds) {
			hubleds->led_order = atoi(value_token);
		}
    }

	readConfFileValue("LEDS_CONFIG", value_token);
    if (strlen(value_token) > 0) {
        strcpy(hubleds->ledsConfig, value_token);
    }
}

// séletion des parametres en fonction de l'état de la borne
static void setLedsState(EvseState state) {
	int decalageConfig = 4*state;
	hubleds->color = hubleds->ledsConfig[decalageConfig+0] - '0';
	hubleds->scheme = hubleds->ledsConfig[decalageConfig+1] - '0';
	hubleds->level = hubleds->ledsConfig[decalageConfig+2] - '0';
	hubleds->level_veille = hubleds->ledsConfig[decalageConfig+3] - '0';

	printf("Nouvelle config leds (%d) : %i, %i, %i, %i, %s\n",state, hubleds->color, hubleds->scheme, hubleds->level, hubleds->level_veille,  __func__);
}

// Couleurs (0 -> 6) : BLUE, GREEN, RED, ORANGE, YELLOW, WHITE, RGB
// Scheme (0 -> 3) : CONSTANT, BLINK, CHENILLE, PULSE
// Level (0 -> 4) : 0%, 10%, 30%, 50%, 100%

static void launchSequence() {
char command[128];
int evPower = 11;
LedColor color;
LedPower startPower, endPower;

	if (warningEVSE && !toggleWarning && (hubleds->state != FAULTED)) {
		color = ORANGE;
		toggleWarning = ON;
	}
	else color = hubleds->color;

	// Permet une transition de veille à normal ou inversement
	if (hubleds->blOn) {
		// ledVeilleOn permet un allumage programmé
		if (ledVeilleOn) startPower = hubleds->level_veille;
		else startPower = LEVEL_OFF;
		endPower = hubleds->level;
	} else {
		startPower = hubleds->level;
		if (ledVeilleOn) endPower = hubleds->level_veille;
		else endPower = LEVEL_OFF;
	}

	// Si oldscheme et scheme == CONSTANT, avec oldblOn == blOn et oldledVeilleOn == ledVeilleOn
	if ((oldscheme == CONSTANT) && (hubleds->scheme == CONSTANT) && (oldblOn == hubleds->blOn)&& (oldledVeilleOn == ledVeilleOn)) startPower = endPower;
	// Si oldscheme et scheme == CONSTANT avec une couleur différente, startPower permet de faire une transition en partant de 0 au premier passage 
	if ((oldscheme == CONSTANT) && (oldcolor != color)) startPower = 0;

	// Lecture de la puissance appelée en kW pour varier la vitesse de défilement en mode chenille
    readNotifFile(NOTIF_VALUE_POWER, active);
    evPower = (atoi(active) / 1000) + 1;
	
	sprintf(command, "led_sequence %d %d %d %d %d %d %d", hubleds->nb_led, hubleds->led_order, color, hubleds->scheme, endPower, startPower, evPower);
	system( command );
}

int main(int argc, char *argv[]) {

	// on configure l'execution de la fonction interruption si ctrl+C
	printf("Inscription de la routine de nettoyage\n");
	signal(SIGKILL, stop_leds);
	signal(SIGABRT, stop_leds);
	signal(SIGINT, stop_leds);
	signal(SIGTERM, stop_leds);
	signal(SIGQUIT, stop_leds);
	/* Step 1. Initialize inotify */
	fd = inotify_init();
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {  // error checking for fcntl
		printf("Problème fcntl\n");
        return EXIT_FAILURE;
	}
	/* Step 2. Add Watch */
	wd = inotify_add_watch(fd,notif_path,IN_MODIFY | IN_CREATE | IN_DELETE);

	if (wd < 0)	printf("Could not watch : %s\n",notif_path);
	else	printf("Watching : %s\n",notif_path);

	// Initialisation des LEDs
	initLeds(ledsConfig);
	launchSequence(hubleds->color, hubleds->scheme, hubleds->level, hubleds->level_veille, hubleds->blOn);
	printf("Boucle hubleds initié, %s\n", hubleds->ledsConfig);

	while(runLed) {
		int i=0,length;
		char buffer[BUF_LEN];

		oldscheme = hubleds->scheme;			// sauvegarde de l'état courant des LEDs
		oldblOn = hubleds->blOn;
		oldwarningESVE = warningEVSE;
		oldcolor = hubleds->color;
		oldledVeilleOn = ledVeilleOn;

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
						//printf( "The file %s was modified.\n", event->name );
						if (!strcmp(event->name, NOTIF_BACKLIGHT)){
							readNotifFile(NOTIF_BACKLIGHT, active);
							hubleds->blOn = !strcmp(active, "On");	// Les LEDs sont en mode normal
						}
						else if (!strcmp(event->name, NOTIF_REDUCE_BL)){
							readNotifFile(NOTIF_REDUCE_BL, active);
							hubleds->blOn = strcmp(active, "On");	// Les LED sont en mode veille
						}
						else if (!strcmp(event->name, NOTIF_STATE_EVSE)){
							readNotifFile(NOTIF_STATE_EVSE, active);
							hubleds->state = atoi(active);	// Les LEDs correspondent à l'état courant de la borne
							readNotifFile(NOTIF_VALUE_LATEST_VE_ERROR, active);
							if (atoi(active)) setLedsState(FAULTED);// Erreur véhicule
							else setLedsState(hubleds->state);
							toggleWarning = OFF;				// En cas de Warning, permet de limiter au premier passage
						}
//						else if (!strcmp(event->name, NOTIF_WARNING)){
//							readNotifFile(NOTIF_WARNING, active);
//							if (atoi(active)) warningEVSE = ON;		// Warning maintenance
//							else warningEVSE = OFF;
//						}
						else if (!strcmp(event->name, NOTIF_CONF)) {
							initLeds(ledsConfig);
						}
						else if (!strcmp(event->name, NOTIF_STOP)){
							stop_leds();
						}
					}
				}
			}
			i += EVENT_SIZE + event->len;
		}

		// Si l'allumage et l'extinction en veille sont programmées, on compare avec l'heure locale
		// Les chaines sont dans le format 24 heures HH:MM
		// Si le fichier n'existe pas, la chaine est vide
		if (ledVeilleOn) {
			readOneLineValue("timeledoff.conf", active, OFF);
			readNotifFile(NOTIF_LOCAL_TIME, active2);
			if (strcmp(active2, active) > 0) ledVeilleOn = !ledVeilleOn;
		}
		if (!ledVeilleOn) {
			readOneLineValue("timeledon.conf", active, OFF);
			readNotifFile(NOTIF_LOCAL_TIME, active2);
			if (strcmp(active2, active) > 0) ledVeilleOn = !ledVeilleOn;
		}
		

		// On ne rafraichit pas si les LEDs sont et étaient fixes, avec la couleur et l'intensité identiques
		// On y ajoute l'allumage et l'extinction programmées
		if ((hubleds->scheme != CONSTANT) || (oldscheme != hubleds->scheme) || (oldcolor != hubleds->color) || (oldblOn != hubleds->blOn) || (oldwarningESVE != warningEVSE) || (oldledVeilleOn != ledVeilleOn))
			launchSequence();
		
		usleep(50000);

	}
	
	nettoyage_leds();

}