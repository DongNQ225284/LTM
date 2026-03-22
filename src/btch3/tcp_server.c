/*
Viết chương trình tcp_server, đợi kết nối ở cổng xác định bởi tham số dòng lệnh. Mỗi khi có client kết
nối đến, thì gửi xâu chào được chỉ ra trong một tệp tin xác định, sau đó ghi toàn bộ nội dung client gửi đến 
vào một tệp tin khác được chỉ ra trong tham số dòng lệnh
tcp_server <cổng> <tệp tin chứa câu chào> <tệp tin lưu nội dung client gửi đến>
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <greeting_file> <output_file>\n", argv[0]);
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
        int client = accept(server, NULL, NULL);
        if (client == -1) {
            perror("accept");
            continue;
        }

        FILE *greeting_file = fopen(argv[2], "r");
        if (greeting_file == NULL) {
            perror("fopen greeting");
            close(client);
            continue;
        }
        
        char greeting[256];
        fgets(greeting, sizeof(greeting), greeting_file);
        fclose(greeting_file);

        send(client, greeting, strlen(greeting), 0);

        FILE *output_file = fopen(argv[3], "ab");
        if (output_file == NULL) {
            perror("fopen output");
            close(client);
            continue;
        }

        char buf[256];
        while (1) {
            int ret = recv(client, buf, sizeof(buf), 0);
            if (ret <= 0) break;
            fwrite(buf, 1, ret, output_file);
        }
        fclose(output_file);
        close(client);
    }
}

/*
rootfolder$ gcc src/btch3/tcp_server.c -o build/tcp_server
rootfolder$ ./build/tcp_server 9000 src/btch3/tcp_greeting.txt src/btch3/tcp_output.txt
*/