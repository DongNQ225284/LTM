#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

typedef struct
{
    int client_fd;
    char db_file[256];
} client_arg_t;

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

static int check_account(const char *db_file, const char *username, const char *password)
{
    FILE *fp = fopen(db_file, "r");
    char user[64];
    char pass[64];

    if (fp == NULL)
        return 0;

    while (fscanf(fp, "%63s %63s", user, pass) == 2)
    {
        if (strcmp(user, username) == 0 && strcmp(pass, password) == 0)
        {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

static void send_command_output(int fd, const char *cmd)
{
    char template_name[] = "/tmp/telnet_mt_XXXXXX";
    char shell_cmd[1024];
    char buf[1024];
    int tmp_fd = mkstemp(template_name);
    FILE *fp;
    size_t nread;

    if (tmp_fd < 0)
    {
        send_text(fd, "Cannot create temp file.\n> ");
        return;
    }
    close(tmp_fd);

    snprintf(shell_cmd, sizeof(shell_cmd), "%s > %s 2>&1", cmd, template_name);
    system(shell_cmd);

    fp = fopen(template_name, "r");
    if (fp == NULL)
    {
        unlink(template_name);
        send_text(fd, "Cannot read command output.\n> ");
        return;
    }

    while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0)
        send(fd, buf, nread, 0);

    fclose(fp);
    unlink(template_name);
    send_text(fd, "\n[END]\n> ");
}

static void *client_thread(void *arg)
{
    client_arg_t *client = (client_arg_t *)arg;
    char username[64];
    char line[BUFFER_SIZE];

    while (1)
    {
        send_text(client->client_fd, "Username: ");
        if (recv_line(client->client_fd, line, sizeof(line)) <= 0)
            goto cleanup;
        trim_line(line);
        if (line[0] == '\0')
            continue;
        strncpy(username, line, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';

        send_text(client->client_fd, "Password: ");
        if (recv_line(client->client_fd, line, sizeof(line)) <= 0)
            goto cleanup;
        trim_line(line);

        if (check_account(client->db_file, username, line))
            break;

        send_text(client->client_fd, "Dang nhap that bai. Thu lai.\n");
    }

    send_text(client->client_fd, "Dang nhap thanh cong.\nNhap lenh, go 'exit' de thoat.\n> ");

    while (1)
    {
        if (recv_line(client->client_fd, line, sizeof(line)) <= 0)
            break;
        trim_line(line);

        if (strcmp(line, "exit") == 0)
        {
            send_text(client->client_fd, "Bye.\n");
            break;
        }

        if (line[0] == '\0')
        {
            send_text(client->client_fd, "> ");
            continue;
        }

        send_command_output(client->client_fd, line);
    }

cleanup:
    close(client->client_fd);
    free(client);
    return NULL;
}

int main(int argc, char *argv[])
{
    int port;
    const char *db_file;
    int listen_fd;
    int opt = 1;
    struct sockaddr_in addr;

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s <port> [account_file]\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    db_file = (argc == 3) ? argv[2] : "src/bt_19_5/telnet_users.txt";

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

    printf("Telnet multithread server listening on port %d\n", port);

    while (1)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        client_arg_t *arg;
        pthread_t tid;

        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        arg = (client_arg_t *)malloc(sizeof(client_arg_t));
        if (arg == NULL)
        {
            close(client_fd);
            continue;
        }

        arg->client_fd = client_fd;
        strncpy(arg->db_file, db_file, sizeof(arg->db_file) - 1);
        arg->db_file[sizeof(arg->db_file) - 1] = '\0';

        if (pthread_create(&tid, NULL, client_thread, arg) == 0)
            pthread_detach(tid);
        else
        {
            close(client_fd);
            free(arg);
        }
    }
}
