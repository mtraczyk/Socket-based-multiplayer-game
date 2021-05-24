cmake_minimum_required(VERSION 3.16)
project(zadanie2SIK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra")
set(DEFAULT_BUILD_TYPE "RELEASE")

# Wskazujemy pliki źródłowe.
set(SERVER_SOURCE_FILES
        src/server/main.cpp
        src/server/server.h
        src/server/server.cpp
        src/server/parsing_functionalities.h
        src/server/parsing_functionalities.cpp
        src/shared_functionalities/err.h
        src/shared_functionalities/err.c
        src/shared_functionalities/event.cpp
        src/shared_functionalities/event.h
        src/shared_functionalities/crc32.h
        src/shared_functionalities/crc32.c
        src/shared_functionalities/parsing_functionalities.h
        src/shared_functionalities/parsing_functionalities.cpp
        )

set(CLIENT_SOURCE_FILES
        src/shared_functionalities/err.h
        src/shared_functionalities/err.c
        src/client/main.cpp
        src/client/parsing_functionalities.h
        src/client/parsing_functionalities.cpp
        src/client/client.h
        src/client/client.cpp
        src/shared_functionalities/parsing_functionalities.h
        src/shared_functionalities/parsing_functionalities.cpp
        )


# Wskazujemy plik wykonywalny.
add_executable(server ${SERVER_SOURCE_FILES})

add_executable(client ${CLIENT_SOURCE_FILES})
