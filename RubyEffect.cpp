#include "RubyEffect.h"

#include "RubyDefines.h"

#include <d3dx11.h>

namespace Ruby
{
    Effect::Effect(ID3D11Device* device, const char* filepath, const char* techName)
        : mEffect(nullptr), mTechnique(nullptr)
    {
        // create the shaders and fx
        DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
        shaderFlags |= D3D10_SHADER_DEBUG;
#endif
        shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;

        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile(filepath, 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, device, &mEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mTechnique = mEffect->GetTechniqueByName(techName);
    }

    Effect::~Effect()
    {
        SAFE_RELEASE(mEffect);
    }

    ID3DX11EffectTechnique* Effect::GetTechnique()
    {
        return mTechnique;
    }

    PbrColorEffect::PbrColorEffect(ID3D11Device* device)
        : Effect(device, "./FX/pbrColor.fx", "ColorTech")
    {
        mWorld = mEffect->GetVariableByName("gWorld")->AsMatrix();
        mWorldInvTranspose = mEffect->GetVariableByName("gWorldInvTranspose")->AsMatrix();
        mWorldViewProj = mEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mMaterial = mEffect->GetVariableByName("gMaterial");
        mDirLight = mEffect->GetVariableByName("gDirLight");
        mPointLight = mEffect->GetVariableByName("gPointLight");
        mEyePosW = mEffect->GetVariableByName("gEyePosW")->AsVector();
        mLightSpaceMatrix = mEffect->GetVariableByName("gLightSpaceMatrix")->AsMatrix();
        mShadowMap = mEffect->GetVariableByName("gShadowMap")->AsShaderResource();
        mIrradianceMap = mEffect->GetVariableByName("gIrradianceMap")->AsShaderResource();
        mPrefilteredColor = mEffect->GetVariableByName("gPrefilteredColor")->AsShaderResource();
        mBrdfLUT = mEffect->GetVariableByName("gBrdfLUT")->AsShaderResource();
    }

    PbrTextureEffect::PbrTextureEffect(ID3D11Device* device)
        : Effect(device, "./FX/pbrTexture.fx", "TextureTech")
    {
        mWorld = mEffect->GetVariableByName("gWorld")->AsMatrix();
        mWorldInvTranspose = mEffect->GetVariableByName("gWorldInvTranspose")->AsMatrix();
        mWorldViewProj = mEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mDirLight = mEffect->GetVariableByName("gDirLight");
        mPointLight = mEffect->GetVariableByName("gPointLight");
        mEyePosW = mEffect->GetVariableByName("gEyePosW")->AsVector();
        mLightSpaceMatrix = mEffect->GetVariableByName("gLightSpaceMatrix")->AsMatrix();
        mShadowMap = mEffect->GetVariableByName("gShadowMap")->AsShaderResource();
        mIrradianceMap = mEffect->GetVariableByName("gIrradianceMap")->AsShaderResource();
        mPrefilteredColor = mEffect->GetVariableByName("gPrefilteredColor")->AsShaderResource();
        mBrdfLUT = mEffect->GetVariableByName("gBrdfLUT")->AsShaderResource();
        mAlbedoMap = mEffect->GetVariableByName("gAlbedoMap")->AsShaderResource();
        mMetallicMap = mEffect->GetVariableByName("gMetallicMap")->AsShaderResource();
        mRoughnessMap = mEffect->GetVariableByName("gRoughnessMap")->AsShaderResource();
        mNormalMap = mEffect->GetVariableByName("gNormalMap")->AsShaderResource();
    }

    DepthEffect::DepthEffect(ID3D11Device* device)
        : Effect(device, "./FX/depth.fx", "DepthTech")
    {
        mWorld = mEffect->GetVariableByName("gWorld")->AsMatrix();
        mLightSpaceMatrix = mEffect->GetVariableByName("gLightSpaceMatrix")->AsMatrix();
    }

    HdrEffect::HdrEffect(ID3D11Device* device)
        : Effect(device, "./FX/hdr.fx", "HdrTech")
    {
        mBackBuffer = mEffect->GetVariableByName("gBackBuffer")->AsShaderResource();
        mBloomBuffer = mEffect->GetVariableByName("gBloomBuffer")->AsShaderResource();
        mTimer = mEffect->GetVariableByName("gTimer");
    }

    BlurEffect::BlurEffect(ID3D11Device* device)
        : Effect(device, "./FX/blur.fx", "BlurTech")
    {
        mHorizontal = mEffect->GetVariableByName("horizontal");
        mImage = mEffect->GetVariableByName("gImage")->AsShaderResource();
    }

    CubemapEffect::CubemapEffect(ID3D11Device* device)
        : Effect(device, "./FX/cubemap.fx", "CubeMapTech")
    {
        mWorldViewProj = mEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mRoughness = mEffect->GetVariableByName("gRoughness");
        mCubeMap = mEffect->GetVariableByName("gCubeMap")->AsShaderResource();
    }

    ConvoluteEffect::ConvoluteEffect(ID3D11Device* device)
        : Effect(device, "./FX/convolute.fx", "ConvoluteTech")
    {
        mWorldViewProj = mEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mCubeMap = mEffect->GetVariableByName("gCubeMap")->AsShaderResource();
    }

    SkyEffect::SkyEffect(ID3D11Device* device)
        : Effect(device, "./FX/sky.fx", "SkyTech")
    {
        mWorldViewProj = mEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mCubeMap = mEffect->GetVariableByName("gCubeMap")->AsShaderResource();
        mTimer = mEffect->GetVariableByName("gTimer");
    }

    BrdfEffect::BrdfEffect(ID3D11Device* device)
        : Effect(device, "./FX/brdf.fx", "BrdfTech")
    {
    }
}