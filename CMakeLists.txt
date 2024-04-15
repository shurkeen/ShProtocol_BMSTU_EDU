cmake_minimum_required(VERSION 3.10)

project(file_transfer)

option(BUILD_SERVER "Build the server application" ON)

option(BUILD_CLIENT "Build the client application" ON)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()