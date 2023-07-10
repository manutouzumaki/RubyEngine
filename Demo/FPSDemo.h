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
    
    ID3D11RasterizerState* mRasterizerStateBackCull;
    ID3D11RasterizerState* mRasterizerStateFrontCull;

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
    ID3DX11EffectShaderResourceVariable* mFxIrradianceMap;
    ID3DX11EffectShaderResourceVariable* mFxPrefilteredColor;
    ID3DX11EffectShaderResourceVariable* mFxBrdfLUT;

    ID3DX11Effect* mCubemapEffect;
    ID3DX11EffectTechnique* mCubemapTechnique;
    ID3DX11EffectMatrixVariable* mCubemapWorldViewProj;
    ID3DX11EffectVariable* mCubemapRoughness;

    ID3DX11EffectShaderResourceVariable* mCubeMap;

    ID3DX11Effect* mConvoluteEffect;
    ID3DX11EffectTechnique* mConvoluteTechnique;
    ID3DX11EffectMatrixVariable* mConvoluteWorldViewProj;
    ID3DX11EffectShaderResourceVariable* mConvoluteCubeMap;

    ID3DX11Effect* mSkyEffect;
    ID3DX11EffectTechnique* mSkyTechnique;
    ID3DX11EffectMatrixVariable* mSkyWorldViewProj;
    ID3DX11EffectShaderResourceVariable* mSkyCubeMap;
    ID3DX11EffectVariable* mSkyTimer;

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

    ID3DX11Effect* mBrdfEffect;
    ID3DX11EffectTechnique* mBrdfTechnique;

    ID3D11InputLayout* mInputLayout;

    XMFLOAT4X4 mWorld;
    XMFLOAT4X4 mView;
    XMFLOAT4X4 mProj;


    Ruby::Pbr::Material mMaterials[49];

    Ruby::MeshGeometry mSky;
    ID3D11Texture2D* mHdrSkyTexture2D;
    ID3D11ShaderResourceView* mHdrSkySRV;

    Ruby::CubeFrameBuffer* mEnviromentMap;
    Ruby::CubeFrameBuffer* mIrradianceMap;
    Ruby::FrameBuffer* mBrdfMap;


    Ruby::Mesh* mMesh[7];

    ShadowMap* mShadowMap;
    Ruby::FrameBuffer* mFrameBuffers[2];
    Ruby::FrameBuffer* mPinPongFrameBuffers[2];

    Ruby::Pbr::DirectionalLight mDirLight;
    Ruby::Pbr::PointLight mPointLight;

    ID3D11ShaderResourceView* mCubeMapSRV;

};