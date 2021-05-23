#include <unistd.h>
#include <string>
#include <iostream>
#include <limits>
#include <vector>
#include "parsing_functionalities.h"

#define OK 0
#define ERROR -1
#define DECIMAL_BASIS 10
#define PORT 'p'
#define SEED 's'
#define TURNING_SPEED 't'
#define ROUNDS_PER_SEC 'v'
#define WIDTH 'w'
#define HEIGHT 'h'
#define MIN_PORT 0
#define MAX_PORT 65535
#define MIN_TURNING 1
#define MAX_TURNING 45
#define MIN_VEL 1
#define MAX_VEL 250
#define MIN_WIDTH 128
#define MAX_WIDTH 1920
#define MIN_HEIGHT 128
#define MAX_HEIGHT 1920

namespace {
  /* atoi like function, with constraints on minimum and maximum value.
   * Returns OK if the number was correctly parsed, ERROR otherwise.
   * Resulting number is stored in value.
   * Leading zeroes are accepted.
   */
  int numFromArg(int64_t *value, const char *arg, int64_t minValue, int64_t maxValue) {
    int64_t auxValue = 0;
    const char *s = arg;
    bool negative = false;

    // Check whether it is a negative number.
    if (*s == '-') {
      negative = true;
      s++;
    }

    // The number has to have at least one digit.
    if (*s == '\0') {
      return ERROR;
    }

    for (; *s != '\0'; ++s) {
      char c = *s;
      if (!('0' <= c && c <= '9')) {
        return ERROR;
      }

      auxValue *= DECIMAL_BASIS;
      auxValue += c - '0';

      if (auxValue > maxValue || (negative && -auxValue < minValue)) {
        return ERROR;
      }
    }

    auxValue = (negative ? -auxValue : auxValue);
    *value = auxValue;

    return OK;
  }
}

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
        if (numFromArg(&auxValue, optarg, std::numeric_limits<int32_t>::min(),
                       std::numeric_limits<uint32_t>::max()) < 0) {
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
