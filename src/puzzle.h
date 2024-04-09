#ifndef _PUZZLE_
    #include "puzz.h"
    // bitmap copies to/from following coordinates which target the unallocated RAM above the bitmap palette
    #define OFF_SCREEN_TOP 240
    #define OFF_SCREEN_LEFT 64
    
    void puzzle_load(void);
    void puzzle_save(void);
    void puzzle_click(int x, int y);
    void read_line_n(FILE * fp, uint8_t n, char *puzzle_filename);
    
    #define _PUZZLE_
#endif
