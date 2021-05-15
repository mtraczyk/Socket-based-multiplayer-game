cmake_minimum_required(VERSION 3.16)
project(zadanie2SIK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra")
set(DEFAULT_BUILD_TYPE "RELEASE")

#Add the google test subdirectory
add_subdirectory(/Users/mac/Desktop/projectsInC++/zadanie2SIK/googletest)

#include googletest/include dir
include_directories(/Users/mac/Desktop/projectsInC++/zadanie2SIK/googletest/include)

#include the googlemock/include dir
include_directories(/Users/mac/Desktop/projectsInC++/zadanie2SIK/googletest/googlemock/include)

# Wskazujemy pliki źródłowe.
set(SERVER_SOURCE_FILES
        server/main.cpp
        )

set(CLIENT_SOURCE_FILES
        client/main.cpp
        )

set(SERVER_TEST_SOURCE_FILES
        additional_src/server_test.cpp
        )

set(CLIENT_TEST_SOURCE_FILES
        additional_src/client_test.cpp
        )

# Wskazujemy plik wykonywalny.
add_executable(server ${SERVER_SOURCE_FILES})

add_executable(client ${CLIENT_SOURCE_FILES})

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