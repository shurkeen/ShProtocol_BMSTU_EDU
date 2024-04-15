cmake_minimum_required(VERSION 3.24.2)
project(server)

set(CMAKE_CXX_STANDARD 17)

add_executable(server src/Server.cpp
		     ../protocol/src/ShProtocol.cpp)