#pragma once

#include <d3dx11Effect.h>

namespace Ruby
{
    class Effect
    {
    public:
        Effect(ID3D11Device* device, const char* filepath, const char* techName);
        ~Effect();
        ID3DX11EffectTechnique* GetTechnique();

    protected:
        ID3DX11Effect* mEffect;
        ID3DX11EffectTechnique* mTechnique;
    };

    class BaseEffect : public Effect
    {
    public:
        BaseEffect(ID3D11Device* device);

        ID3DX11EffectMatrixVariable* mWorld;
        ID3DX11EffectMatrixVariable* mWorldInvTranspose;
        ID3DX11EffectMatrixVariable* mWorldViewProj;
        ID3DX11EffectVariable* mDirLight;
        ID3DX11EffectVariable* mPointLight;
        ID3DX11EffectVariable* mMaterial;
        ID3DX11EffectVectorVariable* mEyePosW;
        ID3DX11EffectMatrixVariable* mLightSpaceMatrix;
        ID3DX11EffectShaderResourceVariable* mShadowMap;
        ID3DX11EffectShaderResourceVariable* mIrradianceMap;
        ID3DX11EffectShaderResourceVariable* mPrefilteredColor;
        ID3DX11EffectShaderResourceVariable* mBrdfLUT;
    };

    class DepthEffect : public Effect
    {
    public:
        DepthEffect(ID3D11Device* device);

        ID3DX11EffectMatrixVariable* mWorld;
        ID3DX11EffectMatrixVariable* mLightSpaceMatrix;
    };

    class HdrEffect : public Effect
    {
    public:
        HdrEffect(ID3D11Device* device);

        ID3DX11EffectShaderResourceVariable* mBackBuffer;
        ID3DX11EffectShaderResourceVariable* mBloomBuffer;
        ID3DX11EffectVariable* mTimer;
    };

    class BlurEffect : public Effect
    {
    public:
        BlurEffect(ID3D11Device* device);

        ID3DX11EffectShaderResourceVariable* mImage;
        ID3DX11EffectVariable* mHorizontal;
    };

    class CubemapEffect : public Effect
    {
    public:
        CubemapEffect(ID3D11Device* device);

        ID3DX11EffectMatrixVariable* mWorldViewProj;
        ID3DX11EffectVariable* mRoughness;
        ID3DX11EffectShaderResourceVariable* mCubeMap;
    };

    class ConvoluteEffect : public Effect
    {
    public:
        ConvoluteEffect(ID3D11Device* device);

        ID3DX11EffectMatrixVariable* mWorldViewProj;
        ID3DX11EffectShaderResourceVariable* mCubeMap;
    };

    class SkyEffect : public Effect
    {
    public:
        SkyEffect(ID3D11Device* device);
        ID3DX11EffectMatrixVariable* mWorldViewProj;
        ID3DX11EffectShaderResourceVariable* mCubeMap;
        ID3DX11EffectVariable* mTimer;
    };

    class BrdfEffect : public Effect
    {
    public:
        BrdfEffect(ID3D11Device* device);
    };

}

