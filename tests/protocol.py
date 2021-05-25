from dataclasses import dataclass, field
from enum import Enum
import struct
import zlib
from typing import Optional, List


class PacketStructureLog:
    def __init__(self):
        self.buf = ''

    def field(self, data, label):
        self.buf += data.hex().ljust(10) + ' ' + label + '\n'


class TurnDirection(Enum):
    STRAIGHT = 0
    RIGHT = 1
    LEFT = 2


@dataclass
class C2SMsg:
    session_id: Optional[int]
    turn_direction: Optional[TurnDirection]
    next_expected_event_no: Optional[int]
    player_name: Optional[str]

    def encode(self):
        ret = struct.pack(">QBI", self.session_id, self.turn_direction.value, self.next_expected_event_no)
        ret += self.player_name.encode()
        return ret

    def decode(self, data):
        (self.session_id, turn_dir_int, self.next_expected_event_no) = struct.unpack(">QBI", data[:13])
        self.turn_direction = TurnDirection(turn_dir_int)
        self.player_name = data[13:]


class Event:
    event_no: Optional[int]
    crc: Optional[int]

    def __init__(self):
        self.event_no = None
        self.crc = None

    @property
    def expected_crc(self):
        data = struct.pack(">IB", self.event_no, self.event_type) + self.encode()
        expected_crc = zlib.crc32(struct.pack(">I", len(data)) + data) % (1 << 32)
        return expected_crc

    @property
    def crc_valid(self):
        return self.crc == self.expected_crc

    @property
    def event_type(self) -> int:
        raise NotImplementedError()

    def encode(self):
        raise NotImplementedError()

    def decode(self, data, log):
        raise NotImplementedError()

    def encode_full(self):
        ret = struct.pack(">IB", self.event_no, self.event_type) + self.encode()
        crc = self.crc
        if crc is None:
            crc = zlib.crc32(struct.pack(">I", len(ret)) + ret) % (1 << 32)
        ret += struct.pack(">I", crc)
        return ret

    @staticmethod
    def decode_full(data, log):
        log.field(data[:4], 'event_no')
        log.field(data[4:5], 'event_type')
        (event_no, event_type_int) = struct.unpack(">IB", data[:5])

        event = UnknownEvent(event_type_int)
        for t in [NewGameEvent, PixelEvent, PlayerEliminatedEvent, GameOverEvent]:
            if event_type_int == t.event_type:
                event = t()

        event.event_no = event_no
        (event.crc,) = struct.unpack(">I", data[-4:])
        event.decode(data[5:-4], log)
        log.field(data[-4:], 'crc')
        return event


class UnknownEvent(Event):
    event_type_int: int
    event_data: bytes

    def __init__(self, event_type):
        super().__init__()
        self.event_type_int = event_type

    @property
    def event_type(self):
        return self.event_type_int

    def encode(self):
        return self.event_data

    def decode(self, data, log):
        log.field(data, 'ev_data')
        self.event_data = data


class NewGameEvent(Event):
    event_type = 0
    maxx: Optional[int]
    maxy: Optional[int]
    player_names: List[str]

    def __init__(self, maxx=None, maxy=None, player_names=None):
        super().__init__()
        self.maxx = maxx
        self.maxy = maxy
        self.player_names = [] if player_names is None else player_names

    def encode(self):
        ret = struct.pack(">II", self.maxx, self.maxy)
        for p in self.player_names:
            ret += p.encode() + b'\x00'
        return ret

    def decode(self, data, log):
        log.field(data[:4], 'maxx')
        log.field(data[4:8], 'maxy')
        log.field(data[8:], 'player_names')
        (self.maxx, self.maxy) = struct.unpack(">II", data[:8])
        if data[-1] != 0:
            raise Exception("new game must end with a null terminator")
        self.player_names = [x.decode() for x in data[8:-1].split(b'\x00')]

    def __repr__(self):
        return f"NEW_GAME#{self.event_no}(maxx={self.maxx}, maxy={self.maxy}, player_names={self.player_names})"


class PixelEvent(Event):
    event_type = 1
    player_number: Optional[int]
    x: Optional[int]
    y: Optional[int]

    def __init__(self, player_number=None, x=None, y=None):
        super().__init__()
        self.player_number = player_number
        self.x = x
        self.y = y

    def encode(self):
        return struct.pack(">BII", self.player_number, self.x, self.y)

    def decode(self, data, log):
        log.field(data[:1], 'player_number')
        log.field(data[1:5], 'x')
        log.field(data[5:9], 'y')
        (self.player_number, self.x, self.y) = struct.unpack(">BII", data)

    def __repr__(self):
        return f"PIXEL#{self.event_no}(player_number={self.player_number}, x={self.x}, y={self.y})"


class PlayerEliminatedEvent(Event):
    event_type = 2
    player_number: Optional[int]

    def __init__(self, player_number=None):
        super().__init__()
        self.player_number = player_number

    def encode(self):
        return struct.pack(">B", self.player_number)

    def decode(self, data, log):
        log.field(data[:1], 'player_number')
        (self.player_number, ) = struct.unpack(">B", data)

    def __repr__(self):
        return f"PLAYER_ELIMINATED#{self.event_no}(player_number={self.player_number})"


class GameOverEvent(Event):
    event_type = 3

    def encode(self):
        return b""

    def decode(self, data, log):
        if len(data) != 0:
            raise Exception("unexpected data")

    def __repr__(self):
        return f"GAME_OVER#{self.event_no}()"


@dataclass
class S2CMsg:
    game_id: Optional[int] = None
    events: List[Event] = field(default_factory=list)

    def encode(self):
        ret = struct.pack(">I", self.game_id)
        for event in self.events:
            event_data = event.encode_full()
            ret += struct.pack(">I", len(event_data) - 4) + event_data
        return ret

    def decode(self, data, log=PacketStructureLog()):
        log.field(data[:4], 'game_id')
        (self.game_id, ) = struct.unpack(">I", data[:4])
        self.events = []
        o = 4
        while o < len(data):
            log.field(data[o:o+4], 'len')
            (e_len,) = struct.unpack(">I", data[o:o+4])
            e_data = data[o+4:o+4+e_len+4]
            log.field(e_data, '(raw event data)')
            self.events.append(Event.decode_full(e_data, log))
            o += 4 + e_len + 4



