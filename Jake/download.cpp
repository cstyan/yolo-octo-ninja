#include "resource1.h"
#include "header.h"
#include <Commdlg.h>
/*typedef struct UDATA
{
	char file[BUFFSIZE];
	int  port;
	
} uData;*/

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: DownloadFile
--
-- DATE: Feb 27, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: void DownloadFile(int s)
--
-- RETURNS: void.
--
-- NOTES:
-- Begins the process of sending a file to the server. 
-- If a file is successfully opened, a thread is spawned to send the file
----------------------------------------------------------------------------------------------------------------------*/
void downloadFile(int s)
{
	HANDLE FileThread;
	uData Download;

	memset(&Download, 0, sizeof(Download));
	
	Download.port = s;
	
	if (SaveFile(&Download))
		FileThread = CreateThread(NULL, 0, DownloadThread, (LPVOID)&Download, 0, NULL);
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
-- INTERFACE: bool SelectFile(uData* Download)
--
-- RETURNS: bool.
--
-- NOTES:
-- Save a file using a dialog box.
----------------------------------------------------------------------------------------------------------------------*/
bool SaveFile(uData* Download)
{
	OPENFILENAME  ofn;

    memset(Download->file, 0, strlen(Download->file));
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = Download->file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if(!GetOpenFileName(&ofn))
    {
        return false;
    }
	return true;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: DownloadThread
--
-- DATE: Feb 27, 2013
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
-- The thread that reads the file specified and sends it to the server using WSASend
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI DownloadThread(LPVOID lpParameter)
{
	HANDLE hFile;
	int port, err;
	SOCKET sd;
	struct hostent	*hp;
	struct sockaddr_in server;
	char sbuf[DATA_BUFSIZE];
	WSADATA WSAData;
	WORD wVersionRequested;
	FILE * fp;
	LPSOCKET_INFORMATION SI;
	DWORD BytesWritten;
	
	uData* Download = (uData*) lpParameter;
	port = Download->port;

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
	memset((char *)&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	struct linger so_linger;
	so_linger.l_onoff = TRUE;
	so_linger.l_linger = 2000;


	// ------------------------------------------------------------------------------------------
	// NOTE: I AM HARDCODING THE IP. THIS HAS TO CHANGE TO THE CLIENT ADDRESS
	// ------------------------------------------------------------------------------------------
	if ((hp = gethostbyname("192.168.0.23")) == NULL)
	{
		err = WSAGetLastError();
		return FALSE;
	}

	// Copy the server address
	memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);

	// Connecting to the server
	if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		err = WSAGetLastError();
		return FALSE;
	}

	memset((char *)sbuf, 0, sizeof(sbuf));
	strcpy_s(sbuf, Download->file);

	fopen_s(&fp, Download->file, "rb");

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
    SI->DataBuf.len = BUFFSIZE;
	sprintf(SI->Buffer, "D %s\r\n", Download->file);
	WSASend(SI->Socket, &SI->DataBuf, 1, NULL, 0, &(SI->Overlapped), NULL);

	SI->Socket = sd;
    ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));  
    SI->BytesSEND = 0;
    SI->BytesRECV = 0;
    SI->DataBuf.len = BUFFSIZE;

	hFile = CreateFile(Download->file, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	while (WSARecv(SI->Socket, &SI->DataBuf, 1, NULL, 0, &SI->Overlapped, NULL) != SOCKET_ERROR && WSAGetLastError() == WSA_IO_PENDING)
	{
		WriteFile(hFile, SI->DataBuf.buf, strlen(SI->DataBuf.buf), &BytesWritten, NULL);
	}

	fclose(fp);
	closesocket (sd);
	WSACleanup();
	return 1;
}
