#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <port>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("connect");
        close(sock);
        return 1;
    }

    char buf[512];

    int n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        perror("recv");
        close(sock);
        return 1;
    }
    buf[n] = '\0';
    printf("%s", buf);

    fgets(buf, sizeof(buf), stdin);
    send(sock, buf, strlen(buf), 0);

    n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        perror("recv");
        close(sock);
        return 1;
    }
    buf[n] = '\0';
    printf("%s", buf);

    fgets(buf, sizeof(buf), stdin);
    send(sock, buf, strlen(buf), 0);

    n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }

    close(sock);
    return 0;
}