// low-level graphics stuff for PUZZ on RP6502
// ceptimus April 2024

#include "gfx.h"

uint8_t bytes_per_row;

void erase_bitmap(void) {
    unsigned i;
    RIA.addr0 = BITMAP_DATA;
    RIA.step0 = 1;
    for (i = 0; i < PALETTE_DATA; i++)
        RIA.rw0 = 0;
    for (i = 0; i < 32; i++)
        RIA.rw0 = 0; // initial palette all black
}

void text_at(uint8_t row, uint8_t col, uint8_t fg, uint8_t bg, char * text) {
    fg = (fg & 0x0F) | (bg << 4);
    RIA.addr0 = CHARACTER_DATA  + row * bytes_per_row + col * 2;
    RIA.step0 = 1;
    while (*text) {
        RIA.rw0 = *text++;
        RIA.rw0 = fg;
    }
}

void text_colour(uint8_t row, uint8_t col, uint8_t fg, uint8_t bg, int length) {
    fg = (fg & 0x0F) | (bg << 4);
    RIA.addr0 = CHARACTER_DATA + row * bytes_per_row + col * 2 + 1;
    RIA.step0 = 2;
    while (length--) {
        RIA.rw0 = fg;
    }
}

void n_chars_at(uint8_t row, uint8_t col, uint8_t fg, uint8_t bg, int n, char c) {
    fg = (fg & 0x0F) | (bg << 4);
    
    RIA.addr0 = CHARACTER_DATA + row * 80 + col * 2;
    RIA.step0 = 1;
    while (n--) {
        RIA.rw0 = c;
        RIA.rw0 = fg;
    }
}

void erase_characters(void) { // clear text screen
    n_chars_at(0, 0, 0, 0, 2400, 0);
}

// move a rectangle of pixels, no overlap of source and destination, pixels are 4 bits wide
// source rectangle is filled with colour fill
// with default PUZZ XRAM allocation, any coordinate after top:240, left:63 will target unallocated off-screen XRAM
void gfx_move(int src_left, int src_top, int dest_left, int dest_top, uint8_t width, uint8_t height, uint8_t fill) {
    unsigned u;
    uint8_t shift, b, w, h, next_row_step0, next_row_step1, preload_left, trailing_right;
    uint8_t fill_high, fill_both;    
    shift = ((uint8_t)src_left ^ (uint8_t)dest_left) & 0x01;
    fill_high = fill << 4;
    fill_both = fill | fill_high;
    RIA.step0 = RIA.step1 = 0;
    if (src_left & 0x01) { // moving/clearing low nibble of source left hand edge
        u = src_top << 5;
        RIA.addr0 = u + (u << 2) + (src_left >> 1); // address containing top left pixel of source
        u = dest_top << 5;
        RIA.addr1 = u + (u << 2) + (dest_left >> 1); // address containing top left pixel of destination
        h = height;
        while (h--) {
            b = shift ? RIA.rw0 << 4 : RIA.rw0 & 0x0F; // get nibble in position
            RIA.rw0 = (RIA.rw0 & 0xF0) | fill; // overwrite source nibble to fill colour
            RIA.rw1 = shift ? (RIA.rw1 & 0x0F) | b : (RIA.rw1 & 0xF0) | b; // write pixel to destination
            RIA.addr0 += 160, RIA.addr1 += 160; // move down to next pixel
        }
        src_left++, dest_left++, width--;
    } else if (shift) { // moving/clearing high nibble of source left hand edge to low nibble of destination
        u = src_top << 5;
        RIA.addr0 = u + (u << 2) + (src_left >> 1); // address containing top left pixel of source
        u = dest_top << 5;
        RIA.addr1 = u + (u << 2) + (dest_left >> 1); // address containing top left pixel of destination
        h = height;
        while (h--) {
            b = RIA.rw0 >> 4; // get nibble in position
            RIA.rw0 = (RIA.rw0 & 0x0F) | fill_high; // overwrite source nibble to fill colour
            RIA.rw1 = (RIA.rw1 & 0xF0) | b; // write pixel to destination
            RIA.addr0 += 160, RIA.addr1 += 160; // move down to next pixel
        }
        src_left++, dest_left++, width--;
    }
    if ((src_left + width) & 0x01) { // moving/clearing high nibble of source right hand edge
        u = src_top << 5;
        RIA.addr0 = u + (u << 2) + ((src_left + width - 1) >> 1); // address containing top right pixel of source
        u = dest_top << 5;
        RIA.addr1 = u + (u << 2) + ((dest_left + width - 1) >> 1); // address containing top right pixel of destination
        h = height;
        while (h--) {
            b = shift ? RIA.rw0 >> 4 : RIA.rw0 & 0xF0; // get nibble in position
            RIA.rw0 = (RIA.rw0 & 0x0F) | fill_high; // overwrite source nibble to fill colour
            RIA.rw1 = shift ? (RIA.rw1 & 0xF0) | b : (RIA.rw1 & 0x0F) | b; // write pixel to destination
            RIA.addr0 += 160, RIA.addr1 += 160; // move down to next pixel
        }
        width--;
    } else if (shift) { // moving/clearing low nibble of source right hand edge to high nibble of destination
        u = src_top << 5;
        RIA.addr0 = u + (u << 2) + ((src_left + width - 2) >> 1); // address containing top right pixel of source
        u = dest_top << 5;
        RIA.addr1 = u + (u << 2) + ((dest_left + width - 1) >> 1); // address containing top right pixel of destination
        h = height;
        while (h--) {
            b = RIA.rw0 << 4; // get nibble in position
            RIA.rw0 = (RIA.rw0 &= 0xF0) | fill; // overwrite source nibble to fill colour
            RIA.rw1 = (RIA.rw1 & 0x0F) | b; // write pixel to destination
            RIA.addr0 += 160, RIA.addr1 += 160; // move down to next pixel
        }
        width--;
    }
    // now copy rest of block. all source bytes can be filled, two pixels at once, with fill colour, as we go
    if (shift) { 
        u = src_top << 5;
        RIA.addr0 = u + (u << 2) + (src_left >> 1); // address containing top left pixel of source
        u = dest_top << 5;
        RIA.addr1 = u + (u << 2) + (dest_left >> 1); // address containing top left pixel of destination
        h = height;
        preload_left = dest_left & 0x01;
        trailing_right = (dest_left + width) & 0x01;        
        next_row_step0 = 159 - (width >> 1);
        next_row_step1 = next_row_step0 + 1 - preload_left;
        width = (width + preload_left) >> 1;
        while (h--) {
            if (preload_left) { // preload high nibble of first b from destination
                RIA.step1 = 0;
                b = RIA.rw1 & 0xF0;
            } else { // get high nibble of first b from source
                b = RIA.rw0 << 4;
                RIA.rw0 = fill_both; // write fill colour to two pixels
                RIA.addr0++;
            }
            RIA.step1 = 1;
            w = width;            
            while (w--) {
                RIA.rw1 = b | (RIA.rw0 >> 4);
                b = RIA.rw0 << 4;
                RIA.rw0 = fill_both; // write fill colour to two pixels
                RIA.addr0++;
            }
            if (trailing_right) { // rightmost pixel of rectangle to write to high nibble of destination
                RIA.step1 = 0;
                RIA.rw1 = (RIA.rw1 & 0x0F) | b;
            }
            RIA.addr0 += next_row_step0;
            RIA.addr1 += next_row_step1;
        }
    } else { // byte aligned: no shifts needed
        width >>= 1;
        next_row_step0 = 160 - width;
        u = src_top << 5;
        RIA.addr0 = u + (u << 2) + (src_left >> 1); // address containing top left pixel of source
        u = dest_top << 5;
        RIA.addr1 = u + (u << 2) + (dest_left >> 1); // address containing top left pixel of destination
        RIA.step1 = 1;
        h = height;
        while (h--) {
            w = width;
            while (w--) {
                RIA.rw1 = RIA.rw0;
                RIA.rw0 = fill_both; // write fill colour to two pixels
                RIA.addr0++;
            }
            RIA.addr0 += next_row_step0;
            RIA.addr1 += next_row_step0;
        }
    }
}

void gfx_init(void) {
    xreg_vga_canvas(1);

    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, x_wrap, true);
    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, y_wrap, true);
    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, x_pos_px, 0);
    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, y_pos_px, 0);
    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, width_px, CANVAS_WIDTH);
    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, height_px, CANVAS_HEIGHT);
    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, xram_data_ptr, BITMAP_DATA);
    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, xram_palette_ptr, PALETTE_DATA);

    xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, x_wrap, false);
    xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, y_wrap, false);
    xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, x_pos_px, 0);
    xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, y_pos_px, 0);
    xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, width_chars, 40);
    xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, height_chars, 30);
    xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, xram_data_ptr, CHARACTER_DATA);
    xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, xram_palette_ptr, 0xFFFF);
    xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, xram_font_ptr, 0xFFFF);

    erase_bitmap();
    xreg_vga_mode(3, 2, BITMAP_STRUCT, 0);
    erase_characters();
    xreg(1, 0, 1, 1, 2, CHARACTER_STRUCT, 1); // character mode (Mode 1) on layer 1
    bytes_per_row = 80;
    // printf("\x0C\x1B[92;40m"); // clear console, bright green text
}

void scroll_screen(void) {
    int offset;
    uint8_t delta, vsync;
    offset = 0;
    for (delta = 1; delta < 6; delta++) {
        offset -= delta;
        xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, x_pos_px, offset);
        xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, y_pos_px, offset * 3 / 4);
        vsync = RIA.vsync; while (vsync == RIA.vsync) continue;
    }
    while (offset > -300) {
        offset -= delta;
        xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, x_pos_px, offset);
        xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, y_pos_px, offset * 3 / 4);
        vsync = RIA.vsync; while (vsync == RIA.vsync) continue;
    }
    for (delta = 5; delta; delta--) {
        offset -= delta;
        xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, x_pos_px, offset);
        xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, y_pos_px, offset * 3 / 4);
        vsync = RIA.vsync; while (vsync == RIA.vsync) continue;
    }
    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, x_pos_px, 0);
    xram0_struct_set(BITMAP_STRUCT, vga_mode3_config_t, y_pos_px, 0);
}