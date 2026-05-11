#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEFAULT_WORKERS 4
#define BUFFER_SIZE 4096

static volatile sig_atomic_t running = 1;

static void handle_term(int sig)
{
    (void)sig;
    running = 0;
}

static void send_all(int fd, const char *buf, size_t len)
{
    while (len > 0)
    {
        ssize_t sent = send(fd, buf, len, 0);
        if (sent <= 0)
            return;
        buf += sent;
        len -= (size_t)sent;
    }
}

static void handle_http_client(int client_fd, pid_t worker_pid)
{
    char buf[BUFFER_SIZE];
    int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (ret <= 0)
        return;

    buf[ret] = 0;
    printf("[worker %d] request:\n%s\n", worker_pid, buf);

    const char *body =
        "<html><body><h1>Xin chao cac ban</h1>"
        "<p>HTTP prefork server dang hoat dong.</p></body></html>";

    char response[BUFFER_SIZE];
    int body_len = (int)strlen(body);
    int n = snprintf(response, sizeof(response),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html; charset=utf-8\r\n"
                     "Content-Length: %d\r\n"
                     "Connection: close\r\n\r\n"
                     "%s",
                     body_len, body);
    if (n > 0)
        send_all(client_fd, response, (size_t)n);
}

static void worker_loop(int listener)
{
    signal(SIGTERM, handle_term);
    signal(SIGINT, handle_term);

    while (running)
    {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0)
        {
            if (errno == EINTR)
                continue;
            perror("accept");
            continue;
        }

        handle_http_client(client_fd, getpid());
        close(client_fd);
    }
}

static pid_t spawn_worker(int listener)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        worker_loop(listener);
        close(listener);
        _exit(0);
    }
    return pid;
}

int main(int argc, char *argv[])
{
    int port;
    int worker_count = DEFAULT_WORKERS;
    int listener;
    int opt = 1;
    struct sockaddr_in addr;
    pid_t workers[64];

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s <port> [worker_count]\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    if (argc == 3)
        worker_count = atoi(argv[2]);

    if (worker_count <= 0 || worker_count > 64)
    {
        fprintf(stderr, "worker_count must be in range 1..64\n");
        return 1;
    }

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket");
        return 1;
    }

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(listener);
        return 1;
    }

    if (listen(listener, 16) == -1)
    {
        perror("listen");
        close(listener);
        return 1;
    }

    signal(SIGTERM, handle_term);
    signal(SIGINT, handle_term);

    for (int i = 0; i < worker_count; i++)
    {
        workers[i] = spawn_worker(listener);
        if (workers[i] < 0)
        {
            perror("fork");
            running = 0;
            worker_count = i;
            break;
        }
    }

    printf("HTTP prefork server listening on port %d with %d workers\n",
           port, worker_count);

    while (running)
    {
        int status;
        pid_t dead = wait(&status);

        if (dead < 0)
        {
            if (errno == EINTR)
                continue;
            perror("wait");
            break;
        }

        for (int i = 0; i < worker_count; i++)
        {
            if (workers[i] == dead)
            {
                if (running)
                {
                    workers[i] = spawn_worker(listener);
                    if (workers[i] < 0)
                    {
                        perror("fork");
                        running = 0;
                    }
                    else
                    {
                        printf("Respawned worker %d -> %d\n", dead, workers[i]);
                    }
                }
                break;
            }
        }
    }

    for (int i = 0; i < worker_count; i++)
    {
        if (workers[i] > 0)
            kill(workers[i], SIGTERM);
    }

    while (waitpid(-1, NULL, 0) > 0)
    {
    }

    close(listener);
    return 0;
}
