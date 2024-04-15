#include "../include/ShProtocol.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <fstream>
#include <stdexcept>

Message read_message(int socket){
    Message msg;
    
    // Чтение типа сообщения
    ssize_t bytes_read = recv(socket, &msg.type, sizeof(msg.type), 0);
    if(bytes_read != sizeof(msg.type)){
        // Генерация исключения в случае ошибки
        throw std::runtime_error("Failed to read message type");
    }
    
    // Чтение длины данных
    bytes_read = recv(socket, &msg.length, sizeof(msg.length), 0);
    if(bytes_read != sizeof(msg.length)){
        // Генерация исключения в случае ошибки
        throw std::runtime_error("Failed to read message length");
    }
    
    // Выделение памяти под данные
    msg.data.resize(msg.length);
    
    // Чтение данных
    bytes_read = recv(socket, msg.data.data(), msg.length, 0);
    if(bytes_read != msg.length){
        // Генерация исключения в случае ошибки
        throw std::runtime_error("Failed to read message data");
    }
    
    return msg;
}

void write_message(int socket, const Message& msg) {
    // Отправка типа сообщения
    ssize_t bytes_written = send(socket, &msg.type, sizeof(msg.type), 0);
    if (bytes_written != sizeof(msg.type)) {
        throw std::runtime_error("Failed to write message type");
    }

    // Отправка длины данных
    bytes_written = send(socket, &msg.length, sizeof(msg.length), 0);
    if (bytes_written != sizeof(msg.length)) {
        throw std::runtime_error("Failed to write message length");
    }

    // Отправка данных
    bytes_written = send(socket, msg.data.data(), msg.length, 0);
    if (bytes_written != msg.length) {
        throw std::runtime_error("Failed to write message data");
    }
}
