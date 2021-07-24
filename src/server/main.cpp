#include <ctime>
#include <iostream>

#include "../shared_functionalities/err.h"
#include "parsing_functionalities.h"
#include "server.h"

#define DEFAULT_PORT_NUM 2021
#define DEFAULT_TURNING_SPEED 6
#define DEFAULT_ROUNDS_PER_SECOND 50
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

int main(int argc, char **argv) {
  // Program parameters.
  uint16_t portNum = DEFAULT_PORT_NUM; // port number
  int64_t seed = time(nullptr); // default seed equals time(nullptr)
  uint8_t turningSpeed = DEFAULT_TURNING_SPEED;
  uint8_t roundsPerSecond = DEFAULT_ROUNDS_PER_SECOND;
  uint16_t boardWidth = DEFAULT_WIDTH; // width in pixels
  uint16_t boardHeight = DEFAULT_HEIGHT; // height in pixels

  // Set program parameters and check their values.
  if (setProgramParameters(argc, argv, &portNum, &seed,
                           &turningSpeed, &roundsPerSecond, &boardWidth, &boardHeight) < 0) {
    // Incorrect program parameters.
    std::cout << "port num: " << portNum << " seed: " << seed << " turning speed: " << turningSpeed
              << " rounds per second: " << roundsPerSecond << " board width: " << boardWidth << " board height: "
              << boardHeight << std::endl;
    fatal("Incorrect program parameters!\nUsage: ./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]");
  }

  // Start server.
  server(portNum, seed, turningSpeed, roundsPerSecond, boardWidth, boardHeight);

  return 0;
}