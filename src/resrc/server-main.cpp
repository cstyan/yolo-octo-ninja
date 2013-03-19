// Assignment 2 Server Main
// server-main.cpp : Defines the entry point and event handlers for the application.
//
#include <cstdio>
#include <string>
#include <fstream>
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

ServerParameters g_current_params = {0};
HINSTANCE hInst;	// current instance
extern SOCKET g_SSock;
bool listening = false;

int APIENTRY _tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HWND hwnd;
	WSADATA wsaData;
	HACCEL hAccelTable;
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(hPrevInstance);
  WORD wVersionRequested = MAKEWORD(2,2);
	
	g_current_params.tcp = true;

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
      //IsDialogMessage(hwnd, &msg)) // Slow!
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
      ofn.lpstrFilter = "All Files\0*.*\0\0";
      ofn.lpstrFileTitle = NULL;
      ofn.nMaxFileTitle  = 0;
      ofn.lpstrInitialDir= NULL;
      ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
      if (GetSaveFileName(&ofn)) {
				g_current_params.file = new ofstream(szFile, ofstream::binary);
				if (*g_current_params.file) {
					printf("%s Opened\n", szFile);
					string file("");
					file += szFile;
					SendMessage(GetDlgItem(hWnd, IDC_TXT_PTYPE), WM_SETTEXT, 0, (LPARAM)file.c_str());
					strcpy(g_current_params.dest_file, szFile);
        }
      }
      break;
      }
    case IDC_BTN_PROC: {
      SendMessage(GetDlgItem(hWnd, IDC_TXT_PTYPE), WM_SETTEXT, 0, (LPARAM)"Data Discarded");
      break;
    }
    case IDC_BTN_GO: {
			if (!listening) {
				EnableWindow(GetDlgItem(hWnd, IDC_BTN_FILE ), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_BTN_PROC ), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_EDT_PMULT), FALSE);
				SendMessage (GetDlgItem(hWnd, IDC_BTN_GO), WM_SETTEXT, 0, (LPARAM) "Stop");
				SetStatus(hWnd, "Listening...");
				/* Connection code */
				ServerParameters *p = new ServerParameters(g_current_params);
				/* Weird way of getting the multiple/size values into p */
				char mult_text[BUFSIZE] = {0};
				GetDlgItemText(hWnd, IDC_EDT_PMULT, mult_text, BUFSIZE);
				{stringstream ss(mult_text); ss >> p->port;}
				CreateThread(NULL, 0, ServerProc, p, 0, NULL);
      } else {
				SetStatus(hWnd,  "Stopping server...");
				closesocket(g_SSock);
				EnableWindow(GetDlgItem(hWnd, IDC_BTN_FILE ), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_BTN_PROC ), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_EDT_PMULT), TRUE);
				SendMessage (GetDlgItem(hWnd, IDC_BTN_GO), WM_SETTEXT, 0, (LPARAM) "Listen...");
      }
      listening = !listening;
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