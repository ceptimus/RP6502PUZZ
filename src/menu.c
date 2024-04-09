// Amiga Intuition-like menus for PUZZ on the RP6502
// ceptimus, April 2024

#include "menu.h"
#include "puzzle.h"

extern bool puzzle_quit;

static void quit(void);

char instructions[6][27];
char save_prompt[15];

static struct MenuItem item_about2 = { NULL, NULL, 2, "Amiga April 2024" };
static struct MenuItem item_about1 = { NULL, &item_about2, 1, " Converted from" };
static struct MenuItem item_about0 = { NULL, &item_about1, 0, "RP6502 Puzz V1.0" };
static struct Menu menu_about = {21, 16, 3, 0, "About", &item_about0, NULL };

static struct MenuItem item_instructions5 = { NULL, NULL, 5, instructions[5] };
static struct MenuItem item_instructions4 = { NULL, &item_instructions5, 4, instructions[4] };
static struct MenuItem item_instructions3 = { NULL, &item_instructions4, 3, instructions[3] };
static struct MenuItem item_instructions2 = { NULL, &item_instructions3, 2, instructions[2] };
static struct MenuItem item_instructions1 = { NULL, &item_instructions2, 1, instructions[1] };
static struct MenuItem item_instructions0 = { NULL, &item_instructions1, 0, instructions[0] };
static struct Menu menu_instructions = { 8, 26, 6, 0, "Instructions", &item_instructions0, &menu_about };

static struct MenuItem item_quit = { quit, NULL, 2, "Quit" };
static struct MenuItem item_save = { puzzle_save, &item_quit, 1, save_prompt};
static struct MenuItem item_restart = { puzzle_load, &item_save, 0, "Restart" };
static struct Menu menu_puzzle = { 1, 14, 3, 0, "Puzzle", &item_restart, &menu_instructions };

static struct MenuBar menu_bar =  { 15, 8, 14, 4, &menu_puzzle };

static struct Menu * active_menu = NULL;
static struct MenuItem * active_item = NULL;
static uint8_t menu_bottom = 0;
static uint8_t menu_left, menu_right;

static void quit(void) {
    puzzle_quit = true;
}

static void erase_active_menu(void) {
    uint8_t row;
    for (row = 1; row <= menu_bottom; row++) {
        n_chars_at(row, menu_left - 1, 0, 0, menu_right - menu_left + 2, 0);
    }
    menu_bottom = 0;
}

static void draw_active_menu(void) {
    uint8_t row;
    struct MenuItem *item;
    menu_left = active_menu->left + active_menu->left_offset;
    menu_right = menu_left + active_menu->box_width;
    menu_bottom = active_menu->box_height + 2;
    // draw the box - assumes box-drawing chars are present in font at standard (code page 850) locations
    n_chars_at(1, menu_left-1, menu_bar.fg_colour, menu_bar.bg_colour, 1, 218); // top left corner
    n_chars_at(1, menu_right, menu_bar.fg_colour, menu_bar.bg_colour, 1, 191); // top right corner
    n_chars_at(menu_bottom, menu_left-1, menu_bar.fg_colour, menu_bar.bg_colour, 1, 192); // bottom left corner
    n_chars_at(menu_bottom, menu_right, menu_bar.fg_colour, menu_bar.bg_colour, 1, 217); // bottom right corner
    n_chars_at(1, menu_left, menu_bar.fg_colour, menu_bar.bg_colour, menu_right - menu_left, 196); // top
    n_chars_at(menu_bottom, menu_left, menu_bar.fg_colour, menu_bar.bg_colour, menu_right - menu_left, 196); // bottom
    item = active_menu->first;
    for (row = 2; row < menu_bottom; row++) {
        n_chars_at(row, menu_left - 1, menu_bar.fg_colour, menu_bar.bg_colour, menu_right - menu_left + 2, 179); // vertical edges
        n_chars_at(row, menu_left, menu_bar.fg_colour, menu_bar.bg_colour, menu_right - menu_left, ' '); // fill box with spaces
        if (item) {
            text_at(row, menu_left, menu_bar.fg_colour, menu_bar.bg_colour, item->text);
            item = item->next;
        }
    }
}

void cancel_active_item(void) {
    if (active_item) {
        text_colour(active_item->row + 2, menu_left, menu_bar.fg_colour, menu_bar.bg_colour, menu_right - menu_left);
    }
    active_item = NULL;
}
void right_mouse_move(int x, int y) {
    struct Menu *menu;
    struct MenuItem *item;
    uint8_t i;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    x >>= 3, y >>= 3; // convert from pixel to character coordinates
    if (!y) { // if mouse pointer is on menu bar
        for (menu = menu_bar.first; menu; menu = menu->next) {
            if (x >= menu->left && x < menu->left + strlen(menu->title)) {
                if (active_menu != menu) {
                    if (active_menu) {
                        erase_active_menu();
                        text_at(0, active_menu->left, menu_bar.fg_colour, menu_bar.bg_colour, active_menu->title);
                    }
                    text_at(0, menu->left, menu_bar.fg_highlight, menu_bar.bg_highlight, menu->title);
                    active_menu = menu;
                    draw_active_menu();
                }
                break;
            }
        }
       cancel_active_item();
    } else if (y > 1 && y < menu_bottom && x >= menu_left && x < menu_right) { // mouse over text in active menu
        item = active_menu->first;
        i = 2;
        while (item && i < y) {
            item = item->next;
            i++;
        }
        if (active_item) {
            if (item != active_item) {
                cancel_active_item();
            }
            if (item) {
                text_at(i, menu_left, menu_bar.fg_highlight, menu_bar.bg_highlight, item->text);
            }
        }
        active_item = item;
    } else {
        cancel_active_item();
    }
}

void right_mouse_down(int x, int y) {
    struct Menu *menu;
    n_chars_at(0, 0, menu_bar.fg_colour, menu_bar.bg_colour, 40, ' '); // grey background top row of text

    for (menu = menu_bar.first; menu; menu = menu->next) {
        text_at(0, menu->left, menu_bar.fg_colour, menu_bar.bg_colour, menu->title);
    }
    right_mouse_move(x, y);
}

void right_mouse_up(int x, int y) {
    right_mouse_move(x, y);
    n_chars_at(0, 0, 0, 0, 40, 0);
    if (active_menu) {
        erase_active_menu();
        active_menu = NULL;
    }
    if (active_item && active_item->action) {
        active_item->action();
    }
    active_item = NULL;
}