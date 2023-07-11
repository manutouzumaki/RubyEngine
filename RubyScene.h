#pragma once
#include "RubyMesh.h"

namespace Ruby
{
    class SceneStaticObject
    {
    private:
        XMFLOAT4X4 world;
        Mesh* mMesh;
    };

    class SceneObject
    {
    private:
        Mesh* mMesh;
    };
    

    template<typename Type>
    struct OctreeNode
    {
        ~OctreeNode();

        XMFLOAT3 center;
        float halfWidth;
        OctreeNode* pChild[8];
        Type* pObjList;
    };

    template<typename T>
    OctreeNode<T>::~OctreeNode()
    {
        for (int i = 0; i < 8; ++i)
        {
            if (pChild[i])
            {
                delete pChild[i];
            }
        }
    }


    template<typename T>
    OctreeNode<T>* BuildOctree(XMFLOAT3 center, float halfWidth, float stopDepth)
    {
        if (stopDepth < 0) return nullptr;
        else
        {
            // contruct and fill in 'root' of this subtree
            OctreeNode<T>* pNode = new OctreeNode<T>;
            pNode->center = center;
            pNode->halfWidth = halfWidth;
            pNode->pObjList = nullptr;

            // Recursively construct the eight children of the subtree
            XMFLOAT3 offset;
            float step = halfWidth * 0.5f;
            for (int i = 0; i < 8; ++i)
            {
                offset.x = ((i & 1) ? step : -step);
                offset.y = ((i & 2) ? step : -step);
                offset.z = ((i & 4) ? step : -step);

                XMFLOAT3 newCenter;
                XMVECTOR newCenterSIMD = XMVectorSet(center.x, center.y, center.z, 0) +
                    XMVectorSet(offset.x, offset.y, offset.z, 0);
                XMStoreFloat3(&newCenter, newCenterSIMD);

                pNode->pChild[i] = BuildOctree<T>(newCenter, step, stopDepth - 1);

            }
            return pNode;
        }
    };


    template<typename Type>
    class Octree
    {
    public:
        OctreeNode<Type>* mRoot;
        Octree(XMFLOAT3 center, float halfWidth, float stopDepth);
        ~Octree();
    };


    template<typename T>
    Octree<T>::Octree(XMFLOAT3 center, float halfWidth, float stopDepth)
    {
        mRoot = BuildOctree<T>(center, halfWidth, stopDepth);
    }

    template<typename T>
    Octree<T>::~Octree()
    {
        if (mRoot) delete mRoot;
    }

    class Scene
    {
    public:
        Scene(XMFLOAT3 center, float halfWidth, float stopDepth)
            :
            mStaticObjectTree(center, halfWidth, stopDepth),
            mObjectTree(center, halfWidth, stopDepth) {}
        ~Scene() {}
        Octree<SceneStaticObject> mStaticObjectTree;
        Octree<SceneObject> mObjectTree;
    };
}

