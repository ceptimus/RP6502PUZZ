/*  PUZZ sliding block puzzle game
    ported from Amiga to picocomputer RP6502 April 2024 by ceptimus
    puzzles run in a 320 * 240 * 16-colour screen
*/
#include "gfx.h"
#include "mouse.h"
#include "puzzle.h"

extern const char puzz_identifier[];
extern char puzzle_filename[];
extern char save_prompt[];
extern char line_buffer[];
extern uint8_t bytes_per_row;

static uint8_t file_number[46];
static uint8_t num_files;
static int selected = -2; // file number highlighted, or -1 for Quit, or -2 for none
uint8_t first_unused_puzz_number; // this number will be used for any saves ##.puzz 

void select(int row, int col) {
    int new_selection;
    if (row < 6 || row == 28 || col == 39 || col == 40) {
        new_selection = -2;
    } else if (row == 29 && col < 39) {
        new_selection = -1;
    } else {
        new_selection = row - 6;
        if (col > 40) new_selection += 22;
        if (new_selection >= num_files) new_selection = -2;
    }
    if (new_selection != selected) {
        
        if (selected > -2) {
            col = selected < 22 ? 0 : 41;
            row = selected == -1 ? 29 : selected % 22 + 6;
            text_colour(row, col, 15, 0, 39);
        }
        selected = new_selection;
        if (selected > -2) {
            col = selected < 22 ? 0 : 41;
            row = selected == -1 ? 29 : selected % 22 + 6;
            text_colour(row, col, 14, 4, 39);
        }
    }
}

// scanner/handler for main menu (choosing puzzle to load) on 640 x 480 canvas. returns ## of selected puzzle (0 to 43)
static uint8_t mouse(void) { 
    static int sx, sy, prev_x, prev_y;
    static uint8_t mb, mx, my;
    int x, y;
    uint8_t rw, changed, pressed, chosen;

    chosen = 0xFF;
    while (chosen == 0xFF) {
        RIA.addr0 = MOUSE_INPUT_STRUCT + 1;
        rw = RIA.rw0;
        if (mx != rw) {
            sx += (int8_t)(rw - mx);
            mx = rw;
            if (sx < -MOUSE_DIV)
                sx = -MOUSE_DIV;
            if (sx > 638 * MOUSE_DIV)
                sx = 638 * MOUSE_DIV;
        }
        RIA.addr0 = MOUSE_INPUT_STRUCT + 2;
        rw = RIA.rw0;
        if (my != rw) {
            sy += (int8_t)(rw - my);
            my = rw;
            if (sy < -MOUSE_DIV)
                sy = -MOUSE_DIV;
            if (sy > 478 * MOUSE_DIV)
                sy = 478 * MOUSE_DIV;
        }
        x = sx / MOUSE_DIV;
        y = sy / MOUSE_DIV;
        xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, x_pos_px, x);
        xram0_struct_set(MOUSE_PTR_STRUCT, vga_mode3_config_t, y_pos_px, y);
        if ((x != prev_x) || (y != prev_y)) {
            prev_x = x;
            prev_y = y;
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            select(y / 16, x / 8); // convert to row, column text coordinates
        }
        RIA.addr0 = MOUSE_INPUT_STRUCT + 0;
        rw = RIA.rw0;
        changed = mb ^ rw;
        pressed = rw & changed;
        mb = rw;
        if (pressed & 1) {
            if (selected == -1) {
                exit(0);
            } else if (selected >= 0) {
                chosen = file_number[selected];
            }
        }
        RIA.addr0 = KEYBOARD_STRUCT + 5; // contains USB HID scan code bits for Enter,Esc,Backspace,Tab,Space,-_,=+,[{
        if (RIA.rw0 & 0x02) { // Esc key pressed
            xreg(0, 0, 0x00, 0xFFFF); // keyboard access no longer needed
            exit(0);
        }
    }
    xreg(0, 0, 0x00, 0xFFFF); // keyboard access no longer needed
    return chosen;
}

void main() {
    FILE * fp;
    
    uint8_t i, row, col;
    while (true) {
        // use a character screen, 80x30 chars, 16-colour, as the 'choose puzzle' main menu
        xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, x_wrap, false);
        xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, y_wrap, false);
        xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, x_pos_px, 0);
        xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, y_pos_px, 0);
        xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, width_chars, 80);
        xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, height_chars, 30);
        xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, xram_data_ptr, CHARACTER_DATA);
        xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, xram_palette_ptr, 0xFFFF);
        xram0_struct_set(CHARACTER_STRUCT, vga_mode1_config_t, xram_font_ptr, 0xFFFF);

        erase_characters();
        xreg_vga_canvas(3);
        xreg(1, 0, 1, 1, 10, CHARACTER_STRUCT, 0); // character mode (Mode 1) on layer 0
        bytes_per_row = 160;
        text_at(0, 10, 11, 0, "PUZZ sliding block puzzle games for the picocomputer RP5502");
        text_at(2, 0, 11, 0, "You need a mouse to play these puzzles. Click a puzzle below, to load it.");
        text_at(3, 0, 11, 0, "Once a puzzle is displayed, left-click on a piece to move it, or right-click to");
        text_at(4, 0, 11, 0, "access the menus for: instructions, restarting, saving, quitting, etcetera.");
        text_at(29, 2, 15, 0, "Quit");
        text_at(29, 18, 15, 0, "Or press Esc to quit");
        text_at(29, 65, 8, 0, "\xB8 2024 ceptimus");

        // wouldn't have to search like this if there were a way to read a directory.  Maybe in a future release?
        first_unused_puzz_number = 43; // will hold number used for saving a puzzle
        num_files = 0; // number of ##.puzz files found (only room on screen for 44)
        for (i = 0; i < 44; i++) {
            sprintf(puzzle_filename, "%02u.puzz", i);
            row = num_files > 21 ? num_files - 16 : num_files + 6;
            col = num_files > 21 ? 41 : 0;
            fp = fopen(puzzle_filename, "r");
            if (fp == NULL) {
                puzzle_filename[2] = '\0';
                text_at(row, col, 15, 0, puzzle_filename);
                if (i < first_unused_puzz_number) first_unused_puzz_number = i;
                continue;
            } else {
                if (fgets(line_buffer, MAX_LINE, fp) && !strncmp(line_buffer, puzz_identifier, 13)) { // if a valid ##.puzz file
                    read_line_n(fp, 1, puzzle_filename);
                    line_buffer[13] = '\0';
                    text_at(row, col + 3, 15, 0, line_buffer);
                    read_line_n(fp, 1, puzzle_filename);
                    line_buffer[21] = '\0';
                    text_at(row, col + 18, 15, 0, line_buffer);
                    puzzle_filename[2] = '\0';
                    text_at(row, col, 15, 0, puzzle_filename);
                    file_number[num_files] = i;
                    num_files++;
                }
                fclose(fp);
            }
        }
        if (!num_files) {
            puts("No ##.puzz files found (where ## = 00 to 43).");
            puts("Have you cd-ed to the correct directory?");
            exit(1);
        } else if (num_files < 44) {
            row = num_files > 21 ? num_files - 16 : num_files + 6;
            col = num_files > 21 ? 41 : 0;
            text_at(row, col, 0, 0, "  ");
        }
        mouse_init();
        xreg(0, 0, 0x00, KEYBOARD_STRUCT); // enable keyboard access to detect pressing of Esc key
        sprintf(puzzle_filename, "%02u.puzz\n", mouse());
        sprintf(save_prompt, "Save (%02u.puzz)", first_unused_puzz_number);
        gfx_init();
        mouse_init();
        puzzle_load();
        mouse_loop();
    }
}
