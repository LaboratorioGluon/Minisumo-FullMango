#ifndef RC5_H__
#define RC5_H__

#include <driver/gpio.h>
#include <driver/rmt_rx.h>
#include <driver/rmt_tx.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define RC5_NUM_SYMBOLS 14U

extern uint8_t       irNewData;
extern QueueHandle_t irRxQueue;

typedef struct {
    uint8_t start1;
    uint8_t start2;
    uint8_t toggle;
    uint8_t addr;
    uint8_t cmd;
} Rc5Packet;

void rc5_init(gpio_num_t gpio, gpio_num_t gpioRx, float duty = 0.25f);

void rc5_send(uint8_t address, uint8_t command);
void rc5_startReceive();

uint16_t rc5_decodeWords(
    rmt_symbol_word_t *words, uint32_t len, Rc5Packet *packet);

#endif // RC5_H__