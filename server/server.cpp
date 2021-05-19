#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <time.h>
#include <poll.h>

#include "server.h"
#include "err.h"

#define MAX_NUM_OF_PLAYERS 25 // maximum number of players is known
#define DESC_ARR_SIZE MAX_NUM_OF_PLAYERS + 1 // descriptors' array is of a size MAX_NUM_OF_PLAYERS + 1

struct pollfd pfds[DESC_ARR_SIZE]; // pollfd array

namespace {
  void setPollfdArray(int sock) {
    pfds[0].fd = sock; // first descriptor in the array is socket's descriptor
    pfds[0].events = POLLIN; // poll will be done for POLLIN

    for (int i = 1; i < MAX_NUM_OF_PLAYERS; i++) {
      pfds[i].fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
      if (pfds[i].fd == -1) {
        syserr("timerfd_create");
      }

      pfds[i].events = POLLIN;
    }
  }
}

void server(uint32_t portNum, int64_t seed, int64_t turningSpeed,
            uint32_t roundPerSecond, uint32_t boardWidth, uint32_t boardHeight) {
  int sock;  // socket descriptor
  struct sockaddr_in6 server_address;
  struct sockaddr_in6 client_address;
  socklen_t client_address_len;

  sock = socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, 0);  // IPv6 UDP nonblock socket

  if (sock < 0) {
    syserr("socket");
  }

  server_address.sin6_family = AF_INET6; // IPv6
  server_address.sin6_addr = in6addr_any; // listening on all interfaces
  server_address.sin6_port = htons(portNum); // port number is stored in portNum

  // bind the socket to a concrete address
  if (bind(sock, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0) {
    syserr("bind, address taken.");
  }

  client_address_len = sizeof(client_address);
  setPollfdArray(sock); // sets the array that will be used for polling



  if (close(sock) == -1) { // very rare errors can occur here, but then
    syserr("close"); // it's healthy to do the check
  }
}