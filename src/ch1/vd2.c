#include <stdio.h>
#include <string.h>

int main() {
    char input[100];

    // Ví dụ chuỗi nhận từ client
    fgets(input, sizeof(input), stdin);

    char cmd[10];
    double x, y;

    // Parse chuỗi
    int n = sscanf(input, "%9s %lf %lf", cmd, &x, &y);

    // Kiểm tra đúng 3 thành phần
    if (n != 3) {
        printf("Sai cu phap!\n");
        return 0;
    }

    // Kiểm tra lệnh hợp lệ
    if (strcmp(cmd, "ADD") != 0 &&
        strcmp(cmd, "SUB") != 0 &&
        strcmp(cmd, "MUL") != 0 &&
        strcmp(cmd, "DIV") != 0) {

        printf("Lenh khong hop le!\n");
        return 0;
    }

    // Hợp lệ
    printf("Chuoi hop le!\n");
    printf("CMD = %s\n", cmd);
    printf("X = %lf\n", x);
    printf("Y = %lf\n", y);

    return 0;
}