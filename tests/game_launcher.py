from dataclasses import dataclass
import subprocess
import time
import sys


DISABLE_STDOUT = True

@dataclass
class ServerLaunchOptions:
    port = None
    seed = None
    turning_speed = None
    rounds_per_sec = None
    maxx = None
    maxy = None

    def fill_defaults(self):
        ret = ServerLaunchOptions()
        ret.port = self.port if self.port is not None else 2021
        ret.seed = self.seed
        ret.turning_speed = self.turning_speed if self.turning_speed is not None else 6
        ret.rounds_per_sec = self.rounds_per_sec if self.rounds_per_sec is not None else 50
        ret.maxx = self.maxx if self.maxx is not None else 640
        ret.maxy = self.maxy if self.maxy is not None else 480
        return ret


class ServerInstance:
    proc: subprocess.Popen
    opts: ServerLaunchOptions

    def __init__(self, proc, opts):
        self.proc = proc
        self.opts = opts.fill_defaults()

    def kill(self):
        self.proc.kill()
        self.proc.wait()


def launch_server(opts: ServerLaunchOptions):
    args = ["./screen-worms-server"]
    if opts.port is not None:
        args += ["-p", str(opts.port)]
    if opts.seed is not None:
        args += ["-s", str(opts.seed)]
    if opts.turning_speed is not None:
        args += ["-t", str(opts.turning_speed)]
    if opts.rounds_per_sec is not None:
        args += ["-v", str(opts.rounds_per_sec)]
    if opts.maxx is not None:
        args += ["-w", str(opts.maxx)]
    if opts.maxy is not None:
        args += ["-h", str(opts.maxy)]
    ret = ServerInstance(subprocess.Popen(args, stdout=(subprocess.DEVNULL if DISABLE_STDOUT else sys.stdout)), opts)
    time.sleep(0.05)
    return ret


@dataclass
class ClientLaunchOptions:
    game_server = None
    game_server_port = None
    ui_server = None
    ui_server_port = None
    player_name = None

    def fill_defaults(self):
        ret = ClientLaunchOptions()
        ret.game_server_port = self.game_server_port if self.game_server_port is not None else 2021
        ret.ui_server = self.ui_server if self.ui_server is not None else "localhost"
        ret.ui_server_port = self.ui_server_port if self.ui_server_port is not None else 20210
        ret.player_name = self.player_name if self.player_name is not None else ""
        return ret


class ClientInstance:
    proc: subprocess.Popen
    opts: ClientLaunchOptions

    def __init__(self, proc, opts):
        self.proc = proc
        self.opts = opts.fill_defaults()

    def kill(self):
        self.proc.kill()
        self.proc.wait()
        self.proc.stderr.close()

    def expect_error_exit(self):
        self.proc.wait(0.1)
        if self.proc.returncode != 1:
            raise Exception(f'proc exited with invalid exit code: {self.proc.returncode}')
        if self.proc.stderr.read() == b'':
            raise Exception(f'proc exited with exit code 1 but no error message was written to stderr')
        self.proc.stderr.close()


def launch_client(opts: ClientLaunchOptions):
    args = ["./screen-worms-client", opts.game_server]
    if opts.game_server_port is not None:
        args += ["-p", str(opts.game_server_port)]
    if opts.ui_server is not None:
        args += ["-i", opts.ui_server]
    if opts.ui_server_port is not None:
        args += ["-r", str(opts.ui_server_port)]
    if opts.player_name is not None:
        args += ["-n", opts.player_name]
    ret = ClientInstance(subprocess.Popen(args, stdout=(subprocess.DEVNULL if DISABLE_STDOUT else sys.stdout), stderr=subprocess.PIPE), opts)
    return ret

