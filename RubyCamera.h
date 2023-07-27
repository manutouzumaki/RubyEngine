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

        XMFLOAT3 mViewPos;

        XMFLOAT3 mPosition;
        XMFLOAT3 mVelocity;
        XMFLOAT3 mAcceleration;

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
        XMFLOAT3 GetViewPosition();

        void Update(float dt, Ruby::Physics::Triangle* triangles, int count);
        void MouseMove(float mouseX, float mouseY);
        void MoveForward(float dt);
        void MoveBackward(float dt);
        void MoveLeft(float dt);
        void MoveRight(float dt);
        void MoveUp(float dt);
        void MoveDown(float dt);

    };

}


