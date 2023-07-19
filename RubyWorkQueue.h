#pragma once

#include <windows.h>
#include "RubyScene.h"

#define RUBY_MAX_THREAD_COUNT 7
#define RUBY_MAX_ENTRY_COUNT 512

namespace Ruby
{
    struct SplitGeometryEntry
    {
        ID3D11Device* mDevice;
        Mesh* mMesh;
        OctreeNode<SceneStaticObject>* pNode;
    };

    class SplitGeometryWorkQueue
    {
    private:
        UINT32 volatile mCompletitionGoal;
        UINT32 volatile mCompletitionCount;
        UINT32 volatile mNextEntryToWrite;
        UINT32 volatile mNextEntryToRead;
        SplitGeometryEntry mEntries[RUBY_MAX_ENTRY_COUNT];
    public:
        SplitGeometryWorkQueue() {};
        ~SplitGeometryWorkQueue() {};
        void AddEntry(SplitGeometryEntry* data);
        void DoNextEntry();
        void CompleteAllWork();
    };
}

DWORD WINAPI ThreadProc(LPVOID lpParameter);

