#pragma once

#include <DirectXMath.h>

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

        XMFLOAT3 mPosition;
        XMFLOAT3 mRotation;

        float mSpeed;
 
    public:
        FPSCamera(XMFLOAT3 position, XMFLOAT3 rotation, float speed);
        ~FPSCamera();

        XMMATRIX GetView();
        XMFLOAT3 GetPosition();

        void Update(float dt);
        void MouseMove(float mouseX, float mouseY);
        void MoveForward(float dt);
        void MoveBackward(float dt);
        void MoveLeft(float dt);
        void MoveRight(float dt);
        void MoveUp(float dt);
        void MoveDown(float dt);

    };

}


