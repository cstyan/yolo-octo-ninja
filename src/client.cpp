/*---------------------------------------------------------------------------------------------
--	SOURCE FILE: client.cpp
--
--	PROGRAM:	client.exe
--
--	FUNCTIONS:	int comm_connect (const char * host, int port)
--				void request_services(SOCKET sock)
--				string recv_services (int sd)
--				void stream_song ()
--				DWORD WINAPI stream_song_proc(LPVOID lpParamter)
--				ChannelInfo extractChannelInfo(const string& channel)
--				DWORD WINAPI join_channel(LPVOID lpParamter)
--				int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, 
--						unsigned int param1, unsigned int param2)
--				ClientContext * start_microphone_stream()
--				int APIENTRY _tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, 
--						LPTSTR lpCmdLine, int nCmdShow)
--					
--	DATE:		11/Mar/2013
--
--	DESIGNERS:	 David Czech, Dennis Ho
--	PROGRAMMERS: David Czech, Dennis Ho
--
--	NOTES:		Provides main comm functions for client.exe
------------------------------------------------------------------------------------------------*/
#include <tchar.h>
#include <iostream>
#include <cstdlib>
#include "CommAudio.h"
#include "client-file.h"
#include "libzplay.h"

using namespace std;
using namespace libZPlay;

// The Server to connect to
char server[256] = "localhost";

HINSTANCE hInst;
bool keep_streaming_channel = false;
int song_sock   = 0;
ZPlay * netplay = NULL;
const string SERVICE_REQUEST_STRING = "list-services\n";

struct ChannelInfo {
	string name;
	SOCKET sock;
	sockaddr_in addr;
};


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	comm_connect
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		David Czech
-- PROGRAMMERS: 	David Czech
--
-- INTERFACE:	int comm_connect (const char * host, int port)
-- RETURNS:		int - socket descriptor
--
-- NOTES:		Connects to the specified host on the specified port, or displays an error
					message box on failure.
----------------------------------------------------------------------------------------------*/
int comm_connect (const char * host, int port) {
	hostent	*hp;
	sockaddr_in server;
	SOCKET sd = socket(AF_INET, SOCK_STREAM, 0);

	// Initialize and set up the address structure
	memset((char *)&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	
	if ((hp = gethostbyname(host)) == NULL)
	{
		MessageBox(0, "Unknown server address", "Error Connecting.", 0);
		return 0;
	}

	// Copy the server address
	memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);

	// Connecting to the server
	if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		MessageBox(0, "Can't connect to server", "Error Connecting.", 0);
		return 0;
	}

	return sd;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	request_services
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		David Czech
-- PROGRAMMERS: 	David Czech
--
-- INTERFACE:	void request_services(SOCKET sock)
-- RETURNS:		void
--
-- NOTES:		Send a request for a list of available services on the server.
----------------------------------------------------------------------------------------------*/
void request_services(SOCKET sock) {   
   send_ec(sock, SERVICE_REQUEST_STRING.c_str(), SERVICE_REQUEST_STRING.length(), 0);
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	recv_services
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		David Czech
-- PROGRAMMERS: 	David Czech
--
-- INTERFACE:	string recv_services (int sd)
--  int sd - socket descriptor to get services reply from.
--
-- RETURNS:		a string with all the services data.
--
-- NOTES:		After the request to list servicies is sent, this function will get the results.
----------------------------------------------------------------------------------------------*/
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


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	stream_song
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		David Czech
-- PROGRAMMERS: 	David Czech
--
-- INTERFACE:	void stream_song ()
-- RETURNS:		nothing
--
-- NOTES:		Stream a song: Continuously recv()s and pushes sound data to playback.
----------------------------------------------------------------------------------------------*/
void stream_song () {
	// Close stream
	netplay->Close();

	// Open new stream
	int i;

	// we open the zplay stream without any real data, and start playback when we actually get input.
	int result = netplay->OpenStream(1, 1, &i, 2, sfPCM);
	if(result == 0) {
		cerr << "Error: " <<  netplay->GetError() << endl;
		netplay->Release();
		return;
	}

	netplay->Play();

	// Mainloop: just recv() remote microphone data and push it to dynamic stream
	while ( true ) {
		// Create a buffer with maximum udp packet size, and recvfrom into it.
		char buf[65507];
		
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
		
		//printf("%lu got %d \n", GetTickCount(), r);
		if (r == 0)
			continue;

		// Might need to protect this with mutex if client GUI modifies netplay as well.
		netplay->PushDataToStream(buf, r);
	}
	closesocket(song_sock);
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	stream_song_proc
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		David Czech
-- PROGRAMMERS: 	David Czech
--
-- INTERFACE:	DWORD WINAPI stream_song_proc(LPVOID lpParamter)
-- RETURNS:		DWORD
--
-- NOTES:		Trampoline to stream_song(), simplifies calling technicalities, etc
----------------------------------------------------------------------------------------------*/
DWORD WINAPI stream_song_proc(LPVOID lpParamter) {
	stream_song();
	return 0;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	extractChannelInfo
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:	Dennis Ho	
-- PROGRAMMERS: Dennis Ho
--
-- INTERFACE:	ChannelInfo extractChannelInfo(const char *channel)
-- RETURNS:		ChannelInfo
--
-- NOTES:		Parses the string to extract the channel name/ip/port
----------------------------------------------------------------------------------------------*/
ChannelInfo extractChannelInfo(const char *channel) {
	ChannelInfo ci;
	
	string channelString(channel);

	size_t lastSpace = channelString.find_last_of(" ");
	size_t portSeperator = channelString.find_last_of(":");
		
	ci.name = channelString.substr(0, lastSpace);
	ci.addr.sin_family = AF_INET;
	ci.addr.sin_addr.s_addr = inet_addr(channelString.substr(lastSpace + 1, portSeperator - (lastSpace + 1)).c_str());
	ci.addr.sin_port = htons(atoi(channelString.substr(portSeperator + 1, channelString.length() - (portSeperator + 1)).c_str()));

	return ci;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	join_channel
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:	Dennis Ho, David Czech
-- PROGRAMMERS: Dennis Ho, David Czech
--
-- INTERFACE:	DWORD WINAPI join_channel(LPVOID lpParameter)
-- RETURNS:		DWORD
--
-- NOTES:		Joins the specified multicast channel. Channel parameter should be in the format:
--				<channel name> <ip/hostname>:<port>
----------------------------------------------------------------------------------------------*/
DWORD WINAPI join_channel(LPVOID lpParameter) {
	int error;
	bool reuseFlag = false;
	SOCKADDR_IN localAddr, sourceAddr;
	struct ip_mreq stMreq;

	// Parse channel info
	ChannelInfo ci = extractChannelInfo((char*)lpParameter);         

	if ((ci.sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {	  
		cout << "error socket" << endl;
		return false;
	}

	if ((error = setsockopt(ci.sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseFlag, sizeof(reuseFlag))) == SOCKET_ERROR) {		
		cout << "error setsockopt" << endl;
		return false;
	}

	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = ci.addr.sin_port;

	if ((error = bind(ci.sock, (struct sockaddr*)&localAddr, sizeof(localAddr))) == SOCKET_ERROR) {
	  cout << "error bind" << endl;
		return false;
	}

	// Join multicast group
	memset((char *)&stMreq, 0, sizeof(ip_mreq));
	stMreq.imr_multiaddr.s_addr = ci.addr.sin_addr.s_addr;
	stMreq.imr_interface.s_addr = INADDR_ANY;   

	if ((error = setsockopt(ci.sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&stMreq, sizeof(stMreq))) == SOCKET_ERROR) {
		cout << "error multicast" << endl;
		return false;
	}

	netplay->Play();

	size_t buffed = 0;

	// While play isn't stopped 
	while (keep_streaming_channel) {
		// Create a buffer with maximum udp packet size, and recvfrom into it.
		char * buf = new char[65507];

		int addr_size = sizeof(struct sockaddr_in);
		int r = recvfrom(ci.sock, buf, 65507, 0, (struct sockaddr*)&sourceAddr, &addr_size);

		if ( r == -1 ) {
			int err = WSAGetLastError();
			if (err == 10054)
				printf("Concoction recent by Pier.\n"); // Connection reset by peer.
			else printf("get last error %d\n", err);
			break;
		}

		netplay->PushDataToStream(buf, r);
		buffed += r;
		
		//if (buffed > 800000)
		//	netplay->Play();

		delete buf;
		if (r == 0)
			break;
	}
	cout << "Closing socket" << endl;
	closesocket(ci.sock);
	return 0;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	stream_cb
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		David Czech
-- PROGRAMMERS: 	David Czech
--
-- INTERFACE:	int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, 
--					unsigned int param2)
-- RETURNS:		int - action for libzplay to take: 
					1: Skip sending decoded data to sound card.
					2: Stop playback
--
-- NOTES:		sends stream to client udp socket, or stops libzplay instance if there is an error.
----------------------------------------------------------------------------------------------*/
int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2) {
	ClientContext * ctx = (ClientContext *) user_data;
	sockaddr_in * client_addr = &ctx->addr;

	if (sendto(ctx->udp, (const char *)param1, param2, 0, (const sockaddr*)client_addr, sizeof(sockaddr_in)) < 0)
		return 2;

	return 1;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	start_microphone_stream
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		David Czech
-- PROGRAMMERS: 	David Czech
--
-- INTERFACE:	ClientContext * start_microphone_stream()
-- RETURNS:		ClientContext * - pointer to an allocated ClientContext, 
--					containing information about the server microphone stream (UDP socket, etc)
--
-- NOTES:		Open microphone stream and setup callbacks to send to server, (re-)uses stream_cb.
----------------------------------------------------------------------------------------------*/
ClientContext * start_microphone_stream() {
	hostent	*hp;
	ClientContext * ctx = new ClientContext;
	memset(ctx, 0, sizeof(ClientContext));
	ctx->decoder = CreateZPlay();

	// Open microphone input stream
	int result = ctx->decoder->OpenFile("wavein://", sfAutodetect);
	if(result == 0) {
		printf("Error: %s\n", ctx->decoder->GetError());
	}
	ctx->udp = create_udp_socket(0);
	ctx->addr.sin_family = AF_INET;
	ctx->addr.sin_port = htons(1338);

	if ((hp = gethostbyname(server)) == NULL)
	{
		MessageBox(0, "Unknown server address", "Error Connecting.", 0);
		return 0;
	}

	// Copy the server address
	memcpy((char *)&ctx->addr.sin_addr, hp->h_addr, hp->h_length);
	// set callback to stream to client (reuse stream_cb?)
	ctx->decoder->SetCallbackFunc(stream_cb, (TCallbackMessage)(MsgWaveBuffer|MsgStop), ctx);
	ctx->decoder->Play();
	return ctx;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	_tWinMain
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		David Czech
-- PROGRAMMERS: 	David Czech
--
-- INTERFACE:	int APIENTRY _tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, 
--					LPTSTR lpCmdLine, int nCmdShow)
-- RETURNS:		int - error level of application.
--
-- NOTES:		Initilizes Winsock, Creates ZPlay instance, initializes GUI, and runs message loop.
----------------------------------------------------------------------------------------------*/
int APIENTRY _tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	HWND hwnd;
	HACCEL hAccelTable;

	// Open up a Winsock v2.2 session
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2,2);
	WSAStartup(wVersionRequested, &wsaData);

	// Create song stream listening UDP socket.
	song_sock = create_udp_socket(1338);

	// Create zplay instance for playing remote mic
	netplay = CreateZPlay();
	netplay->SetSettings(sidSamplerate, 44100);// 44100 samples
	netplay->SetSettings(sidChannelNumber, 2);// 2 channel
	netplay->SetSettings(sidBitPerSample, 16);// 16 bit
	netplay->SetSettings(sidBigEndian, 0); // little endian

	netplay->Play();

	CreateThread(NULL, 0, stream_song_proc, (LPVOID)NULL, 0, NULL);

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