#ifndef START_STOP_MODULE_H__
#define START_STOP_MODULE_H__

#include "rc5.h"

/**
 *  This library translates RC5 packets to Start-Stop Module protocol
 *  https://p1r.se/startmodule/implement-yourself/
 */

#define ADDR_STARTSTOP 0x07
#define ADDR_PROGRAMMING 0x0B
#define ADDR_CUSTOM_PROG 0x13

#define RC5_CUSTOM_CMD_WHITE_LINE_CAL 0x11
#define RC5_CUSTOM_CMD_BLACK_LINE_CAL 0x12
#define RC5_CUSTOM_CMD_RESTORE_LINE 0x13
#define RC5_CUSTOM_CMD_TEST_LINE 0x14
#define RC5_CUSTOM_CMD_SAVE_LINE_CAL 0x15

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
