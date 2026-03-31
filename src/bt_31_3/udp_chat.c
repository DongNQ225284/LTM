#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#define BUF_SIZE 1024

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    if (set_nonblocking(sock) == -1) {
        perror("fcntl");
        close(sock);
        return 1;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port_s);

    if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) == -1) {
        perror("bind");
        close(sock);
        return 1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_d);
    dest_addr.sin_addr.s_addr = inet_addr(ip_d);

    printf("UDP chat started.\n");
    printf("Local port : %d\n", port_s);
    printf("Remote     : %s:%d\n", ip_d, port_d);
    printf("Type message and press Enter.\n");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        int maxfd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        int ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char sendbuf[BUF_SIZE];
            if (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL) {
                size_t len = strlen(sendbuf);
                if (len > 0) {
                    ssize_t sent = sendto(sock, sendbuf, len, 0,
                                          (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                    if (sent == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("sendto");
                    }
                }
            }
        }

        if (FD_ISSET(sock, &readfds)) {
            while (1) {
                char recvbuf[BUF_SIZE];
                struct sockaddr_in peer_addr;
                socklen_t peer_len = sizeof(peer_addr);

                ssize_t n = recvfrom(sock, recvbuf, sizeof(recvbuf) - 1, 0,
                                     (struct sockaddr*)&peer_addr, &peer_len);

                if (n == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    perror("recvfrom");
                    break;
                }

                recvbuf[n] = '\0';
                printf("\n[%s:%d] %s",
                       inet_ntoa(peer_addr.sin_addr),
                       ntohs(peer_addr.sin_port),
                       recvbuf);
                fflush(stdout);
            }
        }
    }

    close(sock);
    return 0;
}