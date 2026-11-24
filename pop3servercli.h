#ifndef POP3SERVERCLI_H
#define POP3SERVERCLI_H

#include "cli.h"
#include "pop3server.h"

class POP3ServerCLI : public CmdLineInterface
{
private:
    POP3Server* server;

public:
    POP3ServerCLI();
    ~POP3ServerCLI();

protected:
    void initCmd() override;

    // Các hàm xử lý lệnh cụ thể
    void doStart(char* argv[], int argc);
    void doStop(char* argv[], int argc);
    void doRestart(char* argv[], int argc);
    void doStatus(char* argv[], int argc);
    void doHelp(char* argv[], int argc);
    void doQuit() override;
};

#endif // POP3SERVERCLI_H


