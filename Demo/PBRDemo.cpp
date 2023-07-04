#include "PBRDemo.h"

#include <algorithm>

const int gNrRows = 7;
const int gNrCols = 7;
const float gSpacing = 2.5f;

static XMMATRIX InverseTranspose(CXMMATRIX M)
{
    // Inverse-transpose is just applied to normals.  So zero out 
    // translation row so that it doesn't get into our inverse-transpose
    // calculation--we don't want the inverse-transpose of the translation.
    XMMATRIX A = M;
    A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    XMVECTOR det = XMMatrixDeterminant(A);
    return XMMatrixTranspose(XMMatrixInverse(&det, A));
}

PBRDemo::PBRDemo(HINSTANCE instance,
    UINT clientWidth,
    UINT clientHeight,
    const char* windowCaption,
    bool enable4xMsaa)
    : Ruby::App(instance, clientWidth, clientHeight, windowCaption, enable4xMsaa)
{
    XMMATRIX identity = XMMatrixIdentity();
    XMStoreFloat4x4(&mWorld, identity);
    XMStoreFloat4x4(&mView, identity);
    XMStoreFloat4x4(&mProj, identity);
}

PBRDemo::~PBRDemo()
{
    SAFE_RELEASE(mPbrEffect);
    SAFE_RELEASE(mInputLayout);
    SAFE_DELETE(mSphere);
}

bool PBRDemo::Init()
{
    if (!Ruby::App::Init())
        return false;

    // Load sphere model ...
    {
        Ruby::GeometryGenerator generator = Ruby::GeometryGenerator();
        Ruby::MeshData meshData{};
        generator.CreateGeosphere(1.0f, 5, meshData);

        std::vector<Ruby::MeshGeometry::Subset> subsetTable;
        Ruby::MeshGeometry::Subset subset;
        subset.Id = 0;
        subset.VertexStart = 0;
        subset.VertexCount = meshData.Vertices.size();
        subset.IndexStart = 0;
        subset.IndexCount = meshData.Indices.size();
        subsetTable.push_back(subset);

        mSphere = new Ruby::MeshGeometry();
        mSphere->SetVertices(mDevice, meshData.Vertices.data(), meshData.Vertices.size());
        mSphere->SetIndices(mDevice, meshData.Indices.data(), meshData.Indices.size());
        mSphere->SetSubsetTable(subsetTable);
    }

    // Create Color Effect
    {
        // create the shaders and fx
        DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
        shaderFlags |= D3D10_SHADER_DEBUG;
        shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/pbr.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mPbrEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mPbrTechnique = mPbrEffect->GetTechniqueByName("PbrTech");
        mPbrMaterial = mPbrEffect->GetVariableByName("gMaterial");
        mPbrPointLight0 = mPbrEffect->GetVariableByName("gPointLight0");
        mPbrPointLight1 = mPbrEffect->GetVariableByName("gPointLight1");
        mPbrPointLight2 = mPbrEffect->GetVariableByName("gPointLight2");
        mPbrPointLight3 = mPbrEffect->GetVariableByName("gPointLight3");
        mPbrWorld = mPbrEffect->GetVariableByName("gWorld")->AsMatrix();
        mPbrWorldViewProj = mPbrEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mPbrWorldInvTranspose = mPbrEffect->GetVariableByName("gWorldInvTranspose")->AsMatrix();
        mPbrCamPos = mPbrEffect->GetVariableByName("gCamPos")->AsVector();
    }

    // set the vertex Layout
    {
        D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        D3DX11_PASS_DESC passDesc;
        mPbrTechnique->GetPassByIndex(0)->GetDesc(&passDesc);
        HRESULT result = mDevice->CreateInputLayout(vertexDesc, 4, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout);
    }

    // set view proj matrices
    {
        // set proj matrices
        XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
        XMStoreFloat4x4(&mProj, P);

        // Build the view matrix.
        XMFLOAT3 eyePos = XMFLOAT3(0.0f, 0.0f, -20.0f);
        XMVECTOR pos = XMVectorSet(eyePos.x, eyePos.y, eyePos.z, 0.0f);
        XMVECTOR target = XMVectorZero();
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
        XMStoreFloat4x4(&mView, V);

        mPbrCamPos->SetRawValue(&eyePos, 0, sizeof(XMFLOAT3));
    }

    // set lights and matrials
    {
        mPointLights[0].Position = XMFLOAT3(-10.0f, 10.0f, -10.0f);
        mPointLights[1].Position = XMFLOAT3(10.0f, 10.0f, -10.0f);
        mPointLights[2].Position = XMFLOAT3(-10.0f, -10.0f, -10.0f);
        mPointLights[3].Position = XMFLOAT3(10.0f, -10.0f, -10.0f);

        mPointLights[0].Color = XMFLOAT4(300.0f, 300.0f, 300.0f, 0.0f);
        mPointLights[1].Color = XMFLOAT4(300.0f, 300.0f, 300.0f, 0.0f);
        mPointLights[2].Color = XMFLOAT4(300.0f, 300.0f, 300.0f, 0.0f);
        mPointLights[3].Color = XMFLOAT4(300.0f, 300.0f, 300.0f, 0.0f);

        mPbrPointLight0->SetRawValue(&mPointLights[0], 0, sizeof(Ruby::Pbr::PointLight));
        mPbrPointLight1->SetRawValue(&mPointLights[1], 0, sizeof(Ruby::Pbr::PointLight));
        mPbrPointLight2->SetRawValue(&mPointLights[2], 0, sizeof(Ruby::Pbr::PointLight));
        mPbrPointLight3->SetRawValue(&mPointLights[3], 0, sizeof(Ruby::Pbr::PointLight));

        for (int row = 0; row < gNrRows; ++row)
        {
            float matallic = (float)row / (float)gNrRows;
            for (int col = 0; col < gNrCols; ++col)
            {
                Ruby::Pbr::Material material{};
                material.Albedo = XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f);
                material.Metallic = matallic;
                material.Roughness = std::clamp((float)col / (float)gNrCols, 0.05f, 1.0f);
                material.Ao = 1.0f;
                mMaterials[row * gNrRows + col] = material;

            }
        }
    }

}

void PBRDemo::OnResize()
{
    Ruby::App::OnResize();
    // set proj matrices
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void PBRDemo::UpdateScene(float dt)
{


}

void PBRDemo::DrawScene()
{
    mImmediateContext->IASetInputLayout(mInputLayout);
    mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    XMVECTORF32 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    mImmediateContext->ClearRenderTargetView(mRenderTargetView, (float*)&clearColor);
    mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    XMMATRIX viewProj = XMLoadFloat4x4(&mView) * XMLoadFloat4x4(&mProj);

    D3DX11_TECHNIQUE_DESC techDesc;
    mPbrTechnique->GetDesc(&techDesc);
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        for(int row = 0; row < gNrRows; ++row)
        {
            for (int col = 0; col < gNrCols; ++col)
            {
                XMMATRIX world = XMMatrixTranslation(
                    (col - (gNrCols / 2)) * gSpacing,
                    (row - (gNrRows / 2)) * gSpacing,
                    0.0f
                );
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = world * viewProj;
                mPbrWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mPbrWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mPbrWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                mPbrMaterial->SetRawValue(&mMaterials[row * gNrRows + col], 0, sizeof(Ruby::Pbr::Material));

                mPbrTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                mSphere->Draw(mImmediateContext, 0);
            }
        }
    }

    mSwapChain->Present(1, 0);
}

void PBRDemo::OnMouseDown(WPARAM btnState, int x, int y)
{

}

void PBRDemo::OnMouseUp(WPARAM btnState, int x, int y)
{

}

void PBRDemo::OnMouseMove(WPARAM btnState, int x, int y)
{

}

void PBRDemo::OnKeyDown(WPARAM vkCode)
{

}

void PBRDemo::OnKeyUp(WPARAM vkCode)
{

}