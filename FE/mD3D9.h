#pragma once

#include <initguid.h>
#include <d3d9.h>

typedef IDirect3D9* (WINAPI* Direct3DCreate9_T)(UINT SDKVersion);
typedef HRESULT(WINAPI* Direct3DCreate9Ex_T)(UINT SDKVersion, IDirect3D9Ex**);

namespace D3D9
{
    extern bool PresentIntervalImmediate;
    extern bool EnableFlip;
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
}

class w_IDirect3D9Ex : public IDirect3D9Ex
{
private:
    IDirect3D9Ex* ProxyInterface;
    GUID WrapperID = IID_IUnknown;

public:
    w_IDirect3D9Ex(IDirect3D9Ex* pDirect3D, GUID DeviceID) :
        ProxyInterface(pDirect3D), WrapperID(DeviceID)
    {
    }

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /*** IDirect3D9 methods ***/
    STDMETHOD(RegisterSoftwareDevice)(THIS_ void* pInitializeFunction);
    STDMETHOD_(UINT, GetAdapterCount)(THIS);
    STDMETHOD(GetAdapterIdentifier)(THIS_ UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier);
    STDMETHOD_(UINT, GetAdapterModeCount)(THIS_ UINT Adapter, D3DFORMAT Format);
    STDMETHOD(EnumAdapterModes)(THIS_ UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode);
    STDMETHOD(GetAdapterDisplayMode)(THIS_ UINT Adapter, D3DDISPLAYMODE* pMode);
    STDMETHOD(CheckDeviceType)(THIS_ UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed);
    STDMETHOD(CheckDeviceFormat)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
    STDMETHOD(CheckDeviceMultiSampleType)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels);
    STDMETHOD(CheckDepthStencilMatch)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
    STDMETHOD(CheckDeviceFormatConversion)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat);
    STDMETHOD(GetDeviceCaps)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps);
    STDMETHOD_(HMONITOR, GetAdapterMonitor)(THIS_ UINT Adapter);
    STDMETHOD(CreateDevice)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);
    STDMETHOD_(UINT, GetAdapterModeCountEx)(THIS_ UINT Adapter, CONST D3DDISPLAYMODEFILTER* pFilter);
    STDMETHOD(EnumAdapterModesEx)(THIS_ UINT Adapter, CONST D3DDISPLAYMODEFILTER* pFilter, UINT Mode, D3DDISPLAYMODEEX* pMode);
    STDMETHOD(GetAdapterDisplayModeEx)(THIS_ UINT Adapter, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation);
    STDMETHOD(CreateDeviceEx)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode, IDirect3DDevice9Ex** ppReturnedDeviceInterface);
    STDMETHOD(GetAdapterLUID)(THIS_ UINT Adapter, LUID* pLUID);
};

class w_IDirect3DDevice9Ex : public IDirect3DDevice9Ex
{
private:
    IDirect3DDevice9Ex* Proxy;
    IDirect3D9Ex* m_pD3DEx;
    GUID WrapperID = IID_IUnknown;

public:
    w_IDirect3DDevice9Ex(IDirect3DDevice9Ex* pDevice, IDirect3D9Ex* pD3D, GUID DeviceID) :
        Proxy(pDevice), m_pD3DEx(pD3D), WrapperID(DeviceID)
    {
    }
    IDirect3DDevice9Ex* GetProxyInterface() { return Proxy; }

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /*** IDirect3DDevice9 methods ***/
    STDMETHOD(TestCooperativeLevel)(THIS);
    STDMETHOD_(UINT, GetAvailableTextureMem)(THIS);
    STDMETHOD(EvictManagedResources)(THIS);
    STDMETHOD(GetDirect3D)(THIS_ IDirect3D9** ppD3D9);
    STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS9* pCaps);
    STDMETHOD(GetDisplayMode)(THIS_ UINT iSwapChain, D3DDISPLAYMODE* pMode);
    STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS* pParameters);
    STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap);
    STDMETHOD_(void, SetCursorPosition)(THIS_ int X, int Y, DWORD Flags);
    STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow);
    STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain);
    STDMETHOD(GetSwapChain)(THIS_ UINT iSwapChain, IDirect3DSwapChain9** pSwapChain);
    STDMETHOD_(UINT, GetNumberOfSwapChains)(THIS);
    STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters);
    STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
    STDMETHOD(GetBackBuffer)(THIS_ UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer);
    STDMETHOD(GetRasterStatus)(THIS_ UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus);
    STDMETHOD(SetDialogBoxMode)(THIS_ BOOL bEnableDialogs);
    STDMETHOD_(void, SetGammaRamp)(THIS_ UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp);
    STDMETHOD_(void, GetGammaRamp)(THIS_ UINT iSwapChain, D3DGAMMARAMP* pRamp);
    STDMETHOD(CreateTexture)(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle);
    STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle);
    STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle);
    STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle);
    STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle);
    STDMETHOD(CreateRenderTarget)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
    STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
    STDMETHOD(UpdateSurface)(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint);
    STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture);
    STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface);
    STDMETHOD(GetFrontBufferData)(THIS_ UINT iSwapChain, IDirect3DSurface9* pDestSurface);
    STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);
    STDMETHOD(ColorFill)(THIS_ IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color);
    STDMETHOD(CreateOffscreenPlainSurface)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
    STDMETHOD(SetRenderTarget)(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget);
    STDMETHOD(GetRenderTarget)(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget);
    STDMETHOD(SetDepthStencilSurface)(THIS_ IDirect3DSurface9* pNewZStencil);
    STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface9** ppZStencilSurface);
    STDMETHOD(BeginScene)(THIS);
    STDMETHOD(EndScene)(THIS);
    STDMETHOD(Clear)(THIS_ DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
    STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
    STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
    STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* pViewport);
    STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* pViewport);
    STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* pMaterial);
    STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL9* pMaterial);
    STDMETHOD(SetLight)(THIS_ DWORD Index, CONST D3DLIGHT9* pLight);
    STDMETHOD(GetLight)(THIS_ DWORD Index, D3DLIGHT9* pLight);
    STDMETHOD(LightEnable)(THIS_ DWORD Index, BOOL Enable);
    STDMETHOD(GetLightEnable)(THIS_ DWORD Index, BOOL* pEnable);
    STDMETHOD(SetClipPlane)(THIS_ DWORD Index, CONST float* pPlane);
    STDMETHOD(GetClipPlane)(THIS_ DWORD Index, float* pPlane);
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State, DWORD Value);
    STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State, DWORD* pValue);
    STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB);
    STDMETHOD(BeginStateBlock)(THIS);
    STDMETHOD(EndStateBlock)(THIS_ IDirect3DStateBlock9** ppSB);
    STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS9* pClipStatus);
    STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS9* pClipStatus);
    STDMETHOD(GetTexture)(THIS_ DWORD Stage, IDirect3DBaseTexture9** ppTexture);
    STDMETHOD(SetTexture)(THIS_ DWORD Stage, IDirect3DBaseTexture9* pTexture);
    STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
    STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
    STDMETHOD(GetSamplerState)(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue);
    STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
    STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses);
    STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber, CONST PALETTEENTRY* pEntries);
    STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber, PALETTEENTRY* pEntries);
    STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber);
    STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT* PaletteNumber);
    STDMETHOD(SetScissorRect)(THIS_ CONST RECT* pRect);
    STDMETHOD(GetScissorRect)(THIS_ RECT* pRect);
    STDMETHOD(SetSoftwareVertexProcessing)(THIS_ BOOL bSoftware);
    STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(THIS);
    STDMETHOD(SetNPatchMode)(THIS_ float nSegments);
    STDMETHOD_(float, GetNPatchMode)(THIS);
    STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
    STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
    STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
    STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
    STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags);
    STDMETHOD(CreateVertexDeclaration)(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl);
    STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* pDecl);
    STDMETHOD(GetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9** ppDecl);
    STDMETHOD(SetFVF)(THIS_ DWORD FVF);
    STDMETHOD(GetFVF)(THIS_ DWORD* pFVF);
    STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader);
    STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* pShader);
    STDMETHOD(GetVertexShader)(THIS_ IDirect3DVertexShader9** ppShader);
    STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount);
    STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount);
    STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount);
    STDMETHOD(GetVertexShaderConstantI)(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount);
    STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT  BoolCount);
    STDMETHOD(GetVertexShaderConstantB)(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount);
    STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride);
    STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride);
    STDMETHOD(SetStreamSourceFreq)(THIS_ UINT StreamNumber, UINT Setting);
    STDMETHOD(GetStreamSourceFreq)(THIS_ UINT StreamNumber, UINT* pSetting);
    STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer9* pIndexData);
    STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer9** ppIndexData);
    STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader);
    STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* pShader);
    STDMETHOD(GetPixelShader)(THIS_ IDirect3DPixelShader9** ppShader);
    STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount);
    STDMETHOD(GetPixelShaderConstantF)(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount);
    STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount);
    STDMETHOD(GetPixelShaderConstantI)(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount);
    STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT  BoolCount);
    STDMETHOD(GetPixelShaderConstantB)(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount);
    STDMETHOD(DrawRectPatch)(THIS_ UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo);
    STDMETHOD(DrawTriPatch)(THIS_ UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo);
    STDMETHOD(DeletePatch)(THIS_ UINT Handle);
    STDMETHOD(CreateQuery)(THIS_ D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery);
    STDMETHOD(SetConvolutionMonoKernel)(THIS_ UINT width, UINT height, float* rows, float* columns);
    STDMETHOD(ComposeRects)(THIS_ IDirect3DSurface9* pSrc, IDirect3DSurface9* pDst, IDirect3DVertexBuffer9* pSrcRectDescs, UINT NumRects, IDirect3DVertexBuffer9* pDstRectDescs, D3DCOMPOSERECTSOP Operation, int Xoffset, int Yoffset);
    STDMETHOD(PresentEx)(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags);
    STDMETHOD(GetGPUThreadPriority)(THIS_ INT* pPriority);
    STDMETHOD(SetGPUThreadPriority)(THIS_ INT Priority);
    STDMETHOD(WaitForVBlank)(THIS_ UINT iSwapChain);
    STDMETHOD(CheckResourceResidency)(THIS_ IDirect3DResource9** pResourceArray, UINT32 NumResources);
    STDMETHOD(SetMaximumFrameLatency)(THIS_ UINT MaxLatency);
    STDMETHOD(GetMaximumFrameLatency)(THIS_ UINT* pMaxLatency);
    STDMETHOD(CheckDeviceState)(THIS_ HWND hDestinationWindow);
    STDMETHOD(CreateRenderTargetEx)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage);
    STDMETHOD(CreateOffscreenPlainSurfaceEx)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage);
    STDMETHOD(CreateDepthStencilSurfaceEx)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage);
    STDMETHOD(ResetEx)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode);
    STDMETHOD(GetDisplayModeEx)(THIS_ UINT iSwapChain, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation);
};