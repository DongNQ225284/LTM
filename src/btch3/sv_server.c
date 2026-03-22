/*
Viết chương trình sv_server, nhận dữ liệu từ sv_client, in ra màn hình và đồng thời ghi vào file sv_log.txt. Dữ liệu được ghi 
trên một dòng với mỗi client, kèm theo địa chỉ IP và thời gian client đã gửi. Tham số cổng và tên file log được nhập từ tham
số dòng lệnh
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <log_file>\n", argv[0]);
        return 1;
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client = accept(server, (struct sockaddr*)&client_addr, &client_len);
        if (client == -1) {
            perror("accept");
            continue;
        }

        char buf[256];
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            close(client);
            continue;
        }
        buf[ret] = '\0';

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));

        time_t now = time(NULL);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

        printf("Received from %s at %s: %s\n", ip_str, time_str, buf);

        FILE *log_file = fopen(argv[2], "a");
        if (log_file != NULL) {
            fprintf(log_file, "%s - %s - %s\n", ip_str, time_str, buf);
            fclose(log_file);
        } else {
            perror("fopen log");
        }

        close(client);
    }
    return 0;
}

/*
rootfolder$ gcc src/btch3/sv_server.c -o build/sv_server
rootfolder$ ./build/sv_server 9000 src/btch3/sv_log.txt
*/