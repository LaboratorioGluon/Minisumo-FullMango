#include "startStopModule.h"
#include "stm32h5xx_hal_uart.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ADDR_STARTSTOP    0x07
#define ADDR_PROGRAMMING  0x0B

uint8_t dohyoOff; /** Command to Stop */
uint8_t dohyoOn;

void startstop_init()
{
    // TODO: Load dohyoOff from NVM
}

char buf[100];
extern UART_HandleTypeDef huart5;

StartStopRet startstop_run(Rc5Packet *rc5)
{
    uint8_t addr;
    addr = rc5->address;

    if( addr == ADDR_STARTSTOP )
    {
        if ( rc5->command == dohyoOff)
        {
            // Turn off the motors
            snprintf(buf, 100, "[STARTSTOP]:: STOP!\r\n");
            HAL_UART_Transmit(&huart5, buf,  strlen(buf), 200);
            return STARTSTOP_STOP;
        }
        else if (rc5->command == dohyoOn) {
            // Start fight
            snprintf(buf, 100, "[STARTSTOP]:: START!\r\n");
            HAL_UART_Transmit(&huart5, buf,  strlen(buf), 200);
            return STARTSTOP_RUN;
        }
        else {
            return STARTSTOP_DOHYOERR;
        }
    }
    else if (addr == ADDR_PROGRAMMING) {
        dohyoOff = rc5->command;
        dohyoOn = dohyoOff | 0x1;
        snprintf(buf, 100, "[STARTSTOP]:: Updated DOHYO to: %d AND %d!\r\n", dohyoOff, dohyoOn);
        HAL_UART_Transmit(&huart5, buf,  strlen(buf), 200);
        return STARTSTOP_OK;
        // TODO: store dohyoOff command.
    }
    else {
        return STARTSTOP_ADDRERR;
    }
}