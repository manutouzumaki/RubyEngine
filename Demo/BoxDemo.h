#pragma once

#include "../RubyApp.h"

class BoxDemo : public Ruby::App
{
public:

    BoxDemo(HINSTANCE instance,
            UINT clientWidth,
            UINT clientHeight,
            const char* windowCaption,
            bool enable4xMsaa);
    ~BoxDemo();

    bool Init();
    void OnResize();
    void UpdateScene();
    void DrawScene();

private:

    Ruby::MeshData mCubeData;

    ID3D11Buffer* mVertexBuffer;
    ID3D11Buffer* mIndexBuffer;

    ID3DX11Effect* mEffect;
    ID3DX11EffectTechnique* mTechnique;
    ID3DX11EffectMatrixVariable* mFxWorldViewProj;
    ID3DX11EffectMatrixVariable* mFxWorld;

    ID3D11InputLayout* mInputLayout;

    XMFLOAT4X4 mWorld;
    XMFLOAT4X4 mView;
    XMFLOAT4X4 mProj;
};


