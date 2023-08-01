#pragma once

#include "../RubyApp.h"
#include "../RubyMesh.h"

#include "../ShadowMap.h"
#include "../RubyFrameBuffer.h"
#include "../RubyScene.h"
#include "../RubyCamera.h"
#include "../RubyWorkQueue.h"
#include "../RubyEffect.h"

class FPSDemo : public Ruby::App
{
public:

    FPSDemo(HINSTANCE instance,
        UINT clientWidth,
        UINT clientHeight,
        const char* windowCaption,
        bool enable4xMsaa);

    ~FPSDemo();

    bool Init();
    void OnResize();
    void UpdateScene();
    void DrawScene();

    void SplitGeometryFast(Ruby::OctreeNode<Ruby::SceneStaticObject>* node);

private:

    ID3D11RasterizerState* mRasterizerStateBackCull;
    ID3D11RasterizerState* mRasterizerStateFrontCull;

    Ruby::PbrColorEffect* mPbrColorEffect;
    Ruby::PbrTextureEffect* mPbrTextureEffect;
    Ruby::DepthEffect* mDepthEffect;
    Ruby::HdrEffect* mHdrEffect;
    Ruby::BlurEffect* mBlurEffect;
    Ruby::CubemapEffect* mCubemapEffect;
    Ruby::ConvoluteEffect* mConvoluteEffect;
    Ruby::SkyEffect* mSkyEffect;
    Ruby::BrdfEffect* mBrdfEffect;

    ID3D11InputLayout* mInputLayout;

    XMFLOAT4X4 mWorld;
    XMFLOAT4X4 mView;
    XMFLOAT4X4 mProj;

    Ruby::MeshGeometry mSky;
    ID3D11Texture2D* mHdrSkyTexture2D;
    ID3D11ShaderResourceView* mHdrSkySRV;

    ID3D11Texture2D* mPbrTextures2D[4];
    ID3D11ShaderResourceView* mPbrSRVs[4];

    Ruby::CubeFrameBuffer* mEnviromentMap;
    Ruby::CubeFrameBuffer* mIrradianceMap;
    Ruby::FrameBuffer* mBrdfMap;

    Ruby::Mesh* mMesh;
    Ruby::Mesh* mGunMesh;
    Ruby::Mesh* mCollider;

    ShadowMap* mShadowMap;
    Ruby::FrameBuffer* mFrameBuffers[2];
    Ruby::FrameBuffer* mPinPongFrameBuffers[2];

    Ruby::Pbr::DirectionalLight mDirLight;
    Ruby::Pbr::PointLight mPointLight;

    Ruby::Scene* mScene;

    Ruby::FPSCamera* mCamera;

    Ruby::SplitGeometryWorkQueue mQueue;
};