#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "../practice_two/worker.h"
#include "math_service.h"


static int int_input(int *number)
{
    long a;
    char buf[1024] = {0};

    if (!fgets(buf, 1024, stdin)) {
        printf("Error reading input");
        return 0;
    }

    char *end_ptr;

    errno = 0;
    a = strtol(buf, &end_ptr, 10);
    
    if (errno == ERANGE) {
        printf("Number out of range");
        return 0;
    }
    
    if (end_ptr == buf) {
        printf("No digits were found");
        return 0;
    }
    
    if (*end_ptr && *end_ptr != '\n') {
        printf("Further characters after number");
        return 0;
    }

    if (a > INT_MAX || a < INT_MIN) {
        printf("Number out of range");
        return 0;
    }

    *number = (int) a;
    return 1;
}

static void maths_input(struct arithmetic_t *arithmetic)
{
    printf("--------------------\n");
    printf("Enter expression: \n");
    printf("Enter first number: \n");
    if (int_input(&arithmetic->a) != 1)
        exit(EXIT_FAILURE);

    printf("Enter operation: \n");
    arithmetic->op = getc(stdin);

    printf("Enter second number: \n");
    while (getc(stdin) != '\n');
    if (int_input(&arithmetic->b) != 1)
        exit(EXIT_FAILURE);

    printf("--------------------\n");
}

int connect_to_server(struct sockaddr_in *serv_addr, int *sock, char *ip, int port)
{
    if ((*sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
     
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = inet_addr(ip);
    serv_addr->sin_port = htons(port);

    printf("Connected to server\n");
    return 0;
}

static void receive_data(void *arg)
{
    void **args = (void **)arg;
    int socket = *(int *)*args;
    struct sockaddr_in serv_addr = *(struct sockaddr_in *)*(args + 1);
    unsigned int len = sizeof(serv_addr);
    char msg[MSG_MAX] = {0};

    while (recvfrom(socket, &msg, MSG_MAX, 0, (struct sockaddr *)&serv_addr, &len) > 0) {
        if (msg[0] == 'q') {
            printf("Server closed connection");
            exit(0);
        } else {
            printf("%s", msg);
        }
    }
}

int main()
{
    int sock;
    int valread;
    struct sockaddr_in serv_addr = {0};
    unsigned int len = sizeof(serv_addr);

    if (connect_to_server(&serv_addr, &sock, IP, PORT) != 0)
        exit(EXIT_FAILURE);

    char msg[MSG_MAX] = {0};

    struct arithmetic_t arithmetic = {0};

    struct worker_threads_t* workers = malloc_worker_threads(2, 2);

    start_worker_threads(workers);

    void *args[] = {&sock, &serv_addr};

    submit_worker_task(workers, receive_data, &args);

    printf("Enter 'Ctrl + C' to quit\n");
    while (1) {
        usleep(100);
        maths_input(&arithmetic);
        sendto(sock, &arithmetic, sizeof(struct arithmetic_t), 0, (struct sockaddr *)&serv_addr, len);
        printf("%s", msg);
    }

    close(sock);
    return 0;
}