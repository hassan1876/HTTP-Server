#include <http_tcpServer.h>

#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <condition_variable>
#include <atomic>
#include <queue>

namespace {
    const int BUFFER_SIZE = 30720;
    std::mutex logMutex;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> isRunning{true};

    void log(const std::string &message )
    {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << message << std::endl;
    }
    void exitWithError(const std::string &errorMessage)
    {
        log("ERROR: "+errorMessage);
        exit(1);
    }
}
namespace http
{
    TcpServer::TcpServer(std::string ip_address , int port)
    : m_ip_address(ip_address),m_port(port),m_socket(),m_new_socket(),m_incomingMessage(),
      m_socketAddress(),
      m_socketAddress_len(sizeof(m_socketAddress)),
      m_serverMessage(buildResponse())
    {
        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());

        if (startServer() != 0)
        {
            std::ostringstream ss;
            ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
            log(ss.str());
        }
        const int THREAD_COUNT = 4;
        for (int i = 0; i < THREAD_COUNT; ++i)
        {
            m_workers.emplace_back(&TcpServer::workerThread, this);
        }
    }
    TcpServer::~TcpServer()
    {
        closeServer();
    }
    void TcpServer::closeServer()
    {
        isRunning = false;
        queueCondition.notify_all();

        closesocket(m_socket);

        for (auto &t : m_workers)
            if(t.joinable()) t.join();

        WSACleanup();
        log("Server shut down gracefully.");
    }
    int TcpServer::startServer()
    {
        if (WSAStartup(MAKEWORD(2, 0), &m_wsaData) != 0)
        {
            exitWithError("WSAStartup failed");
        }
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0)
        {
            exitWithError("Cannot create socket");
            return 1;
        }

        if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0)
        {
            exitWithError("Cannot connect socket to address");
            return 1;
        }

        return 0;
    }

    void TcpServer::workerThread()
    {
        while (isRunning)
        {
            SOCKET clientSocket;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCondition.wait(lock, [this] { return !m_clientQueue.empty() || !isRunning; });

                if (!isRunning && m_clientQueue.empty())
                    return;

                clientSocket = m_clientQueue.front();
                m_clientQueue.pop();
            }

            handleClient(clientSocket);
        }
    }

    void TcpServer::startListen() {
        if (listen(m_socket, 20) < 0)
        {
            exitWithError("Socket listen failed");
        }
        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS: "
            << inet_ntoa(m_socketAddress.sin_addr)
            << " PORT: " << ntohs(m_socketAddress.sin_port)
            << " ***\n\n";
        log(ss.str());
        int bytesReceived;

        while (isRunning)
        {
            log("====== Waiting for a new connection ======\n\n\n");
            SOCKET clientSocket;
            acceptConnection(clientSocket);
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                m_clientQueue.push(clientSocket);
            }
            queueCondition.notify_one();
        }
    }


    void TcpServer::handleClient(SOCKET clientSocket)
    {
        char buffer[BUFFER_SIZE] = {0};
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived < 0)
        {
            log("Failed to receive bytes from client socket connection");
            closesocket(clientSocket);
        }

        std::ostringstream ss;
        ss << "------ Received Request from client ------\n\n";
        log(ss.str());

        sendResponse(clientSocket);

        closesocket(clientSocket);
    }

    void TcpServer::acceptConnection(SOCKET &new_socket)
    {
        new_socket = accept(m_socket, (sockaddr *)&m_socketAddress,
                            &m_socketAddress_len);
        if (new_socket < 0)
        {
            std::ostringstream ss;
            ss <<
            "Server failed to accept incoming connection from ADDRESS: "
            << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: "
            << ntohs(m_socketAddress.sin_port);
            exitWithError(ss.str());
        }
    }
    std::string TcpServer::buildResponse()
    {
        std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from multithreaded C++ HTTP Server</p></body></html>";
        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
           << htmlFile;

        return ss.str();
    }

    void TcpServer::sendResponse(SOCKET clientSocket)
    {
        int bytesSent;
        long totalBytesSent = 0;

        while (totalBytesSent < m_serverMessage.size())
        {
            bytesSent = send(clientSocket, m_serverMessage.c_str(), m_serverMessage.size(), 0);
            if (bytesSent < 0)
            {
                break;
            }
            totalBytesSent += bytesSent;
        }

        if (totalBytesSent == m_serverMessage.size())
        {
            log("------ Server Response sent to client ------\n\n");
        }
        else
        {
            log("Error sending response to client.");
        }
    }

}//namespace http