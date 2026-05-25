#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 512

static void send_text(int fd, const char *text)
{
    send(fd, text, strlen(text), 0);
}

static void trim_line(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

static int recv_line(int fd, char *buf, size_t size)
{
    size_t used = 0;
    while (used + 1 < size)
    {
        char ch;
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0)
            return (int)n;
        buf[used++] = ch;
        if (ch == '\n')
            break;
    }
    buf[used] = '\0';
    return (int)used;
}

static const char *resolve_strftime_format(const char *format)
{
    if (strcmp(format, "dd/mm/yyyy") == 0)
        return "%d/%m/%Y";
    if (strcmp(format, "dd/mm/yy") == 0)
        return "%d/%m/%y";
    if (strcmp(format, "mm/dd/yyyy") == 0)
        return "%m/%d/%Y";
    if (strcmp(format, "mm/dd/yy") == 0)
        return "%m/%d/%y";
    return NULL;
}

static void *client_thread(void *arg)
{
    int client_fd = *(int *)arg;
    char line[BUFFER_SIZE];
    free(arg);

    while (1)
    {
        const char *fmt;
        char requested[64];
        char out[128];
        time_t now;
        struct tm tm_now;

        if (recv_line(client_fd, line, sizeof(line)) <= 0)
            break;
        trim_line(line);

        if (sscanf(line, "GET_TIME %63s", requested) != 1)
        {
            send_text(client_fd, "ERROR Invalid command\n");
            continue;
        }

        fmt = resolve_strftime_format(requested);
        if (fmt == NULL)
        {
            send_text(client_fd, "ERROR Unsupported format\n");
            continue;
        }

        now = time(NULL);
        localtime_r(&now, &tm_now);
        strftime(out, sizeof(out), fmt, &tm_now);
        strcat(out, "\n");
        send_text(client_fd, out);
    }

    close(client_fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    int port;
    int listen_fd;
    int opt = 1;
    struct sockaddr_in addr;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
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

    if (listen(listen_fd, 32) == -1)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("Time multithread server listening on port %d\n", port);

    while (1)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        pthread_t tid;
        int *arg;

        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        arg = (int *)malloc(sizeof(int));
        if (arg == NULL)
        {
            close(client_fd);
            continue;
        }
        *arg = client_fd;

        if (pthread_create(&tid, NULL, client_thread, arg) == 0)
            pthread_detach(tid);
        else
        {
            close(client_fd);
            free(arg);
        }
    }
}
