#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <string.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <netinet/in.h>

#include "common.h"
#include "base64/base64.h"
#include "socket/web.h"
#include "threadpool/worker.h"

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define GUID_LEN strlen(GUID)
#define WS_KEY_LEN 24
#define WS_HEADER "HTTP/1.1 101 Switching Protocols\r\n"\
                "Upgrade: websocket\r\n"\
                "Connection: Upgrade\r\n"\
                "Sec-WebSocket-Accept: "
#define WS_HEADER_LEN strlen(WS_HEADER)

struct thread_data_t {
    int connection;
    bool *running;
};

struct ws_frame_head_t {
    bool fin;
    int8_t opcode;
    size_t payload_len;
    uint8_t mask_key[4];
};

void check_quit(void *arg)
{
    void **args = (void **)arg;
    bool *running = (bool *)*(args);
    int *ws_socket = (int *)*(args + 1);

    while (getc(stdin) != 'q');

    close(*ws_socket);
    LOG("Quitting...\n");
    
    *running = false;
}

int ws_recv_frame_head(int fd, struct ws_frame_head_t *head)
{
    int8_t first_byte;

    if (recv(fd, &first_byte, 1, 0) < 1) {
        return -1;
    }

    head->fin = (first_byte & 0x80) == 0x80;
    head->opcode = first_byte & 0x0F;

    if (recv(fd, &first_byte, 1, 0) < 1) {
        return -1;
    }

    head->payload_len = first_byte & 0x7F;

    if (recv(fd, &head->mask_key, 4, 0) < 4) {
        return -1;
    }

    return 0;
}

int ws_send_frame_head(int fd, struct ws_frame_head_t *head)
{
    char response_head[2] = {0};
    int head_len = 0;
    if (head->payload_len <= 125) {
        *response_head = 0x81;
        *(response_head + 1) = head->payload_len;
        head_len = 2;
    } else {
        return -1;
    }

    if (send(fd, response_head, head_len, 0) < 0) {
        return -1;
    }

    return 0;
}

int readline(char* buffer, int level, char* linebuf)
{
    int len = strlen(buffer);
    for (;level<len;++level) {

        if(*(buffer + level) == '\r' && *(buffer + level + 1) == '\n')
            return level + 2;
        else
            *(linebuf++) = *(buffer + level);
    }

    return -1;
}

int handshake(int cli_fd)
{
    int level = 0;
    char buffer[4096];
    char linebuf[256];
    char accept_key[32];

    unsigned char sha1_digest[SHA_DIGEST_LENGTH+1] = {0};
    char response_header[1024] = {0};

    if (recv(cli_fd,buffer,sizeof(buffer), 0) <= 0)
        return -1;

    LOG("Request: %s", buffer);

    do {
        memset(linebuf,0,sizeof(linebuf));
        level = readline(buffer,level,linebuf);
        

        if (strstr(linebuf,"Sec-WebSocket-Key") != NULL) {

            strcat(linebuf,GUID);
            
            SHA1((unsigned char*)&linebuf + 19, strlen(linebuf + 19), (unsigned char*)&sha1_digest);
            
            base64_encode(sha1_digest, strlen(sha1_digest), accept_key);

            sprintf(response_header, "%s %s\r\n\r\n", WS_HEADER, accept_key);

            LOG("Response: ");
            LOG("%s",response_header);

            if (send(cli_fd,response_header,strlen(response_header), 0) < 0)
                return -1;
            
            break;
        }
    } while((*(buffer + level) != '\r' || *(buffer + level + 1) != '\n') && level != -1);
    
    return 0;
}

void ws_listen(void *arg)
{
    struct thread_data_t *data = (struct thread_data_t *)arg;

    int client_socket = data->connection;

    bool handshaked = false;

    while (*(data->running)) {
        char buffer[4096] = {0};

        if (recv(client_socket, buffer, 1, MSG_PEEK) < 0)
            continue;

        if (handshaked == false) {
            if (handshake(client_socket) < 0) {
                LOG_ERR("Failed to handshake");
                continue;
            }
            handshaked = true;
        }
        
        struct ws_frame_head_t frame;
        if (ws_recv_frame_head(client_socket, &frame) < 0) {
            LOG_ERR("Failed to receive head");
            continue;
        }
        
        if (ws_send_frame_head(client_socket, &frame) < 0) {
            LOG_ERR("Failed to send head");
            continue;
        }

        int size = 0;
        do {
            int rec;
            if ((rec = recv(client_socket, buffer, 4096, 0)) < 0)
                break;
            
            size += rec;

            for (int i = 0; i < size; ++i)
                *(buffer + i) ^= *(frame.mask_key + (i%4));

            LOG("Received: %s", buffer);

            if (send(client_socket, buffer, rec, 0) < 0)
                break;
            
        } while(size < frame.payload_len);
    }

    close(client_socket);

    free(arg);

    LOG("Connection closed: %d", client_socket);
}

void set_non_blocking(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);

    flags |= O_NONBLOCK;

    fcntl(socket, F_SETFL, flags);
}

int main()
{
    struct server_t ws_server;
    init_server(&ws_server, WS_PORT);

    int client_ws_socket = -1;

    struct worker_threads_t *workers = malloc_worker_threads(20, 10);

    start_worker_threads(workers);

    bool running = true;

    LOG("Server started, press 'q' to quit\n");

    submit_worker_task(workers, check_quit, (void *[]){&running, &ws_server.socket});

    set_non_blocking(ws_server.socket);

    while (running) {
        client_ws_socket = accept(ws_server.socket, NULL, NULL);

        if (client_ws_socket != -1) {
            LOG("WebSocket client connected %d \n", client_ws_socket);

            struct thread_data_t *data = (struct thread_data_t *)malloc(sizeof(struct thread_data_t));
            data->connection = client_ws_socket;
            data->running = &running;
            submit_worker_task(workers, ws_listen, data);
            client_ws_socket = -1;
        }
    }

    LOG("Shutting down websocket server.");

    worker_threads_stop(workers);
    free_worker_threads(workers);
    
    return 0;
}