all: client server

client: src/helpers.h src/helpers.cpp src/client.cpp
	g++ -std=c++11 -pthread src/helpers.cpp src/client.cpp -o client

server: src/helpers.h src/helpers.cpp src/server.cpp
	g++ -std=c++11 -pthread src/helpers.cpp src/server.cpp -o server

clean: client server
	rm -f client server