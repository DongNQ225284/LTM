#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 1024
#define BUFFER_SIZE 4096

typedef enum
{
    WAIT_USERNAME,
    WAIT_PASSWORD,
    AUTHENTICATED
} ClientState;

typedef struct
{
    int fd;
    ClientState state;
    char username[64];
    char buffer[BUFFER_SIZE];
    int buffer_len;
} Client;

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

static void reset_client(Client *client)
{
    client->fd = -1;
    client->state = WAIT_USERNAME;
    client->username[0] = 0;
    client->buffer_len = 0;
    client->buffer[0] = 0;
}

static void remove_client(Client clients[], int index)
{
    close(clients[index].fd);
    reset_client(&clients[index]);
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
    char outfile[64];
    char system_cmd[1024];
    FILE *f;
    char read_buf[1024];
    size_t nread;
    int ret;

    snprintf(outfile, sizeof(outfile), "/tmp/telnet_out_%d.txt", fd);
    snprintf(system_cmd, sizeof(system_cmd), "%s > %s 2>&1", cmd, outfile);

    ret = system(system_cmd);

    f = fopen(outfile, "r");
    if (f == NULL)
    {
        send_text(fd, "Khong mo duoc file ket qua lenh.\n> ");
        return;
    }

    if (ret != 0)
        send_text(fd, "Lenh tra ve ma loi khac 0.\n");

    while ((nread = fread(read_buf, 1, sizeof(read_buf), f)) > 0)
        send(fd, read_buf, nread, 0);

    fclose(f);
    remove(outfile);
    send_text(fd, "\n[END]\n> ");
}

static void process_line(Client clients[], int index, char *line, const char *db_file)
{
    Client *client = &clients[index];

    trim_line(line);

    if (client->state == WAIT_USERNAME)
    {
        if (strlen(line) == 0)
        {
            send_text(client->fd, "Username khong duoc rong.\nUsername: ");
            return;
        }

        snprintf(client->username, sizeof(client->username), "%s", line);
        client->state = WAIT_PASSWORD;
        send_text(client->fd, "Password: ");
        return;
    }

    if (client->state == WAIT_PASSWORD)
    {
        if (!check_account(db_file, client->username, line))
        {
            send_text(client->fd, "Dang nhap that bai. Thu lai.\nUsername: ");
            client->state = WAIT_USERNAME;
            client->username[0] = 0;
            return;
        }

        client->state = AUTHENTICATED;
        send_text(client->fd,
                  "Dang nhap thanh cong.\nNhap lenh he thong, go 'exit' de thoat.\n> ");
        printf("Client fd=%d dang nhap thanh cong voi user=%s\n",
               client->fd, client->username);
        return;
    }

    if (strcmp(line, "exit") == 0)
    {
        send_text(client->fd, "Bye.\n");
        remove_client(clients, index);
        return;
    }

    if (strlen(line) == 0)
    {
        send_text(client->fd, "> ");
        return;
    }

    send_command_output(client->fd, line);
}

int main(int argc, char *argv[])
{
    int listener;
    int opt;
    int port;
    const char *db_file;
    struct sockaddr_in addr;
    Client clients[MAX_CLIENTS];
    int i;

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s <port> [account_file]\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    db_file = (argc == 3) ? argv[2] : "src/bt_14_4/telnet_users.txt";

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket");
        return 1;
    }

    opt = 1;
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

    if (listen(listener, 5) == -1)
    {
        perror("listen");
        close(listener);
        return 1;
    }

    for (i = 0; i < MAX_CLIENTS; i++)
        reset_client(&clients[i]);

    printf("Telnet server listening on port %d...\n", port);
    printf("Account file: %s\n", db_file);

    while (1)
    {
        struct pollfd pfds[MAX_CLIENTS + 1];
        int slot_to_client[MAX_CLIENTS + 1];
        int nfds = 1;

        pfds[0].fd = listener;
        pfds[0].events = POLLIN;
        pfds[0].revents = 0;
        slot_to_client[0] = -1;

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].fd == -1)
                continue;

            pfds[nfds].fd = clients[i].fd;
            pfds[nfds].events = POLLIN;
            pfds[nfds].revents = 0;
            slot_to_client[nfds] = i;
            nfds++;
        }

        if (poll(pfds, nfds, -1) < 0)
        {
            perror("poll");
            break;
        }

        if (pfds[0].revents & POLLIN)
        {
            int client_fd = accept(listener, NULL, NULL);
            if (client_fd != -1)
            {
                int inserted = 0;

                for (i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i].fd == -1)
                    {
                        clients[i].fd = client_fd;
                        clients[i].state = WAIT_USERNAME;
                        clients[i].username[0] = 0;
                        clients[i].buffer_len = 0;
                        clients[i].buffer[0] = 0;
                        send_text(client_fd, "Username: ");
                        inserted = 1;
                        break;
                    }
                }

                if (!inserted)
                {
                    send_text(client_fd, "Server dang day.\n");
                    close(client_fd);
                }
            }
        }

        for (i = 1; i < nfds; i++)
        {
            int client_index;
            Client *client;

            if (!(pfds[i].revents & (POLLIN | POLLERR | POLLHUP)))
                continue;

            client_index = slot_to_client[i];
            client = &clients[client_index];

            {
                int received = recv(client->fd,
                                    client->buffer + client->buffer_len,
                                    sizeof(client->buffer) - client->buffer_len - 1,
                                    0);

                if (received <= 0)
                {
                    printf("Client fd=%d da ngat ket noi.\n", client->fd);
                    remove_client(clients, client_index);
                    continue;
                }

                client->buffer_len += received;
                client->buffer[client->buffer_len] = 0;
            }

            {
                char *line_start = client->buffer;
                char *newline;

                while ((newline = strchr(line_start, '\n')) != NULL)
                {
                    *newline = 0;
                    process_line(clients, client_index, line_start, db_file);
                    if (clients[client_index].fd == -1)
                        break;
                    line_start = newline + 1;
                }

                if (clients[client_index].fd == -1)
                    continue;

                {
                    int remaining = (int)strlen(line_start);
                    memmove(client->buffer, line_start, remaining);
                    client->buffer_len = remaining;
                    client->buffer[client->buffer_len] = 0;
                }
            }

            if (client->buffer_len == (int)sizeof(client->buffer) - 1)
            {
                send_text(client->fd, "Lenh qua dai.\n> ");
                client->buffer_len = 0;
                client->buffer[0] = 0;
            }
        }
    }

    close(listener);
    return 0;
}
