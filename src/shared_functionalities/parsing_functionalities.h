#ifndef SHARED_PARSING_FUNCTIONALITIES_H
#define SHARED_PARSING_FUNCTIONALITIES_H

int numFromArg(int64_t *value, const char *arg, int64_t minValue, int64_t maxValue);

template<typename T>
void addNumber(std::string &datagram, T number) {
  auto byteArray = toByte(number);
  datagram += std::string(byteArray.begin(), byteArray.end());
}

#endif /* SHARED_PARSING_FUNCTIONALITIES_H */
