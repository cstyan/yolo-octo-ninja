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

int main(int argc, char **argv) {
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2,2);
	WSAStartup(wVersionRequested, &wsaData);

	mic_serve();

	//
	// Create zplay instance for playback.
	ZPlay *player = CreateZPlay();
	player->SetSettings(sidSamplerate, 44100);// 44100 samples
	player->SetSettings(sidChannelNumber, 2);// 2 channel
	player->SetSettings(sidBitPerSample, 16);// 16 bit
	player->SetSettings(sidBigEndian, 0); // little endian
	int i;
	int result = player->OpenStream(1, 1, &i, 2, sfPCM);
	printf("openstream: %d\n", result);

	
	//
	// Create zplay instance for mic input.
	ZPlay * out = CreateZPlay();
	
	// open file
	result = out->OpenFile("wavein://", sfAutodetect);
	if(result == 0) {
		printf("Error: %s\n", out->GetError());
		out->Release();
		return 0;
	}
	out->SetCallbackFunc(cb, (TCallbackMessage)(MsgWaveBuffer|MsgStop), NULL);
	out->Play();

	// display position and wait for song end
	while(1) {
		char * buf = new char[65507];
		int client_s = sizeof(client);
		printf("recv\n");
		int r = recvfrom (sock, buf, 65507, 0, (sockaddr*)&client, &client_s);
		if (r == -1)
			memset ((char *)&client, 0, sizeof(client)); // clear client addr to accept from other clients.
		printf("%lu got %d \n", GetTickCount(), r);
		player->PushDataToStream(buf, r);
		delete buf;
		player->Play();
		/*getchar();*/
		// get stream status to check if song is still playing
		TStreamStatus status;
		player->GetStatus(&status);	
		if(status.fPlay == 0)
			break; // exit checking loop

		// get current position
		TStreamTime pos;
		player->GetPosition(&pos);
		// display position
		printf("Pos: %02u:%02u:%02u:%03u\r", pos.hms.hour, pos.hms.minute, pos.hms.second, pos.hms.millisecond);
	}

	player->Release();
	return 0;
}
int __stdcall cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2){
		if (message == MsgStop )
			return closesocket(sock);
		
		printf("client port %s %d\n", inet_ntoa(client.sin_addr) ,client.sin_port);
		
		// Send if we have a client to sendto
		if (client.sin_port != 0)
			if (sendto(sock, (const char *)param1, param2, 0, (const struct sockaddr*)&client, sizeof(client)) < 0)
				return 2;
		
		return 1;
}
