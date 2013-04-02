/*
	This file contains the functions for uploading a file to the server,
	and downloading a file from the server. Only uploadFile and DownloadFile
	should be called anywhere outside this file. Thanks.
*/
#include "commaudio.h"
#include "client-file.h"

typedef struct temp 
{
	uData tempData;
	std::string file;
} tempor;
DWORD WINAPI DownloadThread(LPVOID lpParameter);

tempor t;
uData upload;
char filename[1024];
extern int sock;

#define MSG_WAITALL 0x8
using namespace std;

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: DownloadFile
--
-- DATE: Mar 18, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: bool DownloadFile(int s, string filename)
--
-- RETURNS: true if the file is successfully downloaded, false otherwise..
--
-- NOTES:
-- Begins the process of downloading a file to the server. 
-- If a filename is successfully given, the function to actually download the file is called.
----------------------------------------------------------------------------------------------------------------------*/
bool downloadFile(int s, std::string filename)
{
	uData DataDownload;

	memset(&DataDownload, 0, sizeof(DataDownload));
	
	DataDownload.port = s;
	strcpy(DataDownload.file, filename.c_str());
	
	t.file = filename.c_str();

	if (SaveFile(&DataDownload))
	{
		t.tempData = DataDownload;
		CreateThread(NULL, 0, DownloadThread, (LPVOID)&t, 0, NULL);
	}
	else	// display "Error downloading song: <filename>" in status bar and return false
	{
		strcpy_s(displayCurrent, "ERROR downloading song: ");
		strcat_s(displayCurrent, filename.c_str());	
		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text
		return false;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: uploadFile
--
-- DATE: Feb 27, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: void uploadFile(int s)
--
-- RETURNS: void.
--
-- NOTES:
-- Begins the process of sending a file to the server. 
-- If a file is successfully opened, a thread is spawned to send the file
----------------------------------------------------------------------------------------------------------------------*/
void uploadFile(int s)
{
	memset(&upload, 0, sizeof(upload));
	
	upload.port = s;
	
	if (SelectFile(&upload))
		CreateThread(NULL, 0, UploadThread, (LPVOID)&upload, 0, NULL);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: SaveFile
--
-- DATE: Feb 27, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: bool SaveFile(uData* Download)
--
-- RETURNS: true if a file is selected, false otherwise
--
-- NOTES:
-- Save a file using a dialog box.
----------------------------------------------------------------------------------------------------------------------*/
bool SaveFile(uData* Download)
{
	OPENFILENAME  ofn;

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = Download->file;
    ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = "*.wav";
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if(GetSaveFileName(&ofn))
    {
        return true;
    }
	return false;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: DownloadThread
--
-- DATE: Mar 26, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: DWORD WINAPI DownloadThread(LPVOID lpParameter)
--
-- RETURNS: DWORD.
--
-- NOTES:
-- The thread that begins the downloading process.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI DownloadThread(LPVOID lpParameter)
{
	tempor* tt = (tempor*) lpParameter;
	std::string filename = tt->file;
	if (Download(&tt->tempData, filename)){
		// display "Downloading song: <filename> - COMPLETE" in status bar
		strcpy_s(displayCurrent, "Downloading song: ");
		strcat_s(displayCurrent, filename.c_str());	
		strcat_s(displayCurrent, " - COMPLETE");	
		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text
		return true;
	}
	// display "ERROR downloading song: <filename>" in status bar
	strcpy_s(displayCurrent, "ERROR downloading song: ");
	strcat_s(displayCurrent, filename.c_str());	
	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text
	return false;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: Download
--
-- DATE: Feb 27, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: bool DownloadThread(uData* Download, std::string filename)
--
-- RETURNS: true if function succeeded, false otherwise.
--
-- NOTES:
-- The function that copies the data sent from the server into a file.
----------------------------------------------------------------------------------------------------------------------*/
bool Download(uData* Download, std::string filename)
{
	int port, err;
	SOCKET sd;
	struct hostent	*hp;
	struct sockaddr_in server_addr;
	char sbuf[BUFSIZE];
	WSADATA WSAData;
	WORD wVersionRequested;
	FILE * hFile;
	LPSOCKET_INFORMATION SI;
	
	port = Download->port;

	wVersionRequested = MAKEWORD(2,2);
	err = WSAStartup(wVersionRequested, &WSAData);
	if (err != 0) //No usable DLL
	{
		err = WSAGetLastError();
		return false;
	}

	// Create the socket
	if ((sd = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == -1)
	{
		err = WSAGetLastError();
		return false;
	}

	// Initialize and set up the address structure
	memset((char *)&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	struct linger so_linger;
	so_linger.l_onoff = TRUE;
	so_linger.l_linger = 2000;
	

	// ------------------------------------------------------------------------------------------
	// NOTE: I AM HARDCODING THE IP. THIS HAS TO CHANGE TO THE CLIENT ADDRESS
	// ------------------------------------------------------------------------------------------
	if ((hp = gethostbyname(server)) == NULL)
	{
		err = WSAGetLastError();
		return false;
	}

	// Copy the server address
	memcpy((char *)&server_addr.sin_addr, hp->h_addr, hp->h_length);

	// Connecting to the server
	if (connect (sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		err = WSAGetLastError();
		return false;
	}

	setsockopt(sd, SOL_SOCKET, SO_LINGER, (const char*) &so_linger, sizeof(so_linger));

	if ((SI = (LPSOCKET_INFORMATION) GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
    {
         err = GetLastError();
         return false;
    } 

	SI->Socket = sd;

    ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));  
	SI->Overlapped.hEvent = WSACreateEvent();

	SI->BytesSEND = 0;
    SI->BytesRECV = 0;

	SI->DataBuf.buf = sbuf;
	/* sending the download message to the server */
	int ret = sprintf(sbuf, "D %s\n", filename.c_str());
	SI->DataBuf.len = ret;
	WSASend(SI->Socket, &SI->DataBuf, 1, NULL, 0, NULL, NULL);

	DWORD flag = 0;
	DWORD error1 = 1, error2 = 0;

	printf("Opening: %s\n", Download->file);
	hFile = fopen(Download->file, "wb");
	if (hFile == NULL) {
		printf("Could not fopen\n");
	}
	memset(sbuf, 0, sizeof(sbuf));

	ret = 0;
	while (error1 > 0)
	{
		SI->DataBuf.len = BUFSIZE;
		error1 = WSARecv(SI->Socket, &SI->DataBuf, 1, NULL, &flag, &(SI->Overlapped), NULL);

		if ((int)error1 == SOCKET_ERROR && (error2 = WSAGetLastError()) != WSA_IO_PENDING)
			break;

		ret = WSAWaitForMultipleEvents(1, &SI->Overlapped.hEvent, TRUE, 500, TRUE);
		if (ret == WSA_WAIT_TIMEOUT || ret == WSA_WAIT_IO_COMPLETION)
			break;

		WSAGetOverlappedResult(SI->Socket, &SI->Overlapped, &error1, FALSE, &flag);
		fwrite(SI->DataBuf.buf, 1, error1, hFile);
		WSAResetEvent(SI->Overlapped.hEvent);
	}

	fclose(hFile);
	closesocket(sd);
	WSACleanup();
	return true;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: SelectFile
--
-- DATE: Feb 27, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: bool SelectFile(uData* upload)
--
-- RETURNS: true if a file is selected, false otherwise.
--
-- NOTES:
-- Opens a file using a dialog box.
----------------------------------------------------------------------------------------------------------------------*/
bool SelectFile(uData* upload)
{
	OPENFILENAME  ofn;

    memset(upload->file, 0, strlen(upload->file));
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = upload->file;
	ofn.lpstrFileTitle = filename;
	ofn.nMaxFileTitle = MAX_PATH;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if(!GetOpenFileName(&ofn))
    {
        return false;
    }

	// display "Uploading song: <filename>" in status bar
	strcpy_s(displayCurrent, "Uploading song: ");
	strcat_s(displayCurrent, filename);
	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text
	return true;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: UploadThread
--
-- DATE: Feb 27, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: DWORD WINAPI UploadThread(LPVOID lpParameter)
--
-- RETURNS: DWORD.
--
-- NOTES:
-- The thread that reads the file specified and sends it to the server using WSASend
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI UploadThread(LPVOID lpParameter)
{
	int port, err;
	SOCKET sd;
	struct hostent	*hp;
	struct sockaddr_in server_addr;
	char sbuf[BUFSIZE];
	WSADATA WSAData;
	WORD wVersionRequested;
	FILE * fp;
	LPSOCKET_INFORMATION SI;
	
	uData* upload = (uData*) lpParameter;
	port = upload->port;

	wVersionRequested = MAKEWORD(2,2);
	err = WSAStartup(wVersionRequested, &WSAData);
	if (err != 0) //No usable DLL
	{
		err = WSAGetLastError();
		return FALSE;
	}

	// Create the socket
	if ((sd = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == -1)
	{
		err = WSAGetLastError();
		return FALSE;
	}

	// Initialize and set up the address structure
	memset((char *)&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	struct linger so_linger;
	so_linger.l_onoff = TRUE;
	so_linger.l_linger = 2000;


	// ------------------------------------------------------------------------------------------
	// NOTE: I AM HARDCODING THE IP. THIS HAS TO CHANGE TO THE SERVER ADDRESS - done! is now "server"
	// ------------------------------------------------------------------------------------------
	if ((hp = gethostbyname(server)) == NULL)
	{
		err = WSAGetLastError();
		return FALSE;
	}

	// Copy the server address
	memcpy((char *)&server_addr.sin_addr, hp->h_addr, hp->h_length);

	// Connecting to the client
	if (connect (sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		err = WSAGetLastError();
		return FALSE;
	}

	memset((char *)sbuf, 0, sizeof(sbuf));
	strcpy(sbuf, upload->file);

	fp = fopen(upload->file, "rb");

	setsockopt(sd, SOL_SOCKET, SO_LINGER, (const char*) &so_linger, sizeof(so_linger));

	if ((SI = (LPSOCKET_INFORMATION) GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
    {
         err = GetLastError();
         return FALSE;
    } 

	SI->Socket = sd;
    ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));  
    SI->BytesSEND = 0;
    SI->BytesRECV = 0;

	SI->DataBuf.buf = sbuf;
	int ret = sprintf(sbuf, "U %s\n", filename);
	SI->DataBuf.len = ret;
	WSASend(SI->Socket, &SI->DataBuf, 1, NULL, 0, NULL, NULL);
	
	char reply_buffer[9] = {0};
	std::string go_reply("go-ahead");
	recv(SI->Socket, reply_buffer, 8, MSG_WAITALL);
	
	if (reply_buffer != go_reply)
		cout << "Didn't get the go-ahead!" << endl;

	fseek(fp, 0, SEEK_END); // seek to end of file
	size_t total_size = ftell(fp); // get current file pointer
	fseek(fp, 0, SEEK_SET); // seek back to beginning of file

	set_progress_bar(0);
	set_progress_bar_range(total_size);

	memset(sbuf, 0, sizeof(sbuf));
	ret = 0;
	while ((ret = fread(sbuf, 1, BUFSIZE, fp)) > 0)
	{
		SI->DataBuf.len = ret;
		increment_progress_bar(ret);
		
		WSASend(SI->Socket, &SI->DataBuf, 1, NULL, 0, NULL, NULL);
		memset(sbuf, 0, sizeof(sbuf));
	}

	fclose(fp);
	closesocket (sd);

	// display "Uploading song: <filename> - COMPLETE" in status bar
	strcpy_s(displayCurrent, "Uploading song: ");
	strcat_s(displayCurrent, filename);
	strcat_s(displayCurrent, " - COMPLETE");
	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text

	// After the socket is closed, the file is flushed.
	get_and_display_services(sock);

	WSACleanup();
	return 1;
}