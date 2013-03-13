#ifndef HEADER
#define HEADER

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cctype>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

typedef struct CONFIGS 
{
	DWORD  packetSize;
	DWORD  protocol;
	DWORD  multiplier;
} config;

typedef struct UDATA
{
	char file[1024];
	int  port;
	
} uData;

// function prototypes //
void menuProc(UINT, WPARAM, CONFIGS*);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CALLBACK WorkerRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void CALLBACK UDPWorkerRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
DWORD WINAPI ServerThread(LPVOID);
DWORD WINAPI UDPServerThread(LPVOID lpParameter);
DWORD WINAPI WorkerThread(LPVOID);
DWORD WINAPI UploadThread(LPVOID);
DWORD WINAPI DownloadThread(LPVOID);
DWORD WINAPI TCPTestThread(LPVOID);
DWORD WINAPI UDPTestThread(LPVOID);
void setProtocol(LPARAM, CONFIGS*);
void setSize(LPARAM, CONFIGS*);
void setMultiplier(LPARAM, CONFIGS*);
void server(HWND, CONFIGS*);
bool SelectFile(uData*);
bool SaveFile(uData*);
void uploadFile(int s);
void test(HWND, CONFIGS*);
void exit();
void init();

typedef struct _SOCKET_INFORMATION {
   OVERLAPPED Overlapped;
   SOCKET Socket;
   CHAR Buffer[DATA_BUFSIZE];
   WSABUF DataBuf;
   DWORD BytesSEND;
   DWORD BytesRECV;
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

// Global variables
static HWND   hWnd;
static HWND   hDisplay;
static RECT   rc;
static SOCKET AcceptSocket;

static TCHAR szWindowClass[] = TEXT("Assign1");
static TCHAR szTitle[] = TEXT("Ghostly Abomination");
#endif
