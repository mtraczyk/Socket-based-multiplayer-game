#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "err.h"

#define BUFFER_SIZE 1000

struct client_server_msg {
    uint64_t session_id;
    uint8_t turn_direction;
    uint32_t next_expected_event_no;
    char player_name[20];
    uint8_t padding;

} __attribute__((packed));

int main(int argc, char *argv[]) {
    int sock;
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;

    int i, flags, sflags;
    char buffer[BUFFER_SIZE];
    size_t len;
    ssize_t snd_len, rcv_len;
    struct sockaddr_in my_address;
    struct sockaddr_in srvr_address;
    socklen_t rcva_len;

    if (argc < 3) {
        fatal("Usage: %s host port message ...\n", argv[0]);
    }

    /// 'converting' host/port in string to struct addrinfo
    (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET; /// IPv4
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;
    if (getaddrinfo(argv[1], NULL, &addr_hints, &addr_result) != 0) {
        syserr("getaddrinfo");
    }

    my_address.sin_family = AF_INET; /// IPv4
    my_address.sin_addr.s_addr =
            ((struct sockaddr_in *) (addr_result->ai_addr))->sin_addr.s_addr; /// address IP
    my_address.sin_port = htons((uint16_t) atoi(argv[2])); /// port from the command line

    freeaddrinfo(addr_result);

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");

    struct client_server_msg msg;
    memset(&msg, 0, sizeof(msg));
    msg.session_id = 0;
    msg.next_expected_event_no = 0;
    msg.player_name[0] = 'h';
    msg.player_name[1] = 'u';
    msg.session_id = 1;
    msg.turn_direction = 1;
    sflags = 0;
    rcva_len = (socklen_t)
    sizeof(my_address);
    /* wysyla strzalke w prawo (gra powinna wystartowac) */
    snd_len = sendto(sock, &msg, 15, sflags,
                     (struct sockaddr *) &my_address, rcva_len);
    if (snd_len != 15) {
        syserr("partial / failed write");
    }

    /* wysyla komunikat z turn 0, aby robak nie zginal za szybko krecac sie */
    msg.turn_direction = 0;
    snd_len = sendto(sock, &msg, 15, sflags,
                     (struct sockaddr *) &my_address, rcva_len);
    if (snd_len != 15) {
        syserr("partial / failed write");
    }


    (void) memset(buffer, 0, sizeof(buffer));
    flags = 0;
    len = (size_t)
    sizeof(buffer) - 1;
    rcva_len = (socklen_t)
    sizeof(srvr_address);
    rcv_len = recvfrom(sock, buffer, len, flags,
                       (struct sockaddr *) &srvr_address, &rcva_len);

    if (rcv_len < 0) {
        syserr("read");
    }
    (void) printf("read from socket: %zd bytes: %s\n", rcv_len, buffer);


    if (close(sock) == -1) { /// very rare errors can occur here, but then
        syserr("close"); /// it's healthy to do the check
    }

    return 0;
}
