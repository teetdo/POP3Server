#include <iostream>
#include <chrono>
#include <exception>
#include <thread>
#include "tcpserver.h"

// Định nghĩa này để đảm bảo server chạy đa luồng
#define _CONCURRENT_MT_SERVER_

using namespace std;

TCPServer::TCPServer(unsigned short localPort)
{
    this->port = localPort;
    running = false;
    this->numCmd = 0;
    this->conf = NULL; // Khởi tạo an toàn
    this->logging = false;
}

TCPServer::~TCPServer()
{
    stopRuntime();
    if(conf)
    {
        delete conf;
        conf = NULL;
    }
}

void TCPServer::addCmd(const string& cmdName, CMD_FUNC f)
{
    if (this->numCmd < SERVER_CMD_MAX_NUM)
    {
        this->cmdDoFunc[this->numCmd] = f;
        this->cmdNameList[this->numCmd] = cmdName;
        this->numCmd++;
    }
    else
    {
        cerr << "Error: Too many commands registered!" << endl;
    }
}

void TCPServer::doCmd(Session* session, uint8_t cmdId, char * cmd_argv[], int cmd_argc)
{
    // Nếu cmdId không hợp lệ hoặc là lệnh UNKNOWN
    if(cmdId >= this->numCmd || cmdId == SERVER_CMD_UNKNOWN)
        session->doUnknown(cmd_argv, cmd_argc);
    else
    {
        CMD_FUNC f = this->cmdDoFunc[cmdId];
        if(f != NULL && session != NULL)
            (session->*f)(cmd_argv, cmd_argc);  // Gọi hàm thành viên thông qua con trỏ
    }
}

bool TCPServer::start()
{
    if (running)
        return true;

    try
    {
        this->configServer();
        this->master.setListen(this->port);
        running = true;
        acceptThread = std::thread(&TCPServer::run, this);
        return true;
    }
    catch(const exception& e)
    {
        cerr << "Start Error: " << e.what() << endl;
        running = false;
        master.close();
        this->cleanServer();
        return false;
    }
}

void TCPServer::stop()
{
    stopRuntime();
    this->cleanServer();
}

void TCPServer::stopRuntime()
{
    running = false;
    {
        lock_guard<mutex> lock(clientsMutex);
        for (set<TcpSocket*>::iterator client = activeClients.begin();
             client != activeClients.end(); ++client)
        {
            int shutdownType = SHUTDOWN_BOTH;
            (*client)->shutdown(shutdownType);
        }
    }

    if (acceptThread.joinable())
        acceptThread.join();

    master.close();

    for (size_t i = 0; i < clientThreads.size(); ++i)
        if (clientThreads[i].thread.joinable())
            clientThreads[i].thread.join();
    clientThreads.clear();
}

bool TCPServer::restart()
{
    stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return start();
}

bool TCPServer::isRunning()
{
    return running;
}

unsigned short TCPServer::getPort() const
{
    return port;
}

// Vòng lặp chính của Server
void TCPServer::run()
{
    while(running)
    {
        reapFinishedClients();
        try
        {
            TcpSocket* slave = master.accept(1);
            if (!slave)
                continue;

            lock_guard<mutex> lock(clientsMutex);
            if (!running)
            {
                delete slave;
                break;
            }
            activeClients.insert(slave);
            try
            {
                clientThreads.push_back(ClientWorker());
                ClientWorker& worker = clientThreads.back();
                worker.finished.reset(new atomic<bool>(false));
                worker.thread = std::thread(&TCPServer::handleClient, this, slave,
                                            worker.finished);
            }
            catch (...)
            {
                if (!clientThreads.empty() && !clientThreads.back().thread.joinable())
                    clientThreads.pop_back();
                activeClients.erase(slave);
                delete slave;
                throw;
            }
        }
        catch(const SocketException& e)
        {
            if (!running)
                break;
            cerr << "Accept Error: " << e.what() << endl;
        }
        catch(const exception& e)
        {
            if (!running)
                break;
            cerr << "Client thread Error: " << e.what() << endl;
        }
    }
}

void TCPServer::reapFinishedClients()
{
    for (size_t i = 0; i < clientThreads.size();)
    {
        if (clientThreads[i].finished && clientThreads[i].finished->load())
        {
            if (clientThreads[i].thread.joinable())
                clientThreads[i].thread.join();
            clientThreads.erase(clientThreads.begin() + i);
        }
        else
            ++i;
    }
}

void TCPServer::handleClient(TcpSocket* client,
                             const std::shared_ptr<std::atomic<bool> >& finished)
{
    try
    {
        startNewSession(client);
    }
    catch(const exception& e)
    {
        cerr << "Session Error: " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "Session Error: unexpected exception" << endl;
    }

    lock_guard<mutex> lock(clientsMutex);
    activeClients.erase(client);
    delete client;
    finished->store(true);
}

