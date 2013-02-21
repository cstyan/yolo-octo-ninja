#include "a2.h"
#include <fstream>
#include <iostream>
#include <cstdio>

// We attempt to recv the maximum udp size, such that configuring the server packet size isn't necessary.
#define MAX_UDP_SIZE 65507

SOCKET g_SSock;

int SetupControlChannel ();
using namespace std;
void CALLBACK ControlChannelRecvComplete ( DWORD dwError, DWORD dwTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags ) {
	ServerParameters * sp = (ServerParameters *) lpOverlapped->hEvent;
	
	//SleepEx(1000, TRUE); // Could be UDP packets in transit.
	
	if (sp->control_buffer[0] == '\x04') // \x04 == EOT.
		printf("Control Channel signals EOT!\n");
		
	if (sp->file != NULL) {
		sp->file->flush();
		sp->file->close(); // clear file and start again.
		delete sp->file;
		sp->file = NULL;
	}
	WSABUF ackBUF;
	stats_t stats;
	stats.time = GetTickCount() - sp->startticks;
	stats.bytes = sp->total_bytes_sent; sp->total_bytes_sent = 0; // Reset sent.
	stats.end = 'A';
	ackBUF.buf = (char*)&stats;
	ackBUF.len = sizeof(stats);
	// Send stats
	DWORD bsent;
	WSASend (sp->tcp ? sp->socket : sp->control_socket, &ackBUF, 1, &bsent, 0, NULL, NULL);
	printf("Client disconnected: %lu total time.\n", GetTickCount() - sp->startticks);
	closesocket((sp->tcp) ? sp->socket : sp->control_socket);
	sp->alive = false;
}

void CALLBACK RecvComplete ( DWORD dwError, DWORD dwTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags ) {
	ServerParameters * sp = (ServerParameters *) lpOverlapped->hEvent;
	if (dwError) {
		printf("Got error %lu\n", dwError);
		sp->alive = false;
		free(sp->buffer);
		return;
	}
	
	sp->total_bytes_sent += dwTransferred;
	printf("Recv: %lu\n", dwTransferred);
	if (sp->file == NULL && strlen(sp->dest_file) != 0) {
		printf("Reopening %s\n", sp->dest_file);
		sp->file = new ofstream(sp->dest_file, ofstream::binary);
	}
	
	printf("Time: %lu\n", GetTickCount() - sp->startticks);
	// If we have a file to write to, write out the buffer.
	if (sp->file != NULL)
		sp->file->write(sp->buffer, dwTransferred);
	
	// Continue to recv until we get EOT.
	bool bEOT = false;
	if (sp->tcp) { // only check for EOT if using TCP.
		for (size_t i = 0; i < dwTransferred; i++) {
			if (sp->buffer[i] == 0x04)
				bEOT = true;
		}
	}
	// Then send an acknowledgement.
	if (bEOT) {
		sp->total_bytes_sent --; // The EOT char doesn't count!.
		if (sp->file != NULL) {
			sp->file->flush();
			sp->file->close(); // clear file and start again.
			delete sp->file;
			sp->file = NULL;
		}
		WSABUF ackBUF;
		stats_t stats;
		stats.time = GetTickCount() - sp->startticks;
		stats.bytes = sp->total_bytes_sent; sp->total_bytes_sent = 0; // Reset sent.
		stats.end = 'A';
		ackBUF.buf = (char*)&stats;
		ackBUF.len = sizeof(stats); // two size_t's and a char
		// Send stats
		DWORD bsent;
		WSASend (sp->tcp ? sp->socket : sp->control_socket, &ackBUF, 1, &bsent, 0, NULL, NULL);
		printf("Client disconnected: %lu total time.\n", GetTickCount() - sp->startticks);
		closesocket((sp->tcp) ? sp->socket : sp->control_socket);
		sp->already_recving = false;
		sp->alive = false;
	  //if (sp->tcp)
		//	WSASendTo(sp->socket, &ackBUF, 1, NULL, 0, (sockaddr*)&sp->addr, sp->addr_size, lpOverlapped, SendComplete);
		return;
	}
	// Disconnected.
	if (dwTransferred == 0) {
		sp->alive = false;
		return;
	}
	DWORD flags = 0;
	WSABUF recvBuf;
	recvBuf.buf = sp->buffer;
	recvBuf.len = MAX_UDP_SIZE;
	memset(sp->buffer, 0, MAX_UDP_SIZE);
	printf("WSARecvFrom udp\n");
	sp->already_recving = true;
	if (WSARecvFrom(sp->socket, &recvBuf, 1, NULL, &flags, (struct sockaddr *)&sp->addr, &sp->addr_size, lpOverlapped, RecvComplete) != 0) {
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
			printf("WSARecv failure: %d\n", err);
	}
}

/* Server thread. */
DWORD WINAPI ServerProc (LPVOID lpParameter) {
  ServerParameters * sp = (ServerParameters *)lpParameter;
	int	client_len, port = sp->port;
	SOCKET new_sd;
	struct	sockaddr_in server, client;
	
	// Create OVERLAPPED TCP socket
	if ((g_SSock = WSASocket(AF_INET, sp->tcp ? SOCK_STREAM : SOCK_DGRAM, 0, NULL, 0,  WSA_FLAG_OVERLAPPED))
					== INVALID_SOCKET) {
		printf("Failed to get a socket %d\n", WSAGetLastError());
		return 1;
	}

	// Initialize and set up the address structure
	memset((char *)&server, 0, sizeof(struct sockaddr_in)); 
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client
	
	// Bind an address to the socket
	if (bind(g_SSock, (struct sockaddr *)&server, sizeof(server)) == -1) {
		perror("Can't bind name to socket");
		return 1;
	}
	// if tcp, listen for connections, queue up to 5 connect requests, accept...
	if (sp->tcp)
		listen(g_SSock, 5);
	else // if udp, use server socket as sending socket.
		sp->socket = g_SSock;

	// Create and clean recv buffer, and assign length.
	sp->buffer = (char*)malloc(MAX_UDP_SIZE);
	sp->size   = MAX_UDP_SIZE;
	memset(sp->buffer, 0, MAX_UDP_SIZE);

	while (TRUE) { // s/TRUE/bRunning/
		if (sp->tcp) {
			client_len= sizeof(client); 
			if ((new_sd = accept (g_SSock, (struct sockaddr *)&client, &client_len)) == -1) {
				fprintf(stderr, "accept failed, server closed?\n"); 
				break;
			}
			sp->socket = new_sd;
			// Start client handler
			printf("New client:  %s\n", inet_ntoa(client.sin_addr));
		}
    
    /* Initialize server parameters to handle client. */
    sp->alive = true;
    sp->startticks = GetTickCount();
    sp->addr_size  = sizeof(sockaddr_in);
    memcpy((char*)&sp->addr, &client, sizeof(client));
    
    /* Pass the pointer to sp via hEvent field in OVERLAPPED. */
    OVERLAPPED * ov = new OVERLAPPED;
    ov->hEvent = sp;

		// The following should run concurrently to handle multiple clients.
		DWORD flags = 0;
		WSABUF recvBuf;
		// Create and clean recv buffer, and assign length.
		recvBuf.buf = sp->buffer;
		recvBuf.len = sp->size;
		
		if (!sp->tcp) {
			sp->control_socket = SetupControlChannel();
			// Intialize startticks for UDP.
			sp->startticks = GetTickCount();
			// Start the Recv for EOT
			OVERLAPPED * ctl_ov = new OVERLAPPED;
			ctl_ov->hEvent = sp;
			WSABUF ctl_rbuf;
			sp->control_buffer = ctl_rbuf.buf = (char*)malloc(1);
			ctl_rbuf.len = 1;
			WSARecv(sp->control_socket, &ctl_rbuf, 1, NULL, &flags, ctl_ov, ControlChannelRecvComplete);
		}
		printf("Wsarecvfrom udp\n");
		
		if (!sp->already_recving && WSARecvFrom(sp->socket, &recvBuf, 1, NULL, &flags, (struct sockaddr *)&sp->addr, &sp->addr_size, ov, RecvComplete) != 0) {
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING) {
				free(sp->buffer);
				delete ov;
				break;
			}
		}
		
		while (sp->alive) {
			SleepEx(INFINITE, TRUE);
		}
	}
	delete sp;
	closesocket(g_SSock);
	SetStatus(g_Hwnd, "Ready. Server stopped.");
	return 0;
}

int SetupControlChannel () {
	int	client_len, port = 31337; // hard coded control channel port.
	SOCKET sd, new_sd;
	struct	sockaddr_in server, client;
	
	if ((sd = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,  WSA_FLAG_OVERLAPPED))
					== INVALID_SOCKET) {
		printf("Failed to get a control socket %d\n", WSAGetLastError());
		return 1;
	}

	// Initialize and set up the address structure
	memset((char *)&server, 0, sizeof(struct sockaddr_in)); 
	server.sin_family = AF_INET;
	server.sin_port = htons(port); 
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client
	
	// Bind an address to the socket
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
		perror("Can't bind name to socket");
		return 1;
	}
	listen(sd, 5);

	client_len= sizeof(client); 
	if ((new_sd = accept (sd, (struct sockaddr *)&client, &client_len)) == -1) {
		fprintf(stderr, "accept failed, server closed?\n");
	}
	printf("Control Socket connected!\n");
	closesocket(sd);
	
	return new_sd;
}