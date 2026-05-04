#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 9000
#define MAX_CLIENTS FD_SETSIZE
#define MAX_LINE 1024
#define MAX_TOPIC_LEN 63
#define MAX_MSG_LEN 900

typedef struct {
    int fd;
    char addr[64];
    char topics[32][MAX_TOPIC_LEN + 1];
    int topic_count;
    char inbuf[MAX_LINE];
    int inbuf_len;
} client_t;

static void reset_client(client_t *client) {
    if (client->fd != -1) {
        close(client->fd);
    }
    client->fd = -1;
    client->addr[0] = '\0';
    client->topic_count = 0;
    client->inbuf_len = 0;
    client->inbuf[0] = '\0';
}

static void trim_line(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

static int topic_index(const client_t *client, const char *topic) {
    for (int i = 0; i < client->topic_count; i++) {
        if (strcmp(client->topics[i], topic) == 0) {
            return i;
        }
    }
    return -1;
}

static int subscribe_topic(client_t *client, const char *topic) {
    if (topic_index(client, topic) >= 0) {
        return 0;
    }
    if (client->topic_count >= 32) {
        return -1;
    }
    strncpy(client->topics[client->topic_count], topic, MAX_TOPIC_LEN);
    client->topics[client->topic_count][MAX_TOPIC_LEN] = '\0';
    client->topic_count++;
    return 1;
}

static int unsubscribe_topic(client_t *client, const char *topic) {
    int idx = topic_index(client, topic);
    if (idx < 0) {
        return 0;
    }
    for (int i = idx; i < client->topic_count - 1; i++) {
        strcpy(client->topics[i], client->topics[i + 1]);
    }
    client->topic_count--;
    return 1;
}

static int send_text(int fd, const char *text) {
    size_t left = strlen(text);
    const char *p = text;
    while (left > 0) {
        ssize_t sent = send(fd, p, left, 0);
        if (sent <= 0) {
            return -1;
        }
        p += sent;
        left -= (size_t)sent;
    }
    return 0;
}

static void send_reply(client_t *client, const char *text) {
    if (send_text(client->fd, text) == -1) {
        fprintf(stderr, "send failed for %s\n", client->addr);
        reset_client(client);
    }
}

static void broadcast_topic(client_t clients[], int sender_fd, const char *topic, const char *msg) {
    char out[MAX_LINE + 128];
    snprintf(out, sizeof(out), "[%s] %s\n", topic, msg);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1 || clients[i].fd == sender_fd) {
            continue;
        }
        if (topic_index(&clients[i], topic) >= 0) {
            if (send_text(clients[i].fd, out) == -1) {
                fprintf(stderr, "broadcast failed for %s\n", clients[i].addr);
                reset_client(&clients[i]);
            }
        }
    }
}

static void handle_command(client_t clients[], client_t *client, char *line) {
    char topic[MAX_TOPIC_LEN + 1];
    char msg[MAX_MSG_LEN + 1];

    if (sscanf(line, "SUB %63s", topic) == 1) {
        int rc = subscribe_topic(client, topic);
        if (rc == 1) {
            send_reply(client, "OK SUB\n");
        } else if (rc == 0) {
            send_reply(client, "ERR already subscribed\n");
        } else {
            send_reply(client, "ERR too many topics\n");
        }
        return;
    }

    if (sscanf(line, "UNSUB %63s", topic) == 1) {
        if (unsubscribe_topic(client, topic)) {
            send_reply(client, "OK UNSUB\n");
        } else {
            send_reply(client, "ERR topic not found\n");
        }
        return;
    }

    if (sscanf(line, "PUB %63s %900[^\n]", topic, msg) == 2) {
        broadcast_topic(clients, client->fd, topic, msg);
        send_reply(client, "OK PUB\n");
        return;
    }

    send_reply(client, "ERR invalid command\n");
}

static int extract_line(client_t *client, char *line) {
    for (int i = 0; i < client->inbuf_len; i++) {
        if (client->inbuf[i] == '\n') {
            int len = i + 1;
            memcpy(line, client->inbuf, (size_t)len);
            line[len] = '\0';
            memmove(client->inbuf, client->inbuf + i + 1, (size_t)(client->inbuf_len - len));
            client->inbuf_len -= len;
            client->inbuf[client->inbuf_len] = '\0';
            trim_line(line);
            return 1;
        }
    }
    return 0;
}

int main(void) {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(listenfd);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(listenfd);
        return 1;
    }

    if (listen(listenfd, 16) == -1) {
        perror("listen");
        close(listenfd);
        return 1;
    }

    client_t clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].addr[0] = '\0';
        clients[i].topic_count = 0;
        clients[i].inbuf_len = 0;
        clients[i].inbuf[0] = '\0';
    }

    printf("Pub/Sub server listening on port %d...\n", SERVER_PORT);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);

        int maxfd = listenfd;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > maxfd) {
                    maxfd = clients[i].fd;
                }
            }
        }

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select");
            continue;
        }

        if (FD_ISSET(listenfd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len);
            if (connfd == -1) {
                perror("accept");
            } else {
                int slot = -1;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == -1) {
                        slot = i;
                        break;
                    }
                }

                if (slot == -1) {
                    send_text(connfd, "ERR server full\n");
                    close(connfd);
                } else {
                    clients[slot].fd = connfd;
                    clients[slot].topic_count = 0;
                    clients[slot].inbuf_len = 0;
                    clients[slot].inbuf[0] = '\0';
                    snprintf(clients[slot].addr, sizeof(clients[slot].addr), "%s:%d",
                             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    printf("Connected: %s\n", clients[slot].addr);
                    send_reply(&clients[slot],
                               "Connected to pub/sub server\n"
                               "Commands: SUB <topic> | UNSUB <topic> | PUB <topic> <msg>\n");
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd == -1 || !FD_ISSET(clients[i].fd, &readfds)) {
                continue;
            }

            ssize_t n = recv(clients[i].fd,
                             clients[i].inbuf + clients[i].inbuf_len,
                             sizeof(clients[i].inbuf) - 1 - (size_t)clients[i].inbuf_len,
                             0);
            if (n <= 0) {
                printf("Disconnected: %s\n", clients[i].addr);
                reset_client(&clients[i]);
                continue;
            }

            clients[i].inbuf_len += (int)n;
            clients[i].inbuf[clients[i].inbuf_len] = '\0';

            char line[MAX_LINE + 1];
            while (extract_line(&clients[i], line)) {
                if (line[0] == '\0') {
                    continue;
                }
                handle_command(clients, &clients[i], line);
                if (clients[i].fd == -1) {
                    break;
                }
            }

            if (clients[i].inbuf_len == (int)sizeof(clients[i].inbuf) - 1) {
                send_reply(&clients[i], "ERR command too long\n");
                clients[i].inbuf_len = 0;
                clients[i].inbuf[0] = '\0';
            }
        }
    }

    close(listenfd);
    return 0;
}
