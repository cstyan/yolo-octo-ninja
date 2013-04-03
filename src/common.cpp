/*---------------------------------------------------------------------------------------------
--	SOURCE FILE: common.cpp
--
--	PROGRAM:	client.exe, server.exe
--
--	FUNCTIONS:	SOCKET create_udp_socket (int port)
--
--	DATE:		23/Mar/2013
--
--	DESIGNERS:	David Czech
--	PROGRAMMERS: David Czech
--
--	NOTES:		Some functions common between client and server programs.
------------------------------------------------------------------------------------------------*/

#include "commaudio.h"

using namespace std;

/*----------------------------------------------------------------------------------------------
-- FUNCTION:	create_udp_socket
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:		David Czech
-- PROGRAMMERS: 	David Czech
--
-- INTERFACE:	SOCKET create_udp_socket (int port)
-- RETURNS:		SOCKET - the new UDP socket created
--
-- NOTES:		Creates a UDP socket and bind it to an address provided by the system.
--				port - bind to set port, defaults to system port selection
----------------------------------------------------------------------------------------------*/
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