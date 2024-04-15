#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <netinet/in.h>

class Client {
public:
    Client(const std::string& server_ip, int port);
    ~Client();
    void send_file_request(const std::string& filename);
    bool receive_file_start(const std::string& filename, int& file_length);
    void send_offset_of_data(const std::string& filename);
    void receive_file(const std::string& filename, int file_length);
private:
    int client_socket;
};

void parseCommandLine(int argc, char* argv[], std::string& server_ip, int& port, std::string& file_name);
int getOffsetFile(const std::string& filename);

#endif // CLIENT_HPP
