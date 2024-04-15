#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <netinet/in.h>

class Server {
public:
    Server(int port, std::string folder_path);
    ~Server();
    void start();
private:
    int server_socket;
    int port;
    std::vector<int> client_sockets;
    std::string folder_path;
    void accept_connections();
    void handle_client(int client_socket);
    void send_file(int client_socket, const std::string& filename);
    void remove_client_socket(int client_socket);
};

void parseCommandLine(int argc, char* argv[], int& port, std::string& folder_path);
void sigpipe_handler(int signal);

#endif // SERVER_HPP
