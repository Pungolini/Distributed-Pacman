#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include "../common/UI_library.h"

extern int CLIENT_PORT;
#define _XOPEN_SOURCE 700
#define _GNU_SOURCE
#define EMPTY 'o'
#define PACMAN 'P'
#define MONSTER 'M'
#define CHERRY 'C'
#define LEMON 'L'
#define BRICK 'B'
#define POWERED 'Z'
#define FINISHED 'F'
#define INVALID -1
#define RECEIVE_SCORE 'R'


// Estas estruturas tem de estar aqui para o cliente tambem ter acesso
typedef struct pt 
{
  unsigned short row,col;
}point;

typedef struct bc
{
  point coord;
  char element;
  int color;
  int id;
  pthread_rwlock_t rwlock;
 // pthread_mutex_t mutex;

}board_cell;


/********** ABSTRACAO DO BOARD_CELL ***************/
void set_id(board_cell *ptr,int id);
void set_clr(board_cell *ptr,int color);
void set_pos(board_cell *ptr,point *pos);
void set_char(board_cell *ptr,char elem);

void get_id(board_cell *ptr,int *id);
void get_clr(board_cell *ptr,int *color);
void get_pos(board_cell *ptr,point *pos);
void get_char_bc(board_cell *ptr,char *ch);
void get_coord(board_cell *ptr,int *r,int *c);


void init_cell(board_cell *cell, int clr,int id, int elem,int r,int c);


void checkMem(void *ptr);
void render_cell(board_cell *);
void serialize(board_cell *cell);
void unserialize(board_cell *cell);
void initSigHandlers(void (*handler)(int));
void track_error(const char *file,const char* func,int line);
void parseArgs(unsigned int *color,int *port, int argc, char const *argv[]);
void start_socket(int * sock_fd,const char *address,struct sockaddr_in *_addr, int port, short is_server);


