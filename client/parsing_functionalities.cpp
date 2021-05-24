#include <unistd.h>
#include <string>
#include <iostream>

#include "parsing_functionalities.h"
#include "../parsing_functionalities/parsing_functionalities.h"
#include "../err/err.h"

#define OK 0
#define ERROR -1
#define DECIMAL_BASIS 10
#define MIN_ASCII_IN_PLAYER_NAME 33
#define MAX_ASCII_IN_PLAYER_NAME 126
#define PLAYER_NAME 'n'
#define GAME_SERVER_PORT 'p'
#define GUI_SERVER 'i'
#define GUI_PORT 'r'
#define MIN_PORT 1
#define MAX_PORT 65535

namespace {
  bool checkNameCorrectness(std::string const &playerName) {
    for (auto const &u : playerName) {
      if (!(u >= MIN_ASCII_IN_PLAYER_NAME && u <= MAX_ASCII_IN_PLAYER_NAME)) {
        return false;
      }
    }

    return true;
  }
}

int setProgramParameters(int argc, char *const argv[], std::string &playerName, uint16_t *gameServerPort,
                         std::string &guiServer, uint16_t *guiServerPort) {
  std::string optstring = "n:p:i:r:";
  int option;
  int64_t auxValue;

  while ((option = getopt(argc, argv, optstring.c_str())) > 0) {
    auxValue = 0;

    switch (option) {
      case PLAYER_NAME:
        playerName = optarg;
        if (!checkNameCorrectness(optarg)) {
          fatal("Incorrect player name");
        }
        break;
      case GAME_SERVER_PORT:
        if (numFromArg(&auxValue, optarg, MIN_PORT, MAX_PORT) < 0) {
          return ERROR;
        }
        *gameServerPort = auxValue;
        break;
      case GUI_SERVER:
        guiServer = optarg;
        break;
      case GUI_PORT:
        if (numFromArg(&auxValue, optarg, MIN_PORT, MAX_PORT) < 0) {
          return ERROR;
        }
        *guiServerPort = auxValue;
        break;
      default:
        return ERROR;
    }
  }

  std::cout << "player name: " << playerName << " game server port: " << *gameServerPort << " gui server: " << guiServer
            << " gui server port: " << *guiServerPort;

  return OK;
}

