#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <vector>
#include <cstdint>

const int BUFFER_SIZE = 512;

struct Message {
    unsigned char type;
    unsigned int length;
    std::vector<uint8_t> data;
};

Message read_message(int socket);
void write_message(int socket, const Message& msg);

#endif // PROTOCOL_HPP
