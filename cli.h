#ifndef CLI_H
#define CLI_H

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

// Các hằng số
#define CMD_BUF_LEN 1024
#define SPACE " \t\r\n"
#define CMD_MAX_ARG_NUM 10
#define MAX_CMD_NUM 50

#define CMD_QUIT_NAME "quit"
#define CMD_QUIT_ID 255
#define CMD_NOOP_ID 254
#define CMD_UNKNOWN_ID 253

class CmdLineInterface; // Khai báo trước class

// Định nghĩa kiểu con trỏ hàm thành viên để xử lý lệnh
typedef void (CmdLineInterface::*CLI_CMD_FUNC)(char* argv[], int argc);

class CmdLineInterface
{
protected:
    string cmdPrompt;  // Chuỗi hiển thị đầu dòng lệnh (ví dụ: "admin> ")
    bool bRunning;
    int numCommands;

    string cmdNameList[MAX_CMD_NUM];
    CLI_CMD_FUNC cmdDoFunc[MAX_CMD_NUM];// Mảng lưu các hàm xử lý tương ứng với tên lệnh

public:
    CmdLineInterface();
    virtual ~CmdLineInterface();

    virtual void run();

    void setCmdPrompt(const string prompt);
    void showCmdPrompt();
    void clearScreen();

protected:
    // Các hàm quản lý lệnh
    void addCmd(const string name, CLI_CMD_FUNC f);

    // Các bước xử lý
    void readCmd(char* sCmdBuf, int buflen);
    uint8_t parseCmd(char* sCmdBuf, char* cmd_argv[], int& cmd_argc);
    void doCmd(uint8_t id, char* cmd_argv[], int cmd_argc);

    // Các hàm ảo cần class con override hoặc dùng mặc định
    virtual void initCmd();
    virtual void doQuit();
    virtual void doUnknown();
};

#endif // CLI_H

