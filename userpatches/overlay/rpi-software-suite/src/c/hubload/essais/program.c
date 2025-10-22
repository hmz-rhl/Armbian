#include <stdio.h>
#include "struct.h"
#include <string.h>
#include <stdlib.h>

Hubleds *leds = NULL;

int main(int argc, char *argv[]) {
    leds = initLeds(NULL, "222100210121412130213121112112211221122131211121312140212121");
    printf("Level des leds : %d\n",leds->level);
    printf("Config des leds : %s\n",leds->ledsConfig);
}