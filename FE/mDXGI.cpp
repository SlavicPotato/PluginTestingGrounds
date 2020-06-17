#include "pch.h"

using namespace Microsoft::WRL;

using namespace std;
using namespace Log;

static CreateSwapChainForHwnd_T CreateSwapChainForHwnd_O;
static CreateSwapChainForCoreWindow_T CreateSwapChainForCoreWindow_O;
static CreateSwapChainForComposition_T CreateSwapChainForComposition_O;
static CreateDXGIFactory_T CreateDXGIFactory_O;
static CreateDXGIFactory_T CreateDXGIFactory1_O;
static CreateDXGIFactory2_T CreateDXGIFactory2_O;
static ResizeBuffers_T ResizeBuffers_O;
static ResizeBuffers1_T ResizeBuffers1_O;
static ResizeTarget_T ResizeTarget_O;
static Present_T Present_O;
static Present1_T Present1_O;
static SetFullscreenState_T SetFullscreenState_O;
static CreateSwapChain_T CreateSwapChain_O;

static DXGIInfo dxgiInfo;

namespace DXGI
{
	UINT BufferCount;
	DXGI_FORMAT DXGIFormat;
	int DXGISwapEffectInt;
	int DisplayMode;
	bool EnableTearing;
	bool ExplicitRebind;

	bool HasSwapEffect;
	bool HasFormat;
	bool FormatAuto = false;

	long long fps_max = 0;
}

using namespace DXGI;

static long long tts;
//UINT presentFlags = 0;
static bool tearing_on = false;
static bool explicit_rebind = false;

static bool isD3D11Loaded;

static cfgSwapEffectToInt_T cfgSwapEffectToInt = {
	{"auto", -1},
	{"discard", 0},
	{"sequential", 1},
	{"flip_sequential", 2},
	{"flip_discard", 3}
};

static cfgSwapEffectIntToSwapEffect_T cfgSwapEffectIntToSwapEffect = {
	{0, DXGI_SWAP_EFFECT_DISCARD},
	{1, DXGI_SWAP_EFFECT_SEQUENTIAL},
	{2, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL},
	{3, DXGI_SWAP_EFFECT_FLIP_DISCARD}
};

static SwapEffectIntToSwapEffectDesc_T SwapEffectIntToSwapEffectDesc = {
	{-1, _T("auto")},
	{0, _T("discard")},
	{1, _T("sequential")},
	{2, _T("flip_sequential")},
	{3, _T("flip_discard")}
};

void DXGIInfo::DXGI_GetCapabilities(IUnknown* pFactory)
{
	caps = 0;
	version = 0;

	{
		ComPtr<IDXGIFactory5> tmp;
		if (SUCCEEDED(
			pFactory->QueryInterface <IDXGIFactory5>((IDXGIFactory5**)&tmp)))
		{
			caps = (
				DXGI_CAP_FLIP_SEQUENTIAL |
				DXGI_CAP_FLIP_DISCARD
				);

			BOOL allowTearing;
			HRESULT hr = tmp->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

			if (SUCCEEDED(hr) && allowTearing) {
				caps |= DXGI_CAP_TEARING;
			}

			version = DXGI_VERSION_5;

			return;
		}
	}

	{
		ComPtr<IDXGIFactory4> tmp;
		if (SUCCEEDED(
			pFactory->QueryInterface <IDXGIFactory4>((IDXGIFactory4**)&tmp)))
		{
			caps = (
				DXGI_CAP_FLIP_SEQUENTIAL |
				DXGI_CAP_FLIP_DISCARD
				);

			version = DXGI_VERSION_4;

			return;
		}
	}

	{
		ComPtr<IDXGIFactory3> tmp;
		if (SUCCEEDED(
			pFactory->QueryInterface <IDXGIFactory3>((IDXGIFactory3**)&tmp)))
		{
			caps = DXGI_CAP_FLIP_SEQUENTIAL;

			version = DXGI_VERSION_3;

			return;
		}
	}

	{
		ComPtr<IDXGIFactory2> tmp;
		if (SUCCEEDED(
			pFactory->QueryInterface <IDXGIFactory2>((IDXGIFactory2**)&tmp)))
		{
			version = DXGI_VERSION_2;
			return;
		}
	}

	{
		ComPtr<IDXGIFactory1> tmp;
		if (SUCCEEDED(
			pFactory->QueryInterface <IDXGIFactory1>((IDXGIFactory1**)&tmp)))
		{
			version = DXGI_VERSION_1;
			return;
		}
	}
}

static HRESULT STDMETHODCALLTYPE SetFullscreenState_Hook(
	IDXGISwapChain* pSwapChain,
	BOOL Fullscreen,
	_In_opt_  IDXGIOutput* pTarget)
{
	if (DisplayMode > -1) {

		Fullscreen = static_cast<BOOL>(DisplayMode);

		MESSAGE("%d -> %d", Fullscreen, DisplayMode);
		//SetFullscreenState_O(pSwapChain, Fullscreen, pTarget);
	}
	else {
		MESSAGE("%d", Fullscreen);
	}

	//return DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;

	return SetFullscreenState_O(pSwapChain, Fullscreen, pTarget);
}

static __forceinline
void OnPresent(UINT& SyncInterval, UINT& PresentFlags)
{
	if (fps_max > 0) {
		while (PerfCounter::deltal(tts, PerfCounter::Query()) < fps_max) {
			Sleep(0);
		}
		tts = PerfCounter::Query();
	}

	if (tearing_on && SyncInterval == 0) {
		PresentFlags |= DXGI_PRESENT_ALLOW_TEARING;
	}
}

static HRESULT STDMETHODCALLTYPE Present_Rebind_Hook(
	IDXGISwapChain* pSwapChain,
	UINT SyncInterval,
	UINT PresentFlags)
{
	OnPresent(SyncInterval, PresentFlags);

	ID3D11DeviceContext* ctx = nullptr;
	ID3D11RenderTargetView* v1 = nullptr;
	ID3D11DepthStencilView* v2 = nullptr;

	ID3D11Device* dev;
	if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&dev))) {
		dev->GetImmediateContext(&ctx);
		ctx->OMGetRenderTargets(1, &v1, &v2);
	}

	HRESULT r = Present_O(pSwapChain, SyncInterval, PresentFlags);

	if (ctx != nullptr) {
		ctx->OMSetRenderTargets(1, &v1, v2);
		if (v1 != nullptr) {
			v1->Release();
		}
		if (v2 != nullptr) {
			v2->Release();
		}
		ctx->Release();
	}

	return r;
};

static HRESULT STDMETHODCALLTYPE Present_Hook(
	IDXGISwapChain* pSwapChain,
	UINT SyncInterval,
	UINT PresentFlags)
{
	OnPresent(SyncInterval, PresentFlags);

	return Present_O(pSwapChain, SyncInterval, PresentFlags);
};

static HRESULT STDMETHODCALLTYPE Present1_Rebind_Hook(
	IDXGISwapChain1* pSwapChain,
	UINT SyncInterval,
	UINT PresentFlags,
	_In_  const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
	OnPresent(SyncInterval, PresentFlags);

	ID3D11DeviceContext* ctx = nullptr;
	ID3D11RenderTargetView* v1 = nullptr;
	ID3D11DepthStencilView* v2 = nullptr;

	ID3D11Device* dev;
	if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&dev))) {
		dev->GetImmediateContext(&ctx);
		ctx->OMGetRenderTargets(1, &v1, &v2);
	}

	HRESULT r = Present1_O(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);

	if (ctx != nullptr) {
		ctx->OMSetRenderTargets(1, &v1, v2);
		if (v1 != nullptr) {
			v1->Release();
		}
		if (v2 != nullptr) {
			v2->Release();
		}
		ctx->Release();
	}

	return r;
}

static HRESULT STDMETHODCALLTYPE Present1_Hook(
	IDXGISwapChain1* pSwapChain,
	UINT SyncInterval,
	UINT PresentFlags,
	_In_  const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
	OnPresent(SyncInterval, PresentFlags);

	return Present1_O(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static __forceinline
void OnResizeBuffers(
	UINT& BufferCount,
	DXGI_FORMAT& Format,
	UINT& SwapChainFlags)
{
	if (tearing_on) {
		SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}

	if (BufferCount > 0) {
		BufferCount = BufferCount;
	}

	if (HasFormat) {
		Format = DXGIFormat;
	}
}

static HRESULT STDMETHODCALLTYPE ResizeBuffers_Hook(
	IDXGISwapChain* ppSwapChain,
	UINT BufferCount,
	UINT Width,
	UINT Height,
	DXGI_FORMAT NewFormat,
	UINT SwapChainFlags)
{
	OnResizeBuffers(BufferCount, NewFormat, SwapChainFlags);

	MESSAGE("%ux%u, BufferCount: %u, Format: %d, Flags: 0x%.8X",
		Width, Height, BufferCount, NewFormat, SwapChainFlags);

	return ResizeBuffers_O(ppSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
};

static HRESULT STDMETHODCALLTYPE ResizeBuffers1_Hook(
	IDXGISwapChain3* pSwapChain,
	_In_  UINT BufferCount,
	_In_  UINT Width,
	_In_  UINT Height,
	_In_  DXGI_FORMAT Format,
	_In_  UINT SwapChainFlags,
	_In_reads_(BufferCount)  const UINT* pCreationNodeMask,
	_In_reads_(BufferCount)  IUnknown* const* ppPresentQueue)
{
	OnResizeBuffers(BufferCount, Format, SwapChainFlags);

	MESSAGE("%ux%u, BufferCount: %u, Format: %d, Flags: 0x%.8X",
		Width, Height, BufferCount, Format, SwapChainFlags);

	return ResizeBuffers1_O(pSwapChain, BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
}

static HRESULT STDMETHODCALLTYPE ResizeTarget_Hook(
	IDXGISwapChain* pSwapChain,
	_In_  const DXGI_MODE_DESC* pNewTargetParameters)
{
	auto desc = const_cast<DXGI_MODE_DESC*>(pNewTargetParameters);

	if (HasFormat) {
		desc->Format = DXGIFormat;
	}

	if (!DisplayMode) {
		desc->RefreshRate.Denominator = 0;
		desc->RefreshRate.Numerator = 0;
	}

	MESSAGE("%ux%u (%u/%u), Format: %d",
		pNewTargetParameters->Width, pNewTargetParameters->Height,
		pNewTargetParameters->RefreshRate.Denominator, pNewTargetParameters->RefreshRate.Numerator,
		pNewTargetParameters->Format
	);

	return ResizeTarget_O(pSwapChain, pNewTargetParameters);
}

static void
InstallDXGISwapChainVTHooks(void* pSwapChain)
{
	bool r1 = IHook::DetourVTable(pSwapChain, 0xD, ResizeBuffers_Hook, &ResizeBuffers_O);
	bool r2 = IHook::DetourVTable(pSwapChain, 0xA, SetFullscreenState_Hook, &SetFullscreenState_O);
	bool r3 = IHook::DetourVTable(pSwapChain, 0xE, ResizeTarget_Hook, &ResizeTarget_O);

	bool r4 = false, r5 = false, r6 = false;

	if (fps_max > 0 || tearing_on || explicit_rebind) {
		tts = PerfCounter::Query();
		r4 = IHook::DetourVTable(pSwapChain, 0x8, explicit_rebind ? Present_Rebind_Hook : Present_Hook, &Present_O);
		if (dxgiInfo.version >= DXGI_VERSION_2) {
			r5 = IHook::DetourVTable(pSwapChain, 0x16, explicit_rebind ? Present1_Rebind_Hook : Present1_Hook, &Present1_O);
		}
	}

	if (dxgiInfo.version >= DXGI_VERSION_4) {
		r6 = IHook::DetourVTable(pSwapChain, 0x27, ResizeBuffers1_Hook, &ResizeBuffers1_O);
	}

	MESSAGE("IDXGISwapChain vfuncs: (%d, %d, %d, %d, %d, %d)", r1, r2, r3, r4, r5, r6);
}

static __forceinline
DXGI_SWAP_EFFECT GetSwapEffectWindowedAuto()
{
	if (dxgiInfo.caps & DXGI_CAP_FLIP_DISCARD) {
		return DXGI_SWAP_EFFECT_FLIP_DISCARD;
	}
	else if (dxgiInfo.caps & DXGI_CAP_FLIP_SEQUENTIAL) {
		return DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	}

	return DXGI_SWAP_EFFECT_DISCARD;
}

static __forceinline
DXGI_SWAP_EFFECT GetSwapEffectManual()
{
	DXGI_SWAP_EFFECT se = cfgSwapEffectIntToSwapEffect[DXGISwapEffectInt];
	DXGI_SWAP_EFFECT nse = se;

	if (nse == DXGI_SWAP_EFFECT_FLIP_DISCARD &&
		!(dxgiInfo.caps & DXGI_CAP_FLIP_DISCARD))
	{
		nse = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	}

	if (nse == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL &&
		!(dxgiInfo.caps & DXGI_CAP_FLIP_SEQUENTIAL))
	{
		nse = DXGI_SWAP_EFFECT_DISCARD;
	}

	if (nse != se) {
		MESSAGE("swap effect %s not supported, using %s",
			 se, nse);
		se = nse;
	}

	return se;
}

static __forceinline
DXGI_SWAP_EFFECT GetSwapEffect(bool isWindowed)
{
	if (DXGISwapEffectInt == -1) {
		if (isWindowed) {
			return GetSwapEffectWindowedAuto();
		}
		else {
			return DXGI_SWAP_EFFECT_DISCARD;
		}
	}
	else {
		return GetSwapEffectManual();
	}
}

static __forceinline
void OnPreCreateSwapChain1(DXGI_SWAP_CHAIN_DESC1* pDesc, bool isWindowed)
{
	if (HasSwapEffect) {
		pDesc->SwapEffect = GetSwapEffect(isWindowed);
	}

	if (BufferCount > 0) {
		pDesc->BufferCount = BufferCount;
	}

	bool is_flip = pDesc->SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
		pDesc->SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	if (HasFormat) {
		if (FormatAuto) {
			if (is_flip) {
				DXGIFormat = pDesc->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			}
			else {
				HasFormat = false;
			}
		}
		else {
			pDesc->Format = DXGIFormat;
		}
	}

	if (is_flip) {
		pDesc->SampleDesc.Count = 1;
		pDesc->SampleDesc.Quality = 0;

		if (pDesc->BufferCount < 2) {
			pDesc->BufferCount = 2;
		}

		if (EnableTearing && (dxgiInfo.caps & DXGI_CAP_TEARING)) {
			pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
			tearing_on = true;
		}

		explicit_rebind = ExplicitRebind;
	}

	MESSAGE("%ux%u, SwapEffect: %d, Format: %d , Buffers: %u, Flags: 0x%.8X",		
		pDesc->Width, pDesc->Height,
		pDesc->SwapEffect,
		pDesc->Format,
		pDesc->BufferCount,
		pDesc->Flags
	);
}

static HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd_Hook(
	IDXGIFactory2* pFactory,
	_In_  IUnknown* pDevice,
	_In_  HWND hWnd,
	_In_  const DXGI_SWAP_CHAIN_DESC1* pDesc_,
	_In_opt_  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
	_In_opt_  IDXGIOutput* pRestrictToOutput,
	_COM_Outptr_  IDXGISwapChain1** ppSwapChain)
{
	auto pd = *pDesc_;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsdesc_tmp;
	if (pFullscreenDesc != nullptr) {
		fsdesc_tmp = *pFullscreenDesc;
		pFullscreenDesc = &fsdesc_tmp;
	}

	if (DisplayMode > -1) {
		if (pFullscreenDesc == nullptr) {
			if (DisplayMode == 1) {
				fsdesc_tmp.Windowed = FALSE;
				fsdesc_tmp.RefreshRate.Denominator = 0U;
				fsdesc_tmp.RefreshRate.Numerator = 0U;
				fsdesc_tmp.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
				pFullscreenDesc = &fsdesc_tmp;
			}
		}
		else {
			fsdesc_tmp.Windowed = !static_cast<BOOL>(DisplayMode);

			if (fsdesc_tmp.Windowed) {
				fsdesc_tmp.RefreshRate.Denominator = 0U;
				fsdesc_tmp.RefreshRate.Numerator = 0U;
			}
		}
	}

	bool isWindowed = pFullscreenDesc && pFullscreenDesc->Windowed;

	OnPreCreateSwapChain1(&pd, isWindowed);

	if (pFullscreenDesc != nullptr) {
		MESSAGE("Windowed: %d (%u/%u)",
			fsdesc_tmp.Windowed,
			fsdesc_tmp.RefreshRate.Denominator,
			fsdesc_tmp.RefreshRate.Numerator);
	}
	else {
		MESSAGE("Windowed: 1");
	}

	HRESULT r = CreateSwapChainForHwnd_O(pFactory, pDevice, hWnd, &pd, pFullscreenDesc, pRestrictToOutput, ppSwapChain);

	if (SUCCEEDED(r)) {
		MESSAGE("CreateSwapChainForHwnd succeeded");

		InstallDXGISwapChainVTHooks(*ppSwapChain);
		Window::OnSwapChainCreate(hWnd);
	}
	else {
		MESSAGE("CreateSwapChainForHwnd failed (0x%lX)", r);
	}

	return r;
}

static HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow_Hook(
	IDXGIFactory2* pFactory,
	_In_  IUnknown* pDevice,
	_In_  IUnknown* pWindow,
	_In_  const DXGI_SWAP_CHAIN_DESC1* pDesc,
	_In_opt_  IDXGIOutput* pRestrictToOutput,
	_COM_Outptr_  IDXGISwapChain1** ppSwapChain)
{
	auto pd = *pDesc;

	OnPreCreateSwapChain1(&pd, true);

	HRESULT r = CreateSwapChainForCoreWindow_O(pFactory, pDevice, pWindow, &pd, pRestrictToOutput, ppSwapChain);
	if (SUCCEEDED(r)) {
		MESSAGE("CreateSwapChainForCoreWindow succeeded");
	}
	else {
		MESSAGE("CreateSwapChainForCoreWindow failed: 0x%lX", r);
	}

	return r;
}

static HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition_Hook(
	IDXGIFactory2* pFactory,
	_In_  IUnknown* pDevice,
	_In_  const DXGI_SWAP_CHAIN_DESC1* pDesc,
	_In_opt_  IDXGIOutput* pRestrictToOutput,
	_COM_Outptr_  IDXGISwapChain1** ppSwapChain)
{
	auto pd = *pDesc;

	OnPreCreateSwapChain1(&pd, true);

	HRESULT r = CreateSwapChainForComposition_O(pFactory, pDevice, &pd, pRestrictToOutput, ppSwapChain);
	if (SUCCEEDED(r)) {
		MESSAGE("CreateSwapChainForComposition succeeded");
	}
	else {
		MESSAGE("CreateSwapChainForComposition failed: 0x%lX", r);
	}

	return r;
}

static HRESULT STDMETHODCALLTYPE CreateSwapChain_Hook(
	IDXGIFactory* pFactory,
	_In_  IUnknown* pDevice,
	_In_  DXGI_SWAP_CHAIN_DESC* pDesc_,
	_COM_Outptr_  IDXGISwapChain** ppSwapChain)
{
	auto desc = *pDesc_;

	if (DisplayMode > -1) {
		MESSAGE("Applying display mode: %d", DisplayMode);

		desc.Windowed = !static_cast<BOOL>(DisplayMode);

		if (desc.Windowed) {
			desc.BufferDesc.RefreshRate.Denominator = 0;
			desc.BufferDesc.RefreshRate.Numerator = 0;
		}
	}

	if (HasSwapEffect) {
		desc.SwapEffect = GetSwapEffect(desc.Windowed);
	}

	if (BufferCount > 0) {
		desc.BufferCount = BufferCount;
	}

	if (HasFormat) {
		desc.BufferDesc.Format = DXGIFormat;
	}

	//desc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	bool is_flip = desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
		desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	if (HasFormat) {
		if (FormatAuto) {
			if (is_flip) {
				DXGIFormat = desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			}
			else {
				HasFormat = false;
			}
		}
		else {
			desc.BufferDesc.Format = DXGIFormat;
		}
	}

	if (is_flip) {
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		if (desc.BufferCount < 2) {
			desc.BufferCount = 2;
		}

		if (EnableTearing && (dxgiInfo.caps & DXGI_CAP_TEARING)) {
			desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
			tearing_on = true;
		}

		explicit_rebind = ExplicitRebind;
		
	}

	MESSAGE("SwapEffect: %d, Format: %d , Buffers: %u, Flags: 0x%.8X, Windowed: %d",		
		desc.SwapEffect,
		desc.BufferDesc.Format,
		desc.BufferCount,
		desc.Flags,
		desc.Windowed
	);

	MESSAGE("%ux%u (%u/%u)",
		desc.BufferDesc.Width, desc.BufferDesc.Height,
		desc.BufferDesc.RefreshRate.Denominator,
		desc.BufferDesc.RefreshRate.Numerator);


	HRESULT r = CreateSwapChain_O(pFactory, pDevice, &desc, ppSwapChain);

	if (SUCCEEDED(r)) {
		MESSAGE("CreateSwapChain succeeded");

		InstallDXGISwapChainVTHooks(*ppSwapChain);

		Window::OnSwapChainCreate(desc.OutputWindow);
	}
	else {
		MESSAGE("CreateSwapChain failed: 0x%lX", r);
	}

	return r;
}

static void
OnCreateDXGIFactory(void* pFactory)
{
	if (dxgiInfo.RunOnce(reinterpret_cast<IUnknown*>(pFactory))) {
		MESSAGE("DXGI version: %u, capabilities: 0x%.8X", dxgiInfo.version, dxgiInfo.caps);
	}

	bool r1 = IHook::DetourVTable(pFactory, 0xA, CreateSwapChain_Hook, &CreateSwapChain_O);
	bool r2 = false, r3 = false, r4 = false;

	if (dxgiInfo.version >= DXGI_VERSION_2) {
		r2 = IHook::DetourVTable(pFactory, 0xF, CreateSwapChainForHwnd_Hook, &CreateSwapChainForHwnd_O);
		r3 = IHook::DetourVTable(pFactory, 0x10, CreateSwapChainForCoreWindow_Hook, &CreateSwapChainForCoreWindow_O);
		r4 = IHook::DetourVTable(pFactory, 0x18, CreateSwapChainForComposition_Hook, &CreateSwapChainForComposition_O);
	}

	MESSAGE("IDXGIFactory vfuncs: (%d, %d, %d, %d)", r1, r2, r3, r4);
}

static HRESULT WINAPI CreateDXGIFactory_Hook(REFIID riid, _COM_Outptr_ void** ppFactory)
{
	HRESULT r = CreateDXGIFactory_O(riid, ppFactory);

	if (SUCCEEDED(r)) {
		MESSAGE("CreateDXGIFactory succeeded");

		OnCreateDXGIFactory(*ppFactory);
	}
	else {
		MESSAGE("CreateDXGIFactory failed: 0x%lX", r);
	}

	return r;
}

static HRESULT WINAPI CreateDXGIFactory1_Hook(REFIID riid, _COM_Outptr_ void** ppFactory)
{
	HRESULT r = CreateDXGIFactory1_O(riid, ppFactory);

	if (SUCCEEDED(r)) {
		MESSAGE("CreateDXGIFactory1 succeeded");

		OnCreateDXGIFactory(*ppFactory);
	}
	else {
		MESSAGE("CreateDXGIFactory1 failed: 0x%lX", r);
	}

	return r;
}

static HRESULT WINAPI CreateDXGIFactory2_Hook(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory)
{
	HRESULT r = CreateDXGIFactory2_O(Flags, riid, ppFactory);

	if (SUCCEEDED(r)) {
		MESSAGE("CreateDXGIFactory2 succeeded");

		OnCreateDXGIFactory(*ppFactory);
	}
	else {
		MESSAGE("CreateDXGIFactory2 failed: 0x%lX", r);
	}

	return r;
}

namespace DXGI
{
	bool ConfigTranslateSwapEffect(const string& param, int& out)
	{
		auto it = cfgSwapEffectToInt.find(param);
		if (it != cfgSwapEffectToInt.end()) {
			out = it->second;
			return true;
		}
		else {
			return false;
		}
	}

	STD_STRING GetSwapEffectDesc(int s)
	{
		return SwapEffectIntToSwapEffectDesc[s];
	}

	using namespace Plugin;

	void InstallProxyOverrides(IPlugin* h)
	{
		h->SetProxyOverride(
			Misc::Underlying(Proxy::Index::kCreateDXGIFactory),
			reinterpret_cast<FARPROC>(CreateDXGIFactory_Hook),
			reinterpret_cast<void**>(&CreateDXGIFactory_O));

		h->SetProxyOverride(
			Misc::Underlying(Proxy::Index::kCreateDXGIFactory1),
			reinterpret_cast<FARPROC>(CreateDXGIFactory1_Hook),
			reinterpret_cast<void**>(&CreateDXGIFactory1_O));

		h->SetProxyOverride(
			Misc::Underlying(Proxy::Index::kCreateDXGIFactory2),
			reinterpret_cast<FARPROC>(CreateDXGIFactory2_Hook),
			reinterpret_cast<void**>(&CreateDXGIFactory2_O));
	}
}
