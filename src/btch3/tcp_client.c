/*
Viết chương trình tcp_client, kết nối đến một máy chủ xác định bởi địa chỉ IP và cổng. Sau đó nhận dữ
liệu từ bàn phím và gửi đến server. Tham số được truyền vào từ dòng lệnh có dạng
tcp_client <địa chỉ IP> <cổng>
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <port>\n", argv[0]);
        return 1;
    }

    int client = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(client, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("connect");
        return 1;
    }
    
    char greeting[256];
    int greet_len = recv(client, greeting, sizeof(greeting) - 1, 0);
    if (greet_len > 0) {
        greeting[greet_len] = '\0';
        printf("Server greeting: %s", greeting);
        if (greeting[greet_len - 1] != '\n') {
            printf("\n");
        }
    } else if (greet_len == 0) {
        printf("Server closed connection immediately.\n");
        close(client);
        return 1;
    } else {
        perror("recv greeting");
    }

    char buf[256];
    while (1) {
        printf("Enter a message: ");
        fgets(buf, sizeof(buf), stdin);
        if (strcmp(buf, "exit\n") == 0) {
            break;
        }
        int ret = send(client, buf, strlen(buf), 0);
        if (ret == -1) {
            printf("Failed to send message.\n");
            return 1;
        }
        printf("Return value: %d\n", ret);
    }
    close(client);
    return 0;
}

/*
rootfolder$ gcc src/btch3/tcp_client.c -o build/tcp_client
rootfolder$ ./build/tcp_client 127.0.0.1 9000
*/