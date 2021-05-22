#ifndef EVENT_H
#define EVENT_H


#include <vector>
#include <string>


class Event {
  public:
    virtual ~Event() = default;
    virtual std::string getByteRepresentation() const noexcept = 0;
  private:
    uint32_t len;
    uint32_t eventNo;
    uint8_t eventType;
    uint32_t crc32;
};

class NewGame : public Event {
  public:
    virtual ~NewGame() = default;
    std::string getByteRepresentation() const noexcept override;
  private:
    uint32_t maxx;
    uint32_t maxy;
    std::vector<std::string> playersNames;
};

class Pixel : public Event {
  public:
    virtual ~NewGame() = default;
    std::string getByteRepresentation() const noexcept override;
  private:
    uint8_t playerNum;
    uint32_t x;
    uint32_t y;
};

class PlayerEliminated : public Event {
  public:
    virtual ~NewGame() = default;
    std::string getByteRepresentation() const noexcept override;
  private:
    uint8_t playerNum;
};

class GameOver : public Event {
  public:
    virtual ~NewGame() = default;
    std::string getByteRepresentation() const noexcept override;
};


#endif /* EVENT_H */
