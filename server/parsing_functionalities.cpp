#include <unistd.h>
#include <string>
#include <iostream>
#include <limits>
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

int setProgramParameters(int argc, char *const argv[], uint32_t *portNum, int64_t *seed, int64_t *turningSpeed,
                         uint32_t *roundsPerSecond, uint32_t *boardWidth, uint32_t *boardHeight) {
  std::string optstring = "p:s:t:v:w:h:";
  int option;
  int64_t auxValue;

  while ((option = getopt(argc, argv, optstring.c_str())) > 0) {
    auxValue = 0;

    switch (option) {
      case PORT:
        if (numFromArg(&auxValue, optarg, 0, std::numeric_limits<uint16_t>::max()) < 0) {
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
        if (numFromArg(&auxValue, optarg, std::numeric_limits<int32_t>::min(),
                       std::numeric_limits<uint32_t>::max()) < 0) {
          return ERROR;
        }
        *turningSpeed = auxValue;
        break;
      case ROUNDS_PER_SEC:
        if (numFromArg(&auxValue, optarg, 0, std::numeric_limits<uint16_t>::max()) < 0) {
          return ERROR;
        }
        *roundsPerSecond = auxValue;
        break;
      case WIDTH:
        if (numFromArg(&auxValue, optarg, 0, std::numeric_limits<uint32_t>::max()) < 0) {
          return ERROR;
        }
        *boardWidth = auxValue;
        break;
      case HEIGHT:
        if (numFromArg(&auxValue, optarg, 0, std::numeric_limits<uint32_t>::max()) < 0) {
          return ERROR;
        }
        *boardHeight = auxValue;
        break;
      default:
        return ERROR;
    }

    std::cout << (char) option << " " << auxValue << std::endl;
  }

  return OK;
}