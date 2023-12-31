#include "RubyWorkQueue.h"
#include "RubyDefines.h"

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    for (;;)
    {
        Ruby::SplitGeometryWorkQueue* queue = (Ruby::SplitGeometryWorkQueue*)lpParameter;
        queue->DoNextEntry();
    }

}

namespace Ruby
{
    void SplitGeometryWorkQueue::AddEntry(SplitGeometryEntry* data)
    {
        UINT32 newNextEntryToWrite = (mNextEntryToWrite + 1) % RUBY_MAX_ENTRY_COUNT;
        Assert(newNextEntryToWrite != mNextEntryToRead);
        mEntries[mNextEntryToWrite] = *data;
        ++mCompletitionGoal;
        _WriteBarrier();
        mNextEntryToWrite = newNextEntryToWrite;
    }

    void SplitGeometryWorkQueue::DoNextEntry()
    {
        UINT32 originalNextEntryToRead = mNextEntryToRead;
        UINT32 newNextEntryToRead = (originalNextEntryToRead + 1) % RUBY_MAX_ENTRY_COUNT;
        if (originalNextEntryToRead != mNextEntryToWrite)
        {
            UINT32 index = InterlockedCompareExchange(
                (LONG volatile*)&mNextEntryToRead,
                newNextEntryToRead,
                originalNextEntryToRead);

            if (index == originalNextEntryToRead)
            {
                SplitGeometryEntry* work = mEntries + index;

                float halfWidth = work->pNode->halfWidth;
                XMFLOAT3 center = work->pNode->center;

                Ruby::Plane faces[6] = {

                    {XMFLOAT3(1, 0,   0), center.x - halfWidth},
                    {XMFLOAT3(-1,  0,  0), -center.x - halfWidth},
                    {XMFLOAT3(0, 1,   0), center.y - halfWidth},
                    {XMFLOAT3(0, -1,  0), -center.y - halfWidth},
                    {XMFLOAT3(0, 0,   1), center.z - halfWidth},
                    {XMFLOAT3(0, 0,  -1), -center.z - halfWidth},

                };

                Ruby::Mesh* mesh = work->mMesh;

                for (int i = 0; i < 6; ++i)
                {
                    Ruby::Mesh* tmp = nullptr;
                    if (i > 0) tmp = mesh;
                    mesh = mesh->Clip(work->mDevice, faces[i]);
                    if(tmp) delete tmp;
                    if (mesh == nullptr)
                    {
                        break;
                    }
                }
                if (mesh != nullptr)
                {
                    SceneStaticObject object{};
                    object.mMesh = mesh;
                    for (int i = 0; i < mesh->Indices.size(); i += 3)
                    {
                        XMFLOAT3 a = mesh->Vertices[mesh->Indices[i + 0]].Position;
                        XMFLOAT3 b = mesh->Vertices[mesh->Indices[i + 1]].Position;
                        XMFLOAT3 c = mesh->Vertices[mesh->Indices[i + 2]].Position;
                        Ruby::Physics::Triangle triangle;
                        triangle.a = Ruby::Physics::Vector3(a.x, a.y, a.z);
                        triangle.b = Ruby::Physics::Vector3(b.x, b.y, b.z);
                        triangle.c = Ruby::Physics::Vector3(c.x, c.y, c.z);
                        object.mTriangles.push_back(triangle);
                    }
                    work->pNode->pObjList.push_back(object);
                }
                InterlockedIncrement((LONG volatile*)&mCompletitionCount);
            }

        }
    }

    void SplitGeometryWorkQueue::CompleteAllWork()
    {
        while (mCompletitionGoal != mCompletitionCount)
        {
            DoNextEntry();
        }
        mCompletitionGoal = 0;
        mCompletitionCount = 0;
    }

}
