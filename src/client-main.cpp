// Assignment 2 Client Main
// client-main.cpp : Defines the entry point and event handlers for the application.
//
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>

#include <tchar.h>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "a2.h"
#include "resource.h"

using namespace std;

Parameters g_current_params = {0};
HINSTANCE hInst;	// current instance

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

int APIENTRY _tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
  WORD wVersionRequested = MAKEWORD(2,2);   
	WSADATA wsaData;
	MSG msg;
	HWND hwnd;
	HACCEL hAccelTable;
	
	g_current_params.tcp = true;
	g_current_params.source_procedural = true;
	
	// Init Windows Visual Styles
	//InitCommonControls();
	// Open up a Winsock v2.2 session
	WSAStartup(wVersionRequested, &wsaData);
	
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow, hwnd)) {
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DCWIN1));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0) )
	{
	// 
    //if(!IsDialogMessage(hwnd, &msg)) // Slow!
      if (!TranslateAccelerator(hwnd, hAccelTable, &msg) && !IsDialogMessage(hwnd, &msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
	}
  
  WSACleanup();
  
	return (int) msg.wParam;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_CREATE:
		create_gui ( hWnd );
		SetStatus(hWnd, "Ready...");
    break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
    UNREFERENCED_PARAMETER(wmEvent);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDC_BTN_FILE: {
      char szFile[MAX_PATH] = {0};
      OPENFILENAME ofn = {0};
      ofn.lStructSize  = sizeof ( ofn );
      ofn.hwndOwner = hWnd;
      ofn.lpstrFile = szFile;
      ofn.nMaxFile  = sizeof( szFile );
      ofn.nFilterIndex   = 1;
      ofn.lpstrFileTitle = NULL;
      ofn.nMaxFileTitle  = 0;
      ofn.lpstrInitialDir= NULL;
      ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
      if (GetOpenFileName(&ofn)) {
        printf("opening %s\n", szFile);
        string file("");
        file += szFile;
        SendMessage(GetDlgItem(hWnd, IDC_TXT_PTYPE), WM_SETTEXT, 0, (LPARAM)file.c_str());
        g_current_params.source_procedural = false;
        strcpy(g_current_params.source_file, szFile);
        EnableWindow(GetDlgItem(hWnd, IDC_EDT_PMULT), FALSE);
      }
      break;
      }
    case IDC_BTN_PROC: {
      g_current_params.source_procedural = true;
      SendMessage(GetDlgItem(hWnd, IDC_TXT_PTYPE), WM_SETTEXT, 0, (LPARAM)"Procedural");
      EnableWindow(GetDlgItem(hWnd, IDC_EDT_PMULT), TRUE);
      break;
    }
    case IDC_BTN_GO: {
      SetStatus(hWnd, "Connecting...");
      /* Connection code */
      Parameters *p = new Parameters(g_current_params);
      GetDlgItemText(hWnd, IDC_EDT_DEST, p->dest_host, sizeof(p->dest_host));
      
      /* Weird way of getting the multiple/size values into p */
      char mult_text[BUFSIZE] = {0};
      char size_text[BUFSIZE] = {0};
      GetDlgItemText(hWnd, IDC_EDT_PMULT, mult_text, BUFSIZE);
      GetDlgItemText(hWnd, IDC_EDT_PSIZE, size_text, BUFSIZE);
      {stringstream ss(mult_text); ss >> p->multiple;}
      {stringstream ss(size_text); ss >> p->size;}
      
      string s (p->dest_host);
      string dest;
      unsigned short port;
      // If we can successfully parse the input to get ip and port
      if ( parse_ip_port(s, dest, port) ) {
        strncpy(p->dest_host, dest.c_str(), HOSTSIZE-1);
        p->port = port;
        // ClientProc will use parameters to connect, send, etc.
        CreateThread(NULL, 0, ClientProc, p, 0, NULL);
      }
      break;
    }
    case IDM_FILE_MODE_CLIENT_TCP:
      g_current_params.tcp = true;
	  CheckMenuItem(GetMenu(hWnd), IDM_FILE_MODE_CLIENT_TCP, MF_CHECKED);
	  CheckMenuItem(GetMenu(hWnd), IDM_FILE_MODE_CLIENT_UDP, MF_UNCHECKED);
      break;
    case IDM_FILE_MODE_CLIENT_UDP:
      g_current_params.tcp = false;
	  CheckMenuItem(GetMenu(hWnd), IDM_FILE_MODE_CLIENT_UDP, MF_CHECKED);
	  CheckMenuItem(GetMenu(hWnd), IDM_FILE_MODE_CLIENT_TCP, MF_UNCHECKED);
      break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		} 
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}