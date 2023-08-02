#include "RubyCamera.h"

#include <stdio.h>
#include <Windows.h>

using namespace DirectX;

namespace Ruby
{
    FPSCamera::FPSCamera(XMFLOAT3 position, XMFLOAT3 rotation, float speed)
        :
        mPotentialPosition(position),
        mPosition(position),
        mLastPosition(position),
        mRenderPosition(position),
        mVelocity(0, 0, 0),
        mMovement(0, 0, 0),
        mAcceleration(0, 0, 0),
        mRotation(rotation),
        mSpeed(speed),
        mDumping(0.001f),
        mGrounded(false)
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

        mView = XMMatrixLookAtLH(pos, pos + mFront, mWorldUp);
    }

    FPSCamera::~FPSCamera()
    {
    
    }

    XMFLOAT3 FPSCamera::GetPosition()
    {
        return mRenderPosition;
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

    void CollisionDetection(XMVECTOR pos, XMVECTOR vel, float dt,
        Physics::Triangle* triangles, int count, 
        float& outT, unsigned& collisionCount, XMVECTOR& n)
    {
        collisionCount = 0;
        Ruby::Physics::real smallesT = REAL_MAX;

        XMVECTOR diff = vel * dt;

        Physics::Vector3 movement = Ruby::Physics::Vector3(diff.m128_f32[0], diff.m128_f32[1], diff.m128_f32[2]);

        Physics::Sphere sphere;
        sphere.c = Ruby::Physics::Vector3(pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]);
        sphere.r = 0.75f;

        Physics::Segment segment;
        segment.a = Ruby::Physics::Vector3(pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]);
        segment.b = segment.a + Ruby::Physics::Vector3(diff.m128_f32[0], diff.m128_f32[1], diff.m128_f32[2]);

        // Collision Detection
        // TODO: see if all the t values are in the same range
        for (int i = 0; i < count; ++i)
        {
            Ruby::Physics::Triangle triangle = triangles[i];

            float t = -1.0f;
            Physics::Point q;
            sphere.MovingSpherePlane(movement, triangle.GetPlane(), t, q);

            if (t >= 0.0f && t <= 1.0f)
            {
                if (q.InTriangle(triangle))
                {
                    if (t <= smallesT)
                    {
                        smallesT = t;
                        Ruby::Physics::Vector3 normal = (triangle.b - triangle.a).VectorProduct(triangle.c - triangle.a);
                        normal.Normalize();
                        n = XMVectorSet(normal.x, normal.y, normal.z, 0.0);
                        ++collisionCount;
                    }   
                }
                else
                {
                    Physics::Capsule capsule[3];
                    capsule[0].a = triangle.a;
                    capsule[0].b = triangle.b;
                    capsule[0].r = sphere.r - 0.05f;
                    Physics::Capsule capsule1;
                    capsule[1].a = triangle.b;
                    capsule[1].b = triangle.c;
                    capsule[1].r = sphere.r - 0.05f;
                    Physics::Capsule capsule2;
                    capsule[2].a = triangle.c;
                    capsule[2].b = triangle.a;
                    capsule[2].r = sphere.r - 0.05f;

                    for (int i = 0; i < 3; ++i)
                    {
                        if (segment.IntersectCapsule(capsule[i], t, q))
                        {
                            if (t < smallesT)
                            {
                                Physics::Segment segment;
                                segment.a = capsule[i].a;
                                segment.b = capsule[i].b;
                                smallesT = t;
                                Ruby::Physics::Vector3 normal = q - q.ClosestPointSegement(segment);
                                normal.Normalize();
                                n = XMVectorSet(normal.x, normal.y, normal.z, 0.0);
                                ++collisionCount;
                            }
                        }
                    }

                }
            }

        }
        outT = smallesT;
    }

    void ProccessCollisionDetectionAndResolution(XMVECTOR& pos, XMVECTOR&potPos, XMVECTOR vel, float dt,
        Physics::Triangle* triangles, int count)
    {
        Ruby::Physics::real t = REAL_MAX;
        XMVECTOR n = XMVectorSet(0, 0, 0, 0);

        unsigned int collisionCount = 0;
        CollisionDetection(pos, vel, dt, triangles, count, t, collisionCount, n);

        unsigned int iterations = 0;
#if 1
        while (XMVector3LengthSq(vel).m128_f32[0] > FLT_EPSILON)
        {
            if (collisionCount)
            {
                n = XMVector3Normalize(n);
                pos = (pos + ((vel * dt) * t)) + (n * 0.005f);
                vel = vel - (n * XMVector3Dot(vel, n));
                dt = dt * (1.0f - t);
                CollisionDetection(pos, vel, dt, triangles, count, t, collisionCount, n);
            }
            else
            {
                pos = pos + (vel * dt);
                break;
            }
            ++iterations;
        }
#else
        while (collisionCount && iterations < 20)
        {
            if (iterations == 0)
            {
                int StopHere = 0;
            }

            n = XMVector3Normalize(n);
            pos = (pos + ((vel * dt) * t)) + (n * 0.005f);
            vel = vel - (n * XMVector3Dot(vel, n));
            XMVECTOR scaleVelocity = vel * (1.0f - t);
            potPos = pos + scaleVelocity * dt;
            ++iterations;
            CollisionDetection(pos, vel, dt, triangles, count, t, collisionCount, n);
        }
#endif        
    }

    void GroundDetection(Physics::Ray& ray,
        bool& grounded, XMFLOAT3& vel, XMFLOAT3& acc,
        Physics::Triangle* triangles, int count)
    {
        grounded = false;
        for (int i = 0; i < count; ++i)
        {
            Physics::Triangle triangle = triangles[i];
            Physics::real t = ray.RaycastTriangle(triangle);
            if (t >= 0.0f && t <= 1.0f)
            {
                grounded = true;
                if (vel.y < 0)
                {
                    vel.y = 0;
                    acc.y = 0;
                }
                break;
            }
        }
    }

    void FPSCamera::Update(float dt)
    {
        mFront = XMVector3Transform(mWorldFront, XMMatrixRotationX(mRotation.x));
        mFront = XMVector3Transform(mFront, XMMatrixRotationY(mRotation.y));
        mFront = XMVector3Normalize(XMVector3Transform(mFront, XMMatrixRotationZ(mRotation.z)));
        mRight = XMVector3Normalize(XMVector3Cross(mWorldUp, mFront));
        mUp = XMVector3Normalize(XMVector3Cross(mRight, mFront));
        mDirection = XMVector3Normalize(XMVector3Cross(mRight, mWorldUp));

        XMVECTOR mov = XMVectorSet(mMovement.x, mMovement.y, mMovement.z, 0.0f);
        mov = XMVector3Normalize(mov) * mSpeed;
        XMStoreFloat3(&mMovement, mov);
        if (!mGrounded)
        {
            mMovement.x *= 0.1f;
            mMovement.z *= 0.1f;
        }
        ApplyForce(mMovement);
        mMovement = XMFLOAT3(0, 0, 0);
    
    }

    void FPSCamera::FixUpdate(float dt, Ruby::Physics::Triangle* triangles, int count)
    {
        mLastPosition = mPosition;
        
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);
        XMVECTOR acc = XMVectorSet(mAcceleration.x, mAcceleration.y, mAcceleration.z, 0.0f);
        XMVECTOR vel = XMVectorSet(mVelocity.x, mVelocity.y, mVelocity.z, 0.0f);

        vel = vel + acc * dt;
        
        if (mGrounded)
        {
            float dammping = powf(0.001f, dt);
            vel.m128_f32[0] = vel.m128_f32[0] * dammping;
            vel.m128_f32[2] = vel.m128_f32[2] * dammping;
        }
        else
        {
            float dammping = powf(0.5f, dt);
            vel.m128_f32[0] = vel.m128_f32[0] * dammping;
            vel.m128_f32[2] = vel.m128_f32[2] * dammping;
        }

        XMVECTOR potPos;
        potPos = pos + (vel * dt);

        Physics::Ray ray;
        ray.o = Physics::Vector3(mPosition.x, mPosition.y, mPosition.z);
        ray.d = Physics::Vector3(0, -1, 0) * (0.75f + 0.15);

        XMStoreFloat3(&mVelocity, vel);

        GroundDetection(ray, mGrounded, mVelocity, mAcceleration, triangles, count);
        
        vel = XMVectorSet(mVelocity.x, mVelocity.y, mVelocity.z, 0.0f);

        ProccessCollisionDetectionAndResolution(pos, potPos, vel, dt, triangles, count);
        
        XMStoreFloat3(&mVelocity, vel);
        XMStoreFloat3(&mPotentialPosition, pos);
        XMStoreFloat3(&mPosition, pos);


        char buffer[256];
        sprintf_s(buffer, "mVelocity: (%f, %f, %f)\n", mVelocity.x, mVelocity.y, mVelocity.z);
        OutputDebugStringA(buffer);
        sprintf_s(buffer, "mAcceleration: (%f, %f, %f)\n", mAcceleration.x, mAcceleration.y, mAcceleration.z);
        OutputDebugStringA(buffer);

    }

    void FPSCamera::PostUpdate(float t)
    {   
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);
        XMVECTOR lastPos = XMVectorSet(mLastPosition.x, mLastPosition.y, mLastPosition.z, 1.0f);

        XMVECTOR renderPos = pos * t + lastPos * (1.0f - t);
        mView = XMMatrixLookAtLH(renderPos, renderPos + mFront, mWorldUp);

        XMStoreFloat3(&mPosition, pos);
        XMStoreFloat3(&mRenderPosition, renderPos);
        mAcceleration = XMFLOAT3(0, 0, 0); 
    }

    void FPSCamera::MouseMove(float mouseX, float mouseY, float dt)
    {
        mRotation.y += mouseX;
        mRotation.x += mouseY;

        if (mRotation.x > (89.0f/180.0f) * XM_PI)
            mRotation.x = (89.0f / 180.0f) * XM_PI;
        if (mRotation.x < -(89.0f / 180.0f) * XM_PI)
            mRotation.x = -(89.0f/180.0f) * XM_PI;
    }

    void FPSCamera::MoveForward()
    {
        XMVECTOR mov = XMVectorSet(mMovement.x, mMovement.y, mMovement.z, 0.0f);
        mov = mov + mDirection;
        XMStoreFloat3(&mMovement, mov);
    }

    void FPSCamera::MoveBackward()
    {
        XMVECTOR mov = XMVectorSet(mMovement.x, mMovement.y, mMovement.z, 0.0f);
        mov = mov - mDirection;
        XMStoreFloat3(&mMovement, mov);
    }

    void FPSCamera::MoveLeft()
    {
        XMVECTOR mov = XMVectorSet(mMovement.x, mMovement.y, mMovement.z, 0.0f);
        mov = mov - mRight;
        XMStoreFloat3(&mMovement, mov);
    }

    void FPSCamera::MoveRight()
    {
        XMVECTOR mov = XMVectorSet(mMovement.x, mMovement.y, mMovement.z, 0.0f);
        mov = mov + mRight;
        XMStoreFloat3(&mMovement, mov);
    }

    void FPSCamera::ApplyForce(XMFLOAT3 force)
    {
        mAcceleration.x += force.x;
        mAcceleration.y += force.y;
        mAcceleration.z += force.z;
    }

    void FPSCamera::ApplyImpulse(XMFLOAT3  impulse)
    {
        mVelocity.x += impulse.x;
        mVelocity.y += impulse.y;
        mVelocity.z += impulse.z;
    }
    
    bool FPSCamera::Grounded()
    {
        return mGrounded;
    }


}