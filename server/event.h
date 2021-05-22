#ifndef EVENT_H
#define EVENT_H


#include <vector>
#include <string>


class Event {
  public:
    Event(uint32_t, uint8_t);
    virtual ~Event() = default;
    virtual std::string getByteRepresentation() const noexcept = 0;
  protected:
    uint32_t len;
    uint32_t eventNo;
    uint8_t eventType;
    uint32_t crc32;
};

class NewGame : public Event {
  public:
    using playersNameCollection = std::vector<std::string>;
    NewGame(uint32_t, uint8_t, uint32_t, uint32_t, playersNameCollection);
    virtual ~NewGame() = default;
    std::string getByteRepresentation() const noexcept override;
  private:
    uint32_t maxx;
    uint32_t maxy;
    playersNameCollection playersNames;
};

class Pixel : public Event {
  public:
    Pixel(uint32_t, uint8_t, uint8_t, uint32_t, uint32_t);
    virtual ~Pixel() = default;
    std::string getByteRepresentation() const noexcept override;
  private:
    uint8_t playerNum;
    uint32_t x;
    uint32_t y;
};

class PlayerEliminated : public Event {
  public:
    PlayerEliminated(uint32_t, uint8_t, uint8_t);
    virtual ~PlayerEliminated() = default;
    std::string getByteRepresentation() const noexcept override;
  private:
    uint8_t playerNum;
};

class GameOver : public Event {
  public:
    GameOver(uint32_t, uint8_t);
    virtual ~GameOver() = default;
    std::string getByteRepresentation() const noexcept override;
};


#endif /* EVENT_H */
