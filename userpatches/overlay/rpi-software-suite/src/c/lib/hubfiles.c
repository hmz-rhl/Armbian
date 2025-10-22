#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "hubtypes.h"
#include "hubfiles.h"

void readConfFileValue(const char *key, char *value) {
 	FILE* file = fopen("/usr/share/hubload/params.conf","r");
	if(file == NULL){
		printf("Erreur de lecture du fichier de configuration\n");
		value[0] = '\0';
        return;
	}
	else{
		//printf("Fichier de config ouvert.\n");
   		const char *delimiter_characters = ";";
		char buffer[ BUFFER_SIZE ];
		char *name_token;
		char *value_token;
		// Read each line into the buffer
		while( fgets(buffer, BUFFER_SIZE, file) != NULL ){
			buffer[strcspn(buffer, "\r\n")] = 0;
			//printf("Ligne : %s\n", buffer);

			// Gets each token as a string and prints it
			name_token = strtok( buffer, delimiter_characters );
			//printf("name_token = %s\n", name_token);
			if( name_token != NULL ){
				//printf( "%d->'%s'\n",i, last_token );
				value_token = strtok( NULL, delimiter_characters );
				//printf("value_token = %s\n", value_token);

				if (value_token != NULL) {
					if (strcmp(name_token, key) == 0) {
						strcpy(value, value_token);
					}
				}
			}
		}

		if( ferror(file) ){
			perror( "The following error occurred" );
		}

      	fclose( file );
		//printf("Fin Lecture du fichier de config\n");
	}   
}

void readNotifFile(const char *notifName, char *value) {
	char fileName[50];
	sprintf(fileName, "notif/%s", notifName);
	readOneLineValue(fileName, value, 0);
}

void writeNotifFile(const char *notifName, char *value) {
	char fileName[50];
	sprintf(fileName, "notif/%s", notifName);
	writeOneLineValue(fileName, value, 0);
}

void readOneLineValue(const char *fileName, char *value, int removeFileAfterRead) {
    char filePath[128];
    strcpy(filePath, "/usr/share/hubload/");
    strcat(filePath, fileName);

    FILE* file = fopen(filePath,"r");
    if (file != NULL) {
        fscanf(file,"%s",value);
        fclose(file);

        if (removeFileAfterRead) remove(filePath);
    }
    else {
        value[0] = '\0';
    }
}

void writeOneLineValue(const char *fileName, const char *value, int createAndErase) {
    char filePath[128];
    strcpy(filePath, "/usr/share/hubload/");
    strcat(filePath, fileName);

    char openFlag[3];
    if (createAndErase) {
        strcpy(openFlag, "w+");
    }
    else {
        strcpy(openFlag, "w");
    }

    FILE* file = fopen(filePath, openFlag);
    if (file != NULL) {
        fprintf(file,"%s",value);
        fclose(file);
    }
}