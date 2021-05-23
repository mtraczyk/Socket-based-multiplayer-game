#include <cstdint>
#include <string>

#include "../err/err.h"
#include "parsing_functionalities.h"
#include "client.h"

#define DEFAULT_GAME_SERVER_PORT 20210
#define DEFAULT_GUI_SERVER "localhost"
#define DEFAULT_GUI_SERVER_PORT 20210

int main(int argc, char **argv) {
  if (argc == 0) {
    fatal("Usage: %s game_server ...\n", argv[0]);
  }

  // Program parameters.
  std::string gameServer = argv[1];
  uint16_t myPortNum = 0; // bind to 0 will use first free port which client can use
  std::string playerName;
  uint16_t gameServerPort = DEFAULT_GAME_SERVER_PORT;
  std::string guiServer = DEFAULT_GUI_SERVER;
  uint16_t guiServerPort = DEFAULT_GUI_SERVER_PORT;

  if ((setProgramParameters(playerName, &gameServerPort, guiServer, &guiServerPort) < 0) ||
      !checkNameCorrectness(playerName)) {
    fatal("Incorrect program parameters!\nUsage: "
          "./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]");
  }

  client(gameServer, myPortNum, playerName, gameServerPort, guiServer, guiServerPort);

  return 0;
}