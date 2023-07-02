#pragma once

#include "../RubyApp.h"
#include "../RubyMesh.h"

#include "../ShadowMap.h"
#include "../RubyFrameBuffer.h"

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
    void UpdateScene(float dt);
    void DrawScene();

    void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM btnState, int x, int y);
    void OnKeyDown(WPARAM vkCode);
    void OnKeyUp(WPARAM vkCode);


private:
    

    ID3D11Buffer* mVertexBuffer;
    ID3D11Buffer* mIndexBuffer;

    ID3DX11Effect* mEffect;
    ID3DX11EffectTechnique* mTechnique;
    ID3DX11EffectMatrixVariable* mFxWorld;
    ID3DX11EffectMatrixVariable* mFxWorldInvTranspose;
    ID3DX11EffectMatrixVariable* mFxWorldViewProj;
    ID3DX11EffectVariable* mFxDirLight;
    ID3DX11EffectVariable* mFxPointLight;
    ID3DX11EffectVariable* mFxMaterial;
    ID3DX11EffectVectorVariable* mFxEyePosW;
    ID3DX11EffectMatrixVariable* mFxLightSpaceMatrix;
    ID3DX11EffectShaderResourceVariable* mFxShadowMap;


    ID3DX11Effect* mDepthEffect;
    ID3DX11EffectTechnique* mDepthTechnique;
    ID3DX11EffectMatrixVariable* mDepthFxWorld;
    ID3DX11EffectMatrixVariable* mDepthFxLightSpaceMatrix;

    ID3DX11Effect* mHdrEffect;
    ID3DX11EffectTechnique* mHdrTechnique;
    ID3DX11EffectShaderResourceVariable* mHdrBackBuffer;
    ID3DX11EffectShaderResourceVariable* mBloomBuffer;

    ID3DX11Effect* mBlurEffect;
    ID3DX11EffectTechnique* mBlurTechnique;
    ID3DX11EffectShaderResourceVariable* mBlurFxImage;
    ID3DX11EffectVariable* mBlurFxHorizontal;

    ID3D11InputLayout* mInputLayout;

    XMFLOAT4X4 mWorld;
    XMFLOAT4X4 mView;
    XMFLOAT4X4 mProj;

    Ruby::Mesh* mMesh[6];

    ShadowMap* mShadowMap;
    Ruby::FrameBuffer* mFrameBuffers[2];
    Ruby::FrameBuffer* mPinPongFrameBuffers[2];

    Ruby::DirectionalLight mDirLight;
    Ruby::PointLight mPointLight;

};