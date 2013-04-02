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

extern bool keep_streaming_channel;
extern ZPlay * netplay;

/* Easy styles enabler for msvc */
#ifdef _MSC_VER
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

DWORD WINAPI stream_song_proc(LPVOID lpParamter);
DWORD WINAPI join_channel(LPVOID lpParamter);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ServerSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void get_and_display_services(int control);

// if sock == 0, it means we're not connected.
int sock = 0;
HWND g_Hwnd, slb, clb, progress;
extern HINSTANCE hInst;  // current instance from main.cpp

void set_progress_bar (int value) {
  SetWindowLong (progress, GWL_STYLE, WS_CHILD|WS_VISIBLE|PBS_SMOOTH);
  SendMessage(progress, PBM_SETPOS, value, 0);
}

void set_progress_bar_range (size_t total_size) {
  SendMessage(progress, PBM_SETRANGE32, 0, total_size);
}

void increment_progress_bar (size_t amount) {
  SendMessage(progress, PBM_DELTAPOS, amount, 0);
}

// Wrapper for sending with error checking and reporting (shows a messagebox if call failed).
int send_ec (int s, const char* buf, size_t len, int flags) {
  int ret;
  if ( (ret = send(s, buf, len, flags)) < 1) {
    MessageBox(0, "Disconnected.", "Error while sending to server", 0);
    sock = 0;
  }
  return ret;
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
  --				Kevin Tangeman - Created basic client GUI interface layout
  --
  -- INTERFACE:  void create_gui (HWND hWnd)
  --    hwnd - the handle to the client parent window.
  --
  -- RETURNS:    nothing
  --
  -- NOTES: Populate the parent hwnd with the GUI for the client (listboxes, buttons, etc).
  --------------------------------------------------------------------------------------------------------------------*/
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
    progress = CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, "Progress",  // progress bar
      WS_CHILD|WS_VISIBLE|PBS_SMOOTH|PBS_MARQUEE,
      50, 315, 600, 20, hWnd, NULL, hInst, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Prev button for play control
    CreateWindow("BUTTON", "Prev",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      95, 280, 40, 20, hWnd, (HMENU)IDC_BTN_PREV, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Play button for play control
    CreateWindow("BUTTON", "Play",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      215, 275, 80, 30, hWnd, (HMENU)IDC_BTN_PLAY, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Stop button for play control
    CreateWindow("BUTTON", "Stop",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      155, 275, 40, 30, hWnd, (HMENU)IDC_BTN_STOP, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Pause button for play control
    CreateWindow("BUTTON", "Pause",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      315, 275, 40, 30, hWnd, (HMENU)IDC_BTN_PAUSE, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Next button for play control
    CreateWindow("BUTTON", "Next",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      375, 280, 40, 20, hWnd, (HMENU)IDC_BTN_NEXT, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  /*SendMessage (             // Mic button for using microphone
   CreateWindow("BUTTON", "Chat",
     WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
     580, 260, 60, 30, hWnd, (HMENU)IDC_BTN_CHAT, NULL, NULL)
   ,WM_SETFONT, (WPARAM)hFont, TRUE);*/

  SendMessage (             // Download button for downloading files
    CreateWindow("BUTTON", "Download",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      380, 10, 80, 20, hWnd, (HMENU)IDC_BTN_DOWNLOAD, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Upload button for uploading files
    CreateWindow("BUTTON", "Upload",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
      50, 10, 80, 20, hWnd, (HMENU)IDC_BTN_UPLOAD, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  slb = CreateWindow("LISTBOX", "SongList", // Songs can be listed and selected here
    WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY, 
    50, 55, 410, 210, hWnd, (HMENU)ID_LS_SONGS, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  clb = CreateWindow("LISTBOX", "ChannelList",  // Channels can be listed and selected here
    WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY, 
    475, 55, 175, 210, hWnd, (HMENU)ID_LS_CHANNELS, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("STATIC", "Song List",
    WS_CHILD|WS_VISIBLE|SS_CENTER, 
    50, 40, 410, 15, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("STATIC", "Channel List",
    WS_CHILD|WS_VISIBLE|SS_CENTER, 
    475, 40, 175, 15, hWnd, (HMENU)-1, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("BUTTON", "Stream",
    WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_GROUP, 
    475, 275, 125, 30, hWnd, (HMENU)IDC_BTN_STREAM, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (
  CreateWindow("BUTTON", "Stop",
    WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_GROUP, 
    600, 275, 50, 30, hWnd, (HMENU)IDC_BTN_STREAM_STOP, NULL, NULL)
  ,WM_SETFONT, (WPARAM)hFont, TRUE);
  
  // Show server hostname dialog box.
  DialogBox(hInst, MAKEINTRESOURCE(IDD_SERVERSETUPBOX), hWnd, ServerSetup);
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
  --------------------------------------------------------------------------------------------------------------------*/
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
//  WM_COMMAND  - dispatch and process the appropiate action for the command (stream, download, upload, etc)
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent, countItem, indexItem = -1;
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

  case WM_CTLCOLORBTN: {
    return (LRESULT)hbrBkgnd;
    }

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
      
      // Fall through for Song Listbox double click.
      case ID_LS_SONGS:
        if (wmEvent != LBN_DBLCLK)
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
            if (send_ec(sock, request.data(), request.size(), 0)) {
              // Start progress bar marquee
              SetWindowLong (progress, GWL_STYLE, GetWindowLong(progress, GWL_STYLE) | PBS_MARQUEE);
              SendMessage(progress, PBM_SETMARQUEE, 1, 0);
            }
          }
          break;
         }

      case ID_SONGS_STOPSELECTEDSONG:
      case IDC_BTN_STOP:
        // Stop progress bar marquee
        SetWindowLong (progress, GWL_STYLE, GetWindowLong(progress, GWL_STYLE)| PBS_MARQUEE);
        SendMessage(progress, PBM_SETMARQUEE, 0, 0);

        send_ec(sock, "stop-stream\n", 14, 0);
        stop_and_reset_player();
        netplay->Stop();
        SendMessage(GetDlgItem(hWnd, IDC_BTN_PAUSE), WM_SETTEXT, 0, (LPARAM) "Pause");
        break;

      case ID_SONGS_PAUSESELECTEDSONG:
      case IDC_BTN_PAUSE:
        TStreamStatus status;
        netplay->GetStatus(&status);
        if (status.fPause) {
          if (netplay->Resume()) {
            SendMessage(GetDlgItem(hWnd, IDC_BTN_PAUSE), WM_SETTEXT, 0, (LPARAM) "Pause");
            SendMessage(progress, PBM_SETMARQUEE, 1, 0);
          }
        } else {
          if (netplay->Pause()) {
            SendMessage(GetDlgItem(hWnd, IDC_BTN_PAUSE), WM_SETTEXT, 0, (LPARAM) "Resume");
            SendMessage(progress, PBM_SETMARQUEE, 0, 0);
            SendMessage(progress, WM_PAINT, 0, 0);
          }
        }
        break;

      case ID_SONGS_PLAYPREV:
      case IDC_BTN_PREV:
		indexItem = (int)SendMessage(slb, LB_GETCURSEL, 0, 0);	// gets current selectin index
        
		if (indexItem != LB_ERR) {
		  indexItem -= 1;			// decrements it to get prev index
		  
		  if(indexItem >= 0){		// make sure current selection wasn't already first in list
	  	    SendMessage(slb, LB_SETCURSEL, indexItem, 0);		// select prev song in list
		    SendMessage(hWnd, WM_COMMAND, IDC_BTN_PLAY, 0);		// send 'play' message
		  }
		}
		break;

      case ID_SONGS_PLAYNEXT:
      case IDC_BTN_NEXT:
		indexItem = (int)SendMessage(slb, LB_GETCURSEL, 0, 0); 	// gets current selection index
        
		if (indexItem != LB_ERR) {
		  indexItem += 1;			// increments it to get next index
		  countItem = (int)SendMessage(slb, LB_GETCOUNT, 0, 0);	// get count of items in list
		  
		  // make sure current selection wasn't already last in list
		  if((countItem != LB_ERR) && (indexItem < countItem)){
		    SendMessage(slb, LB_SETCURSEL, indexItem, 0); 		// select prev song in list
		    SendMessage(hWnd, WM_COMMAND, IDC_BTN_PLAY, 0);		// send 'play' message
		  }
		}
        break;

      case IDC_BTN_CHAT:
      case ID_VOICECHAT_CHATWITHSERVER:
        // Reset player
        stop_and_reset_player();
        
        // Send voice chat request
        send_ec(sock, "V\n", 2, 0);

        // start microphone stream
        start_microphone_stream();
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
      
      // Fall through for channel list box events.
      case ID_LS_CHANNELS:
        if (wmEvent != LBN_DBLCLK)
          break;

      case IDC_BTN_STREAM:
      case ID_CHANNELS_STREAMSELECTEDCHANNEL: {
        // Start progress bar marquee
        SetWindowLong (progress, GWL_STYLE, GetWindowLong(progress, GWL_STYLE) | PBS_MARQUEE);
        SendMessage(progress, PBM_SETMARQUEE, 1, 0);

        keep_streaming_channel = true;
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

      case IDC_BTN_STREAM_STOP:
      case ID_CHANNELS_STOPSTREAMING:
        // Stop progress bar marquee
        if (keep_streaming_channel) {
          SetWindowLong (progress, GWL_STYLE, GetWindowLong(progress, GWL_STYLE)| PBS_MARQUEE);
          SendMessage(progress, PBM_SETMARQUEE, 0, 0);
          keep_streaming_channel = false;
        }
       break;

      case ID_SETUP_SELECTSERVER:
        if (sock != 0) {
          send(sock, "stop-stream\n", 14, 0);
          closesocket(sock);
          sock = 0;
        }
        DialogBox(hInst, MAKEINTRESOURCE(IDD_SERVERSETUPBOX), hWnd, ServerSetup);
        break;

      case ID_FILE_REFRESHSERVICES:
        get_and_display_services(sock);
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
      CW_USEDEFAULT, 0, 700, 395, NULL, NULL, hInstance, NULL);

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


/*------------------------------------------------------------------------------------------------------------------
  -- FUNCTION:   ServerSetup Dialog Box Message Handler
  -- DATE:       Mar 29, 2013
  --
  -- DESIGNER:   Kevin Tangeman
  -- PROGRAMMER: Kevin Tangeman
  --
  -- INTERFACE:  INT_PTR CALLBACK ServerSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
  -- RETURNS:    INT_PTR
  --
  -- NOTES: Handles the messages for the Server Setup Dialog Box
  ----------------------------------------------------------------------------------------------------------------------*/
INT_PTR CALLBACK ServerSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemText(hDlg, IDC_ADDR_HOSTNAME, server);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:		// get server name here
				{
					GetDlgItemText(hDlg, IDC_ADDR_HOSTNAME, server, 256);  // get input from edit box
					sock = comm_connect(server);
					if (sock) {
						get_and_display_services(sock);
						EndDialog(hDlg, LOWORD(wParam));
					}
					return (INT_PTR)TRUE;
				}

				case IDCANCEL:
				{
					EndDialog(hDlg, LOWORD(wParam));
					return (INT_PTR)TRUE;
				}
			}
	}
	return (INT_PTR)FALSE;
}
