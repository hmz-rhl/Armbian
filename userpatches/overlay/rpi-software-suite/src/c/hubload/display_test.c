/*
Render text.
Based on SDL_ttf, which is has a separate source tree
https://www.libsdl.org/projects/SDL_ttf/, but is packaged
together with SDL on Debian
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 800
int run = 1;
int quit = 0;
int toRender = 0;

SDL_Event event;
SDL_Rect positionFond = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
SDL_Renderer *renderer;

SDL_Surface *surface_A = NULL; // Fonds de pages    

SDL_Texture *textureFond = NULL; // Textures textes début / duree

SDL_Window *ecran;

void textureSurface(SDL_Surface *surface, char * surfaceName) {
    textureFond = SDL_CreateTextureFromSurface(renderer, surface); 
    if (!textureFond) {
        fprintf(stderr, "Echec du rendu de la surface %s : %s ", surfaceName, SDL_GetError());
    }
    SDL_FreeSurface(surface);
}

void displayStatus(int newStatus) {
    textureSurface(surface_A, (char *)"Init"); 
    toRender = 1;
}


void nettoyage(int n) {	
    SDL_DestroyTexture(textureFond);

    SDL_FreeSurface (surface_A);


    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(ecran);
    SDL_Quit();
	
	exit(EXIT_SUCCESS);
}


void initDisplay() {  
    char name_A[] = "/opt/hubload/resources/img/A_fr.bmp";

    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

    ecran = SDL_CreateWindow("Chargement d'images en SDL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if(ecran) {
        renderer = SDL_CreateRenderer(ecran, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        if ( renderer ) {
            SDL_RendererInfo infoRenderer;
            SDL_GetRendererInfo(renderer, &infoRenderer);

            if (infoRenderer.flags & SDL_RENDERER_ACCELERATED) {
                SDL_Log("Le rendu est gérer par la carte graphique...");

            }
            if (infoRenderer.flags & SDL_RENDERER_SOFTWARE) {
                SDL_Log("Le rendu est gérer par la carte graphique...");
            }

            if (infoRenderer.flags & SDL_RENDERER_TARGETTEXTURE) {
                SDL_Log("Le rendu est autoriser sur des texture...");
            }
            SDL_Log("On va initialiser la couleur");

            surface_A = SDL_LoadBMP(name_A);

            displayStatus(0);
            
        }
        else {
            fprintf(stderr, "echec de création du renderer : %s", SDL_GetError());
        }
    }
    else {
        fprintf(stderr, "Erreur creation window : %s", SDL_GetError());
    }

    while (!quit) {
        while (SDL_PollEvent(&event) == 1) {
            //printf("Event : %i\n", event.type);
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
            else if (event.type == SDL_MOUSEBUTTONUP) {
                char str_pos[50];
                sprintf(str_pos, "(%i,%i)", event.button.x, event.button.y);
                printf("position of the mouse : %s\n", str_pos);
            }
            else if (event.type == SDL_FINGERUP) {
                char str_pos[50];
                sprintf(str_pos, "pos(%.4f,%.4f), d(%.4f,%.4f), p(%.2f)", event.tfinger.x, event.tfinger.y, event.tfinger.dx, event.tfinger.dy, event.tfinger.pressure);

                printf("Event FINGERUP : %s\n", str_pos);
            }
        }

        //if (toRender) {
            SDL_RenderCopy(renderer, textureFond, NULL, &positionFond); // copie de surface grâce au SDL_Renderer

            SDL_RenderPresent(renderer); //Affichage   
            toRender = 0;     
        //}

    }

    nettoyage(1);
}

void prepareToQuit(int n) {
    quit = 1;
}

int main(int argc, char **argv) {
	// on configure l'execution de la fonction interruption si ctrl+C
	signal(SIGINT, prepareToQuit);

	// Code de la logique du programme

	// phase d'initialisation
	printf("On va démarrer l'affichage de l'écran\n");

    initDisplay();
}