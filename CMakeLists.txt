cmake_minimum_required(VERSION 3.16)
project(zadanie2SIK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra")
set(DEFAULT_BUILD_TYPE "RELEASE")

# Wskazujemy pliki źródłowe.
set(SERVER_SOURCE_FILES
        server/main.cpp
        server/server.h
        server/server.cpp
        server/parsing_functionalities.h
        server/parsing_functionalities.cpp
        server/err.h
        server/err.c
        server/event.cpp
        server/event.h
        )

set(CLIENT_SOURCE_FILES
        client/main.c
        client/err.h
        client/err.c
        )

# Wskazujemy plik wykonywalny.
add_executable(server ${SERVER_SOURCE_FILES})

add_executable(client ${CLIENT_SOURCE_FILES})
