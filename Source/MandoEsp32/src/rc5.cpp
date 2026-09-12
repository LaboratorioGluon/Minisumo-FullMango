#include "rc5.h"
#include <esp_err.h>
#include <string.h>

#define RC5_QUEUE_LEN 64

#define RC5_SYMBOL_BIT0 \
    {.duration0 = 889, .level0 = 1, .duration1 = 889, .level1 = 0}
#define RC5_SYMBOL_BIT1 \
    {.duration0 = 889, .level0 = 0, .duration1 = 889, .level1 = 1}

#define RC5_ADDR_START 3
#define RC5_ADDR_LEN 5
#define RC5_CMD_START 8
#define RC5_CMD_LEN 6

#define RC5_PULSE_TIME_THRESH_US 1200

QueueHandle_t irRxQueue;

static rmt_channel_handle_t tx_chan = NULL;
static rmt_encoder_handle_t tx_encoder;
static rmt_encoder_handle_t tx_copy_encoder;
static rmt_transmit_config_t tx_trans_config;

static rmt_channel_handle_t rx_chan = NULL;

uint8_t irNewData = 0;

static bool rc5_irRxDoneCallback(rmt_channel_handle_t channel,
                                 const rmt_rx_done_event_data_t *edata, void *user_data)
{
    irNewData = 1;
    BaseType_t high_task_wakeup = pdFALSE;
    // send the received RMT symbols to the parser task
    xQueueSendFromISR(irRxQueue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

rmt_symbol_word_t buffer[RC5_QUEUE_LEN];

void rc5_one();
void rc5_zero();
void rc5_startReceive();

void rc5_init(gpio_num_t gpioTx, gpio_num_t gpioRx, float duty)
{

    irRxQueue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));

    if (gpioTx != GPIO_NUM_NC)
    {
        rmt_tx_channel_config_t tx_chan_config = {
            .gpio_num = gpioTx,               // GPIO number
            .clk_src = RMT_CLK_SRC_DEFAULT,   // select source clock
            .resolution_hz = 1 * 1000 * 1000, // 1 MHz tick resolution, i.e., 1 tick = 1 µs
            .mem_block_symbols = 64,          // memory block size, 64 * 4 = 256 Bytes
            .trans_queue_depth = 4,           // set the number of transactions that can
                                              // pend in the background
            .flags = {.invert_out = false, .with_dma = false},
        };

        ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_chan));

        rmt_carrier_config_t tx_carrier_cfg = {
            .frequency_hz = 38000,                   // 38 KHz
            .duty_cycle = duty,                      // duty cycle 33%
            .flags = {.polarity_active_low = false}, // carrier should be modulated to high level
        };
        // modulate carrier to TX channel
        ESP_ERROR_CHECK(rmt_apply_carrier(tx_chan, &tx_carrier_cfg));

        ESP_ERROR_CHECK(rmt_enable(tx_chan));
    }

    if (gpioRx != GPIO_NUM_NC)
    {

        rmt_rx_channel_config_t rx_chan_config = {
            .gpio_num = gpioRx,               // GPIO number
            .clk_src = RMT_CLK_SRC_DEFAULT,   // select source clock
            .resolution_hz = 1 * 1000 * 1000, // 1 MHz tick resolution, i.e., 1 tick = 1 µs
            .mem_block_symbols = 64,          // memory block size, 64 * 4 = 256 Bytes
            .intr_priority = 0,
            .flags = {.invert_in = false, .with_dma = false, .io_loop_back = false},
        };

        ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_chan_config, &rx_chan));
        ESP_ERROR_CHECK(rmt_enable(rx_chan));

        rmt_rx_event_callbacks_t cbs = {
            .on_recv_done = rc5_irRxDoneCallback,
        };
        ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_chan, &cbs, nullptr));
    }

    rmt_bytes_encoder_config_t encoderConfig;
    rmt_symbol_word_t bit0, bit1;
    bit0.duration0 = 889;
    bit0.level0 = 1;
    bit0.duration1 = 889;
    bit0.level1 = 0;

    bit1.duration0 = 889;
    bit1.level0 = 0;
    bit1.duration1 = 889;
    bit1.level1 = 1;

    encoderConfig.bit0 = bit0;
    encoderConfig.bit1 = bit1;
    encoderConfig.flags.msb_first = 0;

    rmt_copy_encoder_config_t copyConfig;

    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copyConfig, &tx_copy_encoder));
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&encoderConfig, &tx_encoder));

    tx_trans_config.loop_count = 0;
    tx_trans_config.flags.eot_level = 0;
    tx_trans_config.flags.queue_nonblocking = 0;
}

void rc5_send(uint8_t address, uint8_t command)
{
    uint8_t data[2];
    uint8_t toggle = 0u;
    data[0] = ((address & 0x1F) << 4) | ((toggle & 0x1) << 2U) | 0b11;
    data[1] = command;

    rmt_symbol_word_t symbol[RC5_NUM_SYMBOLS];

    // Build array
    symbol[0] = RC5_SYMBOL_BIT1;
    symbol[1] = RC5_SYMBOL_BIT1;
    symbol[2] = RC5_SYMBOL_BIT0;

    // Build Addr data
    for (uint8_t i = 0; i < RC5_ADDR_LEN; i++)
    {
        if (address & (1 << (RC5_ADDR_LEN - i - 1)))
        {
            symbol[RC5_ADDR_START + i] = RC5_SYMBOL_BIT1;
        }
        else
        {
            symbol[RC5_ADDR_START + i] = RC5_SYMBOL_BIT0;
        }
    }

    // Build CMD data
    for (uint8_t i = 0; i < RC5_CMD_LEN; i++)
    {
        if (command & (1 << (RC5_CMD_LEN - i - 1)))
        {
            symbol[RC5_CMD_START + i] = RC5_SYMBOL_BIT1;
        }
        else
        {
            symbol[RC5_CMD_START + i] = RC5_SYMBOL_BIT0;
        }
    }

    rmt_transmit(tx_chan, tx_copy_encoder, symbol,
                 RC5_NUM_SYMBOLS * sizeof(rmt_symbol_word_t), &tx_trans_config);
}

void rc5_startReceive()
{
    rmt_receive_config_t rxConfig;
    rxConfig.flags.en_partial_rx = 0;
    rxConfig.signal_range_min_ns = 3000;
    rxConfig.signal_range_max_ns = 2000 * 1000;

    ESP_ERROR_CHECK(rmt_receive(rx_chan, &buffer,
                                sizeof(buffer) * sizeof(rmt_symbol_word_t), &rxConfig));
}

static inline uint8_t rc5_checkSymbol(uint8_t symbol[2], uint8_t *nextInSymbol)
{
    uint8_t ret = 0xFF;
    if (*nextInSymbol >= 2)
    {
        if ((symbol[0] == 1) && (symbol[1] == 0))
        {
            ret = 1;
        }
        else if ((symbol[0] == 0) && (symbol[1] == 1))
        {
            ret = 0;
        }
        *nextInSymbol = 0;
    }
    return ret;
}

static void rc5_addShortPulse(uint8_t symbol[2], uint8_t *nextInSymbol,
                              uint8_t level, uint8_t *output, uint8_t *outputNextBit)
{
    uint8_t value;
    symbol[(*nextInSymbol)++] = level;
    if ((value = rc5_checkSymbol(symbol, nextInSymbol)) != 0xFF)
    {
        output[(*outputNextBit)++] = value;
    }
}

uint16_t rc5_decodeWords(
    rmt_symbol_word_t *words, uint32_t len, Rc5Packet *packet)
{
    uint8_t symbol[2] = {1, 0};
    uint8_t nextInSymbol = 1;
    uint8_t output[RC5_NUM_SYMBOLS] = {0};
    uint8_t outputNextBit = 0;
    for (uint32_t i = 0U; i < len; i++)
    {

        if (words[i].duration0 < RC5_PULSE_TIME_THRESH_US)
        {
            rc5_addShortPulse(
                symbol, &nextInSymbol, words[i].level0, output, &outputNextBit);
        }
        else
        {
            rc5_addShortPulse(
                symbol, &nextInSymbol, words[i].level0, output, &outputNextBit);
            rc5_addShortPulse(
                symbol, &nextInSymbol, words[i].level0, output, &outputNextBit);
        }

        if (words[i].duration1 < RC5_PULSE_TIME_THRESH_US)
        {
            rc5_addShortPulse(
                symbol, &nextInSymbol, words[i].level1, output, &outputNextBit);
        }
        else
        {
            rc5_addShortPulse(
                symbol, &nextInSymbol, words[i].level1, output, &outputNextBit);
            rc5_addShortPulse(
                symbol, &nextInSymbol, words[i].level1, output, &outputNextBit);
        }
    }

    packet->start1 = output[0U];
    packet->start2 = output[1U];
    packet->toggle = output[2U];
    packet->addr = 0U;

    for (uint8_t i = 0U; i < RC5_ADDR_LEN; i++)
    {
        packet->addr |= ((output[3 + i] & 0x1) << (RC5_ADDR_LEN - i - 1));
    }

    packet->cmd = 0U;
    for (uint8_t i = 0U; i < RC5_CMD_LEN; i++)
    {
        packet->cmd |= ((output[8 + i] & 0x1) << (RC5_CMD_LEN - i - 1));
    }
    return (outputNextBit == RC5_NUM_SYMBOLS);
}

/**
 *
 * Signal: C | L
 *
 *
 *
 */
