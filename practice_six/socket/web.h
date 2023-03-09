#ifndef WEB_H
#define WEB_H

/* Structs */

struct http_server_t {
    int port;
    int socket;
};

/* Methods */

void init_http_server(struct http_server_t *server, int port);

#endif