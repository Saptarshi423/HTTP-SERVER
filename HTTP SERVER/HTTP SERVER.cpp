#include "HTTP SERVER.h"
#include <iostream>
#include <memory>

HttpServer::HttpServer(const std::string& ip, int p)
    : serverSocket(INVALID_SOCKET), ipAddress(ip), port(p), running(false) {}

HttpServer::~HttpServer() {
    stop();
}

Router& HttpServer::getRouter() {
    return router;
}

bool HttpServer::initialize() {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Could not initialize Winsock\n";
        return false;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Could not create socket. Error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return false;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) != 0) {
        std::cout << "Could not bind socket. Error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    if (listen(serverSocket, 20) != 0) {
        std::cout << "Could not start listening. Error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    std::cout << "Server initialized successfully!\n";
    return true;
}

void HttpServer::start() {
    running = true;
    std::cout << "Server listening on " << ipAddress << ":" << port << "\n";

    while (running) {
        int addrLen = sizeof(serverAddr);
        SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&serverAddr, &addrLen);

        if (clientSocket == INVALID_SOCKET) {
            if (running) std::cout << "Accept failed. Error: " << WSAGetLastError() << "\n";
            continue;
        }

        std::cout << "Client connected!\n";
        handleClient(clientSocket);
    }
}

void HttpServer::stop() {
    running = false;
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    WSACleanup();
    std::cout << "Server stopped.\n";
}

void HttpServer::handleClient(SOCKET clientSocket) {
    std::unique_ptr<char[]> buffer(new char[BUFFER_SIZE]);
    memset(buffer.get(), 0, BUFFER_SIZE);

    int bytesReceived = recv(clientSocket, buffer.get(), BUFFER_SIZE - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }

    buffer[bytesReceived] = '\0';
    HttpRequest request(buffer.get());
    request.print();

    HttpResponse response = router.handleRequest(request);
    sendResponse(clientSocket, response);

    closesocket(clientSocket);
    std::cout << "Client disconnected.\n\n";
}

void HttpServer::sendResponse(SOCKET clientSocket, const HttpResponse& response) {
    std::string responseStr = response.build();
    int totalBytesSent = 0;
    int messageSize = static_cast<int>(responseStr.size());

    while (totalBytesSent < messageSize) {
        int bytesSent = send(clientSocket,
            responseStr.c_str() + totalBytesSent,
            messageSize - totalBytesSent, 0);

        if (bytesSent == SOCKET_ERROR) {
            std::cout << "Send failed. Error: " << WSAGetLastError() << "\n";
            break;
        }
        totalBytesSent += bytesSent;
    }
}