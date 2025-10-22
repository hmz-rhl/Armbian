/**
 * @file display.h
 * @author Gilles Viguié (gilles.viguie@jerecharge.com)
 * @brief Gestion de l'affichage graphique SDL
 * @version 1.0
 * @date 2023-04-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef HUBDISPLAY_H
#define HUBDISPLAY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <linux/types.h>
#include <time.h>
#include <sys/time.h>

#include "lib/hubtypes.h"

    // Initialisation
    static void init_display_struct(char* idBorne);
    static void init_context_struct();
    static void initDisplay();
    static void readConfFile(void);

    // Destruction des éléments de l'affichage
    static void nettoyage_display();
    static void freeMemSDLObjects();
    static void prepareToQuit();

 
    // On définit la texture de fond avec la surface passée en paramètre
    static void textureSurfaceFond();

    static void texture(char *texte, LabelTexture textureId);
    static SDL_Texture * textureText(char *texte, TTF_Font * font, SDL_Color couleur, int x, int y, LabelTexture textureId);
    static void updateText(SDL_Texture * texture, char *texte, TTF_Font * font, SDL_Color couleur, int x, int y, LabelTexture textureId);
    static void formatString(char *display, char *value, double totalSize);
    static void formatValue(char *display, double value, char *unit, int precision, double totalSize);

    // Affichage des informations de haut et bas de page
    static void renderFlag();
    static void renderFooter();

    // Affichage du QR code
    static void renderQrCode(int x, int y);

    // Affichage des informations de warning
    static void renderWarning();

    // Affichage des informations dynamique durant une charge
    static void renderChargingLabels();
    static void renderChargingTexts();
    static void renderChargingStatus();

    // Composition des écrans
    static void boucleRender();
    static void boucleRenderRfid();

    // Surveillance de la dalle tactile
    static void pollSDLEvent();

#endif