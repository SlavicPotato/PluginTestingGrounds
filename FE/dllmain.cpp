#include "pch.h"

using namespace std;

using namespace Log;
using namespace StrHelpers;
using namespace FileHelpers;

static IConfig conf;

IPlugin* pHandle;

static vector<wstring> blockedModules;

static LoadLibraryA_T LoadLibraryA_O = LoadLibraryA;
static LoadLibraryW_T LoadLibraryW_O = LoadLibraryW;
static LoadLibraryExA_T LoadLibraryExA_O = LoadLibraryExA;
static LoadLibraryExW_T LoadLibraryExW_O = LoadLibraryExW;

static bool FilterLoadLibraryW(const wstring& lfn)
{
    auto it = find(blockedModules.begin(), blockedModules.end(), lfn);
    if (it != blockedModules.end())
    {
        MESSAGE(_T("Blocking load: %s"), StrToNative(lfn).c_str());
        return false;
    }

    return true;
}

static void
InstallHooksIfLoaded()
{
    D3D9::InstallHooksIfLoaded();
    D3D11::InstallHooksIfLoaded();
    Inet::InstallHooksIfLoaded();
    Window::InstallHooksIfLoaded();
}

static HMODULE
WINAPI
LoadLibraryA_Hook(
    _In_ LPCSTR lpLibFileName
)
{
    //MESSAGE(_T("%s"), StrToNative(lpLibFileName).c_str());

    auto fileName(GetPathFileNameA(lpLibFileName));

    if (!FilterLoadLibraryW(ToWString(fileName))) {
        SetLastError(ERROR_MOD_NOT_FOUND);
        return NULL;
    }

    HMODULE r = LoadLibraryA_O(lpLibFileName);
    InstallHooksIfLoaded();
    return r;
}

static HMODULE
WINAPI
LoadLibraryW_Hook(
    _In_ LPCWSTR lpLibFileName
)
{
    auto fileName(GetPathFileNameW(lpLibFileName));

    if (!FilterLoadLibraryW(fileName)) {
        SetLastError(ERROR_MOD_NOT_FOUND);
        return NULL;
    }

    HMODULE r = LoadLibraryW_O(lpLibFileName);
    InstallHooksIfLoaded();
    return r;
}


static HMODULE
WINAPI
LoadLibraryExA_Hook(
    _In_ LPCSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
)
{
    auto fileName(GetPathFileNameA(lpLibFileName));

    if (!FilterLoadLibraryW(ToWString(fileName))) {
        SetLastError(ERROR_MOD_NOT_FOUND);
        return NULL;
    }

    HMODULE r = LoadLibraryExA_O(lpLibFileName, hFile, dwFlags);
    InstallHooksIfLoaded();
    return r;
}

static HMODULE
WINAPI
LoadLibraryExW_Hook(
    _In_ LPCWSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
)
{
    auto fileName(GetPathFileNameW(lpLibFileName));

    if (!FilterLoadLibraryW(fileName)) {
        SetLastError(ERROR_MOD_NOT_FOUND);
        return NULL;
    }

    HMODULE r = LoadLibraryExW_O(lpLibFileName, hFile, dwFlags);
    InstallHooksIfLoaded();
    return r;
}

extern "C"
{
    bool __cdecl Initialize(IPlugin* h)
    {
        h->SetPluginName("FE");
        h->SetPluginVersion(0, 3, 2);

        pHandle = h;

        auto loaderVersion = h->GetLoaderVersion();

        STD_STRING exePath = StrToNative(h->GetExecutablePath());
        STD_STRING logPath = exePath + _T("\\DLLPluginLogs");
        STD_STRING configPath = exePath + _T("\\DLLPlugins\\FE.ini");

        gLog.Open(logPath.c_str(), _T("FE.log"));
        MESSAGE(_T("Initializing plugin"));

        if (conf.Load(StrToStr(configPath)) != 0) {
            MESSAGE(_T("Couldn't load config file"));
        }

        DXGI::HasSwapEffect = DXGI::ConfigTranslateSwapEffect(
            conf.Get("DXGI", "SwapEffect", "default"),
            DXGI::DXGISwapEffectInt
        );

        DXGI::HasFormat = conf.Exists("DXGI", "Format");
        if (DXGI::HasFormat) {
            if (conf.Get("DXGI", "Format", "") == "auto") {
                DXGI::FormatAuto = true;
            }
            else {
                DXGI::DXGIFormat = conf.Get("DXGI", "Format", DXGI_FORMAT_R8G8B8A8_UNORM);
            }
        }

        DXGI::BufferCount = clamp<UINT>(conf.Get<UINT>("DXGI", "BufferCount", 0), 0U, 8U);
        DXGI::DisplayMode = clamp(conf.Get("DXGI", "DisplayMode", -1), -1, 1);
        DXGI::EnableTearing = conf.Get("DXGI", "EnableTearing", false);
        DXGI::ExplicitRebind = conf.Get("DXGI", "ExplicitRebind", false);
        D3D11::MaxFrameLatency = clamp(conf.Get("DXGI", "MaxFrameLatency", -1), -1, 16);

        STD_STRING formatStr;
        if (DXGI::HasFormat) {
            if (DXGI::FormatAuto) {
                formatStr = _T("auto");
            }
            else {
                formatStr = STD_TOSTR(static_cast<int>(DXGI::DXGIFormat));
            }
        }
        else {
            formatStr = _T("default");
        }

        STD_STRING swapEffectStr;
        if (DXGI::HasSwapEffect) {
            swapEffectStr = DXGI::GetSwapEffectDesc(DXGI::DXGISwapEffectInt);
        }
        else {
            swapEffectStr = _T("default");
        }

        MESSAGE(_T("SwapEffect: %s, Format: %s, BufferCount: %u, Tearing: %d"),
            swapEffectStr.c_str(), formatStr.c_str(),
            DXGI::BufferCount, DXGI::EnableTearing);

        float framerateLimit = conf.Get("DXGI", "FramerateLimit", 0.0f);
        if (framerateLimit > 0.0f) {
            DXGI::fps_max = static_cast<long long>((1.0f / framerateLimit) * 1000000.0f);
            MESSAGE(_T("Framerate limit: %.6g"), framerateLimit);
        }

        D3D9::EnableFlip = conf.Get("D3D9", "EnableFlip", false);
        D3D9::PresentIntervalImmediate = conf.Get("D3D9", "PresentIntervalImmediate", false);
        D3D9::BufferCount = clamp<int32_t>(conf.Get<int32_t>("D3D9", "BufferCount", -1), -1, D3DPRESENT_BACK_BUFFERS_MAX_EX);
        D3D9::MaxFrameLatency = clamp<int32_t>(conf.Get<int32_t>("D3D9", "MaxFrameLatency", -1), -1, 20);

        D3D9::HasFormat = conf.Exists("D3D9", "Format");
        if (D3D9::HasFormat) {
            if (conf.Get("D3D9", "Format", "") == "auto") {
                D3D9::FormatAuto = true;
            }
            else {
                D3D9::Format = conf.Get("D3D9", "Format", D3DFMT_X8R8G8B8);
            }
        }

        uint32_t numBlockedModules = SplitStringW(
            ToWString(conf.Get("DLL", "BlockedModules", "")),
            L',', blockedModules);

        Inet::blockConnections = conf.Get("Net", "BlockConnections", false);
        Inet::blockListen = conf.Get("Net", "BlockListen", false);
        Inet::blockInternetOpen = conf.Get("Net", "BlockInternetOpen", false);
        Inet::blockDNSResolve = conf.Get("Net", "BlockDNSResolve", false);

        auto hookIf = h->GetHookInterface();

        //MESSAGE(_T(":: %u"), (*hookIf).GetRefCount());

        if (numBlockedModules > 0) {
            MESSAGE(_T("%u module(s) in blocklist"), numBlockedModules);
        }

        hookIf->Register(&(PVOID&)LoadLibraryA_O, (PVOID)LoadLibraryA_Hook);
        hookIf->Register(&(PVOID&)LoadLibraryW_O, (PVOID)LoadLibraryW_Hook);
        hookIf->Register(&(PVOID&)LoadLibraryExA_O, (PVOID)LoadLibraryExA_Hook);
        hookIf->Register(&(PVOID&)LoadLibraryExW_O, (PVOID)LoadLibraryExW_Hook);

        if (Inet::blockConnections) {
            uint32_t numAllowedPorts = SplitString(
                StrToNative(conf.Get("Net", "AllowedPorts", "")),
                _T(','), Inet::allowedPorts);

            MESSAGE(_T("Blocking network connections (%u port(s) in allow list)"), numAllowedPorts);
        }

        if (Inet::blockListen) {
            MESSAGE(_T("Blocking network listen"));
        }

        if (Inet::blockDNSResolve) {
            uint32_t numAllowedHosts = SplitStringW(
                ToWString(conf.Get("Net", "AllowedHosts", "")),
                L',', Inet::allowedHosts);

            MESSAGE(_T("Blocking DNS resolve (%u host(s) in allow list)"), numAllowedHosts);
        }

        if (Inet::blockInternetOpen) {
            MESSAGE(_T("Blocking InternetOpen"));
        }

        if (!IHookLogged(hookIf, &gLog).InstallHooks()) {
            MESSAGE(_T("Detours installed"));
        }

        InstallHooksIfLoaded();

        if (loaderVersion == Loader::Version::DXGI) {
            DXGI::InstallProxyOverrides(h);
        }
        else if (loaderVersion == Loader::Version::D3D9) {
            D3D9::InstallProxyOverrides(h);
        }

        return true;
    }
}

