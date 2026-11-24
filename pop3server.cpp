#include "pop3server.h"
#include "pop3session.h" // Bắt buộc phải có để tạo Session
#include <iostream>

using namespace std;

POP3Server::POP3Server(const string& configFile) : TCPServer(110), configFile(configFile)
{
    this->conf = NULL;
}

POP3Server::~POP3Server()
{
    stop();
}

void POP3Server::setConfigFile(const string& path)
{
    if (isRunning())
        throw runtime_error("Cannot change config file while server is running");
    configFile = path;
}

void POP3Server::configServer()
{
    if (this->conf)
        delete this->conf;
    this->conf = new POP3ServerConfig(configFile);
    this->port = this->conf->getPort();
}

void POP3Server::initCmd()
{

}

void POP3Server::cleanServer()
{
    if (this->conf)
    {
        delete this->conf;
        this->conf = NULL;
    }
}

//kết nối mới (TCP Accepted)
void POP3Server::startNewSession(TcpSocket* slave)
{
    cout << "[INFO] New client connected from: " << slave->getRemoteAddress() << endl;
    POP3ServerConfig* pop3Conf = (POP3ServerConfig*)this->conf;
    POP3Session session(slave, pop3Conf);
    session.run();
    cout << "[INFO] Session closed." << endl;
}


