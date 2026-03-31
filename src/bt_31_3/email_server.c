/*
Server TCP non-blocking:
- Khi client kết nối, hỏi "Ho ten" và "MSSV"
- Trả về email sinh viên theo cấu trúc HUST:
  ten.viettat_hodem.mssv_bo_20_o_dau@sis.hust.edu.vn

Biên dịch:
    gcc email_server.c -o email_server

Chạy:
    ./email_server 9000
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#define MAX_CLIENTS  FD_SETSIZE
#define BUF_SIZE     1024

enum {
    STATE_WAIT_NAME = 0,
    STATE_WAIT_MSSV,
    STATE_DONE
};

typedef struct {
    int fd;
    int state;
    char name[128];
    char mssv[32];
    char inbuf[BUF_SIZE];
    int inbuf_len;
} client_t;

client_t clients[MAX_CLIENTS];

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void trim_newline(char *s) {
    int n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

void str_to_lower_ascii(char *s) {
    for (int i = 0; s[i]; i++) {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
}

void mssv2email(char *full_name, char *mssv, char *email, size_t email_sz) {
    char tmp_name[128];
    char tmp_mssv[32];
    char *tokens[20];
    int count = 0;

    strncpy(tmp_name, full_name, sizeof(tmp_name) - 1);
    tmp_name[sizeof(tmp_name) - 1] = '\0';
    strncpy(tmp_mssv, mssv, sizeof(tmp_mssv) - 1);
    tmp_mssv[sizeof(tmp_mssv) - 1] = '\0';

    str_to_lower_ascii(tmp_name);

    char *p = strtok(tmp_name, " \t");
    while (p != NULL && count < 20) {
        tokens[count++] = p;
        p = strtok(NULL, " \t");
    }

    char firstname[64] = "";
    char initials[32] = "";

    if (count > 0) {
        strncpy(firstname, tokens[count - 1], sizeof(firstname) - 1);
        firstname[sizeof(firstname) - 1] = '\0';

        for (int i = 0; i < count - 1; i++) {
            int len = strlen(initials);
            if (len < (int)sizeof(initials) - 1) {
                initials[len] = tokens[i][0];
                initials[len + 1] = '\0';
            }
        }
    } else {
        strncpy(firstname, "unknown", sizeof(firstname) - 1);
    }

    char *mssv_part = tmp_mssv;
    if (strncmp(tmp_mssv, "20", 2) == 0) {
        mssv_part = tmp_mssv + 2;
    }

    snprintf(email, email_sz, "%s.%s%s@sis.hust.edu.vn", firstname, initials, mssv_part);
}

void remove_client(int i) {
    if (clients[i].fd != -1) {
        close(clients[i].fd);
        clients[i].fd = -1;
        clients[i].state = STATE_WAIT_NAME;
        clients[i].name[0] = '\0';
        clients[i].mssv[0] = '\0';
        clients[i].inbuf_len = 0;
        clients[i].inbuf[0] = '\0';
    }
}

int send_all_nonblocking(int fd, char *msg) {
    size_t left = strlen(msg);
    char *p = msg;

    while (left > 0) {
        ssize_t n = send(fd, p, left, 0);
        if (n > 0) {
            left -= (size_t)n;
            p += n;
        } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        } else {
            return -1;
        }
    }
    return 0;
}

int extract_line(client_t *c, char *out, size_t out_sz) {
    for (int i = 0; i < c->inbuf_len; i++) {
        if (c->inbuf[i] == '\n') {
            int line_len = i + 1;
            if ((size_t)line_len >= out_sz) line_len = (int)out_sz - 1;

            memcpy(out, c->inbuf, line_len);
            out[line_len] = '\0';
            trim_newline(out);

            memmove(c->inbuf, c->inbuf + i + 1, c->inbuf_len - (i + 1));
            c->inbuf_len -= (i + 1);
            c->inbuf[c->inbuf_len] = '\0';
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
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

    if (set_nonblocking(listenfd) == -1) {
        perror("fcntl");
        close(listenfd);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(listenfd);
        return 1;
    }

    if (listen(listenfd, 16) == -1) {
        perror("listen");
        close(listenfd);
        return 1;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);

        int maxfd = listenfd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > maxfd) maxfd = clients[i].fd;
            }
        }

        int ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            continue;
        }

        /* Nhận kết nối mới */
        if (FD_ISSET(listenfd, &readfds)) {
            while (1) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);
                if (connfd == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    perror("accept");
                    break;
                }

                if (set_nonblocking(connfd) == -1) {
                    perror("fcntl connfd");
                    close(connfd);
                    continue;
                }

                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == -1) {
                        clients[i].fd = connfd;
                        clients[i].state = STATE_WAIT_NAME;
                        clients[i].name[0] = '\0';
                        clients[i].mssv[0] = '\0';
                        clients[i].inbuf_len = 0;
                        clients[i].inbuf[0] = '\0';

                        send_all_nonblocking(connfd, "Ho ten: ");
                        added = 1;
                        printf("New client connected: fd=%d\n", connfd);
                        break;
                    }
                }

                if (!added) {
                    send_all_nonblocking(connfd, "Server full.\n");
                    close(connfd);
                }
            }
        }

        /* Xử lý client */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd == -1) continue;

            if (FD_ISSET(clients[i].fd, &readfds)) {
                char buf[256];
                ssize_t n = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);

                if (n <= 0) {
                    printf("Client fd=%d disconnected.\n", clients[i].fd);
                    remove_client(i);
                    continue;
                }

                buf[n] = '\0';

                if (clients[i].inbuf_len + n >= BUF_SIZE) {
                    send_all_nonblocking(clients[i].fd, "Input too long.\n");
                    remove_client(i);
                    continue;
                }

                memcpy(clients[i].inbuf + clients[i].inbuf_len, buf, n);
                clients[i].inbuf_len += (int)n;
                clients[i].inbuf[clients[i].inbuf_len] = '\0';

                char line[256];
                while (extract_line(&clients[i], line, sizeof(line))) {
                    if (clients[i].state == STATE_WAIT_NAME) {
                        strncpy(clients[i].name, line, sizeof(clients[i].name) - 1);
                        clients[i].name[sizeof(clients[i].name) - 1] = '\0';
                        clients[i].state = STATE_WAIT_MSSV;
                        send_all_nonblocking(clients[i].fd, "MSSV: ");
                    } else if (clients[i].state == STATE_WAIT_MSSV) {
                        strncpy(clients[i].mssv, line, sizeof(clients[i].mssv) - 1);
                        clients[i].mssv[sizeof(clients[i].mssv) - 1] = '\0';

                        char email[256];
                        mssv2email(clients[i].name, clients[i].mssv, email, sizeof(email));

                        char response[512];
                        snprintf(response, sizeof(response),
                                 "Email sinh vien: %s\n", email);

                        send_all_nonblocking(clients[i].fd, response);
                        remove_client(i);
                        break;
                    }
                }
            }
        }
    }

    close(listenfd);
    return 0;
}