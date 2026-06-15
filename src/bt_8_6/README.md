# Bài tập 04.02 - FTP Client

Chương trình `ftp_client.c` thực hiện:

- Đăng nhập FTP server `lebavui.io.vn`.
- Lấy danh sách file trên server.
- Tìm file `question_xxxxxx.txt`.
- Tải file question về máy.
- Tạo file `answer_xxxxxx.txt` có nội dung đảo ngược.
- Upload file answer lên server.

Biên dịch:

```bash
mkdir -p build
gcc src/bt_8_6/ftp_client.c -o build/ftp_client
```

Chạy:

```bash
./build/ftp_client <mssv> <ngay_sinh_2_chu_so>
```

Ví dụ sinh viên MSSV `20241234`, sinh ngày `03`:

```bash
./build/ftp_client 20241234 03
```

Chương trình tự tạo:

- Username: `user_<mssv>`
- Password: `4 chữ số cuối MSSV + ngày sinh 2 chữ số`

Ví dụ trên tạo username `user_20241234`, password `123403`.

Nếu cần test với host FTP khác:

```bash
./build/ftp_client 20241234 03 <ftp_host>
```
