#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

int main() {
    int receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiver == -1) {
        printf("Failed to create socket.\n");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    bind(receiver, (struct sockaddr*)&addr, sizeof(addr));

    char buf[256];
    while (1) {
        int ret = recvfrom(receiver, buf, sizeof(buf) - 1, 0, NULL, NULL);
        if (ret == -1) {
            printf("Failed to receive message.\n");
            exit(1);
        }
        buf[ret] = '\0';
        printf("Received message: %s\n", buf);
    }
}
