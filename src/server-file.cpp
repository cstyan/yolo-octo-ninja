/*
	This file contains the functions for uploading a file to the client,
	and downloading a file from the client. Only sUploadFile and sDownloadFile
	should be called anywhere outside this file. Thanks.
*/
#include "commaudio.h"
#include "server-file.h"

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: sDownloadFile
--
-- DATE: Mar 21, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: bool sDownloadFile(SOCKET s, string filename)
--
-- RETURNS: true if the file is successfully downloaded, false otherwise.
--
-- NOTES:
-- Begins the process of downloading a file to the server. 
-- If a filename is successfully given, the function that actually downloads the file is called.
----------------------------------------------------------------------------------------------------------------------*/
bool sDownloadFile(SOCKET s, std::string filename)
{
	char file[BUFSIZE];
	strcpy(file, filename.c_str());
	if (Download(s, file))
		return true;

	return false;		
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: sUploadFile
--
-- DATE: Mar 21, 2013
--
-- DESIGNER:  Jacob Miner
--
-- PROGRAMMER:  Jacob Miner
--
-- INTERFACE: void sUploadFile(SOCKET s, string filename)
--
-- RETURNS: void.
--
-- NOTES:
-- Begins the process of sending a file to the client. 
-- If a file is successfully opened, a thread is spawned to send the file
----------------------------------------------------------------------------------------------------------------------*/
void sUploadFile(SOCKET s, std::string filename)
{
	HANDLE FileThread;
	uData upload;

	memset(&upload, 0, sizeof(upload));
	
	upload.socket = s;
	strcpy(upload.file, filename.c_str());
	
	FileThread = CreateThread(NULL, 0, UploadThread, (LPVOID)&upload, 0, NULL);
	WaitForSingleObject(FileThread, INFINITE);
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
-- INTERFACE: bool Download(SOCKET, char * filename)
--
-- RETURNS: true if function succeeded, false otherwise.
--
-- NOTES:
-- The function that copies the data sent from the client into a file.
----------------------------------------------------------------------------------------------------------------------*/
bool Download(SOCKET sd, char* filename)
{
	HANDLE hFile;
	char sbuf[BUFSIZE];
	DWORD BytesWritten  = 0;
	DWORD bytesRead		= 1;
	DWORD bufferLength  = 0;
	DWORD flag			= 0;
	int error;
	int ret;
	LPSOCKET_INFORMATION SI;

	if ((SI = (LPSOCKET_INFORMATION) GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
    {
         error = GetLastError();
         return false;
    } 

	SI->Socket = sd;

    ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));  
	SI->Overlapped.hEvent = WSACreateEvent();

	SI->BytesSEND = 0;
    SI->BytesRECV = 0;

	SI->DataBuf.buf = sbuf;

	hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	while (bytesRead > 0)
	{
		SI->DataBuf.len = BUFSIZE;
		bufferLength = WSARecv(sd, &SI->DataBuf, 1, NULL, &flag, &(SI->Overlapped), NULL);

		if (bufferLength == SOCKET_ERROR && (error = WSAGetLastError()) != WSA_IO_PENDING)
			break;

		ret = WSAWaitForMultipleEvents(1, &SI->Overlapped.hEvent, TRUE, INFINITE, TRUE);
		if (ret == WSA_WAIT_TIMEOUT || ret == WSA_WAIT_IO_COMPLETION)
			break;

		WSAGetOverlappedResult(SI->Socket, &SI->Overlapped, &bufferLength, FALSE, &flag);
		error = WriteFile(hFile, SI->DataBuf.buf, bufferLength, &BytesWritten, NULL);
		WSAResetEvent(SI->Overlapped.hEvent);
	}

	CloseHandle(hFile);
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
-- The thread that reads the file specified and sends it to the client
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI UploadThread(LPVOID lpParameter)
{
	int err;
	SOCKET sd;
	char sbuf[BUFSIZE];
	FILE * fp;
	LPSOCKET_INFORMATION SI;
	
	uData* upload = (uData*) lpParameter;
	sd = upload->socket;
	memset((char *)sbuf, 0, sizeof(sbuf));

	fp = fopen(upload->file, "rb");

	if ((SI = (LPSOCKET_INFORMATION) GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
    {
         err = GetLastError();
         return FALSE;
    } 

	SI->Socket = sd;
    ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));  
    SI->BytesSEND = 0;
    SI->BytesRECV = 0;
    SI->DataBuf.len = BUFSIZE;

	while (fread(SI->Buffer, 1, BUFSIZE, fp) > 0)
	{
		SI->DataBuf.buf = SI->Buffer;
		WSASend(SI->Socket, &SI->DataBuf, 1, NULL, 0, &(SI->Overlapped), NULL);
		memset(SI->Buffer, 0, sizeof(SI->Buffer));
	}

	fclose(fp);
	return 1;
}