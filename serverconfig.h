#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <string>

using namespace std;

class ServerConfig
{
protected:
    unsigned short port;
    string mailRootPath;

public:
    ServerConfig();
    virtual ~ServerConfig();

    unsigned short getPort() const;
    void setPort(unsigned short p);

    string getMailRootPath() const;
    void setMailRootPath(string path);
    const string& getAccountFilePath() const;
    void setAccountFilePath(const string& path);

protected:
    string accountFilePath;
};

#endif // SERVERCONFIG_H

