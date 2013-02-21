#define BUFSIZE	255
#define HOSTSIZE	1024

#ifndef APSTUDIO_READONLY_SYMBOLS
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>


// Parameters contains all the context needed to initate client or server.
typedef struct {
  bool tcp;
  u_short port;
  size_t size;
  size_t multiple;
  bool source_procedural; 
  char source_file[MAX_PATH];
  char dest_host[HOSTSIZE];

  DWORD startticks;
  size_t total_bytes_sent;
  struct sockaddr_in server;
  int socket;
  int control_socket;
  std::ifstream * file;
  char * buffer;
  size_t offset;
} Parameters;

typedef struct {
	/* Configuration */
  bool tcp;
  bool alive;
  DWORD startticks;
  size_t total_bytes_sent;
  /* Addressing and socket */
  int socket;
  u_short port;
  struct sockaddr_in addr;
  int addr_size;
  char dest_file[MAX_PATH];
  int control_socket; // Control socket for UDP tests.
	/* Buffers and IO */
  char * buffer;
  char * control_buffer;
  size_t size;
  size_t offset;
  std::ofstream * file;
  bool already_recving;
} ServerParameters;

typedef struct {
	size_t time;
	size_t bytes;
	char end;
} stats_t;

/* Server Net */
DWORD WINAPI ServerProc (LPVOID lpParameter);

/* Client Net */
DWORD WINAPI ClientProc (LPVOID lpParameter);

/* Common GUI prototypes */
extern HWND g_Hwnd;
void create_gui (HWND hWnd);
void SetStatus (HWND hWnd, const char * status);
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int, HWND&);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
#endif
