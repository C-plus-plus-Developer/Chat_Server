#pragma once
#include "User.h"
#include "Messages.h"
#include <thread>

#define PORT 7777
#define MESSAGE_LENGTH 1024 // Максимальный размер буфера для данных

using namespace Mysha;

class Chat {
public:
    Chat() {}
    Chat(std::vector<User> _users) : users(_users) {}
    std::string current = std::filesystem::current_path();

    struct sockaddr_in serverAddress, client;
    socklen_t length;
    int serverSocket, clientSocket;
    User currentLogin;

    // Файл с пользователями
    std::string fileUser = current + "/User.txt";
    std::vector<User> users = loadUsersFromFile(fileUser);

    // Файл с публичными сообщениями
    std::string filePublicMessage = current + "/publicMessage.txt";
    std::vector<Message> publicMessage = loadPublicMessageFromFile(filePublicMessage);

    std::vector<User> loadUsersFromFile(const std::string& filename);
    void saveUsersToFile(const std::vector<User>& users, const std::string& filename);
    void savePublicMessageToFile(const std::vector<Message>& mess, const std::string& filename);
    std::vector<Message> loadPublicMessageFromFile(const std::string& filename);
    void savePrivateMessageToFile(const std::vector<Message>& mess, const std::string& filename);
    std::vector<Message> loadPrivateMessageFromFile(const std::string& filename);

    void setup();
private:
    bool CheckPasswordByIndex(int index, const std::string& password);
    void Login(const std::string& login, const std::string& password, int ClientSocket);
    void AddNewUser(const std::string& login, const std::string& password, const std::string& name);
    void handleClient(int clientSocket);
    void SendMessage(const std::string& recipient, const std::string& message);
    void UserPanel(int clientSocket);
    void PrintPrivateMessage();
    void PrintAllUsers();
    void PrintPublicMessage(int clientSocket);
    void SendPublicMessage(std::string& txt);
    void sendKlient(std::string responce, int clientSocket);
    std::string end = " /end.\n";
    std::string two = "#: ";
};