#pragma once

#include <chrono>

class PerfCounter
{
public:
	PerfCounter()
	{
		perf_freq = _Query_perf_frequency();
		perf_freqf = static_cast<float>(perf_freq);
	}

	template <typename T>
	__forceinline static T delta(long long tp1, long long tp2)
	{
		return static_cast<T>(tp2 - tp1) / perf_freqf;
	}

	__forceinline static long long deltal(long long tp1, long long tp2)
	{
		return ((tp2 - tp1) * 1000000) / perf_freq;
	}

	__forceinline static long long Query()
	{
		long long t;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&t));
		return t;
	}
private:
	static long long perf_freq;
	static float perf_freqf;
	static PerfCounter m_Instance;
};
