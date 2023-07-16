#include "RubyDebugProfiler.h"

#include <stdio.h>

namespace Ruby
{
    CounterData DebugProfiler::mData[MAX_DEBUG_COUNTER_COUNT];
    UINT64 DebugProfiler::mCounterCount;

    void DebugProfiler::Begin(const char* name)
    {
        mId = mCounterCount++;
        mData[mId].mName = name;
        mData[mId].mStartOS = ReadOSTimer();
        mData[mId].mStartCPU = ReadCPUTimer();
    }

    void DebugProfiler::End()
    {
        mData[mId].mElapsedOS = ReadOSTimer() - mData[mId].mStartOS;
        mData[mId].mElapsedCPU = ReadCPUTimer() - mData[mId].mStartCPU;
    }

    void DebugProfiler::PrintData()
    {
        double total = (double)mData[0].mElapsedOS;

        for (int i = 0; i < mCounterCount; ++i)
        {
            UINT64 percentage = (UINT64)(((double)mData[i].mElapsedOS / total) * 100.0);
            double secondsElapsed = (double)mData[i].mElapsedOS / (double)GetOSFrequency();
            char buffer[256];
            sprintf_s(buffer, "Cycles Elapsed: %llu, (%llu), %f sec [%s]\n", mData[i].mElapsedCPU, percentage, secondsElapsed, mData[i].mName);
            OutputDebugString(buffer);
        }
        mCounterCount = 0;
    }


    UINT64 DebugProfiler::GetOSFrequency()
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return freq.QuadPart;
    }

    UINT64 DebugProfiler::ReadOSTimer()
    {
        LARGE_INTEGER value;
        QueryPerformanceCounter(&value);
        return value.QuadPart;
    }

    UINT64 DebugProfiler::GetCPUFrequency()
    {
        UINT64 millisecondsToWait = 100.0f;

        UINT64 osFreq = GetOSFrequency();

        UINT64 cpuStart = ReadCPUTimer();
        UINT64 osStart = ReadOSTimer();
        UINT64 osEnd = 0;
        UINT64 osElapsed = 0;
        UINT64 osWaitTime = osFreq * millisecondsToWait / 1000;
        while (osElapsed < osWaitTime)
        {
            osEnd = ReadOSTimer();
            osElapsed = osEnd - osStart;
        }
        UINT64 cpuEnd = ReadCPUTimer();
        UINT64 cpuElapsed = cpuEnd - cpuStart;
        UINT64 cpuFreq = 0;
        if (osElapsed)
        {
            cpuFreq = osFreq * cpuElapsed / osElapsed;
        }

        return cpuFreq;
    }

    UINT64 DebugProfiler::ReadCPUTimer()
    {
        return __rdtsc();
    }
}