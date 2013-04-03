/*---------------------------------------------------------------------------------------------
--	SOURCE FILE: client-gui.cpp
--
--	PROGRAM:	client.exe
--
--	FUNCTIONS:	void set_progress_bar_range (size_t total_size)
--				void set_progress_bar (int value)
--				void increment_progress_bar (size_t amount)
--				int send_ec (int s, const char* buf, size_t len, int flags)
--				bool check_connected ()
--				void create_gui (HWND hWnd)
--				void get_and_display_services(int control)
--				void stop_and_reset_player()
--				DWORD WINAPI fft_draw_loop (LPVOID lpParamter)
--				LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
--				BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hwnd)
--				ATOM MyRegisterClass(HINSTANCE hInstance)
--				INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
--				INT_PTR CALLBACK ServerSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
--					
--	DATE:		23/Mar/2013
--
--	DESIGNERS:	Kevin Tangeman, David Czech
--	PROGRAMMERS: Kevin Tangeman, David Czech
--
--	NOTES:		Defines the window initialization and creation code for the client GUI.
------------------------------------------------------------------------------------------------*/

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
HWND g_Hwnd, slb, clb, progress, hStatus, hFFTwin = 0;;
extern HINSTANCE hInst;  // current instance from main.cpp
char displayServer[1024];
char displayCurrent[1024];
char temp_name[1024];


/*----------------------------------------------------------------------------------------------
-- FUNCTION:   set_progress_bar
--
-- DATE:       Mar 23, 2013
--
-- DESIGNERS:   
-- PROGRAMMERS: 
--
-- INTERFACE:  void set_progress_bar (int value)
-- RETURNS:    void
--
-- NOTES: 	
----------------------------------------------------------------------------------------------*/
void set_progress_bar (int value) {
  SetWindowLong (progress, GWL_STYLE, WS_CHILD|WS_VISIBLE|PBS_SMOOTH);
  SendMessage(progress, PBM_SETPOS, value, 0);
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:   set_progress_bar_range
--
-- DATE:       Mar 23, 2013
--
-- DESIGNERS:   
-- PROGRAMMERS: 
--
-- INTERFACE:  void set_progress_bar_range (size_t total_size)
-- RETURNS:    void
--
-- NOTES: 	
----------------------------------------------------------------------------------------------*/
void set_progress_bar_range (size_t total_size) {
  SendMessage(progress, PBM_SETRANGE32, 0, total_size);
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:   increment_progress_bar
--
-- DATE:       Mar 23, 2013
--
-- DESIGNERS:   
-- PROGRAMMERS: 
--
-- INTERFACE:  void increment_progress_bar (size_t amount)
-- RETURNS:    void
--
-- NOTES: 	
----------------------------------------------------------------------------------------------*/
void increment_progress_bar (size_t amount) {
  SendMessage(progress, PBM_DELTAPOS, amount, 0);
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:   send_ec
--
-- DATE:       Mar 23, 2013
--
-- DESIGNERS:   
-- PROGRAMMERS: 
--
-- INTERFACE:  int send_ec (int s, const char* buf, size_t len, int flags)
-- RETURNS:    int
--
-- NOTES: 	Wrapper for sending with error checking and reporting 
--				(shows a messagebox if call failed).
----------------------------------------------------------------------------------------------*/
int send_ec (int s, const char* buf, size_t len, int flags) {
  int ret;
  if ( (ret = send(s, buf, len, flags)) < 1) {
    MessageBox(0, "Disconnected.", "Error while sending to server", 0);
    sock = 0;
  }
  return ret;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:   check_connected
--
-- DATE:       Mar 23, 2013
--
-- DESIGNERS:   
-- PROGRAMMERS: 
--
-- INTERFACE:  bool check_connected ()
-- RETURNS:    bool
--
-- NOTES: 	
----------------------------------------------------------------------------------------------*/
bool check_connected () {
  if (sock == 0)
    MessageBox(0, "Not connected.", "Error", 0);

  return sock != 0;
}

/*-----------------------------------------------------------------------------------------------
-- FUNCTION:	create_gui
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:	David Czech
-- PROGRAMMERS:	David Czech, Kevin Tangeman
--
-- INTERFACE:	void create_gui (HWND hWnd)
-- RETURNS:		void
--
-- NOTES:		Populate the parent hWnd with the GUI for the client (listboxes, buttons, etc).
------------------------------------------------------------------------------------------------*/
void create_gui (HWND hWnd) {
  HFONT hFont;

  // Create Input and Output Edit Text controls.
  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

  // Setup to create a progress bar for playback, download, upload status
  INITCOMMONCONTROLSEX InitCtrlEx;
  InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
  InitCtrlEx.dwICC  = ICC_PROGRESS_CLASS;
  InitCommonControlsEx(&InitCtrlEx);

  SendMessage (
  clb = CreateWindow("LISTBOX", "ChannelList",  // Channels can be listed and selected here
    WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY|WS_TABSTOP, 
    475, 55, 175, 210, hWnd, (HMENU)ID_LS_CHANNELS, NULL, NULL)
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

  SendMessage (
    slb = CreateWindow("LISTBOX", "SongList", // Songs can be listed and selected here
    WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY|WS_TABSTOP, 
    50, 55, 410, 210, hWnd, (HMENU)ID_LS_SONGS, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Play button for play control
    CreateWindow("BUTTON", "Play",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP,
      215, 275, 80, 30, hWnd, (HMENU)IDC_BTN_PLAY, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Prev button for play control
    CreateWindow("BUTTON", "Prev",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP,
      95, 280, 40, 20, hWnd, (HMENU)IDC_BTN_PREV, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Stop button for play control
    CreateWindow("BUTTON", "Stop",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP,
      150, 275, 50, 30, hWnd, (HMENU)IDC_BTN_STOP, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Pause button for play control
    CreateWindow("BUTTON", "Pause",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP,
      310, 275, 50, 30, hWnd, (HMENU)IDC_BTN_PAUSE, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Next button for play control
    CreateWindow("BUTTON", "Next",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP,
      375, 280, 40, 20, hWnd, (HMENU)IDC_BTN_NEXT, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Upload button for uploading files
    CreateWindow("BUTTON", "Upload",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP,
      50, 10, 80, 20, hWnd, (HMENU)IDC_BTN_UPLOAD, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

  SendMessage (             // Download button for downloading files
    CreateWindow("BUTTON", "Download",
      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|WS_TABSTOP,
      380, 10, 80, 20, hWnd, (HMENU)IDC_BTN_DOWNLOAD, NULL, NULL)
    ,WM_SETFONT, (WPARAM)hFont, TRUE);

   SendMessage(
    progress = CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, "Progress",  // progress bar
      WS_CHILD|WS_VISIBLE|PBS_SMOOTH|PBS_MARQUEE,
      50, 315, 600, 20, hWnd, NULL, hInst, NULL)
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

  SendMessage (		// create status bar at bottom of window
  hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
	WS_CHILD|WS_VISIBLE, 0, 0, 0, 0,
    hWnd, (HMENU)IDC_MAIN_STATUS, GetModuleHandle(NULL), NULL)
	,WM_SETFONT, (WPARAM)hFont, TRUE);

  // make 2 sections in status bar, left for "Currently playing: ... (song or channel)", 
		// right for "Server: ..."
  int statwidths[] = {465, -1};
  SendMessage(hStatus, SB_SETPARTS, sizeof(statwidths)/sizeof(int), (LPARAM)statwidths);

  // Show server hostname dialog box.
  DialogBox(hInst, MAKEINTRESOURCE(IDD_SERVERSETUPBOX), hWnd, ServerSetup);
  hFFTwin = CreateWindow((LPSTR)32770, "FFT Graph", 0, 0, 0, 300, 225, 0, NULL, 0, 0);
  // Toolbar style window.
  /*hFFTwin = CreateWindowEx(WS_EX_TOOLWINDOW, 
                       (LPSTR)32770,
                       "FFT Graph",
                       WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       315,
                       233,
                       NULL,
                       NULL,
                       hInst,
                       NULL);*/
}

/*-----------------------------------------------------------------------------------------------------------
-- FUNCTION:   get_and_display_services
--
-- DATE:       Mar 23, 2013
--
-- DESIGNER:   David Czech
-- PROGRAMMER: David Czech
--
-- INTERFACE:  void get_and_display_services(int control)
-- RETURNS:    nothing
--
-- NOTES: Request services from server, recieve its reply and populate the listboxes in the GUI with the data.
-------------------------------------------------------------------------------------------------------------*/
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

/*-----------------------------------------------------------------------------------------------------------
-- FUNCTION:   stop_and_reset_player
--
-- DATE:       Mar 27, 2013
--
-- DESIGNER:   David Czech
-- PROGRAMMER: David Czech
--
-- INTERFACE:  void stop_and_reset_player()
-- RETURNS:    nothing
--
-- NOTES:		Resets Client ZPlay instance (clears all current data in stream) and re-opens a new dynamic 
--				stream.
------------------------------------------------------------------------------------------------------------*/
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


/*--------------------------------------------------------------------------------------------------
-- FUNCTION:   fft_draw_loop
--
-- DATE:       Mar 23, 2013
--
-- DESIGNERS:   
-- PROGRAMMERS: 
--
-- INTERFACE:  DWORD WINAPI fft_draw_loop (LPVOID lpParamter)
-- RETURNS:    DWORD
--
-- NOTES: 	
----------------------------------------------------------------------------------------------------*/
DWORD WINAPI fft_draw_loop (LPVOID lpParamter) {
  while (true) {
    Sleep(20);
    if (netplay && hFFTwin != 0)
      netplay->DrawFFTGraphOnHWND(hFFTwin, 0, 0, 300, 200);
  }
}


/*--------------------------------------------------------------------------------------------------
-- FUNCTION:   WndProc
--
-- DATE:       Mar 23, 2013
--
-- DESIGNERS:   David Czech, Kevin Tangeman
-- PROGRAMMERS: David Czech, Kevin Tangeman
--
-- INTERFACE:	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
-- RETURNS:		LRESULT
--
-- NOTES:		Processes messages for the main window.
----------------------------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent, countItem, indexItem = -1;
  HDC   hdc;
  PAINTSTRUCT ps;
  RECT  rect;
  static HBRUSH hbrBkgnd;  // handle of background colour brush  
  static COLORREF crBkgnd; // color of main window background 
  static HANDLE hChannelThread = 0;

  switch (message) {

  case WM_CREATE :
    crBkgnd = RGB(102, 178, 255);       // set background colour for main window
    hbrBkgnd = CreateSolidBrush(crBkgnd);   // create background brush with background colour
    create_gui ( hWnd );
    CreateThread(NULL, 0, fft_draw_loop, (LPVOID)NULL, 0, NULL);
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

      case ID_FILE_VIEWFFT:
        ShowWindow(hFFTwin, SW_SHOW);
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
			strcpy(temp_name, song_name);			// copy song_name to temp_name for "pause" function
            cout << "Song selected " << song_name << endl;
            // Build and send request line
            string request = "S " + string(song_name) + "\n";

            if (send_ec(sock, request.data(), request.size(), 0)) {
              // Start progress bar marquee
              SetWindowLong (progress, GWL_STYLE, GetWindowLong(progress, GWL_STYLE) | PBS_MARQUEE);
              SendMessage(progress, PBM_SETMARQUEE, 1, 1);

			  // display "Currently playing: <song_name>" in status bar
			  strcpy_s(displayCurrent, "Currently playing: ");
			  strcat_s(displayCurrent, song_name);	
			  SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text
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
        SendMessage(GetDlgItem(hWnd, IDC_BTN_PAUSE), WM_SETTEXT, 0, (LPARAM) "Pause"); // change button text
		strcpy_s(displayCurrent, "");
		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // clear status bar text
        break;

      case ID_SONGS_PAUSESELECTEDSONG:
      case IDC_BTN_PAUSE:
        TStreamStatus status;
        netplay->GetStatus(&status);
		if(strlen(temp_name) > 0){	// in case pause is pressed before play is pressed for the first time
        if (status.fPause) {	// if already paused, then resume
          if (netplay->Resume()) {
            SendMessage(GetDlgItem(hWnd, IDC_BTN_PAUSE), WM_SETTEXT, 0, (LPARAM) "Pause");
            SendMessage(progress, PBM_SETMARQUEE, 1, 0);

			// display "Currently playing: <song_name>" in status bar
			strcpy_s(displayCurrent, "Currently playing: ");
			strcat_s(displayCurrent, temp_name);	
			SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text
          }
        } else {				// if not paused, then pause
          if (netplay->Pause()) {
            SendMessage(GetDlgItem(hWnd, IDC_BTN_PAUSE), WM_SETTEXT, 0, (LPARAM) "Resume");
            SendMessage(progress, PBM_SETMARQUEE, 0, 0);
            SendMessage(progress, WM_PAINT, 0, 0);

			// display "Currently playing: <song_name>" in status bar
			strcpy_s(displayCurrent, "Currently playing: ");
			strcat_s(displayCurrent, temp_name);	
			strcat_s(displayCurrent, " - PAUSED");
			SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text
          }
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
          indexItem += 1;	   // increments it to get next index
          countItem = (int)SendMessage(slb, LB_GETCOUNT, 0, 0);	// get count of items in list

          // make sure current selection wasn't already last in list
          if((countItem != LB_ERR) && (indexItem < countItem)) {
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

		// display "Currently playing: Live Chat" in status bar
		strcpy_s(displayCurrent, "Currently playing: ");
		strcat_s(displayCurrent, "Live Chat");	
		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text
        break;

      case IDC_BTN_UPLOAD:
      case ID_SONGS_UPLOADSONGTOLIST:
		// display "Uploading song to server" in status bar
		strcpy_s(displayCurrent, "Uploading song to server");
		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text

        uploadFile(1337);
        break;

      case IDC_BTN_DOWNLOAD:
      case ID_SONGS_DOWNLOADSELECTEDSONG: {
        int lbItem = (int)SendMessage(slb, LB_GETCURSEL, 0, 0); 
        if (lbItem != LB_ERR) {
          char* song_name = new char[BUFSIZE];
          SendMessage(slb, LB_GETTEXT, lbItem, (LPARAM)song_name);

		  // display "Downloading song: <song_name>" in status bar
		  strcpy_s(displayCurrent, "Downloading song: ");
		  strcat_s(displayCurrent, song_name);	
		  SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text

          downloadFile(1337, song_name);	// call download() with song_name
        }
        break;
      }
      
      // Fall through for channel list box events.
      case ID_LS_CHANNELS:
        if (wmEvent != LBN_DBLCLK)
          break;

      case IDC_BTN_STREAM:
      case ID_CHANNELS_STREAMSELECTEDCHANNEL: {
        
		keep_streaming_channel = false;
		if (hChannelThread)
			WaitForSingleObject(hChannelThread, 3000);
        keep_streaming_channel = true;
        // Before joining the channel stop anything currently playing.
        send_ec(sock, "stop-stream\n", 14, 0);
        stop_and_reset_player();

        int lbItem = (int)SendMessage(clb, LB_GETCURSEL, 0, 0); 
        if (lbItem != LB_ERR) {
          char* channel = new char[BUFSIZE];
		  SendMessage(clb, LB_GETTEXT, lbItem, (LPARAM)channel);
          hChannelThread = CreateThread(NULL, 0, join_channel, (LPVOID)channel, 0, NULL);

		  // Start progress bar marquee
          SetWindowLong (progress, GWL_STYLE, GetWindowLong(progress, GWL_STYLE) | PBS_MARQUEE);
          SendMessage(progress, PBM_SETMARQUEE, 1, 0);

		  // display "Currently playing: <channel name>" in status bar
		  strcpy_s(displayCurrent, "Currently playing: ");
		  strcat_s(displayCurrent, channel);	
		  SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 // change status bar text
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
		  stop_and_reset_player();

		  // clear status bar text
		  strcpy_s(displayCurrent, "");
		  SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)displayCurrent);	 
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


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	InitInstance
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:   David Czech
-- PROGRAMMERS: David Czech, Kevin Tangeman
--
-- INTERFACE:	BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hwnd)
-- RETURNS:		BOOL
--
-- NOTES: 		Saves instance handle and creates main window.
--				In this function, we save the instance handle in a global variable and
--				create and display the main program window.
------------------------------------------------------------------------------------------------*/
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hwnd)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   g_Hwnd = hWnd = CreateWindow("DCA3", "DJK Player", (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),
      CW_USEDEFAULT, 0, 700, 420, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   hwnd = hWnd;
   return TRUE;
}


/*----------------------------------------------------------------------------------------------
-- FUNCTION:   MyRegisterClass
--
-- DATE:       Mar 23, 2013
--
-- DESIGNERS:   David Czech
-- PROGRAMMERS: David Czech
--
-- INTERFACE:  ATOM MyRegisterClass(HINSTANCE hInstance)
-- RETURNS:    RegisterClassEx
--
-- NOTES: 		Registers the window class. 
--				This function and its usage are only necessary if you want this code
--				to be compatible with Win32 systems prior to the 'RegisterClassEx'
--				function that was added to Windows 95. It is important to call this function
--				so that the application will get 'well formed' small icons associated with it.
------------------------------------------------------------------------------------------------*/
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


/*----------------------------------------------------------------------------------------------
-- FUNCTION:	About
--
-- DATE:		Mar 23, 2013
--
-- DESIGNERS:   David Czech
-- PROGRAMMERS: David Czech
--
-- INTERFACE:	INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
-- RETURNS:		INT_PTR
--
-- NOTES: 		Message handler for about box.
------------------------------------------------------------------------------------------------*/
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


/*--------------------------------------------------------------------------------------------------
-- FUNCTION:	ServerSetup
-- DATE:		Mar 29, 2013
--
-- DESIGNER:	Kevin Tangeman
-- PROGRAMMER:	Kevin Tangeman
--
-- INTERFACE:	INT_PTR CALLBACK ServerSetup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
-- RETURNS:		INT_PTR
--
-- NOTES:		Handles the messages for the Server Setup Dialog Box
----------------------------------------------------------------------------------------------------*/
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
					sock = comm_connect(server);		// create new server connection
					if (sock) {
						get_and_display_services(sock);	// display services from new server
						EndDialog(hDlg, LOWORD(wParam));	// close dialog box

						// display server connected to in status bar
						strcpy_s(displayServer, "Connected to: ");
						strcat_s(displayServer, server);	
						SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)displayServer);
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
