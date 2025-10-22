/**
 * @file    display.c
 * @author	Gilles VIGUIE (gilles.viguie@jerecharge.com)
 *			Frederic BOMPARD (frederic.bompard@jerecharge.com)
 * @brief Gestion de l'affichage de la borne hubload
 * @version 3.2
 * @date 2024-07-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "lib/hubtypes.h"
#include "lib/hubfiles.h"
#include "display.h"
#include <sys/inotify.h>
#include <fcntl.h> // library for fcntl function

#if 1
#define HARDWARE 1
#else
#define HARDWARE 0
#endif

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 800

#define QRCODE_WIDTH 180
#define QRCODE_HEIGHT 180

#define FLAG_WIDTH 60
#define FLAG_HEIGHT 40

#define MAX_EVENTS 1024  /* Maximum number of events to process*/
#define LEN_NAME 16  /* Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE  ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME ))

// Gestion des delais
struct timeval startTempScreen, timeNow;
time_t delayTempScreen;

int tempScreen = OFF;	    // Vrai pour déclencher l'affichage temporaire
#define TIMEOUT_TEMP 2		// Timeout (-1 sec) du l'affichage temporaire 
int errorRfid = OFF;
int analyseRfid = OFF;

int DetailedCharge = OFF;   // Pour un affichage détaillé de la charge

int run = true;

SDL_Texture *textureFond;
SDL_Texture *textureFlag;
SDL_Texture *textureQRCode;
SDL_Texture *texteConnectivite;
SDL_Texture *texteTemperature;
SDL_Texture *texteIdBorne;
SDL_Texture *texteSWversion;
SDL_Texture *labelPuissance;
SDL_Texture *textePuissance;
SDL_Texture *labelEnergie;
SDL_Texture *texteEnergie;
SDL_Texture *labelRecup;
SDL_Texture *texteRecup;
SDL_Texture *labelCourant;
SDL_Texture *texteCourant;
SDL_Texture *labelTension;
SDL_Texture *texteTension;
SDL_Texture *labelCourantMax;
SDL_Texture *texteCourantMax;
SDL_Texture *labelCompteurExterne;
SDL_Texture *texteCompteurExterne;
SDL_Texture *labelCable;
SDL_Texture *texteCable;
SDL_Texture *labelStatus;
SDL_Texture *labelWarning;
SDL_Texture *labelDebut;
SDL_Texture *texteDebut;
SDL_Texture *labelDuree;
SDL_Texture *texteDuree; // Textures textes début / duree

SDL_Event event;
SDL_Rect positionFond = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
SDL_Rect positionQRCode = { 0, 0, QRCODE_WIDTH, QRCODE_HEIGHT};
SDL_Rect positionFlag = { 0, 0, FLAG_WIDTH, FLAG_HEIGHT};

HubDisplay *display = NULL;
SDL_Context *context = NULL;
int runDisplay = ON;
    
char fond_name[50];
char flag_name[50];

char str_puis[15];
char str_recup[15];
char str_courant[15];
char str_tension[15];
char str_courant_max[15];
char str_cable[15];
char str_connectivite[27];
char str_temperature[15];
char str_cpt_ext[15];
char str_ID[27];
char str_versionSW[27];

SDL_Rect t_position;
SDL_Point center;   // Place le "doigt" au centre de la texture
double angle = 90;  // Rotation du texte en mode portrait

/*buffer to store the data of events*/
int fd,wd;
char active[25];


static void init_display_struct(char* idBorne) {
    strcpy(str_cpt_ext, " - ");
    display->toContinue = ON;
    display->status = INIT;
    display->charging = OFF;
    display->maxAmp = 0;
    display->nb_phases = MONO;
    display->voltage = 230;
    display->connectivite = -1;
    display->temperature = 0.0;
    display->ventilation = OFF;
    display->maintenance = OFF;
    strcpy(display->idBorne, idBorne);
    strcpy(display->SWversion, str_versionSW);

    textureFond = NULL;
    texteTemperature = NULL;
    texteIdBorne = NULL;
    texteSWversion = NULL;
    labelPuissance = NULL;
    textePuissance = NULL;
    labelEnergie = NULL;
    texteEnergie = NULL;
    labelRecup = NULL;
    texteRecup = NULL;
    labelCourant = NULL;
    texteCourant = NULL;
    labelTension = NULL;
    texteTension = NULL;
    labelCourantMax = NULL;
    texteCourantMax = NULL;
    labelCompteurExterne = NULL;
    texteCompteurExterne = NULL;
    labelCable = NULL;
    texteCable = NULL;
    labelStatus = NULL;
    labelWarning = NULL;
    labelDebut = NULL;
    texteDebut = NULL;
    labelDuree = NULL;
    texteDuree = NULL;
}


static void init_context_struct() {
    context->couleurLabel.a = 255; context->couleurLabel.r = 255; context->couleurLabel.g = 255; context->couleurLabel.b = 255;
    context->couleurTexte.a = 255; context->couleurTexte.r = 200; context->couleurTexte.g = 200; context->couleurTexte.b = 200;
    context->couleurBlue.a = 255; context->couleurBlue.r = 13; context->couleurBlue.g = 109; context->couleurBlue.b = 169;
    context->couleurViolet.a = 255; context->couleurViolet.r = 58; context->couleurViolet.g = 28; context->couleurViolet.b = 157;
    context->couleurOrange.a = 255; context->couleurOrange.r = 255; context->couleurOrange.g = 140; context->couleurOrange.b = 0;
    context->couleurNoir.a = 255; context->couleurNoir.r = 0; context->couleurNoir.g = 0; context->couleurNoir.b = 0;
    context->couleurRouge.a = 255; context->couleurRouge.r = 169; context->couleurRouge.g = 50; context->couleurRouge.b = 38;
    context->couleurVerte.a = 255; context->couleurVerte.r = 40; context->couleurVerte.g = 180; context->couleurVerte.b = 99;
    context->ecran = NULL;
    context->francais = 1;
    context->renderer = NULL;

    strcpy(context->prefix_surfaces[0], "/opt/hubload/resources/img/rot/I_");
    strcpy(context->prefix_surfaces[1], "/opt/hubload/resources/img/rot/A_");
    strcpy(context->prefix_surfaces[2], "/opt/hubload/resources/img/rot/P0_");
    strcpy(context->prefix_surfaces[3], "/opt/hubload/resources/img/rot/P1_");
    strcpy(context->prefix_surfaces[4], "/opt/hubload/resources/img/rot/P2_");
    strcpy(context->prefix_surfaces[5], "/opt/hubload/resources/img/rot/P3_");
    strcpy(context->prefix_surfaces[6], "/opt/hubload/resources/img/rot/P4_");
    strcpy(context->prefix_surfaces[7], "/opt/hubload/resources/img/rot/C_");
    strcpy(context->prefix_surfaces[8], "/opt/hubload/resources/img/rot/C_");
    strcpy(context->prefix_surfaces[9], "/opt/hubload/resources/img/rot/C_");
    strcpy(context->prefix_surfaces[10], "/opt/hubload/resources/img/rot/Sev_");
    strcpy(context->prefix_surfaces[11], "/opt/hubload/resources/img/rot/Sevse_");
    strcpy(context->prefix_surfaces[12], "/opt/hubload/resources/img/rot/Q_");
    strcpy(context->prefix_surfaces[13], "/opt/hubload/resources/img/rot/U_");
    strcpy(context->prefix_surfaces[14], "/opt/hubload/resources/img/rot/F_");

    readOneLineValue("language.conf", active, 0);
    context->francais = !strcmp(active, "fr");
    
    if (TTF_Init() < 0) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[DEBUG] - %s > %s", __func__, TTF_GetError());

    context->fontStatus = TTF_OpenFont("/opt/hubload/resources/ttf/Arial_Bold_Italic.ttf", 35);
    context->fontTexte = TTF_OpenFont("/opt/hubload/resources/ttf/Arial.ttf", 30);
    context->fontBlue = TTF_OpenFont("/opt/hubload/resources/ttf/Arial.ttf", 20);
    context->fontViolet = TTF_OpenFont("/opt/hubload/resources/ttf/Arial_Bold.ttf", 40);
    context->fontNoir = TTF_OpenFont("/opt/hubload/resources/ttf/Arial.ttf", 18);
    context->fontLabel = TTF_OpenFont("/opt/hubload/resources/ttf/Arial_Bold.ttf", 30);
    context->fontPower = TTF_OpenFont("/opt/hubload/resources/ttf/Arial_Bold.ttf", 150);
    context->fontSidePower = TTF_OpenFont("/opt/hubload/resources/ttf/Arial.ttf", 60);
}

static void readConfFile(void) {
char value_url[256];
char command_text[450];

	printf("Display -> Lecture du fichier de config :\n");

    readConfFileValue("MAX_PHASE_CURRENT", active);
    if (strlen(active)) display->maxAmp = atoi(active);

    readConfFileValue("PHASE_NUMBER", active);
    if (strlen(active)) display->nb_phases = atoi(active);

    readConfFileValue("VOLTAGE", active);
    if (strlen(active)) display->voltage = atof(active);
    // On ne supporte que les tensions secteur dominantes pour le moment
    if (display->voltage >= 200) display->voltage = 230;
    else display->voltage = 115;

    readConfFileValue("MID_S0_ON", active);
    if (!strcmp(active, "On")) strcpy(str_cpt_ext, "MID s0");

    readConfFileValue("QRCODE_VALUE", value_url);
    if (strlen(value_url) < 13) strcpy(value_url, "https://jerecharge.com");

    sprintf(command_text, "sudo qrencode -o /tmp/jrc.png -s 7 -m 5 -d 96 --foreground 0D6DA9 '%s'",value_url);
    system(command_text);
    system("sudo convert /tmp/jrc.png -type truecoloralpha /usr/share/hubload/qrc.bmp");
    system("sudo rm -f /tmp/jrc.png");

    readNotifFile(NOTIF_VALUE_ID_EVSE, display->idBorne);

    readNotifFile(NOTIF_VALUE_SW_EVSE, display->SWversion);
 
    readNotifFile(NOTIF_STATE_CONNECTIVITY, active);
    display->connectivite = atoi(active);

	// Affichage détaillé en charge
    readOneLineValue("detailedDisplay.conf", active, OFF);
    DetailedCharge = !strcmp(active, "On");
}

static void initDisplay() {  
    printf("malloc Hubdisplay\n");
    display = (HubDisplay*)malloc(sizeof(HubDisplay));
    init_display_struct("VOID");

    printf("malloc SDL_Context\n");
    context = (SDL_Context*)malloc(sizeof(SDL_Context));
    init_context_struct(context);

    display->connectivite = -1;
    strcpy(display->idBorne, "YYBBMMnnnn");
    strcpy(display->SWversion, "x.x.x");

    readConfFile();

    strcpy(display->str_debut, "__:__");
    strcpy(display->str_temps, "__h__m");

    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

    context->ecran = SDL_CreateWindow("HUBLOAD", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if(context->ecran) {
        if (HARDWARE) context->renderer = SDL_CreateRenderer(context->ecran, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        else context->renderer = SDL_CreateRenderer(context->ecran, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);

        if ( context->renderer ) {
            SDL_RendererInfo infoRenderer;
            SDL_GetRendererInfo(context->renderer, &infoRenderer);

            if (infoRenderer.flags & SDL_RENDERER_ACCELERATED) SDL_Log("Le rendu est géré par la carte graphique...");

            if (infoRenderer.flags & SDL_RENDERER_SOFTWARE) SDL_Log("Le rendu est géré par le soft...");

            if (infoRenderer.flags & SDL_RENDERER_TARGETTEXTURE) SDL_Log("Le rendu est autorisé sur des texture...");

            SDL_Log("On va initialiser la couleur");

            textureSurfaceFond();
        }
        else fprintf(stderr, "echec de création du renderer : %s", SDL_GetError());
    }
    else fprintf(stderr, "Erreur creation window : %s", SDL_GetError());

    printf("L'affichage est initialisé.\n");
}

static void nettoyage_display() { 
    run = 0;
    freeMemSDLObjects();

    SDL_DestroyRenderer(context->renderer);

    SDL_DestroyWindow(context->ecran);

    TTF_CloseFont(context->fontBlue);
    TTF_CloseFont(context->fontLabel);
    TTF_CloseFont(context->fontNoir);
    TTF_CloseFont(context->fontStatus);
    TTF_CloseFont(context->fontTexte);
    TTF_CloseFont(context->fontViolet);

    TTF_Quit();
    SDL_Quit();
    
	/* Step 5. Remove the watch descriptor and close the inotify instance*/
	inotify_rm_watch( fd, wd );
	close( fd );
   
    free(display);
    free(context);
}

static void freeMemSDLObjects() {
    if (texteConnectivite) SDL_DestroyTexture(texteConnectivite);
    if (texteTemperature) SDL_DestroyTexture(texteTemperature);
    if (texteIdBorne) SDL_DestroyTexture(texteIdBorne);
    if (texteSWversion) SDL_DestroyTexture(texteSWversion);

    if (labelPuissance) SDL_DestroyTexture(labelPuissance);
    if (textePuissance) SDL_DestroyTexture(textePuissance);
    if (labelEnergie) SDL_DestroyTexture(labelEnergie);
    if (texteEnergie) SDL_DestroyTexture(texteEnergie);
    if (labelRecup) SDL_DestroyTexture(labelRecup);
    if (texteRecup) SDL_DestroyTexture(texteRecup);

    if (labelCourant) SDL_DestroyTexture(labelCourant);
    if (texteCourant) SDL_DestroyTexture(texteCourant);
    if (labelTension) SDL_DestroyTexture(labelTension);
    if (texteTension) SDL_DestroyTexture(texteTension);
    if (labelCourantMax) SDL_DestroyTexture(labelCourantMax);
    if (texteCourantMax) SDL_DestroyTexture(texteCourantMax);

    if (labelCompteurExterne) SDL_DestroyTexture(labelCompteurExterne);
    if (texteCompteurExterne) SDL_DestroyTexture(texteCompteurExterne);
    if (labelCable) SDL_DestroyTexture(labelCable);
    if (texteCable) SDL_DestroyTexture(texteCable);
    if (labelStatus) SDL_DestroyTexture(labelStatus);
    if (labelWarning) SDL_DestroyTexture(labelWarning);

    if (labelDebut) SDL_DestroyTexture(labelDebut);
    if (texteDebut) SDL_DestroyTexture(texteDebut);
    if (labelDuree) SDL_DestroyTexture(labelDuree);
    if (texteDuree) SDL_DestroyTexture(texteDuree);

    if (textureFond) SDL_DestroyTexture(textureFond);
    if (textureFlag) SDL_DestroyTexture(textureFlag);
}

static void prepareToQuit() {
    display->toContinue = OFF;
}

static void formatString(char *display, char *value, double totalSize) {
    int tailleDisplay = strlen(value);

    int toAdd = (totalSize - tailleDisplay) / 2;

    for (int i = 0; i < toAdd; i++) display[i] = ' ';
    for (int i = 0; i < tailleDisplay; i++) display[toAdd + i] = value[i];
    for (int i = 0; i < toAdd; i++) display[toAdd + tailleDisplay + i] = ' ';
    display[toAdd + tailleDisplay + toAdd] = 0;
}

static void formatValue(char *display, double value, char *unit, int precision, double totalSize) {
    double converter = precision * 10.0;
    
    double oneDigitValue = (((int)(value*converter))*1.0)/converter;
    int zeroDigitValue = 0;
    if (precision == 0) zeroDigitValue = (int)value;

    char representation[50];
    if (precision == 0)      sprintf(representation, "%d %s", zeroDigitValue, unit);
    else if (precision == 1) sprintf(representation, "%.1f %s", oneDigitValue, unit);
    else if (precision == 2) sprintf(representation, "%.2f %s", oneDigitValue, unit);
    else if (precision == 3) sprintf(representation, "%.3f %s", oneDigitValue, unit);
    else                     sprintf(representation, "%.4f %s", oneDigitValue, unit);

    int tailleDisplay = strlen(representation);

    int toAdd = (totalSize - tailleDisplay) / 2;

    for (int i = 0; i < toAdd; i++) display[i] = ' ';
    for (int i = 0; i < tailleDisplay; i++) display[toAdd + i] = representation[i];
    for (int i = 0; i < toAdd; i++) display[toAdd + tailleDisplay + i] = ' ';
    display[toAdd + tailleDisplay + toAdd] = 0;
}

// Affiche le fond d'écran en fonction de display->status
static void textureSurfaceFond() {
    strcpy(fond_name, context->prefix_surfaces[display->status]);

    if (context->francais)  strcat(fond_name, "fr.bmp");
    else                    strcat(fond_name, "en.bmp");

    SDL_Surface *surface = SDL_LoadBMP(fond_name);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[textureText - textureSurfaceFond %s] - %s > %s", fond_name, __func__, TTF_GetError());
        return;
    }
    if (textureFond) SDL_UpdateTexture(textureFond, &positionFond, surface->pixels, surface->pitch);
    else textureFond = SDL_CreateTextureFromSurface(context->renderer, surface);

    SDL_FreeSurface(surface);
    if (!textureFond) fprintf(stderr, "Echec du rendu de la surface %s : %s ", fond_name, SDL_GetError());
    SDL_RenderCopy(context->renderer, textureFond, NULL, &positionFond);
}

static void texture(char *texte, LabelTexture textureId) {
    if (texte) {
        switch (textureId) {
        case TEXTE_STATUS:
            if (labelStatus) updateText(labelStatus, texte, context->fontStatus, context->couleurViolet, 630, 400, textureId);
            else labelStatus = textureText(texte, context->fontStatus, context->couleurViolet, 630, 400, textureId);
            break;
        case TEXTE_WARNING:
            if (labelWarning) updateText(labelWarning, texte, context->fontStatus, context->couleurOrange, 570, 400, textureId);
            else labelWarning = textureText(texte, context->fontStatus, context->couleurOrange, 570, 400, textureId);
            break;
        case TEXTE_VERSION_SW:
            if (texteSWversion) updateText(texteSWversion, texte, context->fontNoir, context->couleurNoir, 80, 150, textureId);
            else texteSWversion = textureText(texte, context->fontNoir, context->couleurNoir, 80, 150, textureId);
            break;
        case TEXTE_IDBORNE:
            if (texteIdBorne) updateText(texteIdBorne, texte, context->fontNoir, context->couleurNoir, 50, 150, textureId);
            else texteIdBorne = textureText(texte, context->fontNoir, context->couleurNoir, 50, 150, textureId);
            break;
        case TEXTE_TEMPERATURE:
            if (texteTemperature) updateText(texteTemperature, texte, context->fontNoir, context->couleurNoir, 80, 650, textureId);
            else texteTemperature = textureText(texte, context->fontNoir, context->couleurNoir, 80, 650, textureId);
            break;
        case TEXTE_CONNECTIVITE:
            switch (display->connectivite) {
            case 1:
                if (texteConnectivite) updateText(texteConnectivite, texte, context->fontNoir, context->couleurVerte, 50, 650, textureId);
                else texteConnectivite = textureText(texte, context->fontNoir, context->couleurVerte, 50, 650, textureId);
                break;
            case 2:
                if (texteConnectivite) updateText(texteConnectivite, texte, context->fontNoir, context->couleurRouge, 50, 650, textureId);
                else texteConnectivite = textureText(texte, context->fontNoir, context->couleurRouge, 50, 650, textureId);
                break;
            case 3:
                if (texteConnectivite) updateText(texteConnectivite, texte, context->fontNoir, context->couleurVerte, 50, 650, textureId);
                else texteConnectivite = textureText(texte, context->fontNoir, context->couleurVerte, 50, 650, textureId);
                break;
            case 0:
                if (texteConnectivite) updateText(texteConnectivite, texte, context->fontNoir, context->couleurRouge, 50, 650, textureId);
                else texteConnectivite = textureText(texte, context->fontNoir, context->couleurRouge, 50, 650, textureId);
                break;
            default:
                if (texteConnectivite) updateText(texteConnectivite, texte, context->fontNoir, context->couleurNoir, 50, 650, textureId);
                else texteConnectivite = textureText(texte, context->fontNoir, context->couleurNoir, 50, 650, textureId);
                break;
            }
            break;
        case LABEL_DEBUT:
            if (labelDebut) updateText(labelDebut, texte, context->fontBlue, context->couleurBlue, 1050, 530, textureId);
            else labelDebut = textureText(texte, context->fontBlue, context->couleurBlue, 1050, 530, textureId);
            break;
        case TEXTE_DEBUT:
            if (texteDebut) updateText(texteDebut, texte, context->fontViolet, context->couleurViolet, 1020, 530, textureId);
            else texteDebut = textureText(texte, context->fontViolet, context->couleurViolet, 1020, 530, textureId);
            break;
        case LABEL_DUREE:
            if (labelDuree) updateText(labelDuree, texte, context->fontBlue, context->couleurBlue, 1050, 670, textureId);
            else labelDuree = textureText(texte, context->fontBlue, context->couleurBlue, 1050, 670, textureId);
            break;
        case TEXTE_DUREE:
            if (texteDuree) updateText(texteDuree, texte, context->fontViolet, context->couleurViolet, 1020, 670, textureId);
            else texteDuree = textureText(texte, context->fontViolet, context->couleurViolet, 1020, 670, textureId);
            break;
        case LABEL_PUISSANCE:
            if (labelPuissance) updateText(labelPuissance, texte, context->fontLabel, context->couleurLabel, 930, 150, textureId);
            else labelPuissance = textureText(texte, context->fontLabel, context->couleurLabel, 930, 150, textureId);
            break;
        case TEXTE_PUISSANCE:
            if (DetailedCharge) {
                if (textePuissance) updateText(textePuissance, texte, context->fontTexte, context->couleurTexte, 890, 150, textureId);
                else textePuissance = textureText(texte, context->fontTexte, context->couleurTexte, 890, 150, textureId);

            } else {
                if (textePuissance) updateText(textePuissance, texte, context->fontPower, context->couleurTexte, 850, 300, textureId);
                else textePuissance = textureText(texte, context->fontPower, context->couleurTexte, 850, 300, textureId);
            }
            break;
        case LABEL_ENERGIE:
            if (labelEnergie) updateText(labelEnergie, texte, context->fontLabel, context->couleurLabel, 930, 400, textureId);
            else labelEnergie = textureText(texte, context->fontLabel, context->couleurLabel, 930, 400, textureId);
            break;
        case TEXTE_ENERGIE:
            if (DetailedCharge) {
            if (texteEnergie) updateText(texteEnergie, texte, context->fontTexte, context->couleurTexte, 890, 200, textureId);
            else texteEnergie = textureText(texte, context->fontTexte, context->couleurTexte, 890, 200, textureId);
            } else {
            if (texteEnergie) updateText(texteEnergie, texte, context->fontSidePower, context->couleurTexte, 700, 140, textureId);
            else texteEnergie = textureText(texte, context->fontSidePower, context->couleurTexte, 700, 140, textureId);
            }
            break;
        case LABEL_RECUP:
            if (labelRecup) updateText(labelRecup, texte, context->fontLabel, context->couleurLabel, 930, 650, textureId);
            else labelRecup = textureText(texte, context->fontLabel, context->couleurLabel, 930, 650, textureId);
            break;
        case TEXTE_RECUP:
            if (DetailedCharge) {
                if (texteRecup) updateText(texteRecup, texte, context->fontTexte, context->couleurTexte, 890, 650, textureId);
                else texteRecup = textureText(texte, context->fontTexte, context->couleurTexte, 890, 650, textureId);
            } else {
                if (texteRecup) updateText(texteRecup, texte, context->fontSidePower, context->couleurTexte, 700, 600, textureId);
                else texteRecup = textureText(texte, context->fontSidePower, context->couleurTexte, 700, 600, textureId);
            }
            break;
        case LABEL_TENSION:
            if (labelTension) updateText(labelTension, texte, context->fontLabel, context->couleurLabel, 830, 400, textureId);
            else labelTension = textureText(texte, context->fontLabel, context->couleurLabel, 830, 400, textureId);
            break;
        case TEXTE_TENSION:
            if (texteTension) updateText(texteTension, texte, context->fontTexte, context->couleurTexte, 790, 400, textureId);
            else texteTension = textureText(texte, context->fontTexte, context->couleurTexte, 790, 400, textureId);
            break;
        case LABEL_COURANT:
            if (labelCourant) updateText(labelCourant, texte, context->fontLabel, context->couleurLabel, 830, 150, textureId);
            else labelCourant = textureText(texte, context->fontLabel, context->couleurLabel, 830, 150, textureId);
            break;
        case TEXTE_COURANT:
            if (texteCourant) updateText(texteCourant, texte, context->fontTexte, context->couleurTexte, 790, 150, textureId);
            else texteCourant = textureText(texte, context->fontTexte, context->couleurTexte, 790, 150, textureId);
            break;
        case LABEL_COURANT_MAX:
            if (labelCourantMax) updateText(labelCourantMax, texte, context->fontLabel, context->couleurLabel, 830, 650, textureId);
            else labelCourantMax = textureText(texte, context->fontLabel, context->couleurLabel, 830, 650, textureId);
            break;
        case TEXTE_COURANT_MAX:
            if (texteCourantMax) updateText(texteCourantMax, texte, context->fontTexte, context->couleurTexte, 790, 650, textureId);
            else texteCourantMax = textureText(texte, context->fontTexte, context->couleurTexte, 790, 650, textureId);
            break;
        case LABEL_CPT_EXTERNE:
            if (labelCompteurExterne) updateText(labelCompteurExterne, texte, context->fontLabel, context->couleurLabel, 730, 150, textureId);
            else labelCompteurExterne = textureText(texte, context->fontLabel, context->couleurLabel, 730, 150, textureId);
            break;
        case TEXTE_CPT_EXTERNE:
            if (texteCompteurExterne) updateText(texteCompteurExterne, texte, context->fontTexte, context->couleurTexte, 690, 150, textureId);
            else texteCompteurExterne = textureText(texte, context->fontTexte, context->couleurTexte, 690, 150, textureId);
            break;
        case LABEL_CABLE:
            if (labelCable) updateText(labelCable, texte, context->fontLabel, context->couleurLabel, 730, 400, textureId);
            else labelCable = textureText(texte, context->fontLabel, context->couleurLabel, 730, 400, textureId);
            break;
        case TEXTE_CABLE:
            if (texteCable) updateText(texteCable, texte, context->fontTexte, context->couleurTexte, 690, 395, textureId);
            else texteCable = textureText(texte, context->fontTexte, context->couleurTexte, 690, 395, textureId);
            break;
        default:
            break;
        }
    }
}

static SDL_Texture * textureText(char *texte, TTF_Font * font, SDL_Color couleur, int x, int y, LabelTexture textureId) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, texte, couleur);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[textureText - createSurface %d] - %s > %s", textureId, __func__, TTF_GetError());
        return NULL;
    }

    SDL_Texture * texture = SDL_CreateTextureFromSurface(context->renderer, surface);
    if (surface) {
        SDL_FreeSurface(surface);

        SDL_QueryTexture(texture, NULL, NULL, &t_position.w, &t_position.h); // Récupere la dimension de la texture
        // Centre la texture sur l'écran
        t_position.x = x - t_position.w/2;
        t_position.y = y;
        center.x = t_position.w / 2;
        center.y = t_position.h / 2;
        SDL_RenderCopyEx(context->renderer, texture, NULL, &t_position, angle, &center, SDL_FLIP_NONE); // Copie du texte
    }

    return texture;
}

static void updateText(SDL_Texture * texture, char *texte, TTF_Font * font, SDL_Color couleur, int x, int y, LabelTexture textureId) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, texte, couleur);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[textureText - createSurface %d] - %s > %s", textureId, __func__, TTF_GetError());
        return;
    }
    //SDL_Texture * texture = SDL_CreateTextureFromSurface(context->renderer, surface);
    SDL_UpdateTexture(texture, &(surface->clip_rect), surface->pixels, surface->pitch);
    if (surface) {
        SDL_FreeSurface(surface);

        SDL_QueryTexture(texture, NULL, NULL, &t_position.w, &t_position.h); // Récupere la dimension de la texture
        // Centre la texture sur l'écran
        t_position.x = x - t_position.w/2;
        t_position.y = y;
        center.x = t_position.w / 2;
        center.y = t_position.h / 2;
        SDL_RenderCopyEx(context->renderer, texture, NULL, &t_position, angle, &center, SDL_FLIP_NONE); // Copie du texte
    }
}

static void renderQrCode(int x, int y) {
    if (!access("/usr/share/hubload/qrc.bmp", F_OK)) {
        SDL_Surface *surface = SDL_LoadBMP("/usr/share/hubload/qrc.bmp");
        if (!surface) return;

        // Centre la texture sur l'écran
        positionQRCode.x = x;
        positionQRCode.y = y;
        positionQRCode.w = QRCODE_WIDTH;
        positionQRCode.h = QRCODE_HEIGHT;
        center.x = QRCODE_WIDTH / 2;
        center.y = QRCODE_HEIGHT / 2;

        //// BUG IN SDL_UpdateTexture() function ///
        // http://forums.libsdl.org/viewtopic.php?p=49225

        // NOT WORKING: 
        // SDL_UpdateTexture (texture, &rect, pixels, pitch);  
        // WORKING - update the whole texture 
        // SDL_UpdateTexture(texture, NULL, pixels, pitch);

        if (textureQRCode) SDL_UpdateTexture(textureQRCode, NULL, surface->pixels, surface->pitch);
        else textureQRCode = SDL_CreateTextureFromSurface(context->renderer, surface);

        SDL_FreeSurface(surface);    
        SDL_RenderCopyEx(context->renderer, textureQRCode, NULL, &positionQRCode, angle, &center, SDL_FLIP_NONE);
    }
}

// Rendu du drapeau de langue
static void renderFlag() {
    if (context->francais)  strcpy(flag_name, "/opt/hubload/resources/img/Flag_gb.bmp");
    else                    strcpy(flag_name, "/opt/hubload/resources/img/Flag_fr.bmp");

    SDL_Surface *surface = SDL_LoadBMP(flag_name);
    if (!surface) return;

    // Positionne la texture sur l'écran
    positionFlag.x = 1185;
    positionFlag.y = 684;
    positionFlag.w = FLAG_WIDTH;
    positionFlag.h = FLAG_HEIGHT;
    center.x = FLAG_WIDTH / 2;
    center.y = FLAG_HEIGHT / 2;

    //// BUG IN SDL_UpdateTexture() function ///
    // http://forums.libsdl.org/viewtopic.php?p=49225

    // NOT WORKING: 
    // SDL_UpdateTexture (texture, &rect, pixels, pitch);  
    // WORKING - update the whole texture 
    // SDL_UpdateTexture(texture, NULL, pixels, pitch);

    if (textureFlag) SDL_UpdateTexture(textureFlag, NULL, surface->pixels, surface->pitch);
    else textureFlag = SDL_CreateTextureFromSurface(context->renderer, surface);

    SDL_FreeSurface(surface);
    if (!textureFlag) fprintf(stderr, "Echec du rendu de la surface %s : %s ", flag_name, SDL_GetError());

    // copie de surface grâce au SDL_Renderer
    SDL_RenderCopyEx(context->renderer, textureFlag, NULL, &positionFlag, angle, &center, SDL_FLIP_NONE);
}

// Bas de page: ID, version SW, température et connectivité
static void renderFooter() {

    formatString(str_versionSW, display->SWversion, 26);
    texture(str_versionSW, TEXTE_VERSION_SW);
    formatString(str_ID, display->idBorne, 26);
    texture(str_ID, TEXTE_IDBORNE);

    formatValue(str_temperature, display->temperature, "°C", 1, 11);
    texture(str_temperature, TEXTE_TEMPERATURE);

    switch (display->connectivite) {
    case 0:
        formatString(str_connectivite, ((context->francais)?"     Hors ligne     ":"        Offline        "), 26);
        break;
    case 1:
        formatString(str_connectivite, ((context->francais)?"       En ligne       ":"         Online         "), 26);
        break;
    case 2:
        formatString(str_connectivite, ((context->francais)?"OCPP hors ligne":"   OCPP offline   "), 26);
        break;
    case 3:
        formatString(str_connectivite, ((context->francais)?"  OCPP en ligne  ":"    OCPP online    "), 26);
        break;
    default:
        formatString(str_connectivite, "      --------      ", 26);
        break;
    }
    texture(str_connectivite, TEXTE_CONNECTIVITE);
}

static void renderChargingTexts() {
    texture(display->str_debut, TEXTE_DEBUT);
    texture(display->str_temps, TEXTE_DUREE);

    if (display->status > PREPARING_4 && display->status < Finishing) {
        texture(str_puis, TEXTE_PUISSANCE);
        texture(display->str_energy, TEXTE_ENERGIE);
        texture(str_recup, TEXTE_RECUP);

        if (DetailedCharge) {
            formatValue(str_tension, display->voltage, "V", 1, 12);
            formatValue(str_cable, display->amperageCable*1.0, "A", 0, 12);
            formatValue(str_courant_max, display->maxAmp*1.0, "A", 0, 12);

            texture(str_cpt_ext, TEXTE_CPT_EXTERNE);
            texture(str_cable, TEXTE_CABLE);
            texture(str_courant, TEXTE_COURANT);
            texture(str_tension, TEXTE_TENSION);
            texture(str_courant_max, TEXTE_COURANT_MAX);
        }
    }
}

// Les chaines doivent etre de la même longueur en pixels pour pouvoir effacer la précédente lors d'une mise à jour
// un caractère " " occupe 2 fois moins de pixels en moyenne qu'un autre "a" à "Z"
// Attention, des caractères différents ont un nombre de pixels différents, il faut parfois ajouter des " "
static void renderChargingLabels() {
    texture(((context->francais)?"Début":" Start "), LABEL_DEBUT);
    texture(((context->francais)?"Durée":" Time "), LABEL_DUREE);

    if (DetailedCharge) {
        if (display->status > PREPARING_4 && display->status < SuspendedEV)
            texture(((context->francais)?"Puissance":"    Power    "), LABEL_PUISSANCE);

        texture(((context->francais)?" Energie ":"   Energy   "), LABEL_ENERGIE);
        texture(((context->francais)?"km (approx)":"   km (avg)   "), LABEL_RECUP);

        if (display->status > PREPARING_4 && display->status < SuspendedEV) {
            texture(((context->francais)?"Courant":" Current "), LABEL_COURANT);
            texture(((context->francais)?"Tension N/Ph":"  Voltage N/L  "), LABEL_TENSION);
            texture(((context->francais)?"Courant max":" Max current "), LABEL_COURANT_MAX);
        }

        texture(((context->francais)?"Compteur":"    Meter    "), LABEL_CPT_EXTERNE);
        texture(((context->francais)?"Câble":"Cable"), LABEL_CABLE);
    }
}

// 14 caractères chaine la plus longue, toutes les chaines doivent etre de la même longueur en pixels
// un caractère " " occupe 2 fois moins de pixels qu'un autre
static void renderChargingStatus() {
    switch (display->status) {
    case SuspendedEV:
        // 10 -> +8 " ", 12 -> +4 (6 ??) " "
        texture(((context->francais)?"    Pause (VE)    ":"  Suspend (EV)  "), TEXTE_STATUS);
        break;
    case SuspendedEVSE:
        // 12 -> +4 (6 ??) " ", 14
        texture(((context->francais)?"   Pause (EVSE)   ":"Suspend (EVSE)"), TEXTE_STATUS);
        break;
    default:
        // 9 -> +10 " ", 8 -> +12 " "
        texture(((context->francais)?"     En charge     ":"      Charging      "), TEXTE_STATUS);
    }
}

// 13 caractères chaine la plus longue, toutes les chaines doivent etre de la même longueur en pixels
// un caractère " " occupe 2 fois moins de pixels qu'un autre
static void renderWarning() {
    if (display->ventilation)
     // 13, 13
       texture(((context->francais)?"   Ventilation !   ":  "   Ventilation !   "), TEXTE_WARNING);
    else if (display->maintenance || display->sys_maintenance)
    // 13, 11 -> +4 (6 ??) " "
        texture(((context->francais)?" Maintenance ! ":"    Servicing !    "), TEXTE_WARNING);
    else
    // 0 -> +26 " ", 0 -> +26 " "
        texture(((context->francais)?"                          ":"                          "), TEXTE_WARNING);
}

// Composition de l'écran en fonction de l'état courant de la borne
static void boucleRender() {
    SDL_RenderClear(context->renderer);

    // Image en fonction du contexte
    textureSurfaceFond();

    // Affichage du drapeau pour le choix de langue 
    renderFlag();

    // QRCode
    switch (display->status) {
    case AVAILABLE:
    case PREPARING_0:
    case PREPARING_1:
        renderQrCode(700, 120);
        break;
    case PREPARING_2:
    case PREPARING_3:
        renderQrCode(570, 310);
        break;
    default:
        break;
    }

    // Les éléments d'une charge en cours
    if (display->charging) {
        renderChargingLabels();
        renderChargingTexts();
        renderChargingStatus();
    }

    // Warning éventuel
    renderWarning();

    // ID et version, Température et connectivité
    renderFooter();

    //Affichage
    SDL_RenderPresent(context->renderer);
}

// Composition de l'écran d'analyse en cours ou d'erreur RFID
static void boucleRenderRfid() {
    SDL_RenderClear(context->renderer);

    if (context->francais) {
        if (analyseRfid)    strcpy(fond_name, "/opt/hubload/resources/img/rot/RF_fr.bmp");
        if (errorRfid)      strcpy(fond_name, "/opt/hubload/resources/img/rot/ME_fr.bmp");
    } else {
        if (analyseRfid)    strcpy(fond_name, "/opt/hubload/resources/img/rot/RF_en.bmp");
        if (errorRfid)      strcpy(fond_name, "/opt/hubload/resources/img/rot/ME_en.bmp");
    }

    SDL_Surface *surface = SDL_LoadBMP(fond_name);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[textureText - textureSurfaceFond %s] - %s > %s", fond_name, __func__, TTF_GetError());
        return;
    }
    if (textureFond) SDL_UpdateTexture(textureFond, &positionFond, surface->pixels, surface->pitch);
    else textureFond = SDL_CreateTextureFromSurface(context->renderer, surface);

    SDL_FreeSurface(surface);
    if (!textureFond) fprintf(stderr, "Echec du rendu de la surface %s : %s ", fond_name, SDL_GetError());

    SDL_RenderCopy(context->renderer, textureFond, NULL, &positionFond); // copie de surface grâce au SDL_Renderer

    // Rendu du QR code sur analyse
    if (analyseRfid)    renderQrCode(570, 310);

    // Rendu du code RFID si erreur
    if (errorRfid) {
        readNotifFile(NOTIF_VALUE_SCAN, active);
        if (labelStatus) updateText(labelStatus, active, context->fontStatus, context->couleurViolet, 630, 400, TEXTE_STATUS);
        else labelStatus = textureText(active, context->fontStatus, context->couleurViolet, 630, 400, TEXTE_STATUS);
    }

    // Affichage du drapeau de langue
    renderFlag();

    // Warning éventuel
    renderWarning();

    // ID et version, Température et connectivité
    renderFooter();

    //Affichage
    SDL_RenderPresent(context->renderer);
}

// Surveillance de la dalle tactile
static void pollSDLEvent() {
    char str_pos[50];

    while (SDL_PollEvent(&event)) {  // poll until all events are handled!
        if (event.type == SDL_QUIT) display->toContinue = OFF;
        // Pour pouvoir éventuellement remplacer la dalle tactile par une souris
        else if (event.type == SDL_MOUSEBUTTONUP) {
            sprintf(str_pos, "(%i,%i)", event.button.x, event.button.y);
            printf("position of the mouse : %s\n", str_pos);

            // Si en haut à droite dans une matrice (x,y) = (8,5), on permutte la langue d'affichage
            if ((event.button.x >= 0.875) && (event.button.y >= 0.8)) 
                context->francais = !context->francais;

            writeNotifFile(NOTIF_TOUCH, str_pos);
        }
        // Gestion de la dalle tactile
        else if (event.type == SDL_FINGERUP) {
            sprintf(str_pos, "%.4f;%.4f;%.4f;%.4f;%.2f", event.tfinger.x, event.tfinger.y, event.tfinger.dx, event.tfinger.dy, event.tfinger.pressure);
            printf("Event FINGERUP : pos(%.4f,%.4f), d(%.4f,%.4f), p(%.2f)\n", event.tfinger.x, event.tfinger.y, event.tfinger.dx, event.tfinger.dy, event.tfinger.pressure);

            // Si en haut à droite dans une matrice (x,y) = (8,5), on permutte la langue d'affichage
            if ((event.tfinger.x >= 0.875) && (event.tfinger.y >= 0.8)) 
                context->francais = !context->francais;

            writeNotifFile(NOTIF_TOUCH, str_pos);
        }
    }
}

int main(int argc, char *argv[]) {
	// On crée l'affichage
	initDisplay();
    signal(SIGINT, prepareToQuit);
   
	/* Step 1. Initialize inotify */
	fd = inotify_init();
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {  // error checking for fcntl
		printf("Problème fcntl\n");
	   	return EXIT_FAILURE;
	}
	/* Step 2. Add Watch */
	wd = inotify_add_watch(fd,notif_path,IN_MODIFY);

	if (wd < 0) printf("Could not watch : %s\n",notif_path);
	else    printf("Watching : %s\n",notif_path);

    //printf("On a l'affichage\n");

    // On attend que le programme JAVA soit lancé
    // pour que la mise à jour des fichiers de notif soit faite
    do {
        sleep(1);	// Attente 1 sec
        readNotifFile(NOTIF_STATE_JAVA, active);
    } while (strcmp(active, "On"));

    printf("java is launched\n");

    while (display->toContinue) {

        //// CHECK DES FICHIERS DE NOTIFICATION ////

        /* Step 3. Read buffer*/
        int i=0,length;
        char buffer[BUF_LEN];
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
                        if (!strcmp(event->name, NOTIF_ERROR)){
                            // TODO : Afficher message erreur
                        }
                        else if (!strcmp(event->name, NOTIF_VALUE_ID_EVSE) && display){
                            readNotifFile(NOTIF_VALUE_ID_EVSE, display->idBorne);
                        }
                        else if (!strcmp(event->name, NOTIF_VALUE_SW_EVSE) && display){
                            readNotifFile(NOTIF_VALUE_SW_EVSE, display->SWversion);
                        }
                        else if (!strcmp(event->name, NOTIF_WARNING) && display){
                            readNotifFile(NOTIF_WARNING, active);
                            display->maintenance = !strcmp(active, "On");
                        }
                        else if (!strcmp(event->name, NOTIF_MAINTENANCE) && display){
                            readNotifFile(NOTIF_MAINTENANCE, active);
                            display->sys_maintenance = !strcmp(active, "On");
                        }
                        else if (!strcmp(event->name, NOTIF_STATE_TYPE2) && display){
                            readNotifFile(NOTIF_STATE_TYPE2, active);
                            display->ventilation = !strcmp(active, "D");
                        }
                        else if (!strcmp(event->name, NOTIF_VALUE_TEMP) && display){
                            readNotifFile(NOTIF_VALUE_TEMP, active);
                            display->temperature = atof(active);
                        }
                        else if (!strcmp(event->name, NOTIF_STATE_CONNECTIVITY) && display){
                            readNotifFile(NOTIF_STATE_CONNECTIVITY, active);
                            display->connectivite = atoi(active);
                        }
                        else if (!strcmp(event->name, NOTIF_STATE_EVSE) && display){
                            readNotifFile(NOTIF_STATE_EVSE, active);
                            display->status = atoi(active);

                            // En cas d'erreur due au véhicule, on force l'affichage de déconnexion du véhicule
                            readNotifFile(NOTIF_VALUE_LATEST_VE_ERROR, active);
                            if (atoi(active)) display->status = Finishing;

                            // Status de Charge
                            if (display->status > PREPARING_4 && display->status < Finishing) {
                                display->charging = ON;
                                if (display->status == CHARGING_13)      display->amperageCable = 13;
                                else if (display->status == CHARGING_20) display->amperageCable = 20;
                                else if (display->status == CHARGING_32) display->amperageCable = 32;
                            }
                            else display->charging = OFF;
                        }
                        else if (!strcmp(event->name, NOTIF_SESSION_START) && display){
                            readNotifFile(NOTIF_SESSION_START, display->str_debut);
                        }
                        else if (!strcmp(event->name, NOTIF_SESSION_DURATION) && display){
                            readNotifFile(NOTIF_SESSION_DURATION, display->str_temps);
                        }
                        else if (!strcmp(event->name, NOTIF_VALUE_CURRENT) && display){
                            readNotifFile(NOTIF_VALUE_CURRENT, active);
                            double amperage = atof(active);
                            formatValue(str_courant, amperage, "A", 2, 12);
                        }
                        else if (!strcmp(event->name, NOTIF_VALUE_POWER) && display){
                            readNotifFile(NOTIF_VALUE_POWER, active);
                            double power = atof(active);
                            formatValue(str_puis, (power / 1000.0), "kW", 1, 12);
                        }
                        else if (!strcmp(event->name, NOTIF_SESSION_ENERGY) && display){
                            readNotifFile(NOTIF_SESSION_ENERGY, active);
                            long energy = atol(active);
                            formatValue(display->str_energy, (energy / 1000.0), "kWh", 1, 12);

                            double  nbKm = energy / 170.0;
                            formatValue(str_recup, nbKm, "km", 0, 12);
                        }
                        else if (!strcmp(event->name, NOTIF_SESSION_MAX_CURRENT) && display){
                            readNotifFile(NOTIF_SESSION_MAX_CURRENT, active);
                            int max_current = atoi(active);
                            formatValue(str_courant_max, max_current*1.0, "A", 0, 12);
                        }
                        else if (!strcmp(event->name, NOTIF_VALUE_SCAN) && display){
                    		gettimeofday(&startTempScreen, NULL);	
                            analyseRfid = ON;
                            tempScreen = ON;
                        }
                        else if (!strcmp(event->name, NOTIF_FAIL_SCAN) && display){
                    		gettimeofday(&startTempScreen, NULL);	
                            errorRfid = ON;
                            tempScreen = ON;
                        }
                        else if (!strcmp(event->name, NOTIF_CONF)){
                            readConfFile();
                        }
                        else if (!strcmp(event->name, NOTIF_STOP)){
                            display->toContinue = OFF;
                        }
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }

        // Test de la dalle tactile
        pollSDLEvent();

        //// AFFICHAGE GÉNÉRAL OU TEMPORAIRE ////

        if (!tempScreen) boucleRender();
        else {
    		// Temps écoulé depuis le début de l'affichage temporaire
    		gettimeofday(&timeNow, NULL);	
	    	delayTempScreen = timeNow.tv_sec - startTempScreen.tv_sec;
            if (delayTempScreen > TIMEOUT_TEMP) {
                tempScreen = OFF;
                errorRfid = OFF;
                analyseRfid = OFF;
            } else boucleRenderRfid();
        }

        // Attente 200 msec
        usleep(200000);
    }

    nettoyage_display();
}