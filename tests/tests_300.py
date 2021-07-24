import unittest
import configparser
import subprocess
import time

config = configparser.ConfigParser()


class TestClient300(unittest.TestCase):

	def tearDown(self) -> None:
		if self.client.poll() is None:
			self.client.kill()
			self.client.communicate()

	def start_client(self, args: str):
		all_args = [config.get("TESTS_300", "CLIENT_PATH")] + args.split(" ")

		out = None if config.getboolean("TESTS_300_DEBUG", "PRINT_CLIENT_STDOUT") else subprocess.DEVNULL
		err = None if config.getboolean("TESTS_300_DEBUG", "PRINT_CLIENT_STDERR") else subprocess.DEVNULL
		self.client = subprocess.Popen(all_args, stdout=out, stderr=err)
		time.sleep(config.getfloat("TESTS_300", "EXIT_TIME"))

	def do_test(self, args: str):
		self.start_client(args)
		exit_code = self.client.poll()
		self.assertIsNot(exit_code, None, "Client didn't exit.")
		self.assertEqual(exit_code, 1, f"return code {exit_code}, expected 1.")

	def test_301(self):
		self.do_test("")

	def test_302(self):
		self.do_test("Alicja01")

	def test_303(self):
		self.do_test("127.0.0 -n Alicja02")

	def test_304(self):
		self.do_test("127.0.0.1 -p a -n Alicja03")

	def test_305(self):
		self.do_test("127.0.0.1 -p -1 -n Alicja04")

	def test_306(self):
		self.do_test("127.0.0.1 -p 65536 -n Alicja05")

	def test_307(self):
		self.do_test("127.0.0.1 -p 32770a -n Alicja06")

	def test_308(self):
		self.do_test("127.0.0.1 -p 32770 -i 127.0.0 -n Alicja07")

	def test_309(self):
		self.do_test("127.0.0.1 -p 32770 -i 127.0.0.1 -r a -n Alicja08")

	def test_310(self):
		self.do_test("127.0.0.1 -p 32770 -i 127.0.0.1 -r -1 -n Alicja09")

	def test_311(self):
		self.do_test("127.0.0.1 -p 32770 -i 127.0.0.1 -r 65536 -n Alicja10")

	def test_312(self):
		self.do_test("127.0.0.1 -p 32770 -i 127.0.0.1 -r 32769a -n Alicja11")

	def test_313(self):
		self.do_test("ge80::a800:ff:fe54 -p 2238 -n Alicja12")

	def test_314(self):
		self.do_test("127.0.0.1 -p 32770 -i ge80::a800:ff:fe54 -r 2238 -n Alicja13")

	def test_315(self):
		self.do_test("127.0.0.1 -p 32770 -i 127.0.0.1 -r 32769 -n abcdefghijklmnopqrstu")

	def test_316(self):
		self.do_test("127.0.0.1 -n abcdefghijklmnopqrstu")

	def test_317(self):
		self.do_test("-1 -n abcdefghijklmnopqrstu")

	def test_318(self):
		self.do_test("google.com -n abcdefghijklmnopqrstu")

	def test_319(self):
		self.do_test("2607:f8b0:4004:080a:0000:0000:0000:200e -n abcdefghijklmnopqrstu")

	def test_320(self):
		self.do_test("127.0.0.1 -p 65536 -n A licja")

	def test_321(self):
		self.do_test("127.0.0.1 -r 20 -p 30 -n abcdefghijklmnopqrstu")


if __name__ == '__main__':
	config.read("test_config.ini")
	unittest.main()
