/*---------------------------------------------------------------------------------------------
--	SOURCE FILE: common.cpp
--
--	PROGRAM:	client.exe, server.exe
--
--	FUNCTIONS:	SOCKET create_udp_socket (int port)
--
--	DATE:		23/Mar/2013
--
--	DESIGNERS:	
--	PROGRAMMERS: 
--
--	NOTES:		Some functions common between client and server programs.
------------------------------------------------------------------------------------------------*/

#include "commaudio.h"

using namespace std;
/*
* create_udp_socket (int port = 0) 
*	port - bind to set port, defaults to system port selection
* Notes: Create a UDP socket and bind it to an address provided by the system.
* Returns the new UDP socket created
*/
SOCKET create_udp_socket (int port) {
	SOCKET sd;
	sockaddr_in client;

	// Create a datagram socket
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		cerr << "Could not create UDP socket!" << endl;
	}

	// Bind local address to the socket
	memset((char *)&client, 0, sizeof(client));
	client.sin_family = AF_INET;
	client.sin_port = htons(port);  // bind to any available port
	client.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sd, (struct sockaddr *)&client, sizeof(client)) == -1) {
		cerr << "Could not bind udp socket" << endl;
	}

	return sd;
}