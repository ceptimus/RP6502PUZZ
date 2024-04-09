/*  PUZZ sliding block puzzle game
    ported from Amiga to picocomputer RP6502 March 2024 by ceptimus
*/
#ifndef _PUZZ_    
    #include <rp6502.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <string.h>

// graphics 
    #define CANVAS_WIDTH 320
    #define CANVAS_HEIGHT 240
    // XRAM locations
    // BITMAP DATA (320x240 x 4 bits) in XRAM from 0x0000 to 0x95FF
    #define BITMAP_DATA 0x0000
    // PALETTE DATA (16 x 16 bits) in XRAM from 0x9600 to 0x961F
    // follows on directly after BITMAP_DATA, so can be loaded by same read_xram() call
    #define PALETTE_DATA 0x9600
    // 0x9620 to 0xEC4F unallocated, and can be used for off-screen bitmap copies
    // CHARACTER_DATA (80 x 30 chars x 16 bits) in XRAM from 0xEC50 to 0xFF0F
    // for the 40 x 30  character data overlaying the puzzles (menu and Moves count)
    // only the first half of the CHARACTER_DATA XRAM is used: from EC50 to F5AF
    #define CHARACTER_DATA 0xEC50
    // MOUSE POINTER DATA (11x11 x 8 bits) from 0xFF10 to 0xFF88
    #define MOUSE_PTR_DATA 0xFF10
    // keyboard data from 0xFF90 to 0xFFAF
    #define KEYBOARD_STRUCT 0xFF90
    #define CHARACTER_STRUCT 0xFFB0
    #define BITMAP_STRUCT 0xFFD0
    #define MOUSE_PTR_STRUCT 0xFFE0

// mouse
    // Mouse speed divider
    #define MOUSE_DIV 4
    #define MOUSE_INPUT_STRUCT 0xFFF0

// puzzles
    #define MAX_LINE 80
    #define MAX_ACROSS 32
    #define MAX_DOWN 32
    #define MAX_PIECES 255

    #define _PUZZ_
#endif