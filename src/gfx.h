#ifndef _GFX_
    #include "puzz.h"

    void gfx_init(void);
    void erase_bitmap(void);
    void gfx_move(int src_left, int src_top, int dest_left, int dest_top, uint8_t width, uint8_t height, uint8_t fill);
    void text_at(uint8_t row, uint8_t col, uint8_t fg, uint8_t bg, char * text);
    void n_chars_at(uint8_t row, uint8_t col, uint8_t fg, uint8_t bg, int n, char c);
    void text_colour(uint8_t row, uint8_t col, uint8_t fg, uint8_t bg, int length);
    void erase_characters(void); // clear text screen
    void scroll_screen(void); // diagonal scroll to celebrate puzzle completion 
    #define _GFX_
#endif