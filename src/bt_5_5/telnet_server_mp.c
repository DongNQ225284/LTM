#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

static void send_text(int fd, const char *text)
{
    send(fd, text, strlen(text), 0);
}

static void trim_line(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
    {
        s[len - 1] = 0;
        len--;
    }
}

static int recv_line(int fd, char *buf, size_t size)
{
    size_t used = 0;

    while (used + 1 < size)
    {
        char ch;
        int ret = recv(fd, &ch, 1, 0);
        if (ret <= 0)
            return ret;

        buf[used++] = ch;
        if (ch == '\n')
            break;
    }

    buf[used] = 0;
    return (int)used;
}

static int check_account(const char *db_file, const char *username, const char *password)
{
    FILE *f = fopen(db_file, "r");
    char user[64];
    char pass[64];

    if (f == NULL)
        return 0;

    while (fscanf(f, "%63s %63s", user, pass) == 2)
    {
        if (strcmp(user, username) == 0 && strcmp(pass, password) == 0)
        {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

static void send_command_output(int fd, const char *cmd)
{
    char outfile[128];
    char system_cmd[1024];
    char read_buf[1024];
    FILE *f;
    size_t nread;
    snprintf(outfile, sizeof(outfile), "/tmp/telnet_mp_%d.txt", getpid());
    snprintf(system_cmd, sizeof(system_cmd), "%s > %s 2>&1", cmd, outfile);

    system(system_cmd);
    f = fopen(outfile, "r");
    if (f == NULL)
    {
        send_text(fd, "Khong mo duoc file ket qua lenh.\n> ");
        return;
    }

    while ((nread = fread(read_buf, 1, sizeof(read_buf), f)) > 0)
        send(fd, read_buf, nread, 0);

    fclose(f);
    remove(outfile);
    send_text(fd, "\n[END]\n> ");
}

static void handle_client(int client_fd, const char *db_file)
{
    char username[64];
    char line[BUFFER_SIZE];

    while (1)
    {
        send_text(client_fd, "Username: ");
        if (recv_line(client_fd, line, sizeof(line)) <= 0)
            return;
        trim_line(line);
        if (strlen(line) == 0)
            continue;

        strncpy(username, line, sizeof(username) - 1);
        username[sizeof(username) - 1] = 0;

        send_text(client_fd, "Password: ");
        if (recv_line(client_fd, line, sizeof(line)) <= 0)
            return;
        trim_line(line);

        if (check_account(db_file, username, line))
            break;

        send_text(client_fd, "Dang nhap that bai. Thu lai.\n");
    }

    send_text(client_fd,
              "Dang nhap thanh cong.\nNhap lenh he thong, go 'exit' de thoat.\n> ");

    while (1)
    {
        int ret = recv_line(client_fd, line, sizeof(line));
        if (ret <= 0)
            return;

        trim_line(line);
        if (strcmp(line, "exit") == 0)
        {
            send_text(client_fd, "Bye.\n");
            return;
        }

        if (strlen(line) == 0)
        {
            send_text(client_fd, "> ");
            continue;
        }

        send_command_output(client_fd, line);
    }
}

static void reap_children(void)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }
}

int main(int argc, char *argv[])
{
    int listener;
    int opt = 1;
    int port;
    const char *db_file;
    struct sockaddr_in addr;

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s <port> [account_file]\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    db_file = (argc == 3) ? argv[2] : "src/bt_5_5/telnet_users.txt";

    signal(SIGCHLD, SIG_IGN);

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

    printf("Telnet multiprocessing server listening on port %d\n", port);
    printf("Account file: %s\n", db_file);

    while (1)
    {
        fd_set readfds;
        int ret;

        FD_ZERO(&readfds);
        FD_SET(listener, &readfds);

        ret = select(listener + 1, &readfds, NULL, NULL, NULL);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            perror("select");
            break;
        }

        if (FD_ISSET(listener, &readfds))
        {
            int client_fd = accept(listener, NULL, NULL);
            if (client_fd == -1)
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
                close(listener);
                handle_client(client_fd, db_file);
                close(client_fd);
                _exit(0);
            }

            close(client_fd);
            reap_children();
        }
    }

    close(listener);
    return 0;
}
