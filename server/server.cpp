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
#include <set>
#include <vector>

#include "server.h"
#include "err.h"
#include "event.h"

#define MAX_NUM_OF_PLAYERS 25 // maximum number of players is known
#define MIN_NUM_OF_PLAYERS_TO_START_A_GAME 2 // minimum number of players to start a game

/* Data arrays are of a size which equals to MAX_NUM_OF_PLAYERS + 2.
 * One additional is needed for the server's socket, and one more for a turn timer.
 */
#define DATA_ARR_SIZE MAX_NUM_OF_PLAYERS + 2
#define MAX_DATAGRAM_SIZE 550 // datagram can be long up to 550
#define DIS_TIME_SEC 2 // disconnection time in seconds
#define NANO_SEC 1000000000 // one nanosecond
#define DIS_TIME_NANO 2 * NANO_SEC // disconnection time in nanoseconds
#define BUFFER_SIZE 1024
#define CLIENT_DATAGRAM_MIN_SIZE 13 // session_id + turn_direction + next_expected_event_no = 13
#define CLIENT_DATAGRAM_MAX_SIZE 33 // session_id + turn_direction + next_expected_event_no + player_name = 33
#define BITS_IN_A_BYTE_NUM 8 // there are eight bits in one byte
#define EXPECTED_EVENT_NO_BLOCK_START 9 // bytes from 9 to 12 in a datagram from client set out expected_event_no

/* player's name can contain ASCII from 33 to 126 */
#define PLAYER_NAME_ASCII_BEG 33
#define PLAYER_NAME_ASCII_END 126

#define ERROR -1
#define OK 0
#define SUCCESS 1

char buffer[BUFFER_SIZE];

struct pollfd pfds[DATA_ARR_SIZE]; // pollfd array
nfds_t nfds = DATA_ARR_SIZE; // pfds array's size

struct itimerspec newValue[DATA_ARR_SIZE];
struct timespec now; // auxiliary struct to store current time

// active players' info.
struct sockaddr_in clientAddress[DATA_ARR_SIZE];
time_t lastActivity[DATA_ARR_SIZE]; // when was the last activity performed by a client
uint64_t sessionId[DATA_ARR_SIZE]; // session id for every connected player
uint8_t turnDirection[DATA_ARR_SIZE]; // turn direction for every connected player
std::string playerName[DATA_ARR_SIZE]; // players' names
bool takesPartInTheCurrentGame[DATA_ARR_SIZE]; // does some certain player, play the current game
u_short activePlayersNum = 0;

// game info
bool gamePlayed = false; // indicates whether a game is being played
uint32_t gameId; // every game has a randomly generated id

// socket connection data
struct sockaddr_in auxClientAddress;
struct sockaddr_in auxSockInfo;
socklen_t clientAddressLen, rcvaLen, sndaLen;
int flags, sndFlags, len;

namespace {
  using byte = uint8_t;

  template<typename T>
  std::vector<byte> toByte(T input) {
    byte *bytePointer = reinterpret_cast<byte *>(&input);
    return std::vector<byte>(bytePointer, bytePointer + sizeof(T));
  }

  // global structure used to store info about used names
  inline std::set<std::pair<std::string, int>> &namesUsed() {
    static auto *s = new std::set<std::pair<std::string, int>>();
    return *s;
  }

  // global structure used to store events that occurred in the current game
  inline std::vector<Event> &events() {
    static auto *s = new std::vector<Event>();
    return *s;
  }

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
            namesUsed().erase({playerName[i], i});
            disarmATimer(i);
          }
        } else { /* POLLERR | POLLHUP */
          syserr("disconnection timer error");
        }
      }
    }
  }

  /* First number in the pair is SUCCESS if a client is connected,
   * ERROR when something failed, and 0 when a client isn't connected. If first number is SUCCESS, the second
   * one indicates client's index in data array, it equals -1 otherwise.
   */
  std::pair<int, int> isClientConnected() {
    int codeResult = 0, clientNumber = -1;
    char *clientIP = new char[INET6_ADDRSTRLEN];
    char *auxIP = new char[INET6_ADDRSTRLEN];
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
            clientNumber = i;
            break;
          }
        }
      }
    } else {
      codeResult = ERROR;
    }

    delete[] clientIP;
    delete[] auxIP;

    return {codeResult, clientNumber};
  }

  bool isDatagramCorrect(int len, uint64_t *auxSessionId, uint8_t *auxTurnDirection,
                         uint32_t *nextExpectedEventNo, std::string &auxPlayerName) {
    if (len >= CLIENT_DATAGRAM_MIN_SIZE && len <= CLIENT_DATAGRAM_MAX_SIZE) {
      int bytesNumber = 8; // uint64_t has 8 bytes

      // numbers in datagram are big endian
      for (int i = 0; i < bytesNumber; i++) {
        *auxSessionId += ((uint64_t) buffer[i] << (bytesNumber * BITS_IN_A_BYTE_NUM - (i + 1) * BITS_IN_A_BYTE_NUM));
      }

      *auxTurnDirection = (uint8_t) buffer[8]; // eighth byte of a datagram sets out turn direction
      if (!(*auxTurnDirection >= 0 && *auxTurnDirection <= 1)) {
        return false;
      }

      bytesNumber = 4; // uint32_t has four bytes
      // bytes 9 to 12 set out expected_event_no
      for (int i = EXPECTED_EVENT_NO_BLOCK_START; i < 9 + bytesNumber; i++) {
        *nextExpectedEventNo += ((uint32_t) buffer[i]
          << (bytesNumber * BITS_IN_A_BYTE_NUM - (i + 1) * BITS_IN_A_BYTE_NUM));
      }

      // player name block starts with byte number 13
      for (int i = EXPECTED_EVENT_NO_BLOCK_START + bytesNumber; i < len; i++) {
        if (!(buffer[i] >= PLAYER_NAME_ASCII_BEG && buffer[i] <= PLAYER_NAME_ASCII_END)) {
          return false;
        }

        auxPlayerName += buffer[i];
      }
    }

    return false;
  }

  void sendDatagram(std::string const &datagram, int sock) {
    const char *message = datagram.c_str();

    int sendFlags = 0;
    sndaLen = (socklen_t) sizeof(auxClientAddress);
    /* Ignore errors, we don't want the server to go down.
     * sock is non blocking.
     */
    sendto(sock, message, sizeof(message), sendFlags, (struct sockaddr *) &auxClientAddress, sndaLen);
  }

  void sendDatagrams(uint32_t nextExpectedEventNo, int sock) {
    uint32_t tmp = htonl(gameId); // convert to big endian
    auto byteVector = toByte(tmp);
    std::string gameIdByteArray(byteVector.begin(), byteVector.end());

    /* put events into one datagram */
    std::string datagram = gameIdByteArray;
    for (uint32_t i = nextExpectedEventNo; i < events().size(); i++) {
      std::string eventByteArray = events()[i].getByteRepresentation();

      if (datagram.size() + eventByteArray.size() <= MAX_DATAGRAM_SIZE) {
        datagram += eventByteArray;
      } else {
        sendDatagram(eventByteArray, sock);
        datagram = gameIdByteArray + eventByteArray;
      }
    }

    if (!datagram.empty()) {
      sendDatagram(datagram, sock);
    }
  }

  // Process a new datagram from a connected client.
  void newDatagramFromAConnectedClient(uint64_t auxSessionId, uint8_t auxTurnDirection, uint32_t nextExpectedEvenNo,
                                       std::string const &auxPlayerName, int indexInDataArray, int sock) {
    // here auxSessionId <= sessionId[indexInDataArray]
    if (auxSessionId == sessionId[indexInDataArray] &&
        namesUsed().find({auxPlayerName, indexInDataArray}) != namesUsed().end()) {
      // datagram isn't ignored
      getCurrentTime();
      lastActivity[indexInDataArray] = now.tv_nsec;
      turnDirection[indexInDataArray] = auxTurnDirection;
      sendDatagrams(nextExpectedEvenNo, sock);
    }
  }

  inline int findFreeIndex() {
    for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
      if (lastActivity[i] == 0) {
        return i;
      }
    }

    return ERROR;
  }

  void processNewPlayer(uint64_t auxSessionId, uint8_t auxTurnDirection, uint32_t nextExpectedEventNo,
                        std::string const &auxPlayerName, int sock) {
    // find free index in the data array
    int index = findFreeIndex();

    // new activity being made
    getCurrentTime();
    lastActivity[index] = now.tv_nsec;

    // set data
    sessionId[index] = auxSessionId;
    turnDirection[index] = auxTurnDirection;
    playerName[index] = auxPlayerName;
    namesUsed().insert({auxPlayerName, index});

    if (gamePlayed) {
      // player is a spectator in the current game, send him all of the datagrams connected to the current match
      sendDatagrams(0, sock);
    }
  }

  void checkDatagram(int sock) {
    sndaLen = sizeof(clientAddress);
    rcvaLen = sizeof(clientAddress);
    flags = 0; // we do net request anything special

    for (char &i : buffer) {
      if (i != '\0') {
        i = '\0';
      } else {
        break;
      }
    }
    len = recvfrom(sock, buffer, sizeof(buffer), flags, (struct sockaddr *) &auxClientAddress, &rcvaLen);

    // variables to store client's data
    uint64_t auxSessionId;
    uint8_t auxTurnDirection;
    uint32_t nextExpectedEventNo;
    std::string auxPlayerName;

    if (isDatagramCorrect(len, &auxSessionId, &auxTurnDirection, &nextExpectedEventNo, auxPlayerName)) {
      // datagram is correct but still might be ignored
      auxSockInfo = (struct sockaddr_in) auxClientAddress;
      std::pair<int, int> clientConnected = isClientConnected();

      if (clientConnected.first == SUCCESS && auxSessionId <= sessionId[clientConnected.second]) {
        newDatagramFromAConnectedClient(auxSessionId, auxTurnDirection,
                                        nextExpectedEventNo, auxPlayerName, clientConnected.second, sock);
      } else if (MAX_NUM_OF_PLAYERS > activePlayersNum) {
        processNewPlayer(auxSessionId, auxTurnDirection, nextExpectedEventNo, auxPlayerName, sock);
# warning DEBUG STUFF
        (void) printf("%.*s\n", (int) len, buffer);
      }
    }
  }

  // counts number of players that want to participate in a game
  inline int numberOfReadyPlayers() {
    int counter;
    for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
      if (lastActivity[i] != 0 && !playerName[i].empty() && turnDirection[i] != 0) {
        counter++;
      }
    }

    return counter;
  }

  void newGame() {
    if (!gamePlayed && numberOfReadyPlayers() >= MIN_NUM_OF_PLAYERS_TO_START_A_GAME) {
#warning START NEW GAME
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
    newGame(); // check the possibility of starting a new game
  }

  if (close(sock) == -1) { // very rare errors can occur here, but then
    syserr("close"); // it's healthy to do the check
  }
}
