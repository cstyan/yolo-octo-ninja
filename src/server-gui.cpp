// Assignment 2 Server GUI
// server-gui.cpp : Defines the window GUI initialization and creation code.
//
#include "a2.h"
#include "resource.h"

/* Easy styles enabler for msvc */
#ifdef _MSC_VER
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

HWND g_Hwnd;
extern HINSTANCE hInst;	 // current instance from main.cpp

void create_gui (HWND hWnd) {
  HFONT hFont;
  // Create Input and Output Edit Text controls.
  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

  ///////////// Packet Source controls.
  SendMessage (
  CreateWindow("BUTTON", "Packet Data Destination:",
      WS_CHILD|WS_VISIBLE|BS_GROUPBOX,
      5, 5, 220, 75, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("EDIT", "Data Discarded",
      WS_CHILD|WS_VISIBLE|ES_READONLY|ES_AUTOHSCROLL, 
      10, 50+1, 210, 27, hWnd, (HMENU)IDC_TXT_PTYPE, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("STATIC", "Server port:",
      WS_CHILD|WS_VISIBLE, 
      10, 28, 70, 15, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "1337", 
    WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP | ES_AUTOHSCROLL | ES_NUMBER,
    70, 25, 35-2, 20, hWnd, (HMENU)IDC_EDT_PMULT, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("BUTTON", "File",
      WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_GROUP, 
      115, 20, 40, 25, hWnd, (HMENU)IDC_BTN_FILE, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("BUTTON", "Discard",
      WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_GROUP, 
      155+2, 20, 65, 25, hWnd, (HMENU)IDC_BTN_PROC, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);
  //////////////////////////////////////////////////
  ///////////// Statistics.
  SendMessage (
    CreateWindow("BUTTON", "Statistics:",
      WS_CHILD|WS_VISIBLE|BS_GROUPBOX, 
      5, 140, 220, 52, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);    

  SendMessage (
  CreateWindow("EDIT", "Ready...",
      WS_CHILD|WS_VISIBLE|ES_READONLY, 
      15, 160, 200, 15, hWnd, (HMENU)IDC_TXT_STATS, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("BUTTON", "Listen...",
      WS_CHILD|WS_VISIBLE| WS_TABSTOP, 
      5, 200, 220, 35, hWnd, (HMENU)IDC_BTN_GO, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);
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

   g_Hwnd = hWnd = CreateWindow("DCA1", "DCA2 Server", (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),
      CW_USEDEFAULT, 0, 235, 287, NULL, NULL, hInstance, NULL);

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
	wcex.lpszClassName	= "DCA1";
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

void SetStatus (HWND hWnd, const char * status) {
	SendMessage(GetDlgItem(hWnd, IDC_TXT_STATS), WM_SETTEXT, 0, (LPARAM) status);
}