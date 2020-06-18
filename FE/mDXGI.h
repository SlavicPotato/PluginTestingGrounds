#pragma once

typedef HRESULT(WINAPI* CreateDXGIFactory_T)(REFIID riid, _COM_Outptr_ void** ppFactory);
typedef HRESULT(WINAPI* CreateDXGIFactory2_T)(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory);

typedef HRESULT(STDMETHODCALLTYPE* CreateSwapChain_T)(
	IDXGIFactory* pFactory,
	_In_  IUnknown* pDevice,
	_In_  DXGI_SWAP_CHAIN_DESC* pDesc,
	_COM_Outptr_  IDXGISwapChain** ppSwapChain);


typedef HRESULT(STDMETHODCALLTYPE* SetMaximumFrameLatency_T)(
	IDXGIDevice1* pDXGIDevice,
	UINT MaxLatency);

typedef HRESULT(STDMETHODCALLTYPE* Present_T)(
	IDXGISwapChain* pSwapChain,
	UINT SyncInterval,
	UINT PresentFlags);

typedef HRESULT(STDMETHODCALLTYPE* Present1_T)(
	IDXGISwapChain1* pSwapChain,
	UINT SyncInterval,
	UINT PresentFlags,
	_In_  const DXGI_PRESENT_PARAMETERS* pPresentParameters);

typedef HRESULT(STDMETHODCALLTYPE* SetFullscreenState_T)(
	IDXGISwapChain* pSwapChain,
	BOOL Fullscreen,
	_In_opt_  IDXGIOutput* pTarget);

typedef HRESULT(STDMETHODCALLTYPE* CreateSwapChainForHwnd_T)(
	IDXGIFactory2* pFactory,
	_In_  IUnknown* pDevice,
	_In_  HWND hWnd,
	_In_  const DXGI_SWAP_CHAIN_DESC1* pDesc,
	_In_opt_  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
	_In_opt_  IDXGIOutput* pRestrictToOutput,
	_COM_Outptr_  IDXGISwapChain1** ppSwapChain);

typedef  HRESULT(STDMETHODCALLTYPE* CreateSwapChainForCoreWindow_T)(
	IDXGIFactory2* pFactory,
	_In_  IUnknown* pDevice,
	_In_  IUnknown* pWindow,
	_In_  const DXGI_SWAP_CHAIN_DESC1* pDesc,
	_In_opt_  IDXGIOutput* pRestrictToOutput,
	_COM_Outptr_  IDXGISwapChain1** ppSwapChain);

typedef HRESULT(STDMETHODCALLTYPE* CreateSwapChainForComposition_T)(
	IDXGIFactory2* pFactory,
	_In_  IUnknown* pDevice,
	_In_  const DXGI_SWAP_CHAIN_DESC1* pDesc,
	_In_opt_  IDXGIOutput* pRestrictToOutput,
	_COM_Outptr_  IDXGISwapChain1** ppSwapChain);

typedef HRESULT(STDMETHODCALLTYPE* ResizeBuffers_T)(
	IDXGISwapChain* pSwapChain,
	UINT BufferCount,
	UINT Width,
	UINT Height,
	DXGI_FORMAT NewFormat,
	UINT SwapChainFlags);

typedef HRESULT(STDMETHODCALLTYPE* ResizeBuffers1_T)(
	IDXGISwapChain3* pSwapChain,
	_In_  UINT BufferCount,
	_In_  UINT Width,
	_In_  UINT Height,
	_In_  DXGI_FORMAT Format,
	_In_  UINT SwapChainFlags,
	_In_reads_(BufferCount)  const UINT* pCreationNodeMask,
	_In_reads_(BufferCount)  IUnknown* const* ppPresentQueue);

typedef HRESULT(STDMETHODCALLTYPE* ResizeTarget_T)(
	IDXGISwapChain* pSwapChain,
	_In_  const DXGI_MODE_DESC* pNewTargetParameters);

typedef std::unordered_map<std::string, int> cfgSwapEffectToInt_T;
typedef std::unordered_map<int, DXGI_SWAP_EFFECT> cfgSwapEffectIntToSwapEffect_T;
typedef std::unordered_map<int, STD_STRING> SwapEffectIntToSwapEffectDesc_T;

class DXGIInfo
{
public:
	DXGIInfo() :
		ran(false),
		version(0),
		caps(0)
	{}

	void DXGI_GetCapabilities(IUnknown* pFactory);

	bool RunOnce(IUnknown* pFactory)
	{
		if (InterlockedCompareExchangeAcquire(&ran, 1, 0)) {
			return false;
		}

		DXGI_GetCapabilities(pFactory);
		return true;
	}

	uint32_t caps;
	uint32_t version;
private:
	LONG volatile ran;
};

namespace DXGI
{
	extern UINT BufferCount;
	extern DXGI_FORMAT Format;
	extern int DXGISwapEffectInt;
	extern int DisplayMode;
	extern bool EnableTearing;
	extern bool ExplicitRebind;

	extern bool HasSwapEffect;
	extern bool HasFormat;
	extern bool FormatAuto;

	extern long long fps_max;

	extern bool ConfigTranslateSwapEffect(const std::string& param, int& out);
	extern STD_STRING GetSwapEffectDesc(int s);


	void InstallProxyOverrides(IPlugin* h);
}

constexpr uint32_t DXGI_CAP_FLIP_DISCARD = 0x00000001U;
constexpr uint32_t DXGI_CAP_FLIP_SEQUENTIAL = 0x00000002U;
constexpr uint32_t DXGI_CAP_TEARING = 0x00000004U;

constexpr uint32_t DXGI_VERSION_1 = 0x00000001U;
constexpr uint32_t DXGI_VERSION_2 = 0x00000002U;
constexpr uint32_t DXGI_VERSION_3 = 0x00000003U;
constexpr uint32_t DXGI_VERSION_4 = 0x00000004U;
constexpr uint32_t DXGI_VERSION_5 = 0x00000005U;

