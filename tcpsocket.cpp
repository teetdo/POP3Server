#ifdef _WIN32
#include <winsock2.h>         // For socket(), connect(), send(), and recv()
#include <ws2tcpip.h>
typedef int socklen_t;
typedef char raw_type;       // Type used for raw data on this platform
#else
#include <sys/types.h>       // For data types
#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()
#include <netinet/in.h>      // For sockaddr_in
typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno
#include <climits>
#include "tcpsocket.h"
void fillAddr(const string&,unsigned short, sockaddr_in&);

using namespace std;

namespace
{
void sendBuffer(int socketDescriptor, const char* buffer, size_t bufferLength)
{
    size_t bytesSent = 0;
    while (bytesSent < bufferLength)
    {
        size_t remaining = bufferLength - bytesSent;
        int chunkLength = remaining > static_cast<size_t>(INT_MAX)
            ? INT_MAX : static_cast<int>(remaining);
        int sent = ::send(socketDescriptor, (raw_type*)(buffer + bytesSent),
                          chunkLength, 0);
        if (sent < 0)
            throw SocketException("Send failed (send())", true);
        if (sent == 0)
            throw SocketException("Send failed (send()) - connection closed");
        bytesSent += static_cast<size_t>(sent);
    }
}
}

// TcpSocket Code

TcpSocket::TcpSocket() : Socket(SOCK_STREAM,IPPROTO_TCP)
{
    shutdownRequested = false;
}

TcpSocket::TcpSocket(int newConnSD) : Socket(newConnSD)
{
    shutdownRequested = false;
}

TcpSocket::TcpSocket(const string &remoteAddress, unsigned short port) : Socket(SOCK_STREAM, IPPROTO_TCP)
{
    shutdownRequested = false;
    connect(remoteAddress, port);
}


void TcpSocket::connect(const string &remoteAddress,
                        unsigned short port)
{
    // Get the address of the requested host
    sockaddr_in destAddr;
    fillAddr(remoteAddress, port, destAddr);
    // If sockDesc is INVALID_SOCKET
    if(sockDesc == -1)
    {
        // Make a new socket
        if ((sockDesc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            throw SocketException("Socket creation failed (socket())", true);
        }
    }
    // Try to connect to the given port
    if (::connect(sockDesc, (sockaddr *) &destAddr, sizeof(destAddr)) < 0)
    {
        throw SocketException("Connect failed (connect())");
    }
}


void TcpSocket::send(const void *buffer, int bufferLen)
{
    if (bufferLen < 0)
        throw SocketException("Send failed (send()) - invalid buffer length");
    sendBuffer(sockDesc, static_cast<const char*>(buffer),
               static_cast<size_t>(bufferLen));
}

void TcpSocket::send(const char* buffer, int bufferLen)
{
    send(static_cast<const void*>(buffer), bufferLen);
}

void TcpSocket::send(const char *str)
{
    sendBuffer(sockDesc, str, strlen(str));
}

void TcpSocket::send(const string& str)
{
    sendBuffer(sockDesc, str.data(), str.size());
}

int TcpSocket::recv(void *buffer, int bufferLen, unsigned short timeout)
{
    if(timeout==0)
    {
        int rtn = ::recv(sockDesc,(raw_type *) buffer, bufferLen, 0);
        if (rtn < 0)
        {
            throw SocketException("Received failed (recv())", true);
        }
        return rtn;
    }
    else if(timeout>0)
    {
        int rtn;
        fd_set	rfds;			/* read file descriptor set	*/
        FD_ZERO(&rfds);
        timeval tv = {timeout,0};
        FD_SET(sockDesc,&rfds);
        int ret;
        if ((ret = select(sockDesc+1, &rfds, NULL, NULL,&tv)) < 0) // Error
        {
            throw SocketException("Select failed (select())", true);
        }
        else if(ret > 0)
        {
            if (FD_ISSET(sockDesc, &rfds))
            {
                rtn = ::recv(sockDesc,(raw_type *) buffer, bufferLen, 0);
                if(rtn<0)
                {
                    throw SocketException("Received failed (recv())", true);
                }
                return rtn;
            }
        }
        else
            return -2;
    }
    return -2;
}

int TcpSocket::recv(char* buffer, int bufferLen, unsigned short timeout)
{
    if(timeout==0)
    {
        int rtn = ::recv(sockDesc, buffer, bufferLen, 0);
        if (rtn < 0)
        {
            throw SocketException("Received failed (recv())", true);
        }
        return rtn;
    }
    else if(timeout>0)
    {
        int rtn;
        fd_set	rfds;			/* read file descriptor set	*/
        FD_ZERO(&rfds);
        timeval tv = {timeout,0};
        FD_SET(sockDesc,&rfds);
        int ret;
        if ((ret = select(sockDesc+1, &rfds, NULL, NULL,&tv)) < 0) // Error
        {
            throw SocketException("Select failed (select())", true);
        }
        else if(ret > 0)
        {
            if (FD_ISSET(sockDesc, &rfds))
            {
                rtn = ::recv(sockDesc, buffer, bufferLen, 0);
                if(rtn<0)
                {
                    throw SocketException("Received failed (recv())", true);
                }
                return rtn;
            }
        }
        else
            return -2;
    }
    return -2;
}


int TcpSocket::recvLine(char* buffer, int bufLen, unsigned short timeout)
{
    if (timeout > 0)
    {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sockDesc, &readSet);
        timeval timeLimit = {timeout, 0};
        int ready = select(sockDesc + 1, &readSet, NULL, NULL, &timeLimit);
        if (ready < 0)
            throw SocketException("Select failed (select())", true);
        if (ready == 0)
            return -2;
    }

    int total_byte_recv = 0;
    int byte_recv;
    while (total_byte_recv < bufLen)
    {
        byte_recv = ::recv(sockDesc, buffer + total_byte_recv, 1, 0);
        if (byte_recv < 0)
            throw SocketException("Received failed (recv())", true);
        if (byte_recv == 0)
            return 0;
        ++total_byte_recv;
        if (total_byte_recv >= 2 && buffer[total_byte_recv - 2] == '\r' &&
            buffer[total_byte_recv - 1] == '\n')
            break;
    }
    return total_byte_recv;
}

void TcpSocket::shutdown(int& type)
{
    shutdownRequested = true;
#ifdef _WIN32
    if(type==SHUTDOWN_SEND)
        ::shutdown(this->sockDesc,SD_SEND);
    else if(type==SHUTDOWN_RECV)
        ::shutdown(this->sockDesc,SD_RECEIVE);
    else if(type==SHUTDOWN_BOTH)
        ::shutdown(sockDesc,SD_BOTH);
#else
    if(type==SHUTDOWN_SEND)
        ::shutdown(this->sockDesc,SHUT_RD);
    else if(type==SHUTDOWN_RECV)
        ::shutdown(this->sockDesc,SHUT_WR);
    else if(type==SHUTDOWN_BOTH)
        ::shutdown(sockDesc,SHUT_RDWR);
#endif // _WIN32
}

bool TcpSocket::isShutdownRequested() const
{
    return shutdownRequested;
}

string TcpSocket::getRemoteAddress()
{
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getpeername(sockDesc, (sockaddr *) &addr,(socklen_t *) &addr_len) < 0)
    {
        throw SocketException("Fetch of foreign address failed (getpeername())", true);
    }
    return inet_ntoa(addr.sin_addr);
}


unsigned long TcpSocket::getRemoteIPv4Address()
{
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getpeername(sockDesc, (sockaddr *) &addr,(socklen_t *) &addr_len) < 0)
    {
        throw SocketException("Fetch of foreign address failed (getpeername())", true);
    }
    return addr.sin_addr.s_addr;
}

unsigned long TcpSocket::getLocalIPv4Address()
{
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getsockname(sockDesc, (sockaddr *) &addr,(socklen_t *) &addr_len) < 0)
    {
        throw SocketException("Fetch of foreign address failed (getsockname())", true);
    }
    return addr.sin_addr.s_addr;
}


unsigned short TcpSocket::getRemotePort()
{
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);
    if (getpeername(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0)
    {
        throw SocketException("Fetch of foreign port failed (getpeername())", true);
    }
    return ntohs(addr.sin_port);
}
