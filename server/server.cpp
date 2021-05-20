#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <time.h>
#include <poll.h>
#include <utility>
#include <string>
#include <iostream>

#include "server.h"
#include "err.h"

#define MAX_NUM_OF_PLAYERS 25 // maximum number of players is known

/* Data arrays are of a size which equals to MAX_NUM_OF_PLAYERS + 2.
 * One additional is needed for the server's socket, and one more for a turn timer.
 */
#define DATA_ARR_SIZE MAX_NUM_OF_PLAYERS + 2
#define DIS_TIME_SEC 2 // disconnection time in seconds
#define NANO_SEC 1000000000 // one nanosecond
#define DIS_TIME_NANO 2 * NANO_SEC // disconnection time in nanoseconds
#define BUFFER_SIZE 1024
#define CLIENT_DATAGRAM_MIN_SIZE 13 // session_id + turn_direction + next_expected_event_no = 13
#define CLIENT_DATAGRAM_MAX_SIZE 33 // session_id + turn_direction + next_expected_event_no + player_name = 33

#define ERROR -1
#define OK 0
#define SUCCESS 1

char buffer[BUFFER_SIZE];

struct pollfd pfds[DATA_ARR_SIZE]; // pollfd array
nfds_t nfds = DATA_ARR_SIZE; // pfds array's size

struct itimerspec newValue[DATA_ARR_SIZE];
struct timespec now; // auxiliary struct to store current time

// Active players' info.
struct sockaddr_in clientAddress[DATA_ARR_SIZE];
time_t lastActivity[DATA_ARR_SIZE]; // when was the last activity performed by a client
u_short activePlayersNum = 0;

struct sockaddr_in auxClientAddress;
struct sockaddr_in auxSockInfo;
socklen_t clientAddressLen, rcvaLen, sndaLen;
int flags, sndFlags, len;

namespace {
  void setPollfdArray(int sock) {
    pfds[0].fd = sock; // first descriptor in the array is socket's descriptor
    pfds[0].events = POLLIN; // poll will be done for POLLIN

    for (int i = 1; i < DATA_ARR_SIZE; i++) {
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
    if (pfds[DATA_ARR_SIZE - 1].revents != 0) {
      if (pfds[DATA_ARR_SIZE - 1].revents & POLLIN) {
        getCurrentTime();
        //set timer again
        newValue[DATA_ARR_SIZE - 1].it_value.tv_sec = now.tv_sec;
        newValue[DATA_ARR_SIZE - 1].it_value.tv_nsec = now.tv_sec + nanoSecPeriod; // first expiration time
        newValue[DATA_ARR_SIZE].it_interval.tv_sec = 0;
        newValue[DATA_ARR_SIZE].it_interval.tv_nsec = nanoSecPeriod; // period
        if (timerfd_settime(pfds[DATA_ARR_SIZE - 1].fd, TFD_TIMER_ABSTIME, &newValue[DATA_ARR_SIZE], NULL) == -1) {
          syserr("timerfd_settime");
        }

        performNextTurn();
      } else { /* POLLERR | POLLHUP */
        syserr("turn timer error");
      }
    }
  }

  inline void disarmATimer(int timerArrNum) {
    newValue[timerArrNum].it_value.tv_sec = 0;
    newValue[timerArrNum].it_value.tv_nsec = 0;
    if (timerfd_settime(pfds[timerArrNum].fd, TFD_TIMER_ABSTIME, &newValue[timerArrNum], NULL) == -1) {
      syserr("timerfd_settime");
    }
  }

  void checkDisconnection() {
    /* Iterate possibly disconnected clients. */
    for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
      if (pfds[i].revents != 0) {
        if (pfds[i].revents & POLLIN) {
          getCurrentTime();
          if (now.tv_nsec - lastActivity[i] > DIS_TIME_NANO) {
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

  // Returns SUCCESS if a client is connected, ERROR when something failed, and 0 when a client isn't connected.
  int isClientConnected() {
    int codeResult = 0;
    char *clientIP = new char [INET6_ADDRSTRLEN];
    char *auxIP = new char [INET6_ADDRSTRLEN];
    in_port_t clientPort;
    in_port_t auxPort = auxClientAddress.sin_port;

    auxIP = const_cast<char *>(inet_ntop(AF_INET6, &auxClientAddress, auxIP, INET6_ADDRSTRLEN));
    if (auxIP != nullptr) {
      for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
        if (lastActivity[i] != 0) {
          // check a connected client
          clientIP = const_cast<char *>(inet_ntop(AF_INET6, &clientAddress[i], clientIP, INET6_ADDRSTRLEN));
          if (clientIP == nullptr) {
            codeResult = ERROR;
            break;
          }

          clientPort = clientAddress[i].sin_port;
          if (std::string(auxIP) == std::string(clientIP) && auxPort == clientPort) {
            codeResult = SUCCESS;
            break;
          }
        }
      }
    } else {
      codeResult = ERROR;
    }

    delete[] clientIP;
    delete[] auxIP;

    return codeResult;
  }

  void checkDatagram(int sock) {
    sndaLen = sizeof(clientAddress);
    rcvaLen = sizeof(clientAddress);
    flags = 0; // we do net request anything special
    len = recvfrom(sock, buffer, sizeof(buffer), flags, (struct sockaddr *) &auxClientAddress, &rcvaLen);

    // ignore len < 0, we don't want the server to fail
    if (len >= CLIENT_DATAGRAM_MIN_SIZE && len <= CLIENT_DATAGRAM_MAX_SIZE) {
      auxSockInfo = (struct sockaddr_in) auxClientAddress;
      if (isClientConnected() == SUCCESS || MAX_NUM_OF_PLAYERS > activePlayersNum) {
        (void) printf("%.*s\n", (int) len, buffer);
      }
    }
  }
}

void server(uint32_t portNum, int64_t seed, int64_t turningSpeed,
            uint32_t roundsPerSecond, uint32_t boardWidth, uint32_t boardHeight) {
  int sock; // socket descriptor
  int ready; // variable to store poll return value
  struct sockaddr_in6 serverAddress;

  sock = socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, 0); // IPv6 UDP nonblock socket

  if (sock < 0) {
    syserr("socket");
  }

  serverAddress.sin6_family = AF_INET6; // IPv6
  serverAddress.sin6_addr = in6addr_any; // listening on all interfaces
  serverAddress.sin6_port = htons(portNum); // port number is stored in portNum

  // bind the socket to a concrete address
  if (bind(sock, (struct sockaddr *) &serverAddress, (socklen_t) sizeof(serverAddress)) < 0) {
    syserr("bind, address taken.");
  }

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
    checkDatagram(sock); // check for something to read in the socket
  }

  if (close(sock) == -1) { // very rare errors can occur here, but then
    syserr("close"); // it's healthy to do the check
  }
}
