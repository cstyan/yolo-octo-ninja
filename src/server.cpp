/*---------------------------------------------------------------------------------------------
--	SOURCE FILE: server.cpp
--
--	PROGRAM:	server.exe
--
--	FUNCTIONS:	
--
	Common Server Functions
		int setup_listening (int port = 1337);
		void wait_for_connections (int lsock);
		string recv_request(SOCKET);
		
	Client Handlers
		DWORD WINAPI handle_client(LPVOID);
		void process_stream_song(ClientContext*, string);
		void process_download_file(ClientContext * ctx, string song);
		void process_upload_song(ClientContext * ctx, string song);
		void process_join_voice(ClientContext * ctx);
		void transmit_from_stream(SOCKET sock, istringstream& stream, streamsize packetSize);
		int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2);

	Song, Channel and Parameter enumeration, validation and processing
		ChannelInfo extract_channel_info(const string& channelString);
		bool validate_param(string param, SOCKET error_sock, string error_msg);
		void add_files_to_songs (std::vector<string>& songs, const char * file);
		void find_songs (std::vector<string>& songs);
		vector<string> retrieve_song_list(const char *playlistName);

--	DATE:		Mar 23, 2013
--
--	DESIGNERS:	David Czech, Dennis Ho, Kevin Tangeman, Jake Miner
--	PROGRAMMERS: David Czech, Dennis Ho
--
--	NOTES:		Contains Main and the main functions for server program.
------------------------------------------------------------------------------------------------*/

#include <direct.h>
#include <iostream>
#include "CommAudio.h"
#include "server-file.h"
#include "libzplay.h"
#include <fstream>
#include <sstream>
#include <string>

using namespace std;
using namespace libZPlay;

Services s;

struct ChannelInfo {
   string name;
   SOCKET sock;   
   sockaddr_in addr;
};

// Forward declarations. Only a couple of these are really needed, but all are listed for completeness
int setup_listening (int port = 1337);
void wait_for_connections (int lsock);
string recv_request(SOCKET);
DWORD WINAPI handle_client(LPVOID);
void process_stream_song(ClientContext*, string);
void process_download_file(ClientContext * ctx, string song);
void process_upload_song(ClientContext * ctx, string song);
void process_join_voice(ClientContext * ctx);
int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2);
void transmit_from_stream(SOCKET sock, istringstream& stream, streamsize packetSize);
bool validate_param(string param, SOCKET error_sock, string error_msg);
void add_files_to_songs (std::vector<string>& songs, const char * file);
void find_songs (std::vector<string>& songs);
vector<string> retrieve_song_list(const char *playlistName);
ChannelInfo extract_channel_info(const string& channelString);


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	setup_listening
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	int setup_listening (int port)
-- RETURNS:		int - the newly created listening socket
--
-- NOTES:		Creates a TCP socket and listens on a port (1337 by default).
--				port - the port number to listen on
----------------------------------------------------------------------------------------------*/
int setup_listening (int port) {
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

/*----------------------------------------------------------------------------------------------
-- FUNCTION:	wait_for_connections
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	void wait_for_connections (int lsock)
-- RETURNS:		void
--
-- NOTES:		Wait for new connections and start a thread to handle accepted clients.
--				lsock - the listening socket to wait on
----------------------------------------------------------------------------------------------*/
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
		ClientContext * ctx = new ClientContext;
		memset(ctx, 0, sizeof(ClientContext));
		ctx->control = new_sd;
		ctx->addr    = client;
		
		if (CreateThread(NULL, 0, handle_client, (LPVOID)ctx, 0, NULL) == NULL)
			cerr << "Couldn't create client handler thread!" << endl;
	}
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	handle_client
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	DWORD WINAPI handle_client(LPVOID lpParameter)
-- RETURNS:		DWORD
--
-- NOTES:		Handle a client's requests.
--				lpParamater - should be a pointer to the ClientContext of the client to handle.
----------------------------------------------------------------------------------------------*/
DWORD WINAPI handle_client(LPVOID lpParameter) {
	ClientContext * ctx = (ClientContext *) lpParameter;
	SOCKET client = ctx->control;

	// Client Loop active while client is connected.
	// Loop exit condition handled in request parsing below.
	while (true) {
		string request = recv_request(client);

		cout << "Client request: " << request << endl;

		// Empty request or disconnected: client quit.
		if (request.size() == 0) {
			closesocket(client);
			break;
		} else if (request == "list-services") {
			// Generate services list.
			string services = ListServices(s); 
			// Send the list of services.
			send(client, services.data(), services.size(), 0);
			continue;
		} else if (request == "stop-stream") {
			if (ctx->decoder) {
				cout << "Stopping stream" << endl;
				ctx->decoder->Stop();
			}
		}

		// Parse requests with commands (stream song request, download song request, etc)
		string req_command;
		stringstream req_stream(request);
		if (req_stream >> req_command >> ws) {
			if (req_command == "V") { // Parameterless
				process_join_voice(ctx);
			}
			else { // Parameterized
				string param;
				// Get the rest of the line into param.
				getline(req_stream, param); 

				if (req_command == "D")
					process_download_file(ctx, param);
				else if (req_command == "S")
					process_stream_song(ctx, param);
				else if (req_command == "U")
					process_upload_song(ctx, param);				
			}
		}
	}

	return 0;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	recv_request
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	string recv_request (SOCKET client)
-- RETURNS:		string - the data received without the terminating newline
--
-- NOTES:		Recv a request from a client until a newline is encountered.
--				client - the client SOCKET to recv the request from
----------------------------------------------------------------------------------------------*/
string recv_request (SOCKET client) {
	string out;
	char data[BUFSIZE];
	while (true) {
		int bytes_recv = recv(client, data, BUFSIZE, 0);
		if (bytes_recv > 0) {
			out.append(data, bytes_recv);
			// Look for the terminating line
			size_t n = out.find("\n");
			
			// Found last line, break out.
			if (n != string::npos){
				out = out.substr(0, n);
				break;
			} 
		} else break; // Connection closed, or error.
	}
	return out;
}

/*----------------------------------------------------------------------------------------------
-- FUNCTION:	process_stream_song
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	void process_stream_song (ClientContext * ctx, string song)
-- RETURNS:		void
--
-- NOTES:		Recv until a newline is encountered.
--				ctx - should be a pointer to the ClientContext of the client to handle.
--				song - a C++ string containing the song that will be streamed to the client.
----------------------------------------------------------------------------------------------*/
void process_stream_song (ClientContext * ctx, string song) {
	cout << "Streaming song: " << song << endl;
	// Validate
	if (!validate_param(song, ctx->control, "Invalid stream song request: no song specified!"))
		return;

	// Create zplay decoder instance, if it didn't already exist.
	if (ctx->decoder == NULL)
		ctx->decoder = CreateZPlay();

	// Open song
	int result = ctx->decoder->OpenFile(song.c_str(), ctx->decoder->GetFileFormat(song.c_str()));
	if(result == 0) {
		printf("Error: %s\n", ctx->decoder->GetError());
		return;
	}

	// Create UDP socket
	ctx->udp = ctx->udp ? ctx->udp : create_udp_socket(0);
	ctx->addr.sin_port = htons(1338);

	// decode song, send to client address
	ctx->decoder->SetCallbackFunc(stream_cb, (TCallbackMessage)(MsgWaveBuffer|MsgStop), (void*)ctx);
	ctx->decoder->Play();

	// when song is finished, return. TODO: use event instead of poll.
	/*while (true) {
		TStreamStatus status;
		ctx->decoder->GetStatus(&status); 
		if(status.fPlay == 0)
			break; // exit checking loop
		Sleep(100);
	}*/

	return;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	stream_cb
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, 
--						unsigned int param1, unsigned int param2)
-- RETURNS:		1 to continue decoding, 2 to stop
--
-- NOTES:		This callback is called when a chunk of the song has been decoded.
--				void * instance - zplay instance
--				void * user_data - should be a pointer to ClientContext
--				message - Message type to handle
--				param1 - Message parameters 1.
--				param2 - Message parameters 2.
----------------------------------------------------------------------------------------------*/
int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2) {
	ClientContext * ctx = (ClientContext *) user_data;
	sockaddr_in * client_addr = &ctx->addr;
	if (sendto(ctx->udp, (const char *)param1, param2, 0, (const sockaddr*)client_addr, sizeof(sockaddr_in)) < 0)
		return 2;
	
	Sleep(10);

	return 1;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	transmit_from_stream
--
-- DATE:		Mar 23, 2013
--
-- DESIGNER:	Dennis Ho
-- PROGRAMMER:	Dennis Ho, David Czech
--
-- INTERFACE:	void transmit_from_stream (SOCKET sock, ifstream& stream, streamsize packetSize)
-- RETURNS:		void
--
-- NOTES:      
----------------------------------------------------------------------------------------------------------------------*/
void transmit_from_stream (SOCKET sock, ifstream& stream, streamsize packetSize) {
	char buf[BUFSIZE];
	streamsize s = 0;
	while (!stream.eof()) {
		stream.read(buf, BUFSIZE);
		if (send(sock, buf, stream.gcount(), 0) < 0) {
			printf("Error sending from stream.\n");
		}
		s += stream.gcount();
	}
	cout << "Sent " << s << endl;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:   validate_param
--
-- DATE:       Mar 23, 2013
--
-- DESIGNER:   Dennis Ho, David Czech
-- PROGRAMMER: Dennis Ho
--
-- INTERFACE:  bool validate_param(string param, SOCKET error_sock, string error_msg)
-- RETURNS:    true if non-empty string
--
-- NOTES:      Sends an error message to the specified socket on failure
----------------------------------------------------------------------------------------------------------------------*/
bool validate_param(string param, SOCKET error_sock, string error_msg) {
	if (param.size() == 0) {
		send(error_sock, error_msg.c_str(), error_msg.length(), 0);
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:   process_download_file
--
-- DATE:       Mar 23, 2013
--
-- DESIGNER:   Dennis Ho, David Czech
-- PROGRAMMER: Dennis Ho
--
-- INTERFACE:  void process_download_file (ClientContext * ctx, string song)
-- RETURNS: nothing
--
-- NOTES:      
----------------------------------------------------------------------------------------------------------------------*/
void process_download_file (ClientContext * ctx, string song) {
	// Validate
	if (!validate_param(song, ctx->control, "Invalid download file request: no file specified!"))
	  return;

	cout << "Sending file data: " << song << endl;

	// Read file into stringstream
	ifstream f(song.c_str(), ifstream::binary);

	if (!f) {
		string error_msg("Invalid download file request: could not open file!");
		send(ctx->control, error_msg.data(), error_msg.size(), 0);
	}

	// Send file
	transmit_from_stream(ctx->control, f, BUFSIZE);

	closesocket(ctx->control);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:   process_upload_song
--
-- DATE:       Mar 23, 2013
--
-- DESIGNER:   David Czech, Dennis Ho
-- PROGRAMMER: David Czech
--
-- INTERFACE:  void process_upload_song (ClientContext * ctx, string song)
-- RETURNS: nothing
--
-- NOTES:  Gets file content from client and writes to a file.
----------------------------------------------------------------------------------------------------------------------*/
void process_upload_song(ClientContext * ctx, string song) {
	cout << "Uploading " << song << endl;
	// Validate
	if (!validate_param(song, ctx->control, "Invalid upload file request: no file name specified!"))
		return;

	// Give the client the go-ahead- literally.
	send(ctx->control, "go-ahead", 8, 0);

	int read = 0;
	char buffer[BUFSIZE];
	ofstream out(song.c_str(), ofstream::binary);

	if (!out)
		closesocket(ctx->control);

	while ((read = recv(ctx->control, buffer, BUFSIZE, 0)) > 0)
		out.write(buffer, read);

	closesocket(ctx->control);

	// A new song may be available now: refresh-services.
	//services_mutex.lock();
	// Clear songs
	s.songs.clear();
	// Find songs
	find_songs(s.songs);
	//services_mutex.unlock();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	process_join_voice
--
-- DATE:		Mar 23, 2013
--
-- DESIGNER:	David Czech
-- PROGRAMMER:	David Czech
--
-- INTERFACE:	void process_join_voice (ClientContext * ctx)
-- RETURNS:		nothing
--
-- NOTES:  Starts voice chat with client. Only one client may start voice communication at one time.
----------------------------------------------------------------------------------------------------------------------*/
void process_join_voice(ClientContext * ctx) {
	cout << "voice request: " << (s.microphone ? "processing..." : "already in use, dropped...")<< endl;

	// Only allow one microphone user, like a sempahore.
	if (s.microphone) {
		s.microphone = false;
		
		// Create zplay decoder instance, if it didn't already exist.
		if (ctx->decoder == NULL)
			ctx->decoder = CreateZPlay();
		else
			ctx->decoder->Close();

		// Open microphone input stream
		int result = ctx->decoder->OpenFile("wavein://", sfAutodetect);
		if(result == 0) {
			printf("Error: %s\n", ctx->decoder->GetError());
		}

		ctx->udp = create_udp_socket(1338);
		ctx->addr.sin_port = htons(1338);
		// set callback to stream to client (reuse stream_cb?)
		ctx->decoder->SetCallbackFunc(stream_cb, (TCallbackMessage)(MsgWaveBuffer|MsgStop), ctx);
		ctx->decoder->Play();

		// Open remote microphone playback stream
		ZPlay * player = CreateZPlay();
		player->SetSettings(sidSamplerate, 44100);// 44100 samples
		player->SetSettings(sidChannelNumber, 2);// 2 channel
		player->SetSettings(sidBitPerSample, 16);// 16 bit
		player->SetSettings(sidBigEndian, 0); // little endian
		int i;
		player->OpenStream(1, 1, &i, 2, sfPCM);
		player->Play();

		bool continue_streaming = true;

		// main loop:
		while (continue_streaming) {
			char buf[65507];
			
			sockaddr_in server = {0};
			int size = sizeof(server);
			
			// 	Recv from client
			int r = recvfrom (ctx->udp, buf, 65507, 0, (sockaddr*)&server, &size);
			if (r == 0)
				continue_streaming = false;

			// 	push data to stream
			player->PushDataToStream(buf, r);
		}
		cout << "Voice Chat session ended" << endl;
		ctx->decoder->Stop();
		s.microphone = true;
		closesocket(ctx->udp);
		ctx->udp = 0;
	}

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	add_files_to_songs
--
-- DATE:		Mar 23, 2013
--
-- DESIGNER:	David Czech
-- PROGRAMMER:	David Czech
--
-- INTERFACE:	void add_files_to_songs (std::vector<string>& songs, const char * file)
-- RETURNS:		nothing
--
-- NOTES:  Look for songs in the current directory, that match the glob pattern specified in "file", and add them to songs.
----------------------------------------------------------------------------------------------------------------------*/
void add_files_to_songs (std::vector<string>& songs, const char * file) {
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(file, &ffd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			 songs.push_back(string(ffd.cFileName));
		} while (FindNextFile(hFind, &ffd) != 0);
		FindClose(hFind);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	find_songs
--
-- DATE:		Mar 23, 2013
--
-- DESIGNER:	David Czech
-- PROGRAMMER:	David Czech
--
-- INTERFACE:	void find_songs (std::vector<string>& songs)
-- RETURNS:		nothing
--
-- NOTES:  Look for any file that have one of libzplay's supported file type extensions, add add them to songs.
----------------------------------------------------------------------------------------------------------------------*/
void find_songs (std::vector<string>& songs) {
	char songtypes[][7] = {
		{"*.flac"},
		{"*.mp3"},
		{"*.wav"},
		{"*.ogg"},
		{"*.ac3"}
	};

	// Look for any of the above file types and add them to the songs list.
	for (size_t i = 0; i < sizeof(songtypes); ++i)
	{
		add_files_to_songs(songs, songtypes[i]);
	}
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	get_channels
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	void get_channels(vector<string>& channelList)
-- RETURNS:		void
--
-- NOTES:		
----------------------------------------------------------------------------------------------*/
void get_channels(vector<string>& channelList) {	
	string channel;
			
	ifstream ifs("channels.lst");
	
	if (ifs) {
		while (!ifs.eof()) {
			getline(ifs, channel); 
			channelList.push_back(channel);
		}	
	}
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	extract_channel_info
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	ChannelInfo extract_channel_info(const string& channelString)
-- RETURNS:		ChannelInfo
--
-- NOTES:		
----------------------------------------------------------------------------------------------*/
ChannelInfo extract_channel_info(const string& channelString) {
	ChannelInfo ci;

	size_t lastSpace = channelString.find_last_of(" ");
	size_t portSeperator = channelString.find_last_of(":");

	if (lastSpace == string::npos || portSeperator == string::npos) {
		printf("Invalid channel string format.");
	}
	
	ci.name = channelString.substr(0, lastSpace);
	ci.addr.sin_family = AF_INET;
	ci.addr.sin_addr.s_addr = inet_addr(channelString.substr(lastSpace + 1, portSeperator - (lastSpace + 1)).c_str());
	ci.addr.sin_port = htons(atoi(channelString.substr(portSeperator + 1, channelString.length() - (portSeperator + 1)).c_str()));
	return ci;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	multicast_cb
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	int __stdcall multicast_cb(void* instance, void *user_data, 
--						TCallbackMessage message, unsigned int param1, unsigned int param2)
-- RETURNS:		int
--
-- NOTES:		
----------------------------------------------------------------------------------------------*/
int __stdcall multicast_cb(void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2) {
	ChannelInfo *ci = (ChannelInfo*)user_data;
	
	if (message == MsgWaveBuffer)
		while (param2) {
			size_t sb = min(1024, (int)param2);

			if (sendto(ci->sock, (const char *)param1, sb, 0, (const sockaddr*)&ci->addr, sizeof(sockaddr_in)) < 0) {
				return 2;
			} else {
				param1 += sb; // move buffer
				param2 -= sb; // reduce bytes remaining in buffer
			}
		}
	
	DWORD now = GetTickCount();

	while (GetTickCount() - now < 40)
		Sleep(5);

	return 1;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	start_channel
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	DWORD WINAPI start_channel(LPVOID lpParameter)
-- RETURNS:		DWORD
--
-- NOTES:		
----------------------------------------------------------------------------------------------*/
DWORD WINAPI start_channel(LPVOID lpParameter) {
	//return 0;
	int error;
	u_long ttl = 2;
	bool loopback = false;
	SOCKADDR_IN localAddr;

	string *channel = (string*)lpParameter;

	// Parse channel info
	ChannelInfo ci = extract_channel_info(*channel);   	

	// Create socket
	ci.sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (ci.sock == INVALID_SOCKET) {
	  cout << "Error: Invalid socket" << endl;
	  return false;
	}

	// Bind socket   
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = 0;

	if ((error = bind(ci.sock, (struct sockaddr*)&localAddr, sizeof(localAddr))) == SOCKET_ERROR) {
	  cout << "Error: Bind socket" << endl;
	  return false;
	}

	// Join multicast group
	struct ip_mreq stMreq;
	memset((char *)&stMreq, 0, sizeof(ip_mreq));
	stMreq.imr_multiaddr.s_addr = ci.addr.sin_addr.s_addr;
	stMreq.imr_interface.s_addr = INADDR_ANY;   

	if ((error = setsockopt(ci.sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&stMreq, sizeof(stMreq))) == SOCKET_ERROR) {
	  cout << "Error: Invalid socket" << endl;
	  return false;
	}

	// Set TTL
	if ((error = setsockopt(ci.sock, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl))) == SOCKET_ERROR) {
	  cout << "Error: Setting TTL" << endl;
	  return false;
	}

	// Disable loopback
	if ((error = setsockopt(ci.sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loopback, sizeof(loopback))) == SOCKET_ERROR) {
	  cout << "Error: Disable loopback" << endl;
	  return false;
	}

	// Start streaming, Create zplay Instance
	ZPlay *out = CreateZPlay();
	out->SetMasterVolume(100, 100);
	
	//services_mutex.lock();
	vector<string> list = retrieve_song_list(ci.name.c_str());

	for (size_t i = 0; i < list.size();)
	{
		cout << "Streaming song to channel: " << list[i] << endl;
				
		// Open song
		if (out->OpenFile(list[i].c_str(), out->GetFileFormat(list[i].c_str())) == 0) {
			printf("Error: %s\n", out->GetError());
			out->Release();
			//return 1;
		}
		// We're done with services for now, unlock while we play the song.
		//services_mutex.unlock();

		// decode song, send to multicast address
		out->SetCallbackFunc(multicast_cb, (TCallbackMessage)(MsgWaveBuffer|MsgStop), (void*)&ci);
		out->Play();

		TStreamStatus status;
		out->GetStatus(&status);
		while (status.fPlay == 0) {
			out->GetStatus(&status);
			Sleep(192);
		}

		while (status.fPlay != 0) {
			out->GetStatus(&status);
			Sleep(192);
		}

		// Lock to check the size, gets unlocked at the beginning of the loop.
		//services_mutex.lock();
		if ((++i) == list.size())
			i = 0;
	}
	
	// When the channel playlist loop is done, unlock services.
	//services_mutex.unlock();
	return 0;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	start_all_channels
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	void start_all_channels()
-- RETURNS:		void
--
-- NOTES:		
----------------------------------------------------------------------------------------------*/
void start_all_channels() {
	for (vector<string>::const_iterator it = s.channels.begin(); it != s.channels.end(); ++it)
		if (CreateThread(NULL, 0, start_channel, (LPVOID)&(*it), 0, NULL) == NULL)
			cerr << "Couldn't create channel thread!" << endl;	
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	retrieve_song_list
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	vector<string> retrieve_song_list(const char *playlistName)
-- RETURNS:		vector<string>
--
-- NOTES:		
----------------------------------------------------------------------------------------------*/
vector<string> retrieve_song_list(const char *playlistName) {
	vector<string> list;
	string song;
			
	ifstream ifs((string(playlistName) + ".lst").c_str());
	
	if (ifs) {
		while (!ifs.eof()) {
			getline(ifs, song); 
			list.push_back(song);
		}	
	}

	return list;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	main
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		
-- PROGRAMMERS: 	
--
-- INTERFACE:	int main(int argc, char const *argv[])
-- RETURNS:		int
--
-- NOTES:		The main function  to run the server program.
----------------------------------------------------------------------------------------------*/
int main(int argc, char const *argv[])
{
	// Open up a Winsock v2.2 session
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2,2);
	WSAStartup(wVersionRequested, &wsaData);
	
	// Initialize some services.
	s.microphone = true;
	/*s.songs.push_back("tol.flac");
	s.songs.push_back("song2.mp3");
	s.songs.push_back("song3.mp3");*/

	char * u = getenv ("USERPROFILE");
	string path(u);
	path += "\\Music\\";
	_chdir(path.c_str());

	find_songs(s.songs);
	//add_files_to_songs(s.songs, (path+"*.mp3").c_str());
	get_channels(s.channels);	
	start_all_channels();

	int sock = setup_listening();
	wait_for_connections (sock);
	
	
	return 0;
}