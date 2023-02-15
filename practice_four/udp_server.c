#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "math_service.h"
#include "../practice_two/worker.h"

void arithmetic(void *arg)
{
    int ans = 0;
    struct sockaddr_in client = {0};
    unsigned int len = sizeof(client);
    struct arithmetic_t arithmetic;
    char msg[MSG_MAX];
    int socket = *(int *)arg;

    while(recvfrom(socket, &arithmetic, sizeof(struct arithmetic_t), 0, (struct sockaddr*)&client, &len) > 0) {
        switch (arithmetic.op) {
            case '+':
                ans = arithmetic.a + arithmetic.b;
                break;
            case '-':
                ans = arithmetic.a - arithmetic.b;
                break;
            case '*':
                ans = arithmetic.a * arithmetic.b;
                break;
            case '/':
                if (arithmetic.b == 0) {
                    strncpy(msg, "Cannot divide by zero\n", MSG_MAX);
                    send(socket, msg, MSG_MAX, 0);
                    continue;
                }
                ans = arithmetic.a / arithmetic.b;
                break;
            case '%':
                ans = arithmetic.a % arithmetic.b;
                break;
            default:
                strncpy(msg, "Invalid operator: %c\n", arithmetic.op);
        }

        snprintf(msg, MSG_MAX, "Answer is: %d\n", ans);
        sendto(socket, msg, MSG_MAX, 0, (struct sockaddr*)&client, len);
    }

    close(socket);
}

void create_connections(void *arg)
{
    int i = 0;
    int server_fd = *(int *)(arg);

    struct worker_threads_t *workers = malloc_worker_threads(16, 32);
    start_worker_threads(workers);

    for (int i = 0; i < 16; i++)
        submit_worker_task(workers, arithmetic, &server_fd);

    worker_threads_stop(workers);
    free_worker_threads(workers);
}

void check_quit(void *arg)
{
    void **args = (void **)arg;
    bool *running = (bool *)*(args);
    int *connections = (int *)*(args + 1);

    while (getc(stdin) != 'q');

    printf("Quitting...\n");
    *running = false;
    exit(0);
}

int main()
{
    int server_fd;
    int valread;
    int opt = 1;
    int connections[16] = {4};

    size_t addrlen = sizeof(struct sockaddr_in);

    printf("Starting server...\n");

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Creating socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setting socket options failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address = {0};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0) {
        perror("Binding socket failed");
        exit(EXIT_FAILURE);
    }
    
    struct worker_threads_t* workers = malloc_worker_threads(2, 2);
    
    start_worker_threads(workers);
    printf("Server started. Press 'q' to quit.\n");

    submit_worker_task(workers, create_connections, &server_fd);

    bool running = true;
    void *args2[] = {&running, connections};

    submit_worker_task(workers, check_quit, args2);
    
    while (running);

    worker_threads_stop(workers);
    free_worker_threads(workers);
    close(server_fd);
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}