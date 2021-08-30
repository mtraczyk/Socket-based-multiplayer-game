#ifndef SIK2_CLIENT_SERVER_MSG_H
#define SIK2_CLIENT_SERVER_MSG_H

#define PLAYER_NAME_MAX_LENGTH 20
#define CLIENT_SERVER_MSG_LEN 33
#define CLIENT_SERVER_MSG_MIN_LEN 13
#define MAX_UDP_SIZE 548
#define DWORD 4
#define QWORD 8

#include <cstdint>

struct client_server_msg {
    uint64_t session_id;
    uint8_t turn_direction;
    uint32_t next_expected_event_no;
    char player_name[PLAYER_NAME_MAX_LENGTH];
    uint8_t padding;

} __attribute__((packed));

#endif //SIK2_CLIENT_SERVER_MSG_H
