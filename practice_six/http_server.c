#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "common.h"
#include "fileio/fileio.h"
#include "socket/server.h"
#include "routing/routes.h"
#include "threadpool/worker.h"

struct thread_data_t {
    int connection;
    struct routes_t *routes;
    bool *running;
};

void check_quit(void *arg)
{
    void **args = (void **)arg;
    bool *running = (bool *)*(args);
    int *http_socket = (int *)*(args + 1);

    while (getc(stdin) != 'q');

    close(*http_socket);
    LOG("Quitting...\n");
    
    *running = false;
}

int serve_file(struct routes_t *routes, int socket, char *buffer)
{
    
    LOG("HTTP REQUEST");
    LOG("Received: %s", buffer);

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
        return -1;

    struct route_t * destination = find_route(routes, route);

    strcat(template, "./html/");

    if (destination == NULL)
        strcat(template, "404.html");
    else
        strcat(template, destination->value);

    FILE* file = fopen(template, "r");
    struct file_data_t *fd = read_file(file);
    fclose(file);

    char http_header[4096] = "HTTP/1.1 200 OK\r\n\r\n";
    strcat(http_header, fd->data);
    strcat(http_header, "\r\n\r\n");
    send(socket, http_header, sizeof(http_header), 0);
    free(fd->data);
    free(fd);
}

void http_listen(void *arg)
{
    struct thread_data_t *data = (struct thread_data_t *)arg;

    int client_socket = data->connection;
    struct routes_t *routes = data->routes;
    int bytes_recv = 1;

    while (bytes_recv > 0 && *(data->running)) {
        char buffer[4096] = {0};
        bytes_recv = recv(client_socket, buffer, 4096, 0);
        serve_file(routes, client_socket, buffer);
    }

    close(client_socket);

    free(arg);

    LOG("Closed connection to: %d", client_socket);
}

void set_non_blocking(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);

    flags |= O_NONBLOCK;

    fcntl(socket, F_SETFL, flags);
}

int main()
{
    struct server_t http_server;
    init_server(&http_server, HTTP_PORT);

    int client_http_socket = -1;

    struct routes_t routes;
    init_routes(&routes);

    insert_route(&routes, create_route("/", "index.html"));
    insert_route(&routes, create_route("/home", "index.html"));

    struct worker_threads_t *workers = malloc_worker_threads(10, 10);

    start_worker_threads(workers);

    bool running = true;

    LOG("Server started, press 'q' to quit\n");

    submit_worker_task(workers, check_quit, (void *[]){&running, &http_server.socket});

    set_non_blocking(http_server.socket);

    while (running) {

        client_http_socket = accept(http_server.socket, NULL, NULL);

        if (client_http_socket != -1) {
            LOG("HTTP client connected %d \n", client_http_socket);

            struct thread_data_t *data = (struct thread_data_t *)malloc(sizeof(struct thread_data_t));
            data->connection = client_http_socket;
            data->routes = &routes;
            data->running = &running;
            submit_worker_task(workers, http_listen, data);
            client_http_socket = -1;
        }
    }

    LOG("Shutting down http_server.");

    free(routes.base);
    worker_threads_stop(workers);
    free_worker_threads(workers);
    
    return 0;
}