#include <unistd.h>
#include <string>
#include <iostream>
#include <limits>
#include <vector>
#include "parsing_functionalities.h"
#include "../shared_functionalities/parsing_functionalities.h"

#define OK 0
#define ERROR -1
#define DECIMAL_BASIS 10
#define PORT 'p'
#define SEED 's'
#define TURNING_SPEED 't'
#define ROUNDS_PER_SEC 'v'
#define WIDTH 'w'
#define HEIGHT 'h'
#define MIN_PORT 1
#define MAX_PORT 65535
#define MIN_SEED 0
#define MAX_SEED std::numeric_limits<uint32_t>::max()
#define MIN_TURNING 1
#define MAX_TURNING 90
#define MIN_VEL 1
#define MAX_VEL 250
#define MIN_WIDTH 16
#define MAX_WIDTH 4096
#define MIN_HEIGHT 16
#define MAX_HEIGHT 4096

int setProgramParameters(int argc, char *const argv[], uint16_t *portNum, int64_t *seed, uint8_t *turningSpeed,
                         uint8_t *roundsPerSecond, uint16_t *boardWidth, uint16_t *boardHeight) {
  std::string optstring = "p:s:t:v:w:h:";
  int option;
  int64_t auxValue;

  while ((option = getopt(argc, argv, optstring.c_str())) > 0) {
    auxValue = 0;

    switch (option) {
      case PORT:
        if (numFromArg(&auxValue, optarg, MIN_PORT, MAX_PORT) < 0) {
          return ERROR;
        }
        *portNum = auxValue;
        break;
      case SEED:
        if (numFromArg(&auxValue, optarg, MIN_SEED, MAX_SEED) < 0) {
          return ERROR;
        }
        *seed = auxValue;
        break;
      case TURNING_SPEED:
        if (numFromArg(&auxValue, optarg, MIN_TURNING, MAX_TURNING) < 0) {
          return ERROR;
        }
        *turningSpeed = auxValue;
        break;
      case ROUNDS_PER_SEC:
        if (numFromArg(&auxValue, optarg, MIN_VEL, MAX_VEL) < 0) {
          return ERROR;
        }
        *roundsPerSecond = auxValue;
        break;
      case WIDTH:
        if (numFromArg(&auxValue, optarg, MIN_WIDTH, MAX_WIDTH) < 0) {
          return ERROR;
        }
        *boardWidth = auxValue;
        break;
      case HEIGHT:
        if (numFromArg(&auxValue, optarg, MIN_HEIGHT, MAX_HEIGHT) < 0) {
          return ERROR;
        }
        *boardHeight = auxValue;
        break;
      default:
        return ERROR;
    }
  }

  return OK;
}
