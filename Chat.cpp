#include "Chat.h"

using namespace Mysha;

void Chat::sendKlient(std::string responce, int clientSocket) {
    ::send(clientSocket, responce.c_str(), responce.size(), 0);
}

void Chat::setup() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Socket creation failed!" << std::endl;
        exit(1);
    }

    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_family = AF_INET;

    if (::bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Socket binding failed!" << std::endl;
        exit(1);
    }

    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Server is unable to listen for new connections!" << std::endl;
        exit(1);
    } else {
        std::cout << "Server is listening for new connections...\n";
    }

    while (true) {
        length = sizeof(client);
        clientSocket = ::accept(serverSocket, (sockaddr*)&client, &length);
        if (clientSocket == -1) {
            std::cerr << "Server is unable to accept the data from client!" << std::endl;
            continue;
        }

        std::thread clientThread([=]() { handleClient(clientSocket); });
        clientThread.detach();
    }

    close(serverSocket);
}

void Chat::handleClient(int clientSocket) {
    bool connected = true;
    while (connected) {
        char buffer[MESSAGE_LENGTH];
        int bytes_read = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string command(buffer);
            std::istringstream iss(command);
            std::string action;
            iss >> action;

            if (action == "1") {
                std::string login, password, name;
                iss >> login >> password >> name;
                AddNewUser(login, password, name);
            } else if (action == "2") {
                std::string login, password;
                iss >> login >> password;
                Login(login, password, clientSocket);
            } else if (action == "3") {
                PrintAllUsers();
            } else if (action == "4") {
                connected = false;
                close(clientSocket);
                break;
            }
        }
    }
}

void Chat::UserPanel(int clientSocket) {
    std::cout<<"open user panel\n";
    bool connected = true;
    while (connected) {
        char buffer[MESSAGE_LENGTH];
        int bytes_read = recv(clientSocket, buffer, sizeof(buffer), 0);
        if(bytes_read == 0){
            std::cerr<<"Соединение было сброшено.\n";
            return;
        }
        else if(bytes_read<0){
            if (errno == EINTR) {
                std::cerr << "recv прерван сигналом. Повторная попытка...\n";//Пытаемся поторно вызвать recv
                continue; 
            }
        }
        else if (bytes_read > 0) {
            std::cout<<"start bytes_read > 0\n";
            buffer[bytes_read] = '\0';
            std::string command(buffer);
            std::istringstream iss(command);
            std::string action;
            iss >> action;

            if (action == "1") {
                std::cout<<"User choice 1 - send private mess\n";
                std::string recipient, message;
                iss >> recipient >> message;
                std::cout<<"Попытка чтения команды клиента\n";
                std::getline(iss, message);
                std::cout<<"Попытка входа в send message\n";
                SendMessage(recipient, message);
            }
            else if (action == "2") {
                std::string messageTxt;
                iss >> messageTxt;
                std::getline(iss, messageTxt);
                SendPublicMessage(messageTxt);
            } 
            else if (action == "3") {
                PrintPrivateMessage();
            } 
            else if (action == "4") {
                PrintPublicMessage(clientSocket);
            } 
            else if (action == "5") {
                std::string str = "You closed is user";
                sendKlient(str, clientSocket);
                //handleClient(clientSocket);
                connected = false;
                break;
            }
        }
    }
}

void Chat::Login(const std::string& login, const std::string& password, int ClientSocket) {
    std::cout<<"Open login\n";
    for (int i = 0; i < users.size(); ++i) {
        if (users[i].login == login && CheckPasswordByIndex(i, password)) {
            currentLogin = users[i];
            std::cout<<"Login succesful!\n";
            std::string response = "Login successful!";
            sendKlient(response, clientSocket);
            std::cout<<"open User panel\n";
            currentLogin.clientSocket = clientSocket;//Запоминаем значение сокета для юзера
            UserPanel(currentLogin.clientSocket);
            return;
        }
    }
    std::cout << "Wrong password or user not found!" << std::endl;
    std::string response = "Login failed!";
    sendKlient(response, clientSocket);
}

void Chat::AddNewUser(const std::string& login, const std::string& password, const std::string& name) {
    std::string response;
    for (const auto& user : users) {
        if (user.login == login) {
            std::cout << "Login already exists!" << std::endl;
            response = "Login already exists!";
            sendKlient(response, clientSocket);
            return;
        }
    }

    User newUser(name, "", PORT); 
    newUser.login = login;
    newUser.password = sha1(password.c_str(), static_cast<uint>(password.size()));
    users.push_back(newUser);
    saveUsersToFile(users, fileUser);
    response = "User added successfully!";
    sendKlient(response, clientSocket);
}

void Chat::SendMessage(const std::string& recipient, const std::string& message) {
    std::cout<<"Выполняется отправка личного сообщения пользователю "<<recipient<<std::endl;
    for (auto& user : users) {
        if (user.name == recipient) {
            std::cout<<"Recipient finding!\n";
            Message msg;
            msg.from = currentLogin.name;
            msg.to = recipient;
            msg.text = message;

            user.privateMessages.push_back(msg);
            savePrivateMessageToFile(user.privateMessages, user.filePrivateMessage);
            std::cout<<"Message saved to file\n";

            std::string response = "Message sent successfully!";
            sendKlient(response, currentLogin.clientSocket);

            std::string notification = "You have received a message from " + currentLogin.name + ": " + message;
            sendKlient(notification, user.clientSocket);
            return;
        }
    }
    std::cout << "Recipient not found!" << std::endl;
    std::string response = "Recipient not found!";
    sendKlient(response, currentLogin.clientSocket);
}

void Chat::SendPublicMessage(std::string& message) {
    Message mess;
    mess.from = currentLogin.name;
    mess.text = message;
    publicMessage.push_back(mess);
    savePublicMessageToFile(publicMessage, filePublicMessage);

    std::string response = "Message from " + currentLogin.name + ": " + message;
    sendKlient(response, currentLogin.clientSocket);
}

void Chat::PrintPrivateMessage() {
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].name == currentLogin.name) {
            std::string filename = users[i].filePrivateMessage;
            users[i].privateMessages = loadPrivateMessageFromFile(filename);

            if (users[i].privateMessages.empty()) {
                std::string response = "No private messages available.";
                sendKlient(response, currentLogin.clientSocket);
                return;
            }

            std::stringstream ss;
            for (const auto& msg : users[i].privateMessages) {
                ss << "From: " << msg.from << "\n";
                ss << "To: " << msg.to << "\n";
                ss << "Message: " << msg.text << "\n";
                ss << "------------------------\n";
            }

            std::string response = ss.str();
            sendKlient(response, currentLogin.clientSocket);
            return;
        }
    }

    std::string errorResponse = "User not found!";
    sendKlient(errorResponse, currentLogin.clientSocket);
}

void Chat::PrintAllUsers() {
    std::stringstream ss;
    if (users.empty()) {
        std::cout << "Users empty!\n";
        sendKlient("Users empty!", clientSocket);
    }
    for (int i = 0; i < users.size(); ++i) {
        ss << i << " - " << users[i].name << "\n";
    }

    std::string response = ss.str();
    sendKlient(response, clientSocket);
}

void Chat::PrintPublicMessage(int clientSocket) {
    std::stringstream ss;
    if (publicMessage.empty()) {
        sendKlient("Public message empty!", clientSocket);
    }
    for (int i = 0; i < publicMessage.size(); ++i) {
        ss << i << " - " << publicMessage[i].from << ": " << publicMessage[i].text << "\n";
    }

    std::string response = ss.str();
    sendKlient(response, clientSocket);
}

bool Chat::CheckPasswordByIndex(int index, const std::string& password) {
    uint* checkHash = sha1(password.c_str(), password.size());
    bool match = memcmp(checkHash, users[index].password, 20) == 0;
    delete[] checkHash;
    return match;
}

std::vector<User> Chat::loadUsersFromFile(const std::string& filename) {
    std::vector<User> users;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "File is not open: " << filename << std::endl;
        return users;
    }

    std::string username, login;
    while (file >> username >> login) {
        User user(username, "", PORT); // Для начала оставляем пустой IP и порт
        user.login = login;
        user.password = new uint[5]; 
        for (int i = 0; i < 5; ++i) {
            file >> user.password[i];
        }
        users.push_back(user);
    }
    file.close();
    return users;
}

void Chat::saveUsersToFile(const std::vector<User>& users, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "File is not open: " << filename << std::endl;
        return;
    }

    for (const auto& user : users) {
        file << user.name << " " << user.login << " ";
        for (int i = 0; i < 5; ++i) {
            file << user.password[i] << " ";
        }
        file << std::endl;
    }
    file.close();
}

void Chat::savePublicMessageToFile(const std::vector<Message>& mess, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::string error =  "Failed to open file for writing public messages.";
        sendKlient(error, clientSocket);
        return;
    }

    for (const auto& m : mess) {
        file << m.from<< two << m.text << end;
    }
    file.close();
}

std::vector<Message> Chat::loadPublicMessageFromFile(const std::string& filename) {
    std::vector<Message> mess;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error opening file: " << filename << std::endl;
        return mess;
    }

    std::string line;
    while (std::getline(file, line)) {
        Message message;
        std::stringstream ss(line);
        std::getline(ss, message.from, '#');
        if (message.from.empty()) continue;
        std::string temp;
        ss >> temp;
        std::getline(ss, message.text, '/');
        size_t first = message.text.find_first_not_of(" \t\n\r");
        if (first != std::string::npos) message.text = message.text.substr(first);
        mess.push_back(message);
    }
    file.close();
    return mess;
}

void Chat::savePrivateMessageToFile(const std::vector<Message>& messages, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file for saving private messages: " << filename << std::endl;
        return;
    }

    for (const auto& msg : messages) {
        file << msg.from << "#";
        file << "to ";
        file << msg.to << "#";
        file << "text ";
        file << msg.text << "/\n";
    }
    file.close();
}

std::vector<Message> Chat::loadPrivateMessageFromFile(const std::string& filename) {
    std::vector<Message> mess;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error opening file: " << filename << std::endl;
        return mess;
    }

    std::string line;
    while (std::getline(file, line)) {
        Message message;
        std::stringstream ss(line);
        std::getline(ss, message.from, '#');
        if (message.from.empty()) continue;
        std::string temp;
        ss >> temp;
        std::getline(ss, message.to, '#');
        if (message.to.empty()) continue;
        ss >> temp;
        std::getline(ss, message.text, '/');
        size_t first = message.text.find_first_not_of(" \t\n\r");
        if (first != std::string::npos) message.text = message.text.substr(first);
        mess.push_back(message);
    }
    file.close();
    return mess;
}