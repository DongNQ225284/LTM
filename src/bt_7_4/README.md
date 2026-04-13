# Bài tập 7/4

## 1. Chat Server

Biên dịch:

```bash
gcc src/bt_7_4/chat_server.c -o build/chat_server
```

Chạy:

```bash
./build/chat_server 9000
```

Test nhanh bằng `telnet` hoặc `nc` từ nhiều terminal:

```bash
telnet 127.0.0.1 9000
```

Client phải đăng ký đúng mẫu:

```text
abc: nguyenvana
```

Sau đó mỗi tin nhắn từ client `abc` sẽ được phát cho các client khác theo dạng:

```text
2026/04/13 09:30:00PM abc: xin chao
```

## 2. Telnet Server

Biên dịch:

```bash
gcc src/bt_7_4/telnet_server.c -o build/telnet_server
```

Chạy:

```bash
./build/telnet_server 9001 src/bt_7_4/telnet_users.txt
```

Kết nối test:

```bash
telnet 127.0.0.1 9001
```

Tài khoản mẫu nằm trong `src/bt_7_4/telnet_users.txt`.

Sau khi đăng nhập thành công, server nhận lệnh hệ thống, thực thi bằng `system()` và trả nội dung kết quả về cho client. Gõ `exit` để đóng phiên làm việc.
