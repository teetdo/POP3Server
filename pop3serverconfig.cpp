#include "pop3serverconfig.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{
string trim(const string& value)
{
    string::size_type first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos)
        return "";
    string::size_type last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

string configDirectory(const string& path)
{
    string::size_type separator = path.find_last_of("/\\");
    return separator == string::npos ? "" : path.substr(0, separator + 1);
}

string resolveConfigPath(const string& directory, const string& path)
{
    if (path.empty() || path[0] == '/' || path[0] == '\\' ||
        (path.size() > 1 && path[1] == ':'))
        return path;
    return directory + path;
}
}

POP3ServerConfig::POP3ServerConfig(const string& configFile) : ServerConfig()
{
    loadConfig(configFile);
    loadAccounts();
}

POP3ServerConfig::~POP3ServerConfig()
{
}

void POP3ServerConfig::loadAccounts()
{
    ifstream input(getAccountFilePath().c_str());
    if (!input.is_open())
        throw runtime_error("Cannot open account file: " + getAccountFilePath());

    accounts.clear();
    string line;
    int lineNumber = 0;
    while (getline(input, line))
    {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        istringstream row(line);
        string username, password, extra;
        if (!(row >> username >> password) || (row >> extra))
            throw runtime_error("Invalid account entry at " + getAccountFilePath() +
                                ":" + to_string(lineNumber));
        if (username.find('/') != string::npos || username.find('\\') != string::npos ||
            username == "." || username == "..")
            throw runtime_error("Invalid username at " + getAccountFilePath() +
                                ":" + to_string(lineNumber));
        if (!accounts.insert(make_pair(username, password)).second)
            throw runtime_error("Duplicate username at " + getAccountFilePath() +
                                ":" + to_string(lineNumber));
    }

    if (accounts.empty())
        throw runtime_error("Account file contains no accounts: " + getAccountFilePath());
}

void POP3ServerConfig::loadConfig(const string& configFile)
{
    ifstream input(configFile.c_str());
    if (!input.is_open())
        throw runtime_error("Cannot open server config file: " + configFile);

    const string directory = configDirectory(configFile);
    bool portProvided = false;
    bool mailboxProvided = false;
    bool accountsProvided = false;
    string line;
    int lineNumber = 0;
    while (getline(input, line))
    {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        string::size_type equals = line.find('=');
        if (equals == string::npos)
            throw runtime_error("Invalid config entry at " + configFile +
                                ":" + to_string(lineNumber));
        string key = trim(line.substr(0, equals));
        string value = trim(line.substr(equals + 1));
        if (key == "port")
        {
            istringstream portValue(value);
            unsigned int parsedPort = 0;
            char extra;
            if (!(portValue >> parsedPort) || (portValue >> extra) ||
                parsedPort == 0 || parsedPort > 65535)
                throw runtime_error("Invalid port in " + configFile);
            setPort(static_cast<unsigned short>(parsedPort));
            portProvided = true;
        }
        else if (key == "mailbox")
        {
            setMailRootPath(resolveConfigPath(directory, value));
            mailboxProvided = true;
        }
        else if (key == "accounts")
        {
            setAccountFilePath(resolveConfigPath(directory, value));
            accountsProvided = true;
        }
        else
            throw runtime_error("Unknown config option '" + key + "' in " + configFile);
    }
    if (!portProvided || !mailboxProvided || !accountsProvided)
        throw runtime_error("Config must define port, mailbox, and accounts: " + configFile);
}

bool POP3ServerConfig::checkUserExists(const string& username)
{
    return accounts.find(username) != accounts.end();
}

bool POP3ServerConfig::verifyPassword(const string& username, const string& password)
{
    map<string, string>::const_iterator account = accounts.find(username);
    return account != accounts.end() && account->second == password;
}

shared_ptr<mutex> POP3ServerConfig::getMailboxMutex(const string& username)
{
    lock_guard<mutex> lock(mailboxMutexesLock);
    shared_ptr<mutex>& mailboxLock = mailboxMutexes[username];
    if (!mailboxLock)
        mailboxLock.reset(new mutex());
    return mailboxLock;
}


