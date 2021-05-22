#ifndef PARSING_FUNCTIONALITIES_H
#define PARSING_FUNCTIONALITIES_H

#include <cstdint>
#include <vector>

using byte = uint8_t;

// Sets program parameters, if some parameter is incorrect returns -1, otherwise returns 0.
int setProgramParameters(int argc, char *const argv[], uint32_t *, int64_t *,
                         int64_t *, uint32_t *, uint32_t *, uint32_t *);

template<typename T>
std::vector<byte> toByte(T input) {
  byte *bytePointer = reinterpret_cast<byte *>(&input);
  return std::vector<byte>(bytePointer, bytePointer + sizeof(T));
}

#endif /* PARSING_FUNCTIONALITIES_H */
