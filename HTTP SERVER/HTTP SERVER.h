#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include "Router.h"
#include "HttpResponse.h"

class HttpServer {
private:
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;
    std::string ipAddress;
    int port;
    Router router;
    std::atomic<bool> running;

    const int BUFFER_SIZE = 8192;
    int THREAD_POOL_SIZE;

    // Thread pool
    std::vector<std::thread> workerThreads;
    std::queue<SOCKET> clientQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;

    // Private helper methods
    void workerLoop();
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