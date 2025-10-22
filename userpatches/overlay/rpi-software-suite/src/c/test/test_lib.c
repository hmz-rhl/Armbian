#include "../lib/rtc_eeprom.h"
#include "../lib/expander_i2c.h"
#include "../lib/MCP3202.h"
#include "../lib/pn532.h"
#include "../lib/PN532_Rpi_I2C.h"

int main(int argc, char **argv){

    printf("test de la librairie de l'expander...\n");
    printf("VERSION : %s\n", VERSION_EXPANDER_I2C);
    printf("test librarire expander-i2c.h fini \n");


    printf("test de la librairie de la rtc eeprom...\n");
    printf("VERSION : %s\n", VERSION_RTC_EEPROM);
    printf("test librarire rtc_eeprom.h fini \n");

    printf("test de la librairie du MCP3202...\n");
    printf("VERSION : %s\n", VERSION_MCP3202);
    printf("test librarire MCP3202.h fini \n");

    printf("test de la librairie du pn532...\n");
    printf("VERSION : %s\n", VERSION_PN532);
    printf("test librarire pn532.h PN532_Rpi_I2C.h fini \n");
    

    return 0;

}