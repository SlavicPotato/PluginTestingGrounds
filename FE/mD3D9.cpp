#include "pch.h"

#include "d3d9/d3d9.h"

using namespace Microsoft::WRL;

using namespace std;
using namespace Log;


static Direct3DCreate9_T Direct3DCreate9_O;
static Direct3DCreate9Ex_T Direct3DCreate9Ex_O;


static volatile LONG isD3D9Hooked = 0;

static void HookD3D9(HMODULE hModule)
{
    if (InterlockedCompareExchangeAcquire(&isD3D9Hooked, 1, 0)) {
        return;
    }

    /*auto hookIf = pHandle->GetHookInterface();

    IHookLogged(hookIf, &gLog).InstallHooks();*/
}

static HRESULT WINAPI Direct3DCreate9Ex_Hook(UINT SDKVersion, IDirect3D9Ex** out)
{
    IDirect3D9Ex* ctx;

    auto hr = Direct3DCreate9Ex_O(SDKVersion, &ctx);
    if (SUCCEEDED(hr)) {
        MESSAGE("Direct3DCreate9Ex succeeded (%u)", SDKVersion);

        *out = new m_IDirect3D9Ex(ctx, IID_IDirect3D9Ex);
    }
    else {
        MESSAGE("Direct3DCreate9Ex failed (0x%lX)", hr);
    }

    return hr;
}

static IDirect3D9* WINAPI Direct3DCreate9_Hook(UINT SDKVersion)
{
    MESSAGE("Redirecting Direct3DCreate9->Direct3DCreate9Ex");

    IDirect3D9Ex* ctx;

    auto hr = Direct3DCreate9Ex_Hook(SDKVersion, &ctx);
    if (hr == S_OK) {
        return reinterpret_cast<IDirect3D9*>(ctx);
    }
    else {
        return nullptr;
    }
}

namespace D3D9
{
    bool PresentIntervalImmediate = false;
    bool EnableFlip = false;
    int32_t BufferCount = -1;
    extern int32_t MaxFrameLatency = -1;

    bool HasFormat = false;
    bool FormatAuto = false;
    D3DFORMAT D3DFormat;

    bool CreateTextureUsageDynamic;
    UINT CreateTextureClearUsageFlags;

    bool CreateIndexBufferUsageDynamic;
    bool CreateVertexBufferUsageDynamic;

    bool CreateCubeTextureUsageDynamic;
    bool CreateVolumeTextureUsageDynamic;

    int32_t ForceAdapter;
    int32_t DisplayMode;

    UINT dwPresentFlags = 0;

    void DumpD3DPresentParams(
        D3DPRESENT_PARAMETERS* pPresentationParameters,
        D3DDISPLAYMODEEX* pFullscreenDisplayMode)
    {
        MESSAGE("SwapEffect: %d, BufferCount: %u, Flags: %lX, PresentationInterval: 0x%X, Windowed: %d, Fs_RR: %u",
            pPresentationParameters->SwapEffect,
            pPresentationParameters->BackBufferCount,
            pPresentationParameters->Flags,
            pPresentationParameters->PresentationInterval,
            pPresentationParameters->Windowed,
            pPresentationParameters->FullScreen_RefreshRateInHz);

        if (pFullscreenDisplayMode != nullptr) {
            MESSAGE("D3DDISPLAYMODEEX: %ux%u, Format: %d, RefreshRate: %u",
                pFullscreenDisplayMode->Width,
                pFullscreenDisplayMode->Height,
                pFullscreenDisplayMode->Format,
                pFullscreenDisplayMode->RefreshRate);
        }
        else {
            MESSAGE("%ux%u, Format: %d, RefreshRate: %u",
                pPresentationParameters->BackBufferWidth,
                pPresentationParameters->BackBufferHeight,
                pPresentationParameters->BackBufferFormat,
                pPresentationParameters->FullScreen_RefreshRateInHz);
        }
    }

    D3DFORMAT GetD3DFormat(D3DFORMAT _format)
    {
        if (HasFormat) {
            if (FormatAuto) {
                if (EnableFlip) {
                    return D3DFMT_X8R8G8B8;
                }
            }
            else {
                return D3DFormat;
            }
        }

        return _format;
    }

    void PopulateD3DDisplayModeEx(
        D3DPRESENT_PARAMETERS* pPresentationParameters,
        D3DDISPLAYMODEEX* pFullscreenDisplayMode)
    {
        pFullscreenDisplayMode->Format = GetD3DFormat(pPresentationParameters->BackBufferFormat);
        pFullscreenDisplayMode->Width = pPresentationParameters->BackBufferWidth;
        pFullscreenDisplayMode->Height = pPresentationParameters->BackBufferHeight;
        pFullscreenDisplayMode->RefreshRate = pPresentationParameters->FullScreen_RefreshRateInHz;
        pFullscreenDisplayMode->ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
        pFullscreenDisplayMode->Size = sizeof(D3DDISPLAYMODEEX);
    }

    void PopulateD3DPresentParams(
        D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        if (BufferCount > -1) {
            pPresentationParameters->BackBufferCount = static_cast<UINT>(BufferCount);
        }

        if (EnableFlip) {
            pPresentationParameters->SwapEffect = D3DSWAPEFFECT_FLIPEX;

            pPresentationParameters->MultiSampleQuality = 0;
            pPresentationParameters->MultiSampleType = D3DMULTISAMPLE_NONE;
            pPresentationParameters->Flags &= ~(D3DPRESENTFLAG_LOCKABLE_BACKBUFFER);

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

        pPresentationParameters->BackBufferFormat = GetD3DFormat(pPresentationParameters->BackBufferFormat);

        if (DisplayMode > -1) {
            pPresentationParameters->Windowed = static_cast<BOOL>(DisplayMode);
        }
    }

    void D3DPresentParamsCopyOnReturn(
        D3DPRESENT_PARAMETERS* pPresentationParametersL,
        D3DPRESENT_PARAMETERS* pPresentationParametersR)
    {
        pPresentationParametersR->BackBufferCount = pPresentationParametersL->BackBufferCount;
        pPresentationParametersR->BackBufferWidth = pPresentationParametersL->BackBufferWidth;
        pPresentationParametersR->BackBufferHeight = pPresentationParametersL->BackBufferHeight;
        pPresentationParametersR->BackBufferFormat = pPresentationParametersL->BackBufferFormat;
    }

    
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
