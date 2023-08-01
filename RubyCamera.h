#pragma once

#include <DirectXMath.h>

#include "Physics/Collision.h"

using namespace DirectX;

namespace Ruby
{

    class FPSCamera
    {
        XMMATRIX mView;

        XMVECTOR mFront;
        XMVECTOR mRight;
        XMVECTOR mUp;

        XMVECTOR mWorldFront;
        XMVECTOR mWorldUp;

        XMFLOAT3 mPotentialPosition;
        XMFLOAT3 mLastPosition;
        XMFLOAT3 mPosition;
        XMFLOAT3 mVelocity;
        XMFLOAT3 mAcceleration;

        XMFLOAT3 mRenderPosition;

        XMFLOAT3 mRotation;

        float mSpeed;
        float mDumping;
 
    public:
        FPSCamera(XMFLOAT3 position, XMFLOAT3 rotation, float speed);
        ~FPSCamera();

        XMMATRIX GetView();
        XMFLOAT3 GetPosition();
        XMFLOAT3 GetVelocity();
        XMFLOAT3 GetRotation();
        XMFLOAT3 GetViewDirection();
        XMFLOAT3 GetViewRight();
        XMFLOAT3 GetViewUp();

        void Update(float dt);
        void FixUpdate(float dt, Ruby::Physics::Triangle* triangles, int count);
        void PostUpdate(float t);
        void MouseMove(float mouseX, float mouseY, float dt);
        void MoveForward();
        void MoveBackward();
        void MoveLeft();
        void MoveRight();
        void MoveUp();
        void MoveDown();

    };

}


