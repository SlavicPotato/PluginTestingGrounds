#include "pch.h"

using namespace Microsoft::WRL;

using namespace std;
using namespace Log;

static Direct3DCreate9_T Direct3DCreate9_O;
static Direct3DCreate9Ex_T Direct3DCreate9Ex_O;

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
}

using namespace D3D9;

static UINT dwPresentFlags = 0;

void DumpD3DPresentParams(
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode)
{
    MESSAGE(_T("SwapEffect: %d, BufferCount: %u, Flags: %lX, PresentationInterval: 0x%X, Windowed: %d"),
        pPresentationParameters->SwapEffect,
        pPresentationParameters->BackBufferCount,
        pPresentationParameters->Flags,
        pPresentationParameters->PresentationInterval,
        pPresentationParameters->Windowed);

    if (pFullscreenDisplayMode != nullptr) {
        MESSAGE(_T("%ux%u, Format: %d, RefreshRate: %u"),
            pFullscreenDisplayMode->Width,
            pFullscreenDisplayMode->Height,
            pFullscreenDisplayMode->Format,
            pFullscreenDisplayMode->RefreshRate);
    }
    else {
        MESSAGE(_T("%ux%u, Format: %d, RefreshRate: %u"),
            pPresentationParameters->BackBufferWidth,
            pPresentationParameters->BackBufferHeight,
            pPresentationParameters->BackBufferFormat,
            pPresentationParameters->FullScreen_RefreshRateInHz);
    }

}

static D3DFORMAT GetD3DFormat(D3DFORMAT _format)
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

static void PopulateD3DDisplayModeEx(
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

static void PopulateD3DPresentParams(
    D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    if (BufferCount > -1) {
        pPresentationParameters->BackBufferCount = static_cast<UINT>(BufferCount);
    }

    if (EnableFlip) {
        pPresentationParameters->SwapEffect = D3DSWAPEFFECT_FLIPEX;

        pPresentationParameters->MultiSampleQuality = 0;
        pPresentationParameters->MultiSampleType = D3DMULTISAMPLE_NONE;

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

static void D3DPresentParamsCopyOnReturn(
    D3DPRESENT_PARAMETERS* pPresentationParametersL,
    D3DPRESENT_PARAMETERS* pPresentationParametersR)
{
    pPresentationParametersR->BackBufferCount = pPresentationParametersL->BackBufferCount;
    pPresentationParametersR->BackBufferWidth = pPresentationParametersL->BackBufferWidth;
    pPresentationParametersR->BackBufferHeight = pPresentationParametersL->BackBufferHeight;
    pPresentationParametersR->BackBufferFormat = pPresentationParametersL->BackBufferFormat;
}

static HRESULT WINAPI Direct3DCreate9Ex_Hook(UINT SDKVersion, IDirect3D9Ex** out)
{
    IDirect3D9Ex* ctx;

    auto hr = Direct3DCreate9Ex_O(SDKVersion, &ctx);
    if (SUCCEEDED(hr)) {
        MESSAGE(_T("Direct3DCreate9Ex succeeded (%u)"), SDKVersion);

        *out = new w_IDirect3D9Ex(ctx, IID_IDirect3D9Ex);

        //InstallD3DCTXExVFTHooks(*ctx);
    }
    else {
        MESSAGE(_T("Direct3DCreate9Ex failed (0x%lX)"), hr);
    }

    return hr;
}

static IDirect3D9* WINAPI Direct3DCreate9_Hook(UINT SDKVersion)
{
    MESSAGE(_T("Redirecting Direct3DCreate9->Direct3DCreate9Ex"));

    IDirect3D9Ex* ctx;

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

HRESULT w_IDirect3D9Ex::QueryInterface(REFIID riid, void** ppvObj)
{
    if ((riid == IID_IUnknown || riid == WrapperID) && ppvObj)
    {
        AddRef();

        *ppvObj = this;

        return S_OK;
    }

    return ProxyInterface->QueryInterface(riid, ppvObj);
}

ULONG w_IDirect3D9Ex::AddRef()
{
    return ProxyInterface->AddRef();
}

ULONG w_IDirect3D9Ex::Release()
{
    ULONG count = ProxyInterface->Release();

    if (count == 0)
    {
        delete this;
    }

    return count;
}

HRESULT w_IDirect3D9Ex::EnumAdapterModes(THIS_ UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->EnumAdapterModes(Adapter, Format, Mode, pMode);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED: %d"), Format);
    }
    else {
        //MESSAGE(_T("OK: %d %ux%u %u"), pMode->Format, pMode->Width, pMode->Height, pMode->RefreshRate);
    }

    return hr;
}

UINT w_IDirect3D9Ex::GetAdapterCount()
{
    return ProxyInterface->GetAdapterCount();
}

HRESULT w_IDirect3D9Ex::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->GetAdapterDisplayMode(Adapter, pMode);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED: %u"), Adapter);
    }
    else {
        //MESSAGE(_T("OK: %u %d %ux%u %u"), Adapter, pMode->Format, pMode->Width, pMode->Height, pMode->RefreshRate);
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->GetAdapterIdentifier(Adapter, Flags, pIdentifier);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

UINT w_IDirect3D9Ex::GetAdapterModeCount(THIS_ UINT Adapter, D3DFORMAT Format)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->GetAdapterModeCount(Adapter, Format);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED: %d"), Format);
    }
    else {
        //MESSAGE(_T("OK: %d"), Format);
    }

    return hr;
}

HMONITOR w_IDirect3D9Ex::GetAdapterMonitor(UINT Adapter)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    return ProxyInterface->GetAdapterMonitor(Adapter);
}

HRESULT w_IDirect3D9Ex::GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->GetDeviceCaps(Adapter, DeviceType, pCaps);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::RegisterSoftwareDevice(void* pInitializeFunction)
{
    HRESULT hr = ProxyInterface->RegisterSoftwareDevice(pInitializeFunction);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED: %u %d %d %lX %d %d"), Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::CheckDeviceMultiSampleType(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    if (DisplayMode > -1) {
        Windowed = static_cast<BOOL>(Windowed);
    }

    HRESULT hr = ProxyInterface->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::CheckDeviceType(UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    if (DisplayMode > -1) {
        Windowed = static_cast<BOOL>(Windowed);
    }

    HRESULT hr = ProxyInterface->CheckDeviceType(Adapter, CheckType, DisplayFormat, BackBufferFormat, Windowed);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED: %u %d %d %d %d"), Adapter, CheckType, DisplayFormat, BackBufferFormat, Windowed);
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::CheckDeviceFormatConversion(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->CheckDeviceFormatConversion(Adapter, DeviceType, SourceFormat, TargetFormat);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
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

    return this->CreateDeviceEx(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, (IDirect3DDevice9Ex**)ppReturnedDeviceInterface);
}

UINT w_IDirect3D9Ex::GetAdapterModeCountEx(THIS_ UINT Adapter, CONST D3DDISPLAYMODEFILTER* pFilter)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->GetAdapterModeCountEx(Adapter, pFilter);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::EnumAdapterModesEx(THIS_ UINT Adapter, CONST D3DDISPLAYMODEFILTER* pFilter, UINT Mode, D3DDISPLAYMODEEX* pMode)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->EnumAdapterModesEx(Adapter, pFilter, Mode, pMode);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }
    else {
        //MESSAGE(_T("OK: %d %ux%u %u"), pMode->Format, pMode->Width, pMode->Height, pMode->RefreshRate);
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::GetAdapterDisplayModeEx(THIS_ UINT Adapter, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->GetAdapterDisplayModeEx(Adapter, pMode, pRotation);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED: %u"), Adapter);
    }
    else {
        //MESSAGE(_T("OK: %u %d %ux%u %u"), Adapter, pMode->Format, pMode->Width, pMode->Height, pMode->RefreshRate);
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::CreateDeviceEx(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode, IDirect3DDevice9Ex** ppReturnedDeviceInterface)
{
    D3DPRESENT_PARAMETERS pp = *pPresentationParameters;
    PopulateD3DPresentParams(&pp);
    DumpD3DPresentParams(&pp, pFullscreenDisplayMode);

    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    if (pp.Windowed) {
        pFullscreenDisplayMode = nullptr;
    }

    MESSAGE("AutoDepthStencilFormat:%d EnableAutoDepthStencil:%d MultiSampleQuality:%lu MultiSampleType: %lu BackBufferFormat:%d BackBufferWidth:%u BackBufferHeight:%u hDeviceWindow:%p",
        pp.AutoDepthStencilFormat,
        pp.EnableAutoDepthStencil,
        pp.MultiSampleQuality,
        pp.MultiSampleType,
        pp.BackBufferFormat,
        pp.BackBufferWidth,
        pp.BackBufferHeight,
        pp.hDeviceWindow);

    MESSAGE("Adapter:%u DeviceType: %d BehaviorFlags: %lX hFocusWindow:%p",
        Adapter, DeviceType, BehaviorFlags, hFocusWindow, hFocusWindow);

    IDirect3DDevice9Ex* tmp;

    auto hr = ProxyInterface->CreateDeviceEx(Adapter, DeviceType, hFocusWindow, BehaviorFlags, &pp, pFullscreenDisplayMode, &tmp);
    if (SUCCEEDED(hr)) {
        MESSAGE(_T("Succeeded"));

        D3DPresentParamsCopyOnReturn(&pp, pPresentationParameters);

        auto* w = new w_IDirect3DDevice9Ex(tmp, this, IID_IDirect3DDevice9Ex);

        if (MaxFrameLatency > -1) {
            w->SetMaximumFrameLatency(
                static_cast<UINT>(MaxFrameLatency));
        }

        Window::OnSwapChainCreate(pPresentationParameters->hDeviceWindow);

        *ppReturnedDeviceInterface = w;
    }
    else {
        MESSAGE(_T("Failed (0x%lX)"), hr);
    }

    return hr;
}

HRESULT w_IDirect3D9Ex::GetAdapterLUID(THIS_ UINT Adapter, LUID* pLUID)
{
    if (ForceAdapter > -1) {
        Adapter = static_cast<UINT>(ForceAdapter);
    }

    HRESULT hr = ProxyInterface->GetAdapterLUID(Adapter, pLUID);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT w_IDirect3DDevice9Ex::QueryInterface(REFIID riid, void** ppvObj)
{
    if ((riid == IID_IUnknown || riid == WrapperID) && ppvObj)
    {
        AddRef();

        *ppvObj = this;

        return S_OK;
    }

    return Proxy->QueryInterface(riid, ppvObj);
}

ULONG w_IDirect3DDevice9Ex::AddRef()
{
    return Proxy->AddRef();
}

ULONG w_IDirect3DDevice9Ex::Release()
{
    ULONG count = Proxy->Release();

    if (count == 0)
    {
        delete this;
    }

    return count;
}
HRESULT w_IDirect3DDevice9Ex::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    D3DDISPLAYMODEEX fsd, * pFullscreenDisplayMode;

    if (pPresentationParameters->Windowed == FALSE) {
        PopulateD3DDisplayModeEx(pPresentationParameters, &fsd);
        pFullscreenDisplayMode = &fsd;
    }
    else {
        pFullscreenDisplayMode = nullptr;
    }

    MESSAGE(_T("Redirecting Reset->ResetEx"));
    return this->ResetEx(pPresentationParameters, pFullscreenDisplayMode);
}

HRESULT w_IDirect3DDevice9Ex::EndScene()
{
    HRESULT hr = Proxy->EndScene();

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

void w_IDirect3DDevice9Ex::SetCursorPosition(int X, int Y, DWORD Flags)
{
    return Proxy->SetCursorPosition(X, Y, Flags);
}

HRESULT w_IDirect3DDevice9Ex::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap)
{
    HRESULT hr = Proxy->SetCursorProperties(XHotSpot, YHotSpot, pCursorBitmap);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

BOOL w_IDirect3DDevice9Ex::ShowCursor(BOOL bShow)
{
    HRESULT hr = Proxy->ShowCursor(bShow);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** ppSwapChain)
{
    HRESULT hr = Proxy->CreateAdditionalSwapChain(pPresentationParameters, ppSwapChain);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateCubeTexture(THIS_ UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle)
{
    if (Pool == D3DPOOL_MANAGED) {
        Pool = D3DPOOL_DEFAULT;
    }

    if (!Usage && CreateCubeTextureUsageDynamic) {
        Usage = D3DUSAGE_DYNAMIC;
    }

    HRESULT hr = Proxy->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateDepthStencilSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    if (EnableFlip) {
        MultisampleQuality = 0;
        MultiSample = D3DMULTISAMPLE_NONE;
    }

    HRESULT hr = Proxy->CreateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateIndexBuffer(THIS_ UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle)
{
    if (Pool == D3DPOOL_MANAGED) {
        Pool = D3DPOOL_DEFAULT;
    }

    if (!Usage && CreateIndexBufferUsageDynamic) {
        Usage |= D3DUSAGE_DYNAMIC;
    }

    HRESULT hr = Proxy->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);

    if (!SUCCEEDED(hr))
    {
        MESSAGE("FAILED: %p %u %lu %d %d %p %p", Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateRenderTarget(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    if (EnableFlip) {
        MultisampleQuality = 0;
        MultiSample = D3DMULTISAMPLE_NONE;
    }

    HRESULT hr = Proxy->CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateTexture(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
    if (Pool == D3DPOOL_MANAGED) {
        Pool = D3DPOOL_DEFAULT;
    }

    if (!Usage && CreateTextureUsageDynamic) {
        Usage = D3DUSAGE_DYNAMIC;
    }

    Usage &= ~(CreateTextureClearUsageFlags);

    HRESULT hr = Proxy->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);

    if (!SUCCEEDED(hr))
    {
        MESSAGE("FAILED: %u %u %u %X %d %d %p %p", Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateVertexBuffer(THIS_ UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle)
{
    if (Pool == D3DPOOL_MANAGED) {
        Pool = D3DPOOL_DEFAULT;
    }

    if (!Usage && CreateVertexBufferUsageDynamic) {
        Usage |= D3DUSAGE_DYNAMIC;
    }

    HRESULT hr = Proxy->CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);

    if (!SUCCEEDED(hr))
    {
        MESSAGE("FAILED: %u %lu %lu %d %p %p", Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateVolumeTexture(THIS_ UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)
{
    if (Pool == D3DPOOL_MANAGED) {
        Pool = D3DPOOL_DEFAULT;
    }

    if (!Usage && CreateVolumeTextureUsageDynamic) {
        Usage = D3DUSAGE_DYNAMIC;
    }

    HRESULT hr = Proxy->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::BeginStateBlock()
{
    HRESULT hr = Proxy->BeginStateBlock();

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateStateBlock(THIS_ D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)
{
    HRESULT hr = Proxy->CreateStateBlock(Type, ppSB);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::EndStateBlock(THIS_ IDirect3DStateBlock9** ppSB)
{
    HRESULT hr = Proxy->EndStateBlock(ppSB);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetClipStatus(D3DCLIPSTATUS9* pClipStatus)
{
    HRESULT hr = Proxy->GetClipStatus(pClipStatus);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetDisplayMode(THIS_ UINT iSwapChain, D3DDISPLAYMODE* pMode)
{
    HRESULT hr = Proxy->GetDisplayMode(iSwapChain, pMode);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue)
{
    HRESULT hr = Proxy->GetRenderState(State, pValue);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetRenderTarget(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget)
{
    HRESULT hr = Proxy->GetRenderTarget(RenderTargetIndex, ppRenderTarget);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)
{
    HRESULT hr = Proxy->GetTransform(State, pMatrix);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus)
{
    HRESULT hr = Proxy->SetClipStatus(pClipStatus);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
    HRESULT hr = Proxy->SetRenderState(State, Value);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetRenderTarget(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
{
    HRESULT hr = Proxy->SetRenderTarget(RenderTargetIndex, pRenderTarget);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    HRESULT hr = Proxy->SetTransform(State, pMatrix);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

void w_IDirect3DDevice9Ex::GetGammaRamp(THIS_ UINT iSwapChain, D3DGAMMARAMP* pRamp)
{
    return Proxy->GetGammaRamp(iSwapChain, pRamp);
}

void w_IDirect3DDevice9Ex::SetGammaRamp(THIS_ UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp)
{
    return Proxy->SetGammaRamp(iSwapChain, Flags, pRamp);
}

HRESULT w_IDirect3DDevice9Ex::DeletePatch(UINT Handle)
{
    HRESULT hr = Proxy->DeletePatch(Handle);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
    HRESULT hr = Proxy->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
    HRESULT hr = Proxy->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetIndices(THIS_ IDirect3DIndexBuffer9** ppIndexData)
{
    HRESULT hr = Proxy->GetIndices(ppIndexData);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetIndices(THIS_ IDirect3DIndexBuffer9* pIndexData)
{
    HRESULT hr = Proxy->SetIndices(pIndexData);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

UINT w_IDirect3DDevice9Ex::GetAvailableTextureMem()
{
    return Proxy->GetAvailableTextureMem();
}

HRESULT w_IDirect3DDevice9Ex::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters)
{
    HRESULT hr = Proxy->GetCreationParameters(pParameters);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetDeviceCaps(D3DCAPS9* pCaps)
{
    HRESULT hr = Proxy->GetDeviceCaps(pCaps);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetDirect3D(IDirect3D9** ppD3D9)
{
    if (ppD3D9)
    {
        m_pD3DEx->AddRef();

        *ppD3D9 = m_pD3DEx;

        return D3D_OK;
    }
    return D3DERR_INVALIDCALL;
}

HRESULT w_IDirect3DDevice9Ex::GetRasterStatus(THIS_ UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus)
{
    HRESULT hr = Proxy->GetRasterStatus(iSwapChain, pRasterStatus);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetLight(DWORD Index, D3DLIGHT9* pLight)
{
    HRESULT hr = Proxy->GetLight(Index, pLight);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetLightEnable(DWORD Index, BOOL* pEnable)
{
    HRESULT hr = Proxy->GetLightEnable(Index, pEnable);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetMaterial(D3DMATERIAL9* pMaterial)
{
    HRESULT hr = Proxy->GetMaterial(pMaterial);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::LightEnable(DWORD LightIndex, BOOL bEnable)
{
    HRESULT hr = Proxy->LightEnable(LightIndex, bEnable);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetLight(DWORD Index, CONST D3DLIGHT9* pLight)
{
    HRESULT hr = Proxy->SetLight(Index, pLight);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetMaterial(CONST D3DMATERIAL9* pMaterial)
{
    HRESULT hr = Proxy->SetMaterial(pMaterial);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::MultiplyTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    HRESULT hr = Proxy->MultiplyTransform(State, pMatrix);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::ProcessVertices(THIS_ UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags)
{
    HRESULT hr = Proxy->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::TestCooperativeLevel()
{
    HRESULT hr = Proxy->TestCooperativeLevel();

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetCurrentTexturePalette(UINT* pPaletteNumber)
{
    HRESULT hr = Proxy->GetCurrentTexturePalette(pPaletteNumber);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries)
{
    HRESULT hr = Proxy->GetPaletteEntries(PaletteNumber, pEntries);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetCurrentTexturePalette(UINT PaletteNumber)
{
    HRESULT hr = Proxy->SetCurrentTexturePalette(PaletteNumber);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries)
{
    HRESULT hr = Proxy->SetPaletteEntries(PaletteNumber, pEntries);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreatePixelShader(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader)
{
    HRESULT hr = Proxy->CreatePixelShader(pFunction, ppShader);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetPixelShader(THIS_ IDirect3DPixelShader9** ppShader)
{
    HRESULT hr = Proxy->GetPixelShader(ppShader);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetPixelShader(THIS_ IDirect3DPixelShader9* pShader)
{
    return Proxy->SetPixelShader(pShader);
}

HRESULT w_IDirect3DDevice9Ex::Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    //return ProxyInterface->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    return Proxy->PresentEx(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, 0);
}

HRESULT w_IDirect3DDevice9Ex::DrawIndexedPrimitive(THIS_ D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    return Proxy->DrawIndexedPrimitive(Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT w_IDirect3DDevice9Ex::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    return Proxy->DrawIndexedPrimitiveUP(PrimitiveType, MinIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT w_IDirect3DDevice9Ex::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    return Proxy->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
}

HRESULT w_IDirect3DDevice9Ex::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    return Proxy->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT w_IDirect3DDevice9Ex::BeginScene()
{
    return Proxy->BeginScene();
}

HRESULT w_IDirect3DDevice9Ex::GetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride)
{
    HRESULT hr = Proxy->GetStreamSource(StreamNumber, ppStreamData, OffsetInBytes, pStride);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    HRESULT hr = Proxy->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes, Stride);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetBackBuffer(THIS_ UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer)
{
    HRESULT hr = Proxy->GetBackBuffer(iSwapChain, iBackBuffer, Type, ppBackBuffer);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface)
{
    HRESULT hr = Proxy->GetDepthStencilSurface(ppZStencilSurface);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture)
{
    HRESULT hr = Proxy->GetTexture(Stage, ppTexture);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
    HRESULT hr = Proxy->GetTextureStageState(Stage, Type, pValue);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
    HRESULT hr = Proxy->SetTexture(Stage, pTexture);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    return Proxy->SetTextureStageState(Stage, Type, Value);
}

HRESULT w_IDirect3DDevice9Ex::UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture)
{
    HRESULT hr = Proxy->UpdateTexture(pSourceTexture, pDestinationTexture);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::ValidateDevice(DWORD* pNumPasses)
{
    return Proxy->ValidateDevice(pNumPasses);
}

HRESULT w_IDirect3DDevice9Ex::GetClipPlane(DWORD Index, float* pPlane)
{
    return Proxy->GetClipPlane(Index, pPlane);
}

HRESULT w_IDirect3DDevice9Ex::SetClipPlane(DWORD Index, CONST float* pPlane)
{
    return Proxy->SetClipPlane(Index, pPlane);
}

HRESULT w_IDirect3DDevice9Ex::Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    return Proxy->Clear(Count, pRects, Flags, Color, Z, Stencil);
}

HRESULT w_IDirect3DDevice9Ex::GetViewport(D3DVIEWPORT9* pViewport)
{
    HRESULT hr = Proxy->GetViewport(pViewport);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetViewport(CONST D3DVIEWPORT9* pViewport)
{
    HRESULT hr = Proxy->SetViewport(pViewport);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateVertexShader(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader)
{
    HRESULT hr = Proxy->CreateVertexShader(pFunction, ppShader);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetVertexShader(THIS_ IDirect3DVertexShader9** ppShader)
{
    HRESULT hr = Proxy->GetVertexShader(ppShader);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetVertexShader(THIS_ IDirect3DVertexShader9* pShader)
{
    HRESULT hr = Proxy->SetVertexShader(pShader);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateQuery(THIS_ D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)
{
    HRESULT hr = Proxy->CreateQuery(Type, ppQuery);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetPixelShaderConstantB(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT  BoolCount)
{
    HRESULT hr = Proxy->SetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetPixelShaderConstantB(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    HRESULT hr = Proxy->GetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetPixelShaderConstantI(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    HRESULT hr = Proxy->SetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetPixelShaderConstantI(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    HRESULT hr = Proxy->GetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetPixelShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    HRESULT hr = Proxy->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetPixelShaderConstantF(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    HRESULT hr = Proxy->GetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetStreamSourceFreq(THIS_ UINT StreamNumber, UINT Divider)
{
    HRESULT hr = Proxy->SetStreamSourceFreq(StreamNumber, Divider);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetStreamSourceFreq(THIS_ UINT StreamNumber, UINT* Divider)
{
    HRESULT hr = Proxy->GetStreamSourceFreq(StreamNumber, Divider);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetVertexShaderConstantB(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT  BoolCount)
{
    HRESULT hr = Proxy->SetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetVertexShaderConstantB(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    HRESULT hr = Proxy->GetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetVertexShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    HRESULT hr = Proxy->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetVertexShaderConstantF(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    HRESULT hr = Proxy->GetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetVertexShaderConstantI(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    HRESULT hr = Proxy->SetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetVertexShaderConstantI(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    HRESULT hr = Proxy->GetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetFVF(THIS_ DWORD FVF)
{
    HRESULT hr = Proxy->SetFVF(FVF);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetFVF(THIS_ DWORD* pFVF)
{
    HRESULT hr = Proxy->GetFVF(pFVF);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateVertexDeclaration(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl)
{
    HRESULT hr = Proxy->CreateVertexDeclaration(pVertexElements, ppDecl);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetVertexDeclaration(THIS_ IDirect3DVertexDeclaration9* pDecl)
{
    HRESULT hr = Proxy->SetVertexDeclaration(pDecl);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetVertexDeclaration(THIS_ IDirect3DVertexDeclaration9** ppDecl)
{
    HRESULT hr = Proxy->GetVertexDeclaration(ppDecl);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetNPatchMode(THIS_ float nSegments)
{
    HRESULT hr = Proxy->SetNPatchMode(nSegments);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

float w_IDirect3DDevice9Ex::GetNPatchMode(THIS)
{
    return Proxy->GetNPatchMode();
}

BOOL w_IDirect3DDevice9Ex::GetSoftwareVertexProcessing(THIS)
{
    return Proxy->GetSoftwareVertexProcessing();
}

UINT w_IDirect3DDevice9Ex::GetNumberOfSwapChains(THIS)
{
    return Proxy->GetNumberOfSwapChains();
}

HRESULT w_IDirect3DDevice9Ex::EvictManagedResources(THIS)
{
    HRESULT hr = Proxy->EvictManagedResources();

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetSoftwareVertexProcessing(THIS_ BOOL bSoftware)
{
    HRESULT hr = Proxy->SetSoftwareVertexProcessing(bSoftware);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetScissorRect(THIS_ CONST RECT* pRect)
{
    HRESULT hr = Proxy->SetScissorRect(pRect);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetScissorRect(THIS_ RECT* pRect)
{
    HRESULT hr = Proxy->GetScissorRect(pRect);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetSamplerState(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
    HRESULT hr = Proxy->GetSamplerState(Sampler, Type, pValue);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetSamplerState(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    HRESULT hr = Proxy->SetSamplerState(Sampler, Type, Value);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetDepthStencilSurface(THIS_ IDirect3DSurface9* pNewZStencil)
{
    HRESULT hr = Proxy->SetDepthStencilSurface(pNewZStencil);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateOffscreenPlainSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    HRESULT hr = Proxy->CreateOffscreenPlainSurface(Width, Height, Format, Pool, ppSurface, pSharedHandle);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::ColorFill(THIS_ IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color)
{
    HRESULT hr = Proxy->ColorFill(pSurface, pRect, color);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::StretchRect(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
    HRESULT hr = Proxy->StretchRect(pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetFrontBufferData(THIS_ UINT iSwapChain, IDirect3DSurface9* pDestSurface)
{
    HRESULT hr = Proxy->GetFrontBufferData(iSwapChain, pDestSurface);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetRenderTargetData(THIS_ IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface)
{
    HRESULT hr = Proxy->GetRenderTargetData(pRenderTarget, pDestSurface);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::UpdateSurface(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint)
{
    HRESULT hr = Proxy->UpdateSurface(pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetDialogBoxMode(THIS_ BOOL bEnableDialogs)
{
    HRESULT hr = Proxy->SetDialogBoxMode(bEnableDialogs);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetSwapChain(THIS_ UINT iSwapChain, IDirect3DSwapChain9** ppSwapChain)
{
    HRESULT hr = Proxy->GetSwapChain(iSwapChain, ppSwapChain);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetConvolutionMonoKernel(THIS_ UINT width, UINT height, float* rows, float* columns)
{
    HRESULT hr = Proxy->SetConvolutionMonoKernel(width, height, rows, columns);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::ComposeRects(THIS_ IDirect3DSurface9* pSrc, IDirect3DSurface9* pDst, IDirect3DVertexBuffer9* pSrcRectDescs, UINT NumRects, IDirect3DVertexBuffer9* pDstRectDescs, D3DCOMPOSERECTSOP Operation, int Xoffset, int Yoffset)
{
    HRESULT hr = Proxy->ComposeRects(pSrc, pDst, pSrcRectDescs, NumRects, pDstRectDescs, Operation, Xoffset, Yoffset);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::PresentEx(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
    return Proxy->PresentEx(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags | dwPresentFlags);
}

HRESULT w_IDirect3DDevice9Ex::GetGPUThreadPriority(THIS_ INT* pPriority)
{
    return Proxy->GetGPUThreadPriority(pPriority);
}

HRESULT w_IDirect3DDevice9Ex::SetGPUThreadPriority(THIS_ INT Priority)
{
    return Proxy->SetGPUThreadPriority(Priority);
}

HRESULT w_IDirect3DDevice9Ex::WaitForVBlank(THIS_ UINT iSwapChain)
{
    return Proxy->WaitForVBlank(iSwapChain);
}

HRESULT w_IDirect3DDevice9Ex::CheckResourceResidency(THIS_ IDirect3DResource9** pResourceArray, UINT32 NumResources)
{
    HRESULT hr = Proxy->CheckResourceResidency(pResourceArray, NumResources);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::SetMaximumFrameLatency(THIS_ UINT MaxLatency)
{
    if (MaxFrameLatency > -1) {
        MaxLatency = static_cast<UINT>(MaxFrameLatency);
    }

    auto hr = Proxy->SetMaximumFrameLatency(MaxLatency);
    if (hr == S_OK) {
        MESSAGE(_T("Succeeded"), MaxLatency);
    }
    else {
        MESSAGE(_T("Failed (0x%lX) (%u)"), hr, MaxLatency);
    }
    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetMaximumFrameLatency(THIS_ UINT* pMaxLatency)
{
    HRESULT hr = Proxy->GetMaximumFrameLatency(pMaxLatency);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CheckDeviceState(THIS_ HWND hDestinationWindow)
{
    return Proxy->CheckDeviceState(hDestinationWindow);
}

HRESULT w_IDirect3DDevice9Ex::CreateRenderTargetEx(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage)
{
    if (EnableFlip) {
        MultisampleQuality = 0;
        MultiSample = D3DMULTISAMPLE_NONE;
    }

    HRESULT hr = Proxy->CreateRenderTargetEx(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle, Usage);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateOffscreenPlainSurfaceEx(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage)
{
    HRESULT hr = Proxy->CreateOffscreenPlainSurfaceEx(Width, Height, Format, Pool, ppSurface, pSharedHandle, Usage);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::CreateDepthStencilSurfaceEx(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage)
{
    if (EnableFlip) {
        MultisampleQuality = 0;
        MultiSample = D3DMULTISAMPLE_NONE;
    }

    HRESULT hr = Proxy->CreateDepthStencilSurfaceEx(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle, Usage);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}

HRESULT w_IDirect3DDevice9Ex::ResetEx(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode)
{
    D3DPRESENT_PARAMETERS pp = *pPresentationParameters;
    PopulateD3DPresentParams(&pp);
    DumpD3DPresentParams(&pp, pFullscreenDisplayMode);

    if (pp.Windowed) {
        pFullscreenDisplayMode = nullptr;
    }

    HRESULT hr = Proxy->ResetEx(&pp, pFullscreenDisplayMode);
    if (hr == S_OK) {
        D3DPresentParamsCopyOnReturn(&pp, pPresentationParameters);
        MESSAGE(_T("Succeeded"));
    }
    else {
        MESSAGE(_T("Failed (0x%lX)"), hr);
    }
    return hr;
}

HRESULT w_IDirect3DDevice9Ex::GetDisplayModeEx(THIS_ UINT iSwapChain, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation)
{
    HRESULT hr = Proxy->GetDisplayModeEx(iSwapChain, pMode, pRotation);

    if (!SUCCEEDED(hr))
    {
        MESSAGE(_T("FAILED"));
    }

    return hr;
}
