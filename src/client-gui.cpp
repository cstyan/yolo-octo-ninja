// Assignment 3 Client GUI
// client-gui.cpp : Defines the window GUI initialization and creation code.
//
//	PROGRAMMERS:
//		23-Mar-2013 - Kevin Tangeman - Created basic client GUI interface layout in Win32
//
#define _WIN32_IE 0x301
#include "CommAudio.h"
#include "resource.h"
#include "client-file.h"
#include <commctrl.h>
#include <string>
#include "libzplay.h"

using namespace std;

/* Easy styles enabler for msvc */
#ifdef _MSC_VER
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

DWORD WINAPI stream_song_proc(LPVOID lpParamter);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void get_and_display_services(int control);

int sock;
HWND g_Hwnd, slb, clb;
extern HINSTANCE hInst;	 // current instance from main.cpp

void create_gui (HWND hWnd) {
  HFONT hFont;
  //HWND heInput;
  // Create Input and Output Edit Text controls.
  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

  // Setup to create a progress bar for playback, download, upload status
  INITCOMMONCONTROLSEX InitCtrlEx;
  InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
  InitCtrlEx.dwICC  = ICC_PROGRESS_CLASS;
  InitCommonControlsEx(&InitCtrlEx);

 SendMessage(
    CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, "Progress",	// progress bar
	  WS_CHILD|WS_VISIBLE|PBS_SMOOTH, 
	  50, 300, 600, 20, hWnd, NULL, hInst, NULL)
	,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (							// Prev button for play control
    CreateWindow("BUTTON", "Prev",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      120, 265, 40, 20, hWnd, (HMENU)-1, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (							// Play button for play control
    CreateWindow("BUTTON", "Play",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      170, 260, 60, 30, hWnd, (HMENU)-1, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (							// Next button for play control
    CreateWindow("BUTTON", "Next",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      240, 265, 40, 20, hWnd, (HMENU)-1, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (							// Mic button for using microphone
    CreateWindow("BUTTON", "Chat",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      590, 260, 60, 30, hWnd, (HMENU)ID_VOICECHAT_CHATWITHSERVER, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (							// Download button for uploading files
    CreateWindow("BUTTON", "Dn",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      315, 260, 30, 30, hWnd, (HMENU)-1, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (							// Upload button for uploading files
    CreateWindow("BUTTON", "Up",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      50, 260, 30, 30, hWnd, (HMENU)-1, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
	  slb = CreateWindow("LISTBOX", "SongList",	// Songs can be listed and selected here
		  WS_CHILD|WS_VISIBLE, 
		  50, 40, 295, 210, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  clb = CreateWindow("LISTBOX", "ChannelList",	// Channels can be listed and selected here
      WS_CHILD|WS_VISIBLE, 
      355, 40, 295, 210, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("STATIC", "Song List",
      WS_CHILD|WS_VISIBLE|SS_CENTER, 
      50, 25, 295, 15, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("STATIC", "Channel List",
      WS_CHILD|WS_VISIBLE|SS_CENTER, 
      355, 25, 295, 15, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("BUTTON", "Stream",
      WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_GROUP, 
      470, 260, 60, 30, hWnd, (HMENU)IDC_BTN_STREAM, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  

  
  // Connect
  sock = comm_connect("localhost");
  get_and_display_services(sock);
}

void get_and_display_services(int control) {
	// Clear List boxes.
	SendMessage(slb, LB_RESETCONTENT, 0, 0);
	SendMessage(clb, LB_RESETCONTENT, 0, 0);

	Services s;
	// Request services
	request_services(control);

	// Recv
	string services = recv_services(control);

	// Parse
	ParseServicesList(services, s);

	for (size_t i = 0; i < s.songs.size(); ++i) {
		SendMessage(slb, LB_ADDSTRING, 0, (LPARAM)s.songs[i].c_str());
	}

	for (size_t i = 0; i < s.channels.size(); ++i) {
		SendMessage(clb, LB_ADDSTRING, 0, (LPARAM)s.channels[i].c_str());
	}
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent;
  HDC		hdc;
  PAINTSTRUCT ps;
  RECT	rect;
  static HBRUSH hbrBkgnd;  // handle of background colour brush  
  static COLORREF crBkgnd; // color of main window background 

  switch (message) {
  case WM_CREATE :
	  crBkgnd = RGB(102, 178, 255);				// set background colour for main window
    hbrBkgnd = CreateSolidBrush(crBkgnd);		// create background brush with background colour
    create_gui ( hWnd );
    break;
  case WM_COMMAND:
    wmId    = LOWORD(wParam);
    wmEvent = HIWORD(wParam);
    UNREFERENCED_PARAMETER(wmEvent);
    // Parse the menu selections:
    switch (wmId) {
      case IDM_ABOUT:
        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
        break;
      case IDC_BTN_STREAM: {
        int lbItem = (int)SendMessage(slb, LB_GETCURSEL, 0, 0); 
        if (lbItem != LB_ERR) {
          char* song_name = new char[BUFSIZE];
          SendMessage(slb, LB_GETTEXT, lbItem, (LPARAM)song_name);
          cout << "Song selected" << song_name << endl;
          CreateThread(NULL, 0, stream_song_proc, (LPVOID)song_name, 0, NULL);
        }

        break;
	  }
	  case ID_VOICECHAT_CHATWITHSERVER:
		  get_and_display_services(sock);
		  break;
	  case ID_SONGS_UPLOADSONGTOLIST:
		  uploadFile(1338);
		  break;
	  case ID_SONGS_DOWNLOADSELECTEDSONG: {
		  int lbItem = (int)SendMessage(slb, LB_GETCURSEL, 0, 0); 
		  if (lbItem != LB_ERR) {
			  char* song_name = new char[BUFSIZE];
			  SendMessage(slb, LB_GETTEXT, lbItem, (LPARAM)song_name);
			  downloadFile(1337, song_name);
		  }
		  break;
	  }
      case IDM_EXIT:
        DestroyWindow(hWnd);
        break;
      default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    } 
    break;
  case WM_PAINT  :
    hdc = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rect);
    FillRect(hdc, &rect, hbrBkgnd);
    EndPaint(hWnd, &ps);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hwnd)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   g_Hwnd = hWnd = CreateWindow("DCA3", "DJK Player", (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),
      CW_USEDEFAULT, 0, 700, 380, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   hwnd = hWnd;
   return TRUE;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DCWIN1));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_DCWIN1);
	wcex.lpszClassName	= "DCA3";
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		{
			string zplay("Libzplay Version: ");
			libZPlay::ZPlay* zp = libZPlay::CreateZPlay();
			stringstream ss;
			ss << (double) (zp->GetVersion()/100.0);
			zplay += ss.str();

			SendMessage( GetDlgItem(hDlg, IDC_LIBZPLAY_VERSION), WM_SETTEXT, NULL, (LPARAM) zplay.c_str());
		}
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
