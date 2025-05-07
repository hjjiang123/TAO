#ifndef SERVER_H
#define SERVER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "command.h"
#include "subtask.h"
#include "thread.h"
#include "scheduler.h"

int init_server(void *arg);

#endif