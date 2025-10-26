#pragma once
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include "Messages.h"
#include "sha1.h"
#include <sstream>
#include <limits>
#include <ostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>

struct User {
    Mysha::uint* password;
    std::string name, login, filePrivateMessage,serverAddress;
    std::string current = std::filesystem::current_path();
    std::vector<Message> privateMessages; 
    int clientSocket = -1, serverPort;

    User() {}
    User(const std::string& userName, const std::string& serverAddr, int port) :
        name(userName),
        current(std::filesystem::current_path()),
        serverAddress(serverAddr),
        serverPort(port) 
    {
        filePrivateMessage = current + "/" + name + "_Private.txt";
    }
    ~User() {
        if (clientSocket != -1) {
            close(clientSocket);
            std::cout << "Client socket closed for user " << name << std::endl;
        }
    }
};