.PHONY: all clean

srcdir_server = src/server
srcdir_client = src/client
srcdir_shared = src/shared

all: ${srcdir_server}/screen-worms-server ${srcdir_client}/screen-worms-client

${srcdir_server}/screen-worms-server: ${srcdir_server}/server.o ${srcdir_server}/main.o ${srcdir_server}/player.o ${srcdir_server}/user.o ${srcdir_server}/worm.o ${srcdir_server}/viewer.o ${srcdir_server}/board.o ${srcdir_server}/event.o
	g++ -o ${srcdir_server}/screen-worms-server $^
	mv ${srcdir_server}/screen-worms-server .

${srcdir_client}/screen-worms-client: ${srcdir_client}/main.o ${srcdir_client}/client.o
	g++ -o ${srcdir_client}/screen-worms-client $^
	mv ${srcdir_client}/screen-worms-client .

${srcdir_server}/server.o: ${srcdir_server}/server.cpp ${srcdir_server}/server.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_server}/server.o $<

${srcdir_server}/main.o: ${srcdir_server}/main.cpp ${srcdir_server}/server.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_server}/main.o $<

${srcdir_server}/player.o: ${srcdir_server}/player.cpp ${srcdir_server}/player.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_server}/player.o $<

${srcdir_server}/user.o: ${srcdir_server}/user.cpp ${srcdir_server}/user.h ${srcdir_shared}/client_server_msg.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_server}/user.o $<

${srcdir_server}/worm.o: ${srcdir_server}/worm.cpp ${srcdir_server}/worm.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_server}/worm.o $<

${srcdir_server}/viewer.o: ${srcdir_server}/viewer.cpp ${srcdir_server}/viewer.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_server}/viewer.o $<

${srcdir_server}/board.o: ${srcdir_server}/board.cpp ${srcdir_server}/board.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_server}/board.o $<

${srcdir_server}/event.o: ${srcdir_server}/event.cpp ${srcdir_server}/event.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_server}/event.o $<

${srcdir_client}/client.o: ${srcdir_client}/client.cpp ${srcdir_client}/client.h ${srcdir_shared}/client_server_msg.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_client}/client.o $<

${srcdir_client}/main.o: ${srcdir_client}/main.cpp ${srcdir_client}/client.h
	g++ -c -Wall -Wextra -std=c++17 -O2 -o ${srcdir_client}/main.o $<

clean:
	rm -f screen-worms-server ${srcdir_server}/*.o screen-worms-client ${srcdir_client}/*.o
