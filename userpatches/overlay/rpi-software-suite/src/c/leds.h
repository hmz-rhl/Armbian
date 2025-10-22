/**
 * @file leds.h
 * @author Gilles Viguié (gilles.viguie@jerecharge.com)
 * @brief Gestion deu bandeau de leds de la borne
 * @version 1.0
 * @date 2023-04-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef HUBLEDS_H
#define HUBLEDS_H

#include "lib/hubtypes.h"
    static void initLeds(char *initConfig);
    static void launchSequence();

    static void nettoyage_leds();

    static void stop_leds();

    static void setLedsState(EvseState state);
    static void log_trace_leds(char *trace);
#endif
