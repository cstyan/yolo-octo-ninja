#ifndef SERVERFILE
#define SERVERFILE

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#pragma comment(lib, "Ws2_32.lib")

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <stdio.h>

// ==== NOTE: Only these should be needed in final build! ==== //
// =========================================================== //
#include <string>
#include <Commdlg.h>

typedef struct UDATA
{
	char file[1024];
	SOCKET socket;
} uData;

DWORD WINAPI UploadThread(LPVOID);
bool sDownloadFile(SOCKET, std::string);
void sUploadFile(SOCKET, std::string);
bool Download(SOCKET, char*);
// =========================================================== //

typedef struct _SOCKET_INFORMATION {
   OVERLAPPED Overlapped;
   SOCKET Socket;
   CHAR Buffer[BUFSIZE];
   WSABUF DataBuf;
   DWORD BytesSEND;
   DWORD BytesRECV;
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

#endif
