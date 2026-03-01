#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <string>
#include "Router.h"
#include "HttpResponse.h"

class HttpServer {
private:
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;
    std::string ipAddress;
    int port;
    Router router;
    bool running;

    const int BUFFER_SIZE = 8192;

    // Private helper methods
    void handleClient(SOCKET clientSocket);
    void sendResponse(SOCKET clientSocket, const HttpResponse& response);

public:
    HttpServer(const std::string& ip, int p);
    ~HttpServer();

    Router& getRouter();
    bool initialize();
    void start();
    void stop();
};