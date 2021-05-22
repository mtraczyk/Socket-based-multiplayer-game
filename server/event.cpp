#include "event.h"

std::string NewGame::getByteRepresentation() const noexcept {
  return std::string();
}

NewGame::NewGame(uint32_t, uint8_t, uint32_t, uint32_t, NewGame::playersNameCollection) {

}

std::string Pixel::getByteRepresentation() const noexcept {
  return std::string();
}

Pixel::Pixel(uint32_t, uint8_t, uint8_t, uint32_t, uint32_t) {

}


std::string PlayerEliminated::getByteRepresentation() const noexcept {
  return std::string();
}

PlayerEliminated::PlayerEliminated(uint32_t, uint8_t, uint8_t) {

}

std::string GameOver::getByteRepresentation() const noexcept {
  return std::string();
}

GameOver::GameOver(uint32_t, uint8_t) {
  Event();
}


Event::Event(uint32_t, uint8_t) {

}
