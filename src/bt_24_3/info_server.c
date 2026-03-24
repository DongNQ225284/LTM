#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

int main() 
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        printf("Failed to create socket.\n");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        printf("bind() failed.\n");
        exit(1);
    }

    if (listen(listener, 5)) {
        printf("listen() failed.\n");
        exit(1);
    }

    printf("Waiting for client...\n");

    int client = accept(listener, NULL, NULL);
    if (client == -1) {
        printf("accept() failed.\n");
        exit(1);
    }

    printf("Client connected.\n\n");

    char buf[1024];

    // Nhận thư mục hiện tại
    int len;
    recv(client, &len, sizeof(int), 0);
    recv(client, buf, len, 0);
    buf[len] = 0;

    printf("Received folder: %s\n", buf);

    // Nhận số lượng file
    int count;
    recv(client, &count, sizeof(int), 0);

    for (int i = 0; i < count; i++) {
        // Nhận tên file
        recv(client, &len, sizeof(int), 0);
        recv(client, buf, len, 0);
        buf[len] = 0;

        // Nhận kích thước
        int size;
        recv(client, &size, sizeof(int), 0);

        printf("%s - %d bytes\n", buf, size);
    }

    close(client);
    close(listener);

    return 0;
}

/*
rootfolder$ gcc src/bt_24_3/info_server.c -o build/info_server
rootfolder$ ./build/info_server
*/