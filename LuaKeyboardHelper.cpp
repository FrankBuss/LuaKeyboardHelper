// LuaKeyboardHelper.cpp : Defines the entry point for the application.
//

#include "header.h"
#include "LuaKeyboardHelper.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#define MAX_LOADSTRING 100

#define IDT_TIMER1 100

bool timerRunning = false;

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND hWnd;
char szTitle[MAX_LOADSTRING];                  // The title bar text
char szWindowClass[MAX_LOADSTRING];            // the main window class name
lua_State *L;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void error(const char* message)
{
	MessageBox(NULL, message, "LuaKeyboardHelper", MB_ICONWARNING);
	exit(0);
}

void sendInput(WORD wVk, DWORD dwFlags)
{
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = wVk;
	ip.ki.wScan = MapVirtualKeyA(wVk, MAPVK_VK_TO_VSC);
	ip.ki.dwFlags = dwFlags;
	ip.ki.time = 0;  // timestamp set by Windows
	ip.ki.dwExtraInfo = 0;
	SendInput(1, &ip, sizeof(INPUT));
}

void destroyTimer()
{
	if (timerRunning) {
		KillTimer(hWnd, IDT_TIMER1);
		timerRunning = false;
	}
}

void createTimer(int timeout)
{
	if (timerRunning) destroyTimer();
	SetTimer(hWnd, IDT_TIMER1, timeout, (TIMERPROC)NULL);
	timerRunning = true;
}

static int l_sendKey(lua_State* L)
{
	int code = (int) lua_tointeger(L, 1);
	sendInput(code, KEYEVENTF_EXTENDEDKEY);
	Sleep(45);
	sendInput(code, KEYEVENTF_KEYUP);
	Sleep(45);
	return 0;
}

static int l_sendChar(lua_State* L)
{
	const char* str = lua_tostring(L, 1);
	char c = str[0];
	HKL kbl = GetKeyboardLayout(0);
	WORD code = VkKeyScanExA(c, kbl);
	sendInput(code, KEYEVENTF_EXTENDEDKEY);
	Sleep(50);
	sendInput(code, KEYEVENTF_KEYUP);
	Sleep(50);
	return 0;
}

static int l_timeout(lua_State* L)
{
	int timeout = (int)lua_tointeger(L, 1);
	createTimer(timeout);
	return 0;
}

static int l_exit(lua_State* L)
{
	exit(0);
}

int callLua(const char* function, int code, int time)
{
	lua_getglobal(L, function);
	if (lua_isnoneornil(L, -1)) {
		lua_pop(L, 1);
		return 0;
	}
	lua_pushinteger(L, code);
	lua_pushinteger(L, time);
	if (lua_pcall(L, 2, 1, 0) != 0) {
		error(lua_tostring(L, -1));
	}
	if (!lua_isboolean(L, -1)) {
		char buf[256];
		sprintf_s(buf, "%s must return a boolean", function);
		error(buf);
	}
	int result = (int) lua_toboolean(L, -1);
	lua_pop(L, 1);
	return result;
}

LRESULT CALLBACK lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		switch (wParam)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			KBDLLHOOKSTRUCT* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
			if ((kbd->flags & LLKHF_INJECTED) == 0) {
				if (callLua("onKeyPressed", kbd->vkCode, kbd->time)) return 1;
			}
			break;
		}
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			KBDLLHOOKSTRUCT* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
			if ((kbd->flags & LLKHF_INJECTED) == 0) {
				if (callLua("onKeyReleased", kbd->vkCode, kbd->time)) return 1;
			}
			break;
		}
		default:
			break;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	// register keyboard hook
	HHOOK kbHookHandle = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)lowLevelKeyboardProc, NULL, 0);

	if (kbHookHandle == NULL) {
		error("keyboard hook failed");
	}

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_LUAKEYBOARDHELPER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	// create Lua interpreter and load libs
	L = luaL_newstate();
	luaL_openlibs(L);

	// register sendKey function
	lua_pushcfunction(L, l_sendKey);
	lua_setglobal(L, "sendKey");

	// register sendChar function
	lua_pushcfunction(L, l_sendChar);
	lua_setglobal(L, "sendChar");

	// register timeout function
	lua_pushcfunction(L, l_timeout);
	lua_setglobal(L, "timeout");

	// register exit function
	lua_pushcfunction(L, l_exit);
	lua_setglobal(L, "exit");

	// run script
	int s = luaL_loadfile(L, "script.lua");
	if (s == 0) {
		s = lua_pcall(L, 0, LUA_MULTRET, 0);
	}
	if (s) {
		error(lua_tostring(L, -1));
	}

	//lua_close(L);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LUAKEYBOARDHELPER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
		if (msg.message == WM_TIMER && msg.wParam == IDT_TIMER1)
		{
			destroyTimer();
			lua_getglobal(L, "onTimeout");
			if (lua_isnoneornil(L, -1)) {
				lua_pop(L, 1);
			}
			else {
				if (lua_pcall(L, 0, 0, 0) != 0) {
					error(lua_tostring(L, -1));
				}
			}
		}
		
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LUAKEYBOARDHELPER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_LUAKEYBOARDHELPER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
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
