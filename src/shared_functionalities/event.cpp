#include "event.h"
#include "../server/parsing_functionalities.h"
#include "parsing_functionalities.h"
#include "crc32.h"

namespace {
  void generateEventInfo(std::string &datagram, uint32_t eventNo, uint8_t eventType) {
    auto byteArray = toByte(eventNo);
    datagram = std::string(byteArray.begin(), byteArray.end());
    byteArray = toByte(eventType);
    datagram += std::string(byteArray.begin(), byteArray.end());
  }

  std::string finalDatagram(std::string &eventDatagramPart) {
    std::string eventPartLength;
    uint32_t len = eventDatagramPart.size();

    addNumber(eventPartLength, len);
    eventDatagramPart = eventPartLength + eventDatagramPart;
    addNumber(eventDatagramPart, crc32(eventDatagramPart.c_str(), sizeof(eventDatagramPart.c_str())));

    return eventDatagramPart;
  }
}

std::string NewGame::getByteRepresentationServer() const noexcept {
  std::string eventDatagramPart; // variable to store part of datagram data
  generateEventInfo(eventDatagramPart, eventNo, eventType);

  addNumber(eventDatagramPart, maxx);
  addNumber(eventDatagramPart, maxy);
  for (auto const &u : playersNames) {
    eventDatagramPart += u + '\0';
  }

  return finalDatagram(eventDatagramPart);
}

std::string NewGame::getByteRepresentationClient() const noexcept {
  std::string eventDatagram = "NEW_GAME ";
  addNumber(eventDatagram, maxx);
  eventDatagram += " ";
  addNumber(eventDatagram, maxy);

  for (const auto &u : playersNames) {
    eventDatagram += " " + u;
  }
  eventDatagram += "\n";

  return eventDatagram;
}

std::string Pixel::getByteRepresentationServer() const noexcept {
  std::string eventDatagramPart; // variable to store part of datagram data
  std::string eventPartLength;
  generateEventInfo(eventDatagramPart, eventNo, eventType);

  addNumber(eventDatagramPart, x);
  addNumber(eventDatagramPart, y);

  return finalDatagram(eventDatagramPart);
}

std::string Pixel::getByteRepresentationClient() const noexcept {
  std::string eventDatagram = "PIXEL ";
  addNumber(eventDatagram, x);
  eventDatagram += " ";
  addNumber(eventDatagram, y);
  eventDatagram += " " + playerName + "\n";

  return eventDatagram;
}

std::string PlayerEliminated::getByteRepresentationServer() const noexcept {
  std::string eventDatagramPart; // variable to store part of datagram data
  std::string eventPartLength;
  generateEventInfo(eventDatagramPart, eventNo, eventType);

  addNumber(eventDatagramPart, playerNum);

  return finalDatagram(eventDatagramPart);
}

std::string PlayerEliminated::getByteRepresentationClient() const noexcept {
  std::string eventDatagram = "PLAYER_ELIMINATED";
  eventDatagram += " " + playerName + "\n";

  return eventDatagram;
}

std::string GameOver::getByteRepresentationServer() const noexcept {
  std::string eventDatagramPart; // variable to store part of datagram data
  std::string eventPartLength;
  generateEventInfo(eventDatagramPart, eventNo, eventType);

  return finalDatagram(eventDatagramPart);
}

std::string GameOver::getByteRepresentationClient() const noexcept {
  return "GAME_OVER\n";
}

