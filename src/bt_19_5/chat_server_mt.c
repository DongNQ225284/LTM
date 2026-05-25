#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_CLIENTS 128
#define BUFFER_SIZE 2048

typedef struct
{
    int fd;
    int active;
    char client_id[64];
    char client_name[128];
} client_t;

static client_t clients[MAX_CLIENTS];
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

static void trim_line(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

static int recv_line(int fd, char *buf, size_t size)
{
    size_t used = 0;
    while (used + 1 < size)
    {
        char ch;
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0)
            return (int)n;
        buf[used++] = ch;
        if (ch == '\n')
            break;
    }
    buf[used] = '\0';
    return (int)used;
}

static void send_text(int fd, const char *text)
{
    send(fd, text, strlen(text), 0);
}

static int allocate_client_slot(void)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!clients[i].active)
        {
            clients[i].active = 1;
            pthread_mutex_unlock(&clients_mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return -1;
}

static void remove_client_slot(int idx)
{
    pthread_mutex_lock(&clients_mutex);
    clients[idx].active = 0;
    clients[idx].fd = -1;
    clients[idx].client_id[0] = '\0';
    clients[idx].client_name[0] = '\0';
    pthread_mutex_unlock(&clients_mutex);
}

static int parse_registration(const char *line, char *client_id, size_t id_size, char *client_name, size_t name_size)
{
    const char *sep = strchr(line, ':');
    size_t id_len;
    const char *name;

    if (sep == NULL)
        return 0;

    id_len = (size_t)(sep - line);
    while (id_len > 0 && line[id_len - 1] == ' ')
        id_len--;

    name = sep + 1;
    while (*name == ' ')
        name++;

    if (id_len == 0 || *name == '\0')
        return 0;

    if (id_len >= id_size)
        id_len = id_size - 1;

    memcpy(client_id, line, id_len);
    client_id[id_len] = '\0';
    strncpy(client_name, name, name_size - 1);
    client_name[name_size - 1] = '\0';
    return 1;
}

static void current_timestamp(char *buf, size_t size)
{
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    strftime(buf, size, "%Y/%m/%d %I:%M:%S%p", &tm_now);
}

static void broadcast_message(int sender_idx, const char *payload)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!clients[i].active || i == sender_idx)
            continue;
        send(clients[i].fd, payload, strlen(payload), 0);
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void *client_thread(void *arg)
{
    int idx = *(int *)arg;
    int client_fd = idx >= 0 ? clients[idx].fd : -1;
    char line[BUFFER_SIZE];
    free(arg);

    while (1)
    {
        send_text(client_fd, "Register as client_id: client_name\n");
        if (recv_line(client_fd, line, sizeof(line)) <= 0)
            goto cleanup;
        trim_line(line);

        if (parse_registration(line, clients[idx].client_id, sizeof(clients[idx].client_id),
                               clients[idx].client_name, sizeof(clients[idx].client_name)))
            break;

        send_text(client_fd, "Invalid format. Try again.\n");
    }

    send_text(client_fd, "Registered. Start chatting.\n");

    while (1)
    {
        char timestamp[64];
        char message[BUFFER_SIZE + 256];

        if (recv_line(client_fd, line, sizeof(line)) <= 0)
            break;

        trim_line(line);
        if (line[0] == '\0')
            continue;

        current_timestamp(timestamp, sizeof(timestamp));
        snprintf(message, sizeof(message), "%s %s: %s\n", timestamp, clients[idx].client_id, line);
        broadcast_message(idx, message);
    }

cleanup:
    close(client_fd);
    remove_client_slot(idx);
    return NULL;
}

int main(int argc, char *argv[])
{
    int port;
    int listen_fd;
    int opt = 1;
    struct sockaddr_in addr;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    memset(clients, 0, sizeof(clients));
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

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

    printf("Chat multithread server listening on port %d\n", port);

    while (1)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        pthread_t tid;
        int slot;
        int *arg;

        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        slot = allocate_client_slot();
        if (slot < 0)
        {
            send_text(client_fd, "Server busy.\n");
            close(client_fd);
            continue;
        }

        clients[slot].fd = client_fd;
        clients[slot].client_id[0] = '\0';
        clients[slot].client_name[0] = '\0';

        arg = (int *)malloc(sizeof(int));
        if (arg == NULL)
        {
            close(client_fd);
            remove_client_slot(slot);
            continue;
        }
        *arg = slot;

        if (pthread_create(&tid, NULL, client_thread, arg) == 0)
            pthread_detach(tid);
        else
        {
            close(client_fd);
            remove_client_slot(slot);
            free(arg);
        }
    }
}
