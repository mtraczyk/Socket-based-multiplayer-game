import subprocess
import unittest
import random
import time

MIN_TURNING_SPEED = 1
MAX_TURNING_SPEED = 90
MIN_ROUNDS_PER_SEC = 1
MAX_ROUNDS_PER_SEC = 250
SERVER_PORT = random.randint(10000, 20000)
LAUNCH_CMD = ['./screen-worms-server', '-p', str(SERVER_PORT)]


print("WARNING: The test does not check the port argument since you might have random stuff running on your computer.")
print("WARNING: Please check that you handle the port argument correctly manually.")


def launch_app(args, expected_ok):
    p = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        p.wait(0.05)
    except subprocess.TimeoutExpired:
        p.kill()
        p.wait()
        if expected_ok:
            return
        else:
            raise Exception('test failed: app has not exited, but it should have; args=' + str(args))
    if expected_ok:
        raise Exception('test failed: app exited but it should not have; args=' + str(args))
    else:
        if p.returncode != 1:
            raise Exception('test failed: app exited with an error code other than 1; args=' + str(args))


class ServerParameterTests(unittest.TestCase):
    def test_basic(self):
        launch_app(LAUNCH_CMD + [], True)
        launch_app(LAUNCH_CMD + ['-s', '1337'], True)
        launch_app(LAUNCH_CMD + ['-t', '10'], True)
        launch_app(LAUNCH_CMD + ['-v', '10'], True)
        launch_app(LAUNCH_CMD + ['-w', '500'], True)
        launch_app(LAUNCH_CMD + ['-h', '500'], True)

        launch_app(LAUNCH_CMD + ['-g', '7'], False)  # non existent param
        launch_app(LAUNCH_CMD + ['garbage'], False)  # unexpected value

        for p in ['-p', '-s', '-t', '-v', '-w', '-h']:
            launch_app(LAUNCH_CMD + [p], False)
            launch_app(LAUNCH_CMD + [p, 'a'], False)
            launch_app(LAUNCH_CMD + [p, '1a'], False)
            launch_app(LAUNCH_CMD + [p, '-1'], False)

    def test_bind_same_port(self):
        p = subprocess.Popen(LAUNCH_CMD, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(0.01)
        try:
            launch_app(LAUNCH_CMD, False)  # reuse of the same port
        finally:
            p.kill()
            p.wait()

    def test_garbage_codes(self):
        for p in ['-p', '-s', '-t', '-v', '-w', '-h']:
            for c in range(1, 256):
                if ord('0') <= c <= ord('9'):
                    continue
                launch_app(LAUNCH_CMD + [p, chr(c)], False)
                launch_app(LAUNCH_CMD + [p, '1' + chr(c)], False)

    def test_board_sizes(self):
        launch_app(LAUNCH_CMD + ['-w', '0'], False)
        launch_app(LAUNCH_CMD + ['-h', '0'], False)
        launch_app(LAUNCH_CMD + ['-w', '0', '-h', '100'], False)
        launch_app(LAUNCH_CMD + ['-w', '16', '-h', '16'], True)
        launch_app(LAUNCH_CMD + ['-w', '100', '-h', '100'], True)
        launch_app(LAUNCH_CMD + ['-w', '100', '-h', '0'], False)
        launch_app(LAUNCH_CMD + ['-w', '500', '-h', '500'], True)
        launch_app(LAUNCH_CMD + ['-w', '50000', '-h', '50000'], False)

    @staticmethod
    def _get_values_to_check(min, max):
        s = set()
        for i in range(0, 100):
            s.add(int(min + (max - min) / 100 * i))
        for i in range(-10, 10):
            s.add(min + i)
            s.add(max + i)
        return list(s)

    def test_turning_speed(self):
        for i in self._get_values_to_check(MIN_TURNING_SPEED, MAX_TURNING_SPEED):
            launch_app(LAUNCH_CMD + ['-t', str(i)], MIN_TURNING_SPEED <= i <= MAX_TURNING_SPEED)

    def test_rounds_per_sec(self):
        for i in self._get_values_to_check(MIN_ROUNDS_PER_SEC, MAX_ROUNDS_PER_SEC):
            launch_app(LAUNCH_CMD + ['-v', str(i)], MIN_ROUNDS_PER_SEC <= i <= MAX_ROUNDS_PER_SEC)

    def test_seed(self):
        for t in [-1337, -1, -0xffffffff, -0x7fffffff, 0x100000000]:
            launch_app(LAUNCH_CMD + ['-s', str(t)], False)
        # launch_app(LAUNCH_CMD + ['-s', '0'], True) - 0 might be valid or not, currently in discussion
        launch_app(LAUNCH_CMD + ['-s', '1'], True)
        launch_app(LAUNCH_CMD + ['-s', '1337'], True)
        launch_app(LAUNCH_CMD + ['-s', str(0xffffffff)], True)


if __name__ == '__main__':
    unittest.main()