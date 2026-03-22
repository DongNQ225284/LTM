#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    FILE *f;
    char *buffer;
    long fileSize;
    const char *target = "Ky thuat May tinh";

    f = fopen("src/ch1/file1.txt", "r");
    if (f == NULL) {
        printf("Khong mo duoc file!\n");
        return 1;
    }

    // Đưa con trỏ về cuối file
    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    rewind(f);

    // Cấp phát động
    buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL) {
        printf("Khong du bo nho!\n");
        fclose(f);
        return 1;
    }

    // Đọc toàn bộ file
    fread(buffer, 1, fileSize, f);
    buffer[fileSize] = '\0';

    // Kiểm tra chuỗi
    if (strstr(buffer, target))
        printf("Tim thay chuoi!\n");
    else
        printf("Khong tim thay!\n");

    free(buffer);
    fclose(f);

    return 0;
}
