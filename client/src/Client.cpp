#include "../include/Client.hpp"
#include "../../protocol/include/ShProtocol.hpp"
#include "../include/workWithJSON.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <filesystem>
#include <getopt.h>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    // Значения по умолчанию
    std::string server_ip = "127.0.0.1";
    std::string file_name;
    int port = 6000;
    
    // Считываем ключи запуска программы
    parseCommandLine(argc, argv, server_ip, port, file_name);
    
    // Создание клиента
    Client client(server_ip, port);
    
    client.send_file_request(file_name);
    
    int file_length = 0;
    bool ready_read = client.receive_file_start(file_name, file_length);
    if(ready_read){
        client.receive_file(file_name, file_length);
    }

    return 0;
}

Client::Client(const std::string& server_ip, int port) {
    // Создаем сокет
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Error: Cannot create socket" << std::endl;
        exit(1);
    }

    // Настраиваем адрес сервера
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port); // Порт
    inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr); // IP-адрес сервера

    // Подключаемся к серверу
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error: Connection failed" << std::endl;
        exit(1);
    }
}

Client::~Client() {
    // Закрываем сокет
    close(client_socket);
}

void Client::send_file_request(const std::string& filename) {
    // Отправляем запрос на загрузку файла
    Message request_msg;
    request_msg.type = 0x01;
    request_msg.length = filename.length();
    copy(filename.begin(), filename.end(), back_inserter(request_msg.data));
    write_message(client_socket, request_msg);
}

bool Client::receive_file_start(const std::string& filename, int& file_length) {
    // Принимаем сообщение о начале передачи файла
    Message start_msg;
    try {
        start_msg = read_message(client_socket);
    } catch (const std::runtime_error& err){
        std::cerr << err.what() << std::endl;
        exit(-1);
    }
    if (start_msg.type == 0x02) {
        int file_length_req = 0;
        for (uint8_t byte : start_msg.data) {
            file_length_req = (file_length_req << 8) | byte;
        }
        file_length = file_length_req;
        send_offset_of_data(filename);
        std::cout << "Server is ready to send file: " << filename << std::endl;
        return true;
    } else if (start_msg.type == 0x06){
        std::cerr << "Error: The file is not readable from the server" << std::endl;
        return false;
    }
    return false;
}

void Client::send_offset_of_data(const std::string& filename){
    // Определяем количество данных в файле на данный момент c помощью JSON-файла
//    int offset_file;
//    try {
//        offset_file = readProgressFromJson(filename).progress;
//    } catch (const std::runtime_error& err){
//        std::cerr << err.what() << std::endl;
//        return;
//    }
    
    // Определяем количество данных в файле на данный момент c помощью lib -> filesystem
    int offset_file = getOffsetFile(filename);
    
    // Отправляем количество данных в файле на данный момент
    Message request_msg;
    request_msg.type = 0x05;
    do {
        request_msg.data.push_back(offset_file & 0xFF);
        offset_file >>= 8;
    } while (offset_file != 0);
    std::reverse(request_msg.data.begin(), request_msg.data.end());
    request_msg.length = request_msg.data.size();
    write_message(client_socket, request_msg);
}

void Client::receive_file(const std::string& filename, int file_length) {
    // Создаем файл для записи
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create file " << filename << std::endl;
        return;
    }

    // Принимаем и записываем фрагменты файла
    Message fragment_msg;
    bool error_flag = false;
//    int total_file_weight = readProgressFromJson(filename).progress;
    int total_file_weight = getOffsetFile(filename);
    do {
        try {
            fragment_msg = read_message(client_socket);
            total_file_weight += fragment_msg.length;
            std::cout << 1.0 * total_file_weight / file_length * 100 << "%" << '\n';
            printf("\033[2J");
            printf("\033[0;0f");
        } catch (const std::runtime_error& err){
            std::cerr << err.what() << std::endl;
            error_flag = true;
            break;
        }
        
        if (fragment_msg.type == 0x03) {
            file.write(reinterpret_cast<const char*>(fragment_msg.data.data()), fragment_msg.length);
        }
    } while (fragment_msg.type == 0x03);

    // Закрываем файл
    file.close();
    
    // Сохраняем данные о количестве скачанных данных
    saveProgressToJson(filename, total_file_weight);
    
    if (fragment_msg.type != 0x04 && !error_flag) {
        std::cerr << "Error: Invalid end message type" << std::endl;
        return;
    }
}

void parseCommandLine(int argc, char* argv[], std::string& server_ip, int& port, std::string& file_name) {
    int opt;
    while ((opt = getopt(argc, argv, "h:p:f:")) != -1) {
        switch (opt) {
            case 'h':
                server_ip = optarg;
                break;
            case 'p':
                port = std::stoi(optarg);
                break;
            case 'f':
                file_name = optarg;
                break;
            case '?':
                std::cerr << "Usage: " << argv[0] << " -h <server_ip> -p <port> -f <file_name>" << std::endl;
                exit(1);
            default:
                std::cerr << "Unknown option: " << static_cast<char>(optopt) << std::endl;
                exit(1);
        }
    }

    // Проверяем, был ли установлен параметр -f
    if (file_name.empty()) {
        std::cerr << "Error: -f <file_name> is a required parameter." << std::endl;
        exit(1);
    }
}

int getOffsetFile(const std::string& filename){
    if (!std::filesystem::exists(filename)) {
        return 0;
    }
    return std::filesystem::file_size(filename);
}


// const std::string& file_name = "ShProtocol_merged.pdf";
// const std::string& file_name = "video1930659294.mp4";
