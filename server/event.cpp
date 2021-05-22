#include "event.h"
#include "parsing_functionalities.h"

namespace {
  uint32_t crc32(std::string const &input) {
    return 0;
  }
}

std::string NewGame::getByteRepresentation() const noexcept {
  return std::string();
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

