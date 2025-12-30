#ifndef INCLUDED_HTTP_TCPSERVER
#define INCLUDED_HTTP_TCPSERVER

#include <queue>
#include <stdio.h>
#include <winsock.h>
#include <stdlib.h>
#include <string>
#include <thread>

namespace http {

    class TcpServer
    {
    public:
        TcpServer(std::string ip_address, int port);
        ~TcpServer();
        void startListen();

    private:
        std::string m_ip_address;
        int m_port;
        SOCKET m_socket;
        SOCKET m_new_socket;
        long m_incomingMessage;
        struct  sockaddr_in m_socketAddress;
        int m_socketAddress_len;
        std::string m_serverMessage;
        WSADATA m_wsaData;
        std::vector<std::thread> m_workers;
        std::queue<SOCKET> m_clientQueue;
        int startServer();
        void closeServer();
        void handleClient(SOCKET clientSocket);
        void acceptConnection(SOCKET &new_socket);
        std::string buildResponse();
        void workerThread();
        void sendResponse(SOCKET clientSocket);
    };

} // namespace http

#endif
