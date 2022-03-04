

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#include "com.h"
#include "playerManagement.h"

#define SERVER_PORT 3002


static void handleSigInt();
void initSigHandlers();

