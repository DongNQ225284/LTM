# Báo cáo bài tập 04.02 - FTP Client

## 1. Thông tin sinh viên

- Họ và tên: ........................................
- MSSV: ............................................
- Lớp: .............................................
- Môn học: Lập trình mạng

## 2. Yêu cầu bài toán

Sinh viên lập trình FTP client thực hiện các yêu cầu sau:

- Đăng nhập vào FTP server `lebavui.io.vn`.
- Tài khoản có dạng `user_mssv`.
- Mật khẩu có dạng `4 chữ số cuối MSSV + 2 chữ số ngày sinh`.
- Lấy danh sách file trên server.
- Tìm file có tên dạng `question_xxxxxx.txt`.
- Tải file question về máy.
- Tạo file `answer_xxxxxx.txt` tương ứng, nội dung là chuỗi đảo ngược của file question.
- Upload file answer lên server.

Ví dụ: sinh viên có MSSV `20241234`, sinh ngày `03` thì:

- Username: `user_20241234`
- Password: `123403`

## 3. Ý tưởng thực hiện

Chương trình sử dụng socket TCP để giao tiếp trực tiếp với FTP server qua cổng `21`.

Các bước chính:

1. Kết nối tới FTP server `lebavui.io.vn:21`.
2. Đọc phản hồi chào từ server.
3. Gửi lệnh `USER` và `PASS` để đăng nhập.
4. Gửi lệnh `TYPE I` để truyền dữ liệu dạng binary.
5. Gửi lệnh `PASV` để mở kết nối dữ liệu ở passive mode.
6. Gửi lệnh `LIST` để lấy danh sách file.
7. Tìm file có tên bắt đầu bằng `question_` và kết thúc bằng `.txt`.
8. Gửi lệnh `RETR` để tải file question.
9. Đảo ngược nội dung file vừa tải.
10. Tạo file answer tương ứng theo mẫu `answer_xxxxxx.txt`.
11. Gửi lệnh `STOR` để upload file answer lên server.
12. Gửi lệnh `QUIT` để kết thúc phiên FTP.

## 4. Môi trường chạy thử

- Hệ điều hành: Ubuntu/Linux
- Ngôn ngữ lập trình: C
- Trình biên dịch: `gcc`
- Server FTP: `lebavui.io.vn`

## 5. Lệnh biên dịch và chạy

Biên dịch:

```bash
mkdir -p build
gcc src/bt_8_6/ftp_client.c -o build/ftp_client
```

Chạy chương trình:

```bash
./build/ftp_client <mssv> <ngay_sinh_2_chu_so>
```

Ví dụ:

```bash
./build/ftp_client 20241234 03
```

Trong đó:

- `<mssv>` là mã số sinh viên thật.
- `<ngay_sinh_2_chu_so>` là ngày sinh viết đủ 2 chữ số, ví dụ `03`, `09`, `15`.

## 6. Kết quả chạy thử

Sau khi chạy thành công, chương trình hiển thị các lệnh FTP đã gửi và phản hồi từ server. Kết quả cuối cùng gồm:

- Tải được file `question_xxxxxx.txt`.
- Tạo được file `answer_xxxxxx.txt` trên máy.
- Upload thành công file `answer_xxxxxx.txt` lên FTP server.

Ví dụ kết quả cuối chương trình:

```text
Downloaded: question_xxxxxx.txt (100 bytes)
Uploaded:   answer_xxxxxx.txt
```

## 7. Ảnh chụp minh họa

### Ảnh 1. Biên dịch chương trình

Chèn ảnh chụp lệnh biên dịch tại đây:

```markdown
![Bien dich chuong trinh](images/bien_dich.png)
```

### Ảnh 2. Chạy chương trình bằng tài khoản sinh viên

Chèn ảnh chụp lệnh chạy chương trình tại đây:

```markdown
![Chay chuong trinh](images/chay_chuong_trinh.png)
```

### Ảnh 3. Kết quả tải file question và upload file answer

Chèn ảnh chụp kết quả chạy thành công tại đây:

```markdown
![Ket qua FTP client](images/ket_qua.png)
```

### Ảnh 4. File question và answer được tạo ở máy local

Chèn ảnh chụp thư mục có file `question_xxxxxx.txt` và `answer_xxxxxx.txt` tại đây:

```markdown
![File local](images/file_local.png)
```

## 8. Mã nguồn

File mã nguồn: `src/bt_8_6/ftp_client.c`

```c
#define _POSIX_C_SOURCE 200112L

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define FTP_PORT "21"
#define DEFAULT_HOST "lebavui.io.vn"
#define BUF_SIZE 8192

typedef struct {
    int code;
    char text[BUF_SIZE];
} FtpReply;

static void close_if_open(int *fd) {
    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

static int connect_tcp_host(const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *it;
    int sock = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo(%s:%s): %s\n", host, port, gai_strerror(rc));
        return -1;
    }

    for (it = res; it != NULL; it = it->ai_next) {
        sock = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (sock < 0) {
            continue;
        }
        if (connect(sock, it->ai_addr, it->ai_addrlen) == 0) {
            break;
        }
        close_if_open(&sock);
    }

    freeaddrinfo(res);
    return sock;
}

static int connect_tcp_ip(const char *ip, int port) {
    char port_text[16];
    snprintf(port_text, sizeof(port_text), "%d", port);
    return connect_tcp_host(ip, port_text);
}

static ssize_t recv_line(int sock, char *line, size_t size) {
    size_t used = 0;

    while (used + 1 < size) {
        char c;
        ssize_t n = recv(sock, &c, 1, 0);
        if (n <= 0) {
            return n;
        }
        line[used++] = c;
        if (c == '\n') {
            break;
        }
    }

    line[used] = '\0';
    return (ssize_t)used;
}

static int read_reply(int sock, FtpReply *reply) {
    char line[1024];
    char expected_end[5] = {0};
    int multiline = 0;

    reply->code = 0;
    reply->text[0] = '\0';

    while (1) {
        ssize_t n = recv_line(sock, line, sizeof(line));
        if (n <= 0) {
            fprintf(stderr, "FTP server closed control connection\n");
            return -1;
        }

        if (strlen(reply->text) + strlen(line) + 1 < sizeof(reply->text)) {
            strcat(reply->text, line);
        }

        if (reply->code == 0) {
            if (!isdigit((unsigned char)line[0]) ||
                !isdigit((unsigned char)line[1]) ||
                !isdigit((unsigned char)line[2])) {
                continue;
            }
            reply->code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + (line[2] - '0');
            if (line[3] == '-') {
                multiline = 1;
                snprintf(expected_end, sizeof(expected_end), "%03d ", reply->code);
            } else {
                break;
            }
        } else if (multiline && strncmp(line, expected_end, 4) == 0) {
            break;
        }
    }

    printf("<-- %s", reply->text);
    return reply->code;
}

static int send_all(int sock, const void *data, size_t len) {
    const char *p = (const char *)data;
    while (len > 0) {
        ssize_t n = send(sock, p, len, 0);
        if (n <= 0) {
            perror("send");
            return -1;
        }
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

static int ftp_command(int sock, FtpReply *reply, const char *fmt, ...) {
    char cmd[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, ap);
    va_end(ap);

    printf("--> %s\n", cmd);
    if (send_all(sock, cmd, strlen(cmd)) < 0 || send_all(sock, "\r\n", 2) < 0) {
        return -1;
    }
    return read_reply(sock, reply);
}

static int expect_positive(int code) {
    return code >= 200 && code < 400;
}

static int open_passive_data(int ctrl, FtpReply *reply) {
    int h1, h2, h3, h4, p1, p2;
    char ip[64];
    int port;

    int code = ftp_command(ctrl, reply, "PASV");
    if (code != 227) {
        fprintf(stderr, "PASV failed\n");
        return -1;
    }

    char *start = strchr(reply->text, '(');
    if (start == NULL ||
        sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        fprintf(stderr, "Cannot parse PASV reply: %s\n", reply->text);
        return -1;
    }

    snprintf(ip, sizeof(ip), "%d.%d.%d.%d", h1, h2, h3, h4);
    port = p1 * 256 + p2;

    int data = connect_tcp_ip(ip, port);
    if (data < 0) {
        perror("connect passive data");
    }
    return data;
}

static int read_all_from_socket(int sock, char *buf, size_t cap, size_t *out_len) {
    size_t used = 0;

    while (1) {
        if (used == cap) {
            fprintf(stderr, "Buffer too small for FTP data\n");
            return -1;
        }
        ssize_t n = recv(sock, buf + used, cap - used, 0);
        if (n < 0) {
            perror("recv data");
            return -1;
        }
        if (n == 0) {
            break;
        }
        used += (size_t)n;
    }

    *out_len = used;
    return 0;
}

static int ftp_list(int ctrl, FtpReply *reply, char *listing, size_t cap) {
    int data = open_passive_data(ctrl, reply);
    if (data < 0) {
        return -1;
    }

    int code = ftp_command(ctrl, reply, "LIST");
    if (code != 125 && code != 150) {
        close_if_open(&data);
        return -1;
    }

    size_t len = 0;
    if (read_all_from_socket(data, listing, cap - 1, &len) < 0) {
        close_if_open(&data);
        return -1;
    }
    listing[len] = '\0';
    close_if_open(&data);

    code = read_reply(ctrl, reply);
    return code == 226 ? 0 : -1;
}

static int is_question_name(const char *name) {
    const char *prefix = "question_";
    size_t len = strlen(name);
    size_t prefix_len = strlen(prefix);
    const char *suffix = ".txt";
    size_t suffix_len = strlen(suffix);

    return len > prefix_len + suffix_len &&
           strncmp(name, prefix, prefix_len) == 0 &&
           strcmp(name + len - suffix_len, suffix) == 0;
}

static int extract_question_name(const char *listing, char *out, size_t out_size) {
    char copy[BUF_SIZE];
    char *line;
    char *saveptr;

    snprintf(copy, sizeof(copy), "%s", listing);
    for (line = strtok_r(copy, "\r\n", &saveptr); line != NULL; line = strtok_r(NULL, "\r\n", &saveptr)) {
        char *last = strrchr(line, ' ');
        const char *candidate = last ? last + 1 : line;
        if (is_question_name(candidate)) {
            snprintf(out, out_size, "%s", candidate);
            return 0;
        }
    }

    return -1;
}

static int ftp_retr(int ctrl, FtpReply *reply, const char *remote_name, char *content, size_t cap, size_t *len) {
    int data = open_passive_data(ctrl, reply);
    if (data < 0) {
        return -1;
    }

    int code = ftp_command(ctrl, reply, "RETR %s", remote_name);
    if (code != 125 && code != 150) {
        close_if_open(&data);
        return -1;
    }

    if (read_all_from_socket(data, content, cap, len) < 0) {
        close_if_open(&data);
        return -1;
    }
    close_if_open(&data);

    code = read_reply(ctrl, reply);
    return code == 226 ? 0 : -1;
}

static int ftp_stor(int ctrl, FtpReply *reply, const char *remote_name, const char *content, size_t len) {
    int data = open_passive_data(ctrl, reply);
    if (data < 0) {
        return -1;
    }

    int code = ftp_command(ctrl, reply, "STOR %s", remote_name);
    if (code != 125 && code != 150) {
        close_if_open(&data);
        return -1;
    }

    if (send_all(data, content, len) < 0) {
        close_if_open(&data);
        return -1;
    }
    shutdown(data, SHUT_WR);
    close_if_open(&data);

    code = read_reply(ctrl, reply);
    return code == 226 ? 0 : -1;
}

static void reverse_copy(char *dst, const char *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        dst[i] = src[len - 1 - i];
    }
}

static void make_answer_name(const char *question, char *answer, size_t answer_size) {
    snprintf(answer, answer_size, "answer_%s", question + strlen("question_"));
}

static int write_local_file(const char *name, const char *data, size_t len) {
    FILE *f = fopen(name, "wb");
    if (f == NULL) {
        perror(name);
        return -1;
    }
    if (fwrite(data, 1, len, f) != len) {
        perror("fwrite");
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *host = DEFAULT_HOST;
    char username[128];
    char password[32];
    char listing[BUF_SIZE];
    char question_name[256];
    char answer_name[256];
    char question[BUF_SIZE];
    char answer[BUF_SIZE];
    size_t question_len = 0;
    FtpReply reply;
    int ctrl = -1;

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s <mssv> <ngay_sinh_2_chu_so> [host]\n", argv[0]);
        fprintf(stderr, "Example: %s 20241234 03\n", argv[0]);
        return 1;
    }

    if (argc == 4) {
        host = argv[3];
    }

    size_t mssv_len = strlen(argv[1]);
    if (mssv_len < 4 || strlen(argv[2]) != 2) {
        fprintf(stderr, "MSSV phai co it nhat 4 ky tu, ngay sinh phai co 2 chu so, vi du 03\n");
        return 1;
    }

    snprintf(username, sizeof(username), "user_%s", argv[1]);
    snprintf(password, sizeof(password), "%.4s%s", argv[1] + mssv_len - 4, argv[2]);

    ctrl = connect_tcp_host(host, FTP_PORT);
    if (ctrl < 0) {
        fprintf(stderr, "Cannot connect to FTP server %s:%s\n", host, FTP_PORT);
        return 1;
    }

    if (read_reply(ctrl, &reply) < 0 ||
        ftp_command(ctrl, &reply, "USER %s", username) != 331 ||
        !expect_positive(ftp_command(ctrl, &reply, "PASS %s", password)) ||
        !expect_positive(ftp_command(ctrl, &reply, "TYPE I"))) {
        close_if_open(&ctrl);
        return 1;
    }

    if (ftp_list(ctrl, &reply, listing, sizeof(listing)) < 0) {
        fprintf(stderr, "Cannot list remote directory\n");
        close_if_open(&ctrl);
        return 1;
    }

    printf("\nRemote listing:\n%s\n", listing);
    if (extract_question_name(listing, question_name, sizeof(question_name)) < 0) {
        fprintf(stderr, "Cannot find question_*.txt in remote listing\n");
        close_if_open(&ctrl);
        return 1;
    }

    if (ftp_retr(ctrl, &reply, question_name, question, sizeof(question), &question_len) < 0) {
        fprintf(stderr, "Cannot download %s\n", question_name);
        close_if_open(&ctrl);
        return 1;
    }
    if (write_local_file(question_name, question, question_len) < 0) {
        close_if_open(&ctrl);
        return 1;
    }

    reverse_copy(answer, question, question_len);
    make_answer_name(question_name, answer_name, sizeof(answer_name));
    if (write_local_file(answer_name, answer, question_len) < 0) {
        close_if_open(&ctrl);
        return 1;
    }

    if (ftp_stor(ctrl, &reply, answer_name, answer, question_len) < 0) {
        fprintf(stderr, "Cannot upload %s\n", answer_name);
        close_if_open(&ctrl);
        return 1;
    }

    ftp_command(ctrl, &reply, "QUIT");
    close_if_open(&ctrl);

    printf("\nDownloaded: %s (%zu bytes)\n", question_name, question_len);
    printf("Uploaded:   %s\n", answer_name);
    return 0;
}
```

## 9. Kết luận

Chương trình đã thực hiện đúng các yêu cầu của đề bài:

- Kết nối và đăng nhập FTP server bằng tài khoản sinh viên.
- Lấy danh sách file trên server.
- Tải file `question_xxxxxx.txt`.
- Tạo nội dung đảo ngược và lưu vào file `answer_xxxxxx.txt`.
- Upload file answer lên server.

