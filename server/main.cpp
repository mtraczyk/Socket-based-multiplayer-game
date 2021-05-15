#include <iostream>
#include <ctime>
#include "err.h"
#include "parsing_functionalities.h"

#define DEFAULT_PORT_NUM 2021
#define DEFAULT_TURNING_SPEED 6
#define DEFAULT_ROUNDS_PER_SECOND 50
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

int main(int argc, char **argv) {
  // Program parameters.
  uint32_t portNum = DEFAULT_PORT_NUM; // port number
  uint32_t seed = time(nullptr); // default seed equals time(nullptr)
  int32_t turningSpeed = DEFAULT_TURNING_SPEED;
  uint32_t roundsPerSecond = DEFAULT_ROUNDS_PER_SECOND;
  uint32_t boardWidth = DEFAULT_WIDTH; // width in pixels
  uint32_t boardHeight = DEFAULT_HEIGHT; // height in pixels

  if (setProgramParameters(argc, argv, &portNum, &seed,
                           &turningSpeed, &roundsPerSecond, &boardWidth, &boardHeight) < 0) {
    fatal("Wrong program parameters");
  }

  return 0;
}