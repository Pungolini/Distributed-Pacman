#pragma once

#include <pthread.h>
#include <unistd.h>
#include "../common/utils.h"


typedef enum bool{ false,true }bool;


static void terminate_client(int a);
void get_initial_info(int sock_fd, int color);