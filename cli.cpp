#include "cli.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

CmdLineInterface::CmdLineInterface()
{
    numCommands = 0;
    cmdPrompt = "CLI> ";
    bRunning = false;
}

CmdLineInterface::~CmdLineInterface()
{
}

void CmdLineInterface::setCmdPrompt(const string prompt)
{
    this->cmdPrompt = prompt;
}

void CmdLineInterface::showCmdPrompt()
{
    cout << cmdPrompt;
    cout.flush();
}

void CmdLineInterface::clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
//Quản lý danh sách lệnh
void CmdLineInterface::addCmd(const string name, CLI_CMD_FUNC f)
{
    if (numCommands < MAX_CMD_NUM)
    {
        cmdNameList[numCommands] = name;
        cmdDoFunc[numCommands] = f;
        numCommands++;
    }
}

void CmdLineInterface::initCmd()
{
    // Class con sẽ thực hiện
}
// Đọc dữ liệu từ bàn phím
void CmdLineInterface::readCmd(char* sCmdBuf, int buflen)
{
    if (fgets(sCmdBuf, buflen, stdin) == NULL)
    {
        sCmdBuf[0] = '\0';
        return;
    }

    // Xóa ký tự xuống dòng ở cuối
    size_t len = strlen(sCmdBuf);
    while (len > 0 && (sCmdBuf[len-1] == '\n' || sCmdBuf[len-1] == '\r'))
    {
        sCmdBuf[len-1] = '\0';
        len--;
    }
}

uint8_t CmdLineInterface::parseCmd(char* sCmdBuf, char* cmd_argv[], int& cmd_argc)
{
    cmd_argc = 0;
    char* token = strtok(sCmdBuf, SPACE);

    while (token != NULL && cmd_argc < CMD_MAX_ARG_NUM)
    {
        cmd_argv[cmd_argc++] = token;
        token = strtok(NULL, SPACE);
    }

    if (cmd_argc == 0) return CMD_NOOP_ID;

    // Tìm lệnh trong danh sách
    for (int i = 0; i < numCommands; i++)
    {
        if (cmdNameList[i] == cmd_argv[0]) return i;
    }

    if (string(cmd_argv[0]) == CMD_QUIT_NAME) return CMD_QUIT_ID;

    return CMD_UNKNOWN_ID;
}

void CmdLineInterface::doCmd(uint8_t id, char* cmd_argv[], int cmd_argc)
{
    if (id == CMD_NOOP_ID) return;
    if (id == CMD_UNKNOWN_ID) { doUnknown(); return; }
    if (id == CMD_QUIT_ID) { doQuit(); return; }

    // Gọi hàm tương ứng
    //gọi một hàm thành viên thông qua con trỏ hàm
    if (cmdDoFunc[id])
    {
        (this->*cmdDoFunc[id])(cmd_argv, cmd_argc);
    }
}

void CmdLineInterface::doUnknown()
{
    cout << "Unknown command. Type 'help' for list of commands." << endl;
}

void CmdLineInterface::doQuit()
{
    bRunning = false;
    cout << "Exiting CLI..." << endl;
}

void CmdLineInterface::run()
{
    char cmdBuf[CMD_BUF_LEN];
    char* args[CMD_MAX_ARG_NUM];
    int argCount;

    initCmd();
    bRunning = true;

    while (bRunning)
    {
        showCmdPrompt();
        readCmd(cmdBuf, CMD_BUF_LEN);
        if (feof(stdin))
        {
            bRunning = false;
            break;
        }
        uint8_t id = parseCmd(cmdBuf, args, argCount);
        doCmd(id, args, argCount);
    }
}

