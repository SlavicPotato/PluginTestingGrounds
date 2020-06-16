#include "pch.h"

#include "hpfe.cpp"

using namespace Log;

namespace Window
{
	bool BorderlessUpscaling = false;
	bool ForceBorderless = false;
}

using namespace Window;

static GetWindowRect_T GetWindowRect_O;
static GetClientRect_T GetClientRect_O;
static SetWindowPos_T SetWindowPos_O;
static CreateWindowExA_T CreateWindowExA_O;

static HWND gameWindow = nullptr;

static bool WINAPI GetWPos(HWND hWnd, LPRECT lpRect)
{
	HMONITOR hMonitor = MonitorFromWindow(
		hWnd, MONITOR_DEFAULTTOPRIMARY);

	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfoA(hMonitor, &mi))
	{
		return false;
	}

	lpRect->left = mi.rcMonitor.left;
	lpRect->top = mi.rcMonitor.top;
	lpRect->right = mi.rcMonitor.right - mi.rcMonitor.left;
	lpRect->bottom = mi.rcMonitor.bottom - mi.rcMonitor.top;

	return true;
}

static BOOL WINAPI GetWindowRect_Hook(
	HWND   hWnd,
	LPRECT lpRect
)
{
	if (gameWindow != nullptr && hWnd == gameWindow) 
	{
		if (GetWPos(hWnd, lpRect)) {
			return TRUE;
		}
	}

	return GetWindowRect_O(hWnd, lpRect);
}

static BOOL WINAPI GetClientRect_Hook(
	HWND   hWnd,
	LPRECT lpRect
)
{
	if (gameWindow != nullptr && hWnd == gameWindow) 
	{
		if (GetWPos(hWnd, lpRect)) {
			return TRUE;
		}
	}

	return GetClientRect_O(hWnd, lpRect);
}

static BOOL
WINAPI
SetWindowPos_Hook(
	_In_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_ int X,
	_In_ int Y,
	_In_ int cx,
	_In_ int cy,
	_In_ UINT uFlags)
{
	if (gameWindow != nullptr && hWnd == gameWindow) {
		RECT pRect;
		if (GetWPos(hWnd, &pRect)) {
			X = pRect.left;
			Y = pRect.top;
			cx = pRect.right;
			cy = pRect.bottom;
		}
	}

	return SetWindowPos_O(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

static HWND WINAPI CreateWindowExA_Hook(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCSTR lpClassName,
	_In_opt_ LPCSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam)
{
	
	return CreateWindowExA_O(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}


static volatile LONG isWindowHooked = 0;

static void HookWindow(HMODULE hModule)
{
	if (InterlockedCompareExchangeAcquire(&isWindowHooked, 1, 0)) {
		return;
	}

	auto hookIf = pHandle->GetHookInterface();

	if (BorderlessUpscaling) {
		RegisterDetour(hookIf, hModule, GetWindowRect_O, GetWindowRect_Hook, "GetWindowRect");
		RegisterDetour(hookIf, hModule, GetClientRect_O, GetClientRect_Hook, "GetClientRect");
		RegisterDetour(hookIf, hModule, SetWindowPos_O, SetWindowPos_Hook, "SetWindowPos");
	}

	if (ForceBorderless) {
		RegisterDetour(hookIf, hModule, CreateWindowExA_O, CreateWindowExA_Hook, "CreateWindowExA");
	}

	IHookLogged(hookIf, &gLog).InstallHooks();
}

static void RepositionBorderlessWindow(HWND hWnd)
{
	int X, Y, cx, cy;
	RECT pRect;
	if (GetWPos(hWnd, &pRect)) {
		X = pRect.left;
		Y = pRect.top;
		cx = pRect.right;
		cy = pRect.bottom;
	}
	if (SetWindowPos_O != nullptr) {
		SetWindowPos_O(hWnd, HWND_TOP, X, Y, cx, cy, SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS);
	}
	else {
		SetWindowPos(hWnd, HWND_TOP, X, Y, cx, cy, SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS);
	}
}

namespace Window
{
	void OnSwapChainCreate(HWND hWnd)
	{
		if (gameWindow != nullptr || hWnd == nullptr) {
			return;
		}

		MESSAGE(_T("Got game window: %p"), hWnd);

		if (ForceBorderless) 
		{
			LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
			if (!(style & WS_POPUP)) 
			{
				MESSAGE(_T("%p: Borderless fullscreen requested but window doesn't have WS_POPUP style, attempting to fix.."), hWnd);

				style &= ~WS_OVERLAPPEDWINDOW;
				style |= WS_POPUP;
				SetWindowLongPtrA(hWnd, GWL_STYLE, style);
				RepositionBorderlessWindow(hWnd);
			}
		}

		if (BorderlessUpscaling) {
			gameWindow = hWnd;
			RepositionBorderlessWindow(hWnd);
		}
	}

	void InstallHooksIfLoaded()
	{
		InstallHookIfLoaded(isWindowHooked, _T("user32.dll"), HookWindow);
	}
}