#include <iostream>
#include "windows.h"
#include "CommAudio.h"

using namespace std;

void usage (char const *argv[]) {
	cout << argv[0] << " [server] [port]" << endl;
}

Services s;

int setup_listening (int port = 1337) {
	SOCKET lsock;
	struct	sockaddr_in server;
	
	// Create OVERLAPPED TCP socket
	if ((lsock = WSASocket(AF_INET, SOCK_STREAM , 0, NULL, 0,  WSA_FLAG_OVERLAPPED))
					== INVALID_SOCKET) {
		cerr << "Failed to get a socket: " << WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}

	// Initialize and set up the address structure
	memset((char *)&server, 0, sizeof(struct sockaddr_in)); 
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client
	
	// Bind an address to the socket
	if (bind(lsock, (struct sockaddr *)&server, sizeof(server)) == -1) {
		cerr << "Can't bind name to socket: " << WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}
	
	listen(lsock, 5);
	return lsock;
}

void wait_for_connections (int lsock) {
	SOCKET new_sd;
	struct	sockaddr_in client;
	int	client_len = sizeof(client);
	
	while (true) {
		client_len= sizeof(client); 
		
		if ((new_sd = accept (lsock, (struct sockaddr *)&client, &client_len)) == -1) {
			cerr << "accept failed, server closed?\n" << endl; 
			break;
		}
		
		// Start client handler
		cout << "New client: " << inet_ntoa(client.sin_addr) << endl;
		
		// Generate services list.
		string services = ListServices(s); 
		
		send(new_sd, services.data(), services.size(), 0);
	}
}

int main(int argc, char const *argv[])
{
	// Open up a Winsock v2.2 session
	WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2,2);
	WSAStartup(wVersionRequested, &wsaData);
	
	// Initialize some services.
	s.microphone = true;
	s.songs.push_back("song2.mp3");

	int sock = setup_listening();
	wait_for_connections (sock);
	
	return 0;
}