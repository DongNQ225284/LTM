# Bài tập 14/4


## 1. Chat server

Biên dịch:

```bash
gcc src/bt_14_4/chat_server.c -o build/chat_server_14_4
```

Chạy:

```bash
./build/chat_server_14_4 9000
```

Kết nối test bằng `telnet` hoặc `nc`:

```bash
telnet 127.0.0.1 9000
```

Client phải đăng ký đúng mẫu:

```text
abc: nguyenvana
```

Sau đó tin nhắn sẽ được phát tới các client còn lại theo dạng:

```text
2026/04/20 09:30:00PM abc: xin chao
```

## 2. Telnet Server

Biên dịch:

```bash
gcc src/bt_14_4/telnet_server.c -o build/telnet_server_14_4
```

Chạy:

```bash
./build/telnet_server_14_4 9001 src/bt_14_4/telnet_users.txt
```

Kết nối test:

```bash
telnet 127.0.0.1 9001
```

Tài khoản mẫu:

```text
admin admin
guest nopass
sv1 123456
```

Sau khi đăng nhập thành công, server nhận lệnh hệ thống, thực thi bằng `system()` và trả nội dung về client. Gõ `exit` để đóng phiên làm việc.
