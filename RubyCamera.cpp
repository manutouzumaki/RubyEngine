#include "RubyCamera.h"

#include <stdio.h>
#include <Windows.h>

using namespace DirectX;

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

        XMVECTOR diff = vel * dt;

        // Old collision detection and resolution
        Ruby::Physics::Ray camRay;
        camRay.o = Ruby::Physics::Vector3(pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]);
        camRay.d = Ruby::Physics::Vector3(diff.m128_f32[0], diff.m128_f32[1], diff.m128_f32[2]);

        Physics::Sphere sphere;
        sphere.c = Ruby::Physics::Vector3(pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]);
        sphere.r = 0.75f;

        Physics::Segment segment;
        segment.a = Ruby::Physics::Vector3(pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]);
        segment.b = segment.a + Ruby::Physics::Vector3(diff.m128_f32[0], diff.m128_f32[1], diff.m128_f32[2]);

#if 1
        // find the smallest t and solve for that one
        Ruby::Physics::real smallesT = REAL_MAX;
        XMVECTOR n = XMVectorSet(0, 0, 0, 0);

        for (int i = 0; i < count; ++i)
        {
            Ruby::Physics::Triangle triangle = triangles[i];
            //Ruby::Physics::real t = camRay.RaycastPlane(triangle.GetPlane());

            float t = -1.0f;
            Physics::Point q;
            sphere.MovingSpherePlane(camRay.d, triangle.GetPlane(), t, q);

            if (t >= 0.0f && t <= 1.0f)
            {
                if (q.InTriangle(triangle))
                {
                    if (t < smallesT)
                    {
                        smallesT = t;
                        Ruby::Physics::Vector3 normal = (triangle.b - triangle.a).VectorProduct(triangle.c - triangle.a);
                        normal.Normalize();
                        n = XMVectorSet(normal.x, normal.y, normal.z, 0.0);
                    }
                }
                else
                {
                    Physics::Capsule capsule0;
                    capsule0.a = triangle.a;
                    capsule0.b = triangle.b;
                    capsule0.r = sphere.r - 0.01f;
                    Physics::Capsule capsule1;
                    capsule1.a = triangle.b;
                    capsule1.b = triangle.c;
                    capsule1.r = sphere.r - 0.01f;
                    Physics::Capsule capsule2;
                    capsule2.a = triangle.c;
                    capsule2.b = triangle.a;
                    capsule2.r = sphere.r - 0.01f;

                    if (segment.IntersectCapsule(capsule0, t, q))
                    {
                        if (t < smallesT)
                        {
                            Physics::Segment segment;
                            segment.a = capsule0.a;
                            segment.b = capsule0.b;
                            smallesT = t;
                            Ruby::Physics::Vector3 normal = q - q.ClosestPointSegement(segment);
                            //Ruby::Physics::Vector3 normal = (triangle.b - triangle.a).VectorProduct(triangle.c - triangle.a);
                            normal.Normalize();

                            
                            n = XMVectorSet(normal.x, normal.y, normal.z, 0.0);
                        }
                    }
                    else if (segment.IntersectCapsule(capsule1, t, q))
                    {
                        if (t < smallesT)
                        {
                            Physics::Segment segment;
                            segment.a = capsule1.a;
                            segment.b = capsule1.b;

                            smallesT = t;
                            Ruby::Physics::Vector3 normal = q - q.ClosestPointSegement(segment);
                            //Ruby::Physics::Vector3 normal = (triangle.b - triangle.a).VectorProduct(triangle.c - triangle.a);
                            normal.Normalize();
                            n = XMVectorSet(normal.x, normal.y, normal.z, 0.0);
                        }
                    }
                    else if (segment.IntersectCapsule(capsule2, t, q))
                    {
                        if (t < smallesT)
                        {
                            Physics::Segment segment;
                            segment.a = capsule2.a;
                            segment.b = capsule2.b;

                            smallesT = t;
                            Ruby::Physics::Vector3 normal = q - q.ClosestPointSegement(segment);
                            //Ruby::Physics::Vector3 normal = (triangle.b - triangle.a).VectorProduct(triangle.c - triangle.a);
                            normal.Normalize();
                            n = XMVectorSet(normal.x, normal.y, normal.z, 0.0);
                        }
                    }

                }
            }
            
        }

        if (smallesT < REAL_MAX)
        {
            pos = (pos + (diff * smallesT)) + (n * 0.005f);
            vel = vel - (n * XMVector3Dot(vel, n));
            XMVECTOR scaleVelocity = vel * (1.0f - smallesT);
            pos = pos + scaleVelocity * dt;
        }
        else
        {
            pos = pos + diff;
        }
#endif
        
        
        mFront = XMVector3Transform(mWorldFront, XMMatrixRotationX(mRotation.x));
        mFront = XMVector3Transform(mFront, XMMatrixRotationY(mRotation.y));
        mFront = XMVector3Transform(mFront, XMMatrixRotationZ(mRotation.z));
        mRight = XMVector3Normalize(XMVector3Cross(mWorldUp, mFront));
        mUp = XMVector3Normalize(XMVector3Cross(mRight, mFront));

        XMVECTOR viewPos = pos - mFront * 0.0f;
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