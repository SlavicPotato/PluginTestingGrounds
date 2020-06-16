/**
* Copyright (C) 2019 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*/

#include "../pch.h"
#include "d3d9.h"

HRESULT m_IDirect3D9Ex::QueryInterface(REFIID riid, void** ppvObj)
{
    if ((riid == IID_IUnknown || riid == WrapperID) && ppvObj)
    {
        AddRef();

        *ppvObj = this;

        return S_OK;
    }

    return ProxyInterface->QueryInterface(riid, ppvObj);
}

ULONG m_IDirect3D9Ex::AddRef()
{
    return ProxyInterface->AddRef();
}

ULONG m_IDirect3D9Ex::Release()
{
    ULONG count = ProxyInterface->Release();

    if (count == 0)
    {
        delete this;
    }

    return count;
}

HRESULT m_IDirect3D9Ex::EnumAdapterModes(THIS_ UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->EnumAdapterModes(Adapter, Format, Mode, pMode);
}

UINT m_IDirect3D9Ex::GetAdapterCount()
{
    return ProxyInterface->GetAdapterCount();
}

HRESULT m_IDirect3D9Ex::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->GetAdapterDisplayMode(Adapter, pMode);
}

HRESULT m_IDirect3D9Ex::GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
}

UINT m_IDirect3D9Ex::GetAdapterModeCount(THIS_ UINT Adapter, D3DFORMAT Format)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->GetAdapterModeCount(Adapter, Format);
}

HMONITOR m_IDirect3D9Ex::GetAdapterMonitor(UINT Adapter)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->GetAdapterMonitor(Adapter);
}

HRESULT m_IDirect3D9Ex::GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->GetDeviceCaps(Adapter, DeviceType, pCaps);
}

HRESULT m_IDirect3D9Ex::RegisterSoftwareDevice(void* pInitializeFunction)
{
    return ProxyInterface->RegisterSoftwareDevice(pInitializeFunction);
}

HRESULT m_IDirect3D9Ex::CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);
}

HRESULT m_IDirect3D9Ex::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    switch (RType)
    {
    case D3DRTYPE_TEXTURE:
    case D3DRTYPE_VOLUMETEXTURE:
    case D3DRTYPE_CUBETEXTURE:
    case D3DRTYPE_INDEXBUFFER:
    case D3DRTYPE_VERTEXBUFFER:
        if (!(Usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL))) {
            Usage |= D3DUSAGE_DYNAMIC;
        }
        break;
    }

    return ProxyInterface->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
}

HRESULT m_IDirect3D9Ex::CheckDeviceMultiSampleType(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    if (D3D9::DisplayMode > -1) {
        Windowed = static_cast<BOOL>(Windowed);
    }

    return ProxyInterface->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels);
}

HRESULT m_IDirect3D9Ex::CheckDeviceType(UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    if (D3D9::DisplayMode > -1) {
        Windowed = static_cast<BOOL>(Windowed);
    }

    return ProxyInterface->CheckDeviceType(Adapter, CheckType, DisplayFormat, BackBufferFormat, Windowed);
}

HRESULT m_IDirect3D9Ex::CheckDeviceFormatConversion(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->CheckDeviceFormatConversion(Adapter, DeviceType, SourceFormat, TargetFormat);
}

HRESULT m_IDirect3D9Ex::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
    D3DDISPLAYMODEEX* pFullscreenDisplayMode;
    D3DDISPLAYMODEEX fsd;

    if (pPresentationParameters->Windowed == FALSE) {
        D3D9::PopulateD3DDisplayModeEx(pPresentationParameters, &fsd);
        pFullscreenDisplayMode = &fsd;
    }
    else {
        pFullscreenDisplayMode = nullptr;
    }

    MESSAGE("Redirecting CreateDevice->CreateDeviceEx");

    return this->CreateDeviceEx(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, (IDirect3DDevice9Ex**)ppReturnedDeviceInterface);
}

UINT m_IDirect3D9Ex::GetAdapterModeCountEx(THIS_ UINT Adapter, CONST D3DDISPLAYMODEFILTER* pFilter)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->GetAdapterModeCountEx(Adapter, pFilter);
}

HRESULT m_IDirect3D9Ex::EnumAdapterModesEx(THIS_ UINT Adapter, CONST D3DDISPLAYMODEFILTER* pFilter, UINT Mode, D3DDISPLAYMODEEX* pMode)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->EnumAdapterModesEx(Adapter, pFilter, Mode, pMode);
}

HRESULT m_IDirect3D9Ex::GetAdapterDisplayModeEx(THIS_ UINT Adapter, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->GetAdapterDisplayModeEx(Adapter, pMode, pRotation);
}

HRESULT m_IDirect3D9Ex::CreateDeviceEx(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode, IDirect3DDevice9Ex** ppReturnedDeviceInterface)
{
    D3DPRESENT_PARAMETERS pp = *pPresentationParameters;
    D3D9::PopulateD3DPresentParams(&pp);
    D3D9::DumpD3DPresentParams(&pp, pFullscreenDisplayMode);

    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
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

    MESSAGE("Adapter:%u DeviceType: %d BehaviorFlags: 0x%lX hFocusWindow:%p",
        Adapter, DeviceType, BehaviorFlags, hFocusWindow, hFocusWindow);

    auto hr = ProxyInterface->CreateDeviceEx(Adapter, DeviceType, hFocusWindow, BehaviorFlags, &pp, pFullscreenDisplayMode, ppReturnedDeviceInterface);

    if (SUCCEEDED(hr))
    {
        if (ppReturnedDeviceInterface)
        {
            auto w = new m_IDirect3DDevice9Ex(*ppReturnedDeviceInterface, this, IID_IDirect3DDevice9Ex);

            if (D3D9::MaxFrameLatency > -1) {
                w->SetMaximumFrameLatency(
                    static_cast<UINT>(D3D9::MaxFrameLatency));
            }

            Window::OnSwapChainCreate(pPresentationParameters->hDeviceWindow);

            D3D9::D3DPresentParamsCopyOnReturn(&pp, pPresentationParameters);

            *ppReturnedDeviceInterface = w;
        }
    }
    else {
        MESSAGE("Failed (0x%lX)", hr);
    }

    return hr;
}

HRESULT m_IDirect3D9Ex::GetAdapterLUID(THIS_ UINT Adapter, LUID* pLUID)
{
    if (D3D9::ForceAdapter > -1) {
        Adapter = static_cast<UINT>(D3D9::ForceAdapter);
    }

    return ProxyInterface->GetAdapterLUID(Adapter, pLUID);
}
