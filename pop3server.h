#ifndef POP3SERVER_H
#define POP3SERVER_H

#include "tcpserver.h"
#include "pop3serverconfig.h"
#include <string>

class POP3Server : public TCPServer
{
public:
    explicit POP3Server(const string& configFile = "pop3.conf");
    virtual ~POP3Server();
    void setConfigFile(const string& configFile);

protected:
    // Thiết lập các thông số ban đầu cho Server trước khi chạy
    void configServer() override;

    // Dọn dẹp tài nguyên (bộ nhớ, file, socket) khi Server dừng
    void cleanServer() override;

    // Khởi tạo danh sách các lệnh của giao thức POP3
    void initCmd() override;

    // Được gọi mỗi khi có một Client (người dùng) mới kết nối tới
    void startNewSession(TcpSocket* slave) override;

private:
    string configFile;
};

#endif // POP3SERVER_H


