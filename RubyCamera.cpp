#include "RubyCamera.h"

#include <stdio.h>
#include <Windows.h>

namespace Ruby
{



    FPSCamera::FPSCamera(XMFLOAT3 position, XMFLOAT3 rotation, float speed)
        :
        mPosition(position),
        mVelocity(0, 0, 0),
        mAcceleration(0, 0, 0),
        mRotation(rotation),
        mSpeed(speed),
        mDumping(0.001f)
    {

        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);

        mFront = XMVectorSet(0, 0, 1, 0);
        mRight = XMVectorSet(1, 0, 0, 0 );
        mUp = XMVectorSet(0, 1, 0, 0);

        mWorldFront = mFront;
        mWorldUp = mUp;

        mFront = XMVector3Transform(mWorldFront, XMMatrixRotationX(mRotation.x));
        mFront = XMVector3Transform(mFront, XMMatrixRotationY(mRotation.y));
        mFront = XMVector3Transform(mFront, XMMatrixRotationZ(mRotation.z));
        mRight = XMVector3Normalize(XMVector3Cross(mWorldUp, mFront));
        mUp    = XMVector3Normalize(XMVector3Cross(mRight, mFront));

        XMVECTOR viewPos = pos - mFront * 2.0f;
        mView = XMMatrixLookAtLH(viewPos, viewPos + mFront, mWorldUp);
        XMStoreFloat3(&mViewPos, viewPos);
    }

    FPSCamera::~FPSCamera()
    {
    
    }

    XMFLOAT3 FPSCamera::GetPosition()
    {
        return mPosition;
    }

    XMFLOAT3 FPSCamera::GetRotation()
    {
        return mRotation;
    }

    XMFLOAT3 FPSCamera::GetVelocity()
    {
        return mVelocity;
    }


    XMMATRIX FPSCamera::GetView()
    {
        return mView;
    }

    XMFLOAT3 FPSCamera::GetViewPosition()
    {
        return mViewPos;
    }


    XMFLOAT3 FPSCamera::GetViewDirection()
    {
        XMVECTOR front =  XMVector3Normalize(mFront);
        XMFLOAT3 result;
        XMStoreFloat3(&result, front);
        return result;
    }

    XMFLOAT3 FPSCamera::GetViewRight()
    {
        XMVECTOR right = XMVector3Normalize(mRight);
        XMFLOAT3 result;
        XMStoreFloat3(&result, right);
        return result;
    }


    XMFLOAT3 FPSCamera::GetViewUp()
    {
        XMVECTOR right = XMVector3Normalize(mUp);
        XMFLOAT3 result;
        XMStoreFloat3(&result, right);
        return result;
    }



    void FPSCamera::Update(float dt, Ruby::Physics::Triangle* triangles, int count)
    {
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);
        XMVECTOR acc = XMVectorSet(mAcceleration.x, mAcceleration.y, mAcceleration.z, 0.0f);
        XMVECTOR vel = XMVectorSet(mVelocity.x, mVelocity.y, mVelocity.z, 0.0f);

        vel = vel + (XMVector3Normalize(acc) * mSpeed) * dt;
        vel = vel * powf(mDumping, dt);

        XMVECTOR newPos = pos + vel * dt;

        XMVECTOR diff = vel * dt;

        Ruby::Physics::Ray camRay;
        camRay.o = Ruby::Physics::Vector3(pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]);
        camRay.d = Ruby::Physics::Vector3(diff.m128_f32[0], diff.m128_f32[1], diff.m128_f32[2]);

        // find the smallest t and solve for that one
        Ruby::Physics::real smallesT = REAL_MAX;
        XMVECTOR n = XMVectorSet(0, 0, 0, 0);

        int collisionCount = 0;

        for (int i = 0; i < count; ++i)
        {
            Ruby::Physics::Triangle triangle = triangles[i];
            Ruby::Physics::real t = camRay.RaycastTriangle(triangle);

            float diffLen = XMVector3Length(diff).m128_f32[0];

            if (t >= 0.0f && t <= 1.0f)
            {
                if (t < smallesT)
                {
                    smallesT = t;
                    Ruby::Physics::Vector3 normal = (triangle.b - triangle.a).VectorProduct(triangle.c - triangle.a);
                    normal.Normalize();
                    n = XMVectorSet(normal.x, normal.y, normal.z, 0.0);
                }
                collisionCount++;
            }
        }

        if (smallesT < REAL_MAX)
        {
            pos = (pos + (diff * smallesT)) + (n * 0.005f);
            vel = vel - (n * XMVector3Dot(vel, n));
            if (collisionCount >= 2)
            {
                vel = XMVectorSet(0, 0, 0, 0);
            }

            XMVECTOR scaleVelocity = vel * (1.0f - smallesT);
            newPos = pos + scaleVelocity * dt;
        }

        pos = newPos;
        
        mFront = XMVector3Transform(mWorldFront, XMMatrixRotationX(mRotation.x));
        mFront = XMVector3Transform(mFront, XMMatrixRotationY(mRotation.y));
        mFront = XMVector3Transform(mFront, XMMatrixRotationZ(mRotation.z));
        mRight = XMVector3Normalize(XMVector3Cross(mWorldUp, mFront));
        mUp = XMVector3Normalize(XMVector3Cross(mRight, mFront));

        XMVECTOR viewPos = pos - mFront * 2.0f;
        mView = XMMatrixLookAtLH(viewPos, viewPos + mFront, mWorldUp);

        XMStoreFloat3(&mPosition, pos);
        XMStoreFloat3(&mVelocity, vel);
        XMStoreFloat3(&mViewPos, viewPos);

        mAcceleration = XMFLOAT3(0, 0, 0);
    }

    void FPSCamera::MouseMove(float mouseX, float mouseY)
    {
        mRotation.y += mouseX;
        mRotation.x += mouseY;

        if (mRotation.x > (89.0f/180.0f) * XM_PI)
            mRotation.x = (89.0f / 180.0f) * XM_PI;
        if (mRotation.x < -(89.0f / 180.0f) * XM_PI)
            mRotation.x = -(89.0f/180.0f) * XM_PI;
    }

    void FPSCamera::MoveForward(float dt)
    {
        XMVECTOR acc = XMVectorSet(mAcceleration.x, mAcceleration.y, mAcceleration.z, 0.0f);
        acc = acc + mFront;
        XMStoreFloat3(&mAcceleration, acc);

    }

    void FPSCamera::MoveBackward(float dt)
    {
        XMVECTOR acc = XMVectorSet(mAcceleration.x, mAcceleration.y, mAcceleration.z, 0.0f);
        acc = acc - mFront;
        XMStoreFloat3(&mAcceleration, acc);
    }

    void FPSCamera::MoveLeft(float dt)
    {
        XMVECTOR acc = XMVectorSet(mAcceleration.x, mAcceleration.y, mAcceleration.z, 0.0f);
        acc = acc - mRight;
        XMStoreFloat3(&mAcceleration, acc);
    }

    void FPSCamera::MoveRight(float dt)
    {
        XMVECTOR acc = XMVectorSet(mAcceleration.x, mAcceleration.y, mAcceleration.z, 0.0f);
        acc = acc + mRight;
        XMStoreFloat3(&mAcceleration, acc);
    }

    void FPSCamera::MoveUp(float dt)
    {
        XMVECTOR acc = XMVectorSet(mAcceleration.x, mAcceleration.y, mAcceleration.z, 0.0f);
        acc = acc + mWorldUp;
        XMStoreFloat3(&mAcceleration, acc);
    }

    void FPSCamera::MoveDown(float dt)
    {
        XMVECTOR acc = XMVectorSet(mAcceleration.x, mAcceleration.y, mAcceleration.z, 0.0f);
        acc = acc - mWorldUp;
        XMStoreFloat3(&mAcceleration, acc);
    }

    

}