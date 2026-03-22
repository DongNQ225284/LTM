#include <stdio.h>
#include <string.h>

int main() {
    FILE *f;
    const char *target = "Ky thuat May tinh";
    int len = strlen(target);

    int i = 0;
    int ch;

    f = fopen("src/ch1/file2.txt", "r");
    if (f == NULL) {
        printf("Khong mo duoc file!\n");
        return 1;
    }

    // Đọc từng ký tự
    while ((ch = fgetc(f)) != EOF) {

        if (ch == target[i]) {
            i++;
            if (i == len) {
                printf("Tim thay chuoi!\n");
                fclose(f);
                return 0;
            }
        } else {
            // reset nếu sai
            if (ch == target[0])
                i = 1;
            else
                i = 0;
        }
    }

    printf("Khong tim thay!\n");

    fclose(f);
    return 0;
}