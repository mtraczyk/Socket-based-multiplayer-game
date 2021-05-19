#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <time.h>
#include <poll.h>
#include <utility>

#include "server.h"
#include "err.h"

#define MAX_NUM_OF_PLAYERS 25 // maximum number of players is known

/* Descriptors' array is of a size which equals to MAX_NUM_OF_PLAYERS + 2.
 * One additional is needed for the server's socket, and one more for a turn timer.
 */
#define DESC_ARR_SIZE MAX_NUM_OF_PLAYERS + 2
#define DIS_TIME_SEC 2 // disconnection time in seconds
#define NANO_SEC 1000000000 // one nanosecond
#define DIS_TIME_NANO 2 * NANO_SEC // disconnection time in nanoseconds

struct pollfd pfds[DESC_ARR_SIZE]; // pollfd array
nfds_t nfds = DESC_ARR_SIZE; // pfds array's size

struct itimerspec new_value[DESC_ARR_SIZE];
struct timespec now; // auxiliary struct to store current time

// Active players' info.
struct sockaddr_in clientAddress[DESC_ARR_SIZE];
time_t lastActivity[DESC_ARR_SIZE]; // when was the last activity performed by a client
u_short activePlayersNum = 0;

struct sockaddr_in auxClientAddress;

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

  void performNextTurn() {

  }

  inline void getCurrentTime() {
    if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
      syserr("clock_gettime");
    }
  }

  void checkNextTurn(time_t nanoSecPeriod) {
    if (pfds[DESC_ARR_SIZE - 1].revents != 0) {
      if (pfds[DESC_ARR_SIZE - 1].revents & POLLIN) {
        getCurrentTime();
        //set timer again
        new_value[DESC_ARR_SIZE - 1].it_value.tv_sec = now.tv_sec;
        new_value[DESC_ARR_SIZE - 1].it_value.tv_nsec = now.tv_sec + nanoSecPeriod; // first expiration time
        new_value[DESC_ARR_SIZE].it_interval.tv_sec = 0;
        new_value[DESC_ARR_SIZE].it_interval.tv_nsec = nanoSecPeriod; // period
        if (timerfd_settime(pfds[DESC_ARR_SIZE - 1].fd, TFD_TIMER_ABSTIME, &new_value[DESC_ARR_SIZE], NULL) == -1) {
          syserr("timerfd_settime");
        }

        performNextTurn();
      } else { /* POLLERR | POLLHUP */
        syserr("turn timer error");
      }
    }
  }

  inline void disarmATimer(int timerArrNum) {
    new_value[timerArrNum].it_value.tv_sec = 0;
    new_value[timerArrNum].it_value.tv_nsec = 0;
    if (timerfd_settime(pfds[timerArrNum].fd, TFD_TIMER_ABSTIME, &new_value[timerArrNum], NULL) == -1) {
      syserr("timerfd_settime");
    }
  }

  void checkDisconnection() {
    /* Iterate possibly disconnected clients. */
    for (int i = 1; i < DESC_ARR_SIZE - 1; i++) {
      if (pfds[i].revents != 0) {
        if (pfds[i].revents & POLLIN) {
          getCurrentTime();
          if (now.tv_nsec - lastActivity[i] >= DIS_TIME_NANO) {
            // disconnected
            activePlayersNum--; // update number of players
            lastActivity[i] = 0;
            disarmATimer(i);
          }
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
            uint32_t roundsPerSecond, uint32_t boardWidth, uint32_t boardHeight) {
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
    checkNextTurn(NANO_SEC / roundsPerSecond);
    checkDisconnection(); // check for disconnected clients
    checkDatagram(); // check for something to read in the socket
  }

  if (close(sock) == -1) { // very rare errors can occur here, but then
    syserr("close"); // it's healthy to do the check
  }
}
