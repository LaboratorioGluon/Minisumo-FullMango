#include <freertos/FreeRTOS.h>
#include "rc5.h"
#include <ssd1306.h>
#include <esp_log.h>
#include "menu.h"
// #include "common.h"

constexpr uint8_t ADDR_STARTSTOP = 0x07;
constexpr uint8_t ADDR_PROGRAMMING = 0x0B;

constexpr gpio_num_t BUTTON_LEFT = GPIO_NUM_5;
constexpr gpio_num_t BUTTON_CENTER = GPIO_NUM_6;
constexpr gpio_num_t BUTTON_RIGHT = GPIO_NUM_7;
constexpr gpio_num_t OLED_SDA = GPIO_NUM_8;
constexpr gpio_num_t OLED_SCL = GPIO_NUM_9;

constexpr gpio_num_t SLIDER_UP = GPIO_NUM_21;
constexpr gpio_num_t SLIDER_DOWN = GPIO_NUM_10;
constexpr gpio_num_t SLIDER_PRESS = GPIO_NUM_20;

// OLED
ssd1306_config_t dev_cfg = I2C_SSD1306_128x64_CONFIG_DEFAULT;
ssd1306_handle_t dev_hdl;

// I2C
i2c_master_bus_handle_t busHandle;
i2c_master_dev_handle_t devHandle;

uint8_t selectedMenuIndex = 0;

uint8_t menuOptions[8][50] =
    {
        "Enemy Front",
        "Enemy Left",
        "Enemy Right",
        "",
        "",
        "",
        "",
        "",
};

void update_screen()
{
    /*ssd1306_clear_display(dev_hdl, false);
    ssd1306_set_contrast(dev_hdl, 0xff);
    ssd1306_clear_display(dev_hdl, false);*/
    for (uint8_t i = 0; i < 8; i++)
    {
        if (strlen((char *)menuOptions[i]) > 0)
        {
            ssd1306_display_text(dev_hdl, i, (char *)menuOptions[i], selectedMenuIndex == i);
        }
        /*ssd1306_display_text(dev_hdl, 0, "SSD1306 128x64", false);
        ssd1306_display_text(dev_hdl, 1, "Hello World!!", false);
        ssd1306_display_text(dev_hdl, 2, "SSD1306 128x64", true);
        ssd1306_display_text(dev_hdl, 3, "Hello World!!", true);*/
    }
}

extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(2000));
    rc5_init(GPIO_NUM_4, GPIO_NUM_NC);

    gpio_config_t buttons[6] = {0};
    buttons[0].pin_bit_mask = (1 << BUTTON_LEFT);
    buttons[1].pin_bit_mask = (1 << BUTTON_CENTER);
    buttons[2].pin_bit_mask = (1 << BUTTON_RIGHT);
    buttons[3].pin_bit_mask = (1 << SLIDER_UP);
    buttons[4].pin_bit_mask = (1 << SLIDER_DOWN);
    buttons[5].pin_bit_mask = (1 << SLIDER_PRESS);

    dev_cfg.flip_enabled = true;

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_SDA,
        .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {.enable_internal_pullup = true},
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &busHandle));

    i2c_device_config_t i2cDev;

    ssd1306_init(busHandle, &dev_cfg, &dev_hdl);
    if (dev_hdl == NULL)
    {
        ESP_LOGE("MANDO", "ssd1306 handle init failed");
        assert(dev_hdl);
    }

    for (uint8_t i = 0; i < 6; i++)
    {
        buttons[i].mode = GPIO_MODE_INPUT;
        buttons[i].pull_down_en = GPIO_PULLDOWN_DISABLE;
        buttons[i].pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&buttons[i]);
    }

    ssd1306_clear_display(dev_hdl, false);
    menu_init(&dev_hdl);
    MenuInput menuInput = MENU_NONE;
    while (1)
    {
        if (gpio_get_level(BUTTON_LEFT) == 0)
        {
            printf("Sending START cmd\r\n");
            rc5_send(ADDR_STARTSTOP, 0x1B);
        }
        else if (gpio_get_level(BUTTON_RIGHT) == 0)
        {
            printf("Sending STOP cmd\r\n");
            rc5_send(ADDR_STARTSTOP, 0x1A);
        }

        menuInput = MENU_NONE;
        if (gpio_get_level(SLIDER_UP) == 0)
        {
            printf("Slider up!");
            menuInput = MENU_UP;
        }
        else if (gpio_get_level(SLIDER_DOWN) == 0)
        {
            printf("Slider down!");
            menuInput = MENU_DOWN;
        }
        else if (gpio_get_level(SLIDER_PRESS) == 0)
        {
            printf("Slider pressed!");
            menuInput = MENU_ENTER;
        }
        else if (gpio_get_level(BUTTON_CENTER) == 0)
        {
            menuInput = MENU_EXIT;
        }
        menu_loop(menuInput);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}