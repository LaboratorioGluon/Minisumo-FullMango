#ifndef MENU_H__
#define MENU_H__

#include "ssd1306.h"

#define MENU_LEN 8U
#define MENU_TEXT_LEN 17U

typedef void (*fPtrMenu)();
struct Menu;

typedef struct
{
    char text[MENU_TEXT_LEN];
    fPtrMenu action;
    struct Menu *nextMenu;
} MenuEntry;

typedef struct Menu
{
    MenuEntry entries[MENU_LEN];
    uint8_t numEntries;
    struct Menu *prevMenu;
} Menu;

typedef enum
{
    MENU_NONE = 0,
    MENU_UP,
    MENU_DOWN,
    MENU_ENTER,
    MENU_EXIT
} MenuInput;

void menu_init(ssd1306_handle_t *handle);
void menu_loop(MenuInput in);

#endif // MENU_H__