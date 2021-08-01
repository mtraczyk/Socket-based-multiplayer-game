#ifndef SHARED_PARSING_FUNCTIONALITIES_H
#define SHARED_PARSING_FUNCTIONALITIES_H

#include <vector>
#include <string>
#include <arpa/inet.h>
#include <iostream>

using byte = uint8_t;

int numFromArg(int64_t *value, const char *arg, int64_t minValue, int64_t maxValue);

template<typename T>
std::vector<byte> toByte(T input) {
  byte *bytePointer = reinterpret_cast<byte *>(&input);
  return std::vector<byte>(bytePointer, bytePointer + sizeof(T));
}

template<typename T>
void addNumber(std::string &datagram, T number) {
  std::cout << "addNumber debug info" << std::endl;
  std::cout << number << " " << htonl(number) << std::endl;
  number = htonl(number);
  auto byteArray = toByte(number);

  datagram += std::string(byteArray.begin(), byteArray.end());
}

#endif /* SHARED_PARSING_FUNCTIONALITIES_H */
