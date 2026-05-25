#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_QUEUE 128
#define BUFFER_SIZE 2048

typedef struct
{
    int items[MAX_QUEUE];
    int front;
    int rear;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} client_queue_t;

typedef struct
{
    int a_fd;
    int b_fd;
} chat_pair_t;

static client_queue_t wait_queue = {
    .front = 0,
    .rear = 0,
    .size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
};

static void send_text(int fd, const char *text)
{
    send(fd, text, strlen(text), 0);
}

static void queue_push(client_queue_t *queue, int fd)
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
        send_text(fd, "Server queue is full.\n");
        close(fd);
    }
    pthread_mutex_unlock(&queue->mutex);
}

static int queue_pop_pair(client_queue_t *queue, int *a_fd, int *b_fd)
{
    pthread_mutex_lock(&queue->mutex);
    while (queue->size < 2)
        pthread_cond_wait(&queue->cond, &queue->mutex);

    *a_fd = queue->items[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE;
    *b_fd = queue->items[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE;
    queue->size -= 2;
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

static void close_both(int a_fd, int b_fd)
{
    if (a_fd >= 0)
        close(a_fd);
    if (b_fd >= 0)
        close(b_fd);
}

static void relay_loop(int a_fd, int b_fd)
{
    char buf[BUFFER_SIZE];

    send_text(a_fd, "Paired. You can chat now.\n");
    send_text(b_fd, "Paired. You can chat now.\n");

    while (1)
    {
        fd_set readfds;
        int maxfd = (a_fd > b_fd) ? a_fd : b_fd;
        int rc;

        FD_ZERO(&readfds);
        FD_SET(a_fd, &readfds);
        FD_SET(b_fd, &readfds);

        rc = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (rc <= 0)
        {
            if (rc < 0 && errno == EINTR)
                continue;
            break;
        }

        if (FD_ISSET(a_fd, &readfds))
        {
            ssize_t n = recv(a_fd, buf, sizeof(buf), 0);
            if (n <= 0)
            {
                send_text(b_fd, "Partner disconnected. Session closed.\n");
                break;
            }
            send(b_fd, buf, (size_t)n, 0);
        }

        if (FD_ISSET(b_fd, &readfds))
        {
            ssize_t n = recv(b_fd, buf, sizeof(buf), 0);
            if (n <= 0)
            {
                send_text(a_fd, "Partner disconnected. Session closed.\n");
                break;
            }
            send(a_fd, buf, (size_t)n, 0);
        }
    }
}

static void *chat_session_thread(void *arg)
{
    chat_pair_t *pair = (chat_pair_t *)arg;
    relay_loop(pair->a_fd, pair->b_fd);
    close_both(pair->a_fd, pair->b_fd);
    free(pair);
    return NULL;
}

static void *pairing_thread(void *arg)
{
    (void)arg;

    while (1)
    {
        int a_fd;
        int b_fd;
        pthread_t tid;
        chat_pair_t *pair;

        queue_pop_pair(&wait_queue, &a_fd, &b_fd);

        pair = (chat_pair_t *)malloc(sizeof(chat_pair_t));
        if (pair == NULL)
        {
            close_both(a_fd, b_fd);
            continue;
        }

        pair->a_fd = a_fd;
        pair->b_fd = b_fd;

        if (pthread_create(&tid, NULL, chat_session_thread, pair) == 0)
            pthread_detach(tid);
        else
        {
            close_both(a_fd, b_fd);
            free(pair);
        }
    }
}

int main(int argc, char *argv[])
{
    int port;
    int listen_fd;
    int opt = 1;
    struct sockaddr_in addr;
    pthread_t pairing_tid;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);

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

    if (pthread_create(&pairing_tid, NULL, pairing_thread, NULL) != 0)
    {
        perror("pthread_create");
        close(listen_fd);
        return 1;
    }
    pthread_detach(pairing_tid);

    printf("Pair chat server listening on port %d\n", port);

    while (1)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        send_text(client_fd, "Waiting for a partner...\n");
        queue_push(&wait_queue, client_fd);
    }

    close(listen_fd);
    return 0;
}
