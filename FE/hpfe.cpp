#include "pch.h"

template <typename T>
void RegisterDetour(IScoped<IHook>& hookIf, HMODULE hModule, T& orig, PVOID func, const char* fn)
{
	orig = reinterpret_cast<T>(GetProcAddress(hModule, fn));

	if (orig != nullptr) {
		hookIf->Register(&(PVOID&)orig, func);
		Log::_MESSAGE(_T("%s:%d (%s): %s (%p -> %p)"), 
			_T("hpfe.cpp"), __LINE__, _T(__FUNCTION__),
			StrHelpers::StrToNative(fn).c_str(),
			orig, func);
	}
	else {
		Log::_MESSAGE(_T("%s:%d (%s): %s GetProcAddress(%s) = nullptr ???"), 
			_T("hpfe.cpp"), __LINE__, _T(__FUNCTION__),
			StrHelpers::StrToNative(fn).c_str());
	}
}
