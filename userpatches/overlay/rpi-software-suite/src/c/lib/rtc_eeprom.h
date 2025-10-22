/**
 * @file EEPROM.h
 * @author Hamza RAHAL
 * @brief header du driver pour piloter MCP79410
 * @version 0.1
 * @date 2022-06-04
 * 
 * @copyright Saemload (c) 2022
 * 
 */
#ifndef _RTC_EEPROM_H
#define _RTC_EEPROM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define EEPROM_ADDRESS 0x57 // Adresse de l'EEPROM du MCP79410
#define RTC_ADDRESS 0x6F // Adresse du RTC du MCP79410
#define VERSION_RTC_EEPROM "1.0"

typedef struct{

    int eeprom_fd; // pour l'eeprom
    int rtc_fd; // pour la rtc
    uint8_t buf[129];
    // TODO : a mettre en place dans les fonction
    int error;

}rtc_eeprom_t;

rtc_eeprom_t *rtc_eeprom_init(void);


uint8_t int2bcd(uint8_t val);
uint8_t bcd2int(uint8_t val); 

uint8_t eeprom_read(rtc_eeprom_t* rtc_eeprom, uint8_t reg); // Permet de lire un registre de l'EEPROM
void eeprom_write(rtc_eeprom_t* rtc_eeprom, uint8_t reg, uint8_t val); // Permet d'écrire dans l'EEPROM Libre

uint8_t eeprom_readProtected(rtc_eeprom_t* rtc_eeprom, uint8_t reg); // Permet de lire un registre dans l'EEPROM Protégée
void eeprom_writeProtected(rtc_eeprom_t* rtc_eeprom, uint8_t reg, uint8_t val); // Permet d'écrire dans l'EEPROM Protégée

void eeprom_setAll(rtc_eeprom_t* rtc_eeprom); // Set a OxFF les registres

void eeprom_resetAllProtected(rtc_eeprom_t* rtc_eeprom); 

uint8_t eeprom_readStatus(rtc_eeprom_t *rtc_eeprom); //lit le contenue du registre STATUS(0xFF)

void eeprom_print(rtc_eeprom_t *rtc_eeprom);
void eeprom_printProtected(rtc_eeprom_t *rtc_eeprom);

uint8_t rtc_readSeconds(rtc_eeprom_t* rtc_eeprom); // Lit le temps ( secondes ) du RTC
void rtc_writeSeconds(rtc_eeprom_t* rtc_eeprom, uint8_t val); // Ecrit le temps ( secondes ) du RTC

void rtc_writeMinutes(rtc_eeprom_t* rtc_eeprom, uint8_t val); // Lit le temps ( minutes ) du RTC
uint8_t rtc_readMinutes(rtc_eeprom_t* rtc_eeprom); // Ecrit le temps ( minutes ) du RTC

void rtc_writeHours(rtc_eeprom_t* rtc_eeprom, uint8_t val); // Lit le temps ( heures ) du RTC
uint8_t rtc_readHours(rtc_eeprom_t* rtc_eeprom); // Ecrit le temps ( heures ) du RTC

void rtc_writeDate(rtc_eeprom_t* rtc_eeprom, uint8_t val); // Lit la date ( Jour ) du RTC
uint8_t rtc_readDate(rtc_eeprom_t* rtc_eeprom); // Ecrit la date ( Jour ) du RTC

void rtc_writeMonth(rtc_eeprom_t* rtc_eeprom, uint8_t val); // Lit la date ( Mois ) du RTC
uint8_t rtc_readMonth(rtc_eeprom_t* rtc_eeprom); // Ecrit la date ( Mois ) du RTC

void rtc_writeYear(rtc_eeprom_t* rtc_eeprom, uint8_t val); // Lit la date ( Année ) du RTC
uint8_t rtc_readYear(rtc_eeprom_t* rtc_eeprom); // Ecrit la date ( Année ) du RTC

uint8_t rtc_isPwrFail(rtc_eeprom_t* rtc_eeprom);

uint8_t rtc_isOscRunning(rtc_eeprom_t* rtc_eeprom);

uint8_t rtc_isVbatEnabled(rtc_eeprom_t * rtc_eeprom);
void rtc_enableVbat(rtc_eeprom_t * rtc_eeprom);

void rtc_enableExtOsc(rtc_eeprom_t * rtc_eeprom);

void rtc_mode24h(rtc_eeprom_t *rtc_eeprom); // Active le mode 24H
void rtc_mode12h(rtc_eeprom_t *rtc_eeprom); // Active le mode 12H ( AM / PM )

void rtc_printDate(rtc_eeprom_t *rtc_eeprom); // Affiche la date ( Jour Mois Année)
void rtc_printTime(rtc_eeprom_t *rtc_eeprom); // Affiche l'heure ( Heure Minutes Secondes)




void rtc_startClock(rtc_eeprom_t* rtc_eeprom); // Lance un cronomètre
void rtc_stopClock(rtc_eeprom_t* rtc_eeprom); // Stop le chronomètre

void rtc_eeprom_closeAndFree(rtc_eeprom_t *rtc_eeprom); // Libère la mémoire allouée
void rtc_eeprom_Free(rtc_eeprom_t *rtc_eeprom); // Libère la mémoire allouée

#endif
