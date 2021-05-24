#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <sys/timerfd.h>
#include <iostream>

#include "client.h"
#include "../shared_functionalities/err.h"

#define DATA_ARR_SIZE 3
#define FREQUENCY 30000000 // how frequent are messages to the game server in nanoseconds
#define BUFFER_SIZE 4096
#define KEY_UP 0
#define KEY_DOWN 1

struct pollfd pfds[DATA_ARR_SIZE]; // pollfd array
nfds_t nfds = DATA_ARR_SIZE; // pfds array's size
struct itimerspec newValue;
struct timespec now; // auxiliary struct to store current time
struct sockaddr_in gameServerAddress;
char buffer[BUFFER_SIZE];
uint8_t leftKey = KEY_UP;
uint8_t rightKey = KEY_UP;
std::string guiMessages; // used to store fragments that are yet to be parsed
std::string possibleGuiMessages[4] = {"LEFT_KEY_DOWN", "LEFT_KEY_UP", "RIGHT_KEY_DOWN", "RIGHT_KEY_UP"};

namespace {
  int getGuiSocket(std::string const &guiServer, uint16_t guiServerPort) {
    int sock, err;
    struct addrinfo addrHints;
    struct addrinfo *addrResult;

    // 'converting' host/port in string to struct addrinfo
    memset(&addrHints, 0, sizeof(struct addrinfo));
    addrHints.ai_family = AF_INET6; // IPv6
    addrHints.ai_socktype = SOCK_STREAM; // TCP
    addrHints.ai_protocol = IPPROTO_TCP;

    err = getaddrinfo(guiServer.c_str(), std::to_string(guiServerPort).c_str(), &addrHints, &addrResult);
    if (err == EAI_SYSTEM) { // system error
      syserr("getaddrinfo: %s", gai_strerror(err));
    } else if (err != 0) { // other error (host not found, etc.)
      fatal("getaddrinfo: %s", gai_strerror(err));
    }

    // initialize socket according to getaddrinfo results
    sock = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if (sock < 0)
      syserr("socket");

    // connect socket to the server
    if (connect(sock, addrResult->ai_addr, addrResult->ai_addrlen) < 0)
      syserr("connect");

    freeaddrinfo(addrResult);

    return sock;
  }

  inline void getCurrentTime() {
    if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
      syserr("clock_gettime");
    }
  }

  inline void setTimerForServerResponse() {
    // set timer
    getCurrentTime();
    newValue.it_value.tv_sec = now.tv_sec;
    newValue.it_value.tv_nsec = now.tv_nsec + FREQUENCY; // first expiration time
    newValue.it_interval.tv_sec = 0;
    newValue.it_interval.tv_nsec = FREQUENCY; // period
    if (timerfd_settime(pfds[2].fd, TFD_TIMER_ABSTIME, &newValue, nullptr) == -1) {
      syserr("timerfd_settime");
    }
  }

  inline void setPollfdArray(int guiSocket, int udpSocket) {
    pfds[0].fd = guiSocket;
    pfds[1].fd = udpSocket;
    pfds[2].fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (pfds[2].fd == -1) {
      syserr("timerfd_create");
    }

    setTimerForServerResponse(); // set timer

    for (auto &pfd : pfds) {
      pfd.events = POLLIN;
    }
  }

  void setGameServerAddress(std::string const &gameServer, uint16_t gameServerPort) {
    struct addrinfo addrHints;
    struct addrinfo *addrResult;

    // 'converting' host/port in string to struct addrinfo
    (void) memset(&addrHints, 0, sizeof(struct addrinfo));
    addrHints.ai_family = AF_INET6; // IPv6
    addrHints.ai_socktype = SOCK_DGRAM;
    addrHints.ai_protocol = IPPROTO_UDP;
    addrHints.ai_flags = 0;
    addrHints.ai_addrlen = 0;
    addrHints.ai_addr = nullptr;
    addrHints.ai_canonname = nullptr;
    addrHints.ai_next = nullptr;
    if (getaddrinfo(gameServer.c_str(), nullptr, &addrHints, &addrResult) != 0) {
      syserr("getaddrinfo");
    }

    gameServerAddress.sin_family = AF_INET6; // IPv6
    gameServerAddress.sin_addr.s_addr =
      ((struct sockaddr_in *) (addrResult->ai_addr))->sin_addr.s_addr; // address IP
    gameServerAddress.sin_port = htons(gameServerPort); // port from the command line

    freeaddrinfo(addrResult);
  }

  inline void cleanBuffer() {
    for (char &i : buffer) {
      if (i != 0) {
        i = 0;
      } else {
        break;
      }
    }
  }

  inline void parseGuiMessage(std::string const &message) {
    std::string aux = guiMessages;
    guiMessages.clear();

    for (auto const &u : message) {
      if (u == '\n') {
        if (aux == possibleGuiMessages[0]) {
          leftKey = KEY_DOWN;
        } else if (aux == possibleGuiMessages[1]) {
          leftKey = KEY_UP;
        } else if (aux == possibleGuiMessages[2]) {
          rightKey = KEY_DOWN;
        } else if (aux == possibleGuiMessages[3]) {
          rightKey = KEY_UP;
        }
        // otherwise ignore the message

        aux.clear();
      } else {
        aux += u;
      }
    }

    guiMessages = aux; // if there is something left, it needs to be saved
  }

  void checkMessageFromGui(int guiSocket) {
    if (pfds[0].revents != 0) {
      if (pfds[0].revents & POLLIN) {
        // there is a message from gui
        cleanBuffer();
        int len = read(guiSocket, buffer, sizeof(buffer) - 1);
        if (len < 0) {
          syserr("read");
        }

        parseGuiMessage(std::string(buffer));
      } else { /* POLLERR | POLLHUP */
        syserr("turn timer error");
      }
    }
  }

  void checkMessageFromGameServer() {

  }

  void checkSendMessageToGameServer() {

  }
}

void client(std::string const &gameServer, std::string const &playerName,
            uint16_t gameServerPort, std::string const &guiServer, uint16_t guiServerPort) {
  int guiSocket = getGuiSocket(guiServer, guiServerPort); // obtains socket for client - gui connection
  int udpSocket, ready;
  setGameServerAddress(gameServer, gameServerPort);

  udpSocket = socket(PF_INET, SOCK_DGRAM, 0);
  if (udpSocket < 0) {
    syserr("socket");
  }

  setPollfdArray(guiSocket, udpSocket); // sets the array that will be used for polling

  for (;;) {
    ready = poll(pfds, nfds, -1); // wait as long as needed
    if (ready == -1) {
      syserr("poll error");
    }

    checkMessageFromGui(guiSocket);
    checkMessageFromGameServer();
    checkSendMessageToGameServer();
  }

  (void) close(guiSocket);
  (void) close(udpSocket);
}