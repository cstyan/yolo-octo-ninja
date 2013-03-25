/*---------------------------------------------------------------------------------------------
--	SOURCE FILE: ComAudioServices.cpp
--
--	PROGRAM:	Provides List of Services for ComAudio Project for COMP4985
--
--	FUNCTIONS USEFUL TO PROJECT:	
--				void ParceServicesList(string list, Services& s)
--				string ListServices(const Services& s)
--				void printStruct(const Services& s)
--					
--	DATE:		14/Mar/2013
--	REVISION:	2.00	
--
--	DESIGNER:	Kevin Tangeman
--	PROGRAMMER:	Kevin Tangeman
--
--	NOTES:		This program takes the server's struct full of client options for songs and channels,
--				converts it to a string to send to the client, then reads the string and parses it,
--				and puts it into the client's struct.
------------------------------------------------------------------------------------------------*/
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include "CommAudio.h"

using namespace std;

/*---------------------------------------------------------------------------------------
-- 	FUNCTION: ParseServicesList()
--
-- 	DATE: 	5/Mar/2013
-- 	REVISIONS: 
--			18/Mar/2013 - David Czech - improved efficiency with stringstreams
--
-- 	DESIGNER: Kevin Tangeman
-- 	PROGRAMMER: Kevin Tangeman
--
-- 	INTERFACE: void ParceServicesList(string list, Services& s)
-- 	RETURNS: void
--
-- 	NOTES:	This function reads in the string sent from the server and updates the
--			Services struct on the client.
----------------------------------------------------------------------------------------*/
void ParseServicesList(string list, Services& s)
{
	stringstream ss(list);
	string token;
	while (ss >> token) {
		if (token == "S") {
			string song;
			if (getline(ss, song))
				s.songs.push_back(song);
		} else if (token == "C") {
			string channel;
			if (getline(ss, channel))
				s.channels.push_back(channel);
		} else if (token == "M") {
			string mic;
			if (ss >> mic)
				s.microphone = (mic == "true" ? true : false);
		}
	}
}

/*---------------------------------------------------------------------------------------
-- 	FUNCTION: ListServices()
--
-- 	DATE: 5/Mar/2013
-- 	REVISIONS: 
--
-- 	DESIGNER: Kevin Tangeman
-- 	PROGRAMMER: Kevin Tangeman
--
-- 	INTERFACE: string ListServices(const Services& s)
-- 	RETURNS: string - listing all services currently available to the client
--
-- 	NOTES:	This function generates a string from the Services struct to send to the 
-- 			client.
--			The string will look like "S song1.wav\nS song2.wav\nC channel1 ipaddress:port\nC 
--			channel2 ipaddress:port\nM true/false"
----------------------------------------------------------------------------------------*/
string ListServices(const Services& s) {
	vector<string>::const_iterator it;
	string serv =  "";
	
	for(it = s.songs.begin(); it != s.songs.end();  ++it)
		serv += "S " + *it + "\n";

	for(it = s.channels.begin(); it != s.channels.end();  ++it)
		serv += "C " + *it + "\n";

	serv += (s.microphone ? "M true" : "M false");
	serv += "\n";

	return serv;
}


/*---------------------------------------------------------------------------------------
-- 	FUNCTION: printStruct()
--
-- 	DATE: 14/Mar/2013
-- 	REVISIONS: 2.00
--
-- 	DESIGNER: Kevin Tangeman
-- 	PROGRAMMER: Kevin Tangeman
--
-- 	INTERFACE: void printStruct(const Services& s)
-- 	RETURNS: void
--
-- 	NOTES:	This function prints out the contents of the Services struct.
----------------------------------------------------------------------------------------*/
void printStruct(const Services& s) {
	vector<string>::const_iterator it;

	cout << endl << "Songs in List:" << endl;
	for(it = s.songs.begin(); it != s.songs.end();  ++it)
		cout << *it << endl;

	cout << endl << "Channels in List:" << endl;
	for(it = s.channels.begin(); it != s.channels.end();  ++it)
		cout << *it << endl;

	if(s.microphone)
		cout << endl << "Microphone Available" << endl;
	else
		cout << endl << "Microphone NOT Available" << endl;

	cout << "------------------------------------------------" << endl;
}

#ifdef _SERVICE_TEST
Services sData, cData;	    // Services structs for server (sData) and client (cData)
/*---------------------------------------------------------------------------------------------
--	FUNCTION:	main
--
--	DATE:		14/Mar/2013
--	REVISION:	1.00	
--
--	DESIGNER:	Kevin Tangeman
--	PROGRAMMER:	Kevin Tangeman
--
--	INTERFACE:	
--
--	NOTES:		Main executing function 
------------------------------------------------------------------------------------------------*/
int main(){
	string text;

	//sData.microphone = true;			// microphone available
	sData.microphone = false;			// microphone NOT available

	sData.songs.push_back("Disco Dancing");		// copy songs to the sData songs vector
	sData.songs.push_back("Disco Duck");
	sData.songs.push_back("Disco David Dancing in a Truck");

	sData.channels.push_back("Disco Channel 192.168.0.23:5050");		// copy "name ip:port" to the sData channels vector
	sData.channels.push_back("CNN 192.168.0.24:500");
	sData.channels.push_back("Rastafarian 192.168.0.13:4550");

//	1) displaying the server Services struct sData
	
	cout << endl << "Server Data Struct" << endl;
	cout << "******************" << endl;
	printStruct(sData);

//	2) calling ListServices() to generate a string from the Services struct sData
	text = ListServices(sData);

//	3) displaying the string
	cout << endl << "String to Send to Client" << endl;
	cout << "************************" << endl;
	cout << text << endl;
	cout << "------------------------------------------------" << endl;

//	4) calling ParseServicesList() to read in the string and update the client struct cData
	ParseServicesList(text, cData);

//	5) displaying the server Services struct cData
	cout << endl <<"Client Data Struct" << endl;
	cout << "******************" << endl;
	printStruct(cData);


}

//// Utilities
// Function parse_ip_port - parse ip and port fields from a string.
// Interface: bool parse_ip_port (string& s, string& ip, unsigned short& port)
//   s - reference to a string containing the "ip:port" to parse.
//   ip - output of the resulting IP string.
//   port - output of the resulting port.
// Returns: boolean - true if parsing was successful, false otherwise.
bool parse_ip_port (string& s, string& ip, unsigned short& port) {
  stringstream ss( s );
  if (ss.eof() || !getline( ss, ip, ':' )) 
    return false;
  // This can't be negative, but must allow all unsigned short values.
  // desired behaviour: "-1" -> fail, "64000" -> 64000
  // get string, check for negative sign and fail if there is one.
  if (ss.eof() || !(ss >> port))
      return false;
  // Now we should be at end of input, if we're not, then fail.
  return (ss.eof());
}

#endif