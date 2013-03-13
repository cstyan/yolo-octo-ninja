#include "resource1.h"
#include "header.h"
#include <Commdlg.h>
/*typedef struct UDATA
{
	char file[BUFFSIZE];
	int  port;
	
} uData;*/

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
	HANDLE FileThread;
	uData upload;

	memset(&upload, 0, sizeof(upload));
	
	upload.port = s;
	
	if (SelectFile(&upload))
		FileThread = CreateThread(NULL, 0, UploadThread, (LPVOID)&upload, 0, NULL);
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
-- RETURNS: bool.
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
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if(!GetOpenFileName(&ofn))
    {
        return false;
    }
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
	struct sockaddr_in server;
	char sbuf[DATA_BUFSIZE];
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
	memset((char *)&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	struct linger so_linger;
	so_linger.l_onoff = TRUE;
	so_linger.l_linger = 2000;


	// ------------------------------------------------------------------------------------------
	// NOTE: I AM HARDCODING THE IP. THIS HAS TO CHANGE TO THE SERVER ADDRESS
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
	strcpy_s(sbuf, upload->file);

	fopen_s(&fp, upload->file, "rb");

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
	sprintf(SI->Buffer, "U %s\r\n", upload->file);
	WSASend(SI->Socket, &SI->DataBuf, 1, NULL, 0, &(SI->Overlapped), NULL);

	SI->Socket = sd;
    ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));  
    SI->BytesSEND = 0;
    SI->BytesRECV = 0;
    SI->DataBuf.len = BUFFSIZE;

	while (fread(SI->Buffer, 1, BUFFSIZE, fp) > 0)
	{
		SI->DataBuf.buf = SI->Buffer;
		WSASend(SI->Socket, &SI->DataBuf, 1, NULL, 0, &(SI->Overlapped), NULL);
		memset(SI->Buffer, 0, sizeof(SI->Buffer));
	}

	fclose(fp);
	closesocket (sd);
	WSACleanup();
	return 1;
}
