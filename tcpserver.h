#ifndef SERVER_H__
#define SERVER_H__

// Include các thư viện chuẩn cần thiết
#include <string>
#include <iostream>
#include <vector>
#include <cstdint>
#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

// Include các file header trong project
#include "tcpserversocket.h"
#include "session.h"
#include "serverconfig.h" // [QUAN TRỌNG] Phải include để nhận diện lớp ServerConfig

using namespace std;

// Định nghĩa số lượng lệnh tối đa server hỗ trợ
#ifndef SERVER_CMD_MAX_NUM
#define SERVER_CMD_MAX_NUM 64
#endif

#define SERVER_CMD_UNKNOWN 255

class TCPServer
{
protected:
    ServerConfig* conf;         // Cấu hình server
    TcpServerSocket master;     // Socket lắng nghe (Master socket)
    std::atomic<bool> running;  // Cờ trạng thái chạy
    bool logging;               // Cờ ghi log
    unsigned short port;        // Cổng lắng nghe

    // Quản lý danh sách lệnh (Dùng cho kiến trúc Server quản lý lệnh tập trung)
    unsigned short numCmd;
    string  cmdNameList[SERVER_CMD_MAX_NUM];
    CMD_FUNC cmdDoFunc[SERVER_CMD_MAX_NUM]; // CMD_FUNC phải được define trong session.h

    string logFileName;
    std::thread acceptThread;
    struct ClientWorker
    {
        std::thread thread;
        std::shared_ptr<std::atomic<bool> > finished;
    };
    std::vector<ClientWorker> clientThreads;
    std::mutex clientsMutex;
    std::set<TcpSocket*> activeClients;

public:
    TCPServer(unsigned short localPort);
    virtual ~TCPServer();

    bool start();
    void stop();
    bool restart();
    bool isRunning();
    unsigned short getPort() const;

    bool getLogging()
    {
        return logging;
    }

    const string& getLogFileName() const
    {
        return logFileName;
    }

protected:
    // Vòng lặp chính xử lý kết nối
    void run();
    void handleClient(TcpSocket* client, const std::shared_ptr<std::atomic<bool> >& finished);
    void reapFinishedClients();
    void stopRuntime();

    // Các hàm thuần ảo (Pure Virtual) - Lớp con (POP3Server) BẮT BUỘC phải viết lại
    virtual void startNewSession(TcpSocket* slave) = 0;
    virtual void configServer() = 0;
    virtual void cleanServer() = 0;
    virtual void initCmd() = 0;

    // Các hàm quản lý lệnh
    void addCmd(const string& cmdName, CMD_FUNC f);
    void doCmd(Session* session, uint8_t cmdId, char* cmdArgv[], int cmdArgc);
};

#endif // SERVER_H_INCLUDED


