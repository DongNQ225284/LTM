/*
Viết chương trình sv_client, cho phép người dùng nhập dữ liệu là thông tin của sinh viên bao gồm MSSV, họ tên, ngày sinh,
và điểm trung bình các môn học. Các thông tin trên được đóng gói và gửi sang sv_server. Địa chỉ và cổng của server được
nhập từ tham số dòng lệnh. 
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

struct sinhvien {
    char mssv[16];
    char hoten[32];
    char ngaysinh[16];
    float cpa;
};

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

    char mssv[20], name[50], dob[20];
    float gpa;

    while(1) {
        printf("Enter MSSV: ");
        fgets(mssv, sizeof(mssv), stdin);
        mssv[strcspn(mssv, "\n")] = '\0'; 

        printf("Enter Name: ");
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = '\0'; 

        printf("Enter Date of Birth (YYYY-MM-DD): ");
        fgets(dob, sizeof(dob), stdin);
        dob[strcspn(dob, "\n")] = '\0'; 

        printf("Enter GPA: ");
        scanf("%f", &gpa);

        char buf[256];
        snprintf(buf, sizeof(buf), "%s|%s|%s|%.2f", mssv, name, dob, gpa);

        int ret = send(client, buf, strlen(buf), 0);
        if (ret == -1) {
            printf("Failed to send message.\n");
            return 1;
        }
        printf("Data sent successfully.\n");
        printf("Do you want to enter another student? (y/n): ");
        char choice;
        scanf(" %c", &choice);
        getchar();
        if (choice != 'y' && choice != 'Y') {
            break;
        }
    }
    close(client);
    return 0;
}

/*
rootfolder$ gcc src/btch3/sv_client.c -o build/sv_client
rootfolder$ ./build/sv_client 127.0.0.1 9000
*/