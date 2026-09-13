#ifndef START_STOP_MODULE_H__
#define START_STOP_MODULE_H__

#include "rc5.h"

/**
 *  This library translates RC5 packets to Start-Stop Module protocol
 *  https://p1r.se/startmodule/implement-yourself/
 */

#define ADDR_STARTSTOP 0x07
#define ADDR_PROGRAMMING 0x0B
#define ADDR_CUSTOM_PROG 0xA3

typedef enum {
    STARTSTOP_OK = 0,
    STARTSTOP_RUN,  // Start fighting
    STARTSTOP_STOP,
    STARTSTOP_DOHYOERR,
    STARTSTOP_ADDRERR,
    STARTSTOP_PROGRAM_OK,
} StartStopRet;

void startstop_init();

StartStopRet startstop_run(Rc5Packet* rc5);

#endif