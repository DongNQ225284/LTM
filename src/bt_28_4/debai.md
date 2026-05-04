# Bài tập 28/4

## 1. Yêu cầu bài toán

Xây dựng chương trình server TCP thực hiện định tuyến dữ liệu theo mô hình publish/subscribe:

- Server lắng nghe tại cổng `9000`.
- Client gửi `SUB <topic>` để đăng ký nhận tin của chủ đề `topic`.
- Client gửi `UNSUB <topic>` để hủy đăng ký chủ đề `topic`.
- Client gửi `PUB <topic> <msg>` để phát thông điệp `msg` lên chủ đề `topic`.
- Server chuyển tiếp thông điệp tới tất cả client đang đăng ký chủ đề tương ứng.
- Một client có thể đăng ký đồng thời nhiều chủ đề khác nhau.

## 2. Biên dịch

```bash
gcc src/bt_28_4/pubsub_server.c -o build/pubsub_server
```

## 3. Chạy chương trình

Chạy server:

```bash
./build/pubsub_server
```

Chạy client:
```bash
`telnet 127.0.0.1 9000` 
```
