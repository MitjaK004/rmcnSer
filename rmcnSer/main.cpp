#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "12345"
#define DEFAULT_BUFLEN 1024

void convertIC(int a, char b[4]) {
    b[0] = (a >> 24) & 0xFF;
    b[1] = (a >> 16) & 0xFF;
    b[2] = (a >> 8) & 0xFF;
    b[3] = a & 0xFF;
}

void convertCI(char a[4], int& b) {
    b = 0;
    for (int i = 3; i >= 0; i--) {
        b += a[i];
    }
}

int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return 1;
    }

    struct addrinfo* addrResult = nullptr;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &addrResult);
    if (result != 0) {
        std::cerr << "getaddrinfo failed with error: " << result << std::endl;
        WSACleanup();
        return 1;
    }

    SOCKET listenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(addrResult);
        WSACleanup();
        return 1;
    }

    result = bind(listenSocket, addrResult->ai_addr, static_cast<int>(addrResult->ai_addrlen));
    if (result == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(addrResult);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(addrResult);

    bool quit = false;
    while (1) {
        result = listen(listenSocket, SOMAXCONN);
        if (result == SOCKET_ERROR) {
            std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "Server is listening for incoming connections..." << std::endl;

        SOCKET clientSocket;
        struct sockaddr_in clientInfo;
        int clientInfoSize = sizeof(clientInfo);

        clientSocket = accept(listenSocket, reinterpret_cast<struct sockaddr*>(&clientInfo), &clientInfoSize);
        
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "Client connected!\n";

        int recvResult = 1;
        std::vector<std::string> recvBuffers;
        char recvBuffersN[4];
        char sendBuffer[DEFAULT_BUFLEN];

        int buffers = 0;
        sendBuffer[0] = '\0';

        const char* ready = "ready";

        do {
            std::cout << "$";

            std::string sendBufferStr;
            std::getline(std::cin, sendBufferStr);

            if (sendBufferStr == "clear") {
                system("cls");
            }
            else{
                const char* sendBufferC = sendBufferStr.c_str();

                result = send(clientSocket, sendBufferC, static_cast<int>(strlen(sendBufferC) + 1), 0);

                if (result == SOCKET_ERROR) {
                    int errorCode = WSAGetLastError();
                    if (errorCode == 10054) {
                        std::cout << "connection closed by client" << std::endl;
                        break;
                    }
                    else {
                        std::cerr << "send failed with error: " << errorCode << std::endl;
                        closesocket(clientSocket);
                        closesocket(listenSocket);
                        WSACleanup();
                        return 1;
                    }
                    return 1;
                }

                else {

                    result = recv(clientSocket, recvBuffersN, 4, 0);
                    if (result > 0) {
                        convertCI(recvBuffersN, buffers);
                    }
                    else if (recvResult == 0) {
                        std::cout << "Connection closed by client" << std::endl;
                    }
                    else {
                        int error = WSAGetLastError();
                        if (error == 10054) {
                            std::cout << "Connection closed by client" << std::endl;
                            recvResult = 0;
                            break;
                        }
                        else if (error == 0) {
                            std::cout << "Connection closed by client" << std::endl;
                            recvResult = 0;
                            break;
                        }
                        else {
                            std::cerr << "Recv failed with error: " << error << std::endl;
                            closesocket(clientSocket);
                            closesocket(listenSocket);
                            WSACleanup();
                        }
                    }

                    recvBuffers.clear();
                    for (int i = 0; i < buffers; i++) {
                        char recvBuffer[DEFAULT_BUFLEN];
                        result = recv(clientSocket, recvBuffer, DEFAULT_BUFLEN, 0);
                        if (result > 0) {
                            recvBuffers.push_back(recvBuffer);
                        }
                        else if (recvResult == 0) {
                            std::cout << "Connection closed by client" << std::endl;
                        }
                        else {
                            int error = WSAGetLastError();
                            if (error == 10054) {
                                std::cout << "Connection closed by client" << std::endl;
                                recvResult = 0;
                                break;
                            }
                            else {
                                std::cerr << "Recv failed with error: " << error << std::endl;
                                closesocket(clientSocket);
                                closesocket(listenSocket);
                                WSACleanup();
                            }
                        }
                    }
                    for (auto rb : recvBuffers) {
                        std::cout << rb;
                    }
                }
            }
        } while (recvResult != 0);

        result = shutdown(clientSocket, SD_SEND);

        if (result == SOCKET_ERROR) {
            std::cerr << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        closesocket(clientSocket);
    }

    closesocket(listenSocket);

    WSACleanup();
    return 0;
}