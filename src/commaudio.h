#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif
#ifndef COMMAUDIO_H
#define COMMAUDIO_H
#define BUFSIZE 1024
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "resource.h"

// Services structure - contains information about available songs, channels, resources etc.
struct Services {
	std::vector<std::string> songs;     // "filename1.wav"
	std::vector<std::string> channels;  // "name ip:port"
	bool microphone;                    // true if microphone is available
	Services () : microphone(false) {};
};

// Common Networking
SOCKET create_udp_socket (int port = 0);

// Utilities
void ParseServicesList(std::string list, Services& s);
std::string ListServices(const Services& s);
void printStruct(const Services& s);
bool parse_ip_port (std::string& s, std::string& ip, unsigned short& port);

// GUI (these prototypes are valid for both client and server, but implemented differently!)
void create_gui (HWND hWnd);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hwnd);
ATOM MyRegisterClass(HINSTANCE hInstance);

#endif
