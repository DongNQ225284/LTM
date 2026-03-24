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
    // Tạo socket UDP
    int server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server == -1) {
        printf("Failed to create socket.\n");
        exit(1);
    }

    // Cấu hình địa chỉ server
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    // Bind socket vào địa chỉ
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr))) {
        printf("bind() failed.\n");
        exit(1);
    }

    printf("UDP Echo Server listening on port 9000...\n");

    char buf[256];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1) {
        // Nhận dữ liệu từ client
        int ret = recvfrom(server, buf, sizeof(buf) - 1, 0, 
                          (struct sockaddr *)&client_addr, &client_addr_len);
        if (ret == -1) {
            printf("Failed to receive message.\n");
            continue;
        }

        buf[ret] = '\0';
        printf("Received from %s:%d: %s\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port), 
               buf);

        // Gửi lại dữ liệu cho client
        if (sendto(server, buf, ret, 0, 
                  (struct sockaddr *)&client_addr, client_addr_len) == -1) {
            printf("Failed to send message.\n");
            continue;
        }
        printf("Echoed back to %s:%d\n\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
    }

    close(server);
    return 0;
}

/*
rootfolder$ gcc src/bt_24_3/udp_echo.c -o build/udp_echo
rootfolder$ ./build/udp_echo
*/
