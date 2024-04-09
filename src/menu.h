#ifndef _MENU_
    #include "puzz.h"
    #include "gfx.h"

    struct MenuItem {
        void (*action)(void);
        struct MenuItem * next;
        uint8_t row;
        char * text;
    };
    struct Menu {
        uint8_t left, box_width, box_height; // # of chars, # rows of chars, that will fit inside box
        int8_t left_offset; // signed, so left edge of box may be to the left of menu title
        char * title;
        struct MenuItem * first;
        struct Menu * next; // NULL for last menu
    };
    struct MenuBar {
        uint8_t fg_colour, bg_colour, fg_highlight, bg_highlight;
        struct Menu * first;
    };

    void right_mouse_down(int x, int y);
    void right_mouse_up(int x, int y);
    void right_mouse_move(int x, int y);
    #define _MENU_
#endif