// Assignment 2 Client Main
// client-net.cpp : Defines the networking handling code.
//
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <assert.h>
#include "a2.h"
#include "resource.h"

using namespace std;

int active_client_threads = 0;

int SetupControlChannel (Parameters * p);
void ClientProcCleanup (int sd, OVERLAPPED * sendOv, Parameters * p, void * buffer ); // Internal threadproc cleanup

// send completion routine.
void CALLBACK SendComplete ( DWORD dwError, DWORD dwTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags ) {
	if (dwError != 0) {
		printf("Sendcomplete error: %lu\n", dwError);
	}
	// Get our userdata from the ov's hevent
	Parameters * p = (Parameters*) lpOverlapped->hEvent;
	p->total_bytes_sent += dwTransferred;
	DWORD dwBytes;
	WSABUF sendBuf;
	sendBuf.buf = p->buffer;
	sendBuf.len = p->size;

	if (dwTransferred != p->size)
		printf("partial send! only %lu of %u\n", dwTransferred, p->size);

	// Using file source, and it's at EOF
	if (!p->source_procedural) {
		if (!(*p->file)) {
			p->multiple = 0;
	  } else {
			memset(p->buffer, 0, p->size);
			// Using file source, not at EOF, try reading next p->size bytes.
			p->file->read(p->buffer, p->size);
			p->size = min(p->size, (size_t)p->file->gcount());
			sendBuf.buf = p->buffer;
			sendBuf.len = p->size;
		}
	}
	
	// p->multiple is the thread exit flag, as soon as we do the decrement,
	// it could become zero and that could mean the death of this thread.
	if ( (p->multiple-1) == 0 ) { // so we check if we're about to become zero
		//printf("Done!\n");       // to display our done message.
		/*DWORD time = GetTickCount() - p->startticks;
		string status("Done, time: ");
		{stringstream ss; ss << time << " ms"; status += ss.str();}
		SetStatus(g_Hwnd, status.c_str());*/
	}
	if (p->source_procedural) // Only decrement multiplier if source is procedural
		p->multiple--;          // At this point, thread could die
													  // And that doesn't matter,
	if (p->multiple != 0) {   // cause like Cave Johnson always says: "We're done here."
		//printf("messages remaining %u\n", p->multiple);

		// Update status. // "hot" line: Slow!!!
		//string status("Sending messages... "); 
		//{stringstream ss; ss << p->multiple << " remaining"; status += ss.str();}
		//SetStatus(g_Hwnd, status.c_str());
	
		int send_result;
		if (p->tcp)
			send_result = WSASend  ( p->socket, &sendBuf, 1, &dwBytes, 0, /*Buffers*/
											lpOverlapped, SendComplete ); /*Overlapped*/
		else
			send_result = WSASendTo( p->socket, &sendBuf, 1, &dwBytes, 0, /*Buffers*/
										(struct sockaddr *)&p->server, sizeof(p->server),/*Target*/
															lpOverlapped, SendComplete ); /*Overlapped*/
		if (send_result != 0) {
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING) {
				printf("WSASend failure: %d\n", err);
				string failure("Sending failure: ");
				{stringstream ss; ss << err; failure += ss.str();}
				MessageBox(g_Hwnd, failure.c_str(), "Error sending", 0);
				p->multiple = 0;
			}
		}
	}
}

DWORD WINAPI ClientProc (LPVOID lpParameter) {
	active_client_threads ++; // Need to synchronize...
	Parameters * p = (Parameters*) lpParameter;
	p->file = new ifstream(p->source_file, ifstream::binary);
	p->startticks = GetTickCount();
	
	//
	// Attempt to allocate the procedural/file buffer now, or fail out early.
	p->buffer = (char *) malloc(p->size);
	if (p->buffer == NULL) {
		printf("Error: could not allocate enough memory for procedural buffer");
		ClientProcCleanup(0, NULL, p,  p->buffer);
		return 1;
	} else {
		/* Buffer has been allocated, now we will either procedurally generate contents
			 or read from a file. */
		if (p->source_procedural) {
			for (size_t i = 0; i < p->size; i++)
				p->buffer[i] = (i%94) + 32;
		} else {
			if (p->file != NULL) {
				p->file->read(p->buffer, p->size);
				size_t a = p->file->gcount();
				p->size = min(p->size, (size_t)a);
			} else {
				printf("No file!\n");
			}
		}
	}

	SOCKET sd;
	int port = p->port;
	struct hostent	*hp;
	struct sockaddr_in server;
	
	OVERLAPPED *sendOv = new OVERLAPPED;
	ZeroMemory( sendOv, sizeof( OVERLAPPED ) );
	
	SetStatus(g_Hwnd, "Connecting... looking up host");
	//
	// Initialize and set up the address structure
	memset((char *)&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if ((hp = gethostbyname(p->dest_host)) == NULL) {
		MessageBox(0, "Unknown server address\n", 0, 0);
		SetStatus(g_Hwnd, "Could not resolve host.");
		ClientProcCleanup(0, sendOv, p,  p->buffer);
		return 1;
	}
	// Copy the server address from gethostname result to parameters.
	memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);
	memcpy((char *)&p->server, &server, sizeof(struct sockaddr_in));
	SetStatus(g_Hwnd, "Connecting... host found");
	
	//
	// Handle TCP/UDP
	if (p->tcp) {
		// Create non-blocking TCP socket
		if ((sd = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,  WSA_FLAG_OVERLAPPED))
						== INVALID_SOCKET) {
			printf("Failed to get a socket %d\n", WSAGetLastError());
			ClientProcCleanup(sd, sendOv, p,  p->buffer);
			return 1;
		}
		p->socket = sd;
		
		// Connecting to the server
		if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
			// todo: use WSAGetLastError + Formatmessage ...
			MessageBox(0, "Can't connect to server\n", 0, 0);
			SetStatus(g_Hwnd, "Could not connect to server.");
			ClientProcCleanup(sd, sendOv, p,  p->buffer);
			return 1;
		}
		SetStatus(g_Hwnd, "Connected..."); // hostname: hp->h_name // ip: inet_ntoa(server.sin_addr)

		// Transmit data through the socket
		DWORD dwBytes;
		WSABUF sendBuf;
		sendBuf.buf = p->buffer;
		sendBuf.len = p->size;

		// MS's certified method of passing userdata to completetion routines
		// is through the unused hEvent element in OVERLAPPED
		sendOv->hEvent = lpParameter;
		SetStatus(g_Hwnd, "Sending messages...");
		if (WSASend( sd, &sendBuf, 1, &dwBytes, 0, sendOv, SendComplete ) != 0) {
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
				printf("WSASend failure: %d\n", err);
		}
	} else /* (UDP) */ {
		// Before we connect via UDP, we set up a reliable control channel
		p->control_socket = SetupControlChannel(p);

		// Create overlapped UDP socket
		if ((sd = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0,  WSA_FLAG_OVERLAPPED))
						== INVALID_SOCKET) {
			printf("Failed to get a socket %d\n", WSAGetLastError());
			ClientProcCleanup(sd, sendOv, p,  p->buffer);
			return 1;
		}
		p->socket = sd;

		// Transmit data through the socket
		DWORD dwBytes;
		WSABUF sendBuf;
		sendBuf.buf = p->buffer;
		sendBuf.len = p->size;

		// MS's certified method of passing userdata to completetion routines
		// is through the unused hEvent element in OVERLAPPED
		sendOv->hEvent = lpParameter;
		SetStatus(g_Hwnd, "Sending messages...");
		
		if (WSASendTo( sd, &sendBuf, 1, &dwBytes, 0, (struct sockaddr *)&p->server, sizeof(p->server), sendOv, SendComplete ) != 0) {
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
				printf("WSASend failure: %d\n", err);
		}
	}

	while (p->multiple != 0) {
		SleepEx(INFINITE, TRUE); // sleep in an alertable state, such that the completion routine can be invoked.
	}
	//
	// Send final EOT: buffer with just EOT, length 1.
	// if we're using UDP, we use the control channel socket.
	DWORD sent;
	WSABUF sendBuf; sendBuf.buf = (char *)"\x04"; sendBuf.len = 1;
	if (!p->tcp)
		Sleep(1000); // We don't want to post EOT before the UDP packet arrives.
	if (WSASend( p->tcp ? sd : p->control_socket, &sendBuf, 1, &sent, 0, NULL, NULL ) != 0) {
		printf("Problem sending EOT: %d\n", WSAGetLastError());
	}
	
	DWORD flags = 0;
	stats_t stats;
	sendBuf.buf = (char *) &stats; sendBuf.len = sizeof(stats);
	if (WSARecv( p->tcp ? sd : p->control_socket, &sendBuf, 1, &sent, &flags, NULL, NULL ) != 0) {
		printf("Problem recv end ack %d\n", WSAGetLastError());
	} else {
		if (stats.end != 'A')
			printf("Didn't get end ack?!\n");
		else {
			printf("Stats from server: Bytes Recieved %u Time %u\n", stats.bytes, stats.time);
			DWORD time = GetTickCount() - p->startticks;
			
			string status("Done, time: ");
			{stringstream ss; ss << time << " ms"; status += ss.str();}
			{stringstream ss; ss << " sent: " << p->total_bytes_sent << " B"; status += ss.str();}
			SetStatus(g_Hwnd, status.c_str());
		}
	}
	//SetStatus(g_Hwnd, "Done!"); 

	//shutdown(sock, SD_BOTH);
	ClientProcCleanup(sd, sendOv, p,  p->buffer);
	return 0;
}

void ClientProcCleanup (int sd, OVERLAPPED * sendOv, Parameters * p, void * buffer ) {
	// Need to synchronize...
	//if ((--active_client_threads) == 0)
	//	SetStatus(g_Hwnd, "Ready..."); // now set by comp port.
	closesocket (sd);
	if (p && p->file)
		delete p->file;
	if (buffer)
		free(buffer);
	if (sendOv)
		delete sendOv;
	if (p)
		delete p;
}

int SetupControlChannel (Parameters * p) {
	int sd;
	if ((sd = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,  WSA_FLAG_OVERLAPPED))
						== INVALID_SOCKET) {
		printf("Failed to get a control socket %d\n", WSAGetLastError());
		return -1;
	}
	
	// Address is the same as server, except for port.
	sockaddr_in server_control_addr;
	memcpy(&server_control_addr, &p->server, sizeof(p->server));
	server_control_addr.sin_port = htons(31337); // hard coded control channel port.
	
	// Connecting to the server
	if (connect (sd, (struct sockaddr *)&server_control_addr, sizeof(server_control_addr)) == -1) {
		MessageBox(0, "Could not establish control channel!\n", 0, 0);
		return -1;
	}
	return sd;
}