#pragma once

typedef BOOL
(WINAPI* GetWindowRect_T)(
	_In_ HWND hWnd,
	_Out_ LPRECT lpRect);

typedef BOOL
(WINAPI* GetClientRect_T)(
	_In_ HWND hWnd,
	_Out_ LPRECT lpRect);

typedef BOOL
(WINAPI *SetWindowPos_T)(
    _In_ HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_ int X,
    _In_ int Y,
    _In_ int cx,
    _In_ int cy,
    _In_ UINT uFlags);

namespace Window
{
    extern void OnSwapChainCreate(HWND hWnd);
	void InstallHooksIfLoaded();
}