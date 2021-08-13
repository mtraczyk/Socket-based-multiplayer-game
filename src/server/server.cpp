#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <utility>
#include <string>
#include <iostream>
#include <set>
#include <vector>
#include <cmath>
#include <csignal>
#include <fcntl.h>
#include <cstring>

#include "server.h"
#include "../shared_functionalities/err.h"
#include "../shared_functionalities/event.h"
#include "../shared_functionalities/parsing_functionalities.h"

#define MAX_NUM_OF_PLAYERS 25 // maximum number of players is known
#define MIN_NUM_OF_PLAYERS_TO_START_A_GAME 2 // minimum number of players to start a game

/* Data arrays are of a size which equals to MAX_NUM_OF_PLAYERS + 2.
 * One additional is needed for the server's socket, and one more for a turn timer.
 */
#define DATA_ARR_SIZE MAX_NUM_OF_PLAYERS + 2
#define MAX_DATAGRAM_SIZE 550 // datagram can be long up to 550
#define NANO_SEC 1000000000 // one nanosecond
#define DIS_TIME_NANO 2 * NANO_SEC // disconnection time in nanoseconds
#define DIS_TIME_SEC 2
#define BUFFER_SIZE 4096
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

#define RAND_MULTIPLIER 279410273
#define RAND_MODULO 4294967291
#define PI 3.14159265
#define FULL_CIRCLE 360
#define HALF_CIRCLE 180

#define NEW_GAME 0
#define PIXEL 1
#define PLAYER_ELIMINATED 2
#define GAME_OVER 3
#define LEFT_ARR 1
#define RIGHT_ARR 2

char buffer[BUFFER_SIZE];
char messageAsACArray[BUFFER_SIZE];

struct pollfd pfds[DATA_ARR_SIZE]; // pollfd array
nfds_t nfds = DATA_ARR_SIZE; // pfds array's size

struct itimerspec newValue[DATA_ARR_SIZE];
struct itimerspec oldValue;
struct timespec now; // auxiliary struct to store current time

// connected players' info.
bool isPlayerActive[DATA_ARR_SIZE];
bool hasPlayerResponded[DATA_ARR_SIZE];
struct sockaddr_storage clientAddress[DATA_ARR_SIZE];
time_t lastActivity[DATA_ARR_SIZE]; // when was the last activity performed by a client
uint64_t sessionId[DATA_ARR_SIZE]; // session id for every connected player
uint8_t turnDirection[DATA_ARR_SIZE]; // turn direction for every connected player
std::string playerName[DATA_ARR_SIZE]; // players' names
bool takesPartInTheCurrentGame[DATA_ARR_SIZE]; // does some certain player, play the current game
double playerWormX[DATA_ARR_SIZE], playerWormY[DATA_ARR_SIZE], playerDirection[DATA_ARR_SIZE];
uint32_t roundedPlayerWormX[DATA_ARR_SIZE], roundedPlayerWormY[DATA_ARR_SIZE];
u_short activePlayersNum = 0;

// game info
bool gamePlayed = false; // indicates whether a game is being played
uint32_t gameId; // every game has a randomly generated id
u_short playersInTheGame = 0; // number of people in a particular game
std::string playingPlayerName[DATA_ARR_SIZE]; // the names are store sorted
int playingPlayerDataIndex[DATA_ARR_SIZE]; // index in connection data tables
bool isPlayerDead[DATA_ARR_SIZE];

// socket connection data
struct sockaddr_storage auxClientAddress;
socklen_t rcvaLen, sndaLen;
int flags, len;

int64_t randomNumber;

namespace {
  void addPlayerEliminatedEvent(uint8_t playerNum);
  void addPixelEvent(uint8_t playerNum, uint16_t x, uint16_t y);
  void sendEvent(int sock);

  // global structure used to store info about used names
  inline std::set<std::string> &namesUsed() {
    static auto *s = new std::set<std::string>();
    return *s;
  }

  // global structure used to store events that occurred in the current game
  inline std::vector<Event *> &events() {
    static auto *s = new std::vector<Event *>();
    return *s;
  }

  // global structure used to store info about used squares
  inline std::set<std::pair<uint16_t, uint16_t>> &squaresUsed() {
    static auto *s = new std::set<std::pair<uint16_t, uint16_t>>();
    return *s;
  }

  void setPollfdArray(int sock) {
    pfds[0].fd = sock; // first descriptor in the array is socket's descriptor
    pfds[0].events = POLLIN; // poll will be done for POLLIN
    pfds[0].revents = 0;

    for (int i = 1; i < DATA_ARR_SIZE; i++) {
      pfds[i].fd = timerfd_create(CLOCK_REALTIME, O_NONBLOCK);
      if (pfds[i].fd == -1) {
        syserr("timerfd_create");
      }

      pfds[i].events = POLLIN;
      pfds[i].revents = 0;
    }
  }

  inline double angleToRadian(double x) {
    return x * PI / HALF_CIRCLE;
  }

  inline bool checkBorders(int64_t x, int64_t y, int64_t boardWidth, int64_t boardHeight) {
    return x >= 0 && y >= 0 && x < boardWidth && y < boardHeight;
  }

  void performNextTurn(uint8_t turningSpeed, uint16_t boardWidth, uint16_t boardHeight, int sock) {
    for (int i = 0; i < DATA_ARR_SIZE - 1; i++) {
      auto index = playingPlayerDataIndex[i];

      if (index != -1 && !isPlayerDead[i] && takesPartInTheCurrentGame[index]) {
        if (turnDirection[index] == LEFT_ARR) {
          playerDirection[index] += (double) turningSpeed;
        } else if (turnDirection[index] == RIGHT_ARR) {
          playerDirection[index] -= (double) turningSpeed;
        }

        playerWormX[index] += cos(angleToRadian(playerDirection[index]));
        playerWormY[index] += sin(angleToRadian(playerDirection[index]));
        int64_t auxX = std::floor(playerWormX[index]);
        int64_t auxY = std::floor(playerWormY[index]);

        if (checkBorders(auxX, auxY, boardWidth, boardHeight)) {
          if (!(roundedPlayerWormX[index] == auxX && roundedPlayerWormY[index] == auxY)) {
            roundedPlayerWormX[index] = auxX;
            roundedPlayerWormY[index] = auxY;

            if (squaresUsed().find({auxX, auxY}) != squaresUsed().end()) {
              // player eliminated
              addPlayerEliminatedEvent(i);
              sendEvent(sock);
            } else {
              // eat pixel
              squaresUsed().insert({roundedPlayerWormX[index], roundedPlayerWormY[index]});
              addPixelEvent(i, roundedPlayerWormX[index], roundedPlayerWormY[index]);
              sendEvent(sock);
            }
          }
        }
      }
    }
  }

  inline void getCurrentTime() {
    if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
      syserr("clock_gettime");
    }
  }

  void checkNextTurn(uint8_t turningSpeed, uint16_t boardWidth, uint16_t boardHeight, int sock) {
    if (pfds[DATA_ARR_SIZE - 1].revents != 0) {
      if (pfds[DATA_ARR_SIZE - 1].revents & POLLIN) {
        uint64_t buf;
        int expired = read(pfds[DATA_ARR_SIZE - 1].fd, &buf, sizeof(uint64_t));
        if (expired < 0) {
          syserr("timerfd read");
        }

        getCurrentTime();
        std::cout << "game round: " << now.tv_nsec << std::endl;
        performNextTurn(turningSpeed, boardWidth, boardHeight, sock);
        pfds[DATA_ARR_SIZE - 1].revents = 0;

      } else { /* POLLERR | POLLHUP */
        syserr("turn timer error");
      }
    }
  }

  inline void disarmATimer(int timerArrNum) {
    newValue[timerArrNum].it_value.tv_sec = 0;
    newValue[timerArrNum].it_value.tv_nsec = 0;
    if (timerfd_settime(pfds[timerArrNum].fd, 0, &newValue[timerArrNum], &oldValue) == -1) {
      syserr("timerfd_settime");
    }
  }

  void checkDisconnection() {
    /* Iterate possibly disconnected clients. */
    for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
      if (pfds[i].revents != 0) {
        if (pfds[i].revents & POLLIN) {
          pfds[i].revents = 0;

          uint64_t buf;
          int expired = read(pfds[DATA_ARR_SIZE - 1].fd, &buf, sizeof(uint64_t));
          if (expired < 0) {
            syserr("timerfd read");
          }

          getCurrentTime();
          std::cout << i << " " << now.tv_nsec << std::endl;
          if (!hasPlayerResponded[i]) {
            // disconnected
            activePlayersNum--; // update number of players
            isPlayerActive[i] = false;
            namesUsed().erase(playerName[i]);
            disarmATimer(i);
          }
        } else { /* POLLERR | POLLHUP */
          syserr("disconnection timer error");
        }
      }
    }
  }

  void getIPAndPort(std::string &ip, in_port_t *port, sockaddr_storage *pAddress) {
    char *auxIP = nullptr;
    if (pAddress->ss_family == AF_INET) {
      auxIP = new char[INET_ADDRSTRLEN];
      auxIP = const_cast<char *>(inet_ntop(AF_INET, pAddress, auxIP, INET_ADDRSTRLEN));
      *port = ((struct sockaddr_in *) pAddress)->sin_port;
    } else if (pAddress->ss_family == AF_INET6) {
      auxIP = new char[INET6_ADDRSTRLEN];
      auxIP = const_cast<char *>(inet_ntop(AF_INET6, pAddress, auxIP, INET6_ADDRSTRLEN));
      *port = ((struct sockaddr_in6 *) pAddress)->sin6_port;
    }

    if (auxIP != nullptr) {
      ip = std::string(auxIP);
    }

    delete[] auxIP;
  }

/* First number in the pair is SUCCESS if a client is connected,
   * ERROR when something failed, and 0 when a client isn't connected. If first number is SUCCESS, the second
   * one indicates client's index in data array, it equals -1 otherwise.
   */
  std::pair<int, int> isClientConnected() {
    int codeResult = 0, clientNumber = -1;
    std::string auxIP, clientIP;
    in_port_t clientPort;
    in_port_t auxPort;

    // get ip and port for the new client
    getIPAndPort(auxIP, &auxPort, &auxClientAddress);

    if (!auxIP.empty()) {
      for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
        if (isPlayerActive[i]) {
          // check a connected client
          getIPAndPort(clientIP, &clientPort, &clientAddress[i]);
          if (clientIP.empty()) {
            codeResult = ERROR;
            break;
          }

          if (auxIP == clientIP && auxPort == clientPort) {
            codeResult = SUCCESS;
            clientNumber = i;
            break;
          }
        }
      }
    } else {
      codeResult = ERROR;
    }

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
      if (*auxTurnDirection > 2) {
        return false;
      }

      bytesNumber = 4; // uint32_t has four bytes
      // bytes 9 to 12 set out expected_event_no
      for (int i = EXPECTED_EVENT_NO_BLOCK_START; i < 9 + bytesNumber; i++) {
        *nextExpectedEventNo += ((uint32_t) buffer[i]
          << (bytesNumber * BITS_IN_A_BYTE_NUM - (i + 1 - EXPECTED_EVENT_NO_BLOCK_START) * BITS_IN_A_BYTE_NUM));
      }

      // player name block starts with byte number 13
      for (int i = EXPECTED_EVENT_NO_BLOCK_START + bytesNumber; i < len; i++) {
        if (!(buffer[i] >= PLAYER_NAME_ASCII_BEG && buffer[i] <= PLAYER_NAME_ASCII_END)) {
          return false;
        }

        auxPlayerName += buffer[i];
      }

//      std::cout << "datagram: " << auxPlayerName << " turn direction: "
//                << (int) (*auxTurnDirection) << " next expected even no: " << *nextExpectedEventNo << std::endl;
    } else {
      std::cout << "incorrect datagram" << std::endl;
      return false;
    }

    return true;
  }

  void sendDatagram(std::string const &datagram, int sock) {
    for (uint32_t i = 0; i < datagram.size(); i++) {
      messageAsACArray[i] = datagram[i];
    }

    int sendFlags = 0;
    sndaLen = (socklen_t) sizeof(struct sockaddr_storage);
    /* Ignore errors, we don't want the server to go down.
     * sock is non blocking.
     */
    int len = sendto(sock, messageAsACArray, datagram.size(), sendFlags, (sockaddr *) &auxClientAddress, sndaLen);
    if (len < 0) {
      syserr("send message to a client");
    }
  }

  void sendDatagrams(uint32_t nextExpectedEventNo, int sock) {
    uint32_t tmp = htonl(gameId); // convert to big endian
    auto byteVector = toByte(tmp);
    std::string gameIdByteArray(byteVector.begin(), byteVector.end());

    /* put events into one datagram */
    std::string datagram = gameIdByteArray;
    for (uint32_t i = nextExpectedEventNo; i < events().size(); i++) {
      std::string eventByteArray = (*events()[i]).getByteRepresentationServer();

      if (datagram.size() + eventByteArray.size() <= MAX_DATAGRAM_SIZE) {
        datagram += eventByteArray;
      } else {
        sendDatagram(datagram, sock);
        datagram = gameIdByteArray + eventByteArray;
      }
    }

    uint32_t numberOfBytesInGameId = 4;
    if (datagram.size() != numberOfBytesInGameId) {
      sendDatagram(datagram, sock);
    }
  }

  // Process a new datagram from a connected client.
  void newDatagramFromAConnectedClient(uint64_t auxSessionId, uint8_t auxTurnDirection, uint32_t nextExpectedEvenNo,
                                       std::string const &auxPlayerName, int indexInDataArray, int sock) {
    // here auxSessionId <= sessionId[indexInDataArray]
    if (auxSessionId == sessionId[indexInDataArray] && namesUsed().find(auxPlayerName) != namesUsed().end()) {
      // datagram isn't ignored
      hasPlayerResponded[indexInDataArray] = true;
      turnDirection[indexInDataArray] = auxTurnDirection;
      sendDatagrams(nextExpectedEvenNo, sock);
    }
  }

  inline int findFreeIndex() {
    for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
      if (!isPlayerActive[i]) {
        // It also can't be a bot.
        return i;
      }
    }

    return ERROR;
  }

  void setClientAddress(int index) {
    memset(&clientAddress[index], 0, sizeof(struct sockaddr_storage));
    memcpy(&clientAddress[index], &auxClientAddress, sizeof(struct sockaddr_storage));
  }

  void setDisconnectionTimer(int index) {
    getCurrentTime();
    //set timer
    newValue[index].it_value.tv_sec = 0;
    newValue[index].it_value.tv_nsec = 0; // first expiration time
    newValue[index].it_interval.tv_sec = DIS_TIME_SEC; // period
    newValue[index].it_interval.tv_nsec = 1; // first expiration time
    if (timerfd_settime(pfds[index].fd, 0, &newValue[index], &oldValue) == -1) {
      syserr("timerfd_settime");
    }
  }

  void processNewPlayer(uint64_t auxSessionId, uint8_t auxTurnDirection, std::string const &auxPlayerName, int sock) {
    // find free index in the data array
    int index = findFreeIndex();
    if (index == ERROR) {
      fatal("index in processNewPlayer");
    }

    // new activity being made
    getCurrentTime();
    hasPlayerResponded[index] = true;
    isPlayerActive[index] = true;
    setDisconnectionTimer(index);

    // set data
    sessionId[index] = auxSessionId;
    turnDirection[index] = auxTurnDirection;
    playerName[index] = auxPlayerName;
    setClientAddress(index);
    activePlayersNum++;
    if (!auxPlayerName.empty()) {
      namesUsed().insert(auxPlayerName);
    }

    std::cout << "new player: " << index << " " << playerName[index] << std::endl;

    if (gamePlayed) {
      // player is a spectator in the current game, send him all of the datagrams connected to the current match
      sendDatagrams(0, sock);
    }
  }

  void checkDatagram(int sock) {
    rcvaLen = sizeof(struct sockaddr_storage);
    flags = 0; // we do net request anything special

    memset(buffer, 0, BUFFER_SIZE);
    memset(&auxClientAddress, 0, sizeof(struct sockaddr_storage));
    len = recvfrom(sock, buffer, sizeof(buffer), flags, (sockaddr *) &auxClientAddress, &rcvaLen);

    // variables to store client's data
    uint64_t auxSessionId = 0;
    uint8_t auxTurnDirection = 0;
    uint32_t nextExpectedEventNo = 0;
    std::string auxPlayerName;

    if (len > 0 && isDatagramCorrect(len, &auxSessionId, &auxTurnDirection, &nextExpectedEventNo, auxPlayerName)) {
      // datagram is correct but still might be ignored
      std::pair<int, int> clientConnected = isClientConnected();

      if (clientConnected.first == SUCCESS && auxSessionId <= sessionId[clientConnected.second]) {
        newDatagramFromAConnectedClient(auxSessionId, auxTurnDirection,
                                        nextExpectedEventNo, auxPlayerName, clientConnected.second, sock);
      } else if (!clientConnected.first && MAX_NUM_OF_PLAYERS > activePlayersNum) {
        processNewPlayer(auxSessionId, auxTurnDirection, auxPlayerName, sock);
      }
    }
  }

  // counts number of players that want to participate in a game
  inline int numberOfReadyPlayers() {
    int counter = 0;
    for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
      if (isPlayerActive[i] && !playerName[i].empty() && turnDirection[i] != 0) {
        counter++;
      }
    }

    return counter;
  }

  uint32_t deterministicRand() {
    uint32_t currentRandomNumber = randomNumber;
    randomNumber = (randomNumber * RAND_MULTIPLIER) % RAND_MODULO;

    return currentRandomNumber;
  }

  inline void addPlayerEliminatedEvent(uint8_t playerNum) {
    Event *aux = new PlayerEliminated(events().size(), PLAYER_ELIMINATED, playerNum, playingPlayerName[playerNum]);
    events().push_back(aux);
    playersInTheGame--;
    isPlayerDead[playerNum] = true;
  }

  inline void addPixelEvent(uint8_t playerNum, uint16_t x, uint16_t y) {
    Event *aux = new Pixel(events().size(), PIXEL, playerNum, playingPlayerName[playerNum], x, y);
    events().push_back(aux);
  }

  inline void addGameOverEvent() {
    Event *aux = new GameOver(events().size(), GAME_OVER);
    events().push_back(aux);
    gamePlayed = false;
    disarmATimer(DATA_ARR_SIZE - 1);
  }

  inline void updateCurrentlyPlayingPlayers(std::vector<std::string> const &players) {
    for (int i = 0; i < DATA_ARR_SIZE - 1; i++) {
      playingPlayerDataIndex[i] = -1;
    }

    for (uint32_t i = 0; i < players.size(); i++) {
      playingPlayerName[i] = players[i];

      for (int j = 1; j < DATA_ARR_SIZE - 1; j++) {
        if (playerName[j] == playingPlayerName[i]) {
          playingPlayerDataIndex[i] = j;
          break;
        }
      }
    }

    for (int i = 0; i < DATA_ARR_SIZE - 1; i++) {
      isPlayerDead[i] = false;
    }
  }

  // send new event to all the players
  void sendEvent(int sock) {
    for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
      if (isPlayerActive[i]) {
        memcpy(&auxClientAddress, &clientAddress[i], sizeof(struct sockaddr_storage));
        sendDatagrams(events().size() - 1, sock);
      }
    }
  }

  inline void setRoundTimer(time_t nanoSecPeriod) {
    //set timer
    newValue[DATA_ARR_SIZE - 1].it_value.tv_sec = 0;
    newValue[DATA_ARR_SIZE - 1].it_value.tv_nsec = nanoSecPeriod; // first expiration time
    newValue[DATA_ARR_SIZE - 1].it_interval.tv_sec = 0;
    newValue[DATA_ARR_SIZE - 1].it_interval.tv_nsec = nanoSecPeriod; // period
    if (timerfd_settime(pfds[DATA_ARR_SIZE - 1].fd, 0, &newValue[DATA_ARR_SIZE - 1], NULL) == -1) {
      syserr("timerfd_settime");
    }
  }

  void newGame(time_t nanoSecPeriod, uint32_t boardWidth, uint32_t boardHeight, int sock) {
    if (!gamePlayed && numberOfReadyPlayers() >= MIN_NUM_OF_PLAYERS_TO_START_A_GAME) {
      gamePlayed = true;

      for (auto u : events()) {
        delete u;
      }

      events().clear(); // new game, delete events from a previous game
      squaresUsed().clear(); // new game, clear the board
      gameId = deterministicRand();
      playersInTheGame = 0;
      std::vector<std::string> players(namesUsed().begin(), namesUsed().end());
      updateCurrentlyPlayingPlayers(players);

      Event *aux = new NewGame(0, NEW_GAME, boardWidth, boardHeight, players);
      setRoundTimer(nanoSecPeriod);
      events().push_back(aux);
      sendEvent(sock);

      for (int i = 1; i < DATA_ARR_SIZE - 1; i++) {
        takesPartInTheCurrentGame[i] = is[i] != 0 && namesUsed().find(playerName[i]) != namesUsed().end();
      }

      for (int i = 0; i < DATA_ARR_SIZE - 1; i++) {
        auto index = playingPlayerDataIndex[i];

        if (index != -1 && takesPartInTheCurrentGame[index]) {
          playersInTheGame++; // one more player
          playerWormX[index] = (deterministicRand() % boardWidth) + 0.5;
          playerWormY[index] = (deterministicRand() % boardHeight) + 0.5;
          roundedPlayerWormX[index] = std::floor(playerWormX[index]);
          roundedPlayerWormY[index] = std::floor(playerWormY[index]);
          playerDirection[index] = deterministicRand() % FULL_CIRCLE;

          if (squaresUsed().find({roundedPlayerWormX[index], roundedPlayerWormY[index]}) != squaresUsed().end()) {
            addPlayerEliminatedEvent(index);
          } else {
            squaresUsed().insert({roundedPlayerWormX[index], roundedPlayerWormY[index]});
            addPixelEvent(i, roundedPlayerWormX[index], roundedPlayerWormY[index]);
          }
          sendEvent(sock);
        }
      }

      if (playersInTheGame == 1) {
        // game finished
        addGameOverEvent();
        sendEvent(sock);
      }
    }
  }
}

void server(uint16_t portNum, int64_t seed, uint8_t turningSpeed,
            uint8_t roundsPerSecond, uint16_t boardWidth, uint16_t boardHeight) {
  int sock; // socket descriptor
  int ready; // variable to store poll return value
  struct sockaddr_in6 serverAddress;
  randomNumber = seed; // first random number equals to the seed's value

  sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP); // creating IPv6 UDP socket

  if (sock < 0) {
    syserr("server socket creation");
  }

  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin6_family = AF_INET6;
  serverAddress.sin6_addr = in6addr_any; // listening on all interfaces
  serverAddress.sin6_port = htons(portNum);

  int v6OnlyEnabled = 0;  // disable v-6 only mode
  if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &v6OnlyEnabled, sizeof(v6OnlyEnabled)) != 0) {
    syserr("setsockopt");
  }

  // bind the socket to a concrete address
  if (bind(sock, (struct sockaddr *) &serverAddress, (socklen_t) sizeof(serverAddress)) < 0) {
    syserr("server socket bind, address taken.");
  }

  // socket is non-blocking
  if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
    syserr("server fctl failed");
  }

  signal(SIGPIPE, SIG_IGN); // Ignoring SIGPIPE.
  setPollfdArray(sock); // sets the array that will be used for polling

  for (;;) {
    ready = poll(pfds, nfds, -1); // wait as long as needed
    if (ready == -1) {
      syserr("poll error");
    }

    /* Checks whether timer for the next round had expired.
     * If that's the case needed operations are performed.
     */
    if (gamePlayed) {
      checkNextTurn(turningSpeed, boardWidth, boardHeight, sock);
    }
    //checkDisconnection(); // check for disconnected clients
    checkDatagram(sock); // check for something to read in the socket
    newGame(NANO_SEC / roundsPerSecond, boardWidth, boardHeight, sock); // check the possibility of starting a new game
  }

  if (close(sock) == -1) { // very rare errors can occur here, but then
    syserr("close"); // it's healthy to do the check
  }
}
