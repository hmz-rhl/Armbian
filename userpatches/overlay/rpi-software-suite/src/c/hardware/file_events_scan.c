
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h> // library for fcntl function
#include "../lib/hubfiles.h"

int mainLoop = 1;

/*buffer to store the data of events*/
int fd,wd;

static void stop_signal() {
    mainLoop = 0;
}


static void log_trace(char *trace, char *value) {
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
	printf("%s.%03d - The file %s was modified : %s.\n", buffer, millisec, trace, value);
}


int main(int argc, char *argv[]) {
    
    signal(SIGINT, stop_signal);

    	/* Step 1. Initialize inotify */
	fd = inotify_init();
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {  // error checking for fcntl
		printf("Problème fcntl\n");
	   	return EXIT_FAILURE;
	}
	/* Step 2. Add Watch */
	wd = inotify_add_watch(fd,notif_path,IN_MODIFY | IN_CREATE | IN_DELETE);

	if(wd==-1){
		printf("Could not watch : %s\n",notif_path);
	}
	else{
		printf("Watching : %s\n",notif_path);
	}

	// debut de la boucle infini
	log_trace("On démarre la loop principale", "");
	while(mainLoop) {

		int i=0,length;
		char buffer[BUF_LEN];

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
                        char value[250];
                        readNotifFile(event->name, value);
						
                        log_trace( event->name, value );

						
						if (!strcmp(event->name, NOTIF_STOP)){
							stop_signal();
						}
					}
				}
			}
			i += EVENT_SIZE + event->len;
		}
        usleep(10000);
    }
    return EXIT_SUCCESS;
}


