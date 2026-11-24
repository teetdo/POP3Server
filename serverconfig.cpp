#include "serverconfig.h"

ServerConfig::ServerConfig()
{
    // Cấu hình mặc định
    this->port = 110;
    this->mailRootPath = "Mailboxs";
    this->accountFilePath = "pop3account.conf";
}

ServerConfig::~ServerConfig()
{
}

unsigned short ServerConfig::getPort() const
{
    return this->port;
}

void ServerConfig::setPort(unsigned short p)
{
    this->port = p;
}

string ServerConfig::getMailRootPath() const
{
    return this->mailRootPath;
}

void ServerConfig::setMailRootPath(string path)
{
    this->mailRootPath = path;
}

const string& ServerConfig::getAccountFilePath() const
{
    return accountFilePath;
}

void ServerConfig::setAccountFilePath(const string& path)
{
    accountFilePath = path;
}


