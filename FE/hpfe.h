#pragma once

typedef FARPROC(WINAPI* GetProcAddress_T)(
	_In_ HMODULE hModule,
	_In_ LPCSTR lpProcName);

typedef HMODULE
(WINAPI* LoadLibraryA_T)(
	_In_ LPCSTR lpLibFileName);

typedef HMODULE
(WINAPI* LoadLibraryW_T)(
	_In_ LPCWSTR lpLibFileName);

typedef HMODULE
(WINAPI* LoadLibraryExA_T)(
	_In_ LPCSTR lpLibFileName,
	_Reserved_ HANDLE hFile,
	_In_ DWORD dwFlags);

typedef HMODULE
(WINAPI* LoadLibraryExW_T)(
	_In_ LPCWSTR lpLibFileName,
	_Reserved_ HANDLE hFile,
	_In_ DWORD dwFlags);

//typedef IDirect3D9* (WINAPI *Direct3DCreate9_T)(UINT SDKVersion);
//typedef HRESULT(WINAPI* Direct3DCreate9Ex_T) (UINT SDKVersion, IDirect3D9Ex**);

extern IPlugin* pHandle;

template <typename T>
void RegisterDetour(IScoped<IHook>& hookIf, HMODULE hModule, T& orig, PVOID func, const char* fn);

typedef void (*InstallHook_T)(HMODULE);

__inline
void InstallHookIfLoaded(volatile LONG& cond, const TCHAR* module, InstallHook_T func)
{
	if (!cond) {
		HMODULE hModule = GetModuleHandle(module);
		if (hModule != nullptr) {
			func(hModule);
		}
	}
}
