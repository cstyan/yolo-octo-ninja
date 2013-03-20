#include <iostream>
#include <cstdlib>
#include <windows.h>
#include "CommAudio.h"

using namespace std;

void usage (char const *argv[]) {
	cout << argv[0] << " [server] [port]" << endl;
}

int comm_connect (const char * host, int port = 1337) {
	hostent	*hp;
	sockaddr_in server;
	SOCKET sd = socket(AF_INET, SOCK_STREAM, 0);

	// Initialize and set up the address structure
	memset((char *)&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	
	if ((hp = gethostbyname(host)) == NULL)
	{
		cerr << "Unknown server address" << endl;
		exit(1);
	}

	// Copy the server address
	memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);

	// Connecting to the server
	if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		cerr << "Can't connect to server\n" << endl;
		exit(1);
	}

	return sd;
}

string recv_services (int sd) {
	string out;
	char data[BUFSIZE];
	while (true) {
		int bytes_recv = recv(sd, data, BUFSIZE, 0);
		if (bytes_recv > 0) {
			out.append(data, bytes_recv);
			// Look for the terminating line: its either M true\n or M false\n.
			size_t n = out.find("M true\n");
			if (n == string::npos)
				n = out.find("M false\n");
			// Found last line.
			if (n != string::npos)
				break;
		} else break;
	}
	return out;
}

int main(int argc, char const *argv[])
{
	// Open up a Winsock v2.2 session
	WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2,2);
	WSAStartup(wVersionRequested, &wsaData);
	
	Services s;
	// Connect
	int sock = comm_connect("localhost");
	
	// Recv
	string services = recv_services(sock);
	
	// Parse
	ParseServicesList(services, s);
	
	// Display
	printStruct(s);
	
	if (s.songs.size() >= 1) {
		cout << "Downloading first song: " <<  s.songs[0] << endl;
		downloadFile(sock, s.songs[0]);
	}
	
	return 0;
}