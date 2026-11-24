#include "mailbox.h"

namespace
{
string normalizeLineEndings(const string& content)
{
    string normalized;
    normalized.reserve(content.size());
    for (string::size_type i = 0; i < content.size(); ++i)
    {
        if (content[i] == '\r')
        {
            if (i + 1 < content.size() && content[i + 1] == '\n')
                ++i;
            normalized += "\r\n";
        }
        else if (content[i] == '\n')
            normalized += "\r\n";
        else
            normalized += content[i];
    }
    return normalized;
}
}

Mailbox::Mailbox()
{
    // Để trống hàm này.
    // KHÔNG gọi loadEmailsFromDisk() ở đây nữa.
    // Lý do: Lúc này chưa biết user nào đăng nhập.
}

Mailbox::~Mailbox()
{
    messages.clear();
    deletedFlags.clear();
}

// Hàm đọc file .eml từ đĩa cứng
void Mailbox::loadEmailsFromDisk(const string& username, const string& mailRootPath)
{
    messages.clear();
    deletedFlags.clear();

    this->currentFolderPath = mailRootPath;
    if (!currentFolderPath.empty() && currentFolderPath[currentFolderPath.size() - 1] != '/' &&
        currentFolderPath[currentFolderPath.size() - 1] != '\\')
        currentFolderPath += '/';
    this->currentFolderPath += username + '/';

    int id = 1;

    while (true)
    {
        string filePath = this->currentFolderPath + to_string(id) + ".eml";
        ifstream inFile(filePath);

        if (!inFile.is_open()) {
            break;
        }

        stringstream buffer;
        buffer << inFile.rdbuf();

        Message msg;
        msg.content = normalizeLineEndings(buffer.str());

        messages.push_back(msg);
        deletedFlags.push_back(false);
        inFile.close();

        id++;
    }

    // Log để kiểm tra server đang đọc đúng thư mục không
    cout << "[INFO] User [" << username << "] logged in. Loaded "
         << (id - 1) << " emails from " << currentFolderPath << endl;
}

bool Mailbox::updateMailbox()
{
    if (currentFolderPath.empty())
        return true;

    bool hasDeletedMessages = false;
    for (size_t i = 0; i < deletedFlags.size(); ++i)
        hasDeletedMessages = hasDeletedMessages || deletedFlags[i];
    if (!hasDeletedMessages)
        return true;

    vector<string> temporaryFiles;
    vector<string> backupFiles;
    vector<string> originalFiles;
    vector<string> committedFiles;
    vector<Message> keptMessages;

    for (size_t i = 0; i < messages.size(); ++i)
    {
        string original = currentFolderPath + to_string(i + 1) + ".eml";
        string backup = currentFolderPath + ".pop3bak_" + to_string(i + 1) + ".eml";
        originalFiles.push_back(original);
        backupFiles.push_back(backup);
        if (deletedFlags[i])
            continue;

        string temporary = currentFolderPath + ".pop3tmp_" + to_string(i + 1) + ".eml";
        temporaryFiles.push_back(temporary);
        ofstream output(temporary.c_str(), ios::binary | ios::trunc);
        if (!output.is_open())
            break;
        output.write(messages[i].content.data(),
                     static_cast<streamsize>(messages[i].content.size()));
        output.close();
        if (!output)
            break;

        keptMessages.push_back(messages[i]);
    }

    size_t expectedTemporaryFiles = 0;
    for (size_t i = 0; i < deletedFlags.size(); ++i)
        if (!deletedFlags[i])
            ++expectedTemporaryFiles;

    if (temporaryFiles.size() != expectedTemporaryFiles ||
        keptMessages.size() != expectedTemporaryFiles)
    {
        for (size_t i = 0; i < temporaryFiles.size(); ++i)
            remove(temporaryFiles[i].c_str());
        cerr << "[ERROR] Failed to stage mailbox update." << endl;
        return false;
    }

    size_t backedUp = 0;
    for (; backedUp < originalFiles.size(); ++backedUp)
    {
        if (rename(originalFiles[backedUp].c_str(), backupFiles[backedUp].c_str()) != 0)
            break;
    }
    if (backedUp != originalFiles.size())
    {
        while (backedUp > 0)
        {
            --backedUp;
            if (rename(backupFiles[backedUp].c_str(), originalFiles[backedUp].c_str()) != 0)
                cerr << "[ERROR] Failed to restore mailbox backup: "
                     << originalFiles[backedUp] << endl;
        }
        for (size_t i = 0; i < temporaryFiles.size(); ++i)
            remove(temporaryFiles[i].c_str());
        cerr << "[ERROR] Failed to back up mailbox files." << endl;
        return false;
    }

    for (size_t i = 0; i < temporaryFiles.size(); ++i)
    {
        string target = currentFolderPath + to_string(i + 1) + ".eml";
        if (rename(temporaryFiles[i].c_str(), target.c_str()) != 0)
        {
            for (size_t j = 0; j < committedFiles.size(); ++j)
                remove(committedFiles[j].c_str());
            for (size_t j = 0; j < originalFiles.size(); ++j)
                if (rename(backupFiles[j].c_str(), originalFiles[j].c_str()) != 0)
                    cerr << "[ERROR] Failed to restore mailbox backup: "
                         << originalFiles[j] << endl;
            for (size_t j = i; j < temporaryFiles.size(); ++j)
                remove(temporaryFiles[j].c_str());
            cerr << "[ERROR] Failed to commit mailbox update." << endl;
            return false;
        }
        committedFiles.push_back(target);
    }

    for (size_t i = 0; i < backupFiles.size(); ++i)
    {
        if (remove(backupFiles[i].c_str()) != 0)
            cerr << "[WARN] Could not remove mailbox backup: " << backupFiles[i] << endl;
    }
    messages.swap(keptMessages);
    deletedFlags.assign(messages.size(), false);
    cout << "[INFO] Mailbox updated. " << messages.size() << " messages kept." << endl;
    return true;
}
// +getResMailbox(): Khôi phục các mail đã đánh dấu xóa
void Mailbox::getResMailbox()
{
    for (size_t i = 0; i < deletedFlags.size(); i++)
    {
        deletedFlags[i] = false;
    }
}

// +getAll(): Lấy danh sách message
vector<Message> Mailbox::getAll()
{
    return messages;
}

// +getMailContent(): Lấy nội dung mail theo ID
string Mailbox::getMailContent(int id)
{
    if (isValidId(id))
    {
        return messages[id - 1].content;
    }
    return "";
}

// +getList(): Trả về danh sách kích thước các mail chưa bị xóa
vector<int> Mailbox::getList()
{
    vector<int> sizes;
    for (size_t i = 0; i < messages.size(); i++)
    {
        // Chỉ lấy những mail chưa bị xóa
        if (!deletedFlags[i])
        {
            sizes.push_back(messages[i].getSize());
        }
    }
    return sizes;
}

// +getMessageInfo(id): Lấy kích thước mail cụ thể
int Mailbox::getMessageInfo(int id)
{
    if (isValidId(id))
    {
        return messages[id - 1].getSize();
    }
    return -1;
}

// +markDeleted(n): Đánh dấu xóa
void Mailbox::markDeleted(int n)
{
    if (isValidId(n))
    {
        deletedFlags[n - 1] = true;
    }
}

// +isDeleted(id): Kiểm tra trạng thái xóa
bool Mailbox::isDeleted(int id)
{
    if (isValidId(id))
    {
        return deletedFlags[id - 1];
    }
    return true; // Coi như đã xóa nếu ID không tồn tại
}

// Hàm hỗ trợ STAT (tính tổng dung lượng)
void Mailbox::getStat(int& count, int& totalSize)
{
    count = 0;
    totalSize = 0;
    for (size_t i = 0; i < messages.size(); i++)
    {
        if (!deletedFlags[i])
        {
            count++;
            totalSize += messages[i].getSize();
        }
    }
}

// Kiểm tra ID hợp lệ (POP3 tính từ 1)
bool Mailbox::isValidId(int id)
{
    if (id > 0 && id <= (int)messages.size())
    {
        return true;
    }
    return false;
}
