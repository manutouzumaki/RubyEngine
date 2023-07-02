#include <Windows.h>
#include "RubyTimer.h"

namespace Ruby
{

    Timer::Timer()
        : mSecondsPerCount(0.0),
        mDeltaTime(-1.0),
        mBaseTime(0),
        mPausedTime(0),
        mPrevTime(0),
        mCurrTime(0),
        mStopTime(0),
        mStopped(false)
    {
        __int64 countPerSec;
        QueryPerformanceFrequency((LARGE_INTEGER*)&countPerSec);
        mSecondsPerCount = 1.0 / (double)countPerSec;
    }

    float Timer::TotalTime()
    {
        if (mStopped)
        {
            return (float)(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
        }
        else
        {
            return (float)(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
        }
    }

    float Timer::DeltaTime()
    {
        return (float)mDeltaTime;
    }

    void Timer::Reset()
    {
        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
        mBaseTime = currTime;
        mPrevTime = currTime;
        mStopTime = 0;
        mStopped = false;
    }

    void Timer::Start()
    {
        __int64 startTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

        if (mStopped)
        {
            mPausedTime += (startTime - mStopTime);
            mPrevTime = startTime;
            mStopTime = 0;
            mStopped = false;
        }

    }

    void Timer::Stop()
    {
        if (!mStopped)
        {
            __int64 currTime;
            QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

            mStopTime = currTime;
            mStopped = true;
        }
    }

    void Timer::Tick()
    {
        if (mStopped)
        {
            mDeltaTime = 0.0;
            return;
        }
        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
        mCurrTime = currTime;

        mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

        mPrevTime = mCurrTime;

        if (mDeltaTime < 0.0)
        {
            mDeltaTime = 0.0;
        }
    }
}