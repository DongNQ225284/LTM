#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

int main() 
{
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1) {
        printf("Cannot create socket.\n");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr))) {
        printf("connect() failed.\n");
        close(client);
        exit(1);
    }

    char folder[1024];
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char fullpath[2048];
    char confirm;

    while (1) {
        printf("Enter folder path: ");
        fgets(folder, sizeof(folder), stdin);
        folder[strcspn(folder, "\n")] = 0;

        dir = opendir(folder);
        if (dir == NULL) {
            printf("Cannot open directory.\n");
            continue;
        }

        printf("\nContents of directory %s:\n", folder);

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            sprintf(fullpath, "%s/%s", folder, entry->d_name);

            if (stat(fullpath, &st) == -1)
                continue;

            if (S_ISDIR(st.st_mode))
                printf("[DIR ] %s\n", entry->d_name);
            else if (S_ISREG(st.st_mode))
                printf("[FILE] %s\n", entry->d_name);
        }

        closedir(dir);

        printf("\nConfirm send this folder? [y/n]: ");
        scanf(" %c", &confirm);
        getchar();

        if (confirm == 'y' || confirm == 'Y')
            break;
    }


    int len = strlen(folder);
    send(client, &len, sizeof(int), 0);
    send(client, folder, len, 0);

    dir = opendir(folder);
    if (dir == NULL) {
        printf("opendir() failed.\n");
        close(client);
        exit(1);
    }

    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        sprintf(fullpath, "%s/%s", folder, entry->d_name);

        if (stat(fullpath, &st) == -1)
            continue;

        if (S_ISREG(st.st_mode))
            count++;
    }

    rewinddir(dir);

    send(client, &count, sizeof(int), 0);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        sprintf(fullpath, "%s/%s", folder, entry->d_name);

        if (stat(fullpath, &st) == -1)
            continue;

        if (!S_ISREG(st.st_mode))
            continue;

        len = strlen(entry->d_name);
        send(client, &len, sizeof(int), 0);
        send(client, entry->d_name, len, 0);

        int size = (int)st.st_size;
        send(client, &size, sizeof(int), 0);
    }

    closedir(dir);
    close(client);

    return 0;
}

/*
rootfolder$ gcc src/bt_24_3/info_client.c -o build/info_client
rootfolder$ ./build/info_client
*/