#include "parsing_functionalities.h"
#include "../parsing_functionalities/parsing_functionalities.h"

#define OK 0
#define ERROR -1
#define DECIMAL_BASIS 10

bool checkNameCorrectness(std::string const &playerName) {
  for (auto const &u : playerName) {
    if (!(u >= 33 && u <= 126)) {
      return false;
    }
  }

  return true;
}

int setProgramParameters(std::string &playerName, uint16_t *gameServerPort,
                         std::string &guiServer, uint16_t *guiServerPort) {

  return OK;
}

