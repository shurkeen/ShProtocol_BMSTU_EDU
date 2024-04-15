cmake_minimum_required(VERSION 3.24.2)
project(client)

set(CMAKE_CXX_STANDARD 17)

add_executable(client src/Client.cpp
		     src/workWithJSON.cpp
		     ../protocol/src/ShProtocol.cpp)

find_package(nlohmann_json REQUIRED)
target_link_libraries(client PRIVATE nlohmann_json::nlohmann_json)