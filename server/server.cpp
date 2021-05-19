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

/* Descriptors' array is of a size which equals to MAX_NUM_OF_PLAYERS + 2.
 * One additional is needed for the server's socket, and one more for a turn timer.
 */
#define DESC_ARR_SIZE MAX_NUM_OF_PLAYERS + 2
#define DIS_TIME_SEC 2 // disconnection time in seconds
#define DIS_TIME_NANO 2 * 1000000000 // disconnection time in nanoseconds

struct pollfd pfds[DESC_ARR_SIZE]; // pollfd array
nfds_t nfds = DESC_ARR_SIZE; // pfds array's size

namespace {
  void setPollfdArray(int sock) {
    pfds[0].fd = sock; // first descriptor in the array is socket's descriptor
    pfds[0].events = POLLIN; // poll will be done for POLLIN

    for (int i = 1; i < DESC_ARR_SIZE; i++) {
      pfds[i].fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
      if (pfds[i].fd == -1) {
        syserr("timerfd_create");
      }

      pfds[i].events = POLLIN;
    }
  }

  void checkNextTurn() {
    if (pfds[DESC_ARR_SIZE - 1].revents != 0) {
      if (pfds[DESC_ARR_SIZE - 1].revents & POLLIN) {
        // next turn
        // set timer again
      } else { /* POLLERR | POLLHUP */
        syserr("turn timer error");
      }
    }
  }

  void checkDisconnection() {
    /* Iterate possibly disconnected clients. */
    for (int i = 1; i < DESC_ARR_SIZE - 1; i++) {
      if (pfds[DESC_ARR_SIZE - 1].revents != 0) {
        if (pfds[DESC_ARR_SIZE - 1].revents & POLLIN) {
          // disconnected
          // update number of players
        } else { /* POLLERR | POLLHUP */
          syserr("disconnection timer error");
        }
      }
    }
  }

  void checkDatagram() {

  }
}

void server(uint32_t portNum, int64_t seed, int64_t turningSpeed,
            uint32_t roundPerSecond, uint32_t boardWidth, uint32_t boardHeight) {
  int sock; // socket descriptor
  int ready; // variable to store poll return value
  struct sockaddr_in6 server_address;
  struct sockaddr_in6 client_address;
  socklen_t client_address_len;

  sock = socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, 0); // IPv6 UDP nonblock socket

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

  for (;;) {
    ready = poll(pfds, nfds, -1); // wait as long as needed
    if (ready == -1) {
      syserr("poll error");
    }

    /* Checks whether timer for the next round had expired.
     * If that's the case needed operations are performed.
     */
    checkNextTurn();
    checkDisconnection(); // check for disconnected clients
    checkDatagram(); // check for something to read in the socket
  }

  if (close(sock) == -1) { // very rare errors can occur here, but then
    syserr("close"); // it's healthy to do the check
  }
}