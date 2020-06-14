#include "pch.h"

using namespace Log;

namespace Window
{
	
}

using namespace Window;

static GetWindowRect_T GetWindowRect_O;
static GetClientRect_T GetClientRect_O;
static SetWindowPos_T SetWindowPos_O = nullptr;

static HWND gameWindow = nullptr;

static bool GetWPos(HWND hWnd, LPRECT lpRect)
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

static BOOL GetWindowRect_Hook(
	HWND   hWnd,
	LPRECT lpRect
)
{
	if (gameWindow != nullptr && hWnd == gameWindow) {
		if (GetWPos(hWnd, lpRect)) {
			return TRUE;
		}
	}

	return GetWindowRect_O(hWnd, lpRect);
}

static BOOL GetClientRect_Hook(
	HWND   hWnd,
	LPRECT lpRect
)
{
	if (gameWindow != nullptr && hWnd == gameWindow) {
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


static volatile LONG isWindowHooked = 0;

static void HookWindow(HMODULE hModule)
{
	if (InterlockedCompareExchangeAcquire(&isWindowHooked, 1, 0)) {
		return;
	}

	auto hookIf = pHandle->GetHookInterface();

	RegisterDetour(hookIf, hModule, GetWindowRect_O, GetWindowRect_Hook, "GetWindowRect");
	RegisterDetour(hookIf, hModule, GetClientRect_O, GetClientRect_Hook, "GetClientRect");
	RegisterDetour(hookIf, hModule, SetWindowPos_O, SetWindowPos_Hook, "SetWindowPos");

	IHookLogged(hookIf, &gLog).InstallHooks();
}

namespace Window
{
	void OnSwapChainCreate(HWND hWnd)
	{
		/*if (gameWindow == nullptr && hWnd != nullptr) {
			gameWindow = hWnd;
			MESSAGE(_T("Got game window: %p"),  hWnd);
			if (SetWindowPos_O != nullptr) {
				int X, Y, cx, cy;
				RECT pRect;
				if (GetWPos(hWnd, &pRect)) {
					X = pRect.left;
					Y = pRect.top;
					cx = pRect.right;
					cy = pRect.bottom;
				}
				SetWindowPos_O(hWnd, HWND_TOP, X, Y, cx, cy, SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS);
			}
		}*/
	}

	void InstallHooksIfLoaded()
	{
		//InstallHookIfLoaded(isWindowHooked, _T("user32.dll"), HookWindow);
	}
}