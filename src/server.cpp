#include <iostream>
#include "CommAudio.h"
#include "server-file.h"
#include "libzplay.h"
#include <fstream>
#include <sstream>

using namespace std;
using namespace libZPlay;

Services s;

struct ClientContext {
	SOCKET control;
	SOCKET udp;
   SOCKET download;
	sockaddr_in addr;
};

// Forward declarations. Only a couple of these are really needed, but all are listed for completeness
int setup_listening (int port = 1337);
void wait_for_connections (int lsock);
string recv_request(SOCKET);
DWORD WINAPI handle_client(LPVOID);
void process_stream_song(ClientContext*, string);
void process_download_file(ClientContext * ctx, string song);
int __stdcall stream_cb (void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2);
istringstream read_file_to_stream(string path);
void transmit_from_stream(SOCKET sock, istringstream& stream, streamsize packetSize);

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
			// Generate services list.
			string services = ListServices(s); 
			// Send the list of services.
			send(client, services.data(), services.size(), 0);
			continue;
		}

		// Parse requests with commands (stream song request, download song request, etc)
		string req_command;
		stringstream req_stream(request);
		if (req_stream >> req_command) {
			if (req_command == "D") {
				string file;
				if (req_stream >> file && file.size() != 0) {
					cout << "Sending file data: " << file << endl;
					process_download_file(ctx, file);
				} else {
					send (client, "Invalid download file request: no file specified!", 49, 0);
					closesocket(client); // Close this socket: client download creates a new socket. 
				}
			} else if (req_command == "S") {
				string song;
				if (req_stream >> song && song.size() != 0) {
					cout << "Starting song stream: " << song << endl;
					process_stream_song(ctx, song);
				} else {
					send (client, "Invalid stream song request: no song specified!", 49, 0);
				}
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
-- FUNCTION:   read_file_to_stream
--
-- DATE:       Mar 23, 2013
--
-- DESIGNER:   Dennis Ho
--
-- PROGRAMMER: Dennis Ho
--
-- INTERFACE:  istringstream read_file_to_stream (string path)
--
-- RETURNS: 
--
-- NOTES:      
----------------------------------------------------------------------------------------------------------------------*/
istringstream read_file_to_stream (string path) {
   ifstream is(path.c_str(), ios::binary);

   // Find length
   is.seekg(0, std::ios::end);
   long length = is.tellg();
   is.seekg(0, std::ios::beg);
   
   // Allocate memory
   char *data = new char[length];
   
   // Read into stream
   is.read(data, length);
   istringstream iss;
   iss.str(string(data));

   // Clean up
   delete[] data;
   is.close();

   return iss;
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
void transmit_from_stream (SOCKET sock, istringstream& stream, streamsize packetSize) {
   char buf[BUFSIZE];

   while (stream.read(buf, BUFSIZE))
      if (send(sock, buf, BUFSIZE, 0) < 0)
      {
         // TODO: error handling   
      }
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
-- NOTES:      Currently using control channel to transmit in order to match current client.
--             Should be changed later to create separate socket.
----------------------------------------------------------------------------------------------------------------------*/
void process_download_file (ClientContext * ctx, string song) {
   // Read file into stringstream
   istringstream iss = read_file_to_stream(song);
      
   // TODO: create new socket - client is using control channel currently
   ctx->download = ctx->control;
   
   // Send file
   transmit_from_stream(ctx->download, iss, BUFSIZE); 
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