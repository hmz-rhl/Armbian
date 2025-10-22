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
#ifndef HUBMACHINE_H
#define HUBMACHINE_H

#include "lib/hubtypes.h"

// Gestion de la machine à états
    static void init_machine(LaunchValues *launch_values);
    static void calculTransitionPreparing();
    static void calculTransitionStartCharge();
    static void calculTransitionCharge();
    static void calculTransitionSev();
    static void calculTransitionSevse();
    static void calculTransitionFinishing();
    
// Gestion du hardware
    static void initPinValues(void);
    static void setExpValue(int address, int pin, int value);
    static void switchRelaisPriseEf(int open);
    static void openRelaisType2(int cause);
    static void closeRelaisType2(int cause);

    static int cable_locked_from_feedback(void);
    static void openCableLock(int force, int cause);
    static void closeCableLock(int cause);
    static void switchBl(int OnOff, int propagNotif);
 
// Gestion de l'EEPROM
    static void eeprom_readIdValue(void);
    static void eeprom_writeID(char *id);
    static uint16_t eeprom_getWh();

// Gestion des interruptions
    static void pp_interrupt();
    static void S0_interrupt();
    static void user_key_interrupt();

// Gestion de l'état du type2
    static void lectureCpValue();
    static void lecturePpValue();
    static void setPPMachineValues();
    static void setCPMachineValues();
    static void lectureInputValues();
    static void send_values();
    static void errorCheck();


// Gestion du statut et mise en place des valeurs HW
    static void calculValeurCycle(int courant);

    static void setStatePassive();
    static void setStatePreparingCar();
    static void setStateStartCharging();
    static void setStateError();
    static void setStateCharging(int OnOff);
    static void setState();
    static void checkTransitionEtat();

    static void readConfFile();
    static void initLaunchValues();
    static int  testRcd();
    static int  testLock();
    static void setLaunchValues();
    static void nettoyage_machine();
    static void stop_signal();
    static void rebootBorne();

#endif