// mouse stuff for puzz on RP6502  Mostly copied from Rumbledethump's paint example
// ceptimus  April 2024

#include "mouse.h"
#include "puzzle.h"
#include "menu.h"

extern bool puzzle_quit;

static void draw_mouse_ptr(void) {
    const uint8_t data[121] = { // Amiga Workbench v2.x mouse pointer, grey top edge instead of white
         9, 8,00,00,00,00,00,00,00,00,00,
        16, 9, 8, 8,00,00,00,00,00,00,00,
        00,16, 9, 9, 8, 8,00,00,00,00,00,
        00,16, 9, 9, 9, 9, 8, 8,00,00,00,
        00,00,16, 9, 9, 9, 9, 9, 8, 8,00,
        00,00,16, 9, 9, 9, 9, 9, 9, 9,00,
        00,00,00,16, 9, 9, 9, 8,00,00,00,
        00,00,00,16, 9, 9,16, 9, 8,00,00,
        00,00,00,00,16, 9,00,16, 9, 8,00,
        00,00,00,00,16, 9,00,00,16, 9, 8,
        00,00,00,00,00,00,00,00,00,16, 9
    };
    
    uint8_t i;
    RIA.addr0 = MOUSE_PTR_DATA;
    RIA.step0 = 1;
    for (i = 0; i < 121; i++)
        RIA.rw0 = data[i];
}

static bool mouse(void) { // returns true when quit selected
    static int sx, sy, prev_x, prev_y;
    static uint8_t mb, mx, my;
    int x, y;
    uint8_t rw, changed, pressed, released;

    RIA.addr0 = MOUSE_INPUT_STRUCT + 1;
    rw = RIA.rw0;
    if (mx != rw) {
        sx += (int8_t)(rw - mx);
        mx = rw;
        if (sx < -MOUSE_DIV)
            sx = -MOUSE_DIV;
        if (sx > (CANVAS_WIDTH - 2) * MOUSE_DIV)
            sx = (CANVAS_WIDTH - 2) * MOUSE_DIV;
    }
    RIA.addr0 = MOUSE_INPUT_STRUCT + 2;
    rw = RIA.rw0;
    if (my != rw) {
        sy += (int8_t)(rw - my);
        my = rw;
        if (sy < -MOUSE_DIV)
            sy = -MOUSE_DIV;
        if (sy > (CANVAS_HEIGHT - 2) * MOUSE_DIV)
            sy = (CANVAS_HEIGHT - 2) * MOUSE_DIV;
    }
    x = sx / MOUSE_DIV;
    y = sy / MOUSE_DIV;
    xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, x_pos_px, x);
    xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, y_pos_px, y);
    RIA.addr0 = MOUSE_INPUT_STRUCT + 0;
    rw = RIA.rw0;
    changed = mb ^ rw;
    pressed = rw & changed;
    released = mb & changed;
    mb = rw;
    if (pressed & 1) {
        puzzle_click(x, y);
    }
    if (pressed & 2) {
        right_mouse_down(x, y);
        prev_x = x;
        prev_y = y;
    } else if (released & 2) {
        right_mouse_up(x, y);
    } else if (mb & 2) {
        if ((x != prev_x) || (y != prev_y)) {
            prev_x = x;
            prev_y = y;
            right_mouse_move(x, y);
        }
    }
    return puzzle_quit;
}

void mouse_init(void) {
    xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, x_wrap, false);
    xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, y_wrap, false);   
    xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, width_px, 11);
    xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, height_px, 11);
    xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, xram_data_ptr, MOUSE_PTR_DATA);
    xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);
    
    draw_mouse_ptr();
    xreg_vga_mode(3, 3, MOUSE_PTR_STRUCT, 2); // mouse pointer on (topmost) layer 2
    xreg_ria_mouse(MOUSE_INPUT_STRUCT);
}

void mouse_loop(void) {
    while (!mouse());
}