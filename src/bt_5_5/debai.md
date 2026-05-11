# Bài tập 5/5

## 1. Nội dung đã thực hiện

### 1.1. Telnet server dùng multiprocessing

File cài đặt: `src/bt_5_5/telnet_server_mp.c`

Đặc điểm:

- Tiến trình cha tạo socket lắng nghe TCP.
- Tiến trình cha dùng `select()` để chờ kết nối mới.
- Mỗi khi có client kết nối, tiến trình cha `fork()` ra một tiến trình con để phục vụ riêng client đó.
- Client phải nhập `Username` và `Password`.
- Server kiểm tra tài khoản trong file text `src/bt_5_5/telnet_users.txt`.
- Nếu đăng nhập đúng, client có thể gửi lệnh hệ thống.
- Server thực thi lệnh bằng `system()`, ghi kết quả vào file tạm trong `/tmp`, đọc lại và gửi về client.
- Gõ `exit` để đóng phiên làm việc.

### 1.2. HTTP server dùng preforking

File cài đặt: `src/bt_5_5/http_server_prefork.c`

Đặc điểm:

- Tiến trình cha tạo socket lắng nghe TCP.
- Ngay khi khởi động, tiến trình cha `fork()` sẵn một nhóm worker.
- Các worker cùng chờ `accept()` trên cùng socket lắng nghe.
- Khi có request mới, một worker nhận kết nối, đọc request HTTP và trả về trang HTML đơn giản.
- Tiến trình cha dùng `wait()` để theo dõi worker; nếu worker chết bất thường thì tạo lại worker mới.

## 2. Biên dịch

```bash
gcc src/bt_5_5/telnet_server_mp.c -o build/telnet_server_mp
gcc src/bt_5_5/http_server_prefork.c -o build/http_server_prefork
```

## 3. Chạy chương trình

### 3.1. Telnet server

Chạy server:

```bash
./build/telnet_server_mp 9001 src/bt_5_5/telnet_users.txt
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

Sau khi đăng nhập thành công có thể thử:

```text
pwd
ls src
exit
```

### 3.2. HTTP server

Chạy server:

```bash
./build/http_server_prefork 8080 4
```

Trong đó:

- `8080` là cổng lắng nghe.
- `4` là số worker prefork.

Test nhanh:

```bash
curl http://127.0.0.1:8080/
```

## 4. Kết quả kiểm thử nhanh

### 4.1. Telnet server

- Đăng nhập sai: server báo `Dang nhap that bai. Thu lai.`
- Đăng nhập đúng: server cho phép nhập lệnh hệ thống.
- Chạy lệnh `pwd`: server trả về thư mục làm việc hiện tại.
- Gõ `exit`: server đóng phiên.

### 4.2. HTTP server

- `curl http://127.0.0.1:8080/` nhận được phản hồi `HTTP/1.1 200 OK`.
- Nội dung HTML trả về có tiêu đề `Xin chao cac ban`.

## 5. Ghi chú

- Bài telnet này sử dụng mô hình `1 client = 1 process con`, nên nhiều client có thể làm việc song song.
- Bài HTTP dùng prefork đúng yêu cầu: tiến trình cha tạo sẵn worker và quản lý vòng đời của worker.
