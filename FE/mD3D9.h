#pragma once

#include <d3d9.h>

typedef IDirect3D9* (WINAPI* Direct3DCreate9_T)(UINT SDKVersion);
typedef HRESULT(WINAPI* Direct3DCreate9Ex_T)(UINT SDKVersion, IDirect3D9Ex**);

namespace D3D9
{
    //extern bool EnableEx;
    extern bool EnableFlip;
    extern bool PresentIntervalImmediate;
    extern int32_t BufferCount;
    extern int32_t MaxFrameLatency;

    extern bool HasFormat;
    extern bool FormatAuto;
    extern D3DFORMAT D3DFormat;

    extern bool CreateTextureUsageDynamic;
    extern UINT CreateTextureClearUsageFlags;

    extern bool CreateIndexBufferUsageDynamic;
    extern bool CreateVertexBufferUsageDynamic;

    extern bool CreateCubeTextureUsageDynamic;
    extern bool CreateVolumeTextureUsageDynamic;

    extern int32_t ForceAdapter;

    extern int32_t DisplayMode;

    void InstallHooksIfLoaded();
    void InstallProxyOverrides(IPlugin* h);


    extern UINT dwPresentFlags;

    extern void PopulateD3DDisplayModeEx(
        D3DPRESENT_PARAMETERS* pPresentationParameters,
        D3DDISPLAYMODEEX* pFullscreenDisplayMode);
    extern void PopulateD3DPresentParams(
        D3DPRESENT_PARAMETERS* pPresentationParameters);
    extern void D3DPresentParamsCopyOnReturn(
        D3DPRESENT_PARAMETERS* pPresentationParametersL,
        D3DPRESENT_PARAMETERS* pPresentationParametersR);
    extern D3DFORMAT GetD3DFormat(D3DFORMAT _format);
    extern void DumpD3DPresentParams(
        D3DPRESENT_PARAMETERS* pPresentationParameters,
        D3DDISPLAYMODEEX* pFullscreenDisplayMode);

}
