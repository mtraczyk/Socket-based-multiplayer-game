.PHONY: all clean

srcdir_server = src/server
srcdir_client = src/client
srcdir_shared = src/shared

all: ${srcdir_client}/screen-worms-client

${srcdir_client}/screen-worms-client: ${srcdir_client}/main.o ${srcdir_client}/client.o
	g++ -o ${srcdir_client}/screen-worms-client $^
	mv ${srcdir_client}/screen-worms-client .

${srcdir_client}/client.o: ${srcdir_client}/client.cpp ${srcdir_client}/client.h ${srcdir_shared}/client_server_msg.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_client}/client.o $<

${srcdir_client}/main.o: ${srcdir_client}/main.cpp ${srcdir_client}/client.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_client}/main.o $<

clean:
	rm -f screen-worms-server ${srcdir_server}/.o screen-worms-client ${srcdir_client}/.o