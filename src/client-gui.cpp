// Assignment 3 Client GUI
// client-gui.cpp : Defines the window GUI initialization and creation code.
//
#include "CommAudio.h"
#include "resource.h"

/* Easy styles enabler for msvc */
#ifdef _MSC_VER
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

DWORD WINAPI stream_song_proc(LPVOID lpParamter);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

HWND g_Hwnd;
extern HINSTANCE hInst;	 // current instance from main.cpp

void create_gui (HWND hWnd) {
  HFONT hFont;
  //HWND heInput;
  // Create Input and Output Edit Text controls.
  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

  ///////////// Packet Source controls.
  SendMessage (
  CreateWindow("BUTTON", "Packet Data Source:",
      WS_CHILD|WS_VISIBLE|BS_GROUPBOX,
      5, 5, 220, 75, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("BUTTON", "Stream",
      WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_GROUP, 
      100, 20, 40, 25, hWnd, (HMENU)IDC_BTN_STREAM, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);
  /*SendMessage (
  CreateWindow("EDIT", "Procedural",
      WS_CHILD|WS_VISIBLE|ES_READONLY|ES_AUTOHSCROLL|ES_MULTILINE, 
      10, 25+1, 90, 27, hWnd, (HMENU)IDC_TXT_PTYPE, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);


  SendMessage (
  CreateWindow("BUTTON", "Procedural",
      WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_GROUP, 
      155+2, 20, 65, 25, hWnd, (HMENU)IDC_BTN_PROC, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("STATIC", "Packet size:",
      WS_CHILD|WS_VISIBLE, 
      10, 53, 70, 15, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("STATIC", "B *             Pckts",
      WS_CHILD|WS_VISIBLE, 
      140, 53, 83, 15, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "1024", 
    WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP | ES_AUTOHSCROLL | ES_NUMBER,
    101, 50, 40-2, 20, hWnd, (HMENU)IDC_EDT_PSIZE, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "1024", 
    WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP | ES_AUTOHSCROLL | ES_NUMBER,
    155+3, 50, 35-2, 20, hWnd, (HMENU)IDC_EDT_PMULT, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);
  //////////////////////////////////////////////////
  ///////////// Destination and client controls.
  SendMessage (
    CreateWindow("BUTTON", "Destination:",
      WS_CHILD|WS_VISIBLE|BS_GROUPBOX, 
      5, 85, 220, 52, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  heInput  = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "127.0.0.1:1337", 
    WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP,
    15,  105, 200, 22, hWnd, (HMENU)IDC_EDT_DEST, NULL, NULL);
  SendMessage (heInput,  WM_SETFONT, (WPARAM)hFont, TRUE);
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
  CreateWindow("BUTTON", "Connect...",
      WS_CHILD|WS_VISIBLE| WS_TABSTOP, 
      5, 200, 220, 35, hWnd, (HMENU)IDC_BTN_GO, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);*/
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

  switch (message)
  {
  case WM_CREATE:
    create_gui ( hWnd );
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
    case IDC_BTN_STREAM:
      CreateThread(NULL, 0, stream_song_proc, NULL, 0, NULL);
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

   g_Hwnd = hWnd = CreateWindow("DCA1", "DCA2 Client", (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),
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
