#include "pop3servercli.h"
#include <iostream>

POP3ServerCLI::POP3ServerCLI()
{
    setCmdPrompt("POP3-Admin> ");
    server = new POP3Server();
}

POP3ServerCLI::~POP3ServerCLI()
{
    if (server)
    {
        delete server;
    }
}

void POP3ServerCLI::initCmd()
{   //cmdDofunc
    addCmd("start", (CLI_CMD_FUNC)&POP3ServerCLI::doStart);
    addCmd("stop", (CLI_CMD_FUNC)&POP3ServerCLI::doStop);
    addCmd("restart", (CLI_CMD_FUNC)&POP3ServerCLI::doRestart);
    addCmd("status", (CLI_CMD_FUNC)&POP3ServerCLI::doStatus);
    addCmd("help", (CLI_CMD_FUNC)&POP3ServerCLI::doHelp);
}

void POP3ServerCLI::doStart(char* argv[], int argc)
{
    if (server->isRunning())
    {
        cout << "Server is already running." << endl;
    }
    else if (argc > 2)
    {
        cout << "Usage: start [config-file]" << endl;
    }
    else
    {
        if (argc == 2)
            server->setConfigFile(argv[1]);
        if (server->start())
            cout << "POP3 Server started successfully on port "
                 << server->getPort() << "." << endl;
        else
            cout << "Failed to start server." << endl;
    }
}

void POP3ServerCLI::doStop(char*[], int)
{
    if (server->isRunning())
    {
        server->stop();
        cout << "Server stopped." << endl;
    }
    else
    {
        cout << "Server is not running." << endl;
    }
}

void POP3ServerCLI::doRestart(char* argv[], int argc)
{
    if (argc > 2)
    {
        cout << "Usage: restart [config-file]" << endl;
        return;
    }
    cout << "Restarting server..." << endl;
    server->stop();
    if (argc == 2)
        server->setConfigFile(argv[1]);
    if (server->start())
        cout << "Server restarted successfully." << endl;
    else
        cout << "Failed to restart server." << endl;
}

void POP3ServerCLI::doStatus(char*[], int)
{
    if (server->isRunning())
        cout << "Status: [RUNNING] Listening on port " << server->getPort() << endl;
    else
        cout << "Status: [STOPPED]" << endl;
}

void POP3ServerCLI::doHelp(char*[], int)
{
    cout << "--- POP3 SERVER COMMANDS ---" << endl;
    cout << " start   : Start the POP3 Server" << endl;
    cout << " stop    : Stop the POP3 Server" << endl;
    cout << " restart : Restart the server" << endl;
    cout << " status  : Check server status" << endl;
    cout << " quit    : Exit program" << endl;
}

void POP3ServerCLI::doQuit()
{
    if (server->isRunning())
    {
        cout << "Stopping server before exit..." << endl;
        server->stop();
    }
    CmdLineInterface::doQuit();
    //đóng giao diện dòng lệnh một cách an toàn
}


