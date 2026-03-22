#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int client = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    
    FILE *f = fopen("src/ch3/msg.txt", "rb");
    if (f == NULL) {
        perror("fopen");
        return 1;
    }
    
    if (connect(client, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("connect");
        return 1;
    }
    char buf[2048];

    while (1) {
        int len = fread(buf, 1, sizeof(buf), f);
        if (len <= 0) break;
        send(client, buf, len, 0);
    }

    printf("File sent successfully.\n");
    shutdown(client, SHUT_WR);

    char reply[100];
    int n = recv(client, reply, sizeof(reply) - 1, 0);
    if (n > 0) {
        reply[n] = '\0';
        printf("Server reply: %s\n", reply);
    }
    fclose(f);
    close(client);
    return 0;
}