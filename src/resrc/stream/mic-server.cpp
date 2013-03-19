/*
*
* Two-way Microphone over UDP socket: server.
*
*/
#include <windows.h>
#include <cstdio>
#include "libzplay.h"

using namespace std;
using namespace libZPlay;

int sock;
sockaddr_in server, client;
int __stdcall cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2);

// Function: mic_serve
// interface: void mic_serve (void)
// returns nothing
// Notes:
// 	Setup the socket, and bind server address.
void mic_serve () {
	memset ((char *)&server, 0, sizeof(server));
	memset ((char *)&client, 0, sizeof(client));
	server.sin_family = AF_INET;
	server.sin_port = htons(22);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	// UDP socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) { printf ("Bad socket\n"); exit(1); }
	
	if (bind (sock, (struct sockaddr *)&server, sizeof(server)) == -1) {
		perror ("Can't bind name to socket");
		exit(1);
	}
}

int main(int argc, const char **argv) {
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2,2);
	WSAStartup(wVersionRequested, &wsaData);

	mic_serve();

	//
	// Create zplay instance for mic input.
	ZPlay * out = CreateZPlay();
	
	const char * filename = (argc > 1) ? argv[1] : "file";
	
	// open file
	int result = out->OpenFile(filename, out->GetFileFormat(filename));
	if(result == 0) {
		printf("Error: %s\n", out->GetError());
		out->Release();
		return 0;
	}
	/**/TStreamTime st = {0};
	out->Seek(tfSecond, &st, smFromEnd );
	out->ReverseMode(1);
	out->SetCallbackFunc(cb, (TCallbackMessage)(MsgWaveBuffer|MsgStop), NULL);
	
	// display position and wait for song end
	while(1) {
		char * buf = new char[65507];
		int client_s = sizeof(client);
		printf("recv\n");
		int r = recvfrom (sock, buf, 65507, 0, (sockaddr*)&client, &client_s);
		printf("recv %d\n", r);
		if (r == -1) {
			printf("erro %d\n", WSAGetLastError());
			memset ((char *)&client, 0, sizeof(client)); // clear client addr to accept from other clients.
			out->Stop();
		} else {
			// If reverse
			TStreamTime et = {0};
			out->Seek(tfSecond, &et, smFromEnd );
			/*out->Seek(tfSecond, &et, smFromBeginning  );*/
			out->Play();
		}
		delete buf;
	}

	out->Release();
	return 0;
}
int __stdcall cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2){
		//if (message == MsgStop )
		//	return closesocket(sock);
		
		//printf("client port %s %d\n", inet_ntoa(client.sin_addr) ,client.sin_port);
		printf("Sending %u\n", param2);
		// We don't want to send all the data as soon as it is decoded: it will make a mess on the network.
		// Instead, we slowly send it out, but how slowly should we send it out?
		// we can send it as it is needed, but since we don't have any ACKs in UDP, 
		// we can only calculate approximately when the client should want the next amount
		
		// 8820 /2 channels /2 bytes per sample / 44100 samples per second * 1000 milliseconds per second = 50
		// 50 - 10 fudge value (sleep can sleep more than what you tell it to).
		Sleep(40);

		if (sendto(sock, (const char *)param1, param2, 0, (const struct sockaddr*)&client, sizeof(client)) < 0)
			return 2;
		
		return 1;
}
