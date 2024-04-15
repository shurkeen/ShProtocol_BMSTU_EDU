#include "../include/Server.hpp"
#include "../../protocol/include/ShProtocol.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <fstream>
#include <thread>
#include <csignal>
#include <filesystem>

int main(int argc, char* argv[]) {
    // Параметры по умолчанию
    int port = 6000;
    std::string folder_path;

    parseCommandLine(argc, argv, port, folder_path);
    
    // Установка обработчика сигнала при падении клиента
    std::signal(SIGPIPE, sigpipe_handler);
    
    // Создаем объект сервера и запускаем его
    Server server(port, folder_path);
    server.start();

    return 0;
}

Server::Server(int port, std::string folder_path) : port(port), folder_path(folder_path){
    // Создаем сокет
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Error: Cannot create socket" << std::endl;
        exit(1);
    }

    // Настраиваем адрес сервера
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port); // Порт

    // Привязываем сокет к адресу
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error: Bind failed" << std::endl;
        exit(1);
    }
}

Server::~Server() {
    // Закрываем сокет
    std::cout << "SERV DIS" << std::endl;
    close(server_socket);
}

void Server::start() {
    // Слушаем подключения
    if (listen(server_socket, 5) < 0) {
        std::cerr << "Error: Listen failed" << std::endl;
        exit(1);
    }

    std::cout << "Server is running..." << std::endl;

    while (true) {
        accept_connections();
    }
}

void Server::accept_connections() {
    // Принимаем подключение
    int client_socket = accept(server_socket, nullptr, nullptr);
    if (client_socket < 0) {
        std::cerr << "Error: Accept failed" << std::endl;
        return;
    }

    // Добавляем сокет клиента в список
    client_sockets.push_back(client_socket);

    // Создаем новый поток для обработки клиента
    std::thread client_thread(&Server::handle_client, this, client_socket);
    client_thread.detach(); // Отсоединяем поток, чтобы он работал автономно
}

void Server::handle_client(int client_socket) {
    // Принимаем запрос на загрузку файла
    Message request_msg = read_message(client_socket);
    if (request_msg.type != 0x01) {
        std::cerr << "Error: Invalid request message type" << std::endl;
        return;
    }

    // Получаем имя файла из сообщения
    std::string filename(request_msg.data.begin(), request_msg.data.end());
    
    // Определение пути к файлу
    std::string path_file = folder_path + "/" + filename;
    
    std::cout << "FILE_NAME: " << path_file << std::endl;
    
    std::cout << "COUNT CLIENT/ SOCKET CLIENT: " << client_sockets.size() << " / " << client_socket << std::endl;
    
    // Отправляем файл клиенту
    send_file(client_socket, path_file);

    // Удаляем сокет клиента из списка
    close(client_socket);
    remove_client_socket(client_socket);
}

void Server::remove_client_socket(int client_socket) {
    std::cout << "DISCONNECT CLIENT SOCKET " << client_socket << std::endl;
    auto it = std::find(client_sockets.begin(), client_sockets.end(), client_socket);
    if (it != client_sockets.end()) {
        client_sockets.erase(it);
    } else {
        std::cerr << "Error: Client socket not found in the list" << std::endl;
    }
}

void Server::send_file(int client_socket, const std::string& path_file) {
    // Открываем файл для чтения
    std::ifstream file(path_file, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << path_file << std::endl;
        
        // Отправляем сообщение о невозможности передачи файла
        Message end_msg;
        end_msg.type = 0x06;
        end_msg.length = 0;
        end_msg.data.resize(0);
        write_message(client_socket, end_msg);
        return;
    }

    // Отправляем подтверждение начала передачи файла
    Message start_msg;
    start_msg.type = 0x02;
    int file_length = std::filesystem::file_size(path_file);
    do {
        start_msg.data.push_back(file_length & 0xFF);
        file_length >>= 8;
    } while (file_length != 0);
    std::reverse(start_msg.data.begin(), start_msg.data.end());
    start_msg.length = start_msg.data.size();
    write_message(client_socket, start_msg);
    
    // Принимаем смещение для передачи файла с нужного места
    Message request_msg;
    try {
        request_msg = read_message(client_socket);
        if (request_msg.type != 0x05) {
            std::cerr << "Error: Invalid request type for resuming file transfer" << std::endl;
            return;
        }
    } catch (const std::runtime_error& err){
        std::cerr << err.what() << std::endl;
        return;
    }
    int offset_file = 0;
    for (uint8_t byte : request_msg.data) {
        offset_file = (offset_file << 8) | byte;
    }
    
    // Устанавливаем смещение для файла
    file.seekg(offset_file);

    // Отправляем фрагменты файла
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    bool error_flag = false;
    while (!file.eof()) {
        file.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE);
        std::streamsize bytes_read = file.gcount();
        
        Message fragment_msg;
        fragment_msg.type = 0x03;
        fragment_msg.length = bytes_read;
        fragment_msg.data.assign(buffer.begin(), buffer.begin() + bytes_read);
        
        try {
            write_message(client_socket, fragment_msg);
            
        } catch (const std::runtime_error& err){
            std::cerr << err.what() << std::endl;
            error_flag = true;
            break;
        }
    }

    // Закрываем файл
    file.close();

    if(!error_flag){
        // Отправляем подтверждение успешной передачи файла
        Message end_msg;
        end_msg.type = 0x04;
        end_msg.length = 0;
        end_msg.data.resize(0);
        write_message(client_socket, end_msg);
    }
}

void parseCommandLine(int argc, char* argv[], int& port, std::string& folder_path) {
    int opt;
    while ((opt = getopt(argc, argv, "p:d:")) != -1) {
        switch (opt) {
            case 'p':
                port = std::atoi(optarg);
                break;
            case 'd':
                folder_path = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -p <port> -d <folder_path>" << std::endl;
                exit(EXIT_FAILURE);
        }
    }

    // Проверяем, был ли установлен параметр -d
    if (folder_path.empty()) {
        std::cerr << "Error: -d <folder_path> is a required parameter." << std::endl;
        exit(EXIT_FAILURE);
    }
}

void sigpipe_handler(int signal) {
    std::cerr << "SIGNAL SIGINT" << std::endl;
}
