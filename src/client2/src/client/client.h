#ifndef SIK2_CLIENT_H
#define SIK2_CLIENT_H

#define BYTE 8
#define STRAIGHT 0
#define RIGHT 1
#define LEFT 2
#define DWORD 4
#define QWORD 8
#define NEW_GAME 0
#define PIXEL 1
#define PLAYER_ELIMINATED 2
#define GAME_OVER 3


#include "../shared/client_server_msg.h"

#include <iostream>
#include <cstdint>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/timerfd.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <optional>
#include <vector>
#include <endian.h>
#include <netinet/tcp.h>

class Client {
public:
    Client() : name(), server_port("2021"), gui_server("localhost"),
               gui_port("20210"), game_id(0), last_turn(0),
               next_expected_event_no(0),
               session_id(time(NULL)), timer(), gui_comm{""} {
        timer.fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
        timer.events = POLLIN;
        timer.revents = 0;
    }

    void set_parameters(const int argc, char *argv[]);

    void test_parameters();

    void run();

private:
    std::string name;
    std::string server_name;
    std::string server_port;
    std::string gui_server;
    std::string gui_port;
    int32_t server_sock;
    int32_t gui_sock;
    uint32_t game_id;
    uint8_t last_turn;
    uint32_t next_expected_event_no;
    std::vector <std::string> player_name;
    uint32_t width;
    uint32_t height;
    uint64_t session_id;
    struct pollfd timer;
    struct itimerspec timer_spec;
    std::string gui_comm;


    bool validate_player_name(const std::string &player_name);

    void connect_to_gui();

    void open_server_sock();

    int64_t validate_parameter(const std::string &param, const int64_t lo, const int64_t hi);

    void receive_from_gui();

    void send_to_gui(std::string &msg);

    void parse_gui_msg(const std::string &msg);

    void move(const std::string &comm);

    void send_to_game();

    void receive_from_game();

    void set_timer_spec();

    void launch_timer();

    bool check_crc(uint8_t *buff, size_t offset, uint32_t len);

    bool process_event(uint8_t *buff, size_t &offset, uint32_t game_id_);

    void process_new_game_event(uint8_t *buff, size_t &offset, uint32_t len);

    void process_pixel_event(uint8_t *buff, size_t &offset, uint32_t len);

    void process_player_eliminated_event(uint8_t *buff, size_t &offset, uint32_t len);

    void get_new_players(uint8_t *buff, size_t &offset, uint32_t len);

    uint32_t get_dword(uint8_t *buff, size_t &offset);

    uint64_t get_qword(uint8_t *buff, size_t &offset);

    uint8_t get_byte(uint8_t *buff, size_t &offset);

    void send_new_game_event();

    void send_pixel_event(const uint8_t n, const uint32_t x, const uint32_t y);

    void send_player_eliminated_event(const uint8_t n);

};


#endif //SIK2_CLIENT_H
