#include "event.h"
#include "parsing_functionalities.h"

namespace {
  uint32_t crc32(std::string const &input) {
    return 0;
  }

  void generateEventInfo(std::string &datagram, uint32_t eventNo, uint8_t eventType) {
    const auto byteArray = toByte(eventNo);
    datagram = std::string(byteArray.begin(), byteArray.end());
    byteArray = toByte(eventType);
    datagram += std::string(byteArray.begin(), byteArray.end());
  }
}

std::string NewGame::getByteRepresentation() const noexcept {
  std::string eventDatagramPart; // variable to store part of datagram data
  generateEventInfo(eventDatagramPart, eventNo, eventType);

  for (auto const &u : playersNames) {
    const auto byteArray = toByte(u);
    eventDatagramPart += std::string(byteArray.begin(), byteArray.end());
  }
  uint32_t len = eventDatagramPart.size();

  auto byteArray = toByte(eventDatagramPart.length());
  eventDatagramPart += std::string(byteArray.begin(), byteArray.end());
  byteArray = toByte(len);

  return std::string(byteArray.begin(), byteArray.end()) + eventDatagramPart;
}

std::string Pixel::getByteRepresentation() const noexcept {
  return std::string();
}

std::string PlayerEliminated::getByteRepresentation() const noexcept {
  return std::string();
}

std::string GameOver::getByteRepresentation() const noexcept {
  return std::string();
}

