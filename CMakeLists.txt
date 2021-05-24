cmake_minimum_required(VERSION 3.16)
project(zadanie2SIK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra")
set(DEFAULT_BUILD_TYPE "RELEASE")

#Add the google test subdirectory
add_subdirectory(googletest)

#include googletest/include dir
include_directories(googletest/include)

#include the googlemock/include dir
include_directories(googletest/googlemock/include)

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

set(SERVER_TEST_SOURCE_FILES
        additional_src/server_test.cpp
        src/server/parsing_functionalities.h
        src/server/parsing_functionalities.cpp
        src/shared_functionalities/err.h
        src/shared_functionalities/err.c
        src/server/server.h
        src/server/server.cpp
        src/shared_functionalities/event.cpp
        src/shared_functionalities/event.h
        src/shared_functionalities/crc32.h
        src/shared_functionalities/crc32.c
        )

set(CLIENT_TEST_SOURCE_FILES
        additional_src/client_test.cpp
        )

# Wskazujemy plik wykonywalny.
add_executable(src/server ${SERVER_SOURCE_FILES})

add_executable(src/client ${CLIENT_SOURCE_FILES})

add_executable(server_test ${SERVER_TEST_SOURCE_FILES})

add_executable(client_test ${CLIENT_TEST_SOURCE_FILES})

#Link with GoogleTest
target_link_libraries(server_test gtest gtest_main)

#Link with GoogleMock
target_link_libraries(server_test gmock gmock_main)

#Link with GoogleTest
target_link_libraries(client_test gtest gtest_main)

#Link with GoogleMock
target_link_libraries(client_test gmock gmock_main)