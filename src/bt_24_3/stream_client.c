/*
Ứng dụng client stream: liên tục gửi dữ liệu văn bản sang server
*/

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

    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(client, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("connect");
        return 1;
    }

    printf("Connected to server %s:%s\n", argv[1], argv[2]);

    char buf[256];
    while (1) {
        printf("Enter a message (or 'exit' to quit): ");
        fgets(buf, sizeof(buf), stdin);

        if (strcmp(buf, "exit\n") == 0) {
            break;
        }

        int ret = send(client, buf, strlen(buf), 0);
        if (ret == -1) {
            perror("send");
            return 1;
        }
        printf("Sent %d bytes\n", ret);
    }

    close(client);
    return 0;
}

/*
rootfolder$ gcc src/bt_24_3/stream_client.c -o build/stream_client
rootfolder$ ./build/stream_client 127.0.0.1 9000
*/
