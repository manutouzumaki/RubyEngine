#pragma once

#include "../RubyApp.h"
#include "../RubyMesh.h"

class PBRDemo : public Ruby::App
{
public:
    PBRDemo(HINSTANCE instance,
        UINT clientWidth,
        UINT clientHeight,
        const char* windowCaption,
        bool enable4xMsaa);

    ~PBRDemo();

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
    ID3DX11Effect* mPbrEffect;
    ID3DX11EffectTechnique* mPbrTechnique;

    ID3DX11EffectVariable* mPbrMaterial;
    ID3DX11EffectVariable* mPbrPointLight0;
    ID3DX11EffectVariable* mPbrPointLight1;
    ID3DX11EffectVariable* mPbrPointLight2;
    ID3DX11EffectVariable* mPbrPointLight3;
    ID3DX11EffectMatrixVariable* mPbrWorld;
    ID3DX11EffectMatrixVariable* mPbrWorldViewProj;
    ID3DX11EffectMatrixVariable* mPbrWorldInvTranspose;
    ID3DX11EffectVectorVariable* mPbrCamPos;

    ID3D11InputLayout* mInputLayout;

    XMFLOAT4X4 mWorld;
    XMFLOAT4X4 mView;
    XMFLOAT4X4 mProj;

    Ruby::MeshGeometry* mSphere;
    Ruby::Pbr::PointLight mPointLights[4];
    Ruby::Pbr::Material mMaterials[49];

};

