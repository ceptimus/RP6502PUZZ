// loading and manipulating puzzle.  Much of it ported from a 1989 Amiga version

#include "puzzle.h"
#include "gfx.h"

enum Direction {
    NONE,
    LEFT,
    UP,
    RIGHT,
    DOWN
};

extern char instructions[6][27];
extern uint8_t first_unused_puzz_number;

const char puzz_identifier[17] = "PUZZ_RP6502_V1.0";
char puzzle_filename[16];
char line_buffer[MAX_LINE];
bool puzzle_quit;

static uint8_t grid[MAX_DOWN][MAX_ACROSS];
static uint8_t goal[MAX_DOWN][MAX_ACROSS];
static uint8_t move_list[MAX_PIECES][4];

static int top_left_x, top_left_y, moves, start_moves;
static uint8_t squares_across, squares_down, square_width, square_height, slide, moves_col, moves_row, moves_fg, moves_bg;
static char puzzle_name[14]; // used when saving puzzle

// read line from text file to line_buffer. check it's at least n chars long. abort with error on failure
// also prune any comments (starting with semicolon) and trailing whitespace
void read_line_n(FILE * fp, uint8_t n, char *puzzle_filename) {
    char * c;

    if (!fgets(line_buffer, MAX_LINE, fp)) {
        printf("Unexpected EOF reading %s\n", puzzle_filename);
        fclose(fp);
        exit(1);
    }
    c = strchr(line_buffer, ';'); // check for comment
        if (c) *c = '\0'; // prune comment if present

    // Trim trailing whitespace
    c = line_buffer + strlen(line_buffer) - 1;
    while(c > line_buffer && strchr(" \n\r\t", *c)) *c-- = '\0';

    if (strlen(line_buffer) < n) {
        printf("Line in %s too short: %s\n", puzzle_filename, line_buffer);
        fclose(fp);
        exit(1);
    }
}

void show_grid(uint8_t g[MAX_DOWN][MAX_ACROSS]) {
    int row, col;
    for (row = 0; row < squares_down; row++) {
        for (col = 0; col < squares_down; col++) {
            printf("%d,", g[row][col]);
        }
        printf("\n");
    }
    printf("\n");
}

static void update_score(void) {
    moves++;
    sprintf(line_buffer, "Moves:%4u", moves);
    text_at(moves_row, moves_col, moves_fg, moves_bg, line_buffer);
}

static void check_if_complete(void) {
    uint8_t row, col;
    bool complete;
    if (moves) { // except when (re)starting the puzzle, check if it's been completed
        complete = true;
        for (row = 0; complete && row < squares_down; row++) {
            for (col = 0; complete && col < squares_across; col++) {
                if (goal[row][col] && goal[row][col] != grid[row][col]) complete = false;
            }
        }
        if (complete) scroll_screen();
    }    
}

void puzzle_load(void) { // aborts with error message, or returns silently on success
    FILE * fp;
    int fd; // file descriptor for open()
    uint8_t i, j;
    char * c;

    puzzle_quit = false;
    fp = fopen(puzzle_filename, "r");
    if (fp == NULL) {
        printf("File not found error\n  puzzle_load(\"%s\")\n", puzzle_filename);
        exit(1);
    }
    read_line_n(fp, 16, puzzle_filename); // 1st line of file is PUZZ identifier (and required minimum version number)
	if (strncmp(line_buffer, "PUZZ_RP6502_V", 13)) {
        printf("%s missing %s identifier\n", puzzle_filename, puzz_identifier);
        fclose(fp);
        exit(1);
    }
    read_line_n(fp, 1, puzzle_filename);
    strncpy(puzzle_name, line_buffer, 13);
    read_line_n(fp, 1, puzzle_filename); // throw away (here) description line used by puzzle choice screen
    read_line_n(fp, 1, puzzle_filename);
    start_moves = atoi(line_buffer);
    for (i = 0; i < 6; i++) { // 6 lines of up to 26 chars of puzzle instructions
        read_line_n(fp, 0, puzzle_filename);
        strncpy(instructions[i], line_buffer, 26);
    }
    read_line_n(fp, 1, puzzle_filename);
    squares_across = (uint8_t)atoi(line_buffer);
    read_line_n(fp, 1, puzzle_filename);
    squares_down = (uint8_t)atoi(line_buffer);
    for (i = 0; i < squares_down; i++) {
        read_line_n(fp, squares_across + squares_across - 1, puzzle_filename);
		grid[i][0] = (uint8_t)atoi(strtok(line_buffer, " ,"));
		for (j = 1; j < squares_across; j++) {
			grid[i][j] = (uint8_t)atoi(strtok(NULL, " ,"));
		}
    }
    for (i = 0; i < squares_down; i++) {
        read_line_n(fp, squares_across + squares_across - 1, puzzle_filename);
		goal[i][0] = (uint8_t)atoi(strtok(line_buffer, " ,"));
		for (j = 1; j < squares_across; j++) {
			goal[i][j] = (uint8_t)atoi(strtok(NULL, " ,"));
		}
    }
    read_line_n(fp, 1, puzzle_filename);
    top_left_x = atoi(line_buffer);
    read_line_n(fp, 1, puzzle_filename);
    top_left_y = atoi(line_buffer);
    read_line_n(fp, 1, puzzle_filename);
    square_width = (uint8_t)atoi(line_buffer);
    read_line_n(fp, 1, puzzle_filename);
    square_height = (uint8_t)atoi(line_buffer);
    read_line_n(fp, 1, puzzle_filename);
    slide = (uint8_t)atoi(line_buffer);
    read_line_n(fp, 1, puzzle_filename);
    moves_col = atoi(line_buffer);
    read_line_n(fp, 1, puzzle_filename);
    moves_row = atoi(line_buffer);
    read_line_n(fp, 1, puzzle_filename);
    moves_fg = (uint8_t)atoi(line_buffer);
    read_line_n(fp, 1, puzzle_filename);
    moves_bg = (uint8_t)atoi(line_buffer);
    read_line_n(fp, 10, puzzle_filename); // 1st line of file is PUZZ identifier (and required minimum version number)
	if (strncmp(line_buffer, "**CANVAS**", 10)) {
        printf("%s missing **CANVAS** identifier\n", puzzle_filename);
        fclose(fp);
        exit(1);
    }
    fclose(fp);

    fd = open(puzzle_filename, O_RDONLY);
    if (fd < 0) {
        printf("File not found error\n  open(\"%s\", O_RDONLY)\n", puzzle_filename);
        exit(1);
    }
    // read and discard up to **CANVAS**<CR><LF>
    i = 0; // count of linefeeds
    while (i < 2 * squares_down + 21) {
        if (read(fd, line_buffer, 11) != 11) {            
            printf("Error searching for **CANVAS** in %s\n", puzzle_filename);
            close(fd);
            exit(1);
        }
        line_buffer[11] = '\0';
        for (c = line_buffer; *c; c++) {
            if (*c == '\n') {
                i++;
            }
        }
    }
    do {
        read(fd, line_buffer, 1);
    } while (line_buffer[0] != '\n');
    // now we're at the start of the binary data for the image and palette
    read_xram(0, 0x4B00, fd); // 0x7FFF bytes maximum, so read first half of 0x9600 bytes
    read_xram(0x4B00, 0x4B20, fd); // second half plus palette
    close(fd);
    for (i = 0; i < MAX_PIECES; i++) {
		for (j = 0; j < 4; j++) {
			move_list[i][j] = j + 1;
        }
    }
    n_chars_at(moves_row, moves_col, 0, 0, 10, ' '); // erase any moves count left when restarting
    moves = start_moves - 1;
    update_score();
}

void puzzle_save(void) {
    int fd; // file descriptor for open()
    uint8_t i, j;

    sprintf(line_buffer, "%02u.puzz", first_unused_puzz_number);
    fd = open(line_buffer, O_CREAT | O_WRONLY);
    if (fd < 0) {
        printf("Couldn't create save file\n  open(\"%s\", O_CREAT | O_WRONLY)\n", line_buffer);
        exit(1);
    }
    write(fd, puzz_identifier, strlen(puzz_identifier));
    sprintf(line_buffer, "\n%s\nSaved (%u moves)\n%u\n", puzzle_name, moves, moves);
    write(fd, line_buffer, strlen(line_buffer));
    for (i = 0; i < 6; i++) {
        sprintf(line_buffer, "%s\n", instructions[i]);
        write(fd, line_buffer, strlen(line_buffer));
    }
    sprintf(line_buffer, "%u\n%u\n", squares_across, squares_down);
    write(fd, line_buffer, strlen(line_buffer));
    for (i = 0; i < squares_down; i++) {
        for (j = 0; j < squares_across; j++) {
            sprintf(line_buffer, "%u%c", grid[i][j], j == squares_across - 1 ? '\n' : ',');
            write(fd, line_buffer, strlen(line_buffer));
        }
    }
    for (i = 0; i < squares_down; i++) {
        for (j = 0; j < squares_across; j++) {
            sprintf(line_buffer, "%u%c", goal[i][j], j == squares_across - 1 ? '\n' : ',');
            write(fd, line_buffer, strlen(line_buffer));
        }
    }
    sprintf(line_buffer, "%u\n%u\n%u\n%u\n", top_left_x, top_left_y, square_width, square_height);
    write(fd, line_buffer, strlen(line_buffer));
    sprintf(line_buffer, "%u\n%u\n%u\n%u\n%u\n", slide, moves_col, moves_row, moves_fg, moves_bg);
    write(fd, line_buffer, strlen(line_buffer));
    write(fd, "**CANVAS**\n", 11);
    write_xram(0, 0x4B00, fd); // 0x7FFF bytes maximum, so read first half of 0x9600 bytes
    write_xram(0x4B00, 0x4B20, fd); // second half plus palette
    close(fd);
}

static bool can_move(uint8_t piece, int direction) {
	uint8_t row, col, old;
	for (row = 0; row < squares_down; row++) {
		for (col = 0; col < squares_across; col++) {
			if (piece == grid[row][col]) {    
				switch (direction) {
					case LEFT:
						if (col == 0) return false;
						old = grid[row][col - 1];
						break;
					case UP:
						if (row == 0) return false;
						old = grid[row - 1][col];
						break;
					case RIGHT:
						if (col == squares_across - 1) return false;
						old = grid[row][col + 1];
						break;
					case DOWN:
						if (row == squares_down - 1) return false;
						old = grid[row + 1][col];
						break;
                    default:
                        return false;
                }
				if (old && (old != piece)) return false;
            }
        }
    }
	return true;
}

static void sort_list (uint8_t piece, int move) {
	int i;
	if ((move += 2) > 4) move -= 4;	/*	opposite direction of move	*/
	
	for (i = 0; i < 3; i++) {
		if (move == move_list[piece][i]) {
			while (i < 3) {
				move_list[piece][i] = move_list[piece][i + 1];
				i++;
			}
			move_list[piece][3] = move;	/*	last in new list	*/
			return;
		}
	}
}
	
static int suggest_move(uint8_t piece) {
	int i, move;
	for (i = 0; i < 4; i++)	{	/*	look through move list	*/
		if (can_move(piece, move = move_list[piece][i])) {
			sort_list (piece, move);
			return move;
			}
		}
	return NONE;
}

static void move_one_piece(uint8_t piece, int direction) {
    int x, y;
    int8_t inner_step;
    uint8_t row, col, inner_first, inner_last, outer_limit;
    uint8_t *outer, *inner;
    switch (direction) {
        case LEFT:
            outer = &row;
            outer_limit = squares_down;
            inner = &col;
            inner_first = 0;
            inner_last = squares_across - 1;
            inner_step = 1;
            break;
        case UP:
            outer = &col;
            outer_limit = squares_across;
            inner = &row;
            inner_first = 0;
            inner_last = squares_down - 1;
            inner_step = 1;
            break;
        case RIGHT:
            outer = &row;
            outer_limit = squares_down;
            inner = &col;
            inner_first = squares_across - 1;
            inner_last = 0;
            inner_step = -1;
            break;
        case DOWN:
            outer = &col;
            outer_limit = squares_across;
            inner = &row;
            inner_first = squares_down - 1;
            inner_last = 0;
            inner_step = -1;
            break;
        default:
            return;
    }
    for (*outer = 0; *outer < outer_limit; (*outer)++) {
        *inner = inner_first;
        while (true) {
            if (grid[row][col] == piece) {
                x = top_left_x + col * square_width;
                y = top_left_y + row * square_height;
                switch(direction) {
                    case LEFT:
                    case RIGHT:
                        grid[row][col - inner_step] = piece;
                        gfx_move(x, y, x - inner_step * square_width, y, square_width, square_height, 0);
                        break;
                    case UP:
                    case DOWN:
                        grid[row - inner_step][col] = piece;
                        gfx_move(x, y, x, y - inner_step * square_height, square_width, square_height, 0);
                        break;
                    default:
                        puts("Bad direction in move_one_piece()");
                        exit(1);
                }
                grid[row][col] = 0;
            }
            if (*inner == inner_last) break;
            *inner += inner_step;
        } 
    }
}

static int move_piece(uint8_t piece) {
	int direction;
	
	if ((direction = suggest_move(piece)) != NONE) {
		do {
			move_one_piece(piece, direction);
		} while (slide && can_move(piece, direction));
		update_score();	
	}
	return direction;
}

static void slide_pieces(uint8_t piece) {
	int i, j, x, y, zx, zy, direction;
	int flag = 1;
	
	for (i = 0; flag && i < squares_down; i++) {
		for (j = 0; flag && j < squares_across; j++) {
			if (grid[i][j] == piece) {
				x = j;
				y = i;
				flag = 0;
			}
        }
    }
	
	for (i = 0; i < squares_across; i++) {
		if (grid[y][i] == 0) {
			flag = 1;
			zx = i;
			zy = y;
		}
    }
	
	if (flag == 0) {
		for(i = 0; i < squares_down; i++) {
			if (grid[i][x] == 0) {
				flag = 1;
				zx = x;
				zy = i;
			}
        }
    }
	
	if (flag == 0) return;

	if (x == zx) {
		if (y > zy) {
			for (i = zy + 1; i < y; i++) {
				if (grid[i][x] == 255) return;
            }
			direction = UP;
		} else {
			for (i = y + 1; i < zy; i++) {
				if (grid[i][x] == 255) return;
            }
			direction = DOWN;
        }
	} else {
		if (x > zx) {
			for (i = zx + 1; i < x; i++) {
				if (grid[y][i] == 255) return;
            }
			direction = LEFT;
		} else {
			for (i = x + 1; i < zx; i++) {
				if (grid[y][i] == 255) return;
            }
			direction = RIGHT;
		}
	}
	piece = 0;
    if (x == zx) {
		if (y > zy) {
			for (i = zy + 1; i <= y; i++) {
				if ((grid[i][x]) && (grid[i][x] != piece)) {
					if (can_move(piece = grid[i][x], direction)) {
						move_one_piece(piece, direction);
						if (flag) {
							flag = 0;
							update_score();
						}
					} else return;
				}
			}
        } else {
			for (i = zy - 1; i >= y; i--) {
				if ((grid[i][x]) && (grid[i][x] != piece)) {
					if (can_move(piece = grid[i][x], direction)) {
						move_one_piece(piece, direction);
						if (flag) {
							flag = 0;
							update_score();
						}
					} else return;
                }
			}
		}
    } else {
		if (x > zx) {
			for (i = zx + 1; i <= x; i++) {
				if ((grid[y][i]) && (grid[y][i] != piece)) {
					if (can_move(piece = grid[y][i], direction)) {
                        move_one_piece(piece, direction);
						if (flag) {
							flag = 0;
							update_score();
                        }
					} else return;
				}
			}
        } else {
			for (i = zx - 1; i >= x; i--) {
				if ((grid[y][i]) && (grid[y][i] != piece)) {
					if (can_move(piece = grid[y][i], direction)) {
						move_one_piece(piece, direction);
						if (flag) {
							flag = 0;
							update_score();
						}
					} else return;
				}
			}
		}
    }
}

void puzzle_click(int x, int y) { // left mouse clicked at screen coordinate (x, y)
    uint8_t piece;
    x -= top_left_x;
    if (x >= 0) {
        x /= square_width;
        if (x < squares_across) {
            y -= top_left_y;
            if (y >= 0) {
                y /= square_height;
                if (y < squares_down) {
                    piece = grid[y][x];
                    if (piece && piece != 255) {
                        if (!move_piece(piece) && slide == 2) {
                            slide_pieces(piece);
                        }
                        check_if_complete();
                    }
                }
            }
        }
    }
}
