#ifndef POP3SERVERCONFIG_H
#define POP3SERVERCONFIG_H

#include "serverconfig.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>

using namespace std;

class POP3ServerConfig : public ServerConfig
{
private:
    map<string, string> accounts;
    map<string, shared_ptr<mutex> > mailboxMutexes;
    mutex mailboxMutexesLock;

public:
    explicit POP3ServerConfig(const string& configFile = "pop3.conf");
    ~POP3ServerConfig();

    bool checkUserExists(const string& username);
    bool verifyPassword(const string& username, const string& password);
    void loadAccounts();
    shared_ptr<mutex> getMailboxMutex(const string& username);

private:
    void loadConfig(const string& configFile);
};

#endif // POP3SERVERCONFIG_H


