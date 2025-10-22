#ifndef HUBRFID_H
#define HUBRFID_H

#include "lib/pn532.h"
#include "lib/PN532_Rpi_I2C.h"


typedef struct hub_rfid {
    uint8_t buff[255];
    uint8_t uid[MIFARE_UID_MAX_LENGTH];
    char *str[MIFARE_UID_MAX_LENGTH];

    int32_t uid_len;
    PN532 pn532;

    uint8_t scan_activated;
}HubRfid;

static void rfid_init(void);
static void lectureRfid();
static void stop_signal();
static void nettoyage_rfid();

#endif
