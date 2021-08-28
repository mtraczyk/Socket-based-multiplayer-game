#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <sys/timerfd.h>
#include <chrono>
#include <fcntl.h>
#include <queue>
#include <algorithm>
#include <netinet/tcp.h>

#include "client.h"
#include "../shared_functionalities/err.h"
#include "../shared_functionalities/parsing_functionalities.h"
#include "../shared_functionalities/event.h"
#include "../shared_functionalities/crc32.h"

#define DATA_ARR_SIZE 3
#define FREQUENCY 30000000 // how frequent are messages to the game server in nanoseconds
#define BUFFER_SIZE 4096
#define KEY_UP 0
#define KEY_DOWN 1
#define STRAIGHT 0
#define RIGHT 1
#define LEFT 2
#define NEW_GAME 0
#define PIXEL 1
#define PLAYER_ELIMINATED 2
#define GAME_OVER 3

constexpr static uint32_t numberOfBitsInByte = 8;

struct pollfd pfds[DATA_ARR_SIZE]; // pollfd array
nfds_t nfds = DATA_ARR_SIZE; // pfds array's size
struct itimerspec newValue;
struct timespec now; // auxiliary struct to store current time
char buffer[BUFFER_SIZE];
char messageAsACArray[BUFFER_SIZE];
uint8_t leftKey = KEY_UP;
uint8_t rightKey = KEY_UP;
std::string guiMessages; // used to store fragments that are yet to be parsed
std::string possibleGuiMessages[4] = {"LEFT_KEY_DOWN", "LEFT_KEY_UP", "RIGHT_KEY_DOWN", "RIGHT_KEY_UP"};
uint64_t sessionId;
uint32_t nextExpectedEventNo = 0; // equals zero when a new game starts
uint32_t gameId = 0;
uint32_t gameMaxx, gameMaxy;
bool gamePlayed = false;

namespace {
  int getGuiSocket(std::string const &guiServer, uint16_t guiServerPort) {
    int sock, err;
    struct addrinfo addrHints;
    struct addrinfo *addrResult;

    // 'converting' host/port in string to struct addrinfo
    memset(&addrHints, 0, sizeof(struct addrinfo));
    addrHints.ai_family = AF_UNSPEC;
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

    int disableNagle = 1;  // disable Nagle's algorithm
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &disableNagle, sizeof(disableNagle)) != 0) {
      syserr("setsockopt");
    }

    // connect socket to the server
    if (connect(sock, addrResult->ai_addr, addrResult->ai_addrlen) < 0) {
      syserr("connect");
    }

    freeaddrinfo(addrResult);

    return sock;
  }

  // global structure used to store events that occurred in the current game
  inline std::queue<Event *> &eventsQueue() {
    static auto *s = new std::queue<Event *>();
    return *s;
  }

  // global structure used to store events that occurred in the current game
  inline std::vector<std::string> &playersNames() {
    static auto *s = new std::vector<std::string>();
    return *s;
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

  inline void setPollfdArray(int guiSocket, int servSocket) {
    pfds[0].fd = guiSocket;
    pfds[1].fd = servSocket;
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
          rightKey = KEY_UP;
        } else if (aux == possibleGuiMessages[1]) {
          leftKey = KEY_UP;
        } else if (aux == possibleGuiMessages[2]) {
          rightKey = KEY_DOWN;
          leftKey = KEY_UP;
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

    for (uint32_t i = leftIndex; i <= rightIndex; i++) {
      aux += ((uint32_t) buffer[i]
        << ((rightIndex - leftIndex + 1) * numberOfBitsInByte - (i + 1 - leftIndex) * numberOfBitsInByte));
    }

    return aux;
  }

  inline bool correctName(std::string const &name) {
    if (name.size() > 20) {
      return false;
    }

    for (auto const &u : name) {
      if (!(u >= 33 && u <= 126)) {
        return false;
      }
    }

    return true;
  }

  void decodeNewGameMessage(uint32_t index, uint32_t playersDataLen, uint32_t auxGameId) {
    if (!gamePlayed) {
      auto maxx = readNumberFromBuffer(index, index + 3);
      auto maxy = readNumberFromBuffer(index + 4, index + 7);
      std::vector<std::string> players;

      std::string player;
      for (uint32_t i = index + 8; i < index + playersDataLen + 8; i++) {
        if (buffer[i] == '\0') {
          if (correctName(player)) {
            players.push_back(player);
          } else {
            syserr("incorrect player name");
          }
          player = "";
        } else {
          player += buffer[i];
        }
      }

      if (!player.empty()) {
        syserr("incorrect player name");
      }

      if (players.size() >= 2) {
        gamePlayed = true;
        gameId = auxGameId;
        gameMaxx = maxx;
        gameMaxy = maxy;

        std::sort(players.begin(), players.end());
        eventsQueue().push(new NewGame(nextExpectedEventNo, NEW_GAME, maxx, maxy, players));
        playersNames().clear();
        playersNames() = players;
      } else {
        syserr("incorrect new game data");
      }
    }
  }

  inline void checkPlayerNumber(uint32_t playerNumber) {
    if (playerNumber >= playersNames().size()) {
      syserr("incorrect player number");
    }
  }

  void decodePixelMessage(uint32_t index) {
    auto playerNumber = readNumberFromBuffer(index, index);
    auto x = readNumberFromBuffer(index + 1, index + 4);
    auto y = readNumberFromBuffer(index + 5, index + 8);

    if (x >= gameMaxx || y >= gameMaxy) {
      syserr("player out of bounds");
    }

    checkPlayerNumber(playerNumber);
    eventsQueue().push(new Pixel(nextExpectedEventNo, PIXEL, playerNumber, playersNames()[playerNumber], x, y));
  }

  void decodePlayerEliminatedMessage(uint32_t index) {
    auto playerNumber = readNumberFromBuffer(index, index);

    checkPlayerNumber(playerNumber);
    eventsQueue().push(
      new PlayerEliminated(nextExpectedEventNo, PLAYER_ELIMINATED, playerNumber, playersNames()[playerNumber]));
  }

  void decodeGameOverMessage() {
    eventsQueue().push(new GameOver(nextExpectedEventNo, GAME_OVER));

    nextExpectedEventNo = 0;
    gamePlayed = false;
  }

  void decodeMessageFromGameServer(uint32_t messageLen) {
    std::string datagram;
    for (uint32_t i = 0; i < messageLen; i++) {
      datagram += buffer[i];
    }

    auto auxGameId = readNumberFromBuffer(0, 3);
    if (!gamePlayed || (gamePlayed && auxGameId == gameId)) {
      messageLen -= 4;
      auto currentIndex = 4;
      while (messageLen > 0) {
        auto eventDataLen = readNumberFromBuffer(currentIndex, currentIndex + 3);
        auto eventNo = readNumberFromBuffer(currentIndex + 4, currentIndex + 7);
        auto eventType = readNumberFromBuffer(currentIndex + 8, currentIndex + 8);
        auto eventCrc32 = readNumberFromBuffer(currentIndex + eventDataLen + 4, currentIndex + eventDataLen + 7);

        if (eventNo == nextExpectedEventNo) {
          switch (eventType) {
            case NEW_GAME:
              decodeNewGameMessage(currentIndex + 9, eventDataLen - 13, auxGameId);
              break;
            case PIXEL:
              decodePixelMessage(currentIndex + 9);
              break;
            case PLAYER_ELIMINATED:
              decodePlayerEliminatedMessage(currentIndex + 9);
              break;
            case GAME_OVER:
              decodeGameOverMessage();
          }

          if (eventType <= GAME_OVER) {
            nextExpectedEventNo++;
          }
        } else {
          return;
        }

        messageLen -= (eventDataLen + 8);
        currentIndex += eventDataLen + 8;
      }
    }
  }

  void checkMessageFromGameServer(int servSocket) {
    if (checkPollStatus(1)) {
      int rcvLen = read(servSocket, buffer, BUFFER_SIZE);

      if (rcvLen < 0 || (rcvLen == 0 && !(errno == EAGAIN || errno == EWOULDBLOCK))) {
        syserr("client's read");
      } else {
        decodeMessageFromGameServer(rcvLen);
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

  void sendMessageToServer(int socket, std::string const &message) {
    auto numberOfBytesYetToBeWritten = message.size();
    for (uint32_t i = 0; i < message.size(); i++) {
      messageAsACArray[i] = message[i];
    }

    auto len = write(socket, messageAsACArray, numberOfBytesYetToBeWritten);

    if (len < 0) {
      syserr("write");
    }
    auto numberOfBytesAlreadyWritten = len;
    numberOfBytesYetToBeWritten -= len;

    // Size of the message might be bigger than the buffer's size.
    while (numberOfBytesYetToBeWritten != 0) {
      len = write(socket, &messageAsACArray[numberOfBytesAlreadyWritten], numberOfBytesYetToBeWritten);
      if (len < 0) {
        syserr("write");
      }

      numberOfBytesAlreadyWritten += len;
      numberOfBytesYetToBeWritten -= len;
    }
  }

  void sendMessageToGuiServer(int guiSocket) {
    while (!eventsQueue().empty()) {
      sendMessageToServer(guiSocket, eventsQueue().front()->getByteRepresentationClient());
      eventsQueue().pop();
    }
  }

  void checkSendMessageToGameServer(int servSocket, std::string const &playerName) {
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

      sendMessageToServer(servSocket, message);
    }
  }
}

void client(std::string const &gameServer, std::string const &playerName,
            uint16_t gameServerPort, std::string const &guiServer, uint16_t guiServerPort) {
  sessionId = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  int guiSocket = getGuiSocket(guiServer, guiServerPort); // obtains socket for client - gui connection
  int servSocket, ready;

  // set udp socket for client - game server communication
  struct addrinfo addrHints;
  struct addrinfo *addrResult;

  // 'converting' host/port in string to struct addrinfo
  (void) memset(&addrHints, 0, sizeof(struct addrinfo));
  addrHints.ai_family = AF_UNSPEC; // IPv6 or IPv4
  addrHints.ai_socktype = SOCK_DGRAM;
  addrHints.ai_flags = IPPROTO_UDP;
  if (getaddrinfo(gameServer.c_str(), std::to_string(gameServerPort).c_str(), &addrHints, &addrResult) != 0) {
    syserr("getaddrinfo");
  }

  servSocket = socket(addrResult->ai_family, SOCK_DGRAM, IPPROTO_UDP);
  if (servSocket < 0) {
    syserr("socket");
  }

  if (connect(servSocket, addrResult->ai_addr, addrResult->ai_addrlen) < 0) {
    syserr("connect");
  }

  // socket is non-blocking
  if (fcntl(servSocket, F_SETFL, O_NONBLOCK) != 0) {
    syserr("servSocket fctl failed");
  }

  freeaddrinfo(addrResult);
  setPollfdArray(guiSocket, servSocket); // sets the array that will be used for polling

  for (;;) {
    ready = poll(pfds, nfds, -1); // wait as long as needed
    if (ready == -1) {
      syserr("poll error");
    }

    checkSendMessageToGameServer(servSocket, playerName);
    checkMessageFromGui(guiSocket);
    checkMessageFromGameServer(servSocket);
    sendMessageToGuiServer(guiSocket);
  }

  (void) close(guiSocket);
  (void) close(servSocket);
}
