#include <stdio.h>

int main() {
    char response[] =
        "227 Entering Passive Mode (192,168,1,10,195,44)";

    int h1, h2, h3, h4;
    int p1, p2;

    // Tìm và đọc các số trong dấu ()
    int n = sscanf(response,
        "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
        &h1, &h2, &h3, &h4, &p1, &p2);

    if (n != 6) {
        printf("Chuoi PASV khong hop le\n");
        return 0;
    }

    int port = p1 * 256 + p2;

    printf("IP Address: %d.%d.%d.%d\n", h1, h2, h3, h4);
    printf("Port: %d\n", port);

    return 0;
}