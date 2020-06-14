#pragma once

typedef int
(WSAAPI* listen_T)(
	_In_ SOCKET s,
	_In_ int backlog
	);

typedef int (WSAAPI* connect_T)(
	SOCKET         s,
	const sockaddr* name,
	int            namelen);

typedef int (WSAAPI* WSAConnect_T)(
	SOCKET         s,
	const sockaddr* name,
	int            namelen,
	LPWSABUF       lpCallerData,
	LPWSABUF       lpCalleeData,
	LPQOS          lpSQOS,
	LPQOS          lpGQOS);

typedef HINTERNET(__stdcall* InternetOpenA_T)(
	_In_opt_ LPCSTR lpszAgent,
	_In_ DWORD dwAccessType,
	_In_opt_ LPCSTR lpszProxy,
	_In_opt_ LPCSTR lpszProxyBypass,
	_In_ DWORD dwFlags);

typedef HINTERNET(__stdcall* InternetOpenW_T)(
	_In_opt_ LPCWSTR lpszAgent,
	_In_ DWORD dwAccessType,
	_In_opt_ LPCWSTR lpszProxy,
	_In_opt_ LPCWSTR lpszProxyBypass,
	_In_ DWORD dwFlags);



typedef INT
(WSAAPI* WSALookupServiceBeginA_T)(
	_In_ LPWSAQUERYSETA lpqsRestrictions,
	_In_ DWORD          dwControlFlags,
	_Out_ LPHANDLE       lphLookup);

typedef INT
(WSAAPI* WSALookupServiceBeginW_T)(
	_In_ LPWSAQUERYSETW lpqsRestrictions,
	_In_ DWORD          dwControlFlags,
	_Out_ LPHANDLE       lphLookup);

typedef void
(WSAAPI* WSASetLastError_T)(
	_In_ int iError
	);

typedef PCWSTR
(WSAAPI* InetNtopW_T)(
	_In_                                INT             Family,
	_In_                                const VOID* pAddr,
	_Out_writes_(StringBufSize)         PWSTR           pStringBuf,
	_In_                                size_t          StringBufSize
	);

typedef PCSTR
(WSAAPI* inet_ntop_T)(
	_In_                                INT             Family,
	_In_                                const VOID* pAddr,
	_Out_writes_(StringBufSize)         PSTR            pStringBuf,
	_In_                                size_t          StringBufSize
	);


namespace Inet
{
	extern bool blockConnections;
	extern bool blockListen;
	extern bool blockInternetOpen;
	extern bool blockDNSResolve;

	extern std::vector<std::wstring> allowedHosts;
	extern std::vector<USHORT> allowedPorts;


	extern void InstallHooksIfLoaded();
}