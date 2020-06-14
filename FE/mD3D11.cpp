#include "pch.h"

#include "hpfe.cpp"

using namespace Microsoft::WRL;

using namespace std;
using namespace Log;

namespace D3D11
{
	int32_t MaxFrameLatency;
}

using namespace D3D11;

static PFN_D3D11_CREATE_DEVICE D3D11CreateDevice_O;
static PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN D3D11CreateDeviceAndSwapChain_O;

static SetMaximumFrameLatency_T SetMaximumFrameLatency_O;
static CreateShaderResourceView_T CreateShaderResourceView_O;

static HRESULT STDMETHODCALLTYPE CreateShaderResourceView_Hook(
	ID3D11Device* pDevice,
	_In_  ID3D11Resource* pResource,
	_In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
	_COM_Outptr_opt_  ID3D11ShaderResourceView** ppSRView)
{
	HRESULT r = CreateShaderResourceView_O(pDevice, pResource, pDesc, ppSRView);

	if (r != S_OK)
	{
		MESSAGE(_T("CreateShaderResourceView failed (0x%lX), trying with DXGI_FORMAT_UNKNOWN"), r);

		auto desc = const_cast<D3D11_SHADER_RESOURCE_VIEW_DESC*>(pDesc);
		desc->Format = DXGI_FORMAT_UNKNOWN;

		r = CreateShaderResourceView_O(pDevice, pResource, pDesc, ppSRView);
		if (r != S_OK) {
			MESSAGE(_T("CreateShaderResourceView failed (0x%lX)"));
		}
	}

	return r;
}


static void SetMaxFrameLatency(IUnknown* pDevice)
{
	if (MaxFrameLatency < 0) {
		return;
	}

	IDXGIDevice1* pDXGIDevice;
	auto hr = pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&pDXGIDevice);
	if (SUCCEEDED(hr)) {
		pDXGIDevice->SetMaximumFrameLatency(static_cast<UINT>(MaxFrameLatency));
	}
	else {
		MESSAGE(_T("QueryInterface(IDXGIDevice1) failed"));
	}
}


static HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency_Hook(
	IDXGIDevice1* pDXGIDevice,
	UINT MaxLatency)
{
	if (MaxFrameLatency > -1) {
		MaxLatency = static_cast<UINT>(MaxFrameLatency);
	}

	MESSAGE(_T("%u -> %u"), MaxFrameLatency, MaxLatency);

	return SetMaximumFrameLatency_O(pDXGIDevice, MaxLatency);
}

/*typedef  void (STDMETHODCALLTYPE* OMSetRenderTargets_T)(
	ID3D11DeviceContext* This,
	_In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
	_In_reads_opt_(NumViews)  ID3D11RenderTargetView* const* ppRenderTargetViews,
	_In_opt_  ID3D11DepthStencilView* pDepthStencilView);

OMSetRenderTargets_T OMSetRenderTargets_O;

void STDMETHODCALLTYPE OMSetRenderTargets_Hook(
	ID3D11DeviceContext* This,
	_In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
	_In_reads_opt_(NumViews)  ID3D11RenderTargetView* const* ppRenderTargetViews,
	_In_opt_  ID3D11DepthStencilView* pDepthStencilView)
{

	MESSAGE("YYYYYYYYYYYY : %p  %u  %p %p", This, NumViews, *ppRenderTargetViews, pDepthStencilView);

	OMSetRenderTargets_O(This, NumViews, ppRenderTargetViews, pDepthStencilView);
}*/

static void InstallD3DDeviceVFTHooks(ID3D11Device* pDevice)
{
	bool r1 = IHook::DetourVTable(pDevice, 0x7, CreateShaderResourceView_Hook, &CreateShaderResourceView_O);
	bool r2 = false;

	IDXGIDevice1* pDXGIDevice;
	HRESULT hr = pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&pDXGIDevice);
	if (SUCCEEDED(hr)) {
		r2 = IHook::DetourVTable(pDXGIDevice, 0xC, SetMaximumFrameLatency_Hook, &SetMaximumFrameLatency_O);
	}
	else {
		MESSAGE(_T("Couldn't get DXGIDevice1"));
	}


	/*ID3D11DeviceContext* ctx;
	pDevice->GetImmediateContext(&ctx);

	//__debugbreak();
	bool r3 = IHook::DetourVTable(ctx, 0x21, OMSetRenderTargets_Hook, &OMSetRenderTargets_O);*/

	//ctx->OMSetRenderTargets(0, nullptr, nullptr);

	MESSAGE(_T("ID3D11Device vfuncs: (%d, %d, %d)"), r1, r2, false);
}


HRESULT WINAPI D3D11CreateDevice_Hook(
	_In_opt_ IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	_In_reads_opt_(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	_COM_Outptr_opt_ ID3D11Device** ppDevice,
	_Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel,
	_COM_Outptr_opt_ ID3D11DeviceContext** ppImmediateContext)
{
	HRESULT r = D3D11CreateDevice_O(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
	if (r == S_OK)
	{		
		MESSAGE(_T("D3D11CreateDevice succeeded"));

		InstallD3DDeviceVFTHooks(*ppDevice);
		SetMaxFrameLatency(*ppDevice);
	}
	else {
		MESSAGE(_T("D3D11CreateDevice failed: 0x%lX"), r);
	}

	return r;
}

static HRESULT WINAPI D3D11CreateDeviceAndSwapChain_Hook(
	_In_opt_ IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	_In_reads_opt_(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	_In_opt_ CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	_COM_Outptr_opt_ IDXGISwapChain** ppSwapChain,
	_COM_Outptr_opt_ ID3D11Device** ppDevice,
	_Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel,
	_COM_Outptr_opt_ ID3D11DeviceContext** ppImmediateContext)
{

	HRESULT r = D3D11CreateDeviceAndSwapChain_O(
		pAdapter, DriverType, Software, Flags,
		pFeatureLevels, FeatureLevels, SDKVersion,
		pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel,
		ppImmediateContext);

	//(*ppImmediateContext)->OMSetRenderTargets

	if (r == S_OK) {
		MESSAGE(_T("D3D11CreateDeviceAndSwapChain succeeded"));

		InstallD3DDeviceVFTHooks(*ppDevice);
		SetMaxFrameLatency(*ppDevice);

		if (pSwapChainDesc != nullptr) {
			Window::OnSwapChainCreate(pSwapChainDesc->OutputWindow);
		}
	}
	else {
		MESSAGE(_T("D3D11CreateDeviceAndSwapChain failed: 0x%lX"), r);
	}

	return r;
}


static volatile LONG isD3D11Hooked = 0;

static void HookD3D11(HMODULE hModule)
{
	if (InterlockedCompareExchangeAcquire(&isD3D11Hooked, 1, 0)) {
		return;
	}

	auto hookIf = pHandle->GetHookInterface();

	RegisterDetour(hookIf, hModule, D3D11CreateDevice_O, D3D11CreateDevice_Hook, "D3D11CreateDevice");
	RegisterDetour(hookIf, hModule, D3D11CreateDeviceAndSwapChain_O, D3D11CreateDeviceAndSwapChain_Hook, "D3D11CreateDeviceAndSwapChain");

	IHookLogged(hookIf, &gLog).InstallHooks();
}

namespace D3D11
{
	void InstallHooksIfLoaded()
	{
		InstallHookIfLoaded(isD3D11Hooked, _T("d3d11.dll"), HookD3D11);
	}
}
