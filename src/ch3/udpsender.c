#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

int main() {
    int sender = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sender == -1) {
        printf("Failed to create socket.\n");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9000);

    char buf[256];

    while(1) {
        printf("Enter a message: ");
        fgets(buf, sizeof(buf), stdin);
        int ret = sendto(sender, buf, strlen(buf), 0, (struct sockaddr*)&addr, sizeof(addr));
        if (ret == -1) {
            printf("Failed to send message.\n");
            exit(1);
        }
        printf("Return value: %d\n", ret);
    }
    close(sender);
    return 0;
}