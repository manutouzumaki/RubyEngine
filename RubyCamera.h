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
        XMVECTOR mDirection;

        XMVECTOR mWorldFront;
        XMVECTOR mWorldUp;
        
        // camra rotation
        XMFLOAT3 mRotation;

        // physics of the player
        XMFLOAT3 mPotentialPosition;
        XMFLOAT3 mLastPosition;
        XMFLOAT3 mPosition;
        XMFLOAT3 mVelocity;        
        XMFLOAT3 mAcceleration;
        XMFLOAT3 mMovement;
        
        // render position of the player
        XMFLOAT3 mRenderPosition;

        float mSpeed;
        float mDumping;
        bool mGrounded;
 
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
        bool Grounded();

        void Update(float dt);
        void FixUpdate(float dt, Ruby::Physics::Triangle* triangles, int count);
        void PostUpdate(float t);
        void MouseMove(float mouseX, float mouseY, float dt);
        void MoveForward();
        void MoveBackward();
        void MoveLeft();
        void MoveRight();

        void ApplyForce(XMFLOAT3 force);
        void ApplyImpulse(XMFLOAT3  impulse);

    };

}


