#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define EMPTY 'o'
#define PACMAN 'P'
#define MONSTER 'M'
#define CHERRY 'C'
#define LEMON 'L'
#define BRICK 'B'
#define POWERED 'Z'
#define CELL_SIZE 40


#define IS_SERVER 9999


// Colors definition
extern SDL_Color PALETTE[];

int create_board_window(int dim_x, int dim_y,int );

void get_color_string(char title[],int clr);
void get_board_place(int mouse_x, int mouse_y, unsigned short * board_x, unsigned short *board_y);
void paint_pacman(int  board_x, int board_y , int r, int g, int b);
void paint_powerpacman(int  board_x, int board_y , int r, int g, int b);
void paint_monster(int  board_x, int board_y , int r, int g, int b);
void paint_lemon(int  board_x, int board_y );
void paint_cherry(int  board_x, int board_y);
void paint_brick(int  board_x, int board_y );
void clear_place(int  board_x, int board_y);
void close_board_windows();
