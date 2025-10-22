/**
 * @file hubtypes.h
  * @author	Gilles VIGUIE (gilles.viguie@jerecharge.com)
 *			Frederic BOMPARD (frederic.bompard@jerecharge.com)
 * @brief Définition des types communs de la borne hubload
 * @version 3.2
 * @date 2024-07-09
 * 
 * @copyright Copyright (c) 2024
*
 */

#ifndef HUBTYPES_H
#define HUBTYPES_H

#define BUFFER_SIZE     1024

#define MONO	1
#define TRI		3

#define ON      1
#define OFF     0

// l'expander 0x26: detecteur de courant de fuite et controle des relais de sortie

#define RCD_TRIP_AC_DIS   7     // Réarmenent du détecteur de courant de fuite AC (active low)
#define RCD_TRIP_DC_RESET 6		// Réarmement du détecteur de courant de fuite (Active low)
#define RCD_TST         5		// Test du détecteur de courant de fuite (Active low)
#define RCD_DIS         4		// Désactivation du détecteur de courant de fuite AC (Active low)
#define LOCK_D          3       // Direction du moteur de verrouillage
#define TYPE_E_F_ON     2		// Prise latérale on/off (Active high)
#define TYPE_2_L2L3_ON  1		// Prise type 2 L2/L3 on/off (Active high)
#define TYPE_2_NL1_ON   0		// Prise type 2 N/L1 on/off (Active high)

//l'expander 0x27: controle de l'ADE, CS des composants périphériques sur le SPI et activation du CP en sortie

#define PM0             7       // Pin pour controler le mode de fonctionnement de l'ADE
#define PM1             6       // Pin pour controler le mode de fonctionnement de l'ADE
#define PM_CS           5       // Chip Select de l'ADE (Active low)
#define T_CS            4       // CS du convertisseur de temperature (Active low)
#define CP_CS           3		// CS de lecture du signal CP (Active low)
#define PP_CS           2		// CS de lecture du signal PP (Active low)
#define CP_DIS          1		// Desactivation de la sortie CP (Active low)
#define Unused          0		// Unused

#include "pn532.h"
#include "PN532_Rpi_I2C.h"
#define LED_HARDWARE  1

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

    
   /* typedef enum { //-------------------- WiringPi numbering ----------------------//
        PIN_LOCK_P = 0,					// Type2 lock motor on/off (Active low)
        PIN_CP_PWM = 23,				// CP signal PWM
        PIN_I2C_D = 8,					// I2C
        PIN_I2C_C = 9,					// I2C
        PIN_SM_TIC_D = 2,				// S0 signal
        PIN_SM_TIC_D_BIS = 16,			// TIC signal
        PIN_USER_KEY = 3,				// User key (Active low)
        PIN_MOSI = 12,					// SPI
        PIN_MISO = 13,					// SPI
        PIN_SCLK = 14,					// SPI
        PIN_PP_IN = 22,					// EV cable in (Active low)
        PIN_WD_TRIP = 24,				// WatchDog Type2 and E/F relays (Active low)
        PIN_CF4 = 25,					// ADE interrupt
        PIN_RCD_TRIP_DC = 1,			// DC fault 6mA (Active low)
        PIN_RCD_TRIP_AC = 4,			// AC fault 20mA (Active low)
        PIN_IRQ1 = 5,					// ADE interrupt
        PIN_IRQ0 = 6,					// ADE interrupt
        PIN_LOCK_FB = 31,				// Type 2 lock feedback (Active low, MIDA product locked)
        PIN_DISPLAY_POWER = 30,			// Display BL on/off (active low)
        PIN_DISPLAY_REDUCE_POWER = 26,	// Reduced Display BL
        PIN_LED_STRIP_D = 29			// ws2815 strip command line
    } HwPin;
    */
    typedef enum {
        INIT = 0,           // Pas de câble, pad d'authentification
        AVAILABLE = 1,      // Pas de câble, pad d'authentification
        PREPARING_0 = 2,    // Câble connecté, pas de véhicule pas d'auth
        PREPARING_1 = 3,    // Véhicule connecté, pas d'auth
        PREPARING_2 = 4,    // Authentifié, pas de câble
        PREPARING_3 = 5,    // Authentifié, câble connecté pas de véhicuile
        PREPARING_4 = 6,    // Authentifié, véhicule connecté et prêt
        CHARGING_13 = 7,    // En charge, courant <= 13A
        CHARGING_20 = 8,    // En charge, courant <= 20A
        CHARGING_32 = 9,    // En charge, courant <= 32A
        SuspendedEV = 10,   // En pause par le véhicule
        SuspendedEVSE = 11, // En pause par la borne (Smart Charging)
        Finishing = 12,     // Quitting, fin de charge
        Unavailable = 13,   // Unavailable
        Faulted = 14        // Faulted
    } EvseState;

    typedef struct {
        int state;
        int authenticated;
        int nam;
        int pwm;
        int OE;
        int p0;
        int p1;
        int c0;
        int c1;
        int contact0;
        int contact1;
        int contact2;
        int Faulted;
        int Unavailable;
    } LaunchValues;
    
    typedef  struct {
        int state;
        int NAM;    // No Auth Mode
        int OE;     // Output Enabled
        int D2;     // Contact sec N° 3 (active low)
        int D1;     // Contact sec N° 2 (active low)
        int D0;     // Contact sec N° 1 (active low)
        int A;      // User authentificated
        int U;      // Unavailable required
        int F;      // Fault
		// Fault values :
        //      0: No error
        //      1: System defined error (OCPP, Master CPU) 
        //      2: RCD DC error
        //		3: RCD AC error
		//		4: RCD AC & DC errors
        //      5: CP error 
        //      6: Cable removed during charge process
        //      7: Cable unlock error
        //      8: Cable lock error
		//		9: Preparing mode error
		//	   10: ->C transition error
		int P1;     // PP value - Bit poids fort
        int P0;     // PP value - Bit poids faible
        //      (0,0) : charge <= 0A (PP <= 6V)
        //      (0,1) : charge <= 13A
        //      (1,0) : charge <= 20A
        //      (1,1) : charge <= 32A
        //
        int C1;     // CP value - Bit poids fort
        int C0;     // CP value - Bit poids faible
		//      (0,0) : PP = -12V;
		//      (0,1) : 3V ou 6V;
		//      (1,0) : 9V;
		//      (1,1) : 12V
    } EtatMachine;

    typedef enum {
        BLUE = 0,
        GREEN = 1,
        RED = 2,
        ORANGE = 3,
        YELLOW = 4,
        WHITE = 5 
    } LedColor;

    typedef enum {
        CONSTANT = 0,
        BLINK = 1,
        CHENILLE = 2,
        PULSE = 3 
    } LedScheme;

    typedef  enum {
        LEVEL_OFF = 0,
        LEVEL_MIN = 1,
        LEVEL_NORMAL = 2,
        LEVEL_HIGH = 3,
        LEVEL_MAX = 4 
    } LedPower;

    typedef enum {
        ORDER_RGB = 0,
        ORDER_GBR = 1,
        ORDER_BRG = 2,
        ORDER_GRB = 3,
        ORDER_RBG = 4,
        ORDER_BGR = 5 
    } LedOrder;

    typedef  struct {
        int nb_led;
        int blOn;
        int state;
        char ledsConfig[65];
        LedColor color;
        LedScheme scheme;
        LedPower level;
        LedPower level_veille;
        LedOrder led_order;
    } Hubleds;

    typedef struct sdl_context {
        int francais; // Affichage de l'écran en français
        char prefix_surfaces[15][40];

        SDL_Renderer *renderer;
        SDL_Window *ecran;

        SDL_Color couleurLabel;
        SDL_Color couleurTexte;
        SDL_Color couleurBlue;
        SDL_Color couleurViolet;
        SDL_Color couleurOrange;
        SDL_Color couleurNoir;
        SDL_Color couleurRouge;
        SDL_Color couleurVerte;

        TTF_Font* fontLabel;
        TTF_Font* fontTexte;
        TTF_Font* fontBlue;
        TTF_Font* fontViolet;
        TTF_Font* fontNoir;
        TTF_Font* fontStatus;
        TTF_Font* fontPower;
        TTF_Font* fontSidePower;
    } SDL_Context;

    typedef struct hubdisplay {
        int toContinue;
        int status;
        int charging;           // Ecran en cours de charge avec informations dynamiques
        int ventilation;        // Demande de ventilation en charge (status D)
        int maintenance;        // Demande de maintenance
        int sys_maintenance;    // Demande de maintenance système
        char idBorne[25];       // Identifiant de la borne à afficher en bas d'écran
        char SWversion[15];     // Version logicielle
        int connectivite;       // Connectivité Réseau/OCPP
        double temperature;     // Informations de température à afficher
        int toRender;           

        // Configuration
        int maxAmp;
        double voltage;
        int amperageCable;
        int nb_phases;
        char str_temps[20];
        char str_debut[20];
        char str_energy[20];
    } HubDisplay;

    typedef enum {
        TEXTE_CONNECTIVITE, TEXTE_TEMPERATURE, TEXTE_IDBORNE, TEXTE_VERSION_SW, LABEL_PUISSANCE, TEXTE_PUISSANCE, LABEL_ENERGIE, TEXTE_ENERGIE, LABEL_RECUP, TEXTE_RECUP,
        LABEL_COURANT, TEXTE_COURANT, LABEL_TENSION, TEXTE_TENSION, LABEL_COURANT_MAX, TEXTE_COURANT_MAX,
        LABEL_CPT_EXTERNE, TEXTE_CPT_EXTERNE, LABEL_CABLE, TEXTE_CABLE, TEXTE_STATUS, TEXTE_WARNING,
        LABEL_DEBUT, TEXTE_DEBUT, LABEL_DUREE, TEXTE_DUREE
    } LabelTexture;

#endif