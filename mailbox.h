#ifndef MAILBOX_H
#define MAILBOX_H

#include <vector>
#include <string>
#include <fstream>  // Để đọc file
#include <sstream>  // Để xử lý chuỗi
#include <iostream>
#include <cstdio>   // Để dùng hàm remove()
using namespace std;

// Cấu trúc Message giữ nguyên
struct Message {
    string content;

    int getSize() const {
        return content.length();
    }
};

class Mailbox
{
private:
    // Dùng vector để thay thế cho List nhằm tối ưu truy cập index
    vector<Message> messages;

    // Khớp với UML: -deletedFlags : List<boolean>
    vector<bool> deletedFlags;
    // Biến để lưu đường dẫn thư mục hiện tại (ví dụ: "Mailboxs/tuan/")
    string currentFolderPath;
public:
    Mailbox();
    ~Mailbox();


    // Khôi phục trạng thái (Reset deleted flags)
    void getResMailbox();

    // Lấy tất cả tin nhắn
    vector<Message> getAll();

    // Lấy nội dung tin nhắn.
    // Lưu ý: Thêm tham số id để lấy đúng mail, nếu không có id thì không thể biết client muốn đọc mail nào.
    string getMailContent(int id);

    // Trả về danh sách kích thước các mail (List<int>)
    vector<int> getList();

    // Lấy kích thước của một mail cụ thể theo ID
    int getMessageInfo(int id);

    // Đánh dấu xóa một email theo ID
    void markDeleted(int n);

    // Kiểm tra mail đã bị xóa chưa
    bool isDeleted(int id);

    // --- Các hàm hỗ trợ (Helper) ---

    // Đọc file .eml từ thư mục Mailboxs/tuan
    void loadEmailsFromDisk(const string& username, const string& mailRootPath);

    //Hàm cập nhật thay đổi xuống ổ cứng (Xóa và Sắp xếp lại)
    bool updateMailbox();

    // Hàm hỗ trợ cho lệnh STAT (giữ lại để tương thích với pop3session.cpp)
    void getStat(int& count, int& totalSize);

    // Kiểm tra ID hợp lệ
    bool isValidId(int id);
};

#endif // MAILBOX_H
