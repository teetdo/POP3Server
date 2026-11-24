#include "session.h"

Session::Session(TcpSocket* client, ServerConfig* config)
{
    this->m_client = client;
    this->m_config = config;
}

Session::~Session()
{
}

void Session::doUnknown(char*[], int)
{
    // Phản hồi mặc định khi lệnh không tồn tại
    // Lưu ý: Các giao thức khác nhau có mã lỗi khác nhau (ví dụ POP3 là "-ERR")
    // Ở lớp cha ta chỉ gửi thông báo text đơn giản hoặc log ra console server.
    cout << "Session Info: Unknown command received from " << getClientIP() << endl;
    sendResponse("Unknown command\r\n");
}

string Session::getClientIP() const
{
    if (m_client)
    {
        try
        {
            return m_client->getRemoteAddress();
        }
        catch (...)
        {
            return "Unknown";
        }
    }
    return "Disconnected";
}

void Session::sendResponse(const string& msg)
{
    if (m_client)
    {
        try
        {
            m_client->send(msg);
        }
        catch (SocketException& e)
        {
            cerr << "Session Error (Send): " << e.what() << endl;
        }
    }
}
