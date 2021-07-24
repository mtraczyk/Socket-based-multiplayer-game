import unittest
import configparser
import subprocess
import time

config = configparser.ConfigParser()


class TestServer100(unittest.TestCase):

	def tearDown(self):
		if self.server.poll() is None:
			self.server.kill()
			self.server.communicate()

	def start_server(self, args: str):
		all_args = [config.get("TESTS_100", "SERVER_PATH")] + args.split(" ")

		out = None if config.getboolean("TESTS_100_DEBUG", "PRINT_SERVER_STDOUT") else subprocess.DEVNULL
		err = None if config.getboolean("TESTS_100_DEBUG", "PRINT_SERVER_STDERR") else subprocess.DEVNULL
		self.server = subprocess.Popen(all_args, stdout=out, stderr=err)
		time.sleep(config.getfloat("TESTS_100", "EXIT_TIME"))

	def do_test(self, args):
		self.start_server(args)
		exit_code = self.server.poll()
		self.assertIsNot(exit_code, None, "Server didn't exit.")
		self.assertEqual(exit_code, 1, f"return code {exit_code}, expected 1.")

	def test_101(self):
		self.do_test("W")

	def test_102(self):
		self.do_test("WI")

	def test_103(self):
		self.do_test("-wI")

	def test_104(self):
		self.do_test("-w -h 600")

	def test_105(self):
		self.do_test("-w a")

	def test_106(self):
		self.do_test("-w -1")

	def test_107(self):
		self.do_test("-w 0")

	def test_108(self):
		self.do_test("-w 800a")

	def test_109(self):
		self.do_test("-w 4294967297")

	def test_110(self):
		self.do_test("H")

	def test_111(self):
		self.do_test("HI")

	def test_112(self):
		self.do_test("-hI")

	def test_113(self):
		self.do_test("-h -w 800")

	def test_114(self):
		self.do_test("-h a")

	def test_115(self):
		self.do_test("-h -1")

	def test_116(self):
		self.do_test("-h 0")

	def test_117(self):
		self.do_test("-h 600a")

	def test_118(self):
		self.do_test("-h 4294967297")

	def test_119(self):
		self.do_test("p")

	def test_120(self):
		self.do_test("pi")

	def test_121(self):
		self.do_test("-pi")

	def test_122(self):
		self.do_test("-p -s 777")

	def test_123(self):
		self.do_test("-p a")

	def test_124(self):
		self.do_test("-p 32769a")

	def test_125(self):
		self.do_test("-p -1")

	def test_126(self):
		self.do_test("-p 65536")

	def test_127(self):
		self.do_test("s")

	def test_128(self):
		self.do_test("si")

	def test_129(self):
		self.do_test("-vi")

	def test_130(self):
		self.do_test("-v -s 777")

	def test_131(self):
		self.do_test("-v a")

	def test_132(self):
		self.do_test("-v 0")

	def test_133(self):
		self.do_test("-v 50a")

	def test_134(self):
		self.do_test("t")

	def test_135(self):
		self.do_test("ti")

	def test_136(self):
		self.do_test("-ti")

	def test_137(self):
		self.do_test("-t -s 777")

	def test_138(self):
		self.do_test("-t a")

	def test_139(self):
		self.do_test("-t 0")

	def test_140(self):
		self.do_test("-t -1")

	def test_141(self):
		self.do_test("-t 6a")

	def test_142(self):
		self.do_test("r")

	def test_143(self):
		self.do_test("ri")

	def test_144(self):
		self.do_test("-si")

	def test_145(self):
		self.do_test("-s -v 50")

	def test_146(self):
		self.do_test("-s a")

	def test_147(self):
		self.do_test("-s 1a")

	def test_148(self):
		self.do_test("-a")


if __name__ == '__main__':
	config.read("test_config.ini")
	unittest.main()
