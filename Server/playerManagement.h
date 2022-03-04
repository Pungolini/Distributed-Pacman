#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include "board.h"


#define MAXPLAYERS 100
#define UNAVAILABLE -1

#define SET_SCORE 0
#define SET_EAT 1
#define SET_SPEED_P 2
#define SET_SPEED_M 3
#define GET_CLR 4
#define GET_FD 5
#define GET_ID 6
#define TIME_M 7
#define TIME_P 8




typedef playerStruct* array;



/********API do vetor ********/
array init_array(int n_players);
array insert_player(array arr ,int player_id,int player_fd, int player_color);
playerStruct *get_player_ptr(array arr, int id);
clock_t get_char_time(int id, char ch);

int connected();
void reset_all();
int get_field(char field,int id);
int get_vect_color(array arr,int id);
void generate_coords(int id,board_cell *_monster, board_cell *_pacman);
void set_field(char field,int id,int op);
void free_array(array arr);