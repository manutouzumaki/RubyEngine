#include "FPSDemo.h"
#include "../RubyDebugProfiler.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vector>

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

FPSDemo::FPSDemo(HINSTANCE instance,
    UINT clientWidth,
    UINT clientHeight,
    const char* windowCaption,
    bool enable4xMsaa)
    : Ruby::App(instance, clientWidth, clientHeight, windowCaption, enable4xMsaa),
    mInputLayout(nullptr),
    mMesh()
{
    XMMATRIX identity = XMMatrixIdentity();
    DirectX::XMStoreFloat4x4(&mWorld, identity);
    DirectX::XMStoreFloat4x4(&mView, identity);
    DirectX::XMStoreFloat4x4(&mProj, identity);
}


FPSDemo::~FPSDemo()
{
    SAFE_DELETE(mCamera);

    SAFE_DELETE(mScene);

    SAFE_DELETE(mEnviromentMap);
    SAFE_DELETE(mIrradianceMap);

    SAFE_DELETE(mPinPongFrameBuffers[1]);
    SAFE_DELETE(mPinPongFrameBuffers[0]);
    SAFE_DELETE(mFrameBuffers[1]);
    SAFE_DELETE(mFrameBuffers[0]);
    SAFE_DELETE(mShadowMap);
    SAFE_DELETE(mBrdfMap);

    SAFE_DELETE(mMesh);
    SAFE_DELETE(mGunMesh);

    for (int i = 0; i < 4; ++i)
    {
        SAFE_RELEASE(mPbrTextures2D[i]);
        SAFE_RELEASE(mPbrSRVs[i]);
    }

    SAFE_RELEASE(mHdrSkyTexture2D);
    SAFE_RELEASE(mHdrSkySRV);

    SAFE_DELETE(mPbrColorEffect);
    SAFE_DELETE(mPbrTextureEffect);
    SAFE_DELETE(mDepthEffect);
    SAFE_DELETE(mHdrEffect);
    SAFE_DELETE(mBlurEffect);
    SAFE_DELETE(mCubemapEffect);
    SAFE_DELETE(mConvoluteEffect);
    SAFE_DELETE(mSkyEffect);
    SAFE_DELETE(mBrdfEffect);

    SAFE_RELEASE(mInputLayout);
    SAFE_RELEASE(mRasterizerStateBackCull);
    SAFE_RELEASE(mRasterizerStateFrontCull);

}

static float angle = 0.0f;
static XMVECTOR lightPos;
static float lightDist = 8.0f;
static float lightHeight = 4.0f;

#include <algorithm>

// SplitGeometry multithreaded
void FPSDemo::SplitGeometryFast(Ruby::OctreeNode<Ruby::SceneStaticObject>* node)
{
    if (node->pChild[0] == nullptr)
    {
        Ruby::SplitGeometryEntry data;
        data.mMesh = mMesh;
        data.pNode = node;
        data.mDevice = mDevice;
        mQueue.AddEntry(&data);
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            SplitGeometryFast(node->pChild[i]);
        }
    }
}

bool FPSDemo::Init()
{
    if (!Ruby::App::Init())
        return false;

    HANDLE threadIds[RUBY_MAX_THREAD_COUNT];
    for (int i = 0; i < RUBY_MAX_THREAD_COUNT; ++i)
    {
        DWORD threadId;
        threadIds[i] = CreateThread(0, 0, ThreadProc, &mQueue, 0, &threadId);
    }

    DebugProfilerBegin(Init);

    D3D11_RASTERIZER_DESC fillRasterizerNoneDesc = {};
    fillRasterizerNoneDesc.FillMode = D3D11_FILL_SOLID;
    fillRasterizerNoneDesc.CullMode = D3D11_CULL_BACK;
    fillRasterizerNoneDesc.DepthClipEnable = true;
    mDevice->CreateRasterizerState(&fillRasterizerNoneDesc, &mRasterizerStateBackCull);

    fillRasterizerNoneDesc.FillMode = D3D11_FILL_SOLID;
    fillRasterizerNoneDesc.CullMode = D3D11_CULL_FRONT;
    fillRasterizerNoneDesc.DepthClipEnable = true;
    mDevice->CreateRasterizerState(&fillRasterizerNoneDesc, &mRasterizerStateFrontCull);

    mMesh = new Ruby::Mesh(mDevice, "./assets/level3.gltf", "./assets/level3.bin", "./");
    mGunMesh = new Ruby::Mesh(mDevice, "./assets/gun/gun.gltf", "./assets/gun/gun.bin", "./");
    
    /*
    
    TODO IMPORTANT(MANUTO):
        - FIX memory leak in the octree

    */

    /*
    for (int i = 0; i < mMesh->Indices.size(); i += 3)
    {
        XMFLOAT3 a = mMesh->Vertices[mMesh->Indices[i + 0]].Position;
        XMFLOAT3 b = mMesh->Vertices[mMesh->Indices[i + 1]].Position;
        XMFLOAT3 c = mMesh->Vertices[mMesh->Indices[i + 2]].Position;
        Ruby::Physics::Triangle triangle;
        triangle.a = Ruby::Physics::Vector3(a.x, a.y, a.z);
        triangle.b = Ruby::Physics::Vector3(b.x, b.y, b.z);
        triangle.c = Ruby::Physics::Vector3(c.x, c.y, c.z);
        mTriangles.push_back(triangle);
    }
    */

    XMFLOAT3 min, max;
    mMesh->GetBoundingBox(min, max);

    float meshWidth = max.x - min.x;
    float meshDepth = max.z - min.z;

    float centerX = 0.008616f;
    float centerZ = -0.024896f;

    mScene = new Ruby::Scene(XMFLOAT3(centerX, 0.0f, centerZ), meshDepth * 0.5f, 2);

    Ruby::Octree<Ruby::SceneStaticObject>* octree = &mScene->mStaticObjectTree;
    
    DebugProfilerBegin(SplitGeometryFast);
    SplitGeometryFast(octree->mRoot);
    mQueue.CompleteAllWork();

    // kills the threads
    for (int i = 0; i < RUBY_MAX_THREAD_COUNT; ++i)
    {
        TerminateThread(threadIds[i], 0);
        CloseHandle(threadIds[i]);
    }

    DebugProfilerEnd(SplitGeometryFast);

    OutputDebugStringA("Mesh split end!!\n");


    mCamera = new Ruby::FPSCamera(XMFLOAT3(0, 1, 0), XMFLOAT3(0, 0, 0), 32.0f);

    DebugProfilerBegin(HDRTexture);
    // Load HDR Texture
    {
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        //float* data = stbi_loadf("./assets/newport_loft.hdr", &width, &height, &nrComponents, 0);
        float* data = stbi_loadf("./assets/sky.hdr", &width, &height, &nrComponents, 0);
        if (data)
        {
            // Create HDR Texture2D
            D3D11_TEXTURE2D_DESC texDesc;
            texDesc.Width = width;
            texDesc.Height = height;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            texDesc.SampleDesc.Count = 1;
            texDesc.SampleDesc.Quality = 0;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = 0;

            std::vector<XMFLOAT4> colors;
            for (int i = 0; i < width * height * 3; i += 3)
            {
                XMFLOAT4 color{ data[i + 0], data[i + 1], data[i + 2], 1.0f };
                colors.push_back(color);
            }

            D3D11_SUBRESOURCE_DATA initData{};
            initData.pSysMem = colors.data();
            initData.SysMemPitch = width * (sizeof(float) * 4);
            if (FAILED(mDevice->CreateTexture2D(&texDesc, &initData, &mHdrSkyTexture2D)))
            {
                OutputDebugString("Error Creating HDR Texture2D\n");
            }

            // Create Shader Resource View For HDR Texture
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            if (FAILED(mDevice->CreateShaderResourceView(mHdrSkyTexture2D, &srvDesc, &mHdrSkySRV)))
            {
                OutputDebugString("Error Creating HDR Shader Resource View\n");
            }

            OutputDebugStringA("HDR Texture Loaded\n");

            stbi_image_free(data);
        }
    }
    DebugProfilerEnd(HDRTexture);

    // Load the geometry for the sky (cubemap)
    {
        Ruby::MeshData box;
        Ruby::GeometryGenerator geoGen;
        geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

        std::vector<Ruby::MeshGeometry::Subset> subsetTable;
        Ruby::MeshGeometry::Subset subset{};
        subset.IndexStart = 0;
        subset.IndexCount = box.Indices.size();
        subsetTable.push_back(subset);

        mSky.SetVertices(mDevice, box.Vertices.data(), box.Vertices.size());
        mSky.SetIndices(mDevice, box.Indices.data(), box.Indices.size());
        mSky.SetSubsetTable(subsetTable);
    }

    mShadowMap = new ShadowMap(mDevice, 1024, 1024);

    mFrameBuffers[0] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mFrameBuffers[1] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mPinPongFrameBuffers[0] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mPinPongFrameBuffers[1] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);


    mBrdfMap = new Ruby::FrameBuffer(mDevice, 512, 512, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mEnviromentMap = new Ruby::CubeFrameBuffer(mDevice, 512, 512, DXGI_FORMAT_R32G32B32A32_FLOAT, 5);
    mIrradianceMap = new Ruby::CubeFrameBuffer(mDevice, 512, 512, DXGI_FORMAT_R32G32B32A32_FLOAT, 1);



    DebugProfilerBegin(Effects);

    mPbrColorEffect = new Ruby::PbrColorEffect(mDevice);
    mPbrTextureEffect = new Ruby::PbrTextureEffect(mDevice);
    mDepthEffect = new Ruby::DepthEffect(mDevice);
    mHdrEffect = new Ruby::HdrEffect(mDevice);
    mBlurEffect = new Ruby::BlurEffect(mDevice);
    mCubemapEffect = new Ruby::CubemapEffect(mDevice);
    mConvoluteEffect = new Ruby::ConvoluteEffect(mDevice);
    mSkyEffect = new Ruby::SkyEffect(mDevice);
    mBrdfEffect = new Ruby::BrdfEffect(mDevice);

    DebugProfilerEnd(Effects);

    // set the vertex Layout
    D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3DX11_PASS_DESC passDesc;
    mPbrColorEffect->GetTechnique()->GetPassByIndex(0)->GetDesc(&passDesc);
    HRESULT result = mDevice->CreateInputLayout(vertexDesc, 4, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout);

    // set proj matrices
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 0.001f, 100.0f);
    DirectX::XMStoreFloat4x4(&mProj, P);

    lightPos = XMVectorSet(0.0f, lightHeight, lightDist, 1.0f);

    mDirLight.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    XMVECTOR lightDir = XMVector3Normalize(lightPos);
    DirectX::XMStoreFloat3(&mDirLight.Direction, lightDir);
    mPbrColorEffect->mDirLight->SetRawValue(&mDirLight, 0, sizeof(Ruby::Pbr::DirectionalLight));
    mPbrTextureEffect->mDirLight->SetRawValue(&mDirLight, 0, sizeof(Ruby::Pbr::DirectionalLight));

    // Point light--position is changed every frame to animate in UpdateScene function.
    mPointLight.Color = XMFLOAT4(100.0f, 100.0f, 100.0f, 1.0f);
    mPointLight.Position = XMFLOAT3(4.0f, 6.0f, -10.0f);
    mPbrColorEffect->mPointLight->SetRawValue(&mPointLight, 0, sizeof(Ruby::Pbr::PointLight));
    mPbrTextureEffect->mPointLight->SetRawValue(&mPointLight, 0, sizeof(Ruby::Pbr::PointLight));


    DebugProfilerBegin(PBRTextures);

    D3D11_QUERY_DESC queryDesc{};
    ID3D11Query* pQuery = nullptr;
    queryDesc.Query = D3D11_QUERY_EVENT;
    queryDesc.MiscFlags = 0;
    mDevice->CreateQuery(&queryDesc, &pQuery);
    
    // Create Textures for PBR rendering
    {

        // Render CubeMap to Frame Buffer, we to this just one time at startup
        {
            XMMATRIX captureProj = XMMatrixPerspectiveFovLH((90.0f / 180.0f) * XM_PI, 1.0f, 0.1f, 10.0f);
            XMMATRIX captureViews[] = {
                XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet(1.0f,  0.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f)),
                XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet(-1.0f,  0.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f)),
                XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet(0.0f,  1.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)),
                XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 0.0f,  1.0f, 0.0f)),
                XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f)),
                XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet(0.0f,  0.0f, -1.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f))
            };

            mCubemapEffect->mCubeMap->SetResource(mHdrSkySRV);
            mConvoluteEffect->mCubeMap->SetResource(mHdrSkySRV);

            mImmediateContext->IASetInputLayout(mInputLayout);
            mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            mImmediateContext->RSSetViewports(1, &mViewport);
            mImmediateContext->RSSetState(mRasterizerStateFrontCull);
            
            // create enviroment map
            {
                // Set the viewport transform.
                float CurrentWidht = 512.0f;
                float CurrentHeight = 512.0f;

                for (int mipIndex = 0; mipIndex < mEnviromentMap->GetMipCount(); ++mipIndex)
                {
                    mViewport.TopLeftX = 0;
                    mViewport.TopLeftY = 0;
                    mViewport.Width = CurrentWidht;
                    mViewport.Height = CurrentHeight;
                    mViewport.MinDepth = 0.0f;
                    mViewport.MaxDepth = 1.0f;
                    mImmediateContext->RSSetViewports(1, &mViewport);


                    float roughness = (float)mipIndex / (float)(mEnviromentMap->GetMipCount() - 1);
                    mCubemapEffect->mRoughness->SetRawValue(&roughness, 0, sizeof(float));

                    CurrentWidht *= 0.5f;
                    CurrentHeight *= 0.5f;

                    for (int i = 0; i < 6; ++i)
                    {
                        ID3D11RenderTargetView* renderTarget[1] = { mEnviromentMap->GetRenderTargetView(i, mipIndex) };
                        mImmediateContext->OMSetRenderTargets(1, renderTarget, mEnviromentMap->GetDepthStencilView(mipIndex));

                        XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
                        mImmediateContext->ClearRenderTargetView(mEnviromentMap->GetRenderTargetView(i, mipIndex), (float*)&clearColor);
                        mImmediateContext->ClearDepthStencilView(mEnviromentMap->GetDepthStencilView(mipIndex), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

                        XMMATRIX viewProj = captureViews[i] * captureProj;

                        D3DX11_TECHNIQUE_DESC techDesc;
                        mCubemapEffect->GetTechnique()->GetDesc(&techDesc);
                        for (UINT p = 0; p < techDesc.Passes; ++p)
                        {
                            mCubemapEffect->mWorldViewProj->SetMatrix(reinterpret_cast<float*>(&viewProj));
                            mCubemapEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, mImmediateContext);
                            mSky.Draw(mImmediateContext, 0);
                        }
                    }
                }
            }
            
            
            // create irradiance map
            {
                mViewport.TopLeftX = 0;
                mViewport.TopLeftY = 0;
                mViewport.Width = 512.0f;
                mViewport.Height = 512.0f;
                mViewport.MinDepth = 0.0f;
                mViewport.MaxDepth = 1.0f;
                mImmediateContext->RSSetViewports(1, &mViewport);

                for (int i = 0; i < 6; ++i)
                {
                    ID3D11RenderTargetView* renderTarget[1] = { mIrradianceMap->GetRenderTargetView(i, 0) };
                    mImmediateContext->OMSetRenderTargets(1, renderTarget, mIrradianceMap->GetDepthStencilView(0));

                    XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
                    mImmediateContext->ClearRenderTargetView(mIrradianceMap->GetRenderTargetView(i, 0), (float*)&clearColor);
                    mImmediateContext->ClearDepthStencilView(mIrradianceMap->GetDepthStencilView(0), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

                    XMMATRIX viewProj = captureViews[i] * captureProj;

                    D3DX11_TECHNIQUE_DESC techDesc;
                    mConvoluteEffect->GetTechnique()->GetDesc(&techDesc);
                    for (UINT p = 0; p < techDesc.Passes; ++p)
                    {
                        mConvoluteEffect->mWorldViewProj->SetMatrix(reinterpret_cast<float*>(&viewProj));
                        mConvoluteEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, mImmediateContext);
                        mSky.Draw(mImmediateContext, 0);
                    }
                }
            }
            

            mImmediateContext->RSSetState(mRasterizerStateBackCull);

            mViewport.TopLeftX = 0;
            mViewport.TopLeftY = 0;
            mViewport.Width = mClientWidth;
            mViewport.Height = mClientHeight;
            mViewport.MinDepth = 0.0f;
            mViewport.MaxDepth = 1.0f;
            mImmediateContext->RSSetViewports(1, &mViewport);
        }

        // Brdf Texture Generation
        {
            mViewport.TopLeftX = 0;
            mViewport.TopLeftY = 0;
            mViewport.Width = 512.0f;
            mViewport.Height = 512.0f;
            mViewport.MinDepth = 0.0f;
            mViewport.MaxDepth = 1.0f;
            mImmediateContext->RSSetViewports(1, &mViewport);
            ID3D11RenderTargetView* renderTarget[1] = { mBrdfMap->GetRenderTargetView() };
            mImmediateContext->OMSetRenderTargets(1, renderTarget, 0);

            XMVECTORF32 clearColor = { 0.0f, 1.0f, 1.0f, 1.0f };
            mImmediateContext->ClearRenderTargetView(mBrdfMap->GetRenderTargetView(), (float*)&clearColor);
            {
                D3DX11_TECHNIQUE_DESC techDesc;
                mBrdfEffect->GetTechnique()->GetDesc(&techDesc);
                for (UINT p = 0; p < techDesc.Passes; ++p)
                {
                    mBrdfEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mImmediateContext->Draw(6, 0);
                }
            }
            mViewport.TopLeftX = 0;
            mViewport.TopLeftY = 0;
            mViewport.Width = mClientWidth;
            mViewport.Height = mClientHeight;
            mViewport.MinDepth = 0.0f;
            mViewport.MaxDepth = 1.0f;
            mImmediateContext->RSSetViewports(1, &mViewport);
        }

        
        // Set the cubemap to the shader
        mPbrColorEffect->mIrradianceMap->SetResource(mIrradianceMap->GetShaderResourceView());
        mPbrColorEffect->mPrefilteredColor->SetResource(mEnviromentMap->GetShaderResourceView());
        mPbrColorEffect->mBrdfLUT->SetResource(mBrdfMap->GetShaderResourceView());

        mPbrTextureEffect->mIrradianceMap->SetResource(mIrradianceMap->GetShaderResourceView());
        mPbrTextureEffect->mPrefilteredColor->SetResource(mEnviromentMap->GetShaderResourceView());
        mPbrTextureEffect->mBrdfLUT->SetResource(mBrdfMap->GetShaderResourceView());

        mSkyEffect->mCubeMap->SetResource(mEnviromentMap->GetShaderResourceView());
        
    }

    mImmediateContext->End(pQuery);
    BOOL data = false;
    
    // wait until the GPU finish procesing the commands
   while (data == 0 && mImmediateContext->GetData(pQuery, &data, sizeof(BOOL), 0) != S_OK);
    

    pQuery->Release();

    DebugProfilerEnd(PBRTextures);

    OutputDebugStringA("PBR texture Loaded!!\n");


    // load PBR Textures
    {
        const char* texturesPath[4] = {
            "./assets/gun/A.png",
            "./assets/gun/M.png",
            "./assets/gun/R.png",
            "./assets/gun/N.png",
        };

        for (int i = 0; i < 4; ++i)
        {
            stbi_set_flip_vertically_on_load(false);
            int width, height, nrComponents;
            unsigned char* data = stbi_load(texturesPath[i], &width, &height, &nrComponents, 0);
            if (data)
            {
                // Create Texture2D
                D3D11_TEXTURE2D_DESC texDesc;
                texDesc.Width = width;
                texDesc.Height = height;
                texDesc.MipLevels = 1;
                texDesc.ArraySize = 1;
                texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                texDesc.SampleDesc.Count = 1;
                texDesc.SampleDesc.Quality = 0;
                texDesc.Usage = D3D11_USAGE_DEFAULT;
                texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                texDesc.CPUAccessFlags = 0;
                texDesc.MiscFlags = 0;

                D3D11_SUBRESOURCE_DATA initData{};
                initData.pSysMem = data;
                initData.SysMemPitch = width * sizeof(unsigned int);
                if (FAILED(mDevice->CreateTexture2D(&texDesc, &initData, &mPbrTextures2D[i])))
                {
                    OutputDebugString("Error Creating PBR Texture2D\n");
                }

                // Create Shader Resource View For HDR Texture
                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
                srvDesc.Texture2D.MostDetailedMip = 0;
                if (FAILED(mDevice->CreateShaderResourceView(mPbrTextures2D[i], &srvDesc, &mPbrSRVs[i])))
                {
                    OutputDebugString("Error Creating PBR Shader Resource View\n");
                }

                OutputDebugStringA("PBR Textures Loaded\n");

                stbi_image_free(data);
            }
        }
    }

    mPbrTextureEffect->mAlbedoMap->SetResource(mPbrSRVs[0]);
    mPbrTextureEffect->mMetallicMap->SetResource(mPbrSRVs[1]);
    mPbrTextureEffect->mRoughnessMap->SetResource(mPbrSRVs[2]);
    mPbrTextureEffect->mNormalMap->SetResource(mPbrSRVs[3]);

    OutputDebugStringA("Gun Textures Ended\n");


    DebugProfilerEnd(Init);

    Ruby::DebugProfiler::PrintData();

    return true;
}

void FPSDemo::OnResize()
{
    Ruby::App::OnResize();
    // set proj matrices
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 0.001f, 100.0f);
    DirectX::XMStoreFloat4x4(&mProj, P);

    if (mFrameBuffers[0])
        mFrameBuffers[0]->Resize(mDevice, mClientWidth, mClientHeight);

    if (mFrameBuffers[1])
        mFrameBuffers[1]->Resize(mDevice, mClientWidth, mClientHeight);

    if (mPinPongFrameBuffers[0])
        mPinPongFrameBuffers[0]->Resize(mDevice, mClientWidth, mClientHeight);

    if (mPinPongFrameBuffers[1])
        mPinPongFrameBuffers[1]->Resize(mDevice, mClientWidth, mClientHeight);
}

void FPSDemo::UpdateScene()
{
    float dt = mTimer.DeltaTime();

    if (mInput.KeyIsDown('W'))
    {
        mCamera->MoveForward(dt);
    }
    if (mInput.KeyIsDown('S'))
    {
        mCamera->MoveBackward(dt);
    }
    if (mInput.KeyIsDown('A'))
    {
        mCamera->MoveLeft(dt);
    }
    if (mInput.KeyIsDown('D'))
    {
        mCamera->MoveRight(dt);
    }
    if (mInput.KeyIsDown('R'))
    {
        mCamera->MoveUp(dt);
    }
    if (mInput.KeyIsDown('F'))
    {
        mCamera->MoveDown(dt);
    }

    if (mInput.MouseButtonJustDown(1))
    {
        ShowCursor(false);
    }
    if (mInput.MouseButtonJustUp(1))
    {
        ShowCursor(true);
    }

    if (mInput.MouseButtonIsDown(1))
    {
        int deltaX = mInput.MousePosX() - mInput.MouseLastPosX();
        int deltaY = mInput.MousePosY() - mInput.MouseLastPosY();

        mCamera->MouseMove((float)deltaX * 0.001f, (float)deltaY * 0.001f);

        SetCursorPos(mWindowX + mClientWidth / 2, mWindowY + mClientHeight / 2);
        mInput.mCurrent.mouseX = mClientWidth / 2;
        mInput.mCurrent.mouseY = mClientHeight / 2;
        mInput.mLast.mouseX = mClientWidth / 2;
        mInput.mLast.mouseY = mClientHeight / 2;

    }

    std::vector<Ruby::OctreeNode<Ruby::SceneStaticObject>*> queryResult;
    mScene->mStaticObjectTree.mRoot->Query(mCamera->GetPosition(), XMFLOAT3(4, 4, 4), queryResult);

    std::vector<Ruby::Physics::Triangle> triangles;
    for (int i = 0; i < queryResult.size(); ++i)
    {
        Ruby::SceneStaticObject object = queryResult[i]->pObjList[0];
        triangles.insert(triangles.end(), object.mTriangles.begin(), object.mTriangles.end());
    }

    mCamera->Update(dt, triangles.data(), triangles.size());



    angle += 0.5f * dt;

    XMMATRIX world = XMMatrixRotationY(angle) * XMMatrixRotationX(angle * 2.0f);
    DirectX::XMStoreFloat4x4(&mWorld, world);

    XMFLOAT3 targetDir = XMFLOAT3(0, 0, 0);

    // Build the view matrix.
    XMFLOAT3 eyePos = mCamera->GetViewPosition();

    XMVECTOR pos = XMVectorSet(eyePos.x, eyePos.y, eyePos.z, 1.0f);
    XMVECTOR target = XMVectorSet(targetDir.x, targetDir.y, targetDir.z, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX V = mCamera->GetView();
    DirectX::XMStoreFloat4x4(&mView, V);
    mPbrColorEffect->mEyePosW->SetRawValue(&eyePos, 0, sizeof(XMFLOAT3));
    mPbrTextureEffect->mEyePosW->SetRawValue(&eyePos, 0, sizeof(XMFLOAT3));


    static float timer = 0.0f;

    timer += dt;
    if (timer >= 4.0f)
    {
        timer = 0.0f;
    }
    mSkyEffect->mTimer->SetRawValue(&timer, 0, sizeof(float));
}

void FPSDemo::DrawScene()
{
    mImmediateContext->IASetInputLayout(mInputLayout);
    mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Query the octree
    std::vector<Ruby::OctreeNode<Ruby::SceneStaticObject>*> queryResult;
    mScene->mStaticObjectTree.mRoot->Query(mCamera->GetPosition(), XMFLOAT3(16, 16, 16), queryResult);

    mShadowMap->BindDsvAndSetNullRenderTarget(mImmediateContext);

    // render the scene to the depth buffer only for shadow calculations
    {
        XMVECTOR targetPos = XMVectorZero();
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);


        XMMATRIX V = XMMatrixLookAtLH(lightPos, targetPos, up);

        XMMATRIX P = XMMatrixOrthographicOffCenterLH(-20, 20, -20, 20, 0.0001f, 45.0f);

        XMMATRIX lightSpaceMatrix = V * P;
        mDepthEffect->mLightSpaceMatrix->SetMatrix(reinterpret_cast<float*>(&lightSpaceMatrix));
        mPbrColorEffect->mLightSpaceMatrix->SetMatrix(reinterpret_cast<float*>(&lightSpaceMatrix));
        mPbrTextureEffect->mLightSpaceMatrix->SetMatrix(reinterpret_cast<float*>(&lightSpaceMatrix));


        D3DX11_TECHNIQUE_DESC techDesc;
        mDepthEffect->GetTechnique()->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
            for (int index = 0; index < queryResult.size(); ++index)
            {
                XMMATRIX world = XMMatrixTranslation(0, 0, 0);
                mDepthEffect->mWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::SceneStaticObject object = queryResult[index]->pObjList.back();
                for (UINT i = 0; i < object.mMesh->Mat.size(); ++i)
                {
                    mDepthEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    object.mMesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
        }
    }


    ID3D11RenderTargetView* frameBuffers[2] = { mFrameBuffers[0]->GetRenderTargetView(), mFrameBuffers[1]->GetRenderTargetView() };
    mImmediateContext->OMSetRenderTargets(2, frameBuffers, mDepthStencilView);

    mImmediateContext->RSSetViewports(1, &mViewport);

    mPbrColorEffect->mShadowMap->SetResource(mShadowMap->mDepthMapSRV);
    mPbrTextureEffect->mShadowMap->SetResource(mShadowMap->mDepthMapSRV);


    XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.001f, 1.0f };
    mImmediateContext->ClearRenderTargetView(mFrameBuffers[0]->GetRenderTargetView(), (float*)&clearColor);
    mImmediateContext->ClearRenderTargetView(mFrameBuffers[1]->GetRenderTargetView(), (float*)&clearColor);
    mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    {
        // Build the view matrix.

        // Set constants
        XMMATRIX viewProj = XMLoadFloat4x4(&mView) * XMLoadFloat4x4(&mProj);
        D3DX11_TECHNIQUE_DESC techDesc;
        mPbrColorEffect->GetTechnique()->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
            XMMATRIX world = XMMatrixTranslation(0, 0, 0);
            XMMATRIX worldInvTranspose = InverseTranspose(world);
            XMMATRIX worldViewProj = world * viewProj;
            mPbrColorEffect->mWorld->SetMatrix(reinterpret_cast<float*>(&world));
            mPbrColorEffect->mWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
            mPbrColorEffect->mWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
            for (int index = 0; index < queryResult.size(); ++index)
            {
                Ruby::SceneStaticObject object = queryResult[index]->pObjList.back();
                for (UINT i = 0; i < object.mMesh->Mat.size(); ++i)
                {
                    mPbrColorEffect->mMaterial->SetRawValue(&object.mMesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mPbrColorEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    object.mMesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            
            XMFLOAT3 camPos = mCamera->GetPosition();
            XMFLOAT3 camRot = mCamera->GetRotation();
            XMFLOAT3 camDir = mCamera->GetViewDirection();
            XMFLOAT3 camRight = mCamera->GetViewRight();
            XMFLOAT3 camUp = mCamera->GetViewUp();

            camPos.x -= camDir.x * 0.4f;
            camPos.y -= camDir.y * 0.4f;
            camPos.z -= camDir.z * 0.4f;

            camPos.x += camRight.x * 0.3f;
            camPos.y += camRight.y * 0.3f;
            camPos.z += camRight.z * 0.3f;

            camPos.x += camUp.x * 0.2f;
            camPos.y += camUp.y * 0.2f;
            camPos.z += camUp.z * 0.2f;


            world = XMMatrixScaling(0.009f, 0.009f, 0.009f) *
                XMMatrixRotationY(XM_PI - 0.035f) *
                XMMatrixRotationX(camRot.x) *
                XMMatrixRotationY(camRot.y) *
                XMMatrixRotationZ(camRot.z) *
                XMMatrixTranslation(camPos.x, camPos.y, camPos.z);
            worldInvTranspose = InverseTranspose(world);
            worldViewProj = world * viewProj;
            mPbrTextureEffect->mWorld->SetMatrix(reinterpret_cast<float*>(&world));
            mPbrTextureEffect->mWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
            mPbrTextureEffect->mWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

            Ruby::Mesh* mesh = mGunMesh;
            for (UINT i = 0; i < mesh->Mat.size(); ++i)
            {
                mPbrTextureEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, mImmediateContext);
                mesh->ModelMesh.Draw(mImmediateContext, i);
            }
        }

        mSkyEffect->GetTechnique()->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
            XMFLOAT3 eyePos = mCamera->GetViewPosition();
            XMMATRIX world = XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);
            XMMATRIX worldViewProj = world * viewProj;
            mSkyEffect->mWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
            mSkyEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, mImmediateContext);
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
                                                     mPinPongFrameBuffers[1]->GetRenderTargetView() };

        ID3D11ShaderResourceView* resourceViews[2] = { mPinPongFrameBuffers[0]->GetShaderResourceView(),
                                                       mPinPongFrameBuffers[1]->GetShaderResourceView() };
        for (unsigned int i = 0; i < amount; i++) {

            mImmediateContext->OMSetRenderTargets(1, &renderTargets[horizontal], 0);

            mBlurEffect->mHorizontal->SetRawValue(&horizontal, 0, sizeof(bool));
            mBlurEffect->mImage->SetResource(firstIteration ? mFrameBuffers[1]->GetShaderResourceView() : resourceViews[!horizontal]);

            D3DX11_TECHNIQUE_DESC techDesc;
            mBlurEffect->GetTechnique()->GetDesc(&techDesc);
            for (UINT p = 0; p < techDesc.Passes; ++p)
            {
                mBlurEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, mImmediateContext);
                mImmediateContext->Draw(6, 0);
            }

            horizontal = !horizontal;
            if (firstIteration)
                firstIteration = false;

            ID3D11ShaderResourceView* nullSRV[16] = { 0 };
            mImmediateContext->PSSetShaderResources(0, 16, nullSRV);
        }
    }

    static float timer = 0.0f;
    mHdrEffect->mTimer->SetRawValue(&timer, 0, sizeof(float));
    timer += mTimer.DeltaTime();
    mHdrEffect->mBackBuffer->SetResource(mFrameBuffers[0]->GetShaderResourceView());
    mHdrEffect->mBloomBuffer->SetResource(mPinPongFrameBuffers[1]->GetShaderResourceView());
    // Final render to quad2d
    {
        ID3D11RenderTargetView* renderTarget[1] = { mRenderTargetView };
        mImmediateContext->OMSetRenderTargets(1, renderTarget, 0);

        clearColor = { 0.69f, 0.34f, 0.34f, 1.0f };
        mImmediateContext->ClearRenderTargetView(mRenderTargetView, (float*)&clearColor);
        mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        {
            D3DX11_TECHNIQUE_DESC techDesc;
            mHdrEffect->GetTechnique()->GetDesc(&techDesc);
            for (UINT p = 0; p < techDesc.Passes; ++p)
            {
                mHdrEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, mImmediateContext);
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