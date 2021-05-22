#ifndef EVENT_H
#define EVENT_H


#include <utility>
#include <vector>
#include <string>


class Event {
  public:
    Event(uint32_t mEventNo, uint8_t mEventType) : eventNo(mEventNo), eventType(mEventType) {}

    virtual ~Event() = default;
    virtual std::string getByteRepresentation() const noexcept = 0;
  protected:
    uint32_t eventNo;
    uint8_t eventType;
};

class NewGame : public Event {
  public:
    using playersNameCollection = std::vector<std::string>;

    NewGame(uint32_t mEventNo, uint8_t mEventType, uint32_t mMaxx, uint32_t mMaxy,
            playersNameCollection mPlayersNames) : Event(mEventNo, mEventType), maxx(mMaxx), maxy(mMaxy),
                                                                playersNames(std::move(mPlayersNames)) {}

    virtual ~NewGame() = default;
    std::string getByteRepresentation() const noexcept override;
  private:
    uint32_t maxx;
    uint32_t maxy;
    playersNameCollection playersNames;
};

class Pixel : public Event {
  public:
    Pixel(uint32_t mEventNo, uint8_t mEventType, uint8_t mPlayerNum, uint32_t mX, uint32_t mY) :
      Event(mEventNo, mEventType), playerNum(mPlayerNum), x(mX), y(mY) {}

    virtual ~Pixel() = default;
    std::string getByteRepresentation() const noexcept override;
  private:
    uint8_t playerNum;
    uint32_t x;
    uint32_t y;
};

class PlayerEliminated : public Event {
  public:
    PlayerEliminated(uint32_t mEventNo, uint8_t mEventType, uint8_t mPlayerNum) : Event(mEventNo, mEventType),
                                                                                  playerNum(mPlayerNum) {};
    virtual ~PlayerEliminated() = default;
    std::string getByteRepresentation() const noexcept override;
  private:
    uint8_t playerNum;
};

class GameOver : public Event {
  public:
    GameOver(uint32_t mEventNo, uint8_t mEventType) : Event(mEventNo, mEventType) {}

    virtual ~GameOver() = default;
    std::string getByteRepresentation() const noexcept override;
};


#endif /* EVENT_H */
