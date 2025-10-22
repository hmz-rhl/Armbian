#ifndef HUBFILES_H
#define HUBFILES_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>

static const char *NOTIF_ESTATION = "ESTATION"; //down/eStation Off
static const char *NOTIF_FINISHING_SEV = "FINISHING_SEV"; //down/force finishing after SuspendedEV Off
static const char *NOTIF_LOCAL_DATE = "LOCAL_DATE"; //up/ Local date from network
static const char *NOTIF_LOCAL_TIME = "LOCAL_TIME"; //down/Local time every 15'

static const char *NOTIF_STATE_JAVA = "STATE_JAVA"; // down/value/state/java on
static const char *NOTIF_STATE_MACHINE = "STATE_MACHINE"; // up/value/state/machine On
static const char *NOTIF_STATE_CONNECTIVITY = "STATE_CONNECTIVITY"; // down/value/state/connectivity: 0: Offline; 1: IP Online; 2: OCPP Online;

static const char *NOTIF_VALUE_LATEST_VE_ERROR = "VALUE_LATEST_VE_ERROR"; //up/EV error: 0: No error; 2,3,4: RCD error; 5: CP error while charging; 6: Cable removed or PE error; 9: CP error; 10: VE silent;
static const char *NOTIF_VALUE_LATEST_LOCK_ERROR = "VALUE_LATEST_LOCK_ERROR"; //up/lock error: 0: No Error; 7: Unlock error; 8: Lock error;
static const char *NOTIF_WARNING = "WARNING"; //up/warning ESVE Off
static const char *NOTIF_MAINTENANCE = "MAINTENANCE"; //down/warning maintenance request Off
static const char *NOTIF_ERROR = "ERROR"; //down/ System: 0: No error; 1: System error
static const char *NOTIF_VALUE_RCD_RESULT = "VALUE_RCD_RESULT"; //up/RCD test: 0: No error; 1: RCD reset error; 2: RCD error;
static const char *NOTIF_TEST_RCD = "TEST_RCD"; //down/test RCD request
static const char *NOTIF_CONF = "CONF"; // down/conf/reload

static const char *NOTIF_BUTTON_PRESSED = "BUTTON_PRESSED"; // up/btn/pressed
static const char *NOTIF_BUTTON_DELAY = "BUTTON_DELAY"; // up/btn/delay pressed
static const char *NOTIF_VALUE_COLOR_BUTTON = "VALUE_COLOR_BUTTON"; // down/btn_led

static const char *NOTIF_ID_EVSE_READ = "ID_EVSE_READ"; // down/ID/read
static const char *NOTIF_ID_EVSE_WRITE = "ID_EVSE_WRITE"; // down/ID/write
static const char *NOTIF_SW_EVSE_READ = "SW_EVSE_READ"; // down/SW version/read
static const char *NOTIF_SW_EVSE_WRITE = "SW_EVSE_WRITE"; // down/SW version/write

static const char *NOTIF_VALUE_TEMP = "VALUE_TEMP"; // up/value/temp
static const char *NOTIF_VALUE_CURRENT = "VALUE_CURRENT"; //up/value/s0/current
static const char *NOTIF_VALUE_VOLTAGE = "VALUE_VOLTAGE"; //up/value/voltage
static const char *NOTIF_VALUE_POWER = "VALUE_POWER"; // up/value/s0/power
static const char *NOTIF_VALUE_COMPTEUR = "VALUE_COMPTEUR"; // up/value/compteur, up/value/s0/charge
static const char *NOTIF_VALUE_DUTYCYCLE = "VALUE_DUTYCYCLE"; // up/value/dutycycle
static const char *NOTIF_VALUE_PHASES = "VALUE_PHASES"; // down/value/phase
static const char *NOTIF_VALUE_SCAN = "VALUE_SCAN"; // up/scan
static const char *NOTIF_VALUE_ID_EVSE = "VALUE_ID_EVSE"; // up/ID
static const char *NOTIF_VALUE_SW_EVSE = "VALUE_SW_EVSE"; // up/SW version
static const char *NOTIF_STATE_EVSE = "STATE_EVSE"; // up/value/state/evse
static const char *NOTIF_STATE_COMP = "STATE_COMP"; // up/value/state/comp
static const char *NOTIF_STATE_TYPE2 = "STATE_TYPE2"; // up/value/state/type2

static const char *NOTIF_SESSION_START = "SESSION_START"; // up/value/session/start
static const char *NOTIF_SESSION_DURATION = "SESSION_DURATION"; // up/value/session/duration
static const char *NOTIF_SESSION_ENERGY = "SESSION_ENERGY"; // up/value/session/energy
static const char *NOTIF_SESSION_MAX_CURRENT = "SESSION_MAX_CURRENT"; // down/charger/currentmax

static const char *NOTIF_S0_PULSE = "S0_PULSE"; //
static const char *NOTIF_CL_FEEDBACK = "CL_FEEDBACK"; // up/value/cablelock/feedback
static const char *NOTIF_CL_LOCK = "CL_LOCK"; // down/lockType2/close, down/lockType2/open
static const char *NOTIF_REDUCE_BL = "REDUCE_BL"; // down/reduce_bl/on, down/reduce_bl/off
static const char *NOTIF_BACKLIGHT = "BACKLIGHT"; // down/backlight/on, down/backlight/off, up/backlight/on, up/backlight/off
static const char *NOTIF_TOUCH = "TOUCH"; // x;y;dx;dy;pressure
static const char *NOTIF_EF_CLOSE = "EF_CLOSE"; // down/type_ef/close, On (fermé) ou Off (ouvert)
static const char *NOTIF_VAE_LOCK_CLOSE = "VAE_LOCK_CLOSE"; //down/lock_vae/close
static const char *NOTIF_VAE_LOCK_OPEN = "VAE_LOCK_OPEN"; //down/lock_vae/open
static const char *NOTIF_VAE_POWER_ON = "VAE_POWER_ON"; // down/power_vae/close
static const char *NOTIF_VAE_POWER_OFF = "VAE_POWER_OFF"; //down/power_vae/open
static const char *NOTIF_NO_AUTH_MODE = "NO_AUTH_MODE"; // down/nam/on, down/nam/off
static const char *NOTIF_ENERGY_ALLOWED = "ENERGY_ALLOWED";// down/energy/on, down/energy/off
static const char *NOTIF_UNAVAILABLE = "UNAVAILABLE"; // down/unavailable/on, down/unavailable/off
static const char *NOTIF_AUTHENTICATED = "AUTHENTICATED"; // down/authenticated/on, down/authenticated/off
static const char *NOTIF_ACTIVATE_SCAN = "ACTIVATE_SCAN"; // (down/scan/activate, down/scan/shutdown)
static const char *NOTIF_FAIL_SCAN = "FAIL_SCAN";
static const char *NOTIF_WAKEUP = "WAKEUP"; // up/wakeup
static const char *NOTIF_STOP = "STOP"; // Stop EVSE
static const char *NOTIF_RESTART = "RESTART"; // up/restart
static const char *NOTIF_SCREEN = "SCREEN"; // Contenu : écran à afficher

static const char *notif_path = "/usr/share/hubload/notif";

#define MAX_EVENTS 1024  /* Maximum number of events to process*/
#define LEN_NAME 16  /* Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE  ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME ))

void readNotifFile(const char *notifName, char *value);
void writeNotifFile(const char *notifName, char *value);
void readConfFileValue(const char *key, char *value);
void readOneLineValue(const char *fileName, char *value, int removeFileAfterRead);
void writeOneLineValue(const char *fileName, const char *value, int createAndErase);

#endif