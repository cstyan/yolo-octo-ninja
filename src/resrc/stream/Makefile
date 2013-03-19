all : mic-client mic-server

mic-client: mic-client.cpp
	g++ -Wall -g  mic-client.cpp -lzplay -lWS2_32 -o mic-client

mic-server: mic-server.cpp
	g++ -Wall -g  mic-server.cpp -lzplay -lWS2_32 -o mic-server

clean:
	rm -rf *.exe