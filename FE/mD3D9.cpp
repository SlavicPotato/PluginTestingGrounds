#include "pch.h"

using namespace Microsoft::WRL;

using namespace std;
using namespace Log;

static Direct3DCreate9_T Direct3DCreate9_O;
static Direct3DCreate9Ex_T Direct3DCreate9Ex_O;
static CreateDeviceEx_T CreateDeviceEx_O;
static CreateDevice_T CreateDevice_O;
//static SwapChain_Present_T SwapChain_Present_O;
static Device_Present_T Device_Present_O;
static Device_PresentEx_T Device_PresentEx_O;
static CreateAdditionalSwapChain_T CreateAdditionalSwapChain_O;
static Reset_T Reset_O;
static ResetEx_T ResetEx_O;
static SetMaximumFrameLatency9_T SetMaximumFrameLatency_O;

namespace D3D9
{
    bool PresentIntervalImmediate = false;
    bool EnableFlip = false;
    int32_t BufferCount = -1;
    extern int32_t MaxFrameLatency = -1;

    bool HasFormat = false;
    bool FormatAuto = false;
    D3DFORMAT Format;
}

using namespace D3D9;

static UINT dwPresentFlags = 0;
static D3DPRESENT_PARAMETERS presentParamsCache;

/*static HRESULT STDMETHODCALLTYPE Present_Hook(
    IDirect3DSwapChain9* This,
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    HWND hDestWindowOverride,
    CONST RGNDATA* pDirtyRegion,
    DWORD dwFlags)
{
    MESSAGE(_T("Present_Hook"));

    return SwapChain_Present_O(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

static void InstallD3DSwapChainVFTHooks(IDirect3DSwapChain9 *pSwapChain)
{
    bool r1 = IHook::DetourVTable(pSwapChain, 0x3, Present_Hook, &SwapChain_Present_O);

    MESSAGE(_T("IDirect3DSwapChain9 vfuncs (%d)"),r1);
}

HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain_Hook(
    IDirect3DDevice9Ex* This,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DSwapChain9** pSwapChain)
{
    auto hr = CreateAdditionalSwapChain_O(This, pPresentationParameters, pSwapChain);
    if (hr == S_OK) {
        MESSAGE(_T("CreateAdditionalSwapChain succeeded"));

        InstallD3DSwapChainVFTHooks(*pSwapChain);
    }
    else {
        MESSAGE(_T("CreateAdditionalSwapChain failed (0x%lX)"), hr);
    }

    return hr;
}*/

void DumpD3DPresentParams(
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode)
{
    MESSAGE(_T("%s: SwapEffect: %d, BufferCount: %u, Flags: %lX, PresentationInterval: 0x%X, Windowed: %d"),
        _T(__FUNCTION__),
        pPresentationParameters->SwapEffect,
        pPresentationParameters->BackBufferCount,
        pPresentationParameters->Flags,
        pPresentationParameters->PresentationInterval,
        pPresentationParameters->Windowed);

    if (pFullscreenDisplayMode != nullptr) {
        MESSAGE(_T("%s: %ux%u, Format: %d, RefreshRate: %u"),
            _T(__FUNCTION__),
            pFullscreenDisplayMode->Width,
            pFullscreenDisplayMode->Height,
            pFullscreenDisplayMode->Format,
            pFullscreenDisplayMode->RefreshRate);
    }
    else {
        MESSAGE(_T("%s: %ux%u, Format: %d, RefreshRate: %u"),
            _T(__FUNCTION__),
            pPresentationParameters->BackBufferWidth,
            pPresentationParameters->BackBufferHeight,
            pPresentationParameters->BackBufferFormat,
            pPresentationParameters->FullScreen_RefreshRateInHz);
    }

}

static D3DFORMAT GetFormat(
    D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    if (HasFormat) {
        if (FormatAuto) {
            if (EnableFlip) {
                return D3DFMT_X8R8G8B8;
            }
        }
        else {
            return Format;
        }
    }

    return pPresentationParameters->BackBufferFormat;
}

static void PopulateD3DDisplayModeEx(
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode)
{
    pFullscreenDisplayMode->Format = GetFormat(pPresentationParameters);
    pFullscreenDisplayMode->Width = pPresentationParameters->BackBufferWidth;
    pFullscreenDisplayMode->Height = pPresentationParameters->BackBufferHeight;
    pFullscreenDisplayMode->RefreshRate = pPresentationParameters->FullScreen_RefreshRateInHz;
    pFullscreenDisplayMode->ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
    pFullscreenDisplayMode->Size = sizeof(D3DDISPLAYMODEEX);
}

static void PopulateD3DPresentParams(
    D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    if (BufferCount > -1) {
        pPresentationParameters->BackBufferCount = static_cast<UINT>(BufferCount);
    }

    if (EnableFlip) {
        pPresentationParameters->SwapEffect = D3DSWAPEFFECT_FLIPEX;

        if (PresentIntervalImmediate) {
            pPresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
            if (pPresentationParameters->Windowed == TRUE) {
                dwPresentFlags |= D3DPRESENT_FORCEIMMEDIATE;
            }
            else {
                dwPresentFlags &= ~D3DPRESENT_FORCEIMMEDIATE;
            }
        }
    }

    pPresentationParameters->BackBufferFormat = GetFormat(pPresentationParameters);

}

static void D3DPresentParamsCopyOnReturn(
    D3DPRESENT_PARAMETERS* pPresentationParametersL,
    D3DPRESENT_PARAMETERS* pPresentationParametersR)
{
    pPresentationParametersR->BackBufferCount = pPresentationParametersL->BackBufferCount;
    pPresentationParametersR->BackBufferWidth = pPresentationParametersL->BackBufferWidth;
    pPresentationParametersR->BackBufferHeight = pPresentationParametersL->BackBufferHeight;
    pPresentationParametersR->BackBufferFormat = pPresentationParametersL->BackBufferFormat;
}

static HRESULT STDMETHODCALLTYPE Device_PresentEx_Hook(
    IDirect3DDevice9Ex* This,
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    HWND hDestWindowOverride,
    CONST RGNDATA* pDirtyRegion,
    DWORD dwFlags)
{
    return Device_PresentEx_O(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags | dwPresentFlags);
}

static HRESULT STDMETHODCALLTYPE Device_Present_Hook(
    IDirect3DDevice9Ex* This,
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    HWND hDestWindowOverride,
    CONST RGNDATA* pDirtyRegion)
{
    return This->PresentEx(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, 0);
}

static HRESULT STDMETHODCALLTYPE Reset_Hook(
    IDirect3DDevice9Ex* This,
    D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    D3DDISPLAYMODEEX fsd, *pFullscreenDisplayMode;

    if (pPresentationParameters->Windowed == FALSE) {
        PopulateD3DDisplayModeEx(pPresentationParameters, &fsd);
        pFullscreenDisplayMode = &fsd;
    }
    else {
        pFullscreenDisplayMode = nullptr;
    }

    MESSAGE(_T("Redirecting Reset->ResetEx"));

    return This->ResetEx(pPresentationParameters, pFullscreenDisplayMode);
}

static HRESULT STDMETHODCALLTYPE ResetEx_Hook(
    IDirect3DDevice9Ex* This,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode)
{
    D3DPRESENT_PARAMETERS pp = *pPresentationParameters;
    PopulateD3DPresentParams(&pp);
    DumpD3DPresentParams(&pp, pFullscreenDisplayMode);

    auto hr = ResetEx_O(This, &pp, pFullscreenDisplayMode);
    if (hr == S_OK) {
        D3DPresentParamsCopyOnReturn(&pp, pPresentationParameters);
        MESSAGE(_T("IDirect3DDevice9Ex->ResetEx succeeded"));
    }
    else {
        MESSAGE(_T("IDirect3DDevice9Ex->ResetEx failed (0x%lX)"), hr);
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency_Hook(
    IDirect3DDevice9Ex* This,
    UINT MaxLatency)
{
    if (MaxFrameLatency > -1) {
        MaxLatency = static_cast<UINT>(MaxFrameLatency);
    }

    auto hr = SetMaximumFrameLatency_O(This, MaxLatency);
    if (hr == S_OK) {
        MESSAGE(_T("SetMaximumFrameLatency(%u) succeeded"), MaxLatency);
    }
    else {
        MESSAGE(_T("SetMaximumFrameLatency failed (0x%lX)"), hr);
    }
    return hr;
}

static void InstallD3DDeviceExVFTHooks(IDirect3DDevice9Ex* pDevice)
{
    /*bool r2 = IHook::DetourVTable(pDevice, 0xD, CreateAdditionalSwapChain_Hook, &CreateAdditionalSwapChain_O);

    MESSAGE(_T("IDirect3DDevice9Ex vfuncs (%d)"), r2);

    UINT numSwapChains = pDevice->GetNumberOfSwapChains();

    for (UINT i = 0; i < numSwapChains; i++) {
        IDirect3DSwapChain9* ppSwapChain;

        auto hr = pDevice->GetSwapChain(i, &ppSwapChain);
        if (hr == S_OK) {
            InstallD3DSwapChainVFTHooks(ppSwapChain);
        }
        else {
            MESSAGE(_T("IDirect3DSwapChain9 (index: %u): pDevice->GetSwapChain failed (0x%lX)"), i, hr);
            break;
        }
    }*/

    bool r1 = IHook::DetourVTable(pDevice, 0x11, Device_Present_Hook, &Device_Present_O);
    bool r2 = IHook::DetourVTable(pDevice, 0x79, Device_PresentEx_Hook, &Device_PresentEx_O);
    bool r3 = IHook::DetourVTable(pDevice, 0x10, Reset_Hook, &Reset_O);
    bool r4 = IHook::DetourVTable(pDevice, 0x84, ResetEx_Hook, &ResetEx_O);
    bool r5 = IHook::DetourVTable(pDevice, 0x7E, SetMaximumFrameLatency_Hook, &SetMaximumFrameLatency_O);

    MESSAGE(_T("IDirect3DDevice9 vfuncs (%d %d %d %d)"), r1, r2, r3, r4);
}

static HRESULT STDMETHODCALLTYPE CreateDeviceEx_Hook(
    IDirect3D9Ex* This,
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode,
    IDirect3DDevice9Ex** ppReturnedDeviceInterface)
{
    D3DPRESENT_PARAMETERS pp = *pPresentationParameters;
    PopulateD3DPresentParams(&pp);
    DumpD3DPresentParams(&pp, pFullscreenDisplayMode);

    auto hr = CreateDeviceEx_O(This, Adapter, DeviceType, hFocusWindow, BehaviorFlags, &pp, pFullscreenDisplayMode, ppReturnedDeviceInterface);
    if (hr == S_OK) {
        MESSAGE(_T("CreateDeviceEx succeeded"));

        D3DPresentParamsCopyOnReturn(&pp, pPresentationParameters);
        InstallD3DDeviceExVFTHooks(*ppReturnedDeviceInterface);

        if (MaxFrameLatency > -1) {
            (*ppReturnedDeviceInterface)->SetMaximumFrameLatency(
                static_cast<UINT>(MaxFrameLatency));
        }
    }
    else {
        MESSAGE(_T("CreateDeviceEx failed (0x%lX)"), hr);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE CreateDevice_Hook(
    IDirect3D9Ex* This,
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DDevice9Ex** ppReturnedDeviceInterface)
{
    D3DDISPLAYMODEEX* pFullscreenDisplayMode;
    D3DDISPLAYMODEEX fsd;

    if (pPresentationParameters->Windowed == FALSE) {
        PopulateD3DDisplayModeEx(pPresentationParameters, &fsd);
        pFullscreenDisplayMode = &fsd;
    }
    else {
        pFullscreenDisplayMode = nullptr;
    }

    MESSAGE(_T("Redirecting CreateDevice->CreateDeviceEx"));

    return This->CreateDeviceEx(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, ppReturnedDeviceInterface);
}


static void InstallD3DCTXExVFTHooks(IDirect3D9Ex* ctx)
{
    bool r1 = IHook::DetourVTable(ctx, 0x14, CreateDeviceEx_Hook, &CreateDeviceEx_O);
    bool r2 = IHook::DetourVTable(ctx, 0x10, CreateDevice_Hook, &CreateDevice_O);


    MESSAGE(_T("IDirect3D9Ex vfuncs: (%d %d)"), r1, r2);
}

static HRESULT WINAPI Direct3DCreate9Ex_Hook(UINT SDKVersion, IDirect3D9Ex** ctx)
{
    auto hr = Direct3DCreate9Ex_O(SDKVersion, ctx);
    if (hr == S_OK) {
        MESSAGE(_T("Direct3DCreate9Ex succeeded"));

        InstallD3DCTXExVFTHooks(*ctx);
    }
    else {
        MESSAGE(_T("Direct3DCreate9Ex failed (0x%lX)"), hr);
    }

    return hr;
}

static void InstallD3DCTXVFTHooks(IDirect3D9* ctx)
{
    bool r1 = IHook::DetourVTable(ctx, 0x14, CreateDevice_Hook, &CreateDevice_O);

    MESSAGE(_T("IDirect3D9 vfuncs: (%d)"), r1);
}

static IDirect3D9* WINAPI Direct3DCreate9_Hook(UINT SDKVersion)
{
    IDirect3D9Ex* ctx;

    MESSAGE(_T("Redirecting Direct3DCreate9->Direct3DCreate9Ex"));

    auto hr = Direct3DCreate9Ex_Hook(SDKVersion, &ctx);
    if (hr == S_OK) {
        return reinterpret_cast<IDirect3D9*>(ctx);
    }
    else {
        return nullptr;
    }
}

static volatile LONG isD3D9Hooked = 0;

static void HookD3D9(HMODULE hModule)
{
    if (InterlockedCompareExchangeAcquire(&isD3D9Hooked, 1, 0)) {
        return;
    }

    /*auto hookIf = pHandle->GetHookInterface();

    IHookLogged(hookIf, &gLog).InstallHooks();*/
}

namespace D3D9
{
    void InstallHooksIfLoaded()
    {
        //InstallHookIfLoaded(isD3D9Hooked, _T("d3d9.dll"), HookD3D9);
    }

    using namespace Plugin;

    void InstallProxyOverrides(IPlugin* h)
    {
        h->SetProxyOverride(
            Misc::Underlying(ProxyD3D9::Index::kDirect3DCreate9Ex),
            reinterpret_cast<FARPROC>(Direct3DCreate9Ex_Hook),
            reinterpret_cast<void**>(&Direct3DCreate9Ex_O));

        h->SetProxyOverride(
            Misc::Underlying(ProxyD3D9::Index::kDirect3DCreate9),
            reinterpret_cast<FARPROC>(Direct3DCreate9_Hook),
            reinterpret_cast<void**>(&Direct3DCreate9_O));
    }
}
