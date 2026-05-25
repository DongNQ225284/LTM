#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define MAX_FILES 256
#define MAX_NAME 256

static void send_all(int fd, const void *buf, size_t len)
{
    const char *p = (const char *)buf;
    while (len > 0)
    {
        ssize_t sent = send(fd, p, len, 0);
        if (sent <= 0)
            return;
        p += sent;
        len -= (size_t)sent;
    }
}

static void send_text(int fd, const char *text)
{
    send_all(fd, text, strlen(text));
}

static void trim_line(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
    {
        s[--len] = '\0';
    }
}

static int recv_line(int fd, char *buf, size_t size)
{
    size_t used = 0;
    while (used + 1 < size)
    {
        char ch;
        ssize_t rc = recv(fd, &ch, 1, 0);
        if (rc <= 0)
            return (int)rc;
        buf[used++] = ch;
        if (ch == '\n')
            break;
    }
    buf[used] = '\0';
    return (int)used;
}

static int is_safe_filename(const char *name)
{
    if (name[0] == '\0')
        return 0;
    if (strstr(name, "..") != NULL)
        return 0;
    if (strchr(name, '/') != NULL)
        return 0;
    return 1;
}

static int list_regular_files(const char *dir_path, char files[][MAX_NAME], int max_files)
{
    DIR *dir = opendir(dir_path);
    struct dirent *entry;
    int count = 0;

    if (dir == NULL)
        return -1;

    while ((entry = readdir(dir)) != NULL)
    {
        char full_path[PATH_MAX];
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (stat(full_path, &st) == -1)
            continue;
        if (!S_ISREG(st.st_mode))
            continue;

        strncpy(files[count], entry->d_name, MAX_NAME - 1);
        files[count][MAX_NAME - 1] = '\0';
        count++;
        if (count >= max_files)
            break;
    }

    closedir(dir);
    return count;
}

static int send_file_list(int client_fd, const char *dir_path)
{
    char files[MAX_FILES][MAX_NAME];
    char line[BUFFER_SIZE];
    int count = list_regular_files(dir_path, files, MAX_FILES);

    if (count <= 0)
    {
        send_text(client_fd, "ERROR No files to download\r\n");
        return -1;
    }

    snprintf(line, sizeof(line), "OK %d\r\n", count);
    send_text(client_fd, line);
    for (int i = 0; i < count; i++)
    {
        snprintf(line, sizeof(line), "%s\r\n", files[i]);
        send_text(client_fd, line);
    }
    send_text(client_fd, "\r\n");
    return 0;
}

static int send_file_content(int client_fd, const char *file_path, const char *file_name)
{
    FILE *fp = fopen(file_path, "rb");
    char header[128];
    char buf[BUFFER_SIZE];
    size_t nread;
    struct stat st;

    if (fp == NULL || stat(file_path, &st) == -1)
    {
        if (fp != NULL)
            fclose(fp);
        return -1;
    }

    snprintf(header, sizeof(header), "OK %ld\r\n", (long)st.st_size);
    send_text(client_fd, header);

    while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0)
        send_all(client_fd, buf, nread);

    fclose(fp);
    printf("Sent file %s (%ld bytes)\n", file_name, (long)st.st_size);
    return 0;
}

static void handle_client(int client_fd, const char *dir_path)
{
    char line[BUFFER_SIZE];

    if (send_file_list(client_fd, dir_path) != 0)
        return;

    while (1)
    {
        char requested[MAX_NAME];
        char file_path[PATH_MAX];
        struct stat st;

        send_text(client_fd, "Enter filename:\r\n");
        if (recv_line(client_fd, line, sizeof(line)) <= 0)
            return;

        trim_line(line);
        if (!is_safe_filename(line))
        {
            send_text(client_fd, "ERROR Invalid filename\r\n");
            continue;
        }

        strncpy(requested, line, sizeof(requested) - 1);
        requested[sizeof(requested) - 1] = '\0';
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, requested);

        if (stat(file_path, &st) == -1 || !S_ISREG(st.st_mode))
        {
            send_text(client_fd, "ERROR File not found\r\n");
            continue;
        }

        send_file_content(client_fd, file_path, requested);
        return;
    }
}

int main(int argc, char *argv[])
{
    int port;
    const char *share_dir;
    int listen_fd;
    int opt = 1;
    struct sockaddr_in addr;

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <port> <share_dir>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    share_dir = argv[2];

    signal(SIGCHLD, SIG_IGN);

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd == -1)
    {
        perror("socket");
        return 1;
    }

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 16) == -1)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("File server listening on port %d, share_dir=%s\n", port, share_dir);

    while (1)
    {
        fd_set readfds;
        int rc;

        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);

        rc = select(listen_fd + 1, &readfds, NULL, NULL, NULL);
        if (rc < 0)
        {
            if (errno == EINTR)
                continue;
            perror("select");
            break;
        }

        if (FD_ISSET(listen_fd, &readfds))
        {
            int client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd < 0)
            {
                perror("accept");
                continue;
            }

            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork");
                close(client_fd);
                continue;
            }

            if (pid == 0)
            {
                close(listen_fd);
                handle_client(client_fd, share_dir);
                close(client_fd);
                _exit(0);
            }

            close(client_fd);
        }
    }

    close(listen_fd);
    return 0;
}
