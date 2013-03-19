#include <iostream>
#include <cstdlib>
#include "CommAudio.h"

using namespace std;

void usage (char const *argv[]) {
	cout << argv[0] << " [server] [port]" << endl;
}

int comm_connect (char * host, int port = 1337) {
	return 0;
}

void myterminate () {
  std::cerr << "terminate handler called\n";
  abort();  // forces abnormal termination
}

int main(int argc, char const *argv[])
{
	std::set_terminate (myterminate);
	Services s;
	// Connect
	
	// Recv
	
	// Parse
	ParseServicesList("S song1.mp3\nS song2.mp3", s);
	
	// Display
	printStruct(s);
	return 0;
}