#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_WORKERS 4
#define MAX_QUEUE 128
#define BUFFER_SIZE 4096

typedef struct
{
    int items[MAX_QUEUE];
    int front;
    int rear;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} socket_queue_t;

static socket_queue_t client_queue = {
    .front = 0,
    .rear = 0,
    .size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
};

static void send_all(int fd, const char *buf, size_t len)
{
    while (len > 0)
    {
        ssize_t sent = send(fd, buf, len, 0);
        if (sent <= 0)
            return;
        buf += sent;
        len -= (size_t)sent;
    }
}

static void queue_push(socket_queue_t *queue, int fd)
{
    pthread_mutex_lock(&queue->mutex);
    if (queue->size < MAX_QUEUE)
    {
        queue->items[queue->rear] = fd;
        queue->rear = (queue->rear + 1) % MAX_QUEUE;
        queue->size++;
        pthread_cond_signal(&queue->cond);
    }
    else
    {
        close(fd);
    }
    pthread_mutex_unlock(&queue->mutex);
}

static int queue_pop(socket_queue_t *queue)
{
    int fd;

    pthread_mutex_lock(&queue->mutex);
    while (queue->size == 0)
        pthread_cond_wait(&queue->cond, &queue->mutex);

    fd = queue->items[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE;
    queue->size--;
    pthread_mutex_unlock(&queue->mutex);
    return fd;
}

static void handle_http_client(int client_fd)
{
    char request[BUFFER_SIZE];
    const char *body =
        "<html><body><h1>Xin chao cac ban</h1>"
        "<p>HTTP prethread server dang hoat dong.</p></body></html>";
    char response[BUFFER_SIZE];
    int rc = recv(client_fd, request, sizeof(request) - 1, 0);
    int body_len = (int)strlen(body);
    int n;

    if (rc <= 0)
        return;

    request[rc] = '\0';
    printf("HTTP request:\n%s\n", request);

    n = snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html; charset=utf-8\r\n"
                 "Content-Length: %d\r\n"
                 "Connection: close\r\n\r\n"
                 "%s",
                 body_len, body);

    if (n > 0)
        send_all(client_fd, response, (size_t)n);
}

static void *worker_thread(void *arg)
{
    (void)arg;
    while (1)
    {
        int client_fd = queue_pop(&client_queue);
        handle_http_client(client_fd);
        close(client_fd);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int port;
    int worker_count = DEFAULT_WORKERS;
    int listen_fd;
    int opt = 1;
    struct sockaddr_in addr;

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s <port> [worker_count]\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    if (argc == 3)
        worker_count = atoi(argv[2]);
    if (worker_count <= 0 || worker_count > 64)
    {
        fprintf(stderr, "worker_count must be in range 1..64\n");
        return 1;
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd == -1)
    {
        perror("socket");
        return 1;
    }

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 32) == -1)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    for (int i = 0; i < worker_count; i++)
    {
        pthread_t tid;
        if (pthread_create(&tid, NULL, worker_thread, NULL) == 0)
            pthread_detach(tid);
    }

    printf("HTTP prethread server listening on port %d with %d workers\n", port, worker_count);

    while (1)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }
        queue_push(&client_queue, client_fd);
    }
}
