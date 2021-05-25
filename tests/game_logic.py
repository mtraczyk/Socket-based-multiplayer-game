from protocol import *
from dataclasses import dataclass
import math


class GameRandom:
    def __init__(self, seed):
        self.state = seed

    def reset(self, seed):
        self.state = seed

    def next(self):
        ret = self.state
        self.state = (self.state * 279410273) % 4294967291
        return ret


@dataclass
class GameInsect:
    index: int
    player_name: str
    x: float = 0
    y: float = 0
    dir: int = 0
    eliminated: bool = False


class GameLogic:
    def __init__(self, random, maxx, maxy, turning_speed):
        self.game_id = None
        self.random = random
        self.maxx = maxx
        self.maxy = maxy
        self.turning_speed = turning_speed
        self.events = []
        self.insects = []
        self.dead_insects = 0
        self.eaten = set()

    def event(self, event):
        event.event_no = len(self.events)
        self.events.append(event)

    def begin(self, player_names):
        self.game_id = self.random.next()
        sorted_names = list(sorted(player_names))
        self.event(NewGameEvent(self.maxx, self.maxy, sorted_names))
        self.insects = [GameInsect(i, name) for i, name in enumerate(sorted_names)]
        self.eaten.clear()
        for insect in self.insects:
            insect.x = (self.random.next() % self.maxx) + 0.5
            insect.y = (self.random.next() % self.maxy) + 0.5
            insect.dir = self.random.next() % 360
            self.handle_move(insect)

    def round(self, turn_directions):
        if self.dead_insects == len(self.insects) - 1:
            return
        for insect in self.insects:
            turn_direction = turn_directions[insect.player_name]
            if turn_direction == TurnDirection.RIGHT:
                insect.dir = (insect.dir + self.turning_speed) % 360
            elif turn_direction == TurnDirection.LEFT:
                insect.dir = (insect.dir - self.turning_speed) % 360
            old_pos = (int(insect.x), int(insect.y))
            insect.x += math.cos(insect.dir / 180 * math.pi)
            insect.y += math.sin(insect.dir / 180 * math.pi)
            # print(f"PyPlayer {insect.index} {insect.x} {insect.y} {insect.dir} {turn_direction.value}")
            new_pos = (int(insect.x), int(insect.y))
            if old_pos != new_pos:
                self.handle_move(insect)
                if self.dead_insects == len(self.insects) - 1:
                    return

    def handle_move(self, insect):
        eliminated = False
        x, y = int(insect.x), int(insect.y)
        if insect.x < 0 or insect.y < 0 or x < 0 or y < 0 or x >= self.maxx or y >= self.maxy:
            eliminated = True
        if (x, y) in self.eaten:
            eliminated = True
        if eliminated:
            self.event(PlayerEliminatedEvent(insect.index))
            insect.eliminated = True
            self.dead_insects += 1
            if self.dead_insects == len(self.insects) - 1:
                self.event(GameOverEvent())
        else:
            self.event(PixelEvent(insect.index, x, y))
            self.eaten.add((x, y))


