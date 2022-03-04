#pragma once
//#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include "../common/utils.h"

//inicializa o player
typedef struct _playerStruct
{
  int player_fd;
  int score;
  int playerID;
  int color;
  unsigned int monster_eaten;
  int speed_pac;
  int speed_monster;
  clock_t monster_time;
  clock_t pacman_time;
  pthread_rwlock_t rwlock;
}playerStruct;

extern playerStruct *player_arr;
extern board_cell** game_board;
extern int cols , rows ;

extern unsigned short NUMPLAYERS;
extern unsigned short N_FRUITS;
extern unsigned short N_BRICKS;
extern unsigned short N_PLAYERS;
extern unsigned short LIVRES;
extern unsigned short MorP;


#define BOUNCE 0
#define EAT_FRUIT 1
#define EAT_CHAR 2
#define STEP 3
#define FREE_CELLS (rows*cols) - OCCUPIED_CELLS
#define OCCUPIED_CELLS (N_FRUITS + N_BRICKS + MorP)

#define is_last_row(x)  (x.coord.row == (rows-1))
#define is_last_col(x)  (x.coord.col == (cols-1))
#define is_pacman(x) ( x->element == PACMAN)
#define is_monster(x) (x->element == MONSTER)
#define is_fruit(ch)  ((ch == LEMON) || (ch == CHERRY))
#define SWAP(x, y) do { typeof(x) SWAP = x; x = y; y = SWAP; } while (0)

#define R_LOCK 'R'
#define W_LOCK 'W'
#define UNLOCK 'U'
#define LOCK 'L'
#define DESTROY 'D'
#define INIT 'I'

#define RST 0
#define INC 1
#define DEC 2
#define RET 3

extern sem_t fruit_sem;
extern pthread_mutex_t monster_mutex;
extern pthread_mutex_t pacman_mutex;
extern pthread_mutex_t global_mutex;
extern pthread_rwlock_t nplayers_lock;
extern pthread_rwlock_t globals_lock;
extern enum {LEFT = 1073741904 ,UP = 1073741906, RIGHT = 1073741903, DOWN = 1073741905}keys;


/* ABSTRACAO da player_struct*/
typedef board_cell** BOARD;

void set_player_color(playerStruct* p,int color);
void set_player_fd(playerStruct* p,int fd);
void set_player_id(playerStruct* p,int id);


int get_color(playerStruct *p);
int get_player_fd(playerStruct *p);
int get_player_id(playerStruct *p);
int get_player_score(playerStruct *p);
int get_player_color(playerStruct *p);
int get_monster_eaten(playerStruct *p);
/****************************************************/


/*****Abstracao /API do tabuleiro de jogo****/
void free_board();
BOARD init_board(char *file);
BOARD alloc_board(int n_rows,int n_cols);

void set_cell_id(BOARD *board,int id,int r,int c);
void set_cell_type(BOARD* board,char elem,int r,int c);
void set_cell_clr(BOARD* board,int color,int r,int c);

int get_cell_clr(BOARD board,int r,int c);
int get_cell_id(BOARD board,int r,int c);
char get_cell_type(BOARD board,int r,int c);

int cell_is_brick(BOARD board,int r,int c);
int cell_is_fruit(BOARD board,int r,int c);
int cell_is_character(BOARD board,int r,int c);
int cell_is_mine(BOARD board,int id,int r, int c );
int same_cells(board_cell *c1,board_cell *c2);
board_cell* get_cell_ptr(BOARD board,int r,int c);
void update_cell(BOARD board,int id,board_cell *pos);
/****************************************************/

/*-------- GAMEPLAY -------------*/
int play_is_valid(BOARD board,int playerID,board_cell *old_play,board_cell *new_play);
int cells_are_adjacents(point *cell1,point *cell2);
void swap_positions(board_cell *char1,board_cell *char2);
int get_char_pos(board_cell *old_play,int id,char elem);
void clear_cell(int r,int c);
int bounce(board_cell *old_play,board_cell *play,int dir);
void get_curr_pos(int id,board_cell *old_play,board_cell *new_play);


short get_empty_cells();
short _nplayers(char op);
void show_cell(board_cell *st);
void count_elements(BOARD board);
void printMat(BOARD M,int rows,int cols);
void set_cell(BOARD* game_board,char elem, int r,int c);
void monster_new_pos(board_cell *old_play,board_cell *pos);
void spawn_fruit(BOARD *board,int n_players,board_cell *fruit_pos);
void generate_pos(point *pos);
int chars_are_equal(char c1,char c2);

/******* SYNC *******/
void init_sync();
void mutex(char op, pthread_mutex_t* lock);
void rwLock(char op, pthread_rwlock_t* lock);