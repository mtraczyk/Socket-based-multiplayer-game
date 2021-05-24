#ifndef EVENT_H
#define EVENT_H


#include <utility>
#include <vector>
#include <string>


class Event {
  public:
    Event(uint32_t mEventNo, uint8_t mEventType) : eventNo(mEventNo), eventType(mEventType) {}

    virtual ~Event() = default;
    virtual std::string getByteRepresentationServer() const noexcept = 0;
    virtual std::string getByteRepresentationClient() const noexcept = 0;
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
    std::string getByteRepresentationServer() const noexcept override;
    std::string getByteRepresentationClient() const noexcept override;
  private:
    uint32_t maxx;
    uint32_t maxy;
    playersNameCollection playersNames;
};

class Pixel : public Event {
  public:
    Pixel(uint32_t mEventNo, uint8_t mEventType, uint8_t mPlayerNum, std::string mPlayerName,
          uint32_t mX, uint32_t mY) : Event(mEventNo, mEventType),
                                      playerNum(mPlayerNum), playerName(std::move(mPlayerName)), x(mX), y(mY) {}

    virtual ~Pixel() = default;
    std::string getByteRepresentationServer() const noexcept override;
    std::string getByteRepresentationClient() const noexcept override;
  private:
    uint8_t playerNum;
    std::string playerName;
    uint32_t x;
    uint32_t y;
};

class PlayerEliminated : public Event {
  public:
    PlayerEliminated(uint32_t mEventNo, uint8_t mEventType, uint8_t mPlayerNum, std::string mPlayerName) :
      Event(mEventNo, mEventType), playerNum(mPlayerNum), playerName(std::move(mPlayerName)) {}

    virtual ~PlayerEliminated() = default;
    std::string getByteRepresentationServer() const noexcept override;
    std::string getByteRepresentationClient() const noexcept override;
  private:
    uint8_t playerNum;
    std::string playerName;
};

class GameOver : public Event {
  public:
    GameOver(uint32_t mEventNo, uint8_t mEventType) : Event(mEventNo, mEventType) {}

    virtual ~GameOver() = default;
    std::string getByteRepresentationServer() const noexcept override;
    std::string getByteRepresentationClient() const noexcept override;
};


#endif /* EVENT_H */
