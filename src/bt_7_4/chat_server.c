/*
Sử dụng hàm select()/poll(), viết chương trình
chat_server thực hiện các chức năng sau:
Nhận kết nối từ các client, và vào hỏi tên client cho
đến khi client gửi đúng cú pháp:
“client_id: client_name”
trong đó client_name là tên của client, xâu ký tự viết
liền.
Sau đó nhận dữ liệu từ một client và gửi dữ liệu đó
đến các client còn lại, ví dụ: client có id “abc” gửi “xin
chao” thì các client khác sẽ nhận được: “abc: xin chao”
hoặc có thể thêm thời gian vào trước ví dụ:
“2023/05/06 11:00:00PM abc: xin chao”


gcc src/bt_7_4/chat_server.c -o build/chat_server
./build/chat_server 9000
*/


#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAX_CLIENTS FD_SETSIZE
#define BUFFER_SIZE 4096

typedef struct
{
    int fd;
    int logged_in;
    char client_id[64];
    char client_name[64];
    char buffer[BUFFER_SIZE];
    int buffer_len;
} Client;

static void trim_line(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
    {
        s[len - 1] = 0;
        len--;
    }
}

static int has_space(const char *s)
{
    while (*s)
    {
        if (isspace((unsigned char)*s))
            return 1;
        s++;
    }
    return 0;
}

static void send_text(int fd, const char *text)
{
    send(fd, text, strlen(text), 0);
}

static void get_timestamp(char *out, size_t out_size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(out, out_size, "%Y/%m/%d %I:%M:%S%p", tm_info);
}

static void remove_client(Client clients[], int index)
{
    close(clients[index].fd);
    clients[index].fd = -1;
    clients[index].logged_in = 0;
    clients[index].buffer_len = 0;
    clients[index].client_id[0] = 0;
    clients[index].client_name[0] = 0;
}

static void broadcast_message(Client clients[], int sender_index, const char *message)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd == -1 || i == sender_index || !clients[i].logged_in)
            continue;

        send_text(clients[i].fd, message);
    }
}

static int register_client(Client *client, char *line)
{
    char *sep = strstr(line, ":");
    if (sep == NULL)
        return 0;

    *sep = 0;
    char *id = line;
    char *name = sep + 1;

    while (*name == ' ')
        name++;

    trim_line(id);
    trim_line(name);

    if (strlen(id) == 0 || strlen(name) == 0)
        return 0;

    if (has_space(id) || has_space(name))
        return 0;

    snprintf(client->client_id, sizeof(client->client_id), "%s", id);
    snprintf(client->client_name, sizeof(client->client_name), "%s", name);
    client->logged_in = 1;
    return 1;
}

static void process_line(Client clients[], int index, char *line)
{
    Client *client = &clients[index];

    if (!client->logged_in)
    {
        if (!register_client(client, line))
        {
            send_text(client->fd,
                      "Sai cu phap. Vui long gui: client_id: client_name\n");
            return;
        }

        char welcome[256];
        snprintf(welcome, sizeof(welcome),
                 "Dang ky thanh cong. Xin chao %s (%s)\n",
                 client->client_name, client->client_id);
        send_text(client->fd, welcome);

        printf("Client %d registered as %s (%s)\n",
               client->fd, client->client_name, client->client_id);
        return;
    }

    trim_line(line);
    if (strlen(line) == 0)
        return;

    char timestamp[64];
    char outgoing[BUFFER_SIZE + 128];
    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(outgoing, sizeof(outgoing), "%s %s: %s\n",
             timestamp, client->client_id, line);

    printf("%s", outgoing);
    broadcast_message(clients, index, outgoing);
}

int main(int argc, char *argv[])
{
    int port = 9000;
    if (argc == 2)
        port = atoi(argv[1]);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
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

    Client clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = -1;
        clients[i].logged_in = 0;
        clients[i].buffer_len = 0;
    }

    printf("Chat server listening on port %d...\n", port);

    while (1)
    {
        fd_set fdread;
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        int max_fd = listener;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].fd != -1)
            {
                FD_SET(clients[i].fd, &fdread);
                if (clients[i].fd > max_fd)
                    max_fd = clients[i].fd;
            }
        }

        if (select(max_fd + 1, &fdread, NULL, NULL, NULL) < 0)
        {
            perror("select");
            break;
        }

        if (FD_ISSET(listener, &fdread))
        {
            int client_fd = accept(listener, NULL, NULL);
            if (client_fd != -1)
            {
                int inserted = 0;
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i].fd == -1)
                    {
                        clients[i].fd = client_fd;
                        clients[i].logged_in = 0;
                        clients[i].buffer_len = 0;
                        clients[i].client_id[0] = 0;
                        clients[i].client_name[0] = 0;
                        send_text(client_fd,
                                  "Nhap thong tin dang ky theo mau: client_id: client_name\n");
                        inserted = 1;
                        break;
                    }
                }

                if (!inserted)
                {
                    send_text(client_fd, "Server dang day, vui long thu lai sau.\n");
                    close(client_fd);
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            Client *client = &clients[i];
            if (client->fd == -1 || !FD_ISSET(client->fd, &fdread))
                continue;

            int received = recv(client->fd,
                                client->buffer + client->buffer_len,
                                sizeof(client->buffer) - client->buffer_len - 1,
                                0);
            if (received <= 0)
            {
                printf("Client fd=%d disconnected.\n", client->fd);
                remove_client(clients, i);
                continue;
            }

            client->buffer_len += received;
            client->buffer[client->buffer_len] = 0;

            char *line_start = client->buffer;
            char *newline;
            while ((newline = strchr(line_start, '\n')) != NULL)
            {
                *newline = 0;
                process_line(clients, i, line_start);
                line_start = newline + 1;
            }

            int remaining = (int)strlen(line_start);
            memmove(client->buffer, line_start, remaining);
            client->buffer_len = remaining;
            client->buffer[client->buffer_len] = 0;

            if (client->buffer_len == (int)sizeof(client->buffer) - 1)
            {
                send_text(client->fd, "Du lieu qua dai, vui long gui ngan hon.\n");
                client->buffer_len = 0;
                client->buffer[0] = 0;
            }
        }
    }

    close(listener);
    return 0;
}

/*
gcc src/bt_7_4/chat_server.c -o build/chat_server
./build/chat_server 9000
*/
