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

struct ClientContext {
	SOCKET control;
	SOCKET udp;
	sockaddr_in addr;
};

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
void procress_upload_song(ClientContext * ctx, string song);
void procress_join_channel(ClientContext * ctx, string channel);
void procress_join_voice(ClientContext * ctx);
int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2);
void transmit_from_stream(SOCKET sock, istringstream& stream, streamsize packetSize);
bool validate_param(string param, SOCKET error_sock, string error_msg);
void add_files_to_songs (std::vector<string>& songs, const char * file);

/*
* setup_listening (int port = 1337)
*   port - the port number to listen on
* Notes: Creates a TCP socket and listens on a port (1337 by default)
* Returns the newly created listening socket.
*/
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

/*
* wait_for_connections (int lsock)
*   lsock - the listening socket to wait on.
* Notes: Wait for new connections and start a thread to handle accepted clients.
* Returns void
*/
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
		ctx->control = new_sd;
		ctx->addr    = client;
		
		if (CreateThread(NULL, 0, handle_client, (LPVOID)ctx, 0, NULL) == NULL)
			cerr << "Couldn't create client handler thread!" << endl;
	}
}

/*
* handle_client (LPVOID lpParameter) 
*   lpParamater - should be a pointer to the ClientContext of the client to handle.
* Notes: Handle a client's requests.
* Returns unused.
*/
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
			// refresh-services
			// Clear songs
			//s.songs.clear();
			// Find songs
			//find_songs(s.songs)

			// Generate services list.
			string services = ListServices(s); 
			// Send the list of services.
			send(client, services.data(), services.size(), 0);
			continue;
		}

		// Parse requests with commands (stream song request, download song request, etc)
		string req_command;
		stringstream req_stream(request);
		if (req_stream >> req_command >> ws) {
			if (req_command == "V") { // Parameterless
				procress_join_voice(ctx);
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
					procress_upload_song(ctx, param);            
				else if (req_command == "C")
					procress_join_channel(ctx, param);
			}
		}
	}

	return 0;
}

/*
* recv_request (SOCKET client) - recv a request from a client.
*   client - the client SOCKET to recv the request from.
* Notes: Recv until a newline is encountered.
* Returns the data received without the terminating newline.
*/
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

/*
* process_stream_song (SOCKET client, ClientContext * ctx, string song) .
*   ctx - should be a pointer to the ClientContext of the client to handle.
* 	song - a C++ string containing the song that will be streamed to the client.
* Notes: Recv until a newline is encountered.
* Returns the data received without the terminating newline.
*/
void process_stream_song (ClientContext * ctx, string song) {
	cout << "Streaming song: " << song << endl;
	// Validate
	if (!validate_param(song, ctx->control, "Invalid stream song request: no song specified!"))
		return;

	// Create zplay Instance
	ZPlay * out = CreateZPlay();

	// Open song
	int result = out->OpenFile(song.c_str(), out->GetFileFormat(song.c_str()));
	if(result == 0) {
		printf("Error: %s\n", out->GetError());
		out->Release();
		return;
	}

	// Create UDP socket
	ctx->udp = create_udp_socket(0);
	ctx->addr.sin_port = htons(1338);

	// decode song, send to client address
	out->SetCallbackFunc(stream_cb, (TCallbackMessage)(MsgWaveBuffer|MsgStop), (void*)ctx);
	out->Play();

	// when song is finished, return. TODO: use event instead of poll.
	while (true) {
		TStreamStatus status;
        out->GetStatus(&status); 
        if(status.fPlay == 0)
            break; // exit checking loop
        Sleep(100);
	}

	return;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:   transmit_from_stream
--
-- DATE:       Mar 23, 2013
--
-- DESIGNER:   Dennis Ho
--
-- PROGRAMMER: Dennis Ho
--
-- INTERFACE:  void transmit_from_stream (SOCKET sock, istringstream stream, streamsize packetSize)
--
-- RETURNS: 
--
-- NOTES:      
----------------------------------------------------------------------------------------------------------------------*/
void transmit_from_stream (SOCKET sock, ifstream& stream, streamsize packetSize) {
	char buf[BUFSIZE];
	size_t s = 0;
	while (!stream.eof()) {
		stream.read(buf, BUFSIZE);
		if (send(sock, buf, stream.gcount(), 0) < 0) {
			// TODO: error handling
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
-- DESIGNER:   Dennis Ho
--
-- PROGRAMMER: Dennis Ho
--
-- INTERFACE:  bool validate_param(string param, SOCKET error_sock, string error_msg)
--
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
-- DESIGNER:   Dennis Ho
--
-- PROGRAMMER: Dennis Ho
--
-- INTERFACE:  void process_download_file (ClientContext * ctx, string song)
--
-- RETURNS: 
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

void procress_upload_song(ClientContext * ctx, string song) {
	// Validate
	if (!validate_param(song, ctx->control, "Invalid upload file request: no file name specified!"))
		return;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:   procress_join_channel
--
-- DATE:       Mar 26, 2013
--
-- DESIGNER:   Dennis Ho
--
-- PROGRAMMER: Dennis Ho
--
-- INTERFACE:  void procress_join_channel(ClientContext * ctx, string channel)
--
-- RETURNS: 
--
-- NOTES:      
----------------------------------------------------------------------------------------------------------------------*/
void procress_join_channel(ClientContext * ctx, string channel) {
	// Validate
	if (!validate_param(channel, ctx->control, "Invalid channel request: no channel specified!"))
		return;
   
   for (vector<string>::const_iterator it = s.channels.begin(); it != s.channels.end(); ++it)
      if (it->compare(channel) == 0)
      {

      }
}

void procress_join_voice(ClientContext * ctx) {   
}

/*
* stream_cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2)
*   void * instance - zplay instance
*	void * user_data - should be a pointer to ClientContext
*	message - Message type to handle
*	param1 - Message parameters 1.
*	param2 - Message parameters 2.
* Notes: This callback is called when a chunk of the song has been decoded.
* Return: 1 to continue decoding, 2 to stop.
*/
int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2) {
	ClientContext * ctx = (ClientContext *) user_data;
	sockaddr_in * client_addr = &ctx->addr;

	if (sendto(ctx->udp, (const char *)param1, param2, 0, (const sockaddr*)client_addr, sizeof(sockaddr_in)) < 0)
		return 2;
	
	Sleep(40);

	return 1;
}


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

void find_songs (std::vector<string>& songs) {
	char songtypes[][7] = {
		{"*.flac"},
		{"*.mp3"},
		{"*.wav"},
		{"*.ogg"},
		{"*.ac3"}
	};

	// Look for any of the above file types and add them to the songs list.
	for (int i = 0; i < sizeof(songtypes); ++i)
	{
		add_files_to_songs(songs, songtypes[i]);
	}
}

ChannelInfo extractChannelInfo(const string& channel) {
   ChannelInfo ci;

   ci.name = "The Peak"; // TODO: un-hardcode
   ci.addr.sin_family = AF_INET;
   ci.addr.sin_addr.s_addr = inet_addr("234.5.6.7");
   ci.addr.sin_port = htons(8910);
      
   return ci;
}

int __stdcall multicast_cb(void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2) {
	ChannelInfo *ci = (ChannelInfo*)user_data;

   if (sendto(ci->sock, (const char *)param1, param2, 0, (const sockaddr*)&ci->addr, sizeof(sockaddr_in)) < 0)
		return 2;   
	
	Sleep(40);

	return 1;
}

DWORD WINAPI start_channel(LPVOID lpParameter) {   
   int error;
   u_long ttl = 2;
   bool loopback = false;
   SOCKADDR_IN localAddr;

   string *channel = (string*)lpParameter;

   // Parse channel info
   ChannelInfo ci = extractChannelInfo(*channel);   	
   
   // Create socket
   ci.sock = socket(AF_INET, SOCK_DGRAM, 0);

   if (ci.sock == INVALID_SOCKET) {
      // TODO: error handling
   }

   // Bind socket   
   localAddr.sin_family = AF_INET;
   localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   localAddr.sin_port = 0;

   if ((error = bind(ci.sock, (struct sockaddr*)&localAddr, sizeof(localAddr))) == SOCKET_ERROR) {
      // TODO: error handling
   }

   // Join multicast group
   struct ip_mreq stMreq;
   memset((char *)&stMreq, 0, sizeof(ip_mreq));
   stMreq.imr_multiaddr.s_addr = inet_addr("234.5.6.7");//ci.addr.sin_addr.s_addr;
   stMreq.imr_interface.s_addr = INADDR_ANY;   

   if ((error = setsockopt(ci.sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&stMreq, sizeof(stMreq))) == SOCKET_ERROR) {
      // TODO: error handling
   }

   // Set TTL
   if ((error = setsockopt(ci.sock, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl))) == SOCKET_ERROR) {
      // TODO: error handling
   }

   // Disable loopback
   if ((error = setsockopt(ci.sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loopback, sizeof(loopback))) == SOCKET_ERROR) {
      // TODO: error handling
   }    

   // Start streaming
   // Create zplay Instance
	ZPlay *out = CreateZPlay();

   for (vector<string>::const_iterator it = s.songs.begin(); it != s.songs.end(); ++it) {
      cout << "Streaming song to channel: " << *it << endl;

	   // Open song
	   if (out->OpenFile(it->c_str(), out->GetFileFormat(it->c_str())) == 0) {
		   printf("Error: %s\n", out->GetError());
		   out->Release();
		   return 1;
	   }
      	   
	   // decode song, send to multicast address
	   out->SetCallbackFunc(multicast_cb, (TCallbackMessage)(MsgWaveBuffer|MsgStop), (void*)&ci);
	   out->Play();      	   	         
   }        
   
   Sleep(10000000);

   return 0;
}

void start_all_channels() {
   for (vector<string>::const_iterator it = s.channels.begin(); it != s.channels.end(); ++it)
      if (CreateThread(NULL, 0, start_channel, (LPVOID)&(*it), 0, NULL) == NULL)
		   cerr << "Couldn't create channel thread!" << endl;   
}

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
	s.channels.push_back("The Peak");
   start_all_channels();

	int sock = setup_listening();
	wait_for_connections (sock);
	
	
	return 0;
}