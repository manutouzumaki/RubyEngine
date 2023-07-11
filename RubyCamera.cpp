#include "RubyCamera.h"


namespace Ruby
{



    FPSCamera::FPSCamera(XMFLOAT3 position, XMFLOAT3 rotation, float speed)
        :
        mPosition(position),
        mRotation(rotation),
        mSpeed(speed)
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

        mView = XMMatrixLookAtLH(pos,  pos + mFront, mWorldUp);
    }

    FPSCamera::~FPSCamera()
    {
    
    }

    XMFLOAT3 FPSCamera::GetPosition()
    {
        return mPosition;
    }

    XMMATRIX FPSCamera::GetView()
    {
        return mView;
    }

    void FPSCamera::Update(float dt)
    {
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);

        mFront = XMVector3Transform(mWorldFront, XMMatrixRotationX(mRotation.x));
        mFront = XMVector3Transform(mFront, XMMatrixRotationY(mRotation.y));
        mFront = XMVector3Transform(mFront, XMMatrixRotationZ(mRotation.z));
        mRight = XMVector3Normalize(XMVector3Cross(mWorldUp, mFront));
        mUp = XMVector3Normalize(XMVector3Cross(mRight, mFront));

        mView = XMMatrixLookAtLH(pos, pos + mFront, mWorldUp);
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
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);
        pos = pos + mFront * mSpeed * dt;
        XMStoreFloat3(&mPosition, pos);
    }

    void FPSCamera::MoveBackward(float dt)
    {
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);
        pos = pos - mFront * mSpeed * dt;
        XMStoreFloat3(&mPosition, pos);
    }

    void FPSCamera::MoveLeft(float dt)
    {
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);
        pos = pos - mRight * mSpeed * dt;
        XMStoreFloat3(&mPosition, pos);
    }

    void FPSCamera::MoveRight(float dt)
    {
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);
        pos = pos + mRight * mSpeed * dt;
        XMStoreFloat3(&mPosition, pos);
    }

    void FPSCamera::MoveUp(float dt)
    {
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);
        pos = pos + mWorldUp * mSpeed * dt;
        XMStoreFloat3(&mPosition, pos);
    }

    void FPSCamera::MoveDown(float dt)
    {
        XMVECTOR pos = XMVectorSet(mPosition.x, mPosition.y, mPosition.z, 1.0f);
        pos = pos - mWorldUp * mSpeed * dt;
        XMStoreFloat3(&mPosition, pos);
    }

    

}