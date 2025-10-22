#ifndef LEDS_SEQUENCE_H
#define LEDS_SEQUENCE_H

#include "../lib/hubtypes.h"


    static void initLeds();

    static double getMaxValue(LedPower level);

	static int getDelay(LedPower level);

    static void launchBlink(int redComp, int greenComp, int blueComp, LedPower level);
    
    static void launchFixed(int redComp, int greenComp, int blueComp, LedPower level_end, LedPower level_start);

    static void launchChenille(int redComp, int greenComp, int blueComp, LedPower level, LedPower level_alt);

    static void launchPulse(int redComp, int greenComp, int blueComp, LedPower level);

    static int getRedColorWithDecalage(int red, int green, int blue);

    static int getGreenColorWithDecalage(int red, int green, int blue);

    static int getBlueColorWithDecalage(int red, int green, int blue);

    static void launchSequence(LedColor color, LedScheme scheme, LedPower level_end, LedPower level_start);

#endif