/*
*
* Two-way Microphone over UDP socket: client.
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

// Function: Mic_socket
// interface: void mic_socket (void)
// returns nothing
// Notes:
// 	Setup the socket, server address and bind local client address.
void mic_socket () {
	// open mic socket
	struct hostent	*hp;
	memset((char *)&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(1338);
	if ((hp = gethostbyname("localhost")) == NULL)
		throw ( "Unknown server address\n");

	// Copy the server address
	memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);

	// UDP socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	
	// Bind local address to the socket
	/**/memset((char *)&client, 0, sizeof(client));
	client.sin_family = AF_INET;
	client.sin_port = htons(1338);  // listen back for the streaming data on 1338.
	client.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *)&client, sizeof(client)) == -1) {
		perror ("Can't bind name to socket");
		exit(1);
	}
	sockaddr_in res = {0};
	int thing = sizeof(sockaddr_in);
	if (getsockname(sock, (sockaddr*)&res, &thing) == SOCKET_ERROR )
		printf("error! %d\n", WSAGetLastError());
	printf("client.sin_port: %d\n", ntohs(res.sin_port));

}

int main (int argc, char **argv) {
	// Init winsock.
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2,2);
	WSAStartup(wVersionRequested, &wsaData);
	
	// Create UDP socket for mic
	mic_socket();

	
	//
	// Create zplay instance for playing remote mic
	ZPlay * netplay = CreateZPlay();
	netplay->SetSettings(sidSamplerate, 44100);// 44100 samples
	netplay->SetSettings(sidChannelNumber, 2);// 2 channel
	netplay->SetSettings(sidBitPerSample, 16);// 16 bit
	netplay->SetSettings(sidBigEndian, 0); // little endian
	int i;
	// Remote microphone is a UDP "stream" of PCM data,
	int result = netplay->OpenStream(1, 1, &i, 2, sfPCM); // we open the zplay stream without any real data, and start playback when we actually get input.
	if(result == 0) {
		printf("Error: %s\n", netplay->GetError());
		netplay->Release();
		return 0;
	}
	

	
	//sendto(sock, (const char *)"hi", 2, 0, (const struct sockaddr*)&server, sizeof(server));
	
	netplay->Play();
	// Mainloop: just recv() remote microphone data and push it to dynamic stream
	while(1) {
		char * buf = new char[65507];
		
		int size = sizeof(server);
		printf("recv\n");
		int r = recvfrom (sock, buf, 65507, 0, (sockaddr*)&server, &size);
		printf("donerecv\n");
		if ( r == -1 ) {	
			int err = WSAGetLastError();
			if (err == 10054)
				printf("Concoction recent by Pier.\n"); // Connection reset by peer.
			else printf("get last error %d\n", err);
			break;
		}
		
		printf("%lu got %d \n", GetTickCount(), r);
		netplay->PushDataToStream(buf, r);
		delete buf;
		if (r == 0)
			break;

	}

	while (1) {
		// get stream status to check if song is still playing
		TStreamStatus status;
		netplay->GetStatus(&status);
		if(status.fPlay == 0)
			break; // exit checking loop
			
		TStreamTime pos;
		netplay->GetPosition(&pos);
		printf("Pos: %02u:%02u:%02u:%03u\r", pos.hms.hour, pos.hms.minute, pos.hms.second, pos.hms.millisecond);
		Sleep(10);
	}

	netplay->Release();
	return 0;
}