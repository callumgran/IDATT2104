#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <string.h>
#include <fcntl.h>
#include <openssl/sha.h>

#include "common.h"
#include "base64/base64.h"
#include "fileio/fileio.h"
#include "socket/web.h"
#include "routing/routes.h"
#include "threadpool/worker.h"

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define GUID_LEN strlen(GUID)
#define WS_KEY_LEN 24
#define WS_HEADER "HTTP/1.1 101 Switching Protocols\r\n"\
                "Upgrade: websocket\r\n"\
                "Connection: Upgrade\r\n"\
                "Sec-WebSocket-Accept: "
#define WS_HEADER_LEN strlen(WS_HEADER)

#define PORT 8090

struct thread_data_t {
    int connection;
    struct routes_t *routes;
    bool *running;
};

void check_quit(void *arg)
{
    void **args = (void **)arg;
    bool *running = (bool *)*(args);
    int *socket = (int *)*(args + 1);

    while (getc(stdin) != 'q');

    close(*socket);
    LOG("Quitting...\n");
    
    *running = false;
}

void ws_connect(char *header, int socket)
{
    unsigned char key[WS_KEY_LEN];

    unsigned char *key_start = strstr(header, "Sec-WebSocket-Key: ") + strlen("Sec-WebSocket-Key: ");
    
    strncpy(key, key_start, WS_KEY_LEN);
    key[WS_KEY_LEN] = 0;

    LOG("found Sec-WebSocket-Key: %s", key);

    int fullkey_len = WS_KEY_LEN + GUID_LEN;

    unsigned char digest[SHA_DIGEST_LENGTH];

    unsigned char key_concat[fullkey_len + 1];

    strcpy(key_concat, key);

    strcat(key_concat, GUID);
    SHA1((unsigned char *)key_concat, 60, digest);
    LOG("computed SHA1: %s", digest);

    size_t out = 0;
    unsigned char *base64_digest = base64_encode2(digest, 20);

    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        printf("%02x", digest[i]);
    }
    base64_digest[strlen(base64_digest) - 1] = 0;


    LOG("computed Sec-Websocket-Accept: %s", base64_digest);

    int len = WS_HEADER_LEN + strlen(base64_digest);
    unsigned char outgoing_header[len];
    strcpy(outgoing_header, WS_HEADER);
    strcat(outgoing_header, base64_digest);
    strcat(outgoing_header, "=\r\n\r\n");
    int bytes_sent = send(socket, outgoing_header, 129, MSG_NOSIGNAL);

    LOG("SENT: %s", outgoing_header);

}

void http_listen(void *arg)
{
    struct thread_data_t *data = (struct thread_data_t *)arg;

    int client_socket = data->connection;

    struct routes_t *routes = data->routes;


    int bytes_recv = 1;

    while (bytes_recv > 0 && *(data->running)) {
        char buffer[4096];

        char ws_header[4096];

        bytes_recv = recv(client_socket, buffer, 4096, 0);
        LOG("Receiving %d bytes", bytes_recv);
        LOG("Received: %s", buffer);

        strncpy(ws_header, buffer, 1024);

        char *route = "";
        char *method = "";

        char *header = strtok(buffer, "\n");

        char *token = strtok(header, " ");

        int i = 0;

        while (token != NULL) {
            if (i == 0) {
                method = token;
            } else if (i == 1) {
                route = token;
            }

            token = strtok(NULL, " ");
            i++;
        }

        char template[100] = "";

        if (strcmp(route, "/favicon.ico") == 0)
            break;
        
        struct route_t * destination = find_route(routes, route);

        strcat(template, "./html/");

        if (destination == NULL)
            strcat(template, "404.html");
        else
            strcat(template, destination->value);

        FILE* file = fopen(template, "r");
        struct file_data_t *fd = read_file(file);
        fclose(file);

        if (strstr(ws_header, "Sec-WebSocket-Key") != NULL) {
            LOG("WS UPGRADE START");
            ws_connect(ws_header, client_socket);
            LOG("WS UPGRADE FINISHED");
        }

        char http_header[4096] = "HTTP/1.1 200 OK\r\n\r\n";
        strcat(http_header, fd->data);
        strcat(http_header, "\r\n\r\n");
        send(client_socket, http_header, sizeof(http_header), 0);
        free(fd->data);
        free(fd);
    }

    close(client_socket);

    free(arg);

    LOG("remote closed connection: %d", client_socket);
}

int main()
{
    struct http_server_t server;
    init_http_server(&server, PORT);

    int client_socket;

    struct routes_t routes;
    init_routes(&routes);

    insert_route(&routes, create_route("/", "index.html"));
    insert_route(&routes, create_route("/home", "index.html"));

    struct worker_threads_t *workers = malloc_worker_threads(6, 10);

    start_worker_threads(workers);

    bool running = true;

    printf("Server started, press 'q' to quit\n");

    submit_worker_task(workers, check_quit, (void *[]){&running, &server.socket});

    int flags = fcntl(server.socket, F_GETFL, 0);

    flags |= O_NONBLOCK;

    fcntl(server.socket, F_SETFL, flags);

    while (running) {

        client_socket = accept(server.socket, NULL, NULL);

        if (client_socket != -1) {
            LOG("Client connected %d \n", client_socket);

            struct thread_data_t *data = (struct thread_data_t *)malloc(sizeof(struct thread_data_t));
            data->connection = client_socket;
            data->routes = &routes;
            data->running = &running;
            submit_worker_task(workers, http_listen, data);
        }
    }

    LOG("Shutting down server");

    free(routes.base);
    worker_threads_stop(workers);
    free_worker_threads(workers);
    
    return 0;
}