/**
 * @file network_tst.c
  * @author	Frederic BOMPARD (frederic.bompard@jerecharge.com)
 * @brief Lecture de l'état du réseau pour ne pas perturber le rete de l'application
 * @version 3.3.2
 * @date 2024-11-30
 * 
 * @copyright Copyright (c) 2025
 *
 */
 //#include <maxminddb.h>
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
 #include "network_tst.h"
// #include <MCP3202.h>
// #include <gpio.h>
// #include <dma.h>
// #include <pwm.h>
#include "lib/BSP_wrappers.h"

int mainLoop = true;

/*buffer to store the data of events*/
int fd,wd;

// URL du serveur OCPP éventuel;
char ocppServerURL[50];
char str_temp[BUFFER_SIZE];

// Test de connection
int oldResult, result;

// Pour récupérer l'heure UTC à la nanoseconde
struct timespec now;

// Recupération de la date et de l'heure locales
time_t timestamp; 
int old_tday, old_tmin;
struct tm *t; 


static void nettoyage_machine() {

	/* Step 5. Remove the watch descriptor and close the inotify instance*/
	inotify_rm_watch( fd, wd );
	close( fd );
}

static void stop_signal() {
	printf("On stoppe les threads");
	mainLoop = false;
}

// Test reseau
// 0: Offline; 1: Online; 2: OCPP Off; 3: OCPP On;
static void testNetwork() {
char commande[64];
 
    // Envoi d'un ping vers localhost pour valider l'intégrité de l'interface réseau
	// System() renvoie 0 si OK
	result = !system("ping -c 1 -q localhost");
 
    // Envoi d'un ping vers google.fr
	if (result) result = !system("ping -c 1 -q google.fr");
	else printf("LAN NOT connected");
	
	if (result)	{
		// Si nécessaire, envoi d'un ping vers le serveur OCPP
		readConfFileValue("AUTHENTICATION_TYPE", str_temp);
		if (!strcmp(str_temp, "ocpp")) {
			readConfFileValue("SERVER_ADDRESS", ocppServerURL);
			sprintf(commande, "ping -c 1 -q %s", ocppServerURL);
			if (system(commande))			result = 2;
			else 							result = 3;		// Serveur OCPP Online
		}
	}

	if ((result != oldResult)) {
		sprintf(str_temp, "%d", result);	// Publication de l'état du réseau s'il est différent du précédent
		writeNotifFile(NOTIF_STATE_CONNECTIVITY, str_temp);

		// On capture le temps actuel
		timespec_get(&now, TIME_UTC);

		// On transforme la partie exprimée en seconde en une chaîne de caractères.
		strftime(str_temp, BUFFER_SIZE, "%D %T", gmtime(&now.tv_sec));

		// On affiche le temps avec une précision à la nano-seconde 
		printf("%s.%09ld UTC ", str_temp, now.tv_nsec);
		switch (result) {
		case 3:
			printf("OCPP server connected\n");
			break;
		case 2:
			printf("OCPP server NOT connected\n");
			break;
		case 1:
			printf("WAN connected\n");
			break;
		case 0:
			printf("WAN NOT connected\n");
		}
	}
	// Sauvegarde de l'état précédent
	oldResult = result;
}

// Écriture de la date et de l'heure locale pour les évènements horodatés
static void localDT() {

// TO DO: check local timezone automatically from geoip2
// https://github.com/maxmind/libmaxminddb/blob/main/README.md

	timestamp = time(NULL);
	t = localtime(&timestamp);

	// Date du jour AAAA-MM-JJ
	if (t->tm_mday != old_tday) {
		strftime(str_temp, sizeof(str_temp), "%F", t); 
		writeNotifFile(NOTIF_LOCAL_DATE, str_temp);
	}

	// heure du jour HH:MM
	if (t->tm_min != old_tmin) {
		strftime(str_temp, sizeof(str_temp), "%R", t); 
		writeNotifFile(NOTIF_LOCAL_TIME, str_temp);
	}

	old_tday = t->tm_mday;
	old_tmin = t->tm_min;
}

int main(int argc, char *argv[]) {

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

	// Initialisation
	oldResult = -1;
	old_tday = 32;	// out of range
	old_tmin = 60;	// out of range

	// debut de la boucle infinie
	while (mainLoop) {
		int i=0,length;
		char buffer[BUF_LEN];

		//// TEST RÉCURRENT DU RÉSEAU ////

		testNetwork();

		//// ÉCRITURE DATE ET HEURE LOCALES ////

		localDT();

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
						if (!strcmp(event->name, NOTIF_STOP)) {
							stop_signal();
						}
					}
				}
			}
			i += EVENT_SIZE + event->len;
		}
		
		// Attente 10 sec
		sleep(10);

	}
	// Fin de la boucle principale

	nettoyage_machine();
	return 0;
}
