#ifndef POP3SESSION_H
#define POP3SESSION_H

#include "session.h"
#include "pop3serverconfig.h"
#include "mailbox.h" // [QUAN TRỌNG] Include file header của Mailbox
#include <memory>
#include <mutex>
#include <vector>
#include <string>

using namespace std;

class POP3Session : public Session
{
private:
    enum State {
        AUTHORIZATION,
        TRANSACTION,
        UPDATE
    };

    State state;
    string currentUser;
    bool isAuthenticated;
    shared_ptr<mutex> mailboxMutex;
    unique_lock<mutex> mailboxLock;
    string inputBuffer;

    Mailbox mailbox;

public:
    POP3Session(TcpSocket* sock, ServerConfig* conf);
    virtual ~POP3Session();

    void run() override;

private:
    void handleRequest(const string& request);
    bool receiveRequest(string& request);

    // Các hàm xử lý lệnh
    void handleUSER(const string& user);
    void handlePASS(const string& pass);
    void handleSTAT();
    void handleLIST(int arg);
    void handleRETR(int arg);
    void handleDELE(int arg);
    void handleQUIT();
    void handleRSET();
    void handleNOOP();

    void loadUserEmails();
};

#endif // POP3SESSION_H
