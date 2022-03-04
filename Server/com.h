#pragma once

#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <time.h>
#include "playerManagement.h"

#define _GNU_SOURCE
#define JOIN_THREAD -1
#define THREAD_EXIT 41345
#define _XOPEN_SOURCE 700
#define WAIT_2_SECONDS 2000
#define WAIT_30_SECONDS 30000
#define WAIT_60_SECONDS 60 //sec

extern volatile int done_signal;

#define SAME_PLAY 0
#define WAIT2 1

#define time_ms(x)  (x*1000)/CLOCKS_PER_SEC

typedef struct _timer_t
{
    int destroy;
    sem_t sem;
    board_cell cell;
    playerStruct *player_info;

}timer_;

/*********** SERVER **********/
int send_data(int id,int sock_fd, void* _buff, size_t _n);
int send_board(BOARD board,array arr,int id,int sock_fd);
int process_play(int id,board_cell * old_play,board_cell *play);

void send_scoreboard();
void send_board_to_all();

void *send_score_thread(void *n);
void send_initial_data(int id, int sock_fd);
void disconnect_client(int id,int sock_fd);
void *poll_monster_inactivity(void * poll_time);
void *poll_pacman_inactivity(void * poll_time);
void *fruit_spawn_thread(void *);
void remove_characters(int player_id);
void eat_character(board_cell *old_play,board_cell *play,int flag);
void update_pos(board_cell *old_play,board_cell *play);
void send_play_to_all(array arr,int id,board_cell *play);
void send_cell_to_all(BOARD board,array arr, board_cell *cell);

