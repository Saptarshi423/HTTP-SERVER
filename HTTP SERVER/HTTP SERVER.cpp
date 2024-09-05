// HTTP SERVER.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <WinSock2.h>
#include <string>

int main()
{
    std::cout << "Attempting to create a server..\n";

    SOCKET wsocket;
    SOCKET new_wsocket;
    WSADATA wsaData;
    struct sockaddr_in server;
    int server_len;
    int BUFFER_SIZE = 30720;

    // initialize
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Could not initialize";
    }

    //create a socket
    wsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (wsocket == INVALID_SOCKET) {
        std::cout << "Could not create socket.";
    }

    //bind socket to address
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(8000);
    server_len = sizeof(server);

    if (bind(wsocket, (SOCKADDR*)&server, server_len) != 0) {
        std::cout << "Could not bind socket";
    }

    //listen to address
    if (listen(wsocket, 20) != 0) {
        std::cout << "Could not start listening \n";
    }
    std::cout << "Listening on 127.0.0.1:8000 \n";

    int bytes = 0;
    while (true) {
        new_wsocket = accept(wsocket, (SOCKADDR*)&server, &server_len);
        if (new_wsocket == INVALID_SOCKET) {
            std::cout << "Could not accept connection \n";
        }

        //read request
        char buff[30720] = { 0 };
        bytes = recv(new_wsocket, buff, BUFFER_SIZE, 0);

        if (bytes < 0) {
            std::cout << "Could not read client request";
        }

        std::string serverMessage = "HTTP/1.1 200OK\nContent-type: text/html\nContent-Length: ";
        std::string response = "<html><h1>Hello world</h1></html>";

        serverMessage.append(std::to_string(response.size()));
        serverMessage.append("\n\n");
        serverMessage.append(response);


        int bytesSent = 0;
        int totalBytesSent = 0;
        while (totalBytesSent < serverMessage.size())
        {
            std::cout << "SENDING BYTES";

            bytesSent = send(new_wsocket, serverMessage.c_str(), serverMessage.size(), 0);
            if (bytesSent < 0) {
                std::cout << "Could not bind socket";
            }

            totalBytesSent += bytesSent;
        }
        std::cout << "Sent response to client";

        closesocket(new_wsocket);

    }

    closesocket(wsocket);
    WSACleanup();

    return 0;

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
