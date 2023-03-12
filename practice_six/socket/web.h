#ifndef WEB_H
#define WEB_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/* Structs */

struct server_t {
    int port;
    int socket;
};

/* Methods */

void init_server(struct server_t *server, int port);

#endif