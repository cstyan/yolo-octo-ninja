#define _WIN32_WINNT 0x0502
#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif
#ifndef COMMAUDIO_H
#define COMMAUDIO_H

#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
// Mingw doesn't have this, even in the latest w32api package
#ifndef PBS_MARQUEE
#define PBS_MARQUEE             0x08
#define PBM_SETMARQUEE          (WM_USER+10)
#endif

#include <Windowsx.h>
#include "resource.h"
#include "libzplay.h"

// Services structure - contains information about available songs, channels, resources etc.
struct Services {
	std::vector<std::string> songs;     // "filename1.wav"
	std::vector<std::string> channels;  // "name ip:port"
	bool microphone;                    // true if microphone is available
	Services () : microphone(false) {};
};

struct ClientContext {
	SOCKET control;
	SOCKET udp;
	sockaddr_in addr;
	libZPlay::ZPlay * decoder;
};

// Common Networking
SOCKET create_udp_socket (int port = 0);

// Client Networking
extern char server[256];
extern int sock;
void send_ec (int sock, const char* buf, size_t len, int flags);
int comm_connect (const char * host, int port = 1337);
void request_services(SOCKET sock);
std::string recv_services (int sd);
ClientContext * start_microphone_stream();

// Utilities
void ParseServicesList(std::string list, Services& s);
std::string ListServices(const Services& s);
void printStruct(const Services& s);
bool parse_ip_port (std::string& s, std::string& ip, unsigned short& port);
void get_and_display_services(int control);

// GUI (these prototypes are valid for both client and server, but implemented differently!)
void create_gui (HWND hWnd);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hwnd);
ATOM MyRegisterClass(HINSTANCE hInstance);

#endif
