/**
 * @file	rfid.c
 * @author	Gilles VIGUIE (gilles.viguie@jerecharge.com)
 *			Frederic BOMPARD (frederic.bompard@jerecharge.com)
 * @brief Gestion de la lecture RFID de la borne hubload
 * @version 3.2
 * @date 2024-07-09
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
#include <sys/time.h>
#include <signal.h>

#include "lib/hubtypes.h"
#include "lib/hubfiles.h"
#include "rfid.h"

#include <sys/inotify.h>
#include <fcntl.h> // library for fcntl function

HubRfid *rfid = NULL;
int runRfid;
// Gestion du timeout
struct timeval startReadRfid, timeNow;
#define TIMEOUT_READ 49
int readDone = OFF;		// Lecture RFID effectuée

/* buffer to store the data of events */
int fd, wd;
char active[5];

static void rfid_init() {
    printf("Appel à malloc() (rfid) -> %s\n", __func__);
    rfid = (HubRfid*)malloc(sizeof(HubRfid));
    rfid->uid_len = 0;
    rfid->scan_activated = ON;
	runRfid = true;
	readDone = OFF;
}

static void lectureRfid() {
	// Lecture de l'activation
	readNotifFile(NOTIF_ACTIVATE_SCAN, active);
	rfid->scan_activated = strcmp(active, "Off");	// Lecture activée si différent de "Off"

	// delai de 20 sec éventuel
	if (!strcmp(active, "delayedOn")) sleep(20); 

	// Lecture si elle est activée et si elle n'a pas déjà été effectuée
	if (rfid->scan_activated && !readDone) {
		system("scan_rfid");
		gettimeofday(&startReadRfid, NULL);
	}

}

static void stop_signal() {
    runRfid = false;
}

static void nettoyage_rfid() {
    free(rfid);
	/* Step 5. Remove the watch descriptor and close the inotify instance*/
	inotify_rm_watch( fd, wd );
	close( fd );
}

int main(int argc, char *argv[]) {
	// on configure l'execution de la fonction interruption si ctrl+C
	signal(SIGINT, stop_signal);

	/* Step 1. Initialize inotify */
	fd = inotify_init();
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {  // error checking for fcntl
		printf("Problème fcntl\n");
        return EXIT_FAILURE;
	}
	/* Step 2. Add Watch */
	wd = inotify_add_watch(fd,notif_path,IN_MODIFY);

	if (wd < 0)	printf("Could not watch : %s\n",notif_path);
	else		printf("Watching : %s\n",notif_path);

	rfid_init();

	while (runRfid) {
		int i=0,length;
		char buffer[BUF_LEN];
	
		usleep(500000);		// Attente de 0,5 sec

		/* Step 3. Read buffer*/
		length = read(fd,buffer,BUF_LEN);

		/* Step 4. Process the events which has occurred */
		while (i<length) {
			struct inotify_event *event = (struct inotify_event *) &buffer[i];

			if (event->len) {
				if (event->mask & IN_MODIFY) {
					if (event->mask & IN_ISDIR) {
						//printf( "The directory %s was modified.\n", event->name );
					} else {
						if (!strcmp(event->name, NOTIF_ACTIVATE_SCAN))	{
							printf( "File %s was modified.\n", event->name);
							readDone = OFF;		// Remise à 0 de la lecture
						}
						else if (!strcmp(event->name, NOTIF_VALUE_SCAN)) {
							printf( "File %s was modified.\n", event->name);
							readDone = ON;		// Lecture effectuée 
						}
						else if (!strcmp(event->name, NOTIF_FAIL_SCAN))	{
							printf( "File %s was modified.\n", event->name);
							writeNotifFile(NOTIF_ACTIVATE_SCAN, "On");	// Annule un éventuel delayedOn 
							readDone = OFF;		// Remise à 0 de la lecture
						}
						else if (!strcmp(event->name, "STOP"))	stop_signal();
					}
				}
			}
			i += EVENT_SIZE + event->len;
		}

	    lectureRfid();

		// Si la borne ne change pas d'état (et écrit sur ACTIVATE_SCAN), c'est que l'application java ne répond pas
		if (readDone) {
			gettimeofday(&timeNow, NULL);
			if ((timeNow.tv_sec - startReadRfid.tv_sec) > TIMEOUT_READ)
				writeNotifFile(NOTIF_RESTART, "Timeout Java");
		}
	}
    nettoyage_rfid();
	return EXIT_SUCCESS;
}
