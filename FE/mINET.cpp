#include "pch.h"

#include "hpfe.cpp"

using namespace std;

using namespace Log;
using namespace StrHelpers;

static connect_T connect_O;
static listen_T listen_O;
static WSAConnect_T WSAConnect_O;
static WSALookupServiceBeginA_T WSALookupServiceBeginA_O;
static WSALookupServiceBeginW_T WSALookupServiceBeginW_O;

static InternetOpenA_T InternetOpenA_O;
static InternetOpenW_T InternetOpenW_O;

static WSASetLastError_T WSASetLastError_O;
static InetNtopW_T InetNtopW_O;
static inet_ntop_T inet_ntop_O;

namespace Inet
{
	bool blockConnections = false;
	bool blockListen = false;
	bool blockInternetOpen = false;
	bool blockDNSResolve = false;

	vector<wstring> allowedHosts;
	vector<USHORT> allowedPorts;
}

using namespace Inet;

static bool FilterConnection(const sockaddr* name)
{
	auto sa = reinterpret_cast<const sockaddr_in*>(name);

#ifdef UNICODE
	wchar_t ip[255];
	InetNtopW_O(sa->sin_family, &sa->sin_addr, ip, sizeof(ip));
#else
	char ip[255];
	inet_ntop_O(sa->sin_family, &sa->sin_addr, ip, sizeof(ip));
#endif

	auto it = find(allowedPorts.begin(), allowedPorts.end(), sa->sin_port);
	if (it == allowedPorts.end()) {
		MESSAGE(_T("Blocking connection to: [%s]:%hu"), ip, sa->sin_port);
		return false;
	}
	else {
		MESSAGE(_T("Connect to: [%s]:%hu"), ip, sa->sin_port);
	}

	return true;
}

static int
WSAAPI
listen_Hook(
	_In_ SOCKET s,
	_In_ int backlog
)
{
	MESSAGE(_T("Blocking listen on socket %p"), s);

	WSASetLastError_O(WSAENETDOWN);
	return SOCKET_ERROR;
	//return listen_O(s, backlog);
}

static int
WSAAPI
connect_Hook(
	SOCKET         s,
	const sockaddr* name,
	int            namelen
)
{
	if (!FilterConnection(name)) {
		WSASetLastError_O(WSAENETDOWN);
		return SOCKET_ERROR;
	}
	//__debugbreak();
	return connect_O(s, name, namelen);
}

static int
WSAAPI
WSAConnect_Hook(
	SOCKET         s,
	const sockaddr* name,
	int            namelen,
	LPWSABUF       lpCallerData,
	LPWSABUF       lpCalleeData,
	LPQOS          lpSQOS,
	LPQOS          lpGQOS
)
{
	if (!FilterConnection(name)) {
		WSASetLastError_O(WSAENETDOWN);
		return SOCKET_ERROR;
	}

	return WSAConnect_O(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
}

static
HINTERNET
__stdcall
InternetOpenA_Hook(
	_In_opt_ LPCSTR lpszAgent,
	_In_ DWORD dwAccessType,
	_In_opt_ LPCSTR lpszProxy,
	_In_opt_ LPCSTR lpszProxyBypass,
	_In_ DWORD dwFlags
)
{
	MESSAGE(_T("Blocked"));
	return NULL;
	//return InternetOpenA_O(lpszAgent, dwAccessType, lpszProxy, lpszProxyBypass, dwFlags);
}

static
HINTERNET
__stdcall
InternetOpenW_Hook(
	_In_opt_ LPCWSTR lpszAgent,
	_In_ DWORD dwAccessType,
	_In_opt_ LPCWSTR lpszProxy,
	_In_opt_ LPCWSTR lpszProxyBypass,
	_In_ DWORD dwFlags
)
{
	MESSAGE(_T("Blocked"));
	return NULL;
	//return InternetOpenW_O(lpszAgent, dwAccessType, lpszProxy, lpszProxyBypass, dwFlags);
}


static
bool
FilterLookupService(const wstring& serviceName)
{
	auto it = find(allowedHosts.begin(), allowedHosts.end(), serviceName);
	if (it != allowedHosts.end())
	{
		MESSAGE(_T("Allowing DNS resolve: %s"), StrToNative(serviceName).c_str());
		return true;
	}
	else {
		MESSAGE(_T("Blocking DNS resolve: %s"), StrToNative(serviceName).c_str());
		return false;
	}
}

static
INT
WSAAPI
WSALookupServiceBeginA_Hook(
	_In_ LPWSAQUERYSETA lpqsRestrictions,
	_In_ DWORD          dwControlFlags,
	_Out_ LPHANDLE       lphLookup
)
{
	auto qs = reinterpret_cast<WSAQUERYSETA*>(lpqsRestrictions);

	if (qs->dwNameSpace == NS_DNS && qs->lpszServiceInstanceName != nullptr)
	{
		if (!FilterLookupService(ToWString(qs->lpszServiceInstanceName)))
		{
			WSASetLastError_O(WSASERVICE_NOT_FOUND);
			return SOCKET_ERROR;
		}
	}

	return WSALookupServiceBeginA_O(lpqsRestrictions, dwControlFlags, lphLookup);
}

static
INT
WSAAPI
WSALookupServiceBeginW_Hook(
	_In_ LPWSAQUERYSETW lpqsRestrictions,
	_In_ DWORD          dwControlFlags,
	_Out_ LPHANDLE       lphLookup
)
{
	auto qs = reinterpret_cast<WSAQUERYSETW*>(lpqsRestrictions);

	if (qs->dwNameSpace == NS_DNS && qs->lpszServiceInstanceName != nullptr)
	{
		if (!FilterLookupService(qs->lpszServiceInstanceName))
		{
			WSASetLastError_O(WSASERVICE_NOT_FOUND);
			return SOCKET_ERROR;
		}
	}

	return WSALookupServiceBeginW_O(lpqsRestrictions, dwControlFlags, lphLookup);
}

static volatile LONG isWs2_32Hooked = 0;

static void HookWs2_32(HMODULE hModule)
{
	if (InterlockedCompareExchangeAcquire(&isWs2_32Hooked, 1, 0)) {
		return;
	}

	auto hookIf = pHandle->GetHookInterface();

	if (blockConnections) {
		RegisterDetour(hookIf, hModule, connect_O, connect_Hook, "connect");
		RegisterDetour(hookIf, hModule, WSAConnect_O, WSAConnect_Hook, "WSAConnect");
	}

	if (blockListen) {
		RegisterDetour(hookIf, hModule, listen_O, listen_Hook, "listen");
	}

	if (blockDNSResolve) {
		RegisterDetour(hookIf, hModule, WSALookupServiceBeginA_O, WSALookupServiceBeginA_Hook, "WSALookupServiceBeginA");
		RegisterDetour(hookIf, hModule, WSALookupServiceBeginW_O, WSALookupServiceBeginW_Hook, "WSALookupServiceBeginW");
	}

	WSASetLastError_O = reinterpret_cast<WSASetLastError_T>(GetProcAddress(hModule, "WSASetLastError"));
	InetNtopW_O = reinterpret_cast<InetNtopW_T>(GetProcAddress(hModule, "InetNtopW"));
	inet_ntop_O = reinterpret_cast<inet_ntop_T>(GetProcAddress(hModule, "inet_ntop"));

	IHookLogged(hookIf, &gLog).InstallHooks();
}

static volatile LONG isWininetHooked = 0;

static void HookWininet(HMODULE hModule)
{
	if (InterlockedCompareExchangeAcquire(&isWininetHooked, 1, 0)) {
		return;
	}

	auto hookIf = pHandle->GetHookInterface();

	if (blockInternetOpen) {
		RegisterDetour(hookIf, hModule, InternetOpenA_O, InternetOpenA_Hook, "InternetOpenA");
		RegisterDetour(hookIf, hModule, InternetOpenW_O, InternetOpenW_Hook, "InternetOpenW");
	}

	IHookLogged(hookIf, &gLog).InstallHooks();
}

namespace Inet
{
	void InstallHooksIfLoaded()
	{
		InstallHookIfLoaded(isWs2_32Hooked, _T("Ws2_32.dll"), HookWs2_32);
		InstallHookIfLoaded(isWininetHooked, _T("Wininet.dll"), HookWininet);
	}
}