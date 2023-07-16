#pragma once

#include <windows.h>

#define MAX_DEBUG_COUNTER_COUNT 256

namespace Ruby
{
    struct CounterData
    {
        const char* mName;
        UINT64 mStartOS;
        UINT64 mElapsedOS;
        UINT64 mStartCPU;
        UINT64 mElapsedCPU;
    };

    class DebugProfiler
    {
    private:
        UINT64 mId;
        static CounterData mData[MAX_DEBUG_COUNTER_COUNT];
        static UINT64 mCounterCount;
    public:
        void Begin(const char* name);
        void End();

        static void PrintData();
        static UINT64 GetOSFrequency();
        static UINT64 ReadOSTimer();
        static UINT64 GetCPUFrequency();
        static UINT64 ReadCPUTimer();

    };

}

//#ifdef _DEBUG

#define DebugProfilerBegin(name) Ruby::DebugProfiler name##DEBUG_PROFILER; name##DEBUG_PROFILER.Begin(#name)
#define DebugProfilerEnd(name) name##DEBUG_PROFILER.End()

//#else

//#define DebugProfilerBegin(name)
//#define DebugProfilerEnd(name)

//#endif