#pragma once

#include <d3d9.h>

typedef IDirect3D9* (WINAPI* Direct3DCreate9_T)(UINT SDKVersion);
typedef HRESULT(WINAPI* Direct3DCreate9Ex_T)(UINT SDKVersion, IDirect3D9Ex**);

typedef HRESULT (STDMETHODCALLTYPE *CreateDeviceEx_T)(
	IDirect3D9Ex* This, 
	UINT Adapter, 
	D3DDEVTYPE DeviceType, 
	HWND hFocusWindow, 
	DWORD BehaviorFlags, 
	D3DPRESENT_PARAMETERS* pPresentationParameters, 
	D3DDISPLAYMODEEX* pFullscreenDisplayMode, 
	IDirect3DDevice9Ex** ppReturnedDeviceInterface);

typedef HRESULT(STDMETHODCALLTYPE* CreateDevice_T)(
	IDirect3D9Ex* This,
	UINT Adapter,
	D3DDEVTYPE DeviceType,
	HWND hFocusWindow,
	DWORD BehaviorFlags,
	D3DPRESENT_PARAMETERS* pPresentationParameters,
	IDirect3DDevice9Ex** ppReturnedDeviceInterface);

/*typedef HRESULT (STDMETHODCALLTYPE *SwapChain_Present_T)(
	IDirect3DSwapChain9* This, 
	CONST RECT* pSourceRect, 
	CONST RECT* pDestRect, 
	HWND hDestWindowOverride, 
	CONST RGNDATA* pDirtyRegion, 
	DWORD dwFlags);*/

typedef HRESULT (STDMETHODCALLTYPE* Device_Present_T)(
	IDirect3DDevice9Ex *This, 
	CONST RECT* pSourceRect, 
	CONST RECT* pDestRect, 
	HWND hDestWindowOverride, 
	CONST RGNDATA* pDirtyRegion);

typedef HRESULT (STDMETHODCALLTYPE* Device_PresentEx_T)(
	IDirect3DDevice9Ex *This, 
	CONST RECT* pSourceRect, 
	CONST RECT* pDestRect, 
	HWND hDestWindowOverride, 
	CONST RGNDATA* pDirtyRegion,
	DWORD dwFlags);

typedef HRESULT (STDMETHODCALLTYPE* CreateAdditionalSwapChain_T)(
	IDirect3DDevice9Ex * This,
	D3DPRESENT_PARAMETERS* pPresentationParameters, 
	IDirect3DSwapChain9** pSwapChain);

typedef HRESULT(STDMETHODCALLTYPE* Reset_T)(
	IDirect3DDevice9Ex* This, 
	D3DPRESENT_PARAMETERS* pPresentationParameters);

typedef HRESULT(STDMETHODCALLTYPE* ResetEx_T)(
	IDirect3DDevice9Ex* This, 
	D3DPRESENT_PARAMETERS* pPresentationParameters,
	D3DDISPLAYMODEEX* pFullscreenDisplayMode);

typedef HRESULT(STDMETHODCALLTYPE* SetMaximumFrameLatency9_T)(
	IDirect3DDevice9Ex* This, 
	UINT MaxLatency);

namespace D3D9
{
	extern bool PresentIntervalImmediate;
	extern bool EnableFlip;
	extern int32_t BufferCount;
	extern int32_t MaxFrameLatency;

	extern bool HasFormat;
	extern bool FormatAuto;
	extern D3DFORMAT Format;

	void InstallHooksIfLoaded();
	void InstallProxyOverrides(IPlugin* h);
}