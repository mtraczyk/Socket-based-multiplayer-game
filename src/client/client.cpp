#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <sys/timerfd.h>
#include <chrono>

#include "client.h"
#include "../shared_functionalities/err.h"
#include "../shared_functionalities/parsing_functionalities.h"

#define DATA_ARR_SIZE 3
#define FREQUENCY 30000000 // how frequent are messages to the game server in nanoseconds
#define BUFFER_SIZE 4096
#define KEY_UP 0
#define KEY_DOWN 1
#define STRAIGHT 0
#define RIGHT 1
#define LEFT 2

constexpr static uint32_t numberOfBitsInByte = 8;

struct pollfd pfds[DATA_ARR_SIZE]; // pollfd array
nfds_t nfds = DATA_ARR_SIZE; // pfds array's size
struct itimerspec newValue;
struct timespec now; // auxiliary struct to store current time
struct sockaddr_in6 gameServerAddress;
char buffer[BUFFER_SIZE];
char messageAsACArray[BUFFER_SIZE];
uint8_t leftKey = KEY_UP;
uint8_t rightKey = KEY_UP;
std::string guiMessages; // used to store fragments that are yet to be parsed
std::string possibleGuiMessages[4] = {"LEFT_KEY_DOWN", "LEFT_KEY_UP", "RIGHT_KEY_DOWN", "RIGHT_KEY_UP"};
uint64_t sessionId;
uint32_t nextExpectedEventNo = 0; // equals zero when a new game starts

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
    if (sock < 0) {
      syserr("socket");
    }

    // connect socket to the server
    if (connect(sock, addrResult->ai_addr, addrResult->ai_addrlen) < 0) {
      syserr("connect");
    }

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
    newValue.it_value.tv_sec = 0;
    newValue.it_value.tv_nsec = FREQUENCY; // first expiration time
    newValue.it_interval.tv_sec = 0;
    newValue.it_interval.tv_nsec = FREQUENCY; // period
    if (timerfd_settime(pfds[2].fd, 0, &newValue, nullptr) == -1) {
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
      pfds->revents = 0;
    }
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

  inline bool checkPollStatus(uint8_t arrayIndex) {
    if (pfds[arrayIndex].revents != 0) {
      if (pfds[arrayIndex].revents & POLLIN) {
        pfds[arrayIndex].revents = 0;
        return true;
      } else { /* POLLERR | POLLHUP */
        syserr("check poll error");
      }
    }

    return false;
  }

  void checkMessageFromGui(int guiSocket) {
    if (checkPollStatus(0)) {
      // there is a message from gui
      cleanBuffer();
      int len = read(guiSocket, buffer, BUFFER_SIZE);
      if (len < 0) {
        syserr("read");
      }

      parseGuiMessage(std::string(buffer));
    }
  }

  inline uint32_t readNumberFromBuffer(uint32_t leftIndex, uint32_t rightIndex) {
    uint32_t aux = 0;

    for (uint32_t i = leftIndex; i < rightIndex; i++) {
      aux += ((uint32_t) buffer[i]
        << ((rightIndex - leftIndex + 1) * numberOfBitsInByte - (i + 1) * numberOfBitsInByte));
    }

    return aux;
  }

  void checkMessageFromGameServer(int udpSocket) {
    if (checkPollStatus(1)) {
      int rcvFlags = 0;
      int rcvLen = recv(udpSocket, buffer, BUFFER_SIZE, rcvFlags);
      if (rcvLen < 0) {
        syserr("client's recv");
      }
    }
  }

  inline uint8_t setTurnDirection() {
    if (leftKey == rightKey) {
      return STRAIGHT;
    } else if (leftKey == KEY_DOWN) {
      return LEFT;
    }

    return RIGHT;
  }

  void sendMessageToGameServer(int udpSocket, std::string const &message) {
    auto numberOfBytesYetToBeWritten = message.size();
    for (uint32_t i = 0; i < message.size(); i++) {
      messageAsACArray[i] = message[i];
    }

    int sflags = 0;
    int rcvaLen = (socklen_t) sizeof(gameServerAddress);
    auto len = sendto(udpSocket, messageAsACArray, numberOfBytesYetToBeWritten, sflags,
                      (struct sockaddr *) &gameServerAddress, rcvaLen);
    if (len < 0) {
      syserr("sendto");
    }
    auto numberOfBytesAlreadyWritten = len;
    numberOfBytesYetToBeWritten -= len;

    // Size of the message might be bigger than the buffer's size.
    while (numberOfBytesYetToBeWritten != 0) {
      len = sendto(udpSocket, &messageAsACArray[numberOfBytesAlreadyWritten], numberOfBytesYetToBeWritten, sflags,
                   (struct sockaddr *) &gameServerAddress, rcvaLen);
      if (len < 0) {
        syserr("sendto");
      }

      numberOfBytesAlreadyWritten += len;
      numberOfBytesYetToBeWritten -= len;
    }
  }

  void checkSendMessageToGameServer(int udpSocket, std::string const &playerName) {
    if (checkPollStatus(2)) {
      uint64_t buf;
      int expired = read(pfds[2].fd, &buf, sizeof(uint64_t));
      if (expired < 0) {
        syserr("timerfd read");
      }

      std::string message;
      uint8_t turnDirection = setTurnDirection();
      addNumber(message, sessionId);
      addNumber(message, turnDirection);
      addNumber(message, nextExpectedEventNo);
      message += playerName;

      sendMessageToGameServer(udpSocket, message);
    }
  }
}

void client(std::string const &gameServer, std::string const &playerName,
            uint16_t gameServerPort, std::string const &guiServer, uint16_t guiServerPort) {
  sessionId = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  int guiSocket = getGuiSocket(guiServer, guiServerPort); // obtains socket for client - gui connection
  int udpSocket, ready;

  // set udp socket for client - game server communication

  struct in6_addr result;
  inet_pton(AF_INET6, gameServer.c_str(), &result);

  udpSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  if (udpSocket < 0) {
    syserr("socket");
  }

  gameServerAddress.sin6_family = AF_INET6; // IPv6
  gameServerAddress.sin6_addr = result;
  gameServerAddress.sin6_port = htons(gameServerPort); // port from the command line

  setPollfdArray(guiSocket, udpSocket); // sets the array that will be used for polling

  for (;;) {
    ready = poll(pfds, nfds, -1); // wait as long as needed
    if (ready == -1) {
      syserr("poll error");
    }

    checkSendMessageToGameServer(udpSocket, playerName);
    checkMessageFromGui(guiSocket);
    checkMessageFromGameServer(udpSocket);
  }

  (void) close(guiSocket);
  (void) close(udpSocket);
}
