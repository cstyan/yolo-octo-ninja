#include <iostream>
#include "CommAudio.h"

using namespace std;

void usage (char const *argv[]) {
	cout << argv[0] << " [server] [port]" << endl;
}

int comm_connect () {
	return 0;
}

int main(int argc, char const *argv[])
{
	Services s;
	
	// Connect
	
	// Recv
	
	// Parse
	ParseServicesList("S song1.mp3\nS song2.mp3\nC test\nM false\n", s);
	
	// Display
	printStruct(s);
	return 0;
}