#include "pop3session.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cctype>
#include <cstdlib>

#define BUF_SIZE 1024

POP3Session::POP3Session(TcpSocket* sock, ServerConfig* conf) : Session(sock, conf)
{
    this->state = AUTHORIZATION;
    this->isAuthenticated = false;
    this->currentUser = "";
    // Mailbox tự động load dữ liệu mẫu trong constructor của nó
}

POP3Session::~POP3Session()
{
}

void POP3Session::run()
{
    sendResponse("+OK POP3 Server Ready\r\n");

    bool running = true;

    while (running && m_client)
    {
        string request;
        if (!receiveRequest(request))
            break;

        if (request.empty()) continue;

        string command = request.substr(0, request.find_first_of(" \t"));
        transform(command.begin(), command.end(), command.begin(),
                  [](unsigned char ch) { return static_cast<char>(toupper(ch)); });
        if (command == "QUIT")
        {
            string::size_type argumentStart = request.find_first_not_of(" \t", command.size());
            if (argumentStart == string::npos)
            {
                handleQUIT();
                running = false;
            }
            else
                sendResponse("-ERR QUIT takes no arguments\r\n");
        }
        else
            handleRequest(request);
    }
}

bool POP3Session::receiveRequest(string& request)
{
    while (true)
    {
        string::size_type lineEnd = inputBuffer.find("\r\n");
        if (lineEnd != string::npos)
        {
            request = inputBuffer.substr(0, lineEnd);
            inputBuffer.erase(0, lineEnd + 2);
            return true;
        }

        if (inputBuffer.size() >= BUF_SIZE)
        {
            sendResponse("-ERR Command line too long\r\n");
            return false;
        }

        char buffer[BUF_SIZE];
        int bytes;
        try
        {
            bytes = m_client->recv(buffer, BUF_SIZE, 1);
        }
        catch (const SocketException&)
        {
            if (m_client->isShutdownRequested())
                return false;
            throw;
        }
        if (bytes == -2)
        {
            if (m_client->isShutdownRequested())
                return false;
            continue;
        }
        if (bytes <= 0)
            return false;
        inputBuffer.append(buffer, bytes);
    }
}

void POP3Session::handleRequest(const string& request)
{
    stringstream ss(request);
    string cmd, argStr;
    ss >> cmd >> argStr;
    transform(cmd.begin(), cmd.end(), cmd.begin(),
              [](unsigned char ch) { return static_cast<char>(toupper(ch)); });

    string extra;
    ss >> extra;
    if (cmd == "USER") {
        if (argStr.empty() || !extra.empty()) sendResponse("-ERR USER requires one argument\r\n");
        else handleUSER(argStr);
    }
    else if (cmd == "PASS") {
        if (argStr.empty() || !extra.empty()) sendResponse("-ERR PASS requires one argument\r\n");
        else handlePASS(argStr);
    }
    else if (cmd == "STAT") {
        if (!argStr.empty()) sendResponse("-ERR STAT takes no arguments\r\n");
        else handleSTAT();
    }
    else if (cmd == "LIST") {
        int msgId = 0;
        if (argStr.empty()) handleLIST(msgId);
        else {
            char* end = NULL;
            long parsed = strtol(argStr.c_str(), &end, 10);
            if (!extra.empty() || !end || *end != '\0' || parsed <= 0 ||
                parsed > 2147483647L)
                sendResponse("-ERR Invalid message number\r\n");
            else handleLIST(static_cast<int>(parsed));
        }
    }
    else if (cmd == "RETR") {
        char* end = NULL;
        long parsed = strtol(argStr.c_str(), &end, 10);
        if (argStr.empty() || !extra.empty() || !end || *end != '\0' ||
            parsed <= 0 || parsed > 2147483647L)
            sendResponse("-ERR Invalid message number\r\n");
        else handleRETR(static_cast<int>(parsed));
    }
    else if (cmd == "DELE") {
        char* end = NULL;
        long parsed = strtol(argStr.c_str(), &end, 10);
        if (argStr.empty() || !extra.empty() || !end || *end != '\0' ||
            parsed <= 0 || parsed > 2147483647L)
            sendResponse("-ERR Invalid message number\r\n");
        else handleDELE(static_cast<int>(parsed));
    }
    else if (cmd == "RSET") {
        if (!argStr.empty()) sendResponse("-ERR RSET takes no arguments\r\n");
        else handleRSET();
    }
    else if (cmd == "NOOP") {
        if (!argStr.empty()) sendResponse("-ERR NOOP takes no arguments\r\n");
        else handleNOOP();
    }
    else sendResponse("-ERR Unknown command\r\n");
}

void POP3Session::handleUSER(const string& user)
{
    if (state != AUTHORIZATION) {
        sendResponse("-ERR Command not valid in this state\r\n");
        return;
    }
    POP3ServerConfig* pop3Conf = (POP3ServerConfig*)m_config;
    if (pop3Conf->checkUserExists(user)) {
        currentUser = user;
        sendResponse("+OK User accepted, send PASS\r\n");
    } else {
        sendResponse("-ERR User not found\r\n");
    }
}

void POP3Session::handlePASS(const string& pass)
{
    if (state != AUTHORIZATION) {
        sendResponse("-ERR Command not valid in this state\r\n");
        return;
    }
    if (currentUser.empty()) {
        sendResponse("-ERR Send USER first\r\n");
        return;
    }
    POP3ServerConfig* pop3Conf = (POP3ServerConfig*)m_config;
    if (pop3Conf->verifyPassword(currentUser, pass)) {
        mailboxMutex = pop3Conf->getMailboxMutex(currentUser);
        mailboxLock = unique_lock<mutex>(*mailboxMutex);
        isAuthenticated = true;
        state = TRANSACTION;
        loadUserEmails();
        sendResponse("+OK Logged in\r\n");
    } else {
        sendResponse("-ERR Authentication failed\r\n");
    }
}

// --- CẬP NHẬT LOGIC DÙNG CLASS MAILBOX ---

void POP3Session::handleSTAT()
{
    if (state != TRANSACTION) {
        sendResponse("-ERR Login first\r\n");
        return;
    }

    int count = 0;
    int totalSize = 0;
    // Gọi hàm getStat từ đối tượng mailbox
    mailbox.getStat(count, totalSize);

    stringstream ss;
    ss << "+OK " << count << " " << totalSize << "\r\n";
    sendResponse(ss.str());
}

void POP3Session::handleLIST(int arg)
{
    // Kiểm tra trạng thái đăng nhập
    if (state != TRANSACTION) {
        sendResponse("-ERR Login first\r\n");
        return;
    }

    // TRƯỜNG HỢP 1: LIST n (Xem thông tin 1 mail cụ thể)
    if (arg > 0) {
        int size = mailbox.getMessageInfo(arg);
        if (size >= 0) {
            if (mailbox.isDeleted(arg)) {
                 sendResponse("-ERR Message deleted\r\n");
            } else {
                stringstream ss;
                ss << "+OK " << arg << " " << size << "\r\n";
                sendResponse(ss.str());
            }
        }
        else {
            sendResponse("-ERR No such message\r\n");
        }
    }
    // TRƯỜNG HỢP 2: LIST (Lấy danh sách tất cả mail)
    else {
        // 1. Lấy thống kê số lượng và tổng dung lượng mail chưa xóa
        int count = 0;
        int totalSize = 0;
        mailbox.getStat(count, totalSize); // Hàm này đã có trong Mailbox.h bạn gửi

        // 2. Gửi dòng phản hồi đầu tiên
        stringstream ssHeader;
        ssHeader << "+OK " << count << " messages (" << totalSize << " octets)\r\n";
        sendResponse(ssHeader.str());

        // 3. Duyệt qua tất cả mail để liệt kê
        vector<Message> allMsgs = mailbox.getAll();
        for (size_t i = 0; i < allMsgs.size(); ++i) {
            int currentId = i + 1; // ID trong POP3 bắt đầu từ 1

            // Chỉ liệt kê những mail CHƯA bị đánh dấu xóa
            if (!mailbox.isDeleted(currentId)) {
                stringstream ssLine;
                ssLine << currentId << " " << allMsgs[i].getSize() << "\r\n";
                sendResponse(ssLine.str());
            }
        }

        // 4. Gửi dấu chấm kết thúc danh sách
        sendResponse(".\r\n");
    }
}

void POP3Session::handleRETR(int arg)
{
    if (state != TRANSACTION) {
        sendResponse("-ERR Login first\r\n");
        return;
    }

    if (mailbox.isValidId(arg)) {
        if (mailbox.isDeleted(arg)) {
            sendResponse("-ERR Message deleted\r\n");
        } else {
            string content = mailbox.getMailContent(arg);
            stringstream ss;
            ss << "+OK " << content.length() << " octets\r\n";
            string::size_type start = 0;
            while (start < content.size()) {
                string::size_type end = content.find('\n', start);
                string::size_type lineEnd = end == string::npos ? content.size() : end + 1;
                if (content[start] == '.')
                    ss << '.';
                ss << content.substr(start, lineEnd - start);
                start = lineEnd;
            }
            if (content.empty() || content[content.size() - 1] != '\n')
                ss << "\r\n";
            ss << ".\r\n";
            sendResponse(ss.str());
        }
    } else {
        sendResponse("-ERR Invalid message ID\r\n");
    }
}

void POP3Session::handleDELE(int arg)
{
    if (state != TRANSACTION) {
        sendResponse("-ERR Login first\r\n");
        return;
    }

    if (mailbox.isValidId(arg)) {
        if (mailbox.isDeleted(arg)) {
            sendResponse("-ERR Message already deleted\r\n");
        } else {
            mailbox.markDeleted(arg);
            sendResponse("+OK Message deleted\r\n");
        }
    } else {
        sendResponse("-ERR Invalid message ID\r\n");
    }
}

void POP3Session::handleRSET()
{
    if (state != TRANSACTION) {
        sendResponse("-ERR Login first\r\n");
        return;
    }
    mailbox.getResMailbox(); // Gọi hàm reset của mailbox
    sendResponse("+OK Mailbox reset\r\n");
}

void POP3Session::handleNOOP()
{
    sendResponse("+OK\r\n");
}

void POP3Session::handleQUIT()
{
    if (state == TRANSACTION) {
        state = UPDATE;
        if (mailbox.updateMailbox())
            sendResponse("+OK POP3 server signing off\r\n");
        else
            sendResponse("-ERR Mailbox update failed\r\n");
    } else {
        sendResponse("+OK Bye\r\n");
    }
}

void POP3Session::loadUserEmails()
{
    if (!currentUser.empty())
        mailbox.loadEmailsFromDisk(currentUser, m_config->getMailRootPath());
}
