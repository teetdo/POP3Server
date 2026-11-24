#ifndef SESSION_H
#define SESSION_H

#include <iostream>
#include <string>
#include <vector>
#include "tcpsocket.h"
#include "serverconfig.h"

using namespace std;

class Session; // Khai báo trước (Forward declaration)

// Định nghĩa kiểu con trỏ hàm thành viên xử lý lệnh cho Session.
// TCPServer sử dụng kiểu này để quản lý danh sách lệnh chung.
typedef void (Session::*CMD_FUNC)(char* argv[], int argc);

class Session
{
protected:
    TcpSocket* m_client;    // Socket kết nối với client
    ServerConfig* m_config; // Cấu hình server

public:
    // Constructor nhận vào socket client và cấu hình
    Session(TcpSocket* client, ServerConfig* config);

    // Destructor ảo để đảm bảo cleanup đúng khi delete qua con trỏ lớp cha
    virtual ~Session();

    // Hàm thuần ảo: Logic chính của phiên làm việc (POP3, SMTP, HTTP...) sẽ được viết ở đây
    virtual void run() = 0;

    // Xử lý khi nhận lệnh không xác định (được gọi từ TCPServer::doCmd)
    virtual void doUnknown(char* argv[], int argc);

    // Lấy địa chỉ IP của client
    string getClientIP() const;

protected:
    // Hàm gửi thông báo về client
    void sendResponse(const string& msg);
};

#endif // SESSION_H
