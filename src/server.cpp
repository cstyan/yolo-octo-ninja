#include <iostream>
#include "CommAudio.h"
#include "server-file.h"

using namespace std;

Services s;
DWORD WINAPI handle_client(LPVOID lpParameter);
string recv_request(SOCKET s);

void usage (char const *argv[]) {
	cout << argv[0] << " [server] [port]" << endl;
}

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
		
		if (CreateThread(NULL, 0, handle_client, (LPVOID)new_sd, 0, NULL) == NULL)
			cerr << "Couldn't create client handler thread!" << endl;
	}
}

DWORD WINAPI handle_client(LPVOID lpParameter) {
	SOCKET client = (SOCKET) lpParameter;

	while (true) {
		string request = recv_request(client);

		cout << "Client request: " << request << endl;

		if (request == "list-services") {
			// Generate services list.
			string services = ListServices(s); 
			
			send(client, services.data(), services.size(), 0);
		} else if (request.size() == 0) {
			closesocket(client);
			break;
		}
	}

	return 0;
}

string recv_request (SOCKET client) {
	string out;
	char data[BUFSIZE];
	while (true) {
		int bytes_recv = recv(client, data, BUFSIZE, 0);
		if (bytes_recv > 0) {
			out.append(data, bytes_recv);
			// Look for the terminating line
			size_t n = out.find("\n");
			out = out.substr(0, n);
			// Found last line, break out.
			if (n != string::npos) break;
		} else break; // Connection closed, or error.
	}
	return out;
}

int main(int argc, char const *argv[])
{
	// Open up a Winsock v2.2 session
	WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2,2);
	WSAStartup(wVersionRequested, &wsaData);
	
	// Initialize some services.
	s.microphone = true;
	s.songs.push_back("song1.mp3");
	s.songs.push_back("song2.mp3");
	s.songs.push_back("song3.mp3");

	int sock = setup_listening();
	wait_for_connections (sock);
	
	return 0;
}