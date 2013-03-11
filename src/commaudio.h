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
#include "client-file.h"

struct Services {
	std::vector<std::string> songs;     // "filename1.wav"
	std::vector<std::string> channels;  // "name ip:port"
	bool microphone;                    // true if microphone is available
	Services () : microphone(false) {};
};

void ParseServicesList(std::string list, Services& s);
std::string ListServices(const Services& s);
void printStruct(const Services& s);

#endif
