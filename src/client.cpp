#include <tchar.h>
#include <iostream>
#include <cstdlib>
#include "CommAudio.h"
#include "client-file.h"
#include "libzplay.h"

using namespace std;
using namespace libZPlay;

HINSTANCE hInst;

int client_main (int argc, char const *argv[]);

const string SERVICE_REQUEST_STRING = "list-services\n";

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

void request_services(SOCKET sock) {   
   send(sock, SERVICE_REQUEST_STRING.c_str(), SERVICE_REQUEST_STRING.length(), 0);
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

void stream_song (string song) {
	int sock = comm_connect("localhost");
	int song_sock = create_udp_socket(1338);
	
	// Build request line
	string request = "S " + song + "\n";
	
	send(sock, request.data(), request.size(), 0);

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
		cerr << "Error: " <<  netplay->GetError() << endl;
		netplay->Release();
		return;
	}

	netplay->Play();

	// Mainloop: just recv() remote microphone data and push it to dynamic stream
	while ( true ) {
		// Create a buffer with maximum udp packet size, and recvfrom into it.
		char * buf = new char[65507];
		
		sockaddr_in server;
		int size = sizeof(server);
		int r = recvfrom (song_sock, buf, 65507, 0, (sockaddr*)&server, &size);
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
}

DWORD WINAPI stream_song_proc(LPVOID lpParamter) {
	stream_song("tol.flac");
	return 0;
}

int APIENTRY _tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	HWND hwnd;
	HACCEL hAccelTable;

	// Open up a Winsock v2.2 session
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2,2);
	WSAStartup(wVersionRequested, &wsaData);

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow, hwnd)) {
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DCWIN1));


	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0) )
	{
		//
		//if(!IsDialogMessage(hwnd, &msg)) // Slow!
		if (!TranslateAccelerator(hwnd, hAccelTable, &msg) && !IsDialogMessage(hwnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WSACleanup();
	return (int) msg.wParam;
}

int client_main (int argc, char const *argv[])
{	
	Services s;
	// Connect
	int sock = comm_connect("localhost");
	
	// Request services
	request_services(sock);

	// Recv
	string services = recv_services(sock);
	
	// Parse
	ParseServicesList(services, s);
	
	// Display
	printStruct(s);
	
	if (s.songs.size() >= 1) {
		cout << "Downloading first song: " <<  s.songs[0] << endl;
		downloadFile(1337, s.songs[0]); // TODO: change port to new channel
		//uploadFile(1337);
	}
	
	return 0;
}
