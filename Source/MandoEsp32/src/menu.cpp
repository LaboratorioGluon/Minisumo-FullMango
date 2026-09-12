#include "menu.h"
#include <string.h>
#include "rc5.h"
// #include "common.h"

constexpr uint8_t ADDR_STARTSTOP = 0x07;
constexpr uint8_t ADDR_PROGRAMMING = 0x0B;

/***** MENU ******/

static Menu g_CalibLines =
    {
        .entries = {
            {
                .text = "Start Calib.",
                .action = []()
                { rc5_send(0x10, 0x01); },
                .nextMenu = nullptr,
            },
            {
                .text = "Reset Calib.",
                .action = []()
                { rc5_send(0x10, 0x02); },
                .nextMenu = nullptr,
            },
        },
        .numEntries = 2,
        .prevMenu = nullptr,
};

static Menu g_CalibSharp =
    {
        .entries = {
            {
                .text = "Start Calib.",
                .action = []()
                { rc5_send(0x10, 0x03); },
                .nextMenu = nullptr,
            },
            {
                .text = "Reset Calib.",
                .action = nullptr,
                .nextMenu = nullptr,
            },
        },
        .numEntries = 2,
        .prevMenu = nullptr,
};

static Menu g_calibration = {
    .entries = {
        {
            .text = "Calibrate Line",
            .action = nullptr,
            .nextMenu = &g_CalibLines,
        },
        {
            .text = "Calibrate Sharp",
            .action = nullptr,
            .nextMenu = &g_CalibSharp,
        },
        {
            .text = "subMenu3",
            .action = nullptr,
            .nextMenu = nullptr,
        },
    },
    .numEntries = 3,
    .prevMenu = nullptr,
};

static Menu g_menu = {
    .entries = {
        {
            .text = "Set Normal Mode",
            .action = nullptr,
            .nextMenu = nullptr,
        },
        {
            .text = "Calibration",
            .action = nullptr,
            .nextMenu = &g_calibration,
        },
        {
            .text = "Set Dohyo",
            .action = []()
            { rc5_send(ADDR_PROGRAMMING, 0x1A); },
            .nextMenu = nullptr,
        },
    },
    .numEntries = 3,
    .prevMenu = nullptr,
};
Menu *currentMenu = &g_menu;

static ssd1306_handle_t *g_handle;
static int8_t g_selectedMenuIndex;

static void menu_update()
{
    for (uint8_t i = 0; i < currentMenu->numEntries; i++)
    {
        ssd1306_display_text(*g_handle, i, currentMenu->entries[i].text, g_selectedMenuIndex == i);
    }
}

void menu_init(ssd1306_handle_t *handle)
{
    g_handle = handle;
}

void menu_loop(MenuInput in)
{
    bool needClear = false;
    switch (in)
    {
    case MENU_NONE:
        break;
    case MENU_DOWN:
        g_selectedMenuIndex = (g_selectedMenuIndex + 1 + currentMenu->numEntries) % currentMenu->numEntries;
        break;
    case MENU_UP:
        g_selectedMenuIndex = (g_selectedMenuIndex - 1 + currentMenu->numEntries) % currentMenu->numEntries;
        break;
    case MENU_ENTER:
        if (currentMenu->entries[g_selectedMenuIndex].action != nullptr)
        {
            currentMenu->entries[g_selectedMenuIndex].action();
        }
        else if (currentMenu->entries[g_selectedMenuIndex].nextMenu != nullptr)
        {
            currentMenu->entries[g_selectedMenuIndex].nextMenu->prevMenu = currentMenu;
            currentMenu = currentMenu->entries[g_selectedMenuIndex].nextMenu;
            g_selectedMenuIndex = 0;
            needClear = true;
        }
        break;
    case MENU_EXIT:
        if (currentMenu->prevMenu != nullptr)
        {
            currentMenu = currentMenu->prevMenu;
            g_selectedMenuIndex = 0;
            needClear = true;
        }
        break;
    default:
        break;
    }
    if (needClear)
    {

        ssd1306_clear_display(*g_handle, false);
    }
    menu_update();
}