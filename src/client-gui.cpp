// Assignment 3 Client GUI
// client-gui.cpp : Defines the window GUI initialization and creation code.
//
//  PROGRAMMERS:
//    23-Mar-2013 - Kevin Tangeman - Created basic client GUI interface layout in Win32
//
#define _WIN32_IE 0x301
#include "CommAudio.h"
#include "resource.h"
#include "client-file.h"
#include <commctrl.h>
#include <string>
#include "libzplay.h"

using namespace std;
using namespace libZPlay;

extern ZPlay * netplay;

/* Easy styles enabler for msvc */
#ifdef _MSC_VER
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

DWORD WINAPI stream_song_proc(LPVOID lpParamter);
DWORD WINAPI join_channel(LPVOID lpParamter);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void get_and_display_services(int control);

// if sock == 0, it means we're not connected.
int sock = 0;
HWND g_Hwnd, slb, clb;
extern HINSTANCE hInst;  // current instance from main.cpp

// Wrapper for sending with error checking and reporting (shows a messagebox if call failed).
void send_ec (int s, const char* buf, size_t len, int flags) {
  if (send(s, buf, len, flags) < 1) {
    MessageBox(0, "Disconnected.", "Error while sending to server", 0);
    sock = 0;
  }
}

bool check_connected () {
  if (sock == 0)
    MessageBox(0, "Not connected.", "Error", 0);

  return sock != 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:   create_gui
--
-- DATE:       Mar 23, 2013
--
-- DESIGNER:   David Czech
--
-- PROGRAMMER: David Czech
--
-- INTERFACE:  void create_gui (HWND hWnd)
--    hwnd - the handle to the client parent window.
--
-- RETURNS:    nothing
--
-- NOTES: Populate the parent hwnd with the GUI for the client (listboxes, buttons, etc).
----------------------------------------------------------------------------------------------------------------------*/
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
    CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, "Progress",  // progress bar
      WS_CHILD|WS_VISIBLE|PBS_SMOOTH, 
      50, 300, 600, 20, hWnd, NULL, hInst, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Prev button for play control
    CreateWindow("BUTTON", "Prev",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      120, 265, 40, 20, hWnd, (HMENU)IDC_BTN_PREV, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Play button for play control
    CreateWindow("BUTTON", "Play",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      170, 260, 60, 30, hWnd, (HMENU)IDC_BTN_PLAY, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Next button for play control
    CreateWindow("BUTTON", "Next",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      240, 265, 40, 20, hWnd, (HMENU)IDC_BTN_NEXT, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Mic button for using microphone
    CreateWindow("BUTTON", "Chat",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      580, 260, 60, 30, hWnd, (HMENU)IDC_BTN_CHAT, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Download button for downloading files
    CreateWindow("BUTTON", "Dn",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      315, 260, 30, 30, hWnd, (HMENU)IDC_BTN_DOWNLOAD, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Upload button for uploading files
    CreateWindow("BUTTON", "Up",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      50, 260, 30, 30, hWnd, (HMENU)IDC_BTN_UPLOAD, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  slb = CreateWindow("LISTBOX", "SongList", // Songs can be listed and selected here
    WS_CHILD|WS_VISIBLE|WS_VSCROLL, 
    50, 40, 480, 210, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  clb = CreateWindow("LISTBOX", "ChannelList",  // Channels can be listed and selected here
    WS_CHILD|WS_VISIBLE|WS_VSCROLL, 
    540, 40, 100, 210, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("STATIC", "Song List",
      WS_CHILD|WS_VISIBLE|SS_CENTER, 
      50, 25, 480, 15, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("STATIC", "Channel List",
    WS_CHILD|WS_VISIBLE|SS_CENTER, 
    540, 25, 100, 15, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("BUTTON", "Stream",
    WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_GROUP, 
    470, 260, 60, 30, hWnd, (HMENU)IDC_BTN_STREAM, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);
  
  // Connect
  sock = comm_connect(server);
  get_and_display_services(sock);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:   get_and_display_services
--
-- DATE:       Mar 23, 2013
--
-- DESIGNER:   David Czech
--
-- PROGRAMMER: David Czech
--
-- INTERFACE:  void get_and_display_services(int control)
--
-- RETURNS:    nothing
--
-- NOTES: Request services from server, recieve its reply and populate the listboxes in the GUI with the data.
----------------------------------------------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:   stop_and_reset_player
--
-- DATE:       Mar 27, 2013
--
-- DESIGNER:   David Czech
--
-- PROGRAMMER: David Czech
--
-- INTERFACE:  void stop_and_reset_player()
--
-- RETURNS:    nothing
--
-- NOTES: Resets Client ZPlay instance (clears all current data in stream) and re-opens a new dynamic stream.
----------------------------------------------------------------------------------------------------------------------*/
void stop_and_reset_player() {
  // Close current stream
  netplay->Close();

  // Open new stream
  int i;

  // we open the zplay stream without any real data, and start playback when we actually get input.
  int result = netplay->OpenStream(1, 1, &i, 2, sfPCM);
    if(result == 0) {
    cerr << "Error: " <<  netplay->GetError() << endl;
    netplay->Release();
    return;
  }

  // Start playing as soon as we get data.
  netplay->Play();
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
  HDC   hdc;
  PAINTSTRUCT ps;
  RECT  rect;
  static HBRUSH hbrBkgnd;  // handle of background colour brush  
  static COLORREF crBkgnd; // color of main window background 

  switch (message) {

  case WM_CREATE :
    crBkgnd = RGB(102, 178, 255);       // set background colour for main window
    hbrBkgnd = CreateSolidBrush(crBkgnd);   // create background brush with background colour
    create_gui ( hWnd );
    break;

  case WM_COMMAND:
    wmId    = LOWORD(wParam);
    wmEvent = HIWORD(wParam);
    UNREFERENCED_PARAMETER(wmEvent);

    // If the command requires to be connected to the server,
    // check that we are in fact, connected to the server before attempting.
    if ((wmId >= ID_SONGS_PLAYSELECTEDSONG && wmId <= ID_VOICECHAT_CHATWITHSERVER) ||
        (wmId >= IDC_BTN_STREAM && wmId <= IDC_BTN_CHAT))
      if (!check_connected())
        return 0;

    // Parse the menu selections:
    switch (wmId) {
      case IDM_ABOUT:
        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
        break;

      case ID_SONGS_PLAYSELECTEDSONG:
      case IDC_BTN_PLAY: {
        // The stop-stream command isn't necessary here:
        // the server will just switch the current playing song.
        //send(sock, "stop-stream\n", 14, 0);
        stop_and_reset_player();

        // Play thread needs control channel socket and pointer to zplayer instance.
        int lbItem = (int)SendMessage(slb, LB_GETCURSEL, 0, 0); 
        if (lbItem != LB_ERR) {
          char* song_name = new char[BUFSIZE];
          SendMessage(slb, LB_GETTEXT, lbItem, (LPARAM)song_name);
          cout << "Song selected " << song_name << endl;
          // Build and send request line
          string request = "S " + string(song_name) + "\n";
          send_ec(sock, request.data(), request.size(), 0);
        }
        break;
       }

      case IDC_BTN_CHAT:
      case ID_VOICECHAT_CHATWITHSERVER:
        get_and_display_services(sock);
        break;

      case IDC_BTN_UPLOAD:
      case ID_SONGS_UPLOADSONGTOLIST:
        uploadFile(1337);
        break;

      case IDC_BTN_DOWNLOAD:
      case ID_SONGS_DOWNLOADSELECTEDSONG: {
        int lbItem = (int)SendMessage(slb, LB_GETCURSEL, 0, 0); 
        if (lbItem != LB_ERR) {
          char* song_name = new char[BUFSIZE];
          SendMessage(slb, LB_GETTEXT, lbItem, (LPARAM)song_name);
          downloadFile(1337, song_name);
        }
        break;
        }
      
      case IDC_BTN_STREAM:
      case ID_CHANNELS_STREAMSELECTEDCHANNEL: {
        // Before joining the channel stop anything currently playing.
        send_ec(sock, "stop-stream\n", 14, 0);
        stop_and_reset_player();

        int lbItem = (int)SendMessage(clb, LB_GETCURSEL, 0, 0); 
        if (lbItem != LB_ERR) {
              char* channel = new char[BUFSIZE];
              SendMessage(slb, LB_GETTEXT, lbItem, (LPARAM)channel);          
          CreateThread(NULL, 0, join_channel, (LPVOID)channel, 0, NULL);
        }
        break;
      }

      case IDC_BTN_PREV:

        break;

      case IDC_BTN_NEXT:

        break;

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
    // Tell the server to stop the stream before quiting.
    send(sock, "stop-stream\n", 14, 0);
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

  wcex.style      = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc  = WndProc;
  wcex.cbClsExtra   = 0;
  wcex.cbWndExtra   = 0;
  wcex.hInstance    = hInstance;
  wcex.hIcon      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DCWIN1));
  wcex.hCursor    = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW);
  wcex.lpszMenuName = MAKEINTRESOURCE(IDC_DCWIN1);
  wcex.lpszClassName  = "DCA3";
  wcex.hIconSm    = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

      SendMessage( GetDlgItem(hDlg, IDC_LIBZPLAY_VERSION), WM_SETTEXT, 0, (LPARAM) zplay.c_str());
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
