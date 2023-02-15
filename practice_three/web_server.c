#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#ifdef PORT
#  undef PORT
#endif
#define PORT 8083


void *run(void *p)
{
    int connfd = *(int *)p;
    char out[4096];
    char response[4096 * 2] = "HTTP/1.0 200 OK\n"
        "Content-Type: text/html; charset=utf-8\n\n"
        "<h1>Wagwan</h1>"
        "Header fra klient er:"
        "<ul>";

    read(connfd, &out, 4096);

    char line[4096];
    int i = 0;
    int j = 0;
    while (j < strlen(out)) {
        if (i == 0) {
            strcat(line, "<li>");
            i = 4;
        }

        line[i++] = *(out + j++);

        if (*(out + j) == '\n') {
            strcat(line, "</li>"); 
            strcat(response, line);
            j++;
            i = 0;
            memset(line, 0, 4096);
        }
    }
    strcat(response, "</ul>");
    write(connfd, response, strlen(response) + 1);
    close(connfd);
    return NULL;
}

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "...socket creation failed...\n");
        exit(1);
    }
    printf("Socket successfully created..\n");

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setting socket options failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr = {0};

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
        fprintf(stderr, "...socket bind failed...\n");
        exit(1);
    }
    printf("Socket successfully binded..\n");

    if ((listen(sockfd, 5)) != 0) {
        fprintf(stderr, "...listen failed...\n");
        exit(1);
    }
    printf("Server listening..\n");

    while (1) {
        struct sockaddr_in client = {0};
        unsigned int len = sizeof(client);

        int connfd = accept(sockfd, (struct sockaddr *)&client, &len);
        if (connfd < 0) {
            fprintf(stderr, "server accept failed...\n");
            exit(1);
        }
        printf("server accepted the client...\n");
        pthread_t thread;
        pthread_create(&thread, NULL, run, &connfd);
    }

    close(sockfd);
}