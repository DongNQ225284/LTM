/*
Ứng dụng server stream: nhận dữ liệu từ client, đếm số lần xuất hiện "0123456789"
Xử lý trường hợp pattern nằm giữa 2 lần truyền
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define PATTERN "0123456789"
#define PATTERN_LEN 10
#define BUFFER_SIZE 4096
#define OVERLAP_SIZE (PATTERN_LEN - 1)

// Hàm đếm số lần xuất hiện pattern trong string
int count_pattern(const char *data, int len) {
    int count = 0;
    for (int i = 0; i <= len - PATTERN_LEN; i++) {
        if (strncmp(&data[i], PATTERN, PATTERN_LEN) == 0) {
            count++;
        }
    }
    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server, 5) == -1) {
        perror("listen");
        return 1;
    }

    printf("Server listening on port %s...\n", argv[1]);

    while (1) {
        int client = accept(server, NULL, NULL);
        if (client == -1) {
            perror("accept");
            continue;
        }

        printf("\nClient connected.\n");

        // Buffer để lưu dữ liệu chồng chéo từ lần truyền trước
        char overlap_buf[OVERLAP_SIZE];
        int overlap_len = 0;
        int total_count = 0;

        char recv_buf[BUFFER_SIZE];
        int recv_len;

        while (1) {
            recv_len = recv(client, recv_buf, sizeof(recv_buf), 0);
            if (recv_len <= 0) {
                break;
            }

            // Tạo buffer tạm để kiểm tra: overlap_buf + recv_buf
            char *check_buf = (char *)malloc(overlap_len + recv_len);
            if (check_buf == NULL) {
                perror("malloc");
                break;
            }

            // Copy dữ liệu chồng chéo từ lần trước
            if (overlap_len > 0) {
                memcpy(check_buf, overlap_buf, overlap_len);
            }

            // Copy dữ liệu mới nhận được
            memcpy(check_buf + overlap_len, recv_buf, recv_len);

            int total_len = overlap_len + recv_len;

            // Đếm pattern trong buffer kết hợp
            int count = count_pattern(check_buf, total_len);
            total_count += count;

            // Hiển thị kết quả
            printf("Received %d bytes. Pattern count in this chunk: %d. Total count: %d\n",
                   recv_len, count, total_count);

            // Giữ lại OVERLAP_SIZE ký tự cuối cùng cho lần kiểm tra tiếp theo
            if (total_len >= OVERLAP_SIZE) {
                memcpy(overlap_buf, check_buf + total_len - OVERLAP_SIZE, OVERLAP_SIZE);
                overlap_len = OVERLAP_SIZE;
            } else {
                memcpy(overlap_buf, check_buf, total_len);
                overlap_len = total_len;
            }

            free(check_buf);
        }

        printf("Client disconnected. Final pattern count: %d\n", total_count);
        close(client);
    }

    close(server);
    return 0;
}

/*
rootfolder$ gcc src/bt_24_3/stream_server.c -o build/stream_server
rootfolder$ ./build/stream_server 9000
*/
