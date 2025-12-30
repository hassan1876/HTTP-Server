#include <http_tcpServer.h>


int main()
{
    using namespace http;
    http::TcpServer server("0.0.0.0", 8080);
    server.startListen();
    return 0;
}