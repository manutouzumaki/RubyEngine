#include "FPSDemo.h"

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


void FPSDemo::OnMouseDown(WPARAM btnState, int x, int y)
{

}
void FPSDemo::OnMouseUp(WPARAM btnState, int x, int y)
{

}
void FPSDemo::OnMouseMove(WPARAM btnState, int x, int y)
{

}
void FPSDemo::OnKeyDown(WPARAM vkCode)
{

}
void FPSDemo::OnKeyUp(WPARAM vkCode)
{

}

FPSDemo::FPSDemo(HINSTANCE instance,
    UINT clientWidth, 
    UINT clientHeight,
    const char* windowCaption,
    bool enable4xMsaa)
    : Ruby::App(instance, clientWidth, clientHeight, windowCaption, enable4xMsaa),
    mEffect(nullptr),
    mTechnique(nullptr),
    mFxWorldViewProj(nullptr),
    mInputLayout(nullptr),
    mMesh()
{
    XMMATRIX identity = XMMatrixIdentity();
    XMStoreFloat4x4(&mWorld, identity);
    XMStoreFloat4x4(&mView, identity);
    XMStoreFloat4x4(&mProj, identity);
}

FPSDemo::~FPSDemo()
{
    SAFE_DELETE(mPinPongFrameBuffers[1]);
    SAFE_DELETE(mPinPongFrameBuffers[0]);
    SAFE_DELETE(mFrameBuffers[1]);
    SAFE_DELETE(mFrameBuffers[0]);
    SAFE_DELETE(mShadowMap);

    SAFE_DELETE(mMesh[0]);
    SAFE_DELETE(mMesh[1]);
    SAFE_DELETE(mMesh[2]);
    SAFE_DELETE(mMesh[3]);
    SAFE_DELETE(mMesh[4]);
    SAFE_DELETE(mMesh[5]);

    SAFE_RELEASE(mCubeMapSRV);
    SAFE_RELEASE(mCubemapEffect);
    SAFE_RELEASE(mEffect);
    SAFE_RELEASE(mDepthEffect);
    SAFE_RELEASE(mHdrEffect);
    SAFE_RELEASE(mBlurEffect);
    SAFE_RELEASE(mInputLayout);
}

static float angle = 0.0f;
static XMVECTOR lightPos;
static float lightDist = 8.0f;
static float lightHeight = 4.0f;

#include <algorithm>

static const int gNrRows = 7;
static const int gNrCols = 7;
static const float gSpacing = 1.2f;


bool FPSDemo::Init()
{
    if (!Ruby::App::Init())
        return false;

    mMesh[0] = new Ruby::Mesh(mDevice, "./assets/level1.gltf", "./assets/level1.bin", "./");
    mMesh[1] = new Ruby::Mesh(mDevice, "./assets/sphere.gltf",   "./assets/sphere.bin",  "./");
    mMesh[2] = new Ruby::Mesh(mDevice, "./assets/mono.gltf",   "./assets/mono.bin",  "./");
    mMesh[3] = new Ruby::Mesh(mDevice, "./assets/level.gltf",  "./assets/level.bin", "./");
    mMesh[4] = new Ruby::Mesh(mDevice, "./assets/cube.gltf", "./assets/cube.bin", "./");
    mMesh[5] = new Ruby::Mesh(mDevice, "./assets/castel.gltf", "./assets/castel.bin", "./");

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

    // Load Cube map
    {
        HRESULT result = D3DX11CreateShaderResourceViewFromFileA(mDevice, "./assets/cubemap.dds", 0, 0, &mCubeMapSRV, 0);
        if (SUCCEEDED(result))
        {
            OutputDebugStringA("CubeMap Load succeeded\n");
        }
    }

    // Load the geometry for the sky (cubemap)
    {
        Ruby::MeshData sphere;
        Ruby::GeometryGenerator geoGen;
        geoGen.CreateSphere(1.0f, 30, 30, sphere);

        std::vector<Ruby::MeshGeometry::Subset> subsetTable;
        Ruby::MeshGeometry::Subset subset{};
        subset.VertexStart = 0;
        subset.VertexCount = sphere.Vertices.size();
        subset.IndexStart = 0;
        subset.IndexCount = sphere.Indices.size();
        subsetTable.push_back(subset);

        mSky.SetVertices(mDevice, sphere.Vertices.data(), sphere.Vertices.size());
        mSky.SetIndices(mDevice, sphere.Indices.data(), sphere.Indices.size());
        mSky.SetSubsetTable(subsetTable);
    }
    
    mShadowMap = new ShadowMap(mDevice, 1024, 1024);

    mFrameBuffers[0] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mFrameBuffers[1] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mPinPongFrameBuffers[0] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mPinPongFrameBuffers[1] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);



    // create the shaders and fx
    DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3D10_SHADER_DEBUG;
    shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
    
    // Create Color Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/pbrColor.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mTechnique = mEffect->GetTechniqueByName("ColorTech");
        mFxWorld = mEffect->GetVariableByName("gWorld")->AsMatrix();
        mFxWorldInvTranspose = mEffect->GetVariableByName("gWorldInvTranspose")->AsMatrix();
        mFxWorldViewProj = mEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mFxMaterial = mEffect->GetVariableByName("gMaterial");
        mFxDirLight = mEffect->GetVariableByName("gDirLight");
        mFxPointLight = mEffect->GetVariableByName("gPointLight");
        mFxEyePosW = mEffect->GetVariableByName("gEyePosW")->AsVector();
        mFxLightSpaceMatrix = mEffect->GetVariableByName("gLightSpaceMatrix")->AsMatrix();
        mFxShadowMap = mEffect->GetVariableByName("gShadowMap")->AsShaderResource();
        mFxCubeMap = mEffect->GetVariableByName("gCubeMap")->AsShaderResource();
    }
    // Create Depth Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/depth.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mDepthEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mDepthTechnique = mDepthEffect->GetTechniqueByName("DepthTech");
        mDepthFxWorld = mDepthEffect->GetVariableByName("gWorld")->AsMatrix();
        mDepthFxLightSpaceMatrix = mDepthEffect->GetVariableByName("gLightSpaceMatrix")->AsMatrix();
;
    }
    // Create HDR Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/hdr.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mHdrEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mHdrTechnique = mHdrEffect->GetTechniqueByName("HdrTech");
        mHdrBackBuffer = mHdrEffect->GetVariableByName("gBackBuffer")->AsShaderResource();
        mBloomBuffer = mHdrEffect->GetVariableByName("gBloomBuffer")->AsShaderResource();
    }
    // Create Blur Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/blur.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mBlurEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mBlurTechnique = mBlurEffect->GetTechniqueByName("BlurTech");
        mBlurFxHorizontal = mBlurEffect->GetVariableByName("horizontal");
        mBlurFxImage = mBlurEffect->GetVariableByName("gImage")->AsShaderResource();
    }

    // Create Cubemap Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/cubemap.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mCubemapEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mCubemapTechnique = mCubemapEffect->GetTechniqueByName("CubeMapTech");
        mCubemapWorldViewProj = mCubemapEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mCubeMap = mCubemapEffect->GetVariableByName("gCubeMap")->AsShaderResource();
    }


    // set the vertex Layout
    D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3DX11_PASS_DESC passDesc;
    mTechnique->GetPassByIndex(0)->GetDesc(&passDesc);
    HRESULT result = mDevice->CreateInputLayout(vertexDesc, 4, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout);

    // set proj matrices
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

    // Build the view matrix.
    XMFLOAT3 eyePos = XMFLOAT3(10.0f, 20.0f, -10.0f);
    //XMFLOAT3 eyePos = XMFLOAT3(0, 0.0f, 6.0f);
    XMVECTOR pos = XMVectorSet(eyePos.x, eyePos.y, eyePos.z, 0.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, V);

    mFxEyePosW->SetRawValue(&eyePos, 0, sizeof(XMFLOAT3));

    lightPos = XMVectorSet(0.0f, lightHeight, lightDist, 1.0f);

    mDirLight.Color = XMFLOAT4(5.0f, 2.5f, 0.5f, 1.0f);
    XMVECTOR lightDir = XMVector3Normalize(lightPos);
    XMStoreFloat3(&mDirLight.Direction, lightDir);
    mFxDirLight->SetRawValue(&mDirLight, 0, sizeof(Ruby::Pbr::DirectionalLight));

    // Point light--position is changed every frame to animate in UpdateScene function.
    mPointLight.Color = XMFLOAT4(100.0f, 100.0f, 100.0f, 1.0f);
    mPointLight.Position = XMFLOAT3(4.0f, 6.0f, -10.0f);
    mFxPointLight->SetRawValue(&mPointLight, 0, sizeof(Ruby::Pbr::PointLight));

    // Set the cubemap to the shader
    mFxCubeMap->SetResource(mCubeMapSRV);
    mCubeMap->SetResource(mCubeMapSRV);

    return true;
}

void FPSDemo::OnResize()
{
    Ruby::App::OnResize();
    // set proj matrices
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

    if(mFrameBuffers[0])
        mFrameBuffers[0]->Resize(mDevice, mClientWidth, mClientHeight);

    if (mFrameBuffers[1])
        mFrameBuffers[1]->Resize(mDevice, mClientWidth, mClientHeight);

    if (mPinPongFrameBuffers[0])
        mPinPongFrameBuffers[0]->Resize(mDevice, mClientWidth, mClientHeight);

    if (mPinPongFrameBuffers[1])
        mPinPongFrameBuffers[1]->Resize(mDevice, mClientWidth, mClientHeight);
}

void FPSDemo::UpdateScene(float dt)
{
    angle += 0.5f * dt;

    XMMATRIX world = XMMatrixRotationY(angle) * XMMatrixRotationX(angle * 2.0f);
    XMStoreFloat4x4(&mWorld, world);

    XMVECTOR target = XMVectorZero();

    // Build the view matrix.
    XMFLOAT3 eyePos = XMFLOAT3(cosf(angle) * 20.0f, 2.0f, sinf(angle) * 20.0f);
    //XMFLOAT3 eyePos = XMFLOAT3(0, 0.0f, 6.0f);
    XMVECTOR pos = XMVectorSet(eyePos.x, eyePos.y, eyePos.z, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, V);
    mFxEyePosW->SetRawValue(&eyePos, 0, sizeof(XMFLOAT3));

    //mPointLight.Position = XMFLOAT3(sinf(angle*2) * 5.0f, 6, cosf(angle) * 5.0f);
    //mFxPointLight->SetRawValue(&mPointLight, 0, sizeof(Ruby::Pbr::PointLight));

}



void FPSDemo::DrawScene()
{
    mImmediateContext->IASetInputLayout(mInputLayout);
    mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mShadowMap->BindDsvAndSetNullRenderTarget(mImmediateContext);
    
    // render the scene to the depth buffer only for shadow calculations
    {
        XMVECTOR targetPos = XMVectorZero();
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);


        XMMATRIX V = XMMatrixLookAtLH(lightPos, targetPos, up);

        XMMATRIX P = XMMatrixOrthographicOffCenterLH(-20, 20, -20, 20, 0.0001f, 45.0f);

        XMMATRIX lightSpaceMatrix = V * P;
        mDepthFxLightSpaceMatrix->SetMatrix(reinterpret_cast<float*>(&lightSpaceMatrix));
        mFxLightSpaceMatrix->SetMatrix(reinterpret_cast<float*>(&lightSpaceMatrix));

        D3DX11_TECHNIQUE_DESC techDesc;
        mDepthTechnique->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
            // draw mesh 0
            {
                XMMATRIX world = XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(-2.0f, 3.99f, -2.0f);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[0];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 2
            {
                XMMATRIX world = XMMatrixTranslation(-1, 7, -2);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[2];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 3
            {
                XMMATRIX world = XMMatrixRotationY(XM_PIDIV4) * XMMatrixTranslation(8, 0.127f, -10.0f);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[3];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 4
            {
                XMMATRIX world = XMMatrixScaling(20.0f, 0.0000000001f, 40.0f) * XMMatrixTranslation(0, 0, 0);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[4];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 5
            {
                XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-10, 4.7, -25);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[5];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
        }
    }
    
    //
    // Restore the back and depth buffer to the OM stage.
    //

    ID3D11RenderTargetView* frameBuffers[2] = { mFrameBuffers[0]->GetRenderTargetView(), mFrameBuffers[1]->GetRenderTargetView()};
    mImmediateContext->OMSetRenderTargets(2, frameBuffers, mDepthStencilView);

    mImmediateContext->RSSetViewports(1, &mViewport);

    mFxShadowMap->SetResource(mShadowMap->mDepthMapSRV);

    XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.001f, 1.0f };
    mImmediateContext->ClearRenderTargetView(mFrameBuffers[0]->GetRenderTargetView(), (float*)&clearColor);
    mImmediateContext->ClearRenderTargetView(mFrameBuffers[1]->GetRenderTargetView(), (float*)&clearColor);
    mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    {
        // Build the view matrix.

        // Set constants
        XMMATRIX viewProj = XMLoadFloat4x4(&mView) * XMLoadFloat4x4(&mProj);

        D3DX11_TECHNIQUE_DESC techDesc;
        mTechnique->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
            // draw mesh 3
            {
                XMMATRIX world = XMMatrixRotationY(XM_PIDIV4) * XMMatrixTranslation(8, 0.127f, -10.0f);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = world * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[3];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 0
            {
                XMMATRIX world = XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(-2.0f, 3.99f, -2.0f);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = (world * XMMatrixTranslation(0, -2, 0)) * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[0];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 1
            {
                for (int row = 0; row < gNrRows; ++row)
                {
                    for (int col = 0; col < gNrCols; ++col)
                    {
                        XMMATRIX world = XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation((col - (gNrCols / 2)) * gSpacing,
                            (row - (gNrRows / 2)) * gSpacing + 4, 4.0f);
                        XMMATRIX worldInvTranspose = InverseTranspose(world);
                        XMMATRIX worldViewProj = world * viewProj;
                        mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                        mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                        mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                        Ruby::Mesh* mesh = mMesh[1];
                        for (UINT i = 0; i < mesh->Mat.size(); ++i)
                        {
                            mFxMaterial->SetRawValue(&mMaterials[row * gNrRows + col], 0, sizeof(Ruby::Pbr::Material));
                            mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                            mesh->ModelMesh.Draw(mImmediateContext, i);
                        }
                    }
                }
 
            }

            // draw mesh 2
            {
                XMMATRIX world = XMMatrixTranslation(-1, 7, -2);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = world * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[2];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }

            // draw mesh 4
            {
                //XMMATRIX world = XMMatrixScaling(10.0f, 0.0000000001f, 10.0f) * XMMatrixTranslation(0, 0, 0);
                XMMATRIX world = XMMatrixScaling(20.0f, 0.0000000001f, 40.0f) *  XMMatrixTranslation(0, 0, 0);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = world * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[4];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 5
            {
                XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-10, 4.7, -25);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = world * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[5];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
        }

        mCubemapTechnique->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
            XMFLOAT3 eyePos = XMFLOAT3(cosf(angle) * 20.0f, 2.0f, sinf(angle) * 20.0f);
            XMMATRIX world = XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);
            XMMATRIX worldViewProj = world * viewProj;
            mCubemapWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
            mCubemapTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
            mSky.Draw(mImmediateContext, 0);
        }

    }

    mImmediateContext->RSSetState(0);

    // Blur pass
    {
        bool horizontal = true, firstIteration = true;
        float horizontalFloat = 1.0;
        int amount = 10;
        ID3D11RenderTargetView* renderTargets[2] = { mPinPongFrameBuffers[0]->GetRenderTargetView(),
                                                     mPinPongFrameBuffers[1]->GetRenderTargetView()};

        ID3D11ShaderResourceView* resourceViews[2] = { mPinPongFrameBuffers[0]->GetShaderResourceView(),
                                                       mPinPongFrameBuffers[1]->GetShaderResourceView() };
        for (unsigned int i = 0; i < amount; i++) {
            
            mImmediateContext->OMSetRenderTargets(1, &renderTargets[horizontal], 0);

            mBlurFxHorizontal->SetRawValue(&horizontal, 0, sizeof(bool));
            mBlurFxImage->SetResource(firstIteration ? mFrameBuffers[1]->GetShaderResourceView() : resourceViews[!horizontal]);

            D3DX11_TECHNIQUE_DESC techDesc;
            mBlurTechnique->GetDesc(&techDesc);
            for (UINT p = 0; p < techDesc.Passes; ++p)
            {


                mBlurTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                mImmediateContext->Draw(6, 0);

            }

            horizontal = !horizontal;
            if (firstIteration)
                firstIteration = false;

            ID3D11ShaderResourceView* nullSRV[16] = { 0 };
            mImmediateContext->PSSetShaderResources(0, 16, nullSRV);
        }
    }


    mHdrBackBuffer->SetResource(mFrameBuffers[0]->GetShaderResourceView());
    mBloomBuffer->SetResource(mPinPongFrameBuffers[1]->GetShaderResourceView());
    // Final render to quad2d
    {
        ID3D11RenderTargetView* renderTarget[1] = { mRenderTargetView };
        mImmediateContext->OMSetRenderTargets(1, renderTarget, 0);

        clearColor = { 0.69f, 0.34f, 0.34f, 1.0f };
        mImmediateContext->ClearRenderTargetView(mRenderTargetView, (float*)&clearColor);
        mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        {
            D3DX11_TECHNIQUE_DESC techDesc;
            mHdrTechnique->GetDesc(&techDesc);
            for (UINT p = 0; p < techDesc.Passes; ++p)
            {
                mHdrTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                mImmediateContext->Draw(6, 0);
            }
        }
    }


    // Unbind shadow map as a shader input because we are going to render to it next frame.
    // The shadow might might be at any slot, so clear all slots.
    ID3D11ShaderResourceView* nullSRV[16] = { 0 };
    mImmediateContext->PSSetShaderResources(0, 16, nullSRV);

    mSwapChain->Present(1, 0);


}