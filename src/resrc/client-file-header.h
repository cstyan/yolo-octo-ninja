#ifndef HEADER
#define HEADER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define BUFFSIZE 1024
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
	int  port;
} uData;

DWORD WINAPI UploadThread(LPVOID);
bool downloadFile(int s, std::string filename);
void uploadFile(int s);
bool Download(uData*, std::string);
bool SelectFile(uData*);
bool SaveFile(uData*);
// =========================================================== //

typedef struct _SOCKET_INFORMATION {
   OVERLAPPED Overlapped;
   SOCKET Socket;
   CHAR Buffer[BUFFSIZE];
   WSABUF DataBuf;
   DWORD BytesSEND;
   DWORD BytesRECV;
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

#endif
