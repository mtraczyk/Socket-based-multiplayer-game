#include <cstdint>
#include <iostream>
#include "parsing_functionalities.h"

#define OK 0
#define ERROR -1
#define DECIMAL_BASIS 10

/* atoi like function, with constraints on minimum and maximum value.
 * Returns OK if the number was correctly parsed, ERROR otherwise.
 * Resulting number is stored in value.
 * Leading zeroes are accepted.
 */
int numFromArg(int64_t *value, const char *arg, int64_t minValue, int64_t maxValue) {
  int64_t auxValue = 0;
  const char *s = arg;
  bool negative = false;

  while (isspace(*s)) {
      s++;
  }

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
  }

  if (auxValue > maxValue || (negative && -auxValue < minValue) || (!negative && auxValue < minValue)) {
    return ERROR;
  }

  auxValue = (negative ? -auxValue : auxValue);
  *value = auxValue;

  return OK;
}